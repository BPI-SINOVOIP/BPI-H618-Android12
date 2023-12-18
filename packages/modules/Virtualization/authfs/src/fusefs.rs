/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

use anyhow::Result;
use log::{debug, warn};
use std::collections::BTreeMap;
use std::convert::TryFrom;
use std::ffi::CStr;
use std::fs::OpenOptions;
use std::io;
use std::mem::MaybeUninit;
use std::option::Option;
use std::os::unix::io::AsRawFd;
use std::path::Path;
use std::time::Duration;

use fuse::filesystem::{
    Context, DirEntry, DirectoryIterator, Entry, FileSystem, FsOptions, SetattrValid,
    ZeroCopyReader, ZeroCopyWriter,
};
use fuse::mount::MountOption;

use crate::common::{divide_roundup, ChunkedSizeIter, CHUNK_SIZE};
use crate::file::{
    LocalFileReader, RandomWrite, ReadByChunk, RemoteFileEditor, RemoteFileReader,
    RemoteMerkleTreeReader,
};
use crate::fsverity::{VerifiedFileEditor, VerifiedFileReader};

const DEFAULT_METADATA_TIMEOUT: std::time::Duration = Duration::from_secs(5);

pub type Inode = u64;
type Handle = u64;

/// `FileConfig` defines the file type supported by AuthFS.
pub enum FileConfig {
    /// A file type that is verified against fs-verity signature (thus read-only). The file is
    /// backed by a local file. Debug only.
    LocalVerifiedReadonlyFile {
        reader: VerifiedFileReader<LocalFileReader, LocalFileReader>,
        file_size: u64,
    },
    /// A file type that is a read-only passthrough from a local file. Debug only.
    LocalUnverifiedReadonlyFile { reader: LocalFileReader, file_size: u64 },
    /// A file type that is verified against fs-verity signature (thus read-only). The file is
    /// served from a remote server.
    RemoteVerifiedReadonlyFile {
        reader: VerifiedFileReader<RemoteFileReader, RemoteMerkleTreeReader>,
        file_size: u64,
    },
    /// A file type that is a read-only passthrough from a file on a remote serrver.
    RemoteUnverifiedReadonlyFile { reader: RemoteFileReader, file_size: u64 },
    /// A file type that is initially empty, and the content is stored on a remote server. File
    /// integrity is guaranteed with private Merkle tree.
    RemoteVerifiedNewFile { editor: VerifiedFileEditor<RemoteFileEditor> },
}

struct AuthFs {
    /// Store `FileConfig`s using the `Inode` number as the search index.
    ///
    /// For further optimization to minimize the search cost, since Inode is integer, we may
    /// consider storing them in a Vec if we can guarantee that the numbers are small and
    /// consecutive.
    file_pool: BTreeMap<Inode, FileConfig>,

    /// Maximum bytes in the write transaction to the FUSE device. This limits the maximum size to
    /// a read request (including FUSE protocol overhead).
    max_write: u32,
}

impl AuthFs {
    pub fn new(file_pool: BTreeMap<Inode, FileConfig>, max_write: u32) -> AuthFs {
        AuthFs { file_pool, max_write }
    }

    fn get_file_config(&self, inode: &Inode) -> io::Result<&FileConfig> {
        self.file_pool.get(&inode).ok_or_else(|| io::Error::from_raw_os_error(libc::ENOENT))
    }
}

fn check_access_mode(flags: u32, mode: libc::c_int) -> io::Result<()> {
    if (flags & libc::O_ACCMODE as u32) == mode as u32 {
        Ok(())
    } else {
        Err(io::Error::from_raw_os_error(libc::EACCES))
    }
}

cfg_if::cfg_if! {
    if #[cfg(all(target_arch = "aarch64", target_pointer_width = "64"))] {
        fn blk_size() -> libc::c_int { CHUNK_SIZE as libc::c_int }
    } else {
        fn blk_size() -> libc::c_long { CHUNK_SIZE as libc::c_long }
    }
}

enum FileMode {
    ReadOnly,
    ReadWrite,
}

fn create_stat(ino: libc::ino_t, file_size: u64, file_mode: FileMode) -> io::Result<libc::stat64> {
    let mut st = unsafe { MaybeUninit::<libc::stat64>::zeroed().assume_init() };

    st.st_ino = ino;
    st.st_mode = match file_mode {
        // Until needed, let's just grant the owner access.
        FileMode::ReadOnly => libc::S_IFREG | libc::S_IRUSR,
        FileMode::ReadWrite => libc::S_IFREG | libc::S_IRUSR | libc::S_IWUSR,
    };
    st.st_dev = 0;
    st.st_nlink = 1;
    st.st_uid = 0;
    st.st_gid = 0;
    st.st_rdev = 0;
    st.st_size = libc::off64_t::try_from(file_size)
        .map_err(|_| io::Error::from_raw_os_error(libc::EFBIG))?;
    st.st_blksize = blk_size();
    // Per man stat(2), st_blocks is "Number of 512B blocks allocated".
    st.st_blocks = libc::c_longlong::try_from(divide_roundup(file_size, 512))
        .map_err(|_| io::Error::from_raw_os_error(libc::EFBIG))?;
    Ok(st)
}

fn offset_to_chunk_index(offset: u64) -> u64 {
    offset / CHUNK_SIZE
}

fn read_chunks<W: io::Write, T: ReadByChunk>(
    mut w: W,
    file: &T,
    file_size: u64,
    offset: u64,
    size: u32,
) -> io::Result<usize> {
    let remaining = file_size.saturating_sub(offset);
    let size_to_read = std::cmp::min(size as usize, remaining as usize);
    let total = ChunkedSizeIter::new(size_to_read, offset, CHUNK_SIZE as usize).try_fold(
        0,
        |total, (current_offset, planned_data_size)| {
            // TODO(victorhsieh): There might be a non-trivial way to avoid this copy. For example,
            // instead of accepting a buffer, the writer could expose the final destination buffer
            // for the reader to write to. It might not be generally applicable though, e.g. with
            // virtio transport, the buffer may not be continuous.
            let mut buf = [0u8; CHUNK_SIZE as usize];
            let read_size = file.read_chunk(offset_to_chunk_index(current_offset), &mut buf)?;
            if read_size < planned_data_size {
                return Err(io::Error::from_raw_os_error(libc::ENODATA));
            }

            let begin = (current_offset % CHUNK_SIZE) as usize;
            let end = begin + planned_data_size;
            let s = w.write(&buf[begin..end])?;
            if s != planned_data_size {
                return Err(io::Error::from_raw_os_error(libc::EIO));
            }
            Ok(total + s)
        },
    )?;

    Ok(total)
}

// No need to support enumerating directory entries.
struct EmptyDirectoryIterator {}

impl DirectoryIterator for EmptyDirectoryIterator {
    fn next(&mut self) -> Option<DirEntry> {
        None
    }
}

impl FileSystem for AuthFs {
    type Inode = Inode;
    type Handle = Handle;
    type DirIter = EmptyDirectoryIterator;

    fn max_buffer_size(&self) -> u32 {
        self.max_write
    }

    fn init(&self, _capable: FsOptions) -> io::Result<FsOptions> {
        // Enable writeback cache for better performance especially since our bandwidth to the
        // backend service is limited.
        Ok(FsOptions::WRITEBACK_CACHE)
    }

    fn lookup(&self, _ctx: Context, _parent: Inode, name: &CStr) -> io::Result<Entry> {
        // Only accept file name that looks like an integrer. Files in the pool are simply exposed
        // by their inode number. Also, there is currently no directory structure.
        let num = name.to_str().map_err(|_| io::Error::from_raw_os_error(libc::EINVAL))?;
        // Normally, `lookup` is required to increase a reference count for the inode (while
        // `forget` will decrease it). It is not necessary here since the files are configured to
        // be static.
        let inode = num.parse::<Inode>().map_err(|_| io::Error::from_raw_os_error(libc::ENOENT))?;
        let st = match self.get_file_config(&inode)? {
            FileConfig::LocalVerifiedReadonlyFile { file_size, .. }
            | FileConfig::LocalUnverifiedReadonlyFile { file_size, .. }
            | FileConfig::RemoteUnverifiedReadonlyFile { file_size, .. }
            | FileConfig::RemoteVerifiedReadonlyFile { file_size, .. } => {
                create_stat(inode, *file_size, FileMode::ReadOnly)?
            }
            FileConfig::RemoteVerifiedNewFile { editor } => {
                create_stat(inode, editor.size(), FileMode::ReadWrite)?
            }
        };
        Ok(Entry {
            inode,
            generation: 0,
            attr: st,
            entry_timeout: DEFAULT_METADATA_TIMEOUT,
            attr_timeout: DEFAULT_METADATA_TIMEOUT,
        })
    }

    fn getattr(
        &self,
        _ctx: Context,
        inode: Inode,
        _handle: Option<Handle>,
    ) -> io::Result<(libc::stat64, Duration)> {
        Ok((
            match self.get_file_config(&inode)? {
                FileConfig::LocalVerifiedReadonlyFile { file_size, .. }
                | FileConfig::LocalUnverifiedReadonlyFile { file_size, .. }
                | FileConfig::RemoteUnverifiedReadonlyFile { file_size, .. }
                | FileConfig::RemoteVerifiedReadonlyFile { file_size, .. } => {
                    create_stat(inode, *file_size, FileMode::ReadOnly)?
                }
                FileConfig::RemoteVerifiedNewFile { editor } => {
                    create_stat(inode, editor.size(), FileMode::ReadWrite)?
                }
            },
            DEFAULT_METADATA_TIMEOUT,
        ))
    }

    fn open(
        &self,
        _ctx: Context,
        inode: Self::Inode,
        flags: u32,
    ) -> io::Result<(Option<Self::Handle>, fuse::sys::OpenOptions)> {
        // Since file handle is not really used in later operations (which use Inode directly),
        // return None as the handle.
        match self.get_file_config(&inode)? {
            FileConfig::LocalVerifiedReadonlyFile { .. }
            | FileConfig::LocalUnverifiedReadonlyFile { .. }
            | FileConfig::RemoteVerifiedReadonlyFile { .. }
            | FileConfig::RemoteUnverifiedReadonlyFile { .. } => {
                check_access_mode(flags, libc::O_RDONLY)?;
            }
            FileConfig::RemoteVerifiedNewFile { .. } => {
                // No need to check access modes since all the modes are allowed to the
                // read-writable file.
            }
        }
        // Always cache the file content. There is currently no need to support direct I/O or avoid
        // the cache buffer. Memory mapping is only possible with cache enabled.
        Ok((None, fuse::sys::OpenOptions::KEEP_CACHE))
    }

    fn read<W: io::Write + ZeroCopyWriter>(
        &self,
        _ctx: Context,
        inode: Inode,
        _handle: Handle,
        w: W,
        size: u32,
        offset: u64,
        _lock_owner: Option<u64>,
        _flags: u32,
    ) -> io::Result<usize> {
        match self.get_file_config(&inode)? {
            FileConfig::LocalVerifiedReadonlyFile { reader, file_size } => {
                read_chunks(w, reader, *file_size, offset, size)
            }
            FileConfig::LocalUnverifiedReadonlyFile { reader, file_size } => {
                read_chunks(w, reader, *file_size, offset, size)
            }
            FileConfig::RemoteVerifiedReadonlyFile { reader, file_size } => {
                read_chunks(w, reader, *file_size, offset, size)
            }
            FileConfig::RemoteUnverifiedReadonlyFile { reader, file_size } => {
                read_chunks(w, reader, *file_size, offset, size)
            }
            FileConfig::RemoteVerifiedNewFile { editor } => {
                // Note that with FsOptions::WRITEBACK_CACHE, it's possible for the kernel to
                // request a read even if the file is open with O_WRONLY.
                read_chunks(w, editor, editor.size(), offset, size)
            }
        }
    }

    fn write<R: io::Read + ZeroCopyReader>(
        &self,
        _ctx: Context,
        inode: Self::Inode,
        _handle: Self::Handle,
        mut r: R,
        size: u32,
        offset: u64,
        _lock_owner: Option<u64>,
        _delayed_write: bool,
        _flags: u32,
    ) -> io::Result<usize> {
        match self.get_file_config(&inode)? {
            FileConfig::RemoteVerifiedNewFile { editor } => {
                let mut buf = vec![0; size as usize];
                r.read_exact(&mut buf)?;
                editor.write_at(&buf, offset)
            }
            _ => Err(io::Error::from_raw_os_error(libc::EBADF)),
        }
    }

    fn setattr(
        &self,
        _ctx: Context,
        inode: Inode,
        attr: libc::stat64,
        _handle: Option<Handle>,
        valid: SetattrValid,
    ) -> io::Result<(libc::stat64, Duration)> {
        match self.get_file_config(&inode)? {
            FileConfig::RemoteVerifiedNewFile { editor } => {
                // Initialize the default stat.
                let mut new_attr = create_stat(inode, editor.size(), FileMode::ReadWrite)?;
                // `valid` indicates what fields in `attr` are valid. Update to return correctly.
                if valid.contains(SetattrValid::SIZE) {
                    // st_size is i64, but the cast should be safe since kernel should not give a
                    // negative size.
                    debug_assert!(attr.st_size >= 0);
                    new_attr.st_size = attr.st_size;
                    editor.resize(attr.st_size as u64)?;
                }

                if valid.contains(SetattrValid::MODE) {
                    warn!("Changing st_mode is not currently supported");
                    return Err(io::Error::from_raw_os_error(libc::ENOSYS));
                }
                if valid.contains(SetattrValid::UID) {
                    warn!("Changing st_uid is not currently supported");
                    return Err(io::Error::from_raw_os_error(libc::ENOSYS));
                }
                if valid.contains(SetattrValid::GID) {
                    warn!("Changing st_gid is not currently supported");
                    return Err(io::Error::from_raw_os_error(libc::ENOSYS));
                }
                if valid.contains(SetattrValid::CTIME) {
                    debug!("Ignoring ctime change as authfs does not maintain timestamp currently");
                }
                if valid.intersects(SetattrValid::ATIME | SetattrValid::ATIME_NOW) {
                    debug!("Ignoring atime change as authfs does not maintain timestamp currently");
                }
                if valid.intersects(SetattrValid::MTIME | SetattrValid::MTIME_NOW) {
                    debug!("Ignoring mtime change as authfs does not maintain timestamp currently");
                }
                Ok((new_attr, DEFAULT_METADATA_TIMEOUT))
            }
            _ => Err(io::Error::from_raw_os_error(libc::EBADF)),
        }
    }
}

/// Mount and start the FUSE instance. This requires CAP_SYS_ADMIN.
pub fn loop_forever(
    file_pool: BTreeMap<Inode, FileConfig>,
    mountpoint: &Path,
) -> Result<(), fuse::Error> {
    let max_read: u32 = 65536;
    let max_write: u32 = 65536;
    let dev_fuse = OpenOptions::new()
        .read(true)
        .write(true)
        .open("/dev/fuse")
        .expect("Failed to open /dev/fuse");

    fuse::mount(
        mountpoint,
        "authfs",
        libc::MS_NOSUID | libc::MS_NODEV,
        &[
            MountOption::FD(dev_fuse.as_raw_fd()),
            MountOption::RootMode(libc::S_IFDIR | libc::S_IXUSR | libc::S_IXGRP | libc::S_IXOTH),
            MountOption::AllowOther,
            MountOption::UserId(0),
            MountOption::GroupId(0),
            MountOption::MaxRead(max_read),
        ],
    )
    .expect("Failed to mount fuse");

    fuse::worker::start_message_loop(
        dev_fuse,
        max_write,
        max_read,
        AuthFs::new(file_pool, max_write),
    )
}
