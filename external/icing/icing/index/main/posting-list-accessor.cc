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

#include "icing/index/main/posting-list-accessor.h"

#include <memory>

#include "icing/absl_ports/canonical_errors.h"
#include "icing/index/main/flash-index-storage.h"
#include "icing/index/main/index-block.h"
#include "icing/index/main/posting-list-identifier.h"
#include "icing/index/main/posting-list-used.h"
#include "icing/util/status-macros.h"

namespace icing {
namespace lib {

libtextclassifier3::StatusOr<PostingListAccessor> PostingListAccessor::Create(
    FlashIndexStorage *storage) {
  uint32_t max_posting_list_bytes =
      IndexBlock::CalculateMaxPostingListBytes(storage->block_size());
  std::unique_ptr<uint8_t[]> posting_list_buffer_array =
      std::make_unique<uint8_t[]>(max_posting_list_bytes);
  ICING_ASSIGN_OR_RETURN(
      PostingListUsed posting_list_buffer,
      PostingListUsed::CreateFromUnitializedRegion(
          posting_list_buffer_array.get(), max_posting_list_bytes));
  return PostingListAccessor(storage, std::move(posting_list_buffer_array),
                             std::move(posting_list_buffer));
}

libtextclassifier3::StatusOr<PostingListAccessor>
PostingListAccessor::CreateFromExisting(
    FlashIndexStorage *storage,
    PostingListIdentifier existing_posting_list_id) {
  // Our posting_list_buffer_ will start as empty.
  ICING_ASSIGN_OR_RETURN(PostingListAccessor pl_accessor, Create(storage));
  ICING_ASSIGN_OR_RETURN(PostingListHolder holder,
                         storage->GetPostingList(existing_posting_list_id));
  pl_accessor.preexisting_posting_list_ =
      std::make_unique<PostingListHolder>(std::move(holder));
  return pl_accessor;
}

// Returns the next batch of hits for the provided posting list.
libtextclassifier3::StatusOr<std::vector<Hit>>
PostingListAccessor::GetNextHitsBatch() {
  if (preexisting_posting_list_ == nullptr) {
    if (has_reached_posting_list_chain_end_) {
      return std::vector<Hit>();
    }
    return absl_ports::FailedPreconditionError(
        "Cannot retrieve hits from a PostingListAccessor that was not creaated "
        "from a preexisting posting list.");
  }
  ICING_ASSIGN_OR_RETURN(std::vector<Hit> batch,
                         preexisting_posting_list_->posting_list.GetHits());
  uint32_t block_index = preexisting_posting_list_->block.next_block_index();
  if (block_index != kInvalidBlockIndex) {
    PostingListIdentifier next_posting_list_id(
        block_index, /*posting_list_index=*/0,
        preexisting_posting_list_->block.posting_list_index_bits());
    ICING_ASSIGN_OR_RETURN(PostingListHolder holder,
                           storage_->GetPostingList(next_posting_list_id));
    preexisting_posting_list_ =
        std::make_unique<PostingListHolder>(std::move(holder));
  } else {
    has_reached_posting_list_chain_end_ = true;
    preexisting_posting_list_.reset();
  }
  return batch;
}

libtextclassifier3::Status PostingListAccessor::PrependHit(const Hit &hit) {
  PostingListUsed &active_pl = (preexisting_posting_list_ != nullptr)
                                   ? preexisting_posting_list_->posting_list
                                   : posting_list_buffer_;
  libtextclassifier3::Status status = active_pl.PrependHit(hit);
  if (!absl_ports::IsResourceExhausted(status)) {
    return status;
  }
  // There is no more room to add hits to this current posting list! Therefore,
  // we need to either move those hits to a larger posting list or flush this
  // posting list and create another max-sized posting list in the chain.
  if (preexisting_posting_list_ != nullptr) {
    FlushPreexistingPostingList();
  } else {
    ICING_RETURN_IF_ERROR(FlushInMemoryPostingList());
  }

  // Re-add hit. Should always fit since we just cleared posting_list_buffer_.
  // It's fine to explicitly reference posting_list_buffer_ here because there's
  // no way of reaching this line while preexisting_posting_list_ is still in
  // use.
  return posting_list_buffer_.PrependHit(hit);
}

void PostingListAccessor::FlushPreexistingPostingList() {
  if (preexisting_posting_list_->block.max_num_posting_lists() == 1) {
    // If this is a max-sized posting list, then just keep track of the id for
    // chaining. It'll be flushed to disk when preexisting_posting_list_ is
    // destructed.
    prev_block_identifier_ = preexisting_posting_list_->id;
  } else {
    // If this is NOT a max-sized posting list, then our hits have outgrown this
    // particular posting list. Move the hits into the in-memory posting list
    // and free this posting list.
    //
    // Move will always succeed since posting_list_buffer_ is max_pl_bytes.
    posting_list_buffer_.MoveFrom(&preexisting_posting_list_->posting_list);

    // Now that all the contents of this posting list have been copied, there's
    // no more use for it. Make it available to be used for another posting
    // list.
    storage_->FreePostingList(std::move(*preexisting_posting_list_));
  }
  preexisting_posting_list_.reset();
}

libtextclassifier3::Status PostingListAccessor::FlushInMemoryPostingList() {
  // We exceeded max_pl_bytes(). Need to flush posting_list_buffer_ and update
  // the chain.
  uint32_t max_posting_list_bytes =
      IndexBlock::CalculateMaxPostingListBytes(storage_->block_size());
  ICING_ASSIGN_OR_RETURN(PostingListHolder holder,
                         storage_->AllocatePostingList(max_posting_list_bytes));
  holder.block.set_next_block_index(prev_block_identifier_.block_index());
  prev_block_identifier_ = holder.id;
  return holder.posting_list.MoveFrom(&posting_list_buffer_);
}

PostingListAccessor::FinalizeResult PostingListAccessor::Finalize(
    PostingListAccessor accessor) {
  if (accessor.preexisting_posting_list_ != nullptr) {
    // Our hits are already in an existing posting list. Nothing else to do, but
    // return its id.
    FinalizeResult result = {libtextclassifier3::Status::OK,
                             accessor.preexisting_posting_list_->id};
    return result;
  }
  if (accessor.posting_list_buffer_.BytesUsed() <= 0) {
    FinalizeResult result = {absl_ports::InvalidArgumentError(
                                 "Can't finalize an empty PostingListAccessor. "
                                 "There's nothing to Finalize!"),
                             PostingListIdentifier::kInvalid};
    return result;
  }
  uint32_t posting_list_bytes =
      accessor.posting_list_buffer_.MinPostingListSizeToFit();
  if (accessor.prev_block_identifier_.is_valid()) {
    posting_list_bytes = IndexBlock::CalculateMaxPostingListBytes(
        accessor.storage_->block_size());
  }
  auto holder_or = accessor.storage_->AllocatePostingList(posting_list_bytes);
  if (!holder_or.ok()) {
    FinalizeResult result = {holder_or.status(),
                             accessor.prev_block_identifier_};
    return result;
  }
  PostingListHolder holder = std::move(holder_or).ValueOrDie();
  if (accessor.prev_block_identifier_.is_valid()) {
    holder.block.set_next_block_index(
        accessor.prev_block_identifier_.block_index());
  }

  // Move to allocated area. This should never actually return an error. We know
  // that editor.posting_list() is valid because it wouldn't have successfully
  // returned by AllocatePostingList if it wasn't. We know posting_list_buffer_
  // is valid because we created it in-memory. And finally, we know that the
  // hits from posting_list_buffer_ will fit in editor.posting_list() because we
  // requested it be at at least posting_list_bytes large.
  auto status = holder.posting_list.MoveFrom(&accessor.posting_list_buffer_);
  if (!status.ok()) {
    FinalizeResult result = {std::move(status),
                             accessor.prev_block_identifier_};
    return result;
  }
  FinalizeResult result = {libtextclassifier3::Status::OK, holder.id};
  return result;
}

}  // namespace lib
}  // namespace icing
