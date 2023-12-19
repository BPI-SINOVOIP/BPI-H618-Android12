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

#ifndef ICING_INDEX_FLASH_INDEX_STORAGE_H_
#define ICING_INDEX_FLASH_INDEX_STORAGE_H_

#include <cstdint>
#include <memory>
#include <string>

#include "icing/text_classifier/lib3/utils/base/statusor.h"
#include "icing/absl_ports/canonical_errors.h"
#include "icing/file/filesystem.h"
#include "icing/index/main/flash-index-storage-header.h"
#include "icing/index/main/index-block.h"
#include "icing/index/main/posting-list-free.h"
#include "icing/index/main/posting-list-identifier.h"
#include "icing/index/main/posting-list-used.h"
#include "icing/legacy/core/icing-packed-pod.h"
#include "icing/store/document-id.h"

namespace icing {
namespace lib {

// The PostingListHolder struct exists to group together related PostingListUsed
// IndexBlock pairs and their ids.
struct PostingListHolder {
  // PostingListUseds interpret data that they themselves do NOT own. The data
  // being interpreted is stored on a flash block and its memory mapping is
  // owned by the IndexBlock. As such, the lifecycle of the PostingListUsed must
  // NOT exceed the lifecycle of the IndexBlock.
  PostingListUsed posting_list;
  IndexBlock block;
  // The PostingListIdentifier, which identifies both the IndexBlock and the
  // PostingListUsed, is also returned for convenience.
  PostingListIdentifier id;
};

// The FlashIndexStorage class manages the actual file that makes up the index.
// It allocates IndexBlocks as needed and maintains freelists to prevent
// excessive block fragmentation.
//
// It maintains two types of free lists:
//   1. On-disk, Header free list - This free list is stored in the Header
//      block. There is a free list for every possible posting list size. Each
//      entry for a posting list size contains the block_index of the
//      IndexBlock that starts the free list chain. Each IndexBlock in the free
//      list chain stores the index of the next IndexBlock in the chain.
//   2. In-memory free list - Like the Header free list, there is a free list of
//      every possible posting list size. This free list contains not just the
//      block_index of the available IndexBlock, but also the posting_list_index
//      of the available PostingListUsed within the IndexBlock. This is because,
//      unlike the Header free list, PostingListUseds are not actually freed
//      when added to this free list.
//
// Whether or not the in-memory free list is used can be chosen via the
// in_memory param to the Create factory function.
//
// The advantage of using the in-memory free list is that it reduces the amount
// of flash writes made while editing the index (because actually freeing the
// PostingLists would require writing to that flash block). The disadvantage is
// that it introduces code complexity and potentially leaks blocks if power is
// lost or if FlashIndexStorage is destroyed before emptying the free list.
class FlashIndexStorage {
 public:
  // Creates a FlashIndexStorage at index_filename. in_memory determines whether
  // or not the FlashIndexStorage maintains an in-memory freelist in order to
  // avoid writes to the on-disk freelist.
  //
  // RETURNS:
  //   - On success, a valid instance of FlashIndexStorage
  //   - INTERNAL error if unable to create a new header or read the existing
  //     one from disk.
  static libtextclassifier3::StatusOr<FlashIndexStorage> Create(
      const std::string& index_filename, const Filesystem* filesystem,
      bool in_memory = true);

  // Retrieve the PostingList referred to by PostingListIdentifier. This posting
  // list must have been previously allocated by a prior call to
  // AllocatePostingList.
  //
  // RETURNS:
  //   - On success, a valid instance of PostingListHolder containing the
  //     requested PostingListUsed.
  //   - INVALID_ARGUMENT if id.posting_list_index() is out of bounds in the
  //     IndexBlock referred to by id.block_index()
  //   - INTERNAL_ERROR if unable to access the region in file.
  libtextclassifier3::StatusOr<PostingListHolder> GetPostingList(
      PostingListIdentifier id) const;

  // Allocates and returns a PostingListHolder containing a PostingListUsed that
  // can fit min_posting_list_bytes.
  //
  // RETURNS:
  //   - On success, a valid instance of PostingListHolder containing the
  //     requested PostingListUsed.
  //   - RESOURCE_EXHAUSTED error if unable to grow the index to create a
  //     PostingListUsed of the requested size.
  libtextclassifier3::StatusOr<PostingListHolder> AllocatePostingList(
      uint32_t min_posting_list_bytes);

  ~FlashIndexStorage();
  FlashIndexStorage(FlashIndexStorage&&) = default;
  FlashIndexStorage(const FlashIndexStorage&) = delete;
  FlashIndexStorage& operator=(FlashIndexStorage&&) = default;
  FlashIndexStorage& operator=(const FlashIndexStorage&) = delete;

  // Free the PostingListUsed that this holder holds.
  void FreePostingList(PostingListHolder holder);

  // Used to track the largest docid indexed in the index.
  DocumentId get_last_indexed_docid() const {
    return header_block_->header()->last_indexed_docid;
  }
  void set_last_indexed_docid(DocumentId docid) {
    header_block_->header()->last_indexed_docid = docid;
  }

  // Updates the header and persists all changes to the index to disk. Returns
  // true on success.
  bool PersistToDisk();

  // Returns the size of the index file in bytes.
  int64_t GetDiskUsage() const {
    return filesystem_->GetDiskUsage(block_fd_.get());
  }

  // Returns the size of the index file used to contains hits.
  uint64_t GetElementsSize() const {
    // Element size is the same as disk size excluding the header block.
    return GetDiskUsage() - block_size();
  }

  int num_blocks() const { return num_blocks_; }

  // Info about the index based on the block size.
  int block_size() const { return header_block_->header()->block_size; }

  // Num blocks starts at 1 since the first block is the header.
  bool empty() const { return num_blocks_ <= 1; }

  // The percentage of the maximum index size that is free. Allocated blocks are
  // treated as fully used, even if they are only partially used. In this way,
  // min_free_fraction is a lower bound of available space.
  double min_free_fraction() const {
    return 1.0 - static_cast<double>(num_blocks_) / kMaxBlockIndex;
  }

  libtextclassifier3::Status Reset();

  void GetDebugInfo(int verbosity, std::string* out) const;

 private:
  FlashIndexStorage(const std::string& index_filename,
                    const Filesystem* filesystem, bool has_in_memory_freelists);

  // Init the index from persistence. Create if file does not exist. We do not
  // erase corrupt files.
  //
  // Returns false if unable to create a new header or if the existing one is
  // corrupt.
  bool Init();

  // Create or open the header block. Returns true on success.
  bool InitHeader();

  // Create a new header block for an empty index file.
  bool CreateHeader();

  // Loads the header stored at the beginning of the index file and validates
  // the values stored in it.
  bool OpenHeader(int64_t file_size);

  // Add the IndexBlock referred to by block_index in the on-disk free list with
  // index block_info_index.
  void AddToOnDiskFreeList(uint32_t block_index, int block_info_index,
                           IndexBlock* index_block);

  // Remove the IndexBlock referred to by block_index from the Header free list
  // with index block_info_index.
  void RemoveFromOnDiskFreeList(uint32_t block_index, int block_info_index,
                                IndexBlock* index_block);

  // Returns:
  //   - On success, a valid PostingListHolder created from the first entry of
  //     the in-memory freelist at block_info_index
  //   - NOT_FOUND if there was no entry in the freelist
  //   - RESOURCE_EXHAUSTED if the PostingList in the freelist couldn't be
  //     allocated for some reason.
  libtextclassifier3::StatusOr<PostingListHolder>
  GetPostingListFromInMemoryFreeList(int block_info_index);

  // Returns:
  //   - On success, a valid PostingListHolder created from the first entry of
  //     the on-disk freelist at block_info_index
  //   - NOT_FOUND if there was no entry in the freelist
  //   - RESOURCE_EXHAUSTED if the PostingList in the freelist couldn't be
  //     allocated for some reason.
  libtextclassifier3::StatusOr<PostingListHolder>
  GetPostingListFromOnDiskFreeList(int block_info_index);

  // Returns:
  //   - On success, a valid PostingListHolder created from a newly allocated
  //     IndexBlock.
  //   - RESOURCE_EXHAUSTED if the index couldn't be grown to fit a new
  //     IndexBlock.
  libtextclassifier3::StatusOr<PostingListHolder> AllocateNewPostingList(
      int block_info_index);

  // Returns:
  //   - On success, a newly created IndexBlock at block_index with posting
  //     lists of size posting_list_size
  //   - INTERNAL_ERROR if unable to access the region in file representing the
  //   IndexBlock
  libtextclassifier3::StatusOr<IndexBlock> CreateIndexBlock(
      int block_index, uint32_t posting_list_size) const;

  // Returns:
  //   - On success, the IndexBlock that exists at block_index
  //   - INTERNAL_ERROR if unable to access the region in file representing the
  //   IndexBlock
  libtextclassifier3::StatusOr<IndexBlock> GetIndexBlock(int block_index) const;

  // Add a new block to the end of the file and return its block
  // index. Returns kInvalidBlockIndex if unable to grow the index file.
  int GrowIndex();

  // Return the index into index_block_infos of the smallest posting_list free
  // list that can fit posting_list_bytes or -1 if posting_list_bytes exceeds
  // the max-sized posting list.
  int FindBestIndexBlockInfo(uint32_t posting_list_bytes) const;

  // Flushes the in-memory free list to disk.
  void FlushInMemoryFreeList();

  // Underlying filename.
  std::string index_filename_;

  // We open the index file into this fd.
  ScopedFd block_fd_;
  int num_blocks_;  // can be inferred from index file size

  std::unique_ptr<HeaderBlock> header_block_;

  // In-memory cache of free posting lists.
  struct FreeList {
    // Experimentally determined that high watermark for largest
    // freelist was ~3500.
    static constexpr size_t kMaxSize = 4096;

    // Push a new PostingListIdentifier if there is space.
    void Push(PostingListIdentifier id);

    // Attempt to pop a PostingListIdentifier.
    //
    // RETURNS:
    //  - identifier of a free posting list, on success
    //  - NOT_FOUND if there are no free posting lists on this free list.
    libtextclassifier3::StatusOr<PostingListIdentifier> TryPop();

    std::string DebugString() const;

   private:
    std::vector<PostingListIdentifier> free_list_;
    int free_list_size_high_watermark_ = 0;
    int num_dropped_free_list_entries_ = 0;
  };
  std::vector<FreeList> in_memory_freelists_;

  const Filesystem* filesystem_;  // not owned; can't be null

  bool has_in_memory_freelists_;
};

}  // namespace lib
}  // namespace icing

#endif  // ICING_INDEX_FLASH_INDEX_STORAGE_H_
