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

#include "icing/index/hit/doc-hit-info.h"

#include "icing/index/hit/hit.h"
#include "icing/schema/section.h"
#include "icing/store/document-id.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace icing {
namespace lib {

using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::IsTrue;
using ::testing::Ne;

constexpr DocumentId kSomeDocumentId = 12;
constexpr DocumentId kSomeOtherDocumentId = 54;

TEST(DocHitInfoTest, InitialMaxHitTermFrequencies) {
  DocHitInfo info(kSomeDocumentId);
  for (SectionId i = 0; i <= kMaxSectionId; ++i) {
    EXPECT_THAT(info.hit_term_frequency(i), Eq(Hit::kNoTermFrequency));
  }
}

TEST(DocHitInfoTest, UpdateHitTermFrequenciesForTheFirstTime) {
  DocHitInfo info(kSomeDocumentId);
  ASSERT_THAT(info.hit_term_frequency(3), Eq(Hit::kNoTermFrequency));

  // Updating a section for the first time, should change its hit
  // term_frequency
  info.UpdateSection(3, 16);
  EXPECT_THAT(info.hit_term_frequency(3), Eq(16));
}

TEST(DocHitInfoTest, UpdateSectionLowerHitTermFrequencyHasNoEffect) {
  DocHitInfo info(kSomeDocumentId);
  info.UpdateSection(3, 16);
  ASSERT_THAT(info.hit_term_frequency(3), Eq(16));

  // Updating a section with a term frequency lower than the previously set
  // one should have no effect.
  info.UpdateSection(3, 15);
  EXPECT_THAT(info.hit_term_frequency(3), Eq(16));
}

TEST(DocHitInfoTest, UpdateSectionHigherHitTermFrequencyHasNoEffect) {
  DocHitInfo info(kSomeDocumentId);
  info.UpdateSection(3, 16);
  ASSERT_THAT(info.hit_term_frequency(3), Eq(16));

  // Updating a section with a term frequency higher than the previously set
  // one should have no effect.
  info.UpdateSection(3, 17);
  EXPECT_THAT(info.hit_term_frequency(3), Eq(16));
}

TEST(DocHitInfoTest, UpdateSectionIdMask) {
  DocHitInfo info(kSomeDocumentId);
  EXPECT_THAT(info.hit_section_ids_mask(), Eq(kSectionIdMaskNone));

  info.UpdateSection(3, 16);
  EXPECT_THAT(info.hit_section_ids_mask() & 1U << 3, IsTrue());

  // Calling update again shouldn't do anything
  info.UpdateSection(3, 15);
  EXPECT_THAT(info.hit_section_ids_mask() & 1U << 3, IsTrue());

  // Updating another section shouldn't do anything
  info.UpdateSection(2, 77);
  EXPECT_THAT(info.hit_section_ids_mask() & 1U << 3, IsTrue());
}

TEST(DocHitInfoTest, MergeSectionsFromDifferentDocumentId) {
  // Merging infos with different document_ids works.
  DocHitInfo info1(kSomeDocumentId);
  DocHitInfo info2(kSomeOtherDocumentId);
  info2.UpdateSection(7, 12);
  info1.MergeSectionsFrom(info2);
  EXPECT_THAT(info1.hit_term_frequency(7), Eq(12));
  EXPECT_THAT(info1.document_id(), Eq(kSomeDocumentId));
}

TEST(DocHitInfoTest, MergeSectionsFromKeepsOldSection) {
  // Merging shouldn't override sections that are present info1, but not present
  // in info2.
  DocHitInfo info1(kSomeDocumentId);
  info1.UpdateSection(3, 16);
  DocHitInfo info2(kSomeDocumentId);
  info1.MergeSectionsFrom(info2);
  EXPECT_THAT(info1.hit_term_frequency(3), Eq(16));
}

TEST(DocHitInfoTest, MergeSectionsFromAddsNewSection) {
  // Merging should add sections that were not present in info1, but are present
  // in info2.
  DocHitInfo info1(kSomeDocumentId);
  DocHitInfo info2(kSomeDocumentId);
  info2.UpdateSection(7, 12);
  info1.MergeSectionsFrom(info2);
  EXPECT_THAT(info1.hit_term_frequency(7), Eq(12));
}

TEST(DocHitInfoTest, MergeSectionsFromHigherHitTermFrequencyHasNoEffect) {
  // Merging should not override the value of a section in info1 if the same
  // section is present in info2.
  DocHitInfo info1(kSomeDocumentId);
  info1.UpdateSection(2, 77);
  DocHitInfo info2(kSomeDocumentId);
  info2.UpdateSection(2, 89);
  info1.MergeSectionsFrom(info2);
  EXPECT_THAT(info1.hit_term_frequency(2), Eq(77));
}

TEST(DocHitInfoTest, MergeSectionsFromLowerHitScoreHasNoEffect) {
  // Merging should not override the hit score of a section in info1 if the same
  // section is present in info2.
  DocHitInfo info1(kSomeDocumentId);
  info1.UpdateSection(5, 108);
  DocHitInfo info2(kSomeDocumentId);
  info2.UpdateSection(5, 13);
  info1.MergeSectionsFrom(info2);
  EXPECT_THAT(info1.hit_term_frequency(5), Eq(108));
}

TEST(DocHitInfoTest, Comparison) {
  constexpr DocumentId kDocumentId = 1;
  DocHitInfo info(kDocumentId);
  info.UpdateSection(1, 12);

  constexpr DocumentId kHighDocumentId = 15;
  DocHitInfo high_document_id_info(kHighDocumentId);
  high_document_id_info.UpdateSection(1, 12);

  DocHitInfo high_section_id_info(kDocumentId);
  high_section_id_info.UpdateSection(1, 12);
  high_section_id_info.UpdateSection(6, Hit::kDefaultTermFrequency);

  std::vector<DocHitInfo> infos{info, high_document_id_info,
                                high_section_id_info};
  std::sort(infos.begin(), infos.end());
  EXPECT_THAT(infos,
              ElementsAre(high_document_id_info, info, high_section_id_info));

  // There are no requirements for how DocHitInfos with the same DocumentIds and
  // hit masks will compare, but they must not be equal.
  DocHitInfo different_term_frequency_info(kDocumentId);
  different_term_frequency_info.UpdateSection(1, 76);
  EXPECT_THAT(info < different_term_frequency_info,
              Ne(different_term_frequency_info < info));
}

}  // namespace lib
}  // namespace icing
