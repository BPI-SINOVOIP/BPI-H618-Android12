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

#include <cstdint>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "icing/file/filesystem.h"
#include "icing/index/hit/hit.h"
#include "icing/index/main/flash-index-storage.h"
#include "icing/index/main/index-block.h"
#include "icing/index/main/posting-list-identifier.h"
#include "icing/index/main/posting-list-used.h"
#include "icing/testing/common-matchers.h"
#include "icing/testing/hit-test-utils.h"
#include "icing/testing/tmp-directory.h"

namespace icing {
namespace lib {

namespace {

using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::Lt;
using ::testing::SizeIs;

TEST(PostingListAccessorStorageTest, HitsAddAndRetrieveProperly) {
  std::string test_dir = GetTestTempDir() + "/test_dir";
  std::string file_name = test_dir + "/test_file.idx.index";
  Filesystem filesystem;
  ASSERT_TRUE(filesystem.DeleteDirectoryRecursively(test_dir.c_str()));
  ASSERT_TRUE(filesystem.CreateDirectoryRecursively(test_dir.c_str()));

  ICING_ASSERT_OK_AND_ASSIGN(FlashIndexStorage flash_index_storage,
                             FlashIndexStorage::Create(file_name, &filesystem));
  // Add some hits! Any hits!
  std::vector<Hit> hits1 =
      CreateHits(/*num_hits=*/5, /*desired_byte_length=*/1);
  ICING_ASSERT_OK_AND_ASSIGN(PostingListAccessor pl_accessor,
                             PostingListAccessor::Create(&flash_index_storage));
  for (const Hit& hit : hits1) {
    ICING_ASSERT_OK(pl_accessor.PrependHit(hit));
  }
  PostingListAccessor::FinalizeResult result =
      PostingListAccessor::Finalize(std::move(pl_accessor));
  ICING_EXPECT_OK(result.status);
  EXPECT_THAT(result.id.block_index(), Eq(1));
  EXPECT_THAT(result.id.posting_list_index(), Eq(0));

  // Retrieve some hits.
  ICING_ASSERT_OK_AND_ASSIGN(PostingListHolder pl_holder,
                             flash_index_storage.GetPostingList(result.id));
  EXPECT_THAT(pl_holder.posting_list.GetHits(),
              IsOkAndHolds(ElementsAreArray(hits1.rbegin(), hits1.rend())));
  EXPECT_THAT(pl_holder.block.next_block_index(), Eq(kInvalidBlockIndex));
}

TEST(PostingListAccessorStorageTest, PreexistingPLKeepOnSameBlock) {
  std::string test_dir = GetTestTempDir() + "/test_dir";
  std::string file_name = test_dir + "/test_file.idx.index";
  Filesystem filesystem;
  ASSERT_TRUE(filesystem.DeleteDirectoryRecursively(test_dir.c_str()));
  ASSERT_TRUE(filesystem.CreateDirectoryRecursively(test_dir.c_str()));

  ICING_ASSERT_OK_AND_ASSIGN(FlashIndexStorage flash_index_storage,
                             FlashIndexStorage::Create(file_name, &filesystem));
  ICING_ASSERT_OK_AND_ASSIGN(PostingListAccessor pl_accessor,
                             PostingListAccessor::Create(&flash_index_storage));
  // Add a single hit. This will fit in a min-sized posting list.
  Hit hit1(/*section_id=*/1, /*document_id=*/0, Hit::kDefaultTermFrequency);
  ICING_ASSERT_OK(pl_accessor.PrependHit(hit1));
  PostingListAccessor::FinalizeResult result1 =
      PostingListAccessor::Finalize(std::move(pl_accessor));
  ICING_EXPECT_OK(result1.status);
  // Should have been allocated to the first block.
  EXPECT_THAT(result1.id.block_index(), Eq(1));
  EXPECT_THAT(result1.id.posting_list_index(), Eq(0));

  // Add one more hit. The minimum size for a posting list must be able to fit
  // at least two hits, so this should NOT cause the previous pl to be
  // reallocated.
  ICING_ASSERT_OK_AND_ASSIGN(
      pl_accessor, PostingListAccessor::CreateFromExisting(&flash_index_storage,
                                                           result1.id));
  Hit hit2 = CreateHit(hit1, /*desired_byte_length=*/1);
  ICING_ASSERT_OK(pl_accessor.PrependHit(hit2));
  PostingListAccessor::FinalizeResult result2 =
      PostingListAccessor::Finalize(std::move(pl_accessor));
  ICING_EXPECT_OK(result2.status);
  // Should have been allocated to the same posting list as the first hit.
  EXPECT_THAT(result2.id, Eq(result1.id));

  // The posting list at result2.id should hold all of the hits that have been
  // added.
  ICING_ASSERT_OK_AND_ASSIGN(PostingListHolder pl_holder,
                             flash_index_storage.GetPostingList(result2.id));
  EXPECT_THAT(pl_holder.posting_list.GetHits(),
              IsOkAndHolds(ElementsAre(hit2, hit1)));
}

TEST(PostingListAccessorStorageTest, PreexistingPLReallocateToLargerPL) {
  std::string test_dir = GetTestTempDir() + "/test_dir";
  std::string file_name = test_dir + "/test_file.idx.index";
  Filesystem filesystem;
  ASSERT_TRUE(filesystem.DeleteDirectoryRecursively(test_dir.c_str()));
  ASSERT_TRUE(filesystem.CreateDirectoryRecursively(test_dir.c_str()));

  ICING_ASSERT_OK_AND_ASSIGN(FlashIndexStorage flash_index_storage,
                             FlashIndexStorage::Create(file_name, &filesystem));
  ICING_ASSERT_OK_AND_ASSIGN(PostingListAccessor pl_accessor,
                             PostingListAccessor::Create(&flash_index_storage));
  // The smallest posting list size is 15 bytes. The first four hits will be
  // compressed to one byte each and will be able to fit in the 5 byte padded
  // region. The last hit will fit in one of the special hits. The posting list
  // will be ALMOST_FULL and can fit at most 2 more hits.
  std::vector<Hit> hits1 =
      CreateHits(/*num_hits=*/5, /*desired_byte_length=*/1);
  for (const Hit& hit : hits1) {
    ICING_ASSERT_OK(pl_accessor.PrependHit(hit));
  }
  PostingListAccessor::FinalizeResult result1 =
      PostingListAccessor::Finalize(std::move(pl_accessor));
  ICING_EXPECT_OK(result1.status);
  // Should have been allocated to the first block.
  EXPECT_THAT(result1.id.block_index(), Eq(1));
  EXPECT_THAT(result1.id.posting_list_index(), Eq(0));

  // Now let's add some more hits!
  ICING_ASSERT_OK_AND_ASSIGN(
      pl_accessor, PostingListAccessor::CreateFromExisting(&flash_index_storage,
                                                           result1.id));
  // The current posting list can fit at most 2 more hits. Adding 12 more hits
  // should result in these hits being moved to a larger posting list.
  std::vector<Hit> hits2 = CreateHits(
      /*start_docid=*/hits1.back().document_id() + 1, /*num_hits=*/12,
      /*desired_byte_length=*/1);

  for (const Hit& hit : hits2) {
    ICING_ASSERT_OK(pl_accessor.PrependHit(hit));
  }
  PostingListAccessor::FinalizeResult result2 =
      PostingListAccessor::Finalize(std::move(pl_accessor));
  ICING_EXPECT_OK(result2.status);
  // Should have been allocated to the second (new) block because the posting
  // list should have grown beyond the size that the first block maintains.
  EXPECT_THAT(result2.id.block_index(), Eq(2));
  EXPECT_THAT(result2.id.posting_list_index(), Eq(0));

  // The posting list at result2.id should hold all of the hits that have been
  // added.
  for (const Hit& hit : hits2) {
    hits1.push_back(hit);
  }
  ICING_ASSERT_OK_AND_ASSIGN(PostingListHolder pl_holder,
                             flash_index_storage.GetPostingList(result2.id));
  EXPECT_THAT(pl_holder.posting_list.GetHits(),
              IsOkAndHolds(ElementsAreArray(hits1.rbegin(), hits1.rend())));
}

TEST(PostingListAccessorStorageTest, MultiBlockChainsBlocksProperly) {
  std::string test_dir = GetTestTempDir() + "/test_dir";
  std::string file_name = test_dir + "/test_file.idx.index";
  Filesystem filesystem;
  ASSERT_TRUE(filesystem.DeleteDirectoryRecursively(test_dir.c_str()));
  ASSERT_TRUE(filesystem.CreateDirectoryRecursively(test_dir.c_str()));

  ICING_ASSERT_OK_AND_ASSIGN(FlashIndexStorage flash_index_storage,
                             FlashIndexStorage::Create(file_name, &filesystem));
  ICING_ASSERT_OK_AND_ASSIGN(PostingListAccessor pl_accessor,
                             PostingListAccessor::Create(&flash_index_storage));
  // Add some hits! Any hits!
  std::vector<Hit> hits1 =
      CreateHits(/*num_hits=*/5000, /*desired_byte_length=*/1);
  for (const Hit& hit : hits1) {
    ICING_ASSERT_OK(pl_accessor.PrependHit(hit));
  }
  PostingListAccessor::FinalizeResult result1 =
      PostingListAccessor::Finalize(std::move(pl_accessor));
  ICING_EXPECT_OK(result1.status);
  PostingListIdentifier second_block_id = result1.id;
  // Should have been allocated to the second block, which holds a max-sized
  // posting list.
  EXPECT_THAT(second_block_id, Eq(PostingListIdentifier(
                                   /*block_index=*/2, /*posting_list_index=*/0,
                                   /*posting_list_index_bits=*/0)));

  // Now let's retrieve them!
  ICING_ASSERT_OK_AND_ASSIGN(
      PostingListHolder pl_holder,
      flash_index_storage.GetPostingList(second_block_id));
  // This pl_holder will only hold a posting list with the hits that didn't fit
  // on the first block.
  ICING_ASSERT_OK_AND_ASSIGN(std::vector<Hit> second_block_hits,
                             pl_holder.posting_list.GetHits());
  ASSERT_THAT(second_block_hits, SizeIs(Lt(hits1.size())));
  auto first_block_hits_start = hits1.rbegin() + second_block_hits.size();
  EXPECT_THAT(second_block_hits,
              ElementsAreArray(hits1.rbegin(), first_block_hits_start));

  // Now retrieve all of the hits that were on the first block.
  uint32_t first_block_id = pl_holder.block.next_block_index();
  EXPECT_THAT(first_block_id, Eq(1));

  PostingListIdentifier pl_id(first_block_id, /*posting_list_index=*/0,
                              /*posting_list_index_bits=*/0);
  ICING_ASSERT_OK_AND_ASSIGN(pl_holder,
                             flash_index_storage.GetPostingList(pl_id));
  EXPECT_THAT(
      pl_holder.posting_list.GetHits(),
      IsOkAndHolds(ElementsAreArray(first_block_hits_start, hits1.rend())));
}

TEST(PostingListAccessorStorageTest,
     PreexistingMultiBlockReusesBlocksProperly) {
  std::string test_dir = GetTestTempDir() + "/test_dir";
  std::string file_name = test_dir + "/test_file.idx.index";
  Filesystem filesystem;
  ASSERT_TRUE(filesystem.DeleteDirectoryRecursively(test_dir.c_str()));
  ASSERT_TRUE(filesystem.CreateDirectoryRecursively(test_dir.c_str()));

  ICING_ASSERT_OK_AND_ASSIGN(FlashIndexStorage flash_index_storage,
                             FlashIndexStorage::Create(file_name, &filesystem));
  ICING_ASSERT_OK_AND_ASSIGN(PostingListAccessor pl_accessor,
                             PostingListAccessor::Create(&flash_index_storage));
  // Add some hits! Any hits!
  std::vector<Hit> hits1 =
      CreateHits(/*num_hits=*/5000, /*desired_byte_length=*/1);
  for (const Hit& hit : hits1) {
    ICING_ASSERT_OK(pl_accessor.PrependHit(hit));
  }
  PostingListAccessor::FinalizeResult result1 =
      PostingListAccessor::Finalize(std::move(pl_accessor));
  ICING_EXPECT_OK(result1.status);
  PostingListIdentifier first_add_id = result1.id;
  EXPECT_THAT(first_add_id, Eq(PostingListIdentifier(
                                /*block_index=*/2, /*posting_list_index=*/0,
                                /*posting_list_index_bits=*/0)));

  // Now add a couple more hits. These should fit on the existing, not full
  // second block.
  ICING_ASSERT_OK_AND_ASSIGN(
      pl_accessor, PostingListAccessor::CreateFromExisting(&flash_index_storage,
                                                           first_add_id));
  std::vector<Hit> hits2 = CreateHits(
      /*start_docid=*/hits1.back().document_id() + 1, /*num_hits=*/50,
      /*desired_byte_length=*/1);

  for (const Hit& hit : hits2) {
    ICING_ASSERT_OK(pl_accessor.PrependHit(hit));
  }
  PostingListAccessor::FinalizeResult result2 =
      PostingListAccessor::Finalize(std::move(pl_accessor));
  ICING_EXPECT_OK(result2.status);
  PostingListIdentifier second_add_id = result2.id;
  EXPECT_THAT(second_add_id, Eq(first_add_id));

  // We should be able to retrieve all 5050 hits.
  for (const Hit& hit : hits2) {
    hits1.push_back(hit);
  }
  ICING_ASSERT_OK_AND_ASSIGN(PostingListHolder pl_holder,
                             flash_index_storage.GetPostingList(second_add_id));
  // This pl_holder will only hold a posting list with the hits that didn't fit
  // on the first block.
  ICING_ASSERT_OK_AND_ASSIGN(std::vector<Hit> second_block_hits,
                             pl_holder.posting_list.GetHits());
  ASSERT_THAT(second_block_hits, SizeIs(Lt(hits1.size())));
  auto first_block_hits_start = hits1.rbegin() + second_block_hits.size();
  EXPECT_THAT(second_block_hits,
              ElementsAreArray(hits1.rbegin(), first_block_hits_start));

  // Now retrieve all of the hits that were on the first block.
  uint32_t first_block_id = pl_holder.block.next_block_index();
  EXPECT_THAT(first_block_id, Eq(1));

  PostingListIdentifier pl_id(first_block_id, /*posting_list_index=*/0,
                              /*posting_list_index_bits=*/0);
  ICING_ASSERT_OK_AND_ASSIGN(pl_holder,
                             flash_index_storage.GetPostingList(pl_id));
  EXPECT_THAT(
      pl_holder.posting_list.GetHits(),
      IsOkAndHolds(ElementsAreArray(first_block_hits_start, hits1.rend())));
}

TEST(PostingListAccessorStorageTest, InvalidHitReturnsInvalidArgument) {
  std::string test_dir = GetTestTempDir() + "/test_dir";
  std::string file_name = test_dir + "/test_file.idx.index";
  Filesystem filesystem;
  ASSERT_TRUE(filesystem.DeleteDirectoryRecursively(test_dir.c_str()));
  ASSERT_TRUE(filesystem.CreateDirectoryRecursively(test_dir.c_str()));

  ICING_ASSERT_OK_AND_ASSIGN(FlashIndexStorage flash_index_storage,
                             FlashIndexStorage::Create(file_name, &filesystem));
  ICING_ASSERT_OK_AND_ASSIGN(PostingListAccessor pl_accessor,
                             PostingListAccessor::Create(&flash_index_storage));
  Hit invalid_hit;
  EXPECT_THAT(pl_accessor.PrependHit(invalid_hit),
              StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT));
}

TEST(PostingListAccessorStorageTest, HitsNotDecreasingReturnsInvalidArgument) {
  std::string test_dir = GetTestTempDir() + "/test_dir";
  std::string file_name = test_dir + "/test_file.idx.index";
  Filesystem filesystem;
  ASSERT_TRUE(filesystem.DeleteDirectoryRecursively(test_dir.c_str()));
  ASSERT_TRUE(filesystem.CreateDirectoryRecursively(test_dir.c_str()));

  ICING_ASSERT_OK_AND_ASSIGN(FlashIndexStorage flash_index_storage,
                             FlashIndexStorage::Create(file_name, &filesystem));
  ICING_ASSERT_OK_AND_ASSIGN(PostingListAccessor pl_accessor,
                             PostingListAccessor::Create(&flash_index_storage));
  Hit hit1(/*section_id=*/3, /*document_id=*/1, Hit::kDefaultTermFrequency);
  ICING_ASSERT_OK(pl_accessor.PrependHit(hit1));

  Hit hit2(/*section_id=*/6, /*document_id=*/1, Hit::kDefaultTermFrequency);
  EXPECT_THAT(pl_accessor.PrependHit(hit2),
              StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT));

  Hit hit3(/*section_id=*/2, /*document_id=*/0, Hit::kDefaultTermFrequency);
  EXPECT_THAT(pl_accessor.PrependHit(hit3),
              StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT));
}

TEST(PostingListAccessorStorageTest, NewPostingListNoHitsAdded) {
  std::string test_dir = GetTestTempDir() + "/test_dir";
  std::string file_name = test_dir + "/test_file.idx.index";
  Filesystem filesystem;
  ASSERT_TRUE(filesystem.DeleteDirectoryRecursively(test_dir.c_str()));
  ASSERT_TRUE(filesystem.CreateDirectoryRecursively(test_dir.c_str()));

  ICING_ASSERT_OK_AND_ASSIGN(FlashIndexStorage flash_index_storage,
                             FlashIndexStorage::Create(file_name, &filesystem));
  ICING_ASSERT_OK_AND_ASSIGN(PostingListAccessor pl_accessor,
                             PostingListAccessor::Create(&flash_index_storage));
  PostingListAccessor::FinalizeResult result1 =
      PostingListAccessor::Finalize(std::move(pl_accessor));
  EXPECT_THAT(result1.status,
              StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT));
}

TEST(PostingListAccessorStorageTest, PreexistingPostingListNoHitsAdded) {
  std::string test_dir = GetTestTempDir() + "/test_dir";
  std::string file_name = test_dir + "/test_file.idx.index";
  Filesystem filesystem;
  ASSERT_TRUE(filesystem.DeleteDirectoryRecursively(test_dir.c_str()));
  ASSERT_TRUE(filesystem.CreateDirectoryRecursively(test_dir.c_str()));

  ICING_ASSERT_OK_AND_ASSIGN(FlashIndexStorage flash_index_storage,
                             FlashIndexStorage::Create(file_name, &filesystem));
  ICING_ASSERT_OK_AND_ASSIGN(PostingListAccessor pl_accessor,
                             PostingListAccessor::Create(&flash_index_storage));
  Hit hit1(/*section_id=*/3, /*document_id=*/1, Hit::kDefaultTermFrequency);
  ICING_ASSERT_OK(pl_accessor.PrependHit(hit1));
  PostingListAccessor::FinalizeResult result1 =
      PostingListAccessor::Finalize(std::move(pl_accessor));
  ICING_ASSERT_OK(result1.status);

  ICING_ASSERT_OK_AND_ASSIGN(PostingListAccessor pl_accessor2,
                             PostingListAccessor::CreateFromExisting(
                                 &flash_index_storage, result1.id));
  PostingListAccessor::FinalizeResult result2 =
      PostingListAccessor::Finalize(std::move(pl_accessor2));
  ICING_ASSERT_OK(result2.status);
}

}  // namespace

}  // namespace lib
}  // namespace icing
