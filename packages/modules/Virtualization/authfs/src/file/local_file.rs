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

use std::cmp::min;
use std::fs::File;
use std::io;
use std::os::unix::fs::FileExt;

use super::{ChunkBuffer, ReadByChunk};
use crate::common::CHUNK_SIZE;

/// A read-only file that can be read by chunks.
pub struct LocalFileReader {
    file: File,
    size: u64,
}

impl LocalFileReader {
    /// Creates a `LocalFileReader` to read from for the specified `path`.
    pub fn new(file: File) -> io::Result<LocalFileReader> {
        let size = file.metadata()?.len();
        Ok(LocalFileReader { file, size })
    }

    pub fn len(&self) -> u64 {
        self.size
    }
}

impl ReadByChunk for LocalFileReader {
    fn read_chunk(&self, chunk_index: u64, buf: &mut ChunkBuffer) -> io::Result<usize> {
        let start = chunk_index * CHUNK_SIZE;
        if start >= self.size {
            return Ok(0);
        }
        let end = min(self.size, start + CHUNK_SIZE);
        let read_size = (end - start) as usize;
        debug_assert!(read_size <= buf.len());
        self.file.read_exact_at(&mut buf[..read_size], start)?;
        Ok(read_size)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::env::temp_dir;

    #[test]
    fn test_read_4k_file() -> io::Result<()> {
        let file_reader = LocalFileReader::new(File::open("testdata/input.4k")?)?;
        let mut buf = [0u8; 4096];
        let size = file_reader.read_chunk(0, &mut buf)?;
        assert_eq!(size, buf.len());
        Ok(())
    }

    #[test]
    fn test_read_4k1_file() -> io::Result<()> {
        let file_reader = LocalFileReader::new(File::open("testdata/input.4k1")?)?;
        let mut buf = [0u8; 4096];
        let size = file_reader.read_chunk(0, &mut buf)?;
        assert_eq!(size, buf.len());
        let size = file_reader.read_chunk(1, &mut buf)?;
        assert_eq!(size, 1);
        Ok(())
    }

    #[test]
    fn test_read_4m_file() -> io::Result<()> {
        let file_reader = LocalFileReader::new(File::open("testdata/input.4m")?)?;
        for index in 0..file_reader.len() / 4096 {
            let mut buf = [0u8; 4096];
            let size = file_reader.read_chunk(index, &mut buf)?;
            assert_eq!(size, buf.len());
        }
        Ok(())
    }

    #[test]
    fn test_read_beyond_file_size() -> io::Result<()> {
        let file_reader = LocalFileReader::new(File::open("testdata/input.4k").unwrap()).unwrap();
        let mut buf = [0u8; 4096];
        let size = file_reader.read_chunk(1u64, &mut buf)?;
        assert_eq!(size, 0);
        Ok(())
    }

    #[test]
    fn test_read_empty_file() -> io::Result<()> {
        let mut temp_file = temp_dir();
        temp_file.push("authfs_test_empty_file");
        let file_reader = LocalFileReader::new(File::create(temp_file).unwrap()).unwrap();
        let mut buf = [0u8; 4096];
        let size = file_reader.read_chunk(0, &mut buf)?;
        assert_eq!(size, 0);
        Ok(())
    }
}
