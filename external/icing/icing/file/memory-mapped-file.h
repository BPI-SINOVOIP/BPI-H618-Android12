// Copyright (C) 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Allows memory-mapping a full file or a specific region within the file.
// It also supports efficiently switching the region being mapped.
//
// Note on Performance:
// It supports different optimized strategies for common patterns on both
// read-only and read-write files. This includes using read-ahead buffers for
// faster reads as well as background-sync vs manual-sync of changes to disk.
// For more details, see comments at MemoryMappedFile::Strategy.
//
// Usage:
//
// MemoryMappedFile mmapped_file(filesystem, "/file.pb", READ_WRITE_AUTO_SYNC));
// mmapped_file->Remap(0, 16* 1024);  // load the first 16K of the file.
//
// char read_byte = mmapped_file->region()[100];
// mmapped_file->mutable_region()[10] = write_byte;
//
// mmapped_file->PersistToDisk(); // Optional; immediately writes changes to
// disk.
//
// mmapped_file->Remap(16*1024, 16* 1024);  // load the next 16K.
// mmapped_file->mutable_region()[10] = write_byte;
// mmapped_file.reset();

#ifndef ICING_FILE_MEMORY_MAPPED_FILE_H_
#define ICING_FILE_MEMORY_MAPPED_FILE_H_

#include <unistd.h>

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include "icing/text_classifier/lib3/utils/base/status.h"
#include "icing/file/filesystem.h"

namespace icing {
namespace lib {

class MemoryMappedFile {
 public:
  static size_t __attribute__((const)) system_page_size() {
    static const size_t page_size = sysconf(_SC_PAGE_SIZE);
    return page_size;
  }

  enum Strategy {
    // Memory map a read-only file into a read-only memory region.
    READ_ONLY,

    // Memory map a read-write file into a writable memory region. Any changes
    // made to the region are automatically flushed to the underlying file in
    // the background.
    READ_WRITE_AUTO_SYNC,

    // Memory map a read-write file into a writable memory region. Changes made
    // to this region will never be auto-synced to the underlying file. Unless
    // the caller explicitly calls PersistToDisk(), all changes will be lost
    // when the
    // MemoryMappedFile is destroyed.
    READ_WRITE_MANUAL_SYNC,
  };

  // file_path : Full path of the file that needs to be memory-mapped.
  MemoryMappedFile(const Filesystem& filesystem, std::string_view file_path,
                   Strategy mmap_strategy);
  MemoryMappedFile(const MemoryMappedFile& other) = delete;
  MemoryMappedFile(MemoryMappedFile&& other);
  MemoryMappedFile& operator=(const MemoryMappedFile& other) = delete;
  MemoryMappedFile& operator=(MemoryMappedFile&& other);
  // Frees any region that is still memory-mapped region.
  ~MemoryMappedFile();

  // Memory-map the newly specified region within the file specified by
  // file_offset and mmap_size. Unmaps any previously mmapped region.
  //
  // Returns any encountered IO error.
  libtextclassifier3::Status Remap(size_t file_offset, size_t mmap_size);

  // unmap and free-up the region that has currently been memory mapped.
  void Unmap();

  // Explicitly persist any changes made to the currently mapped region to disk.
  //
  // NOTE: This is only valid if Strategy=READ_WRITE was used.
  //
  // Returns:
  //   OK on success
  //   INTERNAL on I/O error
  //   FAILED_PRECONDITION if Strategy is not implemented
  libtextclassifier3::Status PersistToDisk();

  // Advise the system to help it optimize the memory-mapped region for
  // upcoming read/write operations.
  //
  // NOTE: See linux documentation of madvise() for additional details.
  enum AccessPattern {
    // Future memory access are expected to be in random order. So, readhead
    // will have limited impact on latency.
    ACCESS_RANDOM,

    // Future memory access are expected to be sequential. So, some readahead
    // can greatly improve latency.
    ACCESS_SEQUENTIAL,

    // Future memory access is expected to be high-volume and all over the file.
    // So, preloading the whole region into memory would greatly improve
    // latency.
    ACCESS_ALL,

    // Future memory access is expected to be rare. So, it is best to free up
    // as much of preloaded memory as possible.
    ACCESS_NONE,
  };
  libtextclassifier3::Status OptimizeFor(AccessPattern access_pattern);

  // Accessors to the memory-mapped region. Returns null if nothing is mapped.
  const char* region() const { return region_; }
  char* mutable_region() { return region_; }

  size_t region_size() const { return region_size_; }
  Strategy strategy() const { return strategy_; }

 private:
  // Swaps the contents of this with other.
  void Swap(MemoryMappedFile* other);

  // Cached constructor params.
  const Filesystem* filesystem_;
  std::string file_path_;
  Strategy strategy_;

  // Offset within the file at which the current memory-mapped region starts.
  size_t file_offset_ = 0;

  // Region that is currently memory-mapped.
  char* region_ = nullptr;
  size_t region_size_ = 0;

  // The actual size of the region we mmapped. As the requested region might not
  // align with system pages, we often mmap more bytes than requested.
  size_t adjusted_mmap_size_ = 0;

  // Raw pointer (or error) returned by calls to mmap().
  void* mmap_result_ = nullptr;
};

}  // namespace lib
}  // namespace icing

#endif  // ICING_FILE_MEMORY_MAPPED_FILE_H_
