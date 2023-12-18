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

//! This program is a constrained file/FD server to serve file requests through a remote[1] binder
//! service. The file server is not designed to serve arbitrary file paths in the filesystem. On
//! the contrary, the server should be configured to start with already opened FDs, and serve the
//! client's request against the FDs
//!
//! For example, `exec 9</path/to/file fd_server --ro-fds 9` starts the binder service. A client
//! client can then request the content of file 9 by offset and size.
//!
//! [1] Since the remote binder is not ready, this currently implementation uses local binder
//!     first.

mod fsverity;

use std::cmp::min;
use std::collections::BTreeMap;
use std::convert::TryInto;
use std::ffi::CString;
use std::fs::File;
use std::io;
use std::os::unix::fs::FileExt;
use std::os::unix::io::{AsRawFd, FromRawFd};

use anyhow::{bail, Context, Result};
use log::{debug, error};

use authfs_aidl_interface::aidl::com::android::virt::fs::IVirtFdService::{
    BnVirtFdService, IVirtFdService, ERROR_IO, ERROR_UNKNOWN_FD, MAX_REQUESTING_DATA,
};
use authfs_aidl_interface::binder::{
    add_service, BinderFeatures, ExceptionCode, Interface, ProcessState, Result as BinderResult,
    Status, StatusCode, Strong,
};

const SERVICE_NAME: &str = "authfs_fd_server";

fn new_binder_exception<T: AsRef<str>>(exception: ExceptionCode, message: T) -> Status {
    Status::new_exception(exception, CString::new(message.as_ref()).as_deref().ok())
}

fn validate_and_cast_offset(offset: i64) -> Result<u64, Status> {
    offset.try_into().map_err(|_| {
        new_binder_exception(ExceptionCode::ILLEGAL_ARGUMENT, format!("Invalid offset: {}", offset))
    })
}

fn validate_and_cast_size(size: i32) -> Result<usize, Status> {
    if size > MAX_REQUESTING_DATA {
        Err(new_binder_exception(
            ExceptionCode::ILLEGAL_ARGUMENT,
            format!("Unexpectedly large size: {}", size),
        ))
    } else {
        size.try_into().map_err(|_| {
            new_binder_exception(ExceptionCode::ILLEGAL_ARGUMENT, format!("Invalid size: {}", size))
        })
    }
}

/// Configuration of a file descriptor to be served/exposed/shared.
enum FdConfig {
    /// A read-only file to serve by this server. The file is supposed to be verifiable with the
    /// associated fs-verity metadata.
    Readonly {
        /// The file to read from. fs-verity metadata can be retrieved from this file's FD.
        file: File,

        /// Alternative Merkle tree stored in another file.
        alt_merkle_tree: Option<File>,

        /// Alternative signature stored in another file.
        alt_signature: Option<File>,
    },

    /// A readable/writable file to serve by this server. This backing file should just be a
    /// regular file and does not have any specific property.
    ReadWrite(File),
}

struct FdService {
    /// A pool of opened files, may be readonly or read-writable.
    fd_pool: BTreeMap<i32, FdConfig>,
}

impl FdService {
    pub fn new_binder(fd_pool: BTreeMap<i32, FdConfig>) -> Strong<dyn IVirtFdService> {
        BnVirtFdService::new_binder(FdService { fd_pool }, BinderFeatures::default())
    }

    fn get_file_config(&self, id: i32) -> BinderResult<&FdConfig> {
        self.fd_pool.get(&id).ok_or_else(|| Status::from(ERROR_UNKNOWN_FD))
    }
}

impl Interface for FdService {}

impl IVirtFdService for FdService {
    fn readFile(&self, id: i32, offset: i64, size: i32) -> BinderResult<Vec<u8>> {
        let size: usize = validate_and_cast_size(size)?;
        let offset: u64 = validate_and_cast_offset(offset)?;

        match self.get_file_config(id)? {
            FdConfig::Readonly { file, .. } | FdConfig::ReadWrite(file) => {
                read_into_buf(&file, size, offset).map_err(|e| {
                    error!("readFile: read error: {}", e);
                    Status::from(ERROR_IO)
                })
            }
        }
    }

    fn readFsverityMerkleTree(&self, id: i32, offset: i64, size: i32) -> BinderResult<Vec<u8>> {
        let size: usize = validate_and_cast_size(size)?;
        let offset: u64 = validate_and_cast_offset(offset)?;

        match &self.get_file_config(id)? {
            FdConfig::Readonly { file, alt_merkle_tree, .. } => {
                if let Some(tree_file) = &alt_merkle_tree {
                    read_into_buf(&tree_file, size, offset).map_err(|e| {
                        error!("readFsverityMerkleTree: read error: {}", e);
                        Status::from(ERROR_IO)
                    })
                } else {
                    let mut buf = vec![0; size];
                    let s = fsverity::read_merkle_tree(file.as_raw_fd(), offset, &mut buf)
                        .map_err(|e| {
                            error!("readFsverityMerkleTree: failed to retrieve merkle tree: {}", e);
                            Status::from(e.raw_os_error().unwrap_or(ERROR_IO))
                        })?;
                    debug_assert!(s <= buf.len(), "Shouldn't return more bytes than asked");
                    buf.truncate(s);
                    Ok(buf)
                }
            }
            FdConfig::ReadWrite(_file) => {
                // For a writable file, Merkle tree is not expected to be served since Auth FS
                // doesn't trust it anyway. Auth FS may keep the Merkle tree privately for its own
                // use.
                Err(new_binder_exception(ExceptionCode::UNSUPPORTED_OPERATION, "Unsupported"))
            }
        }
    }

    fn readFsveritySignature(&self, id: i32) -> BinderResult<Vec<u8>> {
        match &self.get_file_config(id)? {
            FdConfig::Readonly { file, alt_signature, .. } => {
                if let Some(sig_file) = &alt_signature {
                    // Supposedly big enough buffer size to store signature.
                    let size = MAX_REQUESTING_DATA as usize;
                    let offset = 0;
                    read_into_buf(&sig_file, size, offset).map_err(|e| {
                        error!("readFsveritySignature: read error: {}", e);
                        Status::from(ERROR_IO)
                    })
                } else {
                    let mut buf = vec![0; MAX_REQUESTING_DATA as usize];
                    let s = fsverity::read_signature(file.as_raw_fd(), &mut buf).map_err(|e| {
                        error!("readFsverityMerkleTree: failed to retrieve merkle tree: {}", e);
                        Status::from(e.raw_os_error().unwrap_or(ERROR_IO))
                    })?;
                    debug_assert!(s <= buf.len(), "Shouldn't return more bytes than asked");
                    buf.truncate(s);
                    Ok(buf)
                }
            }
            FdConfig::ReadWrite(_file) => {
                // There is no signature for a writable file.
                Err(new_binder_exception(ExceptionCode::UNSUPPORTED_OPERATION, "Unsupported"))
            }
        }
    }

    fn writeFile(&self, id: i32, buf: &[u8], offset: i64) -> BinderResult<i32> {
        match &self.get_file_config(id)? {
            FdConfig::Readonly { .. } => Err(StatusCode::INVALID_OPERATION.into()),
            FdConfig::ReadWrite(file) => {
                let offset: u64 = offset.try_into().map_err(|_| {
                    new_binder_exception(ExceptionCode::ILLEGAL_ARGUMENT, "Invalid offset")
                })?;
                // Check buffer size just to make `as i32` safe below.
                if buf.len() > i32::MAX as usize {
                    return Err(new_binder_exception(
                        ExceptionCode::ILLEGAL_ARGUMENT,
                        "Buffer size is too big",
                    ));
                }
                Ok(file.write_at(buf, offset).map_err(|e| {
                    error!("writeFile: write error: {}", e);
                    Status::from(ERROR_IO)
                })? as i32)
            }
        }
    }

    fn resize(&self, id: i32, size: i64) -> BinderResult<()> {
        match &self.get_file_config(id)? {
            FdConfig::Readonly { .. } => Err(StatusCode::INVALID_OPERATION.into()),
            FdConfig::ReadWrite(file) => {
                if size < 0 {
                    return Err(new_binder_exception(
                        ExceptionCode::ILLEGAL_ARGUMENT,
                        "Invalid size to resize to",
                    ));
                }
                file.set_len(size as u64).map_err(|e| {
                    error!("resize: set_len error: {}", e);
                    Status::from(ERROR_IO)
                })
            }
        }
    }
}

fn read_into_buf(file: &File, max_size: usize, offset: u64) -> io::Result<Vec<u8>> {
    let remaining = file.metadata()?.len().saturating_sub(offset);
    let buf_size = min(remaining, max_size as u64) as usize;
    let mut buf = vec![0; buf_size];
    file.read_exact_at(&mut buf, offset)?;
    Ok(buf)
}

fn is_fd_valid(fd: i32) -> bool {
    // SAFETY: a query-only syscall
    let retval = unsafe { libc::fcntl(fd, libc::F_GETFD) };
    retval >= 0
}

fn fd_to_file(fd: i32) -> Result<File> {
    if !is_fd_valid(fd) {
        bail!("Bad FD: {}", fd);
    }
    // SAFETY: The caller is supposed to provide valid FDs to this process.
    Ok(unsafe { File::from_raw_fd(fd) })
}

fn parse_arg_ro_fds(arg: &str) -> Result<(i32, FdConfig)> {
    let result: Result<Vec<i32>, _> = arg.split(':').map(|x| x.parse::<i32>()).collect();
    let fds = result?;
    if fds.len() > 3 {
        bail!("Too many options: {}", arg);
    }
    Ok((
        fds[0],
        FdConfig::Readonly {
            file: fd_to_file(fds[0])?,
            // Alternative Merkle tree, if provided
            alt_merkle_tree: fds.get(1).map(|fd| fd_to_file(*fd)).transpose()?,
            // Alternative signature, if provided
            alt_signature: fds.get(2).map(|fd| fd_to_file(*fd)).transpose()?,
        },
    ))
}

fn parse_arg_rw_fds(arg: &str) -> Result<(i32, FdConfig)> {
    let fd = arg.parse::<i32>()?;
    let file = fd_to_file(fd)?;
    if file.metadata()?.len() > 0 {
        bail!("File is expected to be empty");
    }
    Ok((fd, FdConfig::ReadWrite(file)))
}

fn parse_args() -> Result<BTreeMap<i32, FdConfig>> {
    #[rustfmt::skip]
    let matches = clap::App::new("fd_server")
        .arg(clap::Arg::with_name("ro-fds")
             .long("ro-fds")
             .multiple(true)
             .number_of_values(1))
        .arg(clap::Arg::with_name("rw-fds")
             .long("rw-fds")
             .multiple(true)
             .number_of_values(1))
        .get_matches();

    let mut fd_pool = BTreeMap::new();
    if let Some(args) = matches.values_of("ro-fds") {
        for arg in args {
            let (fd, config) = parse_arg_ro_fds(arg)?;
            fd_pool.insert(fd, config);
        }
    }
    if let Some(args) = matches.values_of("rw-fds") {
        for arg in args {
            let (fd, config) = parse_arg_rw_fds(arg)?;
            fd_pool.insert(fd, config);
        }
    }
    Ok(fd_pool)
}

fn main() -> Result<()> {
    android_logger::init_once(
        android_logger::Config::default().with_tag("fd_server").with_min_level(log::Level::Debug),
    );

    let fd_pool = parse_args()?;

    ProcessState::start_thread_pool();

    add_service(SERVICE_NAME, FdService::new_binder(fd_pool).as_binder())
        .with_context(|| format!("Failed to register service {}", SERVICE_NAME))?;
    debug!("fd_server is running.");

    ProcessState::join_thread_pool();
    bail!("Unexpected exit after join_thread_pool")
}
