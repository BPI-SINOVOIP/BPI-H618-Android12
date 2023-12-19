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

// A file-backed vector that can store fixed-width elements. It provides
// built-in support for checksums to verify data integrity and an in-memory
// cache for fast read/writes.
//
// If the file is corrupted/in an invalid state, all contents are lost, i.e.
// there is no clear recovery path other than recreating/repopulating the
// contents.
//
// Note on Performance:
// The class keeps the vector in a mmapped area. This allows users to specify
// which MemoryMappedFile::Strategy they wish to use with this class. The vector
// will implicitly grow when the user tries to access an element beyond its
// current size. Growing happens in 16KiB chunks, up to a maximum size of 1MiB.
//
// Note on Checksumming:
// Checksumming happens lazily. We do tail checksums to avoid recalculating the
// checksum of the entire file on each modfification. A full checksum will be
// computed/verified at creation time, when persisting to disk, or whenever the
// user manually calls ComputeChecksum(). A separate header checksum is kept for
// a quick integrity check.
//
//
// Usage:
// RETURN_OR_ASSIGN(auto vector, FileBackedVector<char>::Create(...));
//
// ICING_RETURN_IF_ERROR(vector->Set(0, 'a'));
// ICING_RETURN_IF_ERROR(vector->Set(1, 'b'));
// ICING_RETURN_IF_ERROR(vector->Set(2, 'c'));
//
// vector->num_elements();  // Returns 3
//
// vector->At(2);  // Returns 'c'
//
// vector->TruncateTo(1);
// vector->num_elements();  // Returns 1
// vector->At(0);  // Returns 'a'
//
// vector->ComputeChecksum();  // Force a checksum update and gets the checksum
//
// vector->PersistToDisk();  // Persist contents to disk.

#ifndef ICING_FILE_FILE_BACKED_VECTOR_H_
#define ICING_FILE_FILE_BACKED_VECTOR_H_

#include <inttypes.h>
#include <stdint.h>
#include <sys/mman.h>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "icing/text_classifier/lib3/utils/base/status.h"
#include "icing/text_classifier/lib3/utils/base/statusor.h"
#include "icing/absl_ports/canonical_errors.h"
#include "icing/absl_ports/str_cat.h"
#include "icing/file/filesystem.h"
#include "icing/file/memory-mapped-file.h"
#include "icing/legacy/core/icing-string-util.h"
#include "icing/util/crc32.h"
#include "icing/util/logging.h"
#include "icing/util/math-util.h"
#include "icing/util/status-macros.h"

namespace icing {
namespace lib {

template <typename T>
class FileBackedVector {
 public:
  // Header stored at the beginning of the file before the rest of the vector
  // elements. Stores metadata on the vector.
  struct Header {
    // Static assert constants.
    static constexpr int32_t kHeaderSize = 24;
    static constexpr int32_t kHeaderChecksumOffset = 16;

    static constexpr int32_t kMagic = 0x8bbbe237;

    // Holds the magic as quick sanity check against file corruption
    int32_t magic;

    // Byte size of each element in the vector
    int32_t element_size;

    // Number of elements currently in the vector
    int32_t num_elements;

    // Checksum of the vector elements, doesn't include the header fields.
    //
    // TODO(cassiewang): Add a checksum state that can track if the checksum is
    // fresh or stale. This lets us short circuit checksum computations if we
    // know the checksum is fresh.
    uint32_t vector_checksum;

    // Must be below all actual header content fields and above the padding
    // field. Contains the crc checksum of the preceding fields.
    uint32_t header_checksum;

    // This field has no actual meaning here but is just used as padding for the
    // struct so the size of the struct can be a multiple of 8. Doing this makes
    // the address right after the header a multiple of 8 and prevents a ubsan
    // misalign-pointer-use error (go/ubsan).
    //
    // NOTE: please remove this when adding new fields and re-assert that the
    // size is multiple of 8.
    int32_t padding_for_ptr_alignment;

    uint32_t CalculateHeaderChecksum() const {
      // Sanity check that the memory layout matches the disk layout.
      static_assert(std::is_standard_layout<FileBackedVector::Header>::value,
                    "");
      static_assert(sizeof(FileBackedVector::Header) == kHeaderSize, "");
      static_assert(
          sizeof(FileBackedVector::Header) % sizeof(void*) == 0,
          "Header has insufficient padding for void* pointer alignment");
      static_assert(offsetof(FileBackedVector::Header, header_checksum) ==
                        kHeaderChecksumOffset,
                    "");

      Crc32 crc;
      std::string_view header_str(
          reinterpret_cast<const char*>(this),
          offsetof(FileBackedVector::Header, header_checksum));
      crc.Append(header_str);
      return crc.Get();
    }
  };

  // Creates a new FileBackedVector to read/write content to.
  //
  // filesystem: Object to make system level calls
  // file_path : Specifies the file to persist the vector to; must be a path
  //             within a directory that already exists.
  // mmap_strategy : Strategy/optimizations to access the content in the vector,
  //                 see MemoryMappedFile::Strategy for more details
  //
  // Return:
  //   FAILED_PRECONDITION_ERROR if the file checksum doesn't match the stored
  //                             checksum.
  //   INTERNAL_ERROR on I/O errors.
  //   UNIMPLEMENTED_ERROR if created with strategy READ_WRITE_MANUAL_SYNC.
  static libtextclassifier3::StatusOr<std::unique_ptr<FileBackedVector<T>>>
  Create(const Filesystem& filesystem, const std::string& file_path,
         MemoryMappedFile::Strategy mmap_strategy);

  // Deletes the FileBackedVector
  //
  // filesystem: Object to make system level calls
  // file_path : Specifies the file the vector is persisted to.
  static libtextclassifier3::Status Delete(const Filesystem& filesystem,
                                           const std::string& file_path);

  // Not copyable
  FileBackedVector(const FileBackedVector&) = delete;
  FileBackedVector& operator=(const FileBackedVector&) = delete;

  // If the vector was created with
  // MemoryMappedFile::Strategy::READ_WRITE_AUTO_SYNC, then changes will be
  // synced by the system and the checksum will be updated.
  ~FileBackedVector();

  // Gets a copy of the element at idx.
  //
  // This is useful if you think the FileBackedVector may grow before you need
  // to access this return value. When the FileBackedVector grows, the
  // underlying mmap will be unmapped and remapped, which will invalidate any
  // pointers to the previously mapped region. Getting a copy will avoid
  // referencing the now-invalidated region.
  //
  // Returns:
  //   OUT_OF_RANGE_ERROR if idx < 0 or > num_elements()
  libtextclassifier3::StatusOr<T> GetCopy(int32_t idx) const;

  // Gets a pointer to the element at idx.
  //
  // WARNING: Subsequent calls to Set may invalidate the pointer returned by
  // Get.
  //
  // This is useful if you do not think the FileBackedVector will grow before
  // you need to reference this value, and you want to avoid a copy. When the
  // FileBackedVector grows, the underlying mmap will be unmapped and remapped,
  // which will invalidate this pointer to the previously mapped region.
  //
  // Returns:
  //   OUT_OF_RANGE_ERROR if idx < 0 or > num_elements()
  libtextclassifier3::StatusOr<const T*> Get(int32_t idx) const;

  // Writes the value at idx.
  //
  // May grow the underlying file and mmapped region as needed to fit the new
  // value. If it does grow, then any pointers to previous values returned
  // from Get() may be invalidated.
  //
  // Returns:
  //   OUT_OF_RANGE_ERROR if idx < 0 or file cannot be grown idx size
  libtextclassifier3::Status Set(int32_t idx, const T& value);

  // Resizes to first len elements. The crc is cleared on truncation and will be
  // updated on destruction, or once the client calls ComputeChecksum() or
  // PersistToDisk().
  //
  // Returns:
  //   OUT_OF_RANGE_ERROR if len < 0 or >= num_elements()
  libtextclassifier3::Status TruncateTo(int32_t new_num_elements);

  // Flushes content to underlying file.
  //
  // Returns:
  //   OK on success
  //   INTERNAL on I/O error
  libtextclassifier3::Status PersistToDisk();

  // Calculates and returns the disk usage in bytes. Rounds up to the nearest
  // block size.
  //
  // Returns:
  //   Disk usage on success
  //   INTERNAL_ERROR on IO error
  libtextclassifier3::StatusOr<int64_t> GetDiskUsage() const;

  // Returns the file size of the all the elements held in the vector. File size
  // is in bytes. This excludes the size of any internal metadata of the vector,
  // e.g. the vector's header.
  //
  // Returns:
  //   File size on success
  //   INTERNAL_ERROR on IO error
  libtextclassifier3::StatusOr<int64_t> GetElementsFileSize() const;

  // Accessors.
  const T* array() const {
    return reinterpret_cast<const T*>(mmapped_file_->region());
  }

  T* mutable_array() const {
    return reinterpret_cast<T*>(mmapped_file_->mutable_region());
  }

  int32_t num_elements() const { return header_->num_elements; }

  // Updates checksum of the vector contents and returns it.
  //
  // Returns:
  //   INTERNAL_ERROR if the vector's internal state is inconsistent
  libtextclassifier3::StatusOr<Crc32> ComputeChecksum();

 private:
  // We track partial updates to the array for crc updating. This
  // requires extra memory to keep track of original buffers but
  // allows for much faster crc re-computation. This is the frac limit
  // of byte len after which we will discard recorded changes and
  // recompute the entire crc instead.
  static constexpr int32_t kPartialCrcLimitDiv = 8;  // limit is 1/8th

  // Grow file by at least this many elements if array is growable.
  static constexpr int64_t kGrowElements = 1u << 14;  // 16K

  // Max number of elements that can be held by the vector.
  static constexpr int64_t kMaxNumElements = 1u << 20;  // 1M

  // Can only be created through the factory ::Create function
  FileBackedVector(const Filesystem& filesystem, const std::string& file_path,
                   std::unique_ptr<Header> header,
                   std::unique_ptr<MemoryMappedFile> mmapped_file);

  // Initialize a new FileBackedVector, and create the file.
  static libtextclassifier3::StatusOr<std::unique_ptr<FileBackedVector<T>>>
  InitializeNewFile(const Filesystem& filesystem, const std::string& file_path,
                    ScopedFd fd, MemoryMappedFile::Strategy mmap_strategy);

  // Initialize a FileBackedVector from an existing file.
  static libtextclassifier3::StatusOr<std::unique_ptr<FileBackedVector<T>>>
  InitializeExistingFile(const Filesystem& filesystem,
                         const std::string& file_path, ScopedFd fd,
                         MemoryMappedFile::Strategy mmap_strategy);

  // Grows the underlying file to hold at least num_elements
  //
  // Returns:
  //   OUT_OF_RANGE_ERROR if we can't grow to the specified size
  libtextclassifier3::Status GrowIfNecessary(int32_t num_elements);

  // Cached constructor params.
  const Filesystem* const filesystem_;
  const std::string file_path_;
  std::unique_ptr<Header> header_;
  std::unique_ptr<MemoryMappedFile> mmapped_file_;

  // Offset before which all the elements have been included in the calculation
  // of crc at the time it was calculated.
  int32_t changes_end_ = 0;

  // Offset of changes that have happened since the last crc update between [0,
  // changes_end_).
  std::vector<int32_t> changes_;

  // Buffer of the original elements that have been changed since the last crc
  // update. Will be cleared if the size grows too big.
  std::string saved_original_buffer_;

  // Keep track of all pages we touched so we can write them back to
  // disk.
  std::vector<bool> dirty_pages_;
};

template <typename T>
constexpr int32_t FileBackedVector<T>::kPartialCrcLimitDiv;

template <typename T>
constexpr int64_t FileBackedVector<T>::kGrowElements;

template <typename T>
constexpr int64_t FileBackedVector<T>::kMaxNumElements;

template <typename T>
libtextclassifier3::StatusOr<std::unique_ptr<FileBackedVector<T>>>
FileBackedVector<T>::Create(const Filesystem& filesystem,
                            const std::string& file_path,
                            MemoryMappedFile::Strategy mmap_strategy) {
  if (mmap_strategy == MemoryMappedFile::Strategy::READ_WRITE_MANUAL_SYNC) {
    // FileBackedVector's behavior of growing the file underneath the mmap is
    // inherently broken with MAP_PRIVATE. Growing the vector requires extending
    // the file size, then unmapping and then re-mmapping over the new, larger
    // file. But when we unmap, we lose all the vector's contents if they
    // weren't manually persisted. Either don't allow READ_WRITE_MANUAL_SYNC
    // vectors from growing, or make users aware of this somehow
    return absl_ports::UnimplementedError(
        "FileBackedVector currently doesn't support READ_WRITE_MANUAL_SYNC "
        "mmap strategy.");
  }

  ScopedFd fd(filesystem.OpenForWrite(file_path.c_str()));
  if (!fd.is_valid()) {
    return absl_ports::InternalError(
        absl_ports::StrCat("Failed to open ", file_path));
  }

  int64_t file_size = filesystem.GetFileSize(file_path.c_str());
  if (file_size == Filesystem::kBadFileSize) {
    return absl_ports::InternalError(
        absl_ports::StrCat("Bad file size for file ", file_path));
  }

  const bool new_file = file_size == 0;
  if (new_file) {
    return InitializeNewFile(filesystem, file_path, std::move(fd),
                             mmap_strategy);
  }
  return InitializeExistingFile(filesystem, file_path, std::move(fd),
                                mmap_strategy);
}

template <typename T>
libtextclassifier3::StatusOr<std::unique_ptr<FileBackedVector<T>>>
FileBackedVector<T>::InitializeNewFile(
    const Filesystem& filesystem, const std::string& file_path, ScopedFd fd,
    MemoryMappedFile::Strategy mmap_strategy) {
  // Create header.
  auto header = std::make_unique<Header>();
  header->magic = FileBackedVector<T>::Header::kMagic;
  header->element_size = sizeof(T);
  header->header_checksum = header->CalculateHeaderChecksum();

  // We use Write() here, instead of writing through the mmapped region
  // created below, so we can gracefully handle errors that occur when the
  // disk is full. See b/77309668 for details.
  if (!filesystem.PWrite(fd.get(), /*offset=*/0, header.get(),
                         sizeof(Header))) {
    return absl_ports::InternalError("Failed to write header");
  }

  // Constructor of MemoryMappedFile doesn't actually call mmap(), mmap()
  // happens on MemoryMappedFile::Remap(). So having a potentially unflushed fd
  // at this point shouldn't run into issues with a mmap of the same file. But
  // we'll close the fd just in case.
  fd.reset();
  auto mmapped_file =
      std::make_unique<MemoryMappedFile>(filesystem, file_path, mmap_strategy);

  return std::unique_ptr<FileBackedVector<T>>(new FileBackedVector<T>(
      filesystem, file_path, std::move(header), std::move(mmapped_file)));
}

template <typename T>
libtextclassifier3::StatusOr<std::unique_ptr<FileBackedVector<T>>>
FileBackedVector<T>::InitializeExistingFile(
    const Filesystem& filesystem, const std::string& file_path,
    const ScopedFd fd, MemoryMappedFile::Strategy mmap_strategy) {
  int64_t file_size = filesystem.GetFileSize(file_path.c_str());
  if (file_size < sizeof(FileBackedVector<T>::Header)) {
    return absl_ports::InternalError(
        absl_ports::StrCat("File header too short for ", file_path));
  }

  auto header = std::make_unique<Header>();
  if (!filesystem.PRead(fd.get(), header.get(), sizeof(Header),
                        /*offset=*/0)) {
    return absl_ports::InternalError(
        absl_ports::StrCat("Failed to read header of ", file_path));
  }

  // Make sure the header is still valid before we use any of its values. This
  // should technically be included in the header_checksum check below, but this
  // is a quick/fast check that can save us from an extra crc computation.
  if (header->kMagic != FileBackedVector<T>::Header::kMagic) {
    return absl_ports::InternalError(
        absl_ports::StrCat("Invalid header kMagic for ", file_path));
  }

  // Check header
  if (header->header_checksum != header->CalculateHeaderChecksum()) {
    return absl_ports::FailedPreconditionError(
        absl_ports::StrCat("Invalid header crc for ", file_path));
  }

  if (header->element_size != sizeof(T)) {
    return absl_ports::InternalError(IcingStringUtil::StringPrintf(
        "Inconsistent element size, expected %zd, actual %d", sizeof(T),
        header->element_size));
  }

  int64_t min_file_size = header->num_elements * sizeof(T) + sizeof(Header);
  if (min_file_size > file_size) {
    return absl_ports::InternalError(IcingStringUtil::StringPrintf(
        "Inconsistent file size, expected %" PRId64 ", actual %" PRId64,
        min_file_size, file_size));
  }

  // Mmap the content of the vector, excluding the header so its easier to
  // access elements from the mmapped region
  auto mmapped_file =
      std::make_unique<MemoryMappedFile>(filesystem, file_path, mmap_strategy);
  ICING_RETURN_IF_ERROR(
      mmapped_file->Remap(sizeof(Header), file_size - sizeof(Header)));

  // Check vector contents
  Crc32 vector_checksum;
  std::string_view vector_contents(
      reinterpret_cast<const char*>(mmapped_file->region()),
      header->num_elements * sizeof(T));
  vector_checksum.Append(vector_contents);

  if (vector_checksum.Get() != header->vector_checksum) {
    return absl_ports::FailedPreconditionError(
        absl_ports::StrCat("Invalid vector contents for ", file_path));
  }

  return std::unique_ptr<FileBackedVector<T>>(new FileBackedVector<T>(
      filesystem, file_path, std::move(header), std::move(mmapped_file)));
}

template <typename T>
libtextclassifier3::Status FileBackedVector<T>::Delete(
    const Filesystem& filesystem, const std::string& file_path) {
  if (!filesystem.DeleteFile(file_path.c_str())) {
    return absl_ports::InternalError(
        absl_ports::StrCat("Failed to delete file: ", file_path));
  }
  return libtextclassifier3::Status::OK;
}

template <typename T>
FileBackedVector<T>::FileBackedVector(
    const Filesystem& filesystem, const std::string& file_path,
    std::unique_ptr<Header> header,
    std::unique_ptr<MemoryMappedFile> mmapped_file)
    : filesystem_(&filesystem),
      file_path_(file_path),
      header_(std::move(header)),
      mmapped_file_(std::move(mmapped_file)),
      changes_end_(header_->num_elements) {}

template <typename T>
FileBackedVector<T>::~FileBackedVector() {
  if (mmapped_file_->strategy() ==
      MemoryMappedFile::Strategy::READ_WRITE_AUTO_SYNC) {
    if (!PersistToDisk().ok()) {
      ICING_LOG(WARNING)
          << "Failed to persist vector to disk while destructing "
          << file_path_;
    }
  }
}

template <typename T>
libtextclassifier3::StatusOr<T> FileBackedVector<T>::GetCopy(
    int32_t idx) const {
  ICING_ASSIGN_OR_RETURN(const T* value, Get(idx));
  return *value;
}

template <typename T>
libtextclassifier3::StatusOr<const T*> FileBackedVector<T>::Get(
    int32_t idx) const {
  if (idx < 0) {
    return absl_ports::OutOfRangeError(
        IcingStringUtil::StringPrintf("Index, %d, was less than 0", idx));
  }

  if (idx >= header_->num_elements) {
    return absl_ports::OutOfRangeError(IcingStringUtil::StringPrintf(
        "Index, %d, was greater than vector size, %d", idx,
        header_->num_elements));
  }

  return &array()[idx];
}

template <typename T>
libtextclassifier3::Status FileBackedVector<T>::Set(int32_t idx,
                                                    const T& value) {
  if (idx < 0) {
    return absl_ports::OutOfRangeError(
        IcingStringUtil::StringPrintf("Index, %d, was less than 0", idx));
  }

  ICING_RETURN_IF_ERROR(GrowIfNecessary(idx + 1));

  if (idx + 1 > header_->num_elements) {
    header_->num_elements = idx + 1;
  }

  if (mutable_array()[idx] == value) {
    // No need to update
    return libtextclassifier3::Status::OK;
  }

  // Cache original value to update crcs.
  if (idx < changes_end_) {
    // If we exceed kPartialCrcLimitDiv, clear changes_end_ to
    // revert to full CRC.
    if ((saved_original_buffer_.size() + sizeof(T)) *
            FileBackedVector<T>::kPartialCrcLimitDiv >
        changes_end_ * sizeof(T)) {
      ICING_VLOG(2) << "FileBackedVector change tracking limit exceeded";
      changes_.clear();
      saved_original_buffer_.clear();
      changes_end_ = 0;
      header_->vector_checksum = 0;
    } else {
      int32_t start_byte = idx * sizeof(T);

      changes_.push_back(idx);
      saved_original_buffer_.append(
          reinterpret_cast<char*>(const_cast<T*>(array())) + start_byte,
          sizeof(T));
    }
  }

  mutable_array()[idx] = value;
  return libtextclassifier3::Status::OK;
}

template <typename T>
libtextclassifier3::Status FileBackedVector<T>::GrowIfNecessary(
    int32_t num_elements) {
  if (sizeof(T) == 0) {
    // Growing is a no-op
    return libtextclassifier3::Status::OK;
  }

  if (num_elements <= header_->num_elements) {
    return libtextclassifier3::Status::OK;
  }

  if (num_elements > FileBackedVector<T>::kMaxNumElements) {
    return absl_ports::OutOfRangeError(IcingStringUtil::StringPrintf(
        "%d exceeds maximum number of elements allowed, %lld", num_elements,
        static_cast<long long>(FileBackedVector<T>::kMaxNumElements)));
  }

  int64_t current_file_size = filesystem_->GetFileSize(file_path_.c_str());
  int64_t least_file_size_needed = sizeof(Header) + num_elements * sizeof(T);

  if (least_file_size_needed <= current_file_size) {
    // Our underlying file can hold the target num_elements cause we've grown
    // before
    return libtextclassifier3::Status::OK;
  }

  // Otherwise, we need to grow. Grow to kGrowElements boundary.
  least_file_size_needed = math_util::RoundUpTo(
      least_file_size_needed,
      int64_t{FileBackedVector<T>::kGrowElements * sizeof(T)});

  // We use PWrite here rather than Grow because Grow doesn't actually allocate
  // an underlying disk block. This can lead to problems with mmap because mmap
  // has no effective way to signal that it was impossible to allocate the disk
  // block and ends up crashing instead. PWrite will force the allocation of
  // these blocks, which will ensure that any failure to grow will surface here.
  int64_t page_size = getpagesize();
  auto buf = std::make_unique<uint8_t[]>(page_size);
  int64_t size_to_write = page_size - (current_file_size % page_size);
  ScopedFd sfd(filesystem_->OpenForWrite(file_path_.c_str()));
  while (current_file_size < least_file_size_needed) {
    if (!filesystem_->PWrite(sfd.get(), current_file_size, buf.get(),
                             size_to_write)) {
      return absl_ports::InternalError(
          absl_ports::StrCat("Couldn't grow file ", file_path_));
    }
    current_file_size += size_to_write;
    size_to_write = page_size - (current_file_size % page_size);
  }

  ICING_RETURN_IF_ERROR(mmapped_file_->Remap(
      sizeof(Header), least_file_size_needed - sizeof(Header)));

  return libtextclassifier3::Status::OK;
}

template <typename T>
libtextclassifier3::Status FileBackedVector<T>::TruncateTo(
    int32_t new_num_elements) {
  if (new_num_elements < 0) {
    return absl_ports::OutOfRangeError(IcingStringUtil::StringPrintf(
        "Truncated length %d must be >= 0", new_num_elements));
  }

  if (new_num_elements >= header_->num_elements) {
    return absl_ports::OutOfRangeError(IcingStringUtil::StringPrintf(
        "Truncated length %d must be less than the current size %d",
        new_num_elements, header_->num_elements));
  }

  ICING_VLOG(2)
      << "FileBackedVector truncating, need to recalculate entire checksum";
  changes_.clear();
  saved_original_buffer_.clear();
  changes_end_ = 0;
  header_->vector_checksum = 0;

  header_->num_elements = new_num_elements;
  return libtextclassifier3::Status::OK;
}

template <typename T>
libtextclassifier3::StatusOr<Crc32> FileBackedVector<T>::ComputeChecksum() {
  // First apply the modified area. Keep a bitmap of already updated
  // regions so we don't double-update.
  std::vector<bool> updated(changes_end_);
  uint32_t cur_offset = 0;
  Crc32 cur_crc(header_->vector_checksum);
  int num_partial_crcs = 0;
  int num_truncated = 0;
  int num_overlapped = 0;
  int num_duplicate = 0;
  for (size_t i = 0; i < changes_.size(); i++) {
    const int32_t change_offset = changes_[i];
    if (change_offset > changes_end_) {
      return absl_ports::InternalError(IcingStringUtil::StringPrintf(
          "Failed to update crc, change offset %d, changes_end_ %d",
          change_offset, changes_end_));
    }

    // Skip truncated tracked changes.
    if (change_offset >= header_->num_elements) {
      ++num_truncated;
      continue;
    }

    // Turn change buffer into change^original.
    const char* buffer_end = &saved_original_buffer_[cur_offset + sizeof(T)];
    const char* cur_array =
        reinterpret_cast<const char*>(array()) + change_offset * sizeof(T);
    // Now xor in. SSE acceleration please?
    for (char* cur = &saved_original_buffer_[cur_offset]; cur < buffer_end;
         cur++, cur_array++) {
      *cur ^= *cur_array;
    }

    // Skip over already updated bytes by setting update to 0.
    bool new_update = false;
    bool overlap = false;
    uint32_t cur_element = change_offset;
    for (char* cur = &saved_original_buffer_[cur_offset]; cur < buffer_end;
         cur_element++, cur += sizeof(T)) {
      if (updated[cur_element]) {
        memset(cur, 0, sizeof(T));
        overlap = true;
      } else {
        updated[cur_element] = true;
        new_update = true;
      }
    }

    // Apply update to crc.
    if (new_update) {
      // Explicitly create the string_view with length
      std::string_view xored_str(buffer_end - sizeof(T), sizeof(T));
      if (!cur_crc
               .UpdateWithXor(xored_str, changes_end_ * sizeof(T),
                              change_offset * sizeof(T))
               .ok()) {
        return absl_ports::InternalError(IcingStringUtil::StringPrintf(
            "Failed to update crc, change offset %d, change "
            "length %zd changes_end_ %d",
            change_offset, xored_str.length(), changes_end_));
      }
      num_partial_crcs++;
      if (overlap) {
        num_overlapped++;
      }
    } else {
      num_duplicate++;
    }
    cur_offset += sizeof(T);
  }

  if (!changes_.empty()) {
    ICING_VLOG(2) << IcingStringUtil::StringPrintf(
        "Array update partial crcs %d truncated %d overlapped %d duplicate %d",
        num_partial_crcs, num_truncated, num_overlapped, num_duplicate);
  }

  // Now update with grown area.
  if (changes_end_ < header_->num_elements) {
    // Explicitly create the string_view with length
    std::string_view update_str(
        reinterpret_cast<const char*>(array()) + changes_end_ * sizeof(T),
        (header_->num_elements - changes_end_) * sizeof(T));
    cur_crc.Append(update_str);
    ICING_VLOG(2) << IcingStringUtil::StringPrintf(
        "Array update tail crc offset %d -> %d", changes_end_,
        header_->num_elements);
  }

  // Clear, now that we've applied changes.
  changes_.clear();
  saved_original_buffer_.clear();
  changes_end_ = header_->num_elements;

  // Commit new crc.
  header_->vector_checksum = cur_crc.Get();
  return cur_crc;
}

template <typename T>
libtextclassifier3::Status FileBackedVector<T>::PersistToDisk() {
  // Update and write the header
  ICING_ASSIGN_OR_RETURN(Crc32 checksum, ComputeChecksum());
  header_->vector_checksum = checksum.Get();
  header_->header_checksum = header_->CalculateHeaderChecksum();

  if (!filesystem_->PWrite(file_path_.c_str(), /*offset=*/0, header_.get(),
                           sizeof(Header))) {
    return absl_ports::InternalError("Failed to sync header");
  }

  MemoryMappedFile::Strategy strategy = mmapped_file_->strategy();

  if (strategy == MemoryMappedFile::Strategy::READ_WRITE_AUTO_SYNC) {
    // Changes should have been applied to the underlying file, but call msync()
    // as an extra safety step to ensure they are written out.
    ICING_RETURN_IF_ERROR(mmapped_file_->PersistToDisk());
  }

  return libtextclassifier3::Status::OK;
}

template <typename T>
libtextclassifier3::StatusOr<int64_t> FileBackedVector<T>::GetDiskUsage()
    const {
  int64_t size = filesystem_->GetDiskUsage(file_path_.c_str());
  if (size == Filesystem::kBadFileSize) {
    return absl_ports::InternalError(
        "Failed to get disk usage of file-backed vector");
  }
  return size;
}

template <typename T>
libtextclassifier3::StatusOr<int64_t> FileBackedVector<T>::GetElementsFileSize()
    const {
  int64_t total_file_size = filesystem_->GetFileSize(file_path_.c_str());
  if (total_file_size == Filesystem::kBadFileSize) {
    return absl_ports::InternalError(
        "Failed to get file size of elements in the file-backed vector");
  }
  return total_file_size - sizeof(Header);
}

}  // namespace lib
}  // namespace icing

#endif  // ICING_FILE_FILE_BACKED_VECTOR_H_
