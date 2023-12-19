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

#include "icing/index/iterator/doc-hit-info-iterator-all-document-id.h"

#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "icing/index/iterator/doc-hit-info-iterator-test-util.h"
#include "icing/index/iterator/doc-hit-info-iterator.h"
#include "icing/schema/section.h"
#include "icing/store/document-id.h"
#include "icing/testing/common-matchers.h"

namespace icing {
namespace lib {

namespace {

using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::Not;

TEST(DocHitInfoIteratorAllDocumentIdTest, Initialize) {
  {
    DocHitInfoIteratorAllDocumentId all_it(100);

    // We'll always start with an invalid document_id, need to Advance before we
    // get anything out of this.
    EXPECT_THAT(all_it.doc_hit_info().document_id(), Eq(kInvalidDocumentId));
    EXPECT_THAT(all_it.hit_intersect_section_ids_mask(),
                Eq(kSectionIdMaskNone));
  }

  {
    // Can initialize with negative values, but won't ever be able to Advance to
    // a proper document_id
    DocHitInfoIteratorAllDocumentId all_it(-5);
    EXPECT_THAT(all_it.Advance(), Not(IsOk()));
  }
}

TEST(DocHitInfoIteratorAllDocumentIdTest, GetNumBlocksInspected) {
  DocHitInfoIteratorAllDocumentId all_it(100);
  EXPECT_THAT(all_it.GetNumBlocksInspected(), Eq(0));

  // Number of iterations is chosen arbitrarily. Just meant to demonstrate that
  // no matter how many Advance calls are made, GetNumBlocksInspected should
  // always return 0.
  for (int i = 0; i < 5; ++i) {
    EXPECT_THAT(all_it.Advance(), IsOk());
    EXPECT_THAT(all_it.GetNumBlocksInspected(), Eq(0));
  }
}

TEST(DocHitInfoIteratorAllDocumentIdTest, GetNumLeafAdvanceCalls) {
  DocHitInfoIteratorAllDocumentId all_it(100);
  EXPECT_THAT(all_it.GetNumLeafAdvanceCalls(), Eq(0));

  for (int i = 1; i <= 5; ++i) {
    EXPECT_THAT(all_it.Advance(), IsOk());
    EXPECT_THAT(all_it.GetNumLeafAdvanceCalls(), Eq(i));
  }
}

TEST(DocHitInfoIteratorAllDocumentIdTest, Advance) {
  {
    // Can't advance beyond an invalid DocumentId
    EXPECT_THAT(DocHitInfoIteratorAllDocumentId(-1).Advance(), Not(IsOk()));
  }

  {
    // Test one advance
    DocHitInfoIteratorAllDocumentId all_it(5);
    EXPECT_THAT(all_it.Advance(), IsOk());
    EXPECT_THAT(all_it.doc_hit_info().document_id(), Eq(5));

    // Advancing shouldn't affect the intersect section ids mask, since there's
    // no intersecting going on
    EXPECT_THAT(all_it.hit_intersect_section_ids_mask(),
                Eq(kSectionIdMaskNone));
  }

  {
    std::vector<DocumentId> expected_document_ids;
    expected_document_ids.reserve(125);
    for (int i = 124; i >= 0; --i) {
      expected_document_ids.push_back(i);
    }

    // Many advances
    DocHitInfoIteratorAllDocumentId all_it(124);
    EXPECT_THAT(GetDocumentIds(&all_it),
                ElementsAreArray(expected_document_ids));
  }
}

}  // namespace
}  // namespace lib
}  // namespace icing
