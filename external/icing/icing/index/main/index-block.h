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

#ifndef ICING_INDEX_MAIN_INDEX_BLOCK_H_
#define ICING_INDEX_MAIN_INDEX_BLOCK_H_

#include <string.h>
#include <sys/mman.h>

#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "icing/file/memory-mapped-file.h"
#include "icing/index/hit/hit.h"
#include "icing/index/main/posting-list-free.h"
#include "icing/index/main/posting-list-used.h"
#include "icing/legacy/index/icing-bit-util.h"

namespace icing {
namespace lib {

inline constexpr uint32_t kInvalidBlockIndex = 0;

// This class is used to manage I/O to a single flash block and to manage the
// division of that flash block into PostingLists. It provides an interface to
// allocate, free and read posting lists.
//
// An IndexBlock contains a small header and an array of fixed-size posting list
// buffers. Initially, all posting lists are chained in a singly-linked free
// list.
//
// When we want to get a new PostingList from an IndexBlock, we just
// pull one off the free list. When the user wants to return the
// PostingList to the free pool, we prepend it to the free list.
class IndexBlock {
 public:
  // What is the maximum posting list size in bytes that can be stored in this
  // block.
  static uint32_t CalculateMaxPostingListBytes(uint32_t block_size_in_bytes) {
    return (block_size_in_bytes - sizeof(BlockHeader)) / sizeof(Hit) *
           sizeof(Hit);
  }

  // For a given min number of bits needed to store PostingListIndex for a
  // block of "block_size", return the approximate number of hits that a full
  // posting list in this block could accomodate.
  static uint32_t ApproximateFullPostingListHitsForBlock(
      uint32_t block_size, int posting_list_index_bits);

  // Create an IndexBlock to reference the previously used region of the
  // mmapped_file starting at offset with size block_size
  //
  // RETURNS:
  //   - a valid IndexBlock on success
  //   - INVALID_ARGUMENT if size is too small for even just the BlockHeader or
  //     if the posting list size stored in the region is not a valid posting
  //     list size or it exceeds max_posting_list_bytes(size).
  //   - INTERNAL_ERROR if unable to mmap the region [offset, offset+block_size)
  static libtextclassifier3::StatusOr<IndexBlock>
  CreateFromPreexistingIndexBlockRegion(const Filesystem& filesystem,
                                        std::string_view file_path,
                                        off_t offset, uint32_t block_size);

  // Create an IndexBlock to reference an uninitialized region of the
  // mmapped_file starting at offset with size block_size. The IndexBlock will
  // initialize the region to be an empty IndexBlock with posting lists of size
  // posting_list_bytes.
  //
  // RETURNS:
  //   - a valid IndexBlock on success
  //   - INVALID_ARGUMENT if size is too small for even just the BlockHeader or
  //   if posting_list_bytes is not a valid posting list size or it exceeds
  //   max_posting_list_bytes(size).
  //   - INTERNAL_ERROR if unable to mmap the region [offset, offset+block_size)
  static libtextclassifier3::StatusOr<IndexBlock> CreateFromUninitializedRegion(
      const Filesystem& filesystem, std::string_view file_path, off_t offset,
      uint32_t block_size, uint32_t posting_list_bytes);

  IndexBlock(const IndexBlock&) = delete;
  IndexBlock& operator=(const IndexBlock&) = delete;
  IndexBlock(IndexBlock&&) = default;
  IndexBlock& operator=(IndexBlock&&) = default;

  ~IndexBlock() {
    if (mmapped_block_ != nullptr) {
      mmapped_block_->PersistToDisk();
    }
  }

  // Instantiate a PostingListUsed at posting_list_index with the existing
  // content in the IndexBlock.
  //
  // RETURNS:
  //   - a valid PostingListUsed on success
  //   - INVALID_ARGUMENT if posting_list_index >= max_num_posting_lists()
  libtextclassifier3::StatusOr<PostingListUsed> GetAllocatedPostingList(
      PostingListIndex posting_list_index);

  // Allocates a PostingListUsed in the IndexBlock, if possible.
  //
  // RETURNS:
  //   - a valid PostingListIndex that can be used to retrieve the allocated
  //     PostingListUsed via a call to GetAllocatedPostingList
  //   - RESOURCE_EXHAUSTED if !has_free_posting_lists()
  libtextclassifier3::StatusOr<PostingListIndex> AllocatePostingList();

  // Free posting list at posting_list_index.
  //
  // It is considered an error to "double-free" a posting list. You should never
  // call FreePostingList(index) with the same index twice, unless that index
  // was returned by an intervening AllocatePostingList() call.
  //
  // Ex.
  //   PostingListIndex index = block.AllocatePostingList();
  //   DoSomething(block.GetAllocatedPostingList(index));
  //   block.FreePostingList(index);
  //   block.FreePostingList(index);  // Argh! What are you doing?!
  //   ...
  //   PostingListIndex index = block.AllocatePostingList();
  //   DoSomething(block.GetAllocatedPostingList(index));
  //   block.FreePostingList(index);
  //   index = block.AllocatePostingList();
  //   DoSomethingElse(block.GetAllocatedPostingList(index));
  //   // A-Ok! We called AllocatePostingList() since the last FreePostingList()
  //   call. block.FreePostingList(index);
  //
  // Has no effect if posting_list_index >= max_num_posting_lists().
  void FreePostingList(PostingListIndex posting_list_index);

  // Blocks can be chained. The interpretation of the chaining is up
  // to the caller.
  uint32_t next_block_index() const { return header_->next_block_index; }

  void set_next_block_index(uint32_t next_block_index) {
    header_->next_block_index = next_block_index;
  }

  // Retrieves the size (in bytes) of the posting lists in this IndexBlock.
  uint32_t get_posting_list_bytes() const {
    return header_->posting_list_bytes;
  }

  // Maximum number of posting lists in the block.
  uint32_t max_num_posting_lists() const {
    return total_posting_lists_bytes() / get_posting_list_bytes();
  }

  // Number of bits required to store the largest PostingListIndex in this
  // block.
  int posting_list_index_bits() const {
    return BitsToStore(max_num_posting_lists());
  }

  // Returns whether or not there are available posting lists in the free list.
  bool has_free_posting_lists() const {
    return header_->free_list_posting_list_index != kInvalidPostingListIndex;
  }

 private:
  // Assumes that mmapped_file already has established a valid mapping to the
  // requested block.
  IndexBlock(MemoryMappedFile mmapped_block);

  // Resets IndexBlock to hold posting lists of posting_list_bytes size and adds
  // all posting lists to the free list.
  //
  // RETURNS:
  //   - OK, on success
  //   - INVALID_ARGUMENT if posting_list_bytes is a valid posting list size.
  libtextclassifier3::Status Reset(int posting_list_bytes);

  char* get_posting_list_ptr(PostingListIndex posting_list_index);

  // Bytes in the block available for posting lists (minus header,
  // alignment, etc.).
  uint32_t total_posting_lists_bytes() const {
    return block_size_in_bytes_ - sizeof(BlockHeader);
  }

  struct BlockHeader {
    // Index of the next block if this block is being chained or part of a free
    // list.
    uint32_t next_block_index;

    // Index to the first PostingListFree in the IndexBlock. This is the start
    // of the free list.
    PostingListIndex free_list_posting_list_index;

    // The size of each posting list in the IndexBlock.
    uint32_t posting_list_bytes;
  };
  // Pointer to the header of this block. The header is used to store info about
  // this block and its posting lists.
  BlockHeader* header_;
  // Pointer to the beginning of the posting lists region - the area the block
  // after the header.
  char* posting_lists_start_ptr_;
  uint32_t block_size_in_bytes_;

  // MemoryMappedFile used to interact with the underlying flash block.
  std::unique_ptr<MemoryMappedFile> mmapped_block_;
};

}  // namespace lib
}  // namespace icing

#endif  // ICING_INDEX_MAIN_INDEX_BLOCK_H_
