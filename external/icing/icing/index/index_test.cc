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

#include "icing/index/index.h"

#include <cstdint>
#include <limits>
#include <memory>
#include <random>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "icing/text_classifier/lib3/utils/base/status.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "icing/file/filesystem.h"
#include "icing/index/hit/doc-hit-info.h"
#include "icing/index/iterator/doc-hit-info-iterator.h"
#include "icing/legacy/index/icing-filesystem.h"
#include "icing/legacy/index/icing-mock-filesystem.h"
#include "icing/proto/storage.pb.h"
#include "icing/proto/term.pb.h"
#include "icing/schema/section.h"
#include "icing/store/document-id.h"
#include "icing/testing/common-matchers.h"
#include "icing/testing/random-string.h"
#include "icing/testing/tmp-directory.h"
#include "icing/util/crc32.h"

namespace icing {
namespace lib {

namespace {

using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Ge;
using ::testing::Gt;
using ::testing::IsEmpty;
using ::testing::IsTrue;
using ::testing::Ne;
using ::testing::NiceMock;
using ::testing::Not;
using ::testing::Return;
using ::testing::SizeIs;
using ::testing::Test;
using ::testing::UnorderedElementsAre;

int GetBlockSize() { return getpagesize(); }

class IndexTest : public Test {
 protected:
  void SetUp() override {
    index_dir_ = GetTestTempDir() + "/index_test/";
    Index::Options options(index_dir_, /*index_merge_size=*/1024 * 1024);
    ICING_ASSERT_OK_AND_ASSIGN(
        index_, Index::Create(options, &filesystem_, &icing_filesystem_));
  }

  void TearDown() override {
    icing_filesystem_.DeleteDirectoryRecursively(index_dir_.c_str());
  }

  std::unique_ptr<Index> index_;
  std::string index_dir_;
  IcingFilesystem icing_filesystem_;
  Filesystem filesystem_;
};

constexpr DocumentId kDocumentId0 = 0;
constexpr DocumentId kDocumentId1 = 1;
constexpr DocumentId kDocumentId2 = 2;
constexpr DocumentId kDocumentId3 = 3;
constexpr DocumentId kDocumentId4 = 4;
constexpr DocumentId kDocumentId5 = 5;
constexpr DocumentId kDocumentId6 = 6;
constexpr DocumentId kDocumentId7 = 7;
constexpr SectionId kSectionId2 = 2;
constexpr SectionId kSectionId3 = 3;

// The value returned by IndexBlock::ApproximateFullPostingListHitsForBlock(
//    GetBlockSize(),
//    GetPostingListIndexBits(posting_list_utils::min_posting_list_size()));
constexpr int kMinSizePlApproxHits = 3;
// The value returned by IndexBlock::ApproximateFullPostingListHitsForBlock(
//    GetBlockSize(),
//    GetPostingListIndexBits(2 * posting_list_utils::min_posting_list_size()));
constexpr int kSecondSmallestPlApproxHits = 7;

std::vector<DocHitInfo> GetHits(std::unique_ptr<DocHitInfoIterator> iterator) {
  std::vector<DocHitInfo> infos;
  while (iterator->Advance().ok()) {
    infos.push_back(iterator->doc_hit_info());
  }
  return infos;
}

MATCHER_P2(EqualsDocHitInfo, document_id, sections, "") {
  const DocHitInfo& actual = arg;
  SectionIdMask section_mask = kSectionIdMaskNone;
  for (SectionId section : sections) {
    section_mask |= 1U << section;
  }
  *result_listener << "actual is {document_id=" << actual.document_id()
                   << ", section_mask=" << actual.hit_section_ids_mask()
                   << "}, but expected was {document_id=" << document_id
                   << ", section_mask=" << section_mask << "}.";
  return actual.document_id() == document_id &&
         actual.hit_section_ids_mask() == section_mask;
}

MATCHER_P2(EqualsTermMetadata, content, hit_count, "") {
  const TermMetadata& actual = arg;
  *result_listener << "actual is {content=" << actual.content
                   << ", hit_count=" << actual.hit_count
                   << "}, but expected was {content=" << content
                   << ", hit_count=" << hit_count << "}.";
  return actual.content == content && actual.hit_count == hit_count;
}

TEST_F(IndexTest, CreationWithNullPointerShouldFail) {
  Index::Options options(index_dir_, /*index_merge_size=*/1024 * 1024);
  EXPECT_THAT(
      Index::Create(options, &filesystem_, /*icing_filesystem=*/nullptr),
      StatusIs(libtextclassifier3::StatusCode::FAILED_PRECONDITION));
  EXPECT_THAT(
      Index::Create(options, /*filesystem=*/nullptr, &icing_filesystem_),
      StatusIs(libtextclassifier3::StatusCode::FAILED_PRECONDITION));
}

TEST_F(IndexTest, EmptyIndex) {
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("foo", kSectionIdMaskAll, TermMatchType::EXACT_ONLY));
  EXPECT_THAT(itr->Advance(),
              StatusIs(libtextclassifier3::StatusCode::RESOURCE_EXHAUSTED));

  ICING_ASSERT_OK_AND_ASSIGN(
      itr,
      index_->GetIterator("foo", kSectionIdMaskAll, TermMatchType::EXACT_ONLY));
  EXPECT_THAT(itr->Advance(),
              StatusIs(libtextclassifier3::StatusCode::RESOURCE_EXHAUSTED));
}

TEST_F(IndexTest, EmptyIndexAfterMerge) {
  // Merging an empty index should succeed, but have no effects.
  ICING_ASSERT_OK(index_->Merge());

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("foo", kSectionIdMaskAll, TermMatchType::EXACT_ONLY));
  EXPECT_THAT(itr->Advance(),
              StatusIs(libtextclassifier3::StatusCode::RESOURCE_EXHAUSTED));

  ICING_ASSERT_OK_AND_ASSIGN(
      itr,
      index_->GetIterator("foo", kSectionIdMaskAll, TermMatchType::EXACT_ONLY));
  EXPECT_THAT(itr->Advance(),
              StatusIs(libtextclassifier3::StatusCode::RESOURCE_EXHAUSTED));
}

TEST_F(IndexTest, AdvancePastEnd) {
  Index::Editor edit = index_->Edit(
      kDocumentId0, kSectionId2, TermMatchType::EXACT_ONLY, /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("bar", kSectionIdMaskAll, TermMatchType::EXACT_ONLY));
  EXPECT_THAT(itr->Advance(),
              StatusIs(libtextclassifier3::StatusCode::RESOURCE_EXHAUSTED));
  EXPECT_THAT(itr->doc_hit_info(),
              EqualsDocHitInfo(kInvalidDocumentId, std::vector<SectionId>()));

  ICING_ASSERT_OK_AND_ASSIGN(
      itr,
      index_->GetIterator("foo", kSectionIdMaskAll, TermMatchType::EXACT_ONLY));
  EXPECT_THAT(itr->Advance(), IsOk());
  EXPECT_THAT(itr->Advance(),
              StatusIs(libtextclassifier3::StatusCode::RESOURCE_EXHAUSTED));
  EXPECT_THAT(itr->doc_hit_info(),
              EqualsDocHitInfo(kInvalidDocumentId, std::vector<SectionId>()));
}

TEST_F(IndexTest, AdvancePastEndAfterMerge) {
  Index::Editor edit = index_->Edit(
      kDocumentId0, kSectionId2, TermMatchType::EXACT_ONLY, /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  ICING_ASSERT_OK(index_->Merge());

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("bar", kSectionIdMaskAll, TermMatchType::EXACT_ONLY));
  EXPECT_THAT(itr->Advance(),
              StatusIs(libtextclassifier3::StatusCode::RESOURCE_EXHAUSTED));
  EXPECT_THAT(itr->doc_hit_info(),
              EqualsDocHitInfo(kInvalidDocumentId, std::vector<SectionId>()));

  ICING_ASSERT_OK_AND_ASSIGN(
      itr,
      index_->GetIterator("foo", kSectionIdMaskAll, TermMatchType::EXACT_ONLY));
  EXPECT_THAT(itr->Advance(), IsOk());
  EXPECT_THAT(itr->Advance(),
              StatusIs(libtextclassifier3::StatusCode::RESOURCE_EXHAUSTED));
  EXPECT_THAT(itr->doc_hit_info(),
              EqualsDocHitInfo(kInvalidDocumentId, std::vector<SectionId>()));
}

TEST_F(IndexTest, SingleHitSingleTermIndex) {
  Index::Editor edit = index_->Edit(
      kDocumentId0, kSectionId2, TermMatchType::EXACT_ONLY, /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("foo", kSectionIdMaskAll, TermMatchType::EXACT_ONLY));
  EXPECT_THAT(GetHits(std::move(itr)),
              ElementsAre(EqualsDocHitInfo(
                  kDocumentId0, std::vector<SectionId>{kSectionId2})));
}

TEST_F(IndexTest, SingleHitSingleTermIndexAfterMerge) {
  Index::Editor edit = index_->Edit(
      kDocumentId0, kSectionId2, TermMatchType::EXACT_ONLY, /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  ICING_ASSERT_OK(index_->Merge());

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("foo", kSectionIdMaskAll, TermMatchType::EXACT_ONLY));
  EXPECT_THAT(GetHits(std::move(itr)),
              ElementsAre(EqualsDocHitInfo(
                  kDocumentId0, std::vector<SectionId>{kSectionId2})));
}

TEST_F(IndexTest, SingleHitMultiTermIndex) {
  Index::Editor edit = index_->Edit(
      kDocumentId0, kSectionId2, TermMatchType::EXACT_ONLY, /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.BufferTerm("bar"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("foo", kSectionIdMaskAll, TermMatchType::EXACT_ONLY));
  EXPECT_THAT(GetHits(std::move(itr)),
              ElementsAre(EqualsDocHitInfo(
                  kDocumentId0, std::vector<SectionId>{kSectionId2})));
}

TEST_F(IndexTest, SingleHitMultiTermIndexAfterMerge) {
  Index::Editor edit = index_->Edit(
      kDocumentId0, kSectionId2, TermMatchType::EXACT_ONLY, /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.BufferTerm("bar"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  ICING_ASSERT_OK(index_->Merge());

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("foo", kSectionIdMaskAll, TermMatchType::EXACT_ONLY));
  EXPECT_THAT(GetHits(std::move(itr)),
              ElementsAre(EqualsDocHitInfo(
                  kDocumentId0, std::vector<SectionId>{kSectionId2})));
}

TEST_F(IndexTest, NoHitMultiTermIndex) {
  Index::Editor edit = index_->Edit(
      kDocumentId0, kSectionId2, TermMatchType::EXACT_ONLY, /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.BufferTerm("bar"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("baz", kSectionIdMaskAll, TermMatchType::EXACT_ONLY));
  EXPECT_THAT(itr->Advance(),
              StatusIs(libtextclassifier3::StatusCode::RESOURCE_EXHAUSTED));
}

TEST_F(IndexTest, NoHitMultiTermIndexAfterMerge) {
  Index::Editor edit = index_->Edit(
      kDocumentId0, kSectionId2, TermMatchType::EXACT_ONLY, /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.BufferTerm("bar"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  ICING_ASSERT_OK(index_->Merge());

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("baz", kSectionIdMaskAll, TermMatchType::EXACT_ONLY));
  EXPECT_THAT(itr->Advance(),
              StatusIs(libtextclassifier3::StatusCode::RESOURCE_EXHAUSTED));
}

TEST_F(IndexTest, MultiHitMultiTermIndex) {
  Index::Editor edit = index_->Edit(
      kDocumentId0, kSectionId2, TermMatchType::EXACT_ONLY, /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  edit = index_->Edit(kDocumentId1, kSectionId2, TermMatchType::EXACT_ONLY,
                      /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("bar"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  edit = index_->Edit(kDocumentId2, kSectionId3, TermMatchType::EXACT_ONLY,
                      /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("foo", kSectionIdMaskAll, TermMatchType::EXACT_ONLY));
  EXPECT_THAT(
      GetHits(std::move(itr)),
      ElementsAre(
          EqualsDocHitInfo(kDocumentId2, std::vector<SectionId>{kSectionId3}),
          EqualsDocHitInfo(kDocumentId0, std::vector<SectionId>{kSectionId2})));
}

TEST_F(IndexTest, MultiHitMultiTermIndexAfterMerge) {
  Index::Editor edit = index_->Edit(
      kDocumentId0, kSectionId2, TermMatchType::EXACT_ONLY, /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  edit = index_->Edit(kDocumentId1, kSectionId2, TermMatchType::EXACT_ONLY,
                      /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("bar"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  edit = index_->Edit(kDocumentId2, kSectionId3, TermMatchType::EXACT_ONLY,
                      /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  ICING_ASSERT_OK(index_->Merge());

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("foo", kSectionIdMaskAll, TermMatchType::EXACT_ONLY));
  EXPECT_THAT(
      GetHits(std::move(itr)),
      ElementsAre(
          EqualsDocHitInfo(kDocumentId2, std::vector<SectionId>{kSectionId3}),
          EqualsDocHitInfo(kDocumentId0, std::vector<SectionId>{kSectionId2})));
}

TEST_F(IndexTest, MultiHitSectionRestrict) {
  Index::Editor edit = index_->Edit(
      kDocumentId0, kSectionId2, TermMatchType::EXACT_ONLY, /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  edit = index_->Edit(kDocumentId1, kSectionId3, TermMatchType::EXACT_ONLY,
                      /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  SectionIdMask desired_section = 1U << kSectionId2;
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("foo", desired_section, TermMatchType::EXACT_ONLY));
  EXPECT_THAT(GetHits(std::move(itr)),
              ElementsAre(EqualsDocHitInfo(
                  kDocumentId0, std::vector<SectionId>{kSectionId2})));
}

TEST_F(IndexTest, MultiHitSectionRestrictAfterMerge) {
  Index::Editor edit = index_->Edit(
      kDocumentId0, kSectionId2, TermMatchType::EXACT_ONLY, /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  edit = index_->Edit(kDocumentId1, kSectionId3, TermMatchType::EXACT_ONLY,
                      /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  ICING_ASSERT_OK(index_->Merge());

  SectionIdMask desired_section = 1U << kSectionId2;
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("foo", desired_section, TermMatchType::EXACT_ONLY));
  EXPECT_THAT(GetHits(std::move(itr)),
              ElementsAre(EqualsDocHitInfo(
                  kDocumentId0, std::vector<SectionId>{kSectionId2})));
}

TEST_F(IndexTest, SingleHitDedupeIndex) {
  ICING_ASSERT_OK_AND_ASSIGN(int64_t size, index_->GetElementsSize());
  EXPECT_THAT(size, Eq(0));
  Index::Editor edit = index_->Edit(
      kDocumentId0, kSectionId2, TermMatchType::EXACT_ONLY, /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("foo"), IsOk());
  ICING_ASSERT_OK_AND_ASSIGN(size, index_->GetElementsSize());
  EXPECT_THAT(size, Gt(0));
  EXPECT_THAT(edit.BufferTerm("foo"), IsOk());
  ICING_ASSERT_OK_AND_ASSIGN(int64_t new_size, index_->GetElementsSize());
  EXPECT_THAT(new_size, Eq(size));
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("foo", kSectionIdMaskAll, TermMatchType::EXACT_ONLY));
  EXPECT_THAT(GetHits(std::move(itr)),
              ElementsAre(EqualsDocHitInfo(
                  kDocumentId0, std::vector<SectionId>{kSectionId2})));
}

TEST_F(IndexTest, PrefixHit) {
  Index::Editor edit = index_->Edit(kDocumentId0, kSectionId2,
                                    TermMatchType::PREFIX, /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("fool"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("foo", kSectionIdMaskAll, TermMatchType::PREFIX));
  EXPECT_THAT(GetHits(std::move(itr)),
              ElementsAre(EqualsDocHitInfo(
                  kDocumentId0, std::vector<SectionId>{kSectionId2})));
}

TEST_F(IndexTest, PrefixHitAfterMerge) {
  Index::Editor edit = index_->Edit(kDocumentId0, kSectionId2,
                                    TermMatchType::PREFIX, /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("fool"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  ICING_ASSERT_OK(index_->Merge());

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("foo", kSectionIdMaskAll, TermMatchType::PREFIX));
  EXPECT_THAT(GetHits(std::move(itr)),
              ElementsAre(EqualsDocHitInfo(
                  kDocumentId0, std::vector<SectionId>{kSectionId2})));
}

TEST_F(IndexTest, MultiPrefixHit) {
  Index::Editor edit = index_->Edit(kDocumentId0, kSectionId2,
                                    TermMatchType::PREFIX, /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("fool"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  edit = index_->Edit(kDocumentId1, kSectionId3, TermMatchType::EXACT_ONLY,
                      /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("foo", kSectionIdMaskAll, TermMatchType::PREFIX));
  EXPECT_THAT(
      GetHits(std::move(itr)),
      ElementsAre(
          EqualsDocHitInfo(kDocumentId1, std::vector<SectionId>{kSectionId3}),
          EqualsDocHitInfo(kDocumentId0, std::vector<SectionId>{kSectionId2})));
}

TEST_F(IndexTest, MultiPrefixHitAfterMerge) {
  Index::Editor edit = index_->Edit(kDocumentId0, kSectionId2,
                                    TermMatchType::PREFIX, /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("fool"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  edit = index_->Edit(kDocumentId1, kSectionId3, TermMatchType::EXACT_ONLY,
                      /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  ICING_ASSERT_OK(index_->Merge());

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("foo", kSectionIdMaskAll, TermMatchType::PREFIX));
  EXPECT_THAT(
      GetHits(std::move(itr)),
      ElementsAre(
          EqualsDocHitInfo(kDocumentId1, std::vector<SectionId>{kSectionId3}),
          EqualsDocHitInfo(kDocumentId0, std::vector<SectionId>{kSectionId2})));
}

TEST_F(IndexTest, NoExactHitInPrefixQuery) {
  Index::Editor edit = index_->Edit(
      kDocumentId0, kSectionId2, TermMatchType::EXACT_ONLY, /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("fool"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  edit = index_->Edit(kDocumentId1, kSectionId3, TermMatchType::PREFIX,
                      /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("foo", kSectionIdMaskAll, TermMatchType::PREFIX));
  EXPECT_THAT(GetHits(std::move(itr)),
              ElementsAre(EqualsDocHitInfo(
                  kDocumentId1, std::vector<SectionId>{kSectionId3})));
}

TEST_F(IndexTest, NoExactHitInPrefixQueryAfterMerge) {
  Index::Editor edit = index_->Edit(
      kDocumentId0, kSectionId2, TermMatchType::EXACT_ONLY, /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("fool"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  edit = index_->Edit(kDocumentId1, kSectionId3, TermMatchType::PREFIX,
                      /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  ICING_ASSERT_OK(index_->Merge());

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("foo", kSectionIdMaskAll, TermMatchType::PREFIX));
  EXPECT_THAT(GetHits(std::move(itr)),
              ElementsAre(EqualsDocHitInfo(
                  kDocumentId1, std::vector<SectionId>{kSectionId3})));
}

TEST_F(IndexTest, PrefixHitDedupe) {
  Index::Editor edit = index_->Edit(kDocumentId0, kSectionId2,
                                    TermMatchType::PREFIX, /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("foo"), IsOk());
  ASSERT_THAT(edit.BufferTerm("fool"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("foo", kSectionIdMaskAll, TermMatchType::PREFIX));
  EXPECT_THAT(GetHits(std::move(itr)),
              ElementsAre(EqualsDocHitInfo(
                  kDocumentId0, std::vector<SectionId>{kSectionId2})));
}

TEST_F(IndexTest, PrefixHitDedupeAfterMerge) {
  Index::Editor edit = index_->Edit(kDocumentId0, kSectionId2,
                                    TermMatchType::PREFIX, /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("foo"), IsOk());
  ASSERT_THAT(edit.BufferTerm("fool"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  ICING_ASSERT_OK(index_->Merge());

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("foo", kSectionIdMaskAll, TermMatchType::PREFIX));
  EXPECT_THAT(GetHits(std::move(itr)),
              ElementsAre(EqualsDocHitInfo(
                  kDocumentId0, std::vector<SectionId>{kSectionId2})));
}

TEST_F(IndexTest, PrefixToString) {
  SectionIdMask id_mask = (1U << kSectionId2) | (1U << kSectionId3);
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("foo", id_mask, TermMatchType::PREFIX));
  EXPECT_THAT(itr->ToString(),
              Eq("(0000000000001100:foo* OR 0000000000001100:foo*)"));

  ICING_ASSERT_OK_AND_ASSIGN(itr, index_->GetIterator("foo", kSectionIdMaskAll,
                                                      TermMatchType::PREFIX));
  EXPECT_THAT(itr->ToString(),
              Eq("(1111111111111111:foo* OR 1111111111111111:foo*)"));

  ICING_ASSERT_OK_AND_ASSIGN(itr, index_->GetIterator("foo", kSectionIdMaskNone,
                                                      TermMatchType::PREFIX));
  EXPECT_THAT(itr->ToString(),
              Eq("(0000000000000000:foo* OR 0000000000000000:foo*)"));
}

TEST_F(IndexTest, ExactToString) {
  SectionIdMask id_mask = (1U << kSectionId2) | (1U << kSectionId3);
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("foo", id_mask, TermMatchType::EXACT_ONLY));
  EXPECT_THAT(itr->ToString(),
              Eq("(0000000000001100:foo OR 0000000000001100:foo)"));

  ICING_ASSERT_OK_AND_ASSIGN(
      itr,
      index_->GetIterator("foo", kSectionIdMaskAll, TermMatchType::EXACT_ONLY));
  EXPECT_THAT(itr->ToString(),
              Eq("(1111111111111111:foo OR 1111111111111111:foo)"));

  ICING_ASSERT_OK_AND_ASSIGN(itr,
                             index_->GetIterator("foo", kSectionIdMaskNone,
                                                 TermMatchType::EXACT_ONLY));
  EXPECT_THAT(itr->ToString(),
              Eq("(0000000000000000:foo OR 0000000000000000:foo)"));
}

TEST_F(IndexTest, NonAsciiTerms) {
  Index::Editor edit = index_->Edit(kDocumentId0, kSectionId2,
                                    TermMatchType::PREFIX, /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("こんにちは"), IsOk());
  ASSERT_THAT(edit.BufferTerm("あなた"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("こんに", kSectionIdMaskAll, TermMatchType::PREFIX));
  EXPECT_THAT(GetHits(std::move(itr)),
              ElementsAre(EqualsDocHitInfo(
                  kDocumentId0, std::vector<SectionId>{kSectionId2})));

  ICING_ASSERT_OK_AND_ASSIGN(itr,
                             index_->GetIterator("あなた", kSectionIdMaskAll,
                                                 TermMatchType::EXACT_ONLY));
  EXPECT_THAT(GetHits(std::move(itr)),
              ElementsAre(EqualsDocHitInfo(
                  kDocumentId0, std::vector<SectionId>{kSectionId2})));
}

TEST_F(IndexTest, NonAsciiTermsAfterMerge) {
  Index::Editor edit = index_->Edit(kDocumentId0, kSectionId2,
                                    TermMatchType::PREFIX, /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("こんにちは"), IsOk());
  ASSERT_THAT(edit.BufferTerm("あなた"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  ICING_ASSERT_OK(index_->Merge());

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("こんに", kSectionIdMaskAll, TermMatchType::PREFIX));
  EXPECT_THAT(GetHits(std::move(itr)),
              ElementsAre(EqualsDocHitInfo(
                  kDocumentId0, std::vector<SectionId>{kSectionId2})));

  ICING_ASSERT_OK_AND_ASSIGN(itr,
                             index_->GetIterator("あなた", kSectionIdMaskAll,
                                                 TermMatchType::EXACT_ONLY));
  EXPECT_THAT(GetHits(std::move(itr)),
              ElementsAre(EqualsDocHitInfo(
                  kDocumentId0, std::vector<SectionId>{kSectionId2})));
}

TEST_F(IndexTest, FullIndex) {
  // Make a smaller index so that it's easier to fill up.
  Index::Options options(index_dir_, /*index_merge_size=*/1024);
  ICING_ASSERT_OK_AND_ASSIGN(
      index_, Index::Create(options, &filesystem_, &icing_filesystem_));

  std::default_random_engine random;
  std::vector<std::string> query_terms;
  std::string prefix = "prefix";
  for (int i = 0; i < 2600; ++i) {
    constexpr int kTokenSize = 5;
    query_terms.push_back(prefix +
                          RandomString(kAlNumAlphabet, kTokenSize, &random));
  }

  DocumentId document_id = 0;
  libtextclassifier3::Status status = libtextclassifier3::Status::OK;
  std::uniform_int_distribution<size_t> uniform(0u, query_terms.size() - 1);
  while (status.ok()) {
    for (int i = 0; i < 100; ++i) {
      Index::Editor edit =
          index_->Edit(document_id, kSectionId2, TermMatchType::PREFIX,
                       /*namespace_id=*/0);
      size_t idx = uniform(random);
      status = edit.BufferTerm(query_terms.at(idx).c_str());
      if (!status.ok()) {
        break;
      }
      status = edit.IndexAllBufferedTerms();
      if (!status.ok()) {
        break;
      }
    }
    ++document_id;
  }

  // Adding more hits should fail.
  Index::Editor edit =
      index_->Edit(document_id + 1, kSectionId2, TermMatchType::PREFIX,
                   /*namespace_id=*/0);
  std::string term = prefix + "foo";
  EXPECT_THAT(edit.BufferTerm(term.c_str()), IsOk());
  term = prefix + "bar";
  EXPECT_THAT(edit.BufferTerm(term.c_str()), IsOk());
  term = prefix + "baz";
  EXPECT_THAT(edit.BufferTerm(term.c_str()), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(),
              StatusIs(libtextclassifier3::StatusCode::RESOURCE_EXHAUSTED));

  for (int i = 0; i < query_terms.size(); i += 25) {
    ICING_ASSERT_OK_AND_ASSIGN(
        std::unique_ptr<DocHitInfoIterator> itr,
        index_->GetIterator(query_terms.at(i).c_str(), kSectionIdMaskAll,
                            TermMatchType::PREFIX));
    // Each query term should contain at least one hit - there may have been
    // other hits for this term that were added.
    EXPECT_THAT(itr->Advance(), IsOk());
  }
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> last_itr,
      index_->GetIterator(prefix.c_str(), kSectionIdMaskAll,
                          TermMatchType::PREFIX));
  EXPECT_THAT(last_itr->Advance(), IsOk());
  EXPECT_THAT(last_itr->doc_hit_info().document_id(), Eq(document_id - 1));
}

TEST_F(IndexTest, FullIndexMerge) {
  // Make a smaller index so that it's easier to fill up.
  Index::Options options(index_dir_, /*index_merge_size=*/1024);
  ICING_ASSERT_OK_AND_ASSIGN(
      index_, Index::Create(options, &filesystem_, &icing_filesystem_));

  std::default_random_engine random;
  std::vector<std::string> query_terms;
  std::string prefix = "prefix";
  for (int i = 0; i < 2600; ++i) {
    constexpr int kTokenSize = 5;
    query_terms.push_back(prefix +
                          RandomString(kAlNumAlphabet, kTokenSize, &random));
  }

  DocumentId document_id = 0;
  libtextclassifier3::Status status = libtextclassifier3::Status::OK;
  std::uniform_int_distribution<size_t> uniform(0u, query_terms.size() - 1);
  while (status.ok()) {
    for (int i = 0; i < 100; ++i) {
      Index::Editor edit =
          index_->Edit(document_id, kSectionId2, TermMatchType::PREFIX,
                       /*namespace_id=*/0);
      size_t idx = uniform(random);
      status = edit.BufferTerm(query_terms.at(idx).c_str());
      if (!status.ok()) {
        break;
      }
      status = edit.IndexAllBufferedTerms();
      if (!status.ok()) {
        break;
      }
    }
    ++document_id;
  }
  EXPECT_THAT(status,
              StatusIs(libtextclassifier3::StatusCode::RESOURCE_EXHAUSTED));

  // Adding more hits should fail.
  Index::Editor edit =
      index_->Edit(document_id + 1, kSectionId2, TermMatchType::PREFIX,
                   /*namespace_id=*/0);
  std::string term = prefix + "foo";
  EXPECT_THAT(edit.BufferTerm(term.c_str()), IsOk());
  term = prefix + "bar";
  EXPECT_THAT(edit.BufferTerm(term.c_str()), IsOk());
  term = prefix + "baz";
  EXPECT_THAT(edit.BufferTerm(term.c_str()), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(),
              StatusIs(libtextclassifier3::StatusCode::RESOURCE_EXHAUSTED));
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> last_itr,
      index_->GetIterator(prefix.c_str(), kSectionIdMaskAll,
                          TermMatchType::PREFIX));
  EXPECT_THAT(last_itr->Advance(), IsOk());
  EXPECT_THAT(last_itr->doc_hit_info().document_id(), Eq(document_id - 1));

  // After merging with the main index. Adding more hits should succeed now.
  ICING_ASSERT_OK(index_->Merge());
  edit = index_->Edit(document_id + 1, kSectionId2, TermMatchType::PREFIX, 0);
  prefix + "foo";
  EXPECT_THAT(edit.BufferTerm(term.c_str()), IsOk());
  term = prefix + "bar";
  EXPECT_THAT(edit.BufferTerm(term.c_str()), IsOk());
  term = prefix + "baz";
  EXPECT_THAT(edit.BufferTerm(term.c_str()), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator(prefix + "bar", kSectionIdMaskAll,
                          TermMatchType::EXACT_ONLY));
  // We know that "bar" should have at least one hit because we just added it!
  EXPECT_THAT(itr->Advance(), IsOk());
  EXPECT_THAT(itr->doc_hit_info().document_id(), Eq(document_id + 1));
  ICING_ASSERT_OK_AND_ASSIGN(
      last_itr, index_->GetIterator(prefix.c_str(), kSectionIdMaskAll,
                                    TermMatchType::PREFIX));
  EXPECT_THAT(last_itr->Advance(), IsOk());
  EXPECT_THAT(last_itr->doc_hit_info().document_id(), Eq(document_id + 1));
}

TEST_F(IndexTest, IndexCreateIOFailure) {
  // Create the index with mock filesystem. By default, Mock will return false,
  // so the first attempted file operation will fail.
  NiceMock<IcingMockFilesystem> mock_icing_filesystem;
  ON_CALL(mock_icing_filesystem, CreateDirectoryRecursively)
      .WillByDefault(Return(false));
  Index::Options options(index_dir_, /*index_merge_size=*/1024 * 1024);
  EXPECT_THAT(Index::Create(options, &filesystem_, &mock_icing_filesystem),
              StatusIs(libtextclassifier3::StatusCode::INTERNAL));
}

TEST_F(IndexTest, IndexCreateCorruptionFailure) {
  // Add some content to the index
  Index::Editor edit = index_->Edit(kDocumentId0, kSectionId2,
                                    TermMatchType::PREFIX, /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("foo"), IsOk());
  ASSERT_THAT(edit.BufferTerm("bar"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  // Close the index.
  index_.reset();

  // Corrrupt the index file.
  std::string hit_buffer_filename = index_dir_ + "/idx/lite.hb";
  ScopedFd sfd(icing_filesystem_.OpenForWrite(hit_buffer_filename.c_str()));
  ASSERT_THAT(sfd.is_valid(), IsTrue());

  constexpr std::string_view kCorruptBytes = "ffffffffffffffffffffff";
  // The first page of the hit_buffer is taken up by the header. Overwrite the
  // first page of content.
  int hit_buffer_start_offset = GetBlockSize();
  ASSERT_THAT(
      icing_filesystem_.PWrite(sfd.get(), hit_buffer_start_offset,
                               kCorruptBytes.data(), kCorruptBytes.length()),
      IsTrue());

  // Recreate the index.
  Index::Options options(index_dir_, /*index_merge_size=*/1024 * 1024);
  EXPECT_THAT(Index::Create(options, &filesystem_, &icing_filesystem_),
              StatusIs(libtextclassifier3::StatusCode::DATA_LOSS));
}

TEST_F(IndexTest, IndexPersistence) {
  // Add some content to the index
  Index::Editor edit = index_->Edit(kDocumentId0, kSectionId2,
                                    TermMatchType::PREFIX, /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("foo"), IsOk());
  ASSERT_THAT(edit.BufferTerm("bar"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  EXPECT_THAT(index_->PersistToDisk(), IsOk());

  // Close the index.
  index_.reset();

  // Recreate the index.
  Index::Options options(index_dir_, /*index_merge_size=*/1024 * 1024);
  ICING_ASSERT_OK_AND_ASSIGN(
      index_, Index::Create(options, &filesystem_, &icing_filesystem_));

  // Check that the hits are present.
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("f", kSectionIdMaskAll, TermMatchType::PREFIX));
  EXPECT_THAT(GetHits(std::move(itr)),
              ElementsAre(EqualsDocHitInfo(
                  kDocumentId0, std::vector<SectionId>{kSectionId2})));
}

TEST_F(IndexTest, IndexPersistenceAfterMerge) {
  // Add some content to the index
  Index::Editor edit = index_->Edit(kDocumentId0, kSectionId2,
                                    TermMatchType::PREFIX, /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("foo"), IsOk());
  ASSERT_THAT(edit.BufferTerm("bar"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  ICING_ASSERT_OK(index_->Merge());
  EXPECT_THAT(index_->PersistToDisk(), IsOk());

  // Close the index.
  index_.reset();

  // Recreate the index.
  Index::Options options(index_dir_, /*index_merge_size=*/1024 * 1024);
  ICING_ASSERT_OK_AND_ASSIGN(
      index_, Index::Create(options, &filesystem_, &icing_filesystem_));

  // Check that the hits are present.
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("f", kSectionIdMaskAll, TermMatchType::PREFIX));
  EXPECT_THAT(GetHits(std::move(itr)),
              ElementsAre(EqualsDocHitInfo(
                  kDocumentId0, std::vector<SectionId>{kSectionId2})));
}

TEST_F(IndexTest, InvalidHitBufferSize) {
  Index::Options options(
      index_dir_, /*index_merge_size=*/std::numeric_limits<uint32_t>::max());
  EXPECT_THAT(Index::Create(options, &filesystem_, &icing_filesystem_),
              StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT));
}

TEST_F(IndexTest, FindTermByPrefixShouldReturnEmpty) {
  Index::Editor edit = index_->Edit(kDocumentId0, kSectionId2,
                                    TermMatchType::PREFIX, /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("fool"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  EXPECT_THAT(index_->FindTermsByPrefix(/*prefix=*/"foo", /*namespace_ids=*/{0},
                                        /*num_to_return=*/0),
              IsOkAndHolds(IsEmpty()));
  EXPECT_THAT(index_->FindTermsByPrefix(/*prefix=*/"foo", /*namespace_ids=*/{0},
                                        /*num_to_return=*/-1),
              IsOkAndHolds(IsEmpty()));

  ICING_ASSERT_OK(index_->Merge());

  EXPECT_THAT(index_->FindTermsByPrefix(/*prefix=*/"foo", /*namespace_ids=*/{0},
                                        /*num_to_return=*/0),
              IsOkAndHolds(IsEmpty()));
  EXPECT_THAT(index_->FindTermsByPrefix(/*prefix=*/"foo", /*namespace_ids=*/{0},
                                        /*num_to_return=*/-1),
              IsOkAndHolds(IsEmpty()));
}

TEST_F(IndexTest, FindTermByPrefixShouldReturnCorrectResult) {
  Index::Editor edit = index_->Edit(
      kDocumentId0, kSectionId2, TermMatchType::EXACT_ONLY, /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.BufferTerm("bar"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  // "b" should only match "bar" but not "foo".
  EXPECT_THAT(index_->FindTermsByPrefix(/*prefix=*/"b", /*namespace_ids=*/{0},
                                        /*num_to_return=*/10),
              IsOkAndHolds(UnorderedElementsAre(EqualsTermMetadata("bar", 1))));

  ICING_ASSERT_OK(index_->Merge());

  // "b" should only match "bar" but not "foo".
  EXPECT_THAT(index_->FindTermsByPrefix(/*prefix=*/"b", /*namespace_ids=*/{0},
                                        /*num_to_return=*/10),
              IsOkAndHolds(UnorderedElementsAre(
                  EqualsTermMetadata("bar", kMinSizePlApproxHits))));
}

TEST_F(IndexTest, FindTermByPrefixShouldRespectNumToReturn) {
  Index::Editor edit = index_->Edit(
      kDocumentId0, kSectionId2, TermMatchType::EXACT_ONLY, /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("fo"), IsOk());
  EXPECT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.BufferTerm("fool"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  // We have 3 results but only 2 should be returned.
  EXPECT_THAT(index_->FindTermsByPrefix(/*prefix=*/"f", /*namespace_ids=*/{0},
                                        /*num_to_return=*/2),
              IsOkAndHolds(SizeIs(2)));

  ICING_ASSERT_OK(index_->Merge());

  // We have 3 results but only 2 should be returned.
  EXPECT_THAT(index_->FindTermsByPrefix(/*prefix=*/"f", /*namespace_ids=*/{0},
                                        /*num_to_return=*/2),
              IsOkAndHolds(SizeIs(2)));
}

TEST_F(IndexTest, FindTermByPrefixShouldReturnTermsInOneNamespace) {
  Index::Editor edit1 =
      index_->Edit(kDocumentId0, kSectionId2, TermMatchType::EXACT_ONLY,
                   /*namespace_id=*/0);
  EXPECT_THAT(edit1.BufferTerm("fo"), IsOk());
  EXPECT_THAT(edit1.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit1.IndexAllBufferedTerms(), IsOk());

  Index::Editor edit2 =
      index_->Edit(kDocumentId1, kSectionId2, TermMatchType::EXACT_ONLY,
                   /*namespace_id=*/1);
  EXPECT_THAT(edit2.BufferTerm("fool"), IsOk());
  EXPECT_THAT(edit2.IndexAllBufferedTerms(), IsOk());

  // namespace with id 0 has 2 results.
  EXPECT_THAT(index_->FindTermsByPrefix(/*prefix=*/"f", /*namespace_ids=*/{0},
                                        /*num_to_return=*/10),
              IsOkAndHolds(UnorderedElementsAre(EqualsTermMetadata("fo", 1),
                                                EqualsTermMetadata("foo", 1))));
  // namespace with id 1 has 1 result.
  EXPECT_THAT(
      index_->FindTermsByPrefix(/*prefix=*/"f", /*namespace_ids=*/{1},
                                /*num_to_return=*/10),
      IsOkAndHolds(UnorderedElementsAre(EqualsTermMetadata("fool", 1))));

  ICING_ASSERT_OK(index_->Merge());

  // namespace with id 0 has 2 results.
  EXPECT_THAT(index_->FindTermsByPrefix(/*prefix=*/"f", /*namespace_ids=*/{0},
                                        /*num_to_return=*/10),
              IsOkAndHolds(UnorderedElementsAre(
                  EqualsTermMetadata("fo", kMinSizePlApproxHits),
                  EqualsTermMetadata("foo", kMinSizePlApproxHits))));
  // namespace with id 1 has 1 result.
  EXPECT_THAT(index_->FindTermsByPrefix(/*prefix=*/"f", /*namespace_ids=*/{1},
                                        /*num_to_return=*/10),
              IsOkAndHolds(UnorderedElementsAre(
                  EqualsTermMetadata("fool", kMinSizePlApproxHits))));
}

TEST_F(IndexTest, FindTermByPrefixShouldReturnTermsInMultipleNamespaces) {
  Index::Editor edit1 =
      index_->Edit(kDocumentId0, kSectionId2, TermMatchType::EXACT_ONLY,
                   /*namespace_id=*/0);
  EXPECT_THAT(edit1.BufferTerm("fo"), IsOk());
  EXPECT_THAT(edit1.IndexAllBufferedTerms(), IsOk());

  Index::Editor edit2 =
      index_->Edit(kDocumentId1, kSectionId2, TermMatchType::EXACT_ONLY,
                   /*namespace_id=*/1);
  EXPECT_THAT(edit2.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit2.IndexAllBufferedTerms(), IsOk());

  Index::Editor edit3 =
      index_->Edit(kDocumentId2, kSectionId2, TermMatchType::EXACT_ONLY,
                   /*namespace_id=*/2);
  EXPECT_THAT(edit3.BufferTerm("fool"), IsOk());
  EXPECT_THAT(edit3.IndexAllBufferedTerms(), IsOk());

  // Should return "foo" and "fool" which are in namespaces with ids 1 and 2.
  EXPECT_THAT(
      index_->FindTermsByPrefix(/*prefix=*/"f", /*namespace_ids=*/{1, 2},
                                /*num_to_return=*/10),
      IsOkAndHolds(UnorderedElementsAre(EqualsTermMetadata("foo", 1),
                                        EqualsTermMetadata("fool", 1))));

  ICING_ASSERT_OK(index_->Merge());

  EXPECT_THAT(
      index_->FindTermsByPrefix(/*prefix=*/"f", /*namespace_ids=*/{1, 2},
                                /*num_to_return=*/10),
      IsOkAndHolds(UnorderedElementsAre(
          EqualsTermMetadata("foo", kMinSizePlApproxHits),
          EqualsTermMetadata("fool", kMinSizePlApproxHits))));
}

TEST_F(IndexTest, FindTermByPrefixShouldReturnTermsInAllNamespaces) {
  Index::Editor edit1 =
      index_->Edit(kDocumentId0, kSectionId2, TermMatchType::EXACT_ONLY,
                   /*namespace_id=*/0);
  EXPECT_THAT(edit1.BufferTerm("fo"), IsOk());
  EXPECT_THAT(edit1.IndexAllBufferedTerms(), IsOk());

  Index::Editor edit2 =
      index_->Edit(kDocumentId1, kSectionId2, TermMatchType::EXACT_ONLY,
                   /*namespace_id=*/1);
  EXPECT_THAT(edit2.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit2.IndexAllBufferedTerms(), IsOk());

  Index::Editor edit3 =
      index_->Edit(kDocumentId2, kSectionId2, TermMatchType::EXACT_ONLY,
                   /*namespace_id=*/2);
  EXPECT_THAT(edit3.BufferTerm("fool"), IsOk());
  EXPECT_THAT(edit3.IndexAllBufferedTerms(), IsOk());

  // Should return "fo", "foo" and "fool" across all namespaces.
  EXPECT_THAT(index_->FindTermsByPrefix(/*prefix=*/"f", /*namespace_ids=*/{},
                                        /*num_to_return=*/10),
              IsOkAndHolds(UnorderedElementsAre(
                  EqualsTermMetadata("fo", 1), EqualsTermMetadata("foo", 1),
                  EqualsTermMetadata("fool", 1))));

  ICING_ASSERT_OK(index_->Merge());

  // Should return "fo", "foo" and "fool" across all namespaces.
  EXPECT_THAT(index_->FindTermsByPrefix(/*prefix=*/"f", /*namespace_ids=*/{},
                                        /*num_to_return=*/10),
              IsOkAndHolds(UnorderedElementsAre(
                  EqualsTermMetadata("fo", kMinSizePlApproxHits),
                  EqualsTermMetadata("foo", kMinSizePlApproxHits),
                  EqualsTermMetadata("fool", kMinSizePlApproxHits))));
}

TEST_F(IndexTest, FindTermByPrefixShouldReturnCorrectHitCount) {
  Index::Editor edit1 =
      index_->Edit(kDocumentId0, kSectionId2, TermMatchType::EXACT_ONLY,
                   /*namespace_id=*/0);
  EXPECT_THAT(edit1.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit1.BufferTerm("fool"), IsOk());
  EXPECT_THAT(edit1.IndexAllBufferedTerms(), IsOk());

  Index::Editor edit2 =
      index_->Edit(kDocumentId1, kSectionId2, TermMatchType::EXACT_ONLY,
                   /*namespace_id=*/0);
  EXPECT_THAT(edit2.BufferTerm("fool"), IsOk());
  EXPECT_THAT(edit2.IndexAllBufferedTerms(), IsOk());

  // 'foo' has 1 hit, 'fool' has 2 hits.
  EXPECT_THAT(
      index_->FindTermsByPrefix(/*prefix=*/"f", /*namespace_ids=*/{0},
                                /*num_to_return=*/10),
      IsOkAndHolds(UnorderedElementsAre(EqualsTermMetadata("foo", 1),
                                        EqualsTermMetadata("fool", 2))));

  ICING_ASSERT_OK(index_->Merge());

  // foo's one hit should fit on a min-sized pl, fool's two hits should also fit
  // on a min-sized pl.
  EXPECT_THAT(index_->FindTermsByPrefix(/*prefix=*/"f", /*namespace_ids=*/{0},
                                        /*num_to_return=*/10),
              IsOkAndHolds(UnorderedElementsAre(
                  EqualsTermMetadata("foo", kMinSizePlApproxHits),
                  EqualsTermMetadata("fool", kMinSizePlApproxHits))));
}

TEST_F(IndexTest, FindTermByPrefixShouldReturnApproximateHitCountForMain) {
  Index::Editor edit =
      index_->Edit(kDocumentId0, kSectionId2, TermMatchType::EXACT_ONLY,
                   /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.BufferTerm("fool"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  edit = index_->Edit(kDocumentId1, kSectionId2, TermMatchType::EXACT_ONLY,
                      /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("fool"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  edit = index_->Edit(kDocumentId2, kSectionId2, TermMatchType::EXACT_ONLY,
                      /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("fool"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  edit = index_->Edit(kDocumentId3, kSectionId2, TermMatchType::EXACT_ONLY,
                      /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("fool"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  edit = index_->Edit(kDocumentId4, kSectionId2, TermMatchType::EXACT_ONLY,
                      /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("fool"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  edit = index_->Edit(kDocumentId5, kSectionId2, TermMatchType::EXACT_ONLY,
                      /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("fool"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  edit = index_->Edit(kDocumentId6, kSectionId2, TermMatchType::EXACT_ONLY,
                      /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("fool"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  edit = index_->Edit(kDocumentId7, kSectionId2, TermMatchType::EXACT_ONLY,
                      /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("fool"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  // 'foo' has 1 hit, 'fool' has 8 hits.
  EXPECT_THAT(
      index_->FindTermsByPrefix(/*prefix=*/"f", /*namespace_ids=*/{0},
                                /*num_to_return=*/10),
      IsOkAndHolds(UnorderedElementsAre(EqualsTermMetadata("foo", 1),
                                        EqualsTermMetadata("fool", 8))));

  ICING_ASSERT_OK(index_->Merge());

  // foo's hits should fit on a single pl. fool's hits will need two pls.
  EXPECT_THAT(index_->FindTermsByPrefix(/*prefix=*/"f", /*namespace_ids=*/{0},
                                        /*num_to_return=*/10),
              IsOkAndHolds(UnorderedElementsAre(
                  EqualsTermMetadata("foo", kMinSizePlApproxHits),
                  EqualsTermMetadata("fool", kSecondSmallestPlApproxHits))));
}

TEST_F(IndexTest, FindTermByPrefixShouldReturnCombinedHitCount) {
  Index::Editor edit =
      index_->Edit(kDocumentId0, kSectionId2, TermMatchType::EXACT_ONLY,
                   /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.BufferTerm("fool"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  ICING_ASSERT_OK(index_->Merge());

  edit = index_->Edit(kDocumentId1, kSectionId2, TermMatchType::EXACT_ONLY,
                      /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("fool"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  // 'foo' has 1 hit in the main index, 'fool' has 1 hit in the main index and
  // 1 hit in the lite index.
  EXPECT_THAT(index_->FindTermsByPrefix(/*prefix=*/"f", /*namespace_ids=*/{0},
                                        /*num_to_return=*/10),
              IsOkAndHolds(UnorderedElementsAre(
                  EqualsTermMetadata("foo", kMinSizePlApproxHits),
                  EqualsTermMetadata("fool", kMinSizePlApproxHits + 1))));
}

TEST_F(IndexTest, FindTermByPrefixShouldReturnTermsFromBothIndices) {
  Index::Editor edit =
      index_->Edit(kDocumentId0, kSectionId2, TermMatchType::EXACT_ONLY,
                   /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  ICING_ASSERT_OK(index_->Merge());

  edit = index_->Edit(kDocumentId1, kSectionId2, TermMatchType::EXACT_ONLY,
                      /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("fool"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  // 'foo' has 1 hit in the main index, 'fool' has 1 hit in the lite index.
  EXPECT_THAT(index_->FindTermsByPrefix(/*prefix=*/"f", /*namespace_ids=*/{0},
                                        /*num_to_return=*/10),
              IsOkAndHolds(UnorderedElementsAre(
                  EqualsTermMetadata("foo", kMinSizePlApproxHits),
                  EqualsTermMetadata("fool", 1))));
}

TEST_F(IndexTest, GetElementsSize) {
  // Check empty index.
  ICING_ASSERT_OK_AND_ASSIGN(int64_t size, index_->GetElementsSize());
  EXPECT_THAT(size, Eq(0));

  // Add an element.
  Index::Editor edit = index_->Edit(
      kDocumentId0, kSectionId2, TermMatchType::EXACT_ONLY, /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  ICING_ASSERT_OK_AND_ASSIGN(size, index_->GetElementsSize());
  EXPECT_THAT(size, Gt(0));

  ASSERT_THAT(index_->Merge(), IsOk());
  ICING_ASSERT_OK_AND_ASSIGN(size, index_->GetElementsSize());
  EXPECT_THAT(size, Gt(0));
}

TEST_F(IndexTest, ExactResultsFromLiteAndMain) {
  Index::Editor edit = index_->Edit(
      kDocumentId0, kSectionId2, TermMatchType::EXACT_ONLY, /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.BufferTerm("fool"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  edit = index_->Edit(kDocumentId1, kSectionId3, TermMatchType::PREFIX,
                      /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("foot"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  ICING_ASSERT_OK(index_->Merge());

  edit = index_->Edit(kDocumentId2, kSectionId2, TermMatchType::EXACT_ONLY,
                      /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("footer"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  edit = index_->Edit(kDocumentId2, kSectionId3, TermMatchType::PREFIX,
                      /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("foo", kSectionIdMaskAll, TermMatchType::EXACT_ONLY));
  EXPECT_THAT(
      GetHits(std::move(itr)),
      ElementsAre(
          EqualsDocHitInfo(kDocumentId2, std::vector<SectionId>{kSectionId3}),
          EqualsDocHitInfo(kDocumentId0, std::vector<SectionId>{kSectionId2})));
}

TEST_F(IndexTest, PrefixResultsFromLiteAndMain) {
  Index::Editor edit = index_->Edit(
      kDocumentId0, kSectionId2, TermMatchType::EXACT_ONLY, /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.BufferTerm("fool"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  edit = index_->Edit(kDocumentId1, kSectionId3, TermMatchType::PREFIX,
                      /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("foot"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  ICING_ASSERT_OK(index_->Merge());

  edit = index_->Edit(kDocumentId2, kSectionId2, TermMatchType::EXACT_ONLY,
                      /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("footer"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  edit = index_->Edit(kDocumentId2, kSectionId3, TermMatchType::PREFIX,
                      /*namespace_id=*/0);
  EXPECT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("foo", kSectionIdMaskAll, TermMatchType::PREFIX));
  EXPECT_THAT(
      GetHits(std::move(itr)),
      ElementsAre(
          EqualsDocHitInfo(kDocumentId2, std::vector<SectionId>{kSectionId3}),
          EqualsDocHitInfo(kDocumentId1, std::vector<SectionId>{kSectionId3}),
          EqualsDocHitInfo(kDocumentId0, std::vector<SectionId>{kSectionId2})));
}

TEST_F(IndexTest, GetDebugInfo) {
  // Add two documents to the lite index, merge them into the main index and
  // then add another doc to the lite index.
  Index::Editor edit = index_->Edit(
      kDocumentId0, kSectionId2, TermMatchType::EXACT_ONLY, /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("foo"), IsOk());
  ASSERT_THAT(edit.BufferTerm("fool"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  edit = index_->Edit(kDocumentId1, kSectionId3, TermMatchType::PREFIX,
                      /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("foot"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  ICING_ASSERT_OK(index_->Merge());

  edit = index_->Edit(kDocumentId2, kSectionId2, TermMatchType::EXACT_ONLY,
                      /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("footer"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  edit = index_->Edit(kDocumentId2, kSectionId3, TermMatchType::PREFIX,
                      /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  std::string out0;
  index_->GetDebugInfo(/*verbosity=*/0, &out0);
  EXPECT_THAT(out0, Not(IsEmpty()));

  std::string out1;
  index_->GetDebugInfo(/*verbosity=*/1, &out1);
  EXPECT_THAT(out1, SizeIs(Gt(out0.size())));

  // Add one more doc to the lite index. Debug strings should change.
  edit = index_->Edit(kDocumentId3, kSectionId2, TermMatchType::EXACT_ONLY,
                      /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("far"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  std::string out2;
  index_->GetDebugInfo(/*verbosity=*/0, &out2);
  EXPECT_THAT(out2, Ne(out0));

  std::string out3;
  index_->GetDebugInfo(/*verbosity=*/1, &out3);
  EXPECT_THAT(out3, Ne(out1));

  // Merge into the man index. Debuug strings should change again.
  ICING_ASSERT_OK(index_->Merge());

  std::string out4;
  index_->GetDebugInfo(/*verbosity=*/0, &out4);
  EXPECT_THAT(out4, Ne(out0));
  EXPECT_THAT(out4, Ne(out2));

  std::string out5;
  index_->GetDebugInfo(/*verbosity=*/1, &out5);
  EXPECT_THAT(out5, Ne(out1));
  EXPECT_THAT(out5, Ne(out3));
}

TEST_F(IndexTest, BackfillingMultipleTermsSucceeds) {
  // Add two documents to the lite index, merge them into the main index and
  // then add another doc to the lite index.
  Index::Editor edit = index_->Edit(
      kDocumentId0, kSectionId2, TermMatchType::EXACT_ONLY, /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  edit = index_->Edit(kDocumentId0, kSectionId3, TermMatchType::PREFIX,
                      /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("fool"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  edit = index_->Edit(kDocumentId1, kSectionId3, TermMatchType::PREFIX,
                      /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("foot"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  // After this merge the index should have posting lists for
  // "fool" {(doc0,sec3)},
  // "foot" {(doc1,sec3)},
  // "foo"  {(doc1,sec3),(doc0,sec3),(doc0,sec2)}
  ICING_ASSERT_OK(index_->Merge());

  // Add one more doc to the lite index.
  edit = index_->Edit(kDocumentId2, kSectionId2, TermMatchType::EXACT_ONLY,
                      /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("far"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  // After this merge the index should add a posting list for "far" and a
  // backfill branch point for "f". In addition to the posting lists described
  // above, which are unaffected, the new posting lists should be
  // "far" {(doc2,sec2)},
  // "f"   {(doc1,sec3),(doc0,sec3)}
  // Multiple pre-existing hits should be added to the new backfill branch
  // point.
  ICING_ASSERT_OK(index_->Merge());

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("f", kSectionIdMaskAll, TermMatchType::PREFIX));
  EXPECT_THAT(
      GetHits(std::move(itr)),
      ElementsAre(
          EqualsDocHitInfo(kDocumentId1, std::vector<SectionId>{kSectionId3}),
          EqualsDocHitInfo(kDocumentId0, std::vector<SectionId>{kSectionId3})));
}

TEST_F(IndexTest, BackfillingNewTermsSucceeds) {
  // Add two documents to the lite index, merge them into the main index and
  // then add another doc to the lite index.
  Index::Editor edit = index_->Edit(
      kDocumentId0, kSectionId2, TermMatchType::EXACT_ONLY, /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("foo"), IsOk());
  ASSERT_THAT(edit.BufferTerm("fool"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  edit = index_->Edit(kDocumentId1, kSectionId3, TermMatchType::PREFIX,
                      /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("foot"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  // After this merge the index should have posting lists for
  // "fool" {(doc0,sec2)},
  // "foot" {(doc1,sec3)},
  // "foo"  {(doc1,sec3),(doc0,sec2)}
  ICING_ASSERT_OK(index_->Merge());

  edit = index_->Edit(kDocumentId2, kSectionId2, TermMatchType::EXACT_ONLY,
                      /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("footer"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  edit = index_->Edit(kDocumentId2, kSectionId3, TermMatchType::PREFIX,
                      /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  // Add one more doc to the lite index. Debug strings should change.
  edit = index_->Edit(kDocumentId3, kSectionId2, TermMatchType::EXACT_ONLY,
                      /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("far"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  // After this merge the index should add posting lists for "far" and "footer"
  // and a backfill branch point for "f". The new posting lists should be
  // "fool"    {(doc0,sec2)},
  // "foot"    {(doc1,sec3)},
  // "foo"     {(doc2,sec3),(doc1,sec3),(doc0,sec2)}
  // "footer"  {(doc2,sec2)},
  // "far"     {(doc3,sec2)},
  // "f"       {(doc2,sec3),(doc1,sec3)}
  // Multiple pre-existing hits should be added to the new backfill branch
  // point.
  ICING_ASSERT_OK(index_->Merge());

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("f", kSectionIdMaskAll, TermMatchType::PREFIX));
  EXPECT_THAT(
      GetHits(std::move(itr)),
      ElementsAre(
          EqualsDocHitInfo(kDocumentId2, std::vector<SectionId>{kSectionId3}),
          EqualsDocHitInfo(kDocumentId1, std::vector<SectionId>{kSectionId3})));
}

TEST_F(IndexTest, TruncateToInvalidDocumentIdHasNoEffect) {
  ICING_EXPECT_OK(index_->TruncateTo(kInvalidDocumentId));
  EXPECT_THAT(index_->GetElementsSize(), IsOkAndHolds(0));
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("f", kSectionIdMaskAll, TermMatchType::PREFIX));
  EXPECT_THAT(GetHits(std::move(itr)), IsEmpty());

  // Add one document to the lite index
  Index::Editor edit = index_->Edit(kDocumentId0, kSectionId2,
                                    TermMatchType::PREFIX, /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  // Clipping to invalid should have no effect.
  ICING_EXPECT_OK(index_->TruncateTo(kInvalidDocumentId));
  ICING_ASSERT_OK_AND_ASSIGN(
      itr, index_->GetIterator("f", kSectionIdMaskAll, TermMatchType::PREFIX));
  EXPECT_THAT(GetHits(std::move(itr)),
              ElementsAre(EqualsDocHitInfo(
                  kDocumentId0, std::vector<SectionId>{kSectionId2})));

  // Clipping to invalid should still have no effect even if hits are in main.
  ICING_ASSERT_OK(index_->Merge());
  ICING_EXPECT_OK(index_->TruncateTo(kInvalidDocumentId));
  ICING_ASSERT_OK_AND_ASSIGN(
      itr, index_->GetIterator("f", kSectionIdMaskAll, TermMatchType::PREFIX));
  EXPECT_THAT(GetHits(std::move(itr)),
              ElementsAre(EqualsDocHitInfo(
                  kDocumentId0, std::vector<SectionId>{kSectionId2})));

  edit = index_->Edit(kDocumentId1, kSectionId3, TermMatchType::PREFIX,
                      /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("foot"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

  // Clipping to invalid should still have no effect even if both indices have
  // hits.
  ICING_EXPECT_OK(index_->TruncateTo(kInvalidDocumentId));
  ICING_ASSERT_OK_AND_ASSIGN(
      itr, index_->GetIterator("f", kSectionIdMaskAll, TermMatchType::PREFIX));
  EXPECT_THAT(
      GetHits(std::move(itr)),
      ElementsAre(
          EqualsDocHitInfo(kDocumentId1, std::vector<SectionId>{kSectionId3}),
          EqualsDocHitInfo(kDocumentId0, std::vector<SectionId>{kSectionId2})));
}

TEST_F(IndexTest, TruncateToLastAddedDocumentIdHasNoEffect) {
  ICING_EXPECT_OK(index_->TruncateTo(index_->last_added_document_id()));
  EXPECT_THAT(index_->GetElementsSize(), IsOkAndHolds(0));
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("f", kSectionIdMaskAll, TermMatchType::PREFIX));
  EXPECT_THAT(GetHits(std::move(itr)), IsEmpty());

  // Add one document to the lite index
  Index::Editor edit = index_->Edit(kDocumentId0, kSectionId2,
                                    TermMatchType::PREFIX, /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  index_->set_last_added_document_id(kDocumentId0);
  ICING_EXPECT_OK(index_->TruncateTo(index_->last_added_document_id()));
  // Clipping to invalid should have no effect.
  ICING_ASSERT_OK_AND_ASSIGN(
      itr, index_->GetIterator("f", kSectionIdMaskAll, TermMatchType::PREFIX));
  EXPECT_THAT(GetHits(std::move(itr)),
              ElementsAre(EqualsDocHitInfo(
                  kDocumentId0, std::vector<SectionId>{kSectionId2})));

  // Clipping to invalid should still have no effect even if hits are in main.
  ICING_ASSERT_OK(index_->Merge());
  ICING_EXPECT_OK(index_->TruncateTo(index_->last_added_document_id()));
  ICING_ASSERT_OK_AND_ASSIGN(
      itr, index_->GetIterator("f", kSectionIdMaskAll, TermMatchType::PREFIX));
  EXPECT_THAT(GetHits(std::move(itr)),
              ElementsAre(EqualsDocHitInfo(
                  kDocumentId0, std::vector<SectionId>{kSectionId2})));

  edit = index_->Edit(kDocumentId1, kSectionId3, TermMatchType::PREFIX,
                      /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("foot"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  index_->set_last_added_document_id(kDocumentId1);

  // Clipping to invalid should still have no effect even if both indices have
  // hits.
  ICING_EXPECT_OK(index_->TruncateTo(index_->last_added_document_id()));
  ICING_ASSERT_OK_AND_ASSIGN(
      itr, index_->GetIterator("f", kSectionIdMaskAll, TermMatchType::PREFIX));
  EXPECT_THAT(
      GetHits(std::move(itr)),
      ElementsAre(
          EqualsDocHitInfo(kDocumentId1, std::vector<SectionId>{kSectionId3}),
          EqualsDocHitInfo(kDocumentId0, std::vector<SectionId>{kSectionId2})));
}

TEST_F(IndexTest, TruncateToThrowsOutLiteIndex) {
  // Add one document to the lite index and merge it into main.
  Index::Editor edit = index_->Edit(kDocumentId0, kSectionId2,
                                    TermMatchType::PREFIX, /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  index_->set_last_added_document_id(kDocumentId0);

  ICING_ASSERT_OK(index_->Merge());

  // Add another document to the lite index.
  edit = index_->Edit(kDocumentId1, kSectionId3, TermMatchType::PREFIX,
                      /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("foot"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  index_->set_last_added_document_id(kDocumentId1);

  EXPECT_THAT(index_->TruncateTo(kDocumentId0), IsOk());

  // Clipping to document 0 should toss out the lite index, but keep the main.
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("f", kSectionIdMaskAll, TermMatchType::PREFIX));
  EXPECT_THAT(GetHits(std::move(itr)),
              ElementsAre(EqualsDocHitInfo(
                  kDocumentId0, std::vector<SectionId>{kSectionId2})));
}

TEST_F(IndexTest, TruncateToThrowsOutBothIndices) {
  // Add two documents to the lite index and merge them into main.
  Index::Editor edit = index_->Edit(kDocumentId0, kSectionId2,
                                    TermMatchType::PREFIX, /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("foo"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  index_->set_last_added_document_id(kDocumentId0);
  edit = index_->Edit(kDocumentId1, kSectionId2, TermMatchType::PREFIX,
                      /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("foul"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  index_->set_last_added_document_id(kDocumentId1);

  ICING_ASSERT_OK(index_->Merge());

  // Add another document to the lite index.
  edit = index_->Edit(kDocumentId2, kSectionId3, TermMatchType::PREFIX,
                      /*namespace_id=*/0);
  ASSERT_THAT(edit.BufferTerm("foot"), IsOk());
  EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
  index_->set_last_added_document_id(kDocumentId2);

  EXPECT_THAT(index_->TruncateTo(kDocumentId0), IsOk());

  // Clipping to document 0 should toss out both indices.
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<DocHitInfoIterator> itr,
      index_->GetIterator("f", kSectionIdMaskAll, TermMatchType::PREFIX));
  EXPECT_THAT(GetHits(std::move(itr)), IsEmpty());
}

TEST_F(IndexTest, IndexStorageInfoProto) {
  // Add two documents to the lite index and merge them into main.
  {
    Index::Editor edit = index_->Edit(
        kDocumentId0, kSectionId2, TermMatchType::PREFIX, /*namespace_id=*/0);
    ASSERT_THAT(edit.BufferTerm("foo"), IsOk());
    EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());
    edit = index_->Edit(kDocumentId1, kSectionId2, TermMatchType::PREFIX,
                        /*namespace_id=*/0);
    ASSERT_THAT(edit.BufferTerm("foul"), IsOk());
    EXPECT_THAT(edit.IndexAllBufferedTerms(), IsOk());

    ICING_ASSERT_OK(index_->Merge());
  }

  IndexStorageInfoProto storage_info = index_->GetStorageInfo();
  EXPECT_THAT(storage_info.index_size(), Ge(0));
  EXPECT_THAT(storage_info.lite_index_lexicon_size(), Ge(0));
  EXPECT_THAT(storage_info.lite_index_hit_buffer_size(), Ge(0));
  EXPECT_THAT(storage_info.main_index_lexicon_size(), Ge(0));
  EXPECT_THAT(storage_info.main_index_storage_size(), Ge(0));
  EXPECT_THAT(storage_info.main_index_block_size(), Ge(0));
  // There should be 1 block for the header and 1 block for two posting lists.
  EXPECT_THAT(storage_info.num_blocks(), Eq(2));
  EXPECT_THAT(storage_info.min_free_fraction(), Ge(0));
}

}  // namespace

}  // namespace lib
}  // namespace icing
