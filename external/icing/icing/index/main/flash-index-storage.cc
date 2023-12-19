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

#include "icing/index/main/flash-index-storage.h"

#include <errno.h>
#include <inttypes.h>
#include <sys/types.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <unordered_set>

#include "icing/absl_ports/canonical_errors.h"
#include "icing/absl_ports/str_cat.h"
#include "icing/file/memory-mapped-file.h"
#include "icing/index/main/index-block.h"
#include "icing/index/main/posting-list-free.h"
#include "icing/index/main/posting-list-utils.h"
#include "icing/legacy/core/icing-string-util.h"
#include "icing/util/logging.h"
#include "icing/util/math-util.h"
#include "icing/util/status-macros.h"

namespace icing {
namespace lib {

namespace {

uint32_t SelectBlockSize() {
  // This should be close to the flash page size.
  static constexpr uint32_t kMinBlockSize = 4096;

  // Determine a good block size.
  uint32_t page_size = getpagesize();
  uint32_t block_size = std::max(kMinBlockSize, page_size);

  // Align up to the nearest page size.
  return math_util::RoundUpTo(block_size, page_size);
}

}  // namespace

libtextclassifier3::StatusOr<FlashIndexStorage> FlashIndexStorage::Create(
    const std::string& index_filename, const Filesystem* filesystem,
    bool in_memory) {
  ICING_RETURN_ERROR_IF_NULL(filesystem);
  FlashIndexStorage storage(index_filename, filesystem, in_memory);
  if (!storage.Init()) {
    return absl_ports::InternalError(
        "Unable to successfully read header block!");
  }
  return storage;
}

FlashIndexStorage::FlashIndexStorage(const std::string& index_filename,
                                     const Filesystem* filesystem,
                                     bool has_in_memory_freelists)
    : index_filename_(index_filename),
      num_blocks_(0),
      filesystem_(filesystem),
      has_in_memory_freelists_(has_in_memory_freelists) {}

FlashIndexStorage::~FlashIndexStorage() {
  if (header_block_ != nullptr) {
    FlushInMemoryFreeList();
    PersistToDisk();
  }
}

bool FlashIndexStorage::Init() {
  block_fd_ = ScopedFd(filesystem_->OpenForWrite(index_filename_.c_str()));
  if (!block_fd_.is_valid()) {
    return false;
  }

  // Read in or create the header.
  return InitHeader();
}

bool FlashIndexStorage::InitHeader() {
  // Look for an existing file size.
  int64_t file_size = filesystem_->GetFileSize(block_fd_.get());
  if (file_size == Filesystem::kBadFileSize) {
    ICING_LOG(ERROR) << "Could not initialize main index. Bad file size.";
    return false;
  }

  if (file_size == 0) {
    if (!CreateHeader()) {
      ICING_LOG(ERROR)
          << "Could not initialize main index. Unable to create header.";
      return false;
    }
  } else {
    if (!OpenHeader(file_size)) {
      ICING_LOG(ERROR)
          << "Could not initialize main index. Unable to open header.";
      return false;
    }
  }
  in_memory_freelists_.resize(header_block_->header()->num_index_block_infos);

  return true;
}

bool FlashIndexStorage::CreateHeader() {
  uint32_t block_size = SelectBlockSize();
  header_block_ = std::make_unique<HeaderBlock>(filesystem_, block_size);
  // Initialize.
  header_block_->header()->magic = HeaderBlock::Header::kMagic;
  header_block_->header()->block_size = block_size;
  header_block_->header()->last_indexed_docid = kInvalidDocumentId;

  // Work down from the largest posting list that fits in
  // block_size. We don't care about locality of blocks because this
  // is a flash index.
  for (uint32_t posting_list_bytes =
           IndexBlock::CalculateMaxPostingListBytes(block_size);
       posting_list_bytes >= posting_list_utils::min_posting_list_size();
       posting_list_bytes /= 2) {
    uint32_t aligned_posting_list_bytes =
        (posting_list_bytes / sizeof(Hit) * sizeof(Hit));
    ICING_VLOG(1) << IcingStringUtil::StringPrintf(
        "Block size %u: %u", header_block_->header()->num_index_block_infos,
        aligned_posting_list_bytes);

    // Initialize free list to empty.
    HeaderBlock::Header::IndexBlockInfo* block_info =
        header_block_->AddIndexBlockInfo();
    if (block_info == nullptr) {
      // This should never happen anyways. Min block size is 4k, so adding these
      // IndexBlockInfos should never exceed the block size.
      return false;
    }
    block_info->posting_list_bytes = aligned_posting_list_bytes;
    block_info->free_list_block_index = kInvalidBlockIndex;
  }

  // Write the header.
  if (!header_block_->Write(block_fd_.get())) {
    filesystem_->Truncate(block_fd_.get(), 0);
    return false;
  }
  num_blocks_ = 1;
  return true;
}

bool FlashIndexStorage::OpenHeader(int64_t file_size) {
  uint32_t block_size = SelectBlockSize();
  // Read and validate header.
  ICING_ASSIGN_OR_RETURN(
      HeaderBlock read_header,
      HeaderBlock::Read(filesystem_, block_fd_.get(), block_size), false);
  if (read_header.header()->magic != HeaderBlock::Header::kMagic) {
    ICING_LOG(ERROR) << "Index header block wrong magic";
    return false;
  }
  if (file_size % read_header.header()->block_size != 0) {
    ICING_LOG(ERROR) << IcingStringUtil::StringPrintf(
        "Index size %" PRIu64 " not a multiple of block size %u", file_size,
        read_header.header()->block_size);
    return false;
  }

  if (file_size < static_cast<int64_t>(read_header.header()->block_size)) {
    ICING_LOG(ERROR) << IcingStringUtil::StringPrintf(
        "Index size %" PRIu64 " shorter than block size %u", file_size,
        read_header.header()->block_size);
    return false;
  }

  if (read_header.header()->block_size % getpagesize() != 0) {
    ICING_LOG(ERROR) << IcingStringUtil::StringPrintf(
        "Block size %u is not a multiple of page size %d",
        read_header.header()->block_size, getpagesize());
    return false;
  }
  num_blocks_ = file_size / read_header.header()->block_size;
  if (block_size != read_header.header()->block_size) {
    // The block_size changed? That's weird. But the old block_size is still
    // valid (it must be some multiple of the new block_size). So reinitialize
    // with that old block size. Using the old block size means that we can
    // still use the main index, but reads/writes won't be as efficient in terms
    // of flash IO because the 'blocks' that we're reading are actually multiple
    // pages long.
    ICING_LOG(ERROR) << "Block size of existing header ("
                     << read_header.header()->block_size
                     << ") does not match the requested block size ("
                     << block_size << "). Defaulting to existing block size "
                     << read_header.header()->block_size;
    ICING_ASSIGN_OR_RETURN(HeaderBlock read_header,
                           HeaderBlock::Read(filesystem_, block_fd_.get(),
                                             read_header.header()->block_size),
                           false);
  }
  header_block_ = std::make_unique<HeaderBlock>(std::move(read_header));

  // Check for memory alignment on posting_list_bytes. See b/29983315.
  // The issue of potential corruption to the header could also be handled by
  // checksumming the header block.
  for (int i = 0; i < header_block_->header()->num_index_block_infos; ++i) {
    int posting_list_bytes =
        header_block_->header()->index_block_infos[i].posting_list_bytes;
    if (posting_list_bytes % sizeof(Hit) != 0) {
      ICING_LOG(ERROR) << IcingStringUtil::StringPrintf(
          "Posting list size misaligned, index %u, size %u, hit %zu, "
          "file_size %" PRIu64,
          i, header_block_->header()->index_block_infos[i].posting_list_bytes,
          sizeof(Hit), file_size);
      return false;
    }
  }
  return true;
}

bool FlashIndexStorage::PersistToDisk() {
  // First, write header.
  if (!header_block_->Write(block_fd_.get())) {
    ICING_LOG(ERROR) << IcingStringUtil::StringPrintf(
        "Write index header failed: %s", strerror(errno));
    return false;
  }

  // Then sync.
  return filesystem_->DataSync(block_fd_.get());
}

libtextclassifier3::Status FlashIndexStorage::Reset() {
  // Reset in-memory members to default values.
  num_blocks_ = 0;
  header_block_.reset();
  block_fd_.reset();
  in_memory_freelists_.clear();

  // Delete the underlying file.
  if (!filesystem_->DeleteFile(index_filename_.c_str())) {
    return absl_ports::InternalError(
        absl_ports::StrCat("Unable to delete file: ", index_filename_));
  }

  // Re-initialize.
  if (!Init()) {
    return absl_ports::InternalError(
        "Unable to successfully read header block!");
  }
  return libtextclassifier3::Status::OK;
}

libtextclassifier3::StatusOr<PostingListHolder>
FlashIndexStorage::GetPostingList(PostingListIdentifier id) const {
  ICING_ASSIGN_OR_RETURN(IndexBlock block, GetIndexBlock(id.block_index()));
  ICING_ASSIGN_OR_RETURN(
      PostingListUsed posting_list,
      block.GetAllocatedPostingList(id.posting_list_index()));
  PostingListHolder holder = {std::move(posting_list), std::move(block), id};
  return holder;
}

libtextclassifier3::StatusOr<IndexBlock> FlashIndexStorage::GetIndexBlock(
    int block_index) const {
  if (block_index >= num_blocks_) {
    return absl_ports::OutOfRangeError(IcingStringUtil::StringPrintf(
        "Unable to create an index block at index %d when only %d blocks have "
        "been allocated.",
        block_index, num_blocks_));
  }
  off_t offset = static_cast<off_t>(block_index) * block_size();
  return IndexBlock::CreateFromPreexistingIndexBlockRegion(
      *filesystem_, index_filename_, offset, block_size());
}

libtextclassifier3::StatusOr<IndexBlock> FlashIndexStorage::CreateIndexBlock(
    int block_index, uint32_t posting_list_size) const {
  if (block_index >= num_blocks_) {
    return absl_ports::OutOfRangeError(IcingStringUtil::StringPrintf(
        "Unable to create an index block at index %d when only %d blocks have "
        "been allocated.",
        block_index, num_blocks_));
  }
  off_t offset = static_cast<off_t>(block_index) * block_size();
  return IndexBlock::CreateFromUninitializedRegion(
      *filesystem_, index_filename_, offset, block_size(), posting_list_size);
}

int FlashIndexStorage::FindBestIndexBlockInfo(
    uint32_t posting_list_bytes) const {
  int i = header_block_->header()->num_index_block_infos - 1;
  for (; i >= 0; i--) {
    if (header_block_->header()->index_block_infos[i].posting_list_bytes >=
        posting_list_bytes) {
      return i;
    }
  }
  return i;
}

libtextclassifier3::StatusOr<PostingListHolder>
FlashIndexStorage::GetPostingListFromInMemoryFreeList(int block_info_index) {
  // Get something from in memory free list.
  ICING_ASSIGN_OR_RETURN(PostingListIdentifier posting_list_id,
                         in_memory_freelists_[block_info_index].TryPop());
  // Remember, posting lists stored on the in-memory free list were never
  // actually freed. So it will still contain a valid PostingListUsed. First, we
  // need to free this posting list.
  ICING_ASSIGN_OR_RETURN(IndexBlock block,
                         GetIndexBlock(posting_list_id.block_index()));
  block.FreePostingList(posting_list_id.posting_list_index());

  // Now, we can allocate a posting list from the same index block. It may not
  // be the same posting list that was just freed, but that's okay.
  ICING_ASSIGN_OR_RETURN(PostingListIndex posting_list_index,
                         block.AllocatePostingList());
  posting_list_id =
      PostingListIdentifier(posting_list_id.block_index(), posting_list_index,
                            posting_list_id.posting_list_index_bits());
  ICING_ASSIGN_OR_RETURN(
      PostingListUsed posting_list,
      block.GetAllocatedPostingList(posting_list_id.posting_list_index()));
  PostingListHolder holder = {std::move(posting_list), std::move(block),
                              posting_list_id};
  return holder;
}

libtextclassifier3::StatusOr<PostingListHolder>
FlashIndexStorage::GetPostingListFromOnDiskFreeList(int block_info_index) {
  // Get something from the free list.
  uint32_t block_index = header_block_->header()
                             ->index_block_infos[block_info_index]
                             .free_list_block_index;
  if (block_index == kInvalidBlockIndex) {
    return absl_ports::NotFoundError("No available entry in free list.");
  }

  // Get the index block
  ICING_ASSIGN_OR_RETURN(IndexBlock block, GetIndexBlock(block_index));
  ICING_ASSIGN_OR_RETURN(PostingListIndex posting_list_index,
                         block.AllocatePostingList());
  PostingListIdentifier posting_list_id = PostingListIdentifier(
      block_index, posting_list_index, block.posting_list_index_bits());
  ICING_ASSIGN_OR_RETURN(
      PostingListUsed posting_list,
      block.GetAllocatedPostingList(posting_list_id.posting_list_index()));
  if (!block.has_free_posting_lists()) {
    RemoveFromOnDiskFreeList(block_index, block_info_index, &block);
  }
  PostingListHolder holder = {std::move(posting_list), std::move(block),
                              posting_list_id};
  return holder;
}

libtextclassifier3::StatusOr<PostingListHolder>
FlashIndexStorage::AllocateNewPostingList(int block_info_index) {
  uint32_t block_index = GrowIndex();
  if (block_index == kInvalidBlockIndex) {
    return absl_ports::ResourceExhaustedError(
        "Unable to grow the index further!");
  }
  ICING_ASSIGN_OR_RETURN(
      IndexBlock block,
      CreateIndexBlock(block_index, header_block_->header()
                                        ->index_block_infos[block_info_index]
                                        .posting_list_bytes));
  ICING_ASSIGN_OR_RETURN(PostingListIndex posting_list_index,
                         block.AllocatePostingList());
  PostingListIdentifier posting_list_id = PostingListIdentifier(
      block_index, posting_list_index, block.posting_list_index_bits());
  ICING_ASSIGN_OR_RETURN(
      PostingListUsed posting_list,
      block.GetAllocatedPostingList(posting_list_id.posting_list_index()));
  if (block.has_free_posting_lists()) {
    AddToOnDiskFreeList(block_index, block_info_index, &block);
  }
  PostingListHolder holder = {std::move(posting_list), std::move(block),
                              posting_list_id};
  return holder;
}

libtextclassifier3::StatusOr<PostingListHolder>
FlashIndexStorage::AllocatePostingList(uint32_t min_posting_list_bytes) {
  int max_block_size = IndexBlock::CalculateMaxPostingListBytes(block_size());
  if (min_posting_list_bytes > max_block_size) {
    return absl_ports::InvalidArgumentError(IcingStringUtil::StringPrintf(
        "Requested posting list size %d exceeds max posting list size %d",
        min_posting_list_bytes, max_block_size));
  }
  int best_block_info_index = FindBestIndexBlockInfo(min_posting_list_bytes);

  auto holder_or = GetPostingListFromInMemoryFreeList(best_block_info_index);
  if (holder_or.ok()) {
    return std::move(holder_or).ValueOrDie();
  }

  // Nothing in memory. Look for something in the block file.
  holder_or = GetPostingListFromOnDiskFreeList(best_block_info_index);
  if (holder_or.ok()) {
    return std::move(holder_or).ValueOrDie();
  }

  return AllocateNewPostingList(best_block_info_index);
}

void FlashIndexStorage::AddToOnDiskFreeList(uint32_t block_index,
                                            int block_info_index,
                                            IndexBlock* index_block) {
  index_block->set_next_block_index(header_block_->header()
                                        ->index_block_infos[block_info_index]
                                        .free_list_block_index);
  header_block_->header()
      ->index_block_infos[block_info_index]
      .free_list_block_index = block_index;
}

void FlashIndexStorage::RemoveFromOnDiskFreeList(uint32_t block_index,
                                                 int block_info_index,
                                                 IndexBlock* index_block) {
  // Cannot be used anymore. Move free ptr to the next block.
  header_block_->header()
      ->index_block_infos[block_info_index]
      .free_list_block_index = index_block->next_block_index();
  index_block->set_next_block_index(kInvalidBlockIndex);
}

void FlashIndexStorage::FreePostingList(PostingListHolder holder) {
  uint32_t posting_list_bytes = holder.block.get_posting_list_bytes();
  int best_block_info_index = FindBestIndexBlockInfo(posting_list_bytes);

  // It *should* be guaranteed elsewhere that FindBestIndexBlockInfo will not
  // return a value in >= in_memory_freelists_, but check regardless. If it
  // doesn't fit for some reason, then put it in the Header free list instead.
  if (has_in_memory_freelists_ &&
      best_block_info_index < in_memory_freelists_.size()) {
    in_memory_freelists_[best_block_info_index].Push(holder.id);
  } else {
    bool was_full = !holder.block.has_free_posting_lists();
    holder.block.FreePostingList(holder.id.posting_list_index());
    // If this block was not already full, then it is already in the free list.
    if (was_full) {
      AddToOnDiskFreeList(holder.id.block_index(), best_block_info_index,
                          &holder.block);
    }
  }
}

int FlashIndexStorage::GrowIndex() {
  if (num_blocks_ >= kMaxBlockIndex) {
    ICING_VLOG(1) << IcingStringUtil::StringPrintf("Reached max block index %u",
                                                   kMaxBlockIndex);
    return kInvalidBlockIndex;
  }

  // Grow the index file.
  if (!filesystem_->Grow(
          block_fd_.get(),
          static_cast<uint64_t>(num_blocks_ + 1) * block_size())) {
    ICING_VLOG(1) << IcingStringUtil::StringPrintf(
        "Error growing index file: %s", strerror(errno));
    return kInvalidBlockIndex;
  }

  return num_blocks_++;
}

void FlashIndexStorage::FlushInMemoryFreeList() {
  for (int i = 0; i < in_memory_freelists_.size(); ++i) {
    FreeList& freelist = in_memory_freelists_.at(i);
    auto freelist_elt_or = freelist.TryPop();
    while (freelist_elt_or.ok()) {
      PostingListIdentifier freelist_elt = freelist_elt_or.ValueOrDie();
      // Remember, posting lists stored on the in-memory free list were never
      // actually freed. So it will still contain a valid PostingListUsed.
      // First, we need to free this posting list.
      auto block_or = GetIndexBlock(freelist_elt.block_index());
      if (!block_or.ok()) {
        // Can't read the block. Nothing to do here. This posting list will have
        // to leak. Just proceed to the next freelist element.
        freelist_elt_or = freelist.TryPop();
        continue;
      }
      IndexBlock block = std::move(block_or).ValueOrDie();
      bool was_full = !block.has_free_posting_lists();
      block.FreePostingList(freelist_elt.posting_list_index());
      // If this block was not already full, then it is already in the free
      // list.
      if (was_full) {
        AddToOnDiskFreeList(freelist_elt.block_index(), /*block_info_index=*/i,
                            &block);
      }
      freelist_elt_or = freelist.TryPop();
    }
  }
}

void FlashIndexStorage::GetDebugInfo(int verbosity, std::string* out) const {
  // Dump and check integrity of the index block free lists.
  out->append("Free lists:\n");
  for (size_t i = 0; i < header_block_->header()->num_index_block_infos; ++i) {
    // TODO(tjbarron) Port over StringAppendFormat to migrate off of this legacy
    // util.
    IcingStringUtil::SStringAppendF(
        out, 100, "Posting list bytes %u: ",
        header_block_->header()->index_block_infos[i].posting_list_bytes);
    uint32_t block_index =
        header_block_->header()->index_block_infos[i].free_list_block_index;
    int count = 0;
    while (block_index != kInvalidBlockIndex) {
      auto block_or = GetIndexBlock(block_index);
      IcingStringUtil::SStringAppendF(out, 100, "%u ", block_index);
      ++count;

      if (block_or.ok()) {
        block_index = block_or.ValueOrDie().next_block_index();
      } else {
        block_index = kInvalidBlockIndex;
      }
    }
    IcingStringUtil::SStringAppendF(out, 100, "(count=%d)\n", count);
  }

  out->append("In memory free lists:\n");
  if (in_memory_freelists_.size() ==
      header_block_->header()->num_index_block_infos) {
    for (size_t i = 0; i < in_memory_freelists_.size(); ++i) {
      IcingStringUtil::SStringAppendF(
          out, 100, "Posting list bytes %u %s\n",
          header_block_->header()->index_block_infos[i].posting_list_bytes,
          in_memory_freelists_.at(i).DebugString().c_str());
    }
  } else {
    IcingStringUtil::SStringAppendF(
        out, 100,
        "In memory free list size %zu doesn't match index block infos size "
        "%d\n",
        in_memory_freelists_.size(),
        header_block_->header()->num_index_block_infos);
  }
}

// FreeList.
void FlashIndexStorage::FreeList::Push(PostingListIdentifier id) {
  if (free_list_.size() >= kMaxSize) {
    ICING_LOG(WARNING)
        << "Freelist for posting lists of size (block_size / "
        << (1u << id.posting_list_index_bits())
        << ") has reached max size. Dropping freed posting list [block_index:"
        << id.block_index()
        << ", posting_list_index:" << id.posting_list_index() << "]";
    ++num_dropped_free_list_entries_;
    return;
  }

  free_list_.push_back(id);
  free_list_size_high_watermark_ = std::max(
      free_list_size_high_watermark_, static_cast<int>(free_list_.size()));
}

libtextclassifier3::StatusOr<PostingListIdentifier>
FlashIndexStorage::FreeList::TryPop() {
  if (free_list_.empty()) {
    return absl_ports::NotFoundError("No available entry in free list.");
  }

  PostingListIdentifier id = free_list_.back();
  free_list_.pop_back();
  return id;
}

std::string FlashIndexStorage::FreeList::DebugString() const {
  return IcingStringUtil::StringPrintf(
      "size %zu max %d dropped %d", free_list_.size(),
      free_list_size_high_watermark_, num_dropped_free_list_entries_);
}

}  // namespace lib
}  // namespace icing
