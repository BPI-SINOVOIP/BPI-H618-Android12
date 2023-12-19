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

// TODO(cassiewang) Add unit-tests to this class.

#include "icing/file/memory-mapped-file.h"

#include <sys/mman.h>

#include <cerrno>

#include "icing/text_classifier/lib3/utils/base/status.h"
#include "icing/absl_ports/canonical_errors.h"
#include "icing/absl_ports/str_cat.h"
#include "icing/file/filesystem.h"
#include "icing/legacy/core/icing-string-util.h"
#include "icing/util/math-util.h"

namespace icing {
namespace lib {

MemoryMappedFile::MemoryMappedFile(const Filesystem& filesystem,
                                   const std::string_view file_path,
                                   Strategy mmap_strategy)
    : filesystem_(&filesystem),
      file_path_(file_path),
      strategy_(mmap_strategy) {}

MemoryMappedFile::MemoryMappedFile(MemoryMappedFile&& other)
    // Make sure that mmap_result_ is a nullptr before we call Swap. We don't
    // care what values the remaining members hold before we swap into other,
    // but if mmap_result_ holds a non-NULL value before we initialized anything
    // then other will try to free memory at that address when it's destroyed!
    : mmap_result_(nullptr) {
  Swap(&other);
}

MemoryMappedFile& MemoryMappedFile::operator=(MemoryMappedFile&& other) {
  // Swap all of our elements with other. This will ensure that both this now
  // holds other's previous resources and that this's previous resources will be
  // properly freed when other is destructed at the end of this function.
  Swap(&other);
  return *this;
}

MemoryMappedFile::~MemoryMappedFile() { Unmap(); }

void MemoryMappedFile::MemoryMappedFile::Unmap() {
  if (mmap_result_ != nullptr) {
    munmap(mmap_result_, adjusted_mmap_size_);
    mmap_result_ = nullptr;
  }

  file_offset_ = 0;
  region_ = nullptr;
  region_size_ = 0;
  adjusted_mmap_size_ = 0;
}

libtextclassifier3::Status MemoryMappedFile::Remap(size_t file_offset,
                                                   size_t mmap_size) {
  // First unmap any previously mmapped region.
  Unmap();

  if (mmap_size == 0) {
    // Nothing more to do.
    return libtextclassifier3::Status::OK;
  }

  size_t aligned_offset =
      math_util::RoundDownTo(file_offset, system_page_size());
  size_t alignment_adjustment = file_offset - aligned_offset;
  size_t adjusted_mmap_size = alignment_adjustment + mmap_size;

  int mmap_flags = 0;
  // Determines if the mapped region should just be readable or also writable.
  int protection_flags = 0;
  ScopedFd fd;
  switch (strategy_) {
    case Strategy::READ_ONLY: {
      mmap_flags = MAP_PRIVATE;
      protection_flags = PROT_READ;
      fd.reset(filesystem_->OpenForRead(file_path_.c_str()));
      break;
    }
    case Strategy::READ_WRITE_AUTO_SYNC: {
      mmap_flags = MAP_SHARED;
      protection_flags = PROT_READ | PROT_WRITE;
      fd.reset(filesystem_->OpenForWrite(file_path_.c_str()));
      break;
    }
    case Strategy::READ_WRITE_MANUAL_SYNC: {
      mmap_flags = MAP_PRIVATE;
      protection_flags = PROT_READ | PROT_WRITE;
      // TODO(cassiewang) MAP_PRIVATE effectively makes it a read-only file.
      // figure out if we can open this file in read-only mode.
      fd.reset(filesystem_->OpenForWrite(file_path_.c_str()));
      break;
    }
    default:
      return absl_ports::UnknownError(IcingStringUtil::StringPrintf(
          "Invalid value in switch statement: %d", strategy_));
  }

  if (!fd.is_valid()) {
    return absl_ports::InternalError(absl_ports::StrCat(
        "Unable to open file meant to be mmapped: ", file_path_));
  }

  mmap_result_ = mmap(nullptr, adjusted_mmap_size, protection_flags, mmap_flags,
                      fd.get(), aligned_offset);

  if (mmap_result_ == MAP_FAILED) {
    mmap_result_ = nullptr;
    return absl_ports::InternalError(absl_ports::StrCat(
        "Failed to mmap region due to error: ", strerror(errno)));
  }

  file_offset_ = file_offset;
  region_ = reinterpret_cast<char*>(mmap_result_) + alignment_adjustment;
  region_size_ = mmap_size;
  adjusted_mmap_size_ = adjusted_mmap_size;
  return libtextclassifier3::Status::OK;
}

libtextclassifier3::Status MemoryMappedFile::PersistToDisk() {
  if (strategy_ == Strategy::READ_ONLY) {
    return absl_ports::FailedPreconditionError(absl_ports::StrCat(
        "Attempting to PersistToDisk on a read-only file: ", file_path_));
  }

  if (region_ == nullptr) {
    // Nothing mapped to sync.
    return libtextclassifier3::Status::OK;
  }

  if (strategy_ == Strategy::READ_WRITE_AUTO_SYNC &&
      msync(mmap_result_, adjusted_mmap_size_, MS_SYNC) != 0) {
    return absl_ports::InternalError(
        absl_ports::StrCat("Unable to sync file using msync(): ", file_path_));
  }

  // In order to prevent automatic syncing of changes, files that use the
  // READ_WRITE_MANUAL_SYNC strategy are mmapped using MAP_PRIVATE. Such files
  // can't be synced using msync(). So, we have to directly write to the
  // underlying file to update it.
  if (strategy_ == Strategy::READ_WRITE_MANUAL_SYNC &&
      !filesystem_->PWrite(file_path_.c_str(), 0, region(), region_size())) {
    return absl_ports::InternalError(
        absl_ports::StrCat("Unable to sync file using PWrite(): ", file_path_));
  }

  return libtextclassifier3::Status::OK;
}

libtextclassifier3::Status MemoryMappedFile::OptimizeFor(
    AccessPattern access_pattern) {
  int madvise_flag = 0;
  if (access_pattern == AccessPattern::ACCESS_ALL) {
    madvise_flag = MADV_WILLNEED;
  } else if (access_pattern == AccessPattern::ACCESS_NONE) {
    madvise_flag = MADV_DONTNEED;
  } else if (access_pattern == AccessPattern::ACCESS_RANDOM) {
    madvise_flag = MADV_RANDOM;
  } else if (access_pattern == AccessPattern::ACCESS_SEQUENTIAL) {
    madvise_flag = MADV_SEQUENTIAL;
  }

  if (madvise(mmap_result_, adjusted_mmap_size_, madvise_flag) != 0) {
    return absl_ports::InternalError(absl_ports::StrCat(
        "Unable to madvise file ", file_path_, "; Error: ", strerror(errno)));
  }
  return libtextclassifier3::Status::OK;
}

void MemoryMappedFile::Swap(MemoryMappedFile* other) {
  std::swap(filesystem_, other->filesystem_);
  std::swap(file_path_, other->file_path_);
  std::swap(strategy_, other->strategy_);
  std::swap(file_offset_, other->file_offset_);
  std::swap(region_, other->region_);
  std::swap(region_size_, other->region_size_);
  std::swap(adjusted_mmap_size_, other->adjusted_mmap_size_);
  std::swap(mmap_result_, other->mmap_result_);
}

}  // namespace lib
}  // namespace icing
