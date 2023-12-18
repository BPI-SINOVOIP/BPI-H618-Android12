/*
 * Copyright (C) 2020 The Android Open Source Project
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

use libc::EIO;
use std::io;

use super::common::{build_fsverity_digest, merkle_tree_height, FsverityError};
use super::sys::{FS_VERITY_HASH_ALG_SHA256, FS_VERITY_MAGIC};
use crate::auth::Authenticator;
use crate::common::{divide_roundup, CHUNK_SIZE};
use crate::crypto::{CryptoError, Sha256Hasher};
use crate::file::{ChunkBuffer, ReadByChunk};

const ZEROS: [u8; CHUNK_SIZE as usize] = [0u8; CHUNK_SIZE as usize];

// The size of `struct fsverity_formatted_digest` in Linux with SHA-256.
const SIZE_OF_FSVERITY_FORMATTED_DIGEST_SHA256: usize = 12 + Sha256Hasher::HASH_SIZE;

type HashBuffer = [u8; Sha256Hasher::HASH_SIZE];

fn hash_with_padding(chunk: &[u8], pad_to: usize) -> Result<HashBuffer, CryptoError> {
    let padding_size = pad_to - chunk.len();
    Sha256Hasher::new()?.update(&chunk)?.update(&ZEROS[..padding_size])?.finalize()
}

fn verity_check<T: ReadByChunk>(
    chunk: &[u8],
    chunk_index: u64,
    file_size: u64,
    merkle_tree: &T,
) -> Result<HashBuffer, FsverityError> {
    // The caller should not be able to produce a chunk at the first place if `file_size` is 0. The
    // current implementation expects to crash when a `ReadByChunk` implementation reads
    // beyond the file size, including empty file.
    assert_ne!(file_size, 0);

    let chunk_hash = hash_with_padding(&chunk, CHUNK_SIZE as usize)?;

    fsverity_walk(chunk_index, file_size, merkle_tree)?.try_fold(
        chunk_hash,
        |actual_hash, result| {
            let (merkle_chunk, hash_offset_in_chunk) = result?;
            let expected_hash =
                &merkle_chunk[hash_offset_in_chunk..hash_offset_in_chunk + Sha256Hasher::HASH_SIZE];
            if actual_hash != expected_hash {
                return Err(FsverityError::CannotVerify);
            }
            Ok(hash_with_padding(&merkle_chunk, CHUNK_SIZE as usize)?)
        },
    )
}

/// Given a chunk index and the size of the file, returns an iterator that walks the Merkle tree
/// from the leaf to the root. The iterator carries the slice of the chunk/node as well as the
/// offset of the child node's hash. It is up to the iterator user to use the node and hash,
/// e.g. for the actual verification.
#[allow(clippy::needless_collect)]
fn fsverity_walk<T: ReadByChunk>(
    chunk_index: u64,
    file_size: u64,
    merkle_tree: &T,
) -> Result<impl Iterator<Item = Result<([u8; 4096], usize), FsverityError>> + '_, FsverityError> {
    let hashes_per_node = CHUNK_SIZE / Sha256Hasher::HASH_SIZE as u64;
    debug_assert_eq!(hashes_per_node, 128u64);
    let max_level = merkle_tree_height(file_size).expect("file should not be empty") as u32;
    let root_to_leaf_steps = (0..=max_level)
        .rev()
        .map(|x| {
            let leaves_per_hash = hashes_per_node.pow(x);
            let leaves_size_per_hash = CHUNK_SIZE * leaves_per_hash;
            let leaves_size_per_node = leaves_size_per_hash * hashes_per_node;
            let nodes_at_level = divide_roundup(file_size, leaves_size_per_node);
            let level_size = nodes_at_level * CHUNK_SIZE;
            let offset_in_level = (chunk_index / leaves_per_hash) * Sha256Hasher::HASH_SIZE as u64;
            (level_size, offset_in_level)
        })
        .scan(0, |level_offset, (level_size, offset_in_level)| {
            let this_level_offset = *level_offset;
            *level_offset += level_size;
            let global_hash_offset = this_level_offset + offset_in_level;
            Some(global_hash_offset)
        })
        .map(|global_hash_offset| {
            let chunk_index = global_hash_offset / CHUNK_SIZE;
            let hash_offset_in_chunk = (global_hash_offset % CHUNK_SIZE) as usize;
            (chunk_index, hash_offset_in_chunk)
        })
        .collect::<Vec<_>>(); // Needs to collect first to be able to reverse below.

    Ok(root_to_leaf_steps.into_iter().rev().map(move |(chunk_index, hash_offset_in_chunk)| {
        let mut merkle_chunk = [0u8; 4096];
        // read_chunk is supposed to return a full chunk, or an incomplete one at the end of the
        // file. In the incomplete case, the hash is calculated with 0-padding to the chunk size.
        // Therefore, we don't need to check the returned size here.
        let _ = merkle_tree.read_chunk(chunk_index, &mut merkle_chunk)?;
        Ok((merkle_chunk, hash_offset_in_chunk))
    }))
}

fn build_fsverity_formatted_digest(
    root_hash: &HashBuffer,
    file_size: u64,
) -> Result<[u8; SIZE_OF_FSVERITY_FORMATTED_DIGEST_SHA256], CryptoError> {
    let digest = build_fsverity_digest(root_hash, file_size)?;
    // Little-endian byte representation of fsverity_formatted_digest from linux/fsverity.h
    // Not FFI-ed as it seems easier to deal with the raw bytes manually.
    let mut formatted_digest = [0u8; SIZE_OF_FSVERITY_FORMATTED_DIGEST_SHA256];
    formatted_digest[0..8].copy_from_slice(FS_VERITY_MAGIC);
    formatted_digest[8..10].copy_from_slice(&(FS_VERITY_HASH_ALG_SHA256 as u16).to_le_bytes());
    formatted_digest[10..12].copy_from_slice(&(Sha256Hasher::HASH_SIZE as u16).to_le_bytes());
    formatted_digest[12..].copy_from_slice(&digest);
    Ok(formatted_digest)
}

pub struct VerifiedFileReader<F: ReadByChunk, M: ReadByChunk> {
    chunked_file: F,
    file_size: u64,
    merkle_tree: M,
    root_hash: HashBuffer,
}

impl<F: ReadByChunk, M: ReadByChunk> VerifiedFileReader<F, M> {
    pub fn new<A: Authenticator>(
        authenticator: &A,
        chunked_file: F,
        file_size: u64,
        sig: Vec<u8>,
        merkle_tree: M,
    ) -> Result<VerifiedFileReader<F, M>, FsverityError> {
        let mut buf = [0u8; CHUNK_SIZE as usize];
        let size = merkle_tree.read_chunk(0, &mut buf)?;
        if buf.len() != size {
            return Err(FsverityError::InsufficientData(size));
        }
        let root_hash = Sha256Hasher::new()?.update(&buf[..])?.finalize()?;
        let formatted_digest = build_fsverity_formatted_digest(&root_hash, file_size)?;
        let valid = authenticator.verify(&sig, &formatted_digest)?;
        if valid {
            Ok(VerifiedFileReader { chunked_file, file_size, merkle_tree, root_hash })
        } else {
            Err(FsverityError::BadSignature)
        }
    }
}

impl<F: ReadByChunk, M: ReadByChunk> ReadByChunk for VerifiedFileReader<F, M> {
    fn read_chunk(&self, chunk_index: u64, buf: &mut ChunkBuffer) -> io::Result<usize> {
        let size = self.chunked_file.read_chunk(chunk_index, buf)?;
        let root_hash = verity_check(&buf[..size], chunk_index, self.file_size, &self.merkle_tree)
            .map_err(|_| io::Error::from_raw_os_error(EIO))?;
        if root_hash != self.root_hash {
            Err(io::Error::from_raw_os_error(EIO))
        } else {
            Ok(size)
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::auth::FakeAuthenticator;
    use crate::file::{LocalFileReader, ReadByChunk};
    use anyhow::Result;
    use std::fs::{self, File};
    use std::io::Read;

    type LocalVerifiedFileReader = VerifiedFileReader<LocalFileReader, LocalFileReader>;

    fn total_chunk_number(file_size: u64) -> u64 {
        (file_size + 4095) / 4096
    }

    // Returns a reader with fs-verity verification and the file size.
    fn new_reader_with_fsverity(
        content_path: &str,
        merkle_tree_path: &str,
        signature_path: &str,
    ) -> Result<(LocalVerifiedFileReader, u64)> {
        let file_reader = LocalFileReader::new(File::open(content_path)?)?;
        let file_size = file_reader.len();
        let merkle_tree = LocalFileReader::new(File::open(merkle_tree_path)?)?;
        let mut sig = Vec::new();
        let _ = File::open(signature_path)?.read_to_end(&mut sig)?;
        let authenticator = FakeAuthenticator::always_succeed();
        Ok((
            VerifiedFileReader::new(&authenticator, file_reader, file_size, sig, merkle_tree)?,
            file_size,
        ))
    }

    #[test]
    fn fsverity_verify_full_read_4k() -> Result<()> {
        let (file_reader, file_size) = new_reader_with_fsverity(
            "testdata/input.4k",
            "testdata/input.4k.merkle_dump",
            "testdata/input.4k.fsv_sig",
        )?;

        for i in 0..total_chunk_number(file_size) {
            let mut buf = [0u8; 4096];
            assert!(file_reader.read_chunk(i, &mut buf).is_ok());
        }
        Ok(())
    }

    #[test]
    fn fsverity_verify_full_read_4k1() -> Result<()> {
        let (file_reader, file_size) = new_reader_with_fsverity(
            "testdata/input.4k1",
            "testdata/input.4k1.merkle_dump",
            "testdata/input.4k1.fsv_sig",
        )?;

        for i in 0..total_chunk_number(file_size) {
            let mut buf = [0u8; 4096];
            assert!(file_reader.read_chunk(i, &mut buf).is_ok());
        }
        Ok(())
    }

    #[test]
    fn fsverity_verify_full_read_4m() -> Result<()> {
        let (file_reader, file_size) = new_reader_with_fsverity(
            "testdata/input.4m",
            "testdata/input.4m.merkle_dump",
            "testdata/input.4m.fsv_sig",
        )?;

        for i in 0..total_chunk_number(file_size) {
            let mut buf = [0u8; 4096];
            assert!(file_reader.read_chunk(i, &mut buf).is_ok());
        }
        Ok(())
    }

    #[test]
    fn fsverity_verify_bad_merkle_tree() -> Result<()> {
        let (file_reader, _) = new_reader_with_fsverity(
            "testdata/input.4m",
            "testdata/input.4m.merkle_dump.bad", // First leaf node is corrupted.
            "testdata/input.4m.fsv_sig",
        )?;

        // A lowest broken node (a 4K chunk that contains 128 sha256 hashes) will fail the read
        // failure of the underlying chunks, but not before or after.
        let mut buf = [0u8; 4096];
        let num_hashes = 4096 / 32;
        let last_index = num_hashes;
        for i in 0..last_index {
            assert!(file_reader.read_chunk(i, &mut buf).is_err());
        }
        assert!(file_reader.read_chunk(last_index, &mut buf).is_ok());
        Ok(())
    }

    #[test]
    fn invalid_signature() -> Result<()> {
        let authenticator = FakeAuthenticator::always_fail();
        let file_reader = LocalFileReader::new(File::open("testdata/input.4m")?)?;
        let file_size = file_reader.len();
        let merkle_tree = LocalFileReader::new(File::open("testdata/input.4m.merkle_dump")?)?;
        let sig = fs::read("testdata/input.4m.fsv_sig")?;
        assert!(VerifiedFileReader::new(&authenticator, file_reader, file_size, sig, merkle_tree)
            .is_err());
        Ok(())
    }
}
