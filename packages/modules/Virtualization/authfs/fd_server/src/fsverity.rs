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

use nix::ioctl_readwrite;
use std::io;

// Constants/values from uapi/linux/fsverity.h
const FS_VERITY_METADATA_TYPE_MERKLE_TREE: u64 = 1;
const FS_VERITY_METADATA_TYPE_SIGNATURE: u64 = 3;
const FS_IOCTL_MAGIC: u8 = b'f';
const FS_IOCTL_READ_VERITY_METADATA: u8 = 135;

#[repr(C)]
pub struct fsverity_read_metadata_arg {
    metadata_type: u64,
    offset: u64,
    length: u64,
    buf_ptr: u64,
    __reserved: u64,
}

ioctl_readwrite!(
    read_verity_metadata,
    FS_IOCTL_MAGIC,
    FS_IOCTL_READ_VERITY_METADATA,
    fsverity_read_metadata_arg
);

fn read_metadata(fd: i32, metadata_type: u64, offset: u64, buf: &mut [u8]) -> io::Result<usize> {
    let mut arg = fsverity_read_metadata_arg {
        metadata_type,
        offset,
        length: buf.len() as u64,
        buf_ptr: buf.as_mut_ptr() as u64,
        __reserved: 0,
    };
    Ok(unsafe { read_verity_metadata(fd, &mut arg) }.map_err(|e| {
        if let nix::Error::Sys(errno) = e {
            io::Error::from_raw_os_error(errno as i32)
        } else {
            // Document of nix::sys::ioctl indicates the macro-generated function always returns an
            // nix::errno::Errno, which can be converted nix::Error::Sys above. As the result, this
            // branch is unreachable.
            unreachable!();
        }
    })? as usize)
}

/// Read the raw Merkle tree from the fd, if it exists. The API semantics is similar to a regular
/// pread(2), and may not return full requested buffer.
pub fn read_merkle_tree(fd: i32, offset: u64, buf: &mut [u8]) -> io::Result<usize> {
    read_metadata(fd, FS_VERITY_METADATA_TYPE_MERKLE_TREE, offset, buf)
}

/// Read the fs-verity signature from the fd (if exists). The returned signature should be complete.
pub fn read_signature(fd: i32, buf: &mut [u8]) -> io::Result<usize> {
    read_metadata(fd, FS_VERITY_METADATA_TYPE_SIGNATURE, 0 /* offset */, buf)
}
