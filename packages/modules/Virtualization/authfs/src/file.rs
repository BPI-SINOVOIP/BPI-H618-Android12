mod local_file;
mod remote_file;

pub use local_file::LocalFileReader;
pub use remote_file::{RemoteFileEditor, RemoteFileReader, RemoteMerkleTreeReader};

use std::io;

use crate::common::CHUNK_SIZE;

use authfs_aidl_interface::aidl::com::android::virt::fs::IVirtFdService;
use authfs_aidl_interface::binder::{get_interface, Strong};

// TODO(victorhsieh): use remote binder.
pub fn get_local_binder() -> Strong<dyn IVirtFdService::IVirtFdService> {
    let service_name = "authfs_fd_server";
    get_interface(&service_name).expect("Cannot reach authfs_fd_server binder service")
}

pub type ChunkBuffer = [u8; CHUNK_SIZE as usize];

/// A trait for reading data by chunks. Chunks can be read by specifying the chunk index. Only the
/// last chunk may have incomplete chunk size.
pub trait ReadByChunk {
    /// Reads the `chunk_index`-th chunk to a `ChunkBuffer`. Returns the size read, which has to be
    /// `CHUNK_SIZE` except for the last incomplete chunk. Reading beyond the file size (including
    /// empty file) should return 0.
    fn read_chunk(&self, chunk_index: u64, buf: &mut ChunkBuffer) -> io::Result<usize>;
}

/// A trait to write a buffer to the destination at a given offset. The implementation does not
/// necessarily own or maintain the destination state.
///
/// NB: The trait is required in a member of `fusefs::AuthFs`, which is required to be Sync and
/// immutable (this the member).
pub trait RandomWrite {
    /// Writes `buf` to the destination at `offset`. Returns the written size, which may not be the
    /// full buffer.
    fn write_at(&self, buf: &[u8], offset: u64) -> io::Result<usize>;

    /// Writes the full `buf` to the destination at `offset`.
    fn write_all_at(&self, buf: &[u8], offset: u64) -> io::Result<()> {
        let mut input_offset = 0;
        let mut output_offset = offset;
        while input_offset < buf.len() {
            let size = self.write_at(&buf[input_offset..], output_offset)?;
            input_offset += size;
            output_offset += size as u64;
        }
        Ok(())
    }

    /// Resizes the file to the new size.
    fn resize(&self, size: u64) -> io::Result<()>;
}
