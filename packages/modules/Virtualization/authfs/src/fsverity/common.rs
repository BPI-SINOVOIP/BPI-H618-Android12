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

use std::io;

use thiserror::Error;

use super::sys::{FS_VERITY_HASH_ALG_SHA256, FS_VERITY_LOG_BLOCKSIZE, FS_VERITY_VERSION};
use crate::common::{divide_roundup, CHUNK_SIZE};
use crate::crypto::{CryptoError, Sha256Hash, Sha256Hasher};

#[derive(Error, Debug)]
pub enum FsverityError {
    #[error("Cannot verify a signature")]
    BadSignature,
    #[error("Insufficient data, only got {0}")]
    InsufficientData(usize),
    #[error("Cannot verify a block")]
    CannotVerify,
    #[error("I/O error")]
    Io(#[from] io::Error),
    #[error("Crypto")]
    UnexpectedCryptoError(#[from] CryptoError),
    #[error("Invalid state")]
    InvalidState,
}

fn log128_ceil(num: u64) -> Option<u64> {
    match num {
        0 => None,
        n => Some(divide_roundup(64 - (n - 1).leading_zeros() as u64, 7)),
    }
}

/// Return the Merkle tree height for our tree configuration, or None if the size is 0.
pub fn merkle_tree_height(data_size: u64) -> Option<u64> {
    let hashes_per_node = CHUNK_SIZE / Sha256Hasher::HASH_SIZE as u64;
    let hash_pages = divide_roundup(data_size, hashes_per_node * CHUNK_SIZE);
    log128_ceil(hash_pages)
}

pub fn build_fsverity_digest(
    root_hash: &Sha256Hash,
    file_size: u64,
) -> Result<Sha256Hash, CryptoError> {
    // Little-endian byte representation of fsverity_descriptor from linux/fsverity.h
    // Not FFI-ed as it seems easier to deal with the raw bytes manually.
    Sha256Hasher::new()?
        .update(&FS_VERITY_VERSION.to_le_bytes())? // version
        .update(&FS_VERITY_HASH_ALG_SHA256.to_le_bytes())? // hash_algorithm
        .update(&FS_VERITY_LOG_BLOCKSIZE.to_le_bytes())? // log_blocksize
        .update(&0u8.to_le_bytes())? // salt_size
        .update(&0u32.to_le_bytes())? // sig_size
        .update(&file_size.to_le_bytes())? // data_size
        .update(root_hash)? // root_hash, first 32 bytes
        .update(&[0u8; 32])? // root_hash, last 32 bytes, always 0 because we are using sha256.
        .update(&[0u8; 32])? // salt
        .update(&[0u8; 32])? // reserved
        .update(&[0u8; 32])? // reserved
        .update(&[0u8; 32])? // reserved
        .update(&[0u8; 32])? // reserved
        .update(&[0u8; 16])? // reserved
        .finalize()
}
