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

#include "icing/index/main/index-block.h"

#include <inttypes.h>

#include <algorithm>
#include <limits>

#include "icing/text_classifier/lib3/utils/base/statusor.h"
#include "icing/absl_ports/canonical_errors.h"
#include "icing/file/memory-mapped-file.h"
#include "icing/index/main/posting-list-free.h"
#include "icing/index/main/posting-list-utils.h"
#include "icing/legacy/core/icing-string-util.h"
#include "icing/util/math-util.h"
#include "icing/util/status-macros.h"

namespace icing {
namespace lib {

namespace {

libtextclassifier3::Status ValidatePostingListBytes(uint32_t posting_list_bytes,
                                                    uint32_t block_size) {
  if (posting_list_bytes >
          IndexBlock::CalculateMaxPostingListBytes(block_size) ||
      !posting_list_utils::IsValidPostingListSize(posting_list_bytes)) {
    return absl_ports::InvalidArgumentError(IcingStringUtil::StringPrintf(
        "Requested posting list size %d is illegal for a flash block with max "
        "posting list size of %d",
        posting_list_bytes,
        IndexBlock::CalculateMaxPostingListBytes(block_size)));
  }
  return libtextclassifier3::Status::OK;
}

}  // namespace

uint32_t IndexBlock::ApproximateFullPostingListHitsForBlock(
    uint32_t block_size, int posting_list_index_bits) {
  // Assume 50% compressed and most don't have term frequencies.
  uint32_t bytes_per_hit = sizeof(Hit::Value) / 2;
  return (block_size - sizeof(BlockHeader)) /
         ((1u << posting_list_index_bits) * bytes_per_hit);
}

libtextclassifier3::StatusOr<IndexBlock>
IndexBlock::CreateFromPreexistingIndexBlockRegion(const Filesystem& filesystem,
                                                  std::string_view file_path,
                                                  off_t offset,
                                                  uint32_t block_size) {
  if (block_size < sizeof(BlockHeader)) {
    return absl_ports::InvalidArgumentError(IcingStringUtil::StringPrintf(
        "Provided block_size %d is too small to fit even the BlockHeader!",
        block_size));
  }
  MemoryMappedFile mmapped_file(
      filesystem, file_path, MemoryMappedFile::Strategy::READ_WRITE_AUTO_SYNC);
  ICING_RETURN_IF_ERROR(mmapped_file.Remap(offset, block_size));
  IndexBlock block(std::move(mmapped_file));
  ICING_RETURN_IF_ERROR(
      ValidatePostingListBytes(block.get_posting_list_bytes(), block_size));
  return block;
}

libtextclassifier3::StatusOr<IndexBlock>
IndexBlock::CreateFromUninitializedRegion(const Filesystem& filesystem,
                                          std::string_view file_path,
                                          off_t offset, uint32_t block_size,
                                          uint32_t posting_list_bytes) {
  if (block_size < sizeof(BlockHeader)) {
    return absl_ports::InvalidArgumentError(IcingStringUtil::StringPrintf(
        "Provided block_size %d is too small to fit even the BlockHeader!",
        block_size));
  }
  ICING_RETURN_IF_ERROR(
      ValidatePostingListBytes(posting_list_bytes, block_size));
  MemoryMappedFile mmapped_file(
      filesystem, file_path, MemoryMappedFile::Strategy::READ_WRITE_AUTO_SYNC);
  ICING_RETURN_IF_ERROR(mmapped_file.Remap(offset, block_size));
  IndexBlock block(std::move(mmapped_file));
  // Safe to ignore the return value of Reset. Reset returns an error if
  // posting_list_bytes is invalid, but this function ensures that
  // posting_list_bytes is valid thanks to the call to ValidatePostingListBytes
  // above.
  block.Reset(posting_list_bytes);
  return block;
}

IndexBlock::IndexBlock(MemoryMappedFile mmapped_block)
    : header_(reinterpret_cast<BlockHeader*>(mmapped_block.mutable_region())),
      posting_lists_start_ptr_(mmapped_block.mutable_region() +
                               sizeof(BlockHeader)),
      block_size_in_bytes_(mmapped_block.region_size()),
      mmapped_block_(
          std::make_unique<MemoryMappedFile>(std::move(mmapped_block))) {}

libtextclassifier3::Status IndexBlock::Reset(int posting_list_bytes) {
  ICING_RETURN_IF_ERROR(ValidatePostingListBytes(
      posting_list_bytes, mmapped_block_->region_size()));
  header_->free_list_posting_list_index = kInvalidPostingListIndex;
  header_->next_block_index = kInvalidBlockIndex;
  header_->posting_list_bytes = posting_list_bytes;

  // Starting with the last posting list, prepend each posting list to the free
  // list. At the end, the beginning of the free list should be the first
  // posting list.
  for (PostingListIndex posting_list_index = max_num_posting_lists() - 1;
       posting_list_index >= 0; --posting_list_index) {
    // Adding the posting list at posting_list_index to the free list will
    // modify both the posting list and also
    // header_->free_list_posting_list_index.
    FreePostingList(posting_list_index);
  }
  return libtextclassifier3::Status::OK;
}

libtextclassifier3::StatusOr<PostingListUsed>
IndexBlock::GetAllocatedPostingList(PostingListIndex posting_list_index) {
  if (posting_list_index >= max_num_posting_lists() || posting_list_index < 0) {
    return absl_ports::InvalidArgumentError(IcingStringUtil::StringPrintf(
        "Cannot get posting list with index %d in IndexBlock with only %d "
        "posting lists.",
        posting_list_index, max_num_posting_lists()));
  }
  return PostingListUsed::CreateFromPreexistingPostingListUsedRegion(
      get_posting_list_ptr(posting_list_index), get_posting_list_bytes());
}

libtextclassifier3::StatusOr<PostingListIndex>
IndexBlock::AllocatePostingList() {
  if (!has_free_posting_lists()) {
    return absl_ports::ResourceExhaustedError(
        "No available posting lists to allocate.");
  }

  // Pull one off the free list.
  PostingListIndex posting_list_index = header_->free_list_posting_list_index;

  // We know at this point that posting_list_bytes will return a valid pl size
  // (because an already initialized IndexBlock instance can't have an invalid
  // posting_list_bytes). So CreateFromPreexistingPostingListFreeRegion will
  // always return OK and ValueOrDie is safe to call.
  auto posting_list_or =
      PostingListFree::CreateFromPreexistingPostingListFreeRegion(
          get_posting_list_ptr(posting_list_index), get_posting_list_bytes());
  PostingListFree plfree = std::move(posting_list_or).ValueOrDie();

  header_->free_list_posting_list_index = plfree.get_next_posting_list_index();
  if (header_->free_list_posting_list_index != kInvalidPostingListIndex &&
      header_->free_list_posting_list_index >= max_num_posting_lists()) {
    ICING_LOG(ERROR)
        << "Free Posting List points to an invalid posting list index!";
    header_->free_list_posting_list_index = kInvalidPostingListIndex;
  }

  // Make it a used posting list.
  PostingListUsed::CreateFromUnitializedRegion(
      get_posting_list_ptr(posting_list_index), get_posting_list_bytes());
  return posting_list_index;
}

void IndexBlock::FreePostingList(PostingListIndex posting_list_index) {
  if (posting_list_index >= max_num_posting_lists() || posting_list_index < 0) {
    ICING_LOG(ERROR) << "Cannot free posting list with index "
                     << posting_list_index << " in IndexBlock with only "
                     << max_num_posting_lists() << " posting lists.";
    return;
  }

  // We know at this point that posting_list_bytes will return a valid pl size.
  // So CreateFromUninitializedRegion will always return OK and ValueOrDie is
  // safe to call.
  auto posting_list_or = PostingListFree::CreateFromUnitializedRegion(
      get_posting_list_ptr(posting_list_index), get_posting_list_bytes());
  PostingListFree plfree = std::move(posting_list_or).ValueOrDie();

  // Put at the head of the list.
  plfree.set_next_posting_list_index(header_->free_list_posting_list_index);
  header_->free_list_posting_list_index = posting_list_index;
}

char* IndexBlock::get_posting_list_ptr(PostingListIndex posting_list_index) {
  return posting_lists_start_ptr_ +
         get_posting_list_bytes() * posting_list_index;
}

}  // namespace lib
}  // namespace icing
