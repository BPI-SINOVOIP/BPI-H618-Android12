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

#include "icing/result/result-state-manager.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "icing/file/filesystem.h"
#include "icing/portable/equals-proto.h"
#include "icing/schema/schema-store.h"
#include "icing/store/document-store.h"
#include "icing/testing/common-matchers.h"
#include "icing/testing/tmp-directory.h"
#include "icing/util/clock.h"

namespace icing {
namespace lib {
namespace {
using ::icing::lib::portable_equals_proto::EqualsProto;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Gt;
using ::testing::IsEmpty;

ScoringSpecProto CreateScoringSpec() {
  ScoringSpecProto scoring_spec;
  scoring_spec.set_rank_by(ScoringSpecProto::RankingStrategy::DOCUMENT_SCORE);
  return scoring_spec;
}

ResultSpecProto CreateResultSpec(int num_per_page) {
  ResultSpecProto result_spec;
  result_spec.set_num_per_page(num_per_page);
  return result_spec;
}

ScoredDocumentHit CreateScoredHit(DocumentId document_id) {
  return ScoredDocumentHit(document_id, kSectionIdMaskNone, /*score=*/1);
}

class ResultStateManagerTest : public testing::Test {
 protected:
  void SetUp() override {
    schema_store_base_dir_ = GetTestTempDir() + "/schema_store";
    filesystem_.CreateDirectoryRecursively(schema_store_base_dir_.c_str());
    ICING_ASSERT_OK_AND_ASSIGN(
        schema_store_,
        SchemaStore::Create(&filesystem_, schema_store_base_dir_, &clock_));
    SchemaProto schema;
    schema.add_types()->set_schema_type("Document");
    ICING_ASSERT_OK(schema_store_->SetSchema(std::move(schema)));

    doc_store_base_dir_ = GetTestTempDir() + "/document_store";
    filesystem_.CreateDirectoryRecursively(doc_store_base_dir_.c_str());
    ICING_ASSERT_OK_AND_ASSIGN(
        DocumentStore::CreateResult result,
        DocumentStore::Create(&filesystem_, doc_store_base_dir_, &clock_,
                              schema_store_.get()));
    document_store_ = std::move(result.document_store);
  }

  void TearDown() override {
    filesystem_.DeleteDirectoryRecursively(doc_store_base_dir_.c_str());
    filesystem_.DeleteDirectoryRecursively(schema_store_base_dir_.c_str());
  }

  ResultState CreateResultState(
      const std::vector<ScoredDocumentHit>& scored_document_hits,
      int num_per_page) {
    return ResultState(scored_document_hits, /*query_terms=*/{},
                       SearchSpecProto::default_instance(), CreateScoringSpec(),
                       CreateResultSpec(num_per_page), *document_store_);
  }

  ScoredDocumentHit AddScoredDocument(DocumentId document_id) {
    DocumentProto document;
    document.set_namespace_("namespace");
    document.set_uri(std::to_string(document_id));
    document.set_schema("Document");
    document_store_->Put(std::move(document));
    return ScoredDocumentHit(document_id, kSectionIdMaskNone, /*score=*/1);
  }

  const DocumentStore& document_store() const { return *document_store_; }

 private:
  Filesystem filesystem_;
  std::string doc_store_base_dir_;
  std::string schema_store_base_dir_;
  Clock clock_;
  std::unique_ptr<DocumentStore> document_store_;
  std::unique_ptr<SchemaStore> schema_store_;
};

TEST_F(ResultStateManagerTest, ShouldRankAndPaginateOnePage) {
  ResultState original_result_state =
      CreateResultState({AddScoredDocument(/*document_id=*/0),
                         AddScoredDocument(/*document_id=*/1),
                         AddScoredDocument(/*document_id=*/2)},
                        /*num_per_page=*/10);

  ResultStateManager result_state_manager(
      /*max_total_hits=*/std::numeric_limits<int>::max(), document_store());
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state,
      result_state_manager.RankAndPaginate(std::move(original_result_state)));

  EXPECT_THAT(page_result_state.next_page_token, Eq(kInvalidNextPageToken));

  // Should get the original scored document hits
  EXPECT_THAT(
      page_result_state.scored_document_hits,
      ElementsAre(EqualsScoredDocumentHit(CreateScoredHit(/*document_id=*/2)),
                  EqualsScoredDocumentHit(CreateScoredHit(/*document_id=*/1)),
                  EqualsScoredDocumentHit(CreateScoredHit(/*document_id=*/0))));
}

TEST_F(ResultStateManagerTest, ShouldRankAndPaginateMultiplePages) {
  ResultState original_result_state =
      CreateResultState({AddScoredDocument(/*document_id=*/0),
                         AddScoredDocument(/*document_id=*/1),
                         AddScoredDocument(/*document_id=*/2),
                         AddScoredDocument(/*document_id=*/3),
                         AddScoredDocument(/*document_id=*/4)},
                        /*num_per_page=*/2);

  ResultStateManager result_state_manager(
      /*max_total_hits=*/std::numeric_limits<int>::max(), document_store());

  // First page, 2 results
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state1,
      result_state_manager.RankAndPaginate(std::move(original_result_state)));
  EXPECT_THAT(
      page_result_state1.scored_document_hits,
      ElementsAre(EqualsScoredDocumentHit(CreateScoredHit(/*document_id=*/4)),
                  EqualsScoredDocumentHit(CreateScoredHit(/*document_id=*/3))));

  uint64_t next_page_token = page_result_state1.next_page_token;

  // Second page, 2 results
  ICING_ASSERT_OK_AND_ASSIGN(PageResultState page_result_state2,
                             result_state_manager.GetNextPage(next_page_token));
  EXPECT_THAT(
      page_result_state2.scored_document_hits,
      ElementsAre(EqualsScoredDocumentHit(CreateScoredHit(/*document_id=*/2)),
                  EqualsScoredDocumentHit(CreateScoredHit(/*document_id=*/1))));

  // Third page, 1 result
  ICING_ASSERT_OK_AND_ASSIGN(PageResultState page_result_state3,
                             result_state_manager.GetNextPage(next_page_token));
  EXPECT_THAT(
      page_result_state3.scored_document_hits,
      ElementsAre(EqualsScoredDocumentHit(CreateScoredHit(/*document_id=*/0))));

  // No results
  EXPECT_THAT(result_state_manager.GetNextPage(next_page_token),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
}

TEST_F(ResultStateManagerTest, EmptyStateShouldReturnError) {
  ResultState empty_result_state = CreateResultState({}, /*num_per_page=*/1);

  ResultStateManager result_state_manager(
      /*max_total_hits=*/std::numeric_limits<int>::max(), document_store());
  EXPECT_THAT(
      result_state_manager.RankAndPaginate(std::move(empty_result_state)),
      StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT));
}

TEST_F(ResultStateManagerTest, ShouldInvalidateOneToken) {
  ResultState result_state1 =
      CreateResultState({AddScoredDocument(/*document_id=*/0),
                         AddScoredDocument(/*document_id=*/1),
                         AddScoredDocument(/*document_id=*/2)},
                        /*num_per_page=*/1);
  ResultState result_state2 =
      CreateResultState({AddScoredDocument(/*document_id=*/3),
                         AddScoredDocument(/*document_id=*/4),
                         AddScoredDocument(/*document_id=*/5)},
                        /*num_per_page=*/1);

  ResultStateManager result_state_manager(
      /*max_total_hits=*/std::numeric_limits<int>::max(), document_store());
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state1,
      result_state_manager.RankAndPaginate(std::move(result_state1)));
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state2,
      result_state_manager.RankAndPaginate(std::move(result_state2)));

  result_state_manager.InvalidateResultState(
      page_result_state1.next_page_token);

  // page_result_state1.next_page_token() shouldn't be found
  EXPECT_THAT(
      result_state_manager.GetNextPage(page_result_state1.next_page_token),
      StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));

  // page_result_state2.next_page_token() should still exist
  ICING_ASSERT_OK_AND_ASSIGN(
      page_result_state2,
      result_state_manager.GetNextPage(page_result_state2.next_page_token));
  EXPECT_THAT(
      page_result_state2.scored_document_hits,
      ElementsAre(EqualsScoredDocumentHit(CreateScoredHit(/*document_id=*/4))));
}

TEST_F(ResultStateManagerTest, ShouldInvalidateAllTokens) {
  ResultState result_state1 =
      CreateResultState({AddScoredDocument(/*document_id=*/0),
                         AddScoredDocument(/*document_id=*/1),
                         AddScoredDocument(/*document_id=*/2)},
                        /*num_per_page=*/1);
  ResultState result_state2 =
      CreateResultState({AddScoredDocument(/*document_id=*/3),
                         AddScoredDocument(/*document_id=*/4),
                         AddScoredDocument(/*document_id=*/5)},
                        /*num_per_page=*/1);

  ResultStateManager result_state_manager(
      /*max_total_hits=*/std::numeric_limits<int>::max(), document_store());
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state1,
      result_state_manager.RankAndPaginate(std::move(result_state1)));
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state2,
      result_state_manager.RankAndPaginate(std::move(result_state2)));

  result_state_manager.InvalidateAllResultStates();

  // page_result_state1.next_page_token() shouldn't be found
  EXPECT_THAT(
      result_state_manager.GetNextPage(page_result_state1.next_page_token),
      StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));

  // page_result_state2.next_page_token() shouldn't be found
  EXPECT_THAT(
      result_state_manager.GetNextPage(page_result_state2.next_page_token),
      StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
}

TEST_F(ResultStateManagerTest, ShouldRemoveOldestResultState) {
  ResultState result_state1 =
      CreateResultState({AddScoredDocument(/*document_id=*/0),
                         AddScoredDocument(/*document_id=*/1)},
                        /*num_per_page=*/1);
  ResultState result_state2 =
      CreateResultState({AddScoredDocument(/*document_id=*/2),
                         AddScoredDocument(/*document_id=*/3)},
                        /*num_per_page=*/1);
  ResultState result_state3 =
      CreateResultState({AddScoredDocument(/*document_id=*/4),
                         AddScoredDocument(/*document_id=*/5)},
                        /*num_per_page=*/1);

  ResultStateManager result_state_manager(/*max_total_hits=*/2,
                                          document_store());
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state1,
      result_state_manager.RankAndPaginate(std::move(result_state1)));
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state2,
      result_state_manager.RankAndPaginate(std::move(result_state2)));
  // Adding state 3 should cause state 1 to be removed.
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state3,
      result_state_manager.RankAndPaginate(std::move(result_state3)));

  EXPECT_THAT(
      result_state_manager.GetNextPage(page_result_state1.next_page_token),
      StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));

  ICING_ASSERT_OK_AND_ASSIGN(
      page_result_state2,
      result_state_manager.GetNextPage(page_result_state2.next_page_token));
  EXPECT_THAT(page_result_state2.scored_document_hits,
              ElementsAre(EqualsScoredDocumentHit(CreateScoredHit(
                  /*document_id=*/2))));

  ICING_ASSERT_OK_AND_ASSIGN(
      page_result_state3,
      result_state_manager.GetNextPage(page_result_state3.next_page_token));
  EXPECT_THAT(page_result_state3.scored_document_hits,
              ElementsAre(EqualsScoredDocumentHit(CreateScoredHit(
                  /*document_id=*/4))));
}

TEST_F(ResultStateManagerTest,
       InvalidatedResultStateShouldDecreaseCurrentHitsCount) {
  ResultState result_state1 =
      CreateResultState({AddScoredDocument(/*document_id=*/0),
                         AddScoredDocument(/*document_id=*/1)},
                        /*num_per_page=*/1);
  ResultState result_state2 =
      CreateResultState({AddScoredDocument(/*document_id=*/2),
                         AddScoredDocument(/*document_id=*/3)},
                        /*num_per_page=*/1);
  ResultState result_state3 =
      CreateResultState({AddScoredDocument(/*document_id=*/4),
                         AddScoredDocument(/*document_id=*/5)},
                        /*num_per_page=*/1);

  // Add the first three states. Remember, the first page for each result state
  // won't be cached (since it is returned immediately from RankAndPaginate).
  // Each result state has a page size of 1 and a result set of 2 hits. So each
  // result will take up one hit of our three hit budget.
  ResultStateManager result_state_manager(/*max_total_hits=*/3,
                                          document_store());
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state1,
      result_state_manager.RankAndPaginate(std::move(result_state1)));
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state2,
      result_state_manager.RankAndPaginate(std::move(result_state2)));
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state3,
      result_state_manager.RankAndPaginate(std::move(result_state3)));

  // Invalidates state 2, so that the number of hits current cached should be
  // decremented to 2.
  result_state_manager.InvalidateResultState(
      page_result_state2.next_page_token);

  // If invalidating state 2 correctly decremented the current hit count to 2,
  // then adding state 4 should still be within our budget and no other result
  // states should be evicted.
  ResultState result_state4 =
      CreateResultState({AddScoredDocument(/*document_id=*/6),
                         AddScoredDocument(/*document_id=*/7)},
                        /*num_per_page=*/1);
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state4,
      result_state_manager.RankAndPaginate(std::move(result_state4)));

  ICING_ASSERT_OK_AND_ASSIGN(
      page_result_state1,
      result_state_manager.GetNextPage(page_result_state1.next_page_token));
  EXPECT_THAT(page_result_state1.scored_document_hits,
              ElementsAre(EqualsScoredDocumentHit(CreateScoredHit(
                  /*document_id=*/0))));

  EXPECT_THAT(
      result_state_manager.GetNextPage(page_result_state2.next_page_token),
      StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));

  ICING_ASSERT_OK_AND_ASSIGN(
      page_result_state3,
      result_state_manager.GetNextPage(page_result_state3.next_page_token));
  EXPECT_THAT(page_result_state3.scored_document_hits,
              ElementsAre(EqualsScoredDocumentHit(CreateScoredHit(
                  /*document_id=*/4))));

  ICING_ASSERT_OK_AND_ASSIGN(
      page_result_state4,
      result_state_manager.GetNextPage(page_result_state4.next_page_token));
  EXPECT_THAT(page_result_state4.scored_document_hits,
              ElementsAre(EqualsScoredDocumentHit(CreateScoredHit(
                  /*document_id=*/6))));
}

TEST_F(ResultStateManagerTest,
       InvalidatedAllResultStatesShouldResetCurrentHitCount) {
  ResultState result_state1 =
      CreateResultState({AddScoredDocument(/*document_id=*/0),
                         AddScoredDocument(/*document_id=*/1)},
                        /*num_per_page=*/1);
  ResultState result_state2 =
      CreateResultState({AddScoredDocument(/*document_id=*/2),
                         AddScoredDocument(/*document_id=*/3)},
                        /*num_per_page=*/1);
  ResultState result_state3 =
      CreateResultState({AddScoredDocument(/*document_id=*/4),
                         AddScoredDocument(/*document_id=*/5)},
                        /*num_per_page=*/1);

  // Add the first three states. Remember, the first page for each result state
  // won't be cached (since it is returned immediately from RankAndPaginate).
  // Each result state has a page size of 1 and a result set of 2 hits. So each
  // result will take up one hit of our three hit budget.
  ResultStateManager result_state_manager(/*max_total_hits=*/3,
                                          document_store());
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state1,
      result_state_manager.RankAndPaginate(std::move(result_state1)));
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state2,
      result_state_manager.RankAndPaginate(std::move(result_state2)));
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state3,
      result_state_manager.RankAndPaginate(std::move(result_state3)));

  // Invalidates all states so that the current hit count will be 0.
  result_state_manager.InvalidateAllResultStates();

  // If invalidating all states correctly reset the current hit count to 0,
  // then the entirety of state 4 should still be within our budget and no other
  // result states should be evicted.
  ResultState result_state4 =
      CreateResultState({AddScoredDocument(/*document_id=*/6),
                         AddScoredDocument(/*document_id=*/7)},
                        /*num_per_page=*/1);
  ResultState result_state5 =
      CreateResultState({AddScoredDocument(/*document_id=*/8),
                         AddScoredDocument(/*document_id=*/9)},
                        /*num_per_page=*/1);
  ResultState result_state6 =
      CreateResultState({AddScoredDocument(/*document_id=*/10),
                         AddScoredDocument(/*document_id=*/11)},
                        /*num_per_page=*/1);
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state4,
      result_state_manager.RankAndPaginate(std::move(result_state4)));
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state5,
      result_state_manager.RankAndPaginate(std::move(result_state5)));
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state6,
      result_state_manager.RankAndPaginate(std::move(result_state6)));

  EXPECT_THAT(
      result_state_manager.GetNextPage(page_result_state1.next_page_token),
      StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));

  EXPECT_THAT(
      result_state_manager.GetNextPage(page_result_state2.next_page_token),
      StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));

  EXPECT_THAT(
      result_state_manager.GetNextPage(page_result_state3.next_page_token),
      StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));

  ICING_ASSERT_OK_AND_ASSIGN(
      page_result_state4,
      result_state_manager.GetNextPage(page_result_state4.next_page_token));
  EXPECT_THAT(page_result_state4.scored_document_hits,
              ElementsAre(EqualsScoredDocumentHit(CreateScoredHit(
                  /*document_id=*/6))));

  ICING_ASSERT_OK_AND_ASSIGN(
      page_result_state5,
      result_state_manager.GetNextPage(page_result_state5.next_page_token));
  EXPECT_THAT(page_result_state5.scored_document_hits,
              ElementsAre(EqualsScoredDocumentHit(CreateScoredHit(
                  /*document_id=*/8))));

  ICING_ASSERT_OK_AND_ASSIGN(
      page_result_state6,
      result_state_manager.GetNextPage(page_result_state6.next_page_token));
  EXPECT_THAT(page_result_state6.scored_document_hits,
              ElementsAre(EqualsScoredDocumentHit(CreateScoredHit(
                  /*document_id=*/10))));
}

TEST_F(
    ResultStateManagerTest,
    InvalidatedResultStateShouldDecreaseCurrentHitsCountByExactStateHitCount) {
  ResultState result_state1 =
      CreateResultState({AddScoredDocument(/*document_id=*/0),
                         AddScoredDocument(/*document_id=*/1)},
                        /*num_per_page=*/1);
  ResultState result_state2 =
      CreateResultState({AddScoredDocument(/*document_id=*/2),
                         AddScoredDocument(/*document_id=*/3)},
                        /*num_per_page=*/1);
  ResultState result_state3 =
      CreateResultState({AddScoredDocument(/*document_id=*/4),
                         AddScoredDocument(/*document_id=*/5)},
                        /*num_per_page=*/1);

  // Add the first three states. Remember, the first page for each result state
  // won't be cached (since it is returned immediately from RankAndPaginate).
  // Each result state has a page size of 1 and a result set of 2 hits. So each
  // result will take up one hit of our three hit budget.
  ResultStateManager result_state_manager(/*max_total_hits=*/3,
                                          document_store());
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state1,
      result_state_manager.RankAndPaginate(std::move(result_state1)));
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state2,
      result_state_manager.RankAndPaginate(std::move(result_state2)));
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state3,
      result_state_manager.RankAndPaginate(std::move(result_state3)));

  // Invalidates state 2, so that the number of hits current cached should be
  // decremented to 2.
  result_state_manager.InvalidateResultState(
      page_result_state2.next_page_token);

  // If invalidating state 2 correctly decremented the current hit count to 2,
  // then adding state 4 should still be within our budget and no other result
  // states should be evicted.
  ResultState result_state4 =
      CreateResultState({AddScoredDocument(/*document_id=*/6),
                         AddScoredDocument(/*document_id=*/7)},
                        /*num_per_page=*/1);
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state4,
      result_state_manager.RankAndPaginate(std::move(result_state4)));

  // If invalidating result state 2 correctly decremented the current hit count
  // to 2 and adding state 4 correctly incremented it to 3, then adding this
  // result state should trigger the eviction of state 1.
  ResultState result_state5 =
      CreateResultState({AddScoredDocument(/*document_id=*/8),
                         AddScoredDocument(/*document_id=*/9)},
                        /*num_per_page=*/1);
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state5,
      result_state_manager.RankAndPaginate(std::move(result_state5)));

  EXPECT_THAT(
      result_state_manager.GetNextPage(page_result_state1.next_page_token),
      StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));

  EXPECT_THAT(
      result_state_manager.GetNextPage(page_result_state2.next_page_token),
      StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));

  ICING_ASSERT_OK_AND_ASSIGN(
      page_result_state3,
      result_state_manager.GetNextPage(page_result_state3.next_page_token));
  EXPECT_THAT(page_result_state3.scored_document_hits,
              ElementsAre(EqualsScoredDocumentHit(CreateScoredHit(
                  /*document_id=*/4))));

  ICING_ASSERT_OK_AND_ASSIGN(
      page_result_state4,
      result_state_manager.GetNextPage(page_result_state4.next_page_token));
  EXPECT_THAT(page_result_state4.scored_document_hits,
              ElementsAre(EqualsScoredDocumentHit(CreateScoredHit(
                  /*document_id=*/6))));

  ICING_ASSERT_OK_AND_ASSIGN(
      page_result_state5,
      result_state_manager.GetNextPage(page_result_state5.next_page_token));
  EXPECT_THAT(page_result_state5.scored_document_hits,
              ElementsAre(EqualsScoredDocumentHit(CreateScoredHit(
                  /*document_id=*/8))));
}

TEST_F(ResultStateManagerTest, GetNextPageShouldDecreaseCurrentHitsCount) {
  ResultState result_state1 =
      CreateResultState({AddScoredDocument(/*document_id=*/0),
                         AddScoredDocument(/*document_id=*/1)},
                        /*num_per_page=*/1);
  ResultState result_state2 =
      CreateResultState({AddScoredDocument(/*document_id=*/2),
                         AddScoredDocument(/*document_id=*/3)},
                        /*num_per_page=*/1);
  ResultState result_state3 =
      CreateResultState({AddScoredDocument(/*document_id=*/4),
                         AddScoredDocument(/*document_id=*/5)},
                        /*num_per_page=*/1);

  // Add the first three states. Remember, the first page for each result state
  // won't be cached (since it is returned immediately from RankAndPaginate).
  // Each result state has a page size of 1 and a result set of 2 hits. So each
  // result will take up one hit of our three hit budget.
  ResultStateManager result_state_manager(/*max_total_hits=*/3,
                                          document_store());
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state1,
      result_state_manager.RankAndPaginate(std::move(result_state1)));
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state2,
      result_state_manager.RankAndPaginate(std::move(result_state2)));
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state3,
      result_state_manager.RankAndPaginate(std::move(result_state3)));

  // GetNextPage for result state 1 should return its result and decrement the
  // number of cached hits to 2.
  ICING_ASSERT_OK_AND_ASSIGN(
      page_result_state1,
      result_state_manager.GetNextPage(page_result_state1.next_page_token));
  EXPECT_THAT(page_result_state1.scored_document_hits,
              ElementsAre(EqualsScoredDocumentHit(CreateScoredHit(
                  /*document_id=*/0))));

  // If retrieving the next page for result state 1 correctly decremented the
  // current hit count to 2, then adding state 4 should still be within our
  // budget and no other result states should be evicted.
  ResultState result_state4 =
      CreateResultState({AddScoredDocument(/*document_id=*/6),
                         AddScoredDocument(/*document_id=*/7)},
                        /*num_per_page=*/1);
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state4,
      result_state_manager.RankAndPaginate(std::move(result_state4)));

  EXPECT_THAT(
      result_state_manager.GetNextPage(page_result_state1.next_page_token),
      StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));

  ICING_ASSERT_OK_AND_ASSIGN(
      page_result_state2,
      result_state_manager.GetNextPage(page_result_state2.next_page_token));
  EXPECT_THAT(page_result_state2.scored_document_hits,
              ElementsAre(EqualsScoredDocumentHit(CreateScoredHit(
                  /*document_id=*/2))));

  ICING_ASSERT_OK_AND_ASSIGN(
      page_result_state3,
      result_state_manager.GetNextPage(page_result_state3.next_page_token));
  EXPECT_THAT(page_result_state3.scored_document_hits,
              ElementsAre(EqualsScoredDocumentHit(CreateScoredHit(
                  /*document_id=*/4))));

  ICING_ASSERT_OK_AND_ASSIGN(
      page_result_state4,
      result_state_manager.GetNextPage(page_result_state4.next_page_token));
  EXPECT_THAT(page_result_state4.scored_document_hits,
              ElementsAre(EqualsScoredDocumentHit(CreateScoredHit(
                  /*document_id=*/6))));
}

TEST_F(ResultStateManagerTest,
       GetNextPageShouldDecreaseCurrentHitsCountByExactlyOnePage) {
  ResultState result_state1 =
      CreateResultState({AddScoredDocument(/*document_id=*/0),
                         AddScoredDocument(/*document_id=*/1)},
                        /*num_per_page=*/1);
  ResultState result_state2 =
      CreateResultState({AddScoredDocument(/*document_id=*/2),
                         AddScoredDocument(/*document_id=*/3)},
                        /*num_per_page=*/1);
  ResultState result_state3 =
      CreateResultState({AddScoredDocument(/*document_id=*/4),
                         AddScoredDocument(/*document_id=*/5)},
                        /*num_per_page=*/1);

  // Add the first three states. Remember, the first page for each result state
  // won't be cached (since it is returned immediately from RankAndPaginate).
  // Each result state has a page size of 1 and a result set of 2 hits. So each
  // result will take up one hit of our three hit budget.
  ResultStateManager result_state_manager(/*max_total_hits=*/3,
                                          document_store());
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state1,
      result_state_manager.RankAndPaginate(std::move(result_state1)));
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state2,
      result_state_manager.RankAndPaginate(std::move(result_state2)));
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state3,
      result_state_manager.RankAndPaginate(std::move(result_state3)));

  // GetNextPage for result state 1 should return its result and decrement the
  // number of cached hits to 2.
  ICING_ASSERT_OK_AND_ASSIGN(
      page_result_state1,
      result_state_manager.GetNextPage(page_result_state1.next_page_token));
  EXPECT_THAT(page_result_state1.scored_document_hits,
              ElementsAre(EqualsScoredDocumentHit(CreateScoredHit(
                  /*document_id=*/0))));

  // If retrieving the next page for result state 1 correctly decremented the
  // current hit count to 2, then adding state 4 should still be within our
  // budget and no other result states should be evicted.
  ResultState result_state4 =
      CreateResultState({AddScoredDocument(/*document_id=*/6),
                         AddScoredDocument(/*document_id=*/7)},
                        /*num_per_page=*/1);
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state4,
      result_state_manager.RankAndPaginate(std::move(result_state4)));

  // If retrieving the next page for result state 1 correctly decremented the
  // current hit count to 2 and adding state 4 correctly incremented it to 3,
  // then adding this result state should trigger the eviction of state 2.
  ResultState result_state5 =
      CreateResultState({AddScoredDocument(/*document_id=*/8),
                         AddScoredDocument(/*document_id=*/9)},
                        /*num_per_page=*/1);
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state5,
      result_state_manager.RankAndPaginate(std::move(result_state5)));

  EXPECT_THAT(
      result_state_manager.GetNextPage(page_result_state1.next_page_token),
      StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));

  EXPECT_THAT(
      result_state_manager.GetNextPage(page_result_state2.next_page_token),
      StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));

  ICING_ASSERT_OK_AND_ASSIGN(
      page_result_state3,
      result_state_manager.GetNextPage(page_result_state3.next_page_token));
  EXPECT_THAT(page_result_state3.scored_document_hits,
              ElementsAre(EqualsScoredDocumentHit(CreateScoredHit(
                  /*document_id=*/4))));

  ICING_ASSERT_OK_AND_ASSIGN(
      page_result_state4,
      result_state_manager.GetNextPage(page_result_state4.next_page_token));
  EXPECT_THAT(page_result_state4.scored_document_hits,
              ElementsAre(EqualsScoredDocumentHit(CreateScoredHit(
                  /*document_id=*/6))));

  ICING_ASSERT_OK_AND_ASSIGN(
      page_result_state5,
      result_state_manager.GetNextPage(page_result_state5.next_page_token));
  EXPECT_THAT(page_result_state5.scored_document_hits,
              ElementsAre(EqualsScoredDocumentHit(CreateScoredHit(
                  /*document_id=*/8))));
}

TEST_F(ResultStateManagerTest,
       AddingOverBudgetResultStateShouldEvictAllStates) {
  ResultState result_state1 =
      CreateResultState({AddScoredDocument(/*document_id=*/0),
                         AddScoredDocument(/*document_id=*/1),
                         AddScoredDocument(/*document_id=*/2)},
                        /*num_per_page=*/1);
  ResultState result_state2 =
      CreateResultState({AddScoredDocument(/*document_id=*/3),
                         AddScoredDocument(/*document_id=*/4)},
                        /*num_per_page=*/1);

  // Add the first two states. Remember, the first page for each result state
  // won't be cached (since it is returned immediately from RankAndPaginate).
  // Each result state has a page size of 1. So 3 hits will remain cached.
  ResultStateManager result_state_manager(/*max_total_hits=*/4,
                                          document_store());
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state1,
      result_state_manager.RankAndPaginate(std::move(result_state1)));
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state2,
      result_state_manager.RankAndPaginate(std::move(result_state2)));

  // Add a result state that is larger than the entire budget. This should
  // result in all previous result states being evicted, the first hit from
  // result state 3 being returned and the next four hits being cached (the last
  // hit should be dropped because it exceeds the max).
  ResultState result_state3 =
      CreateResultState({AddScoredDocument(/*document_id=*/5),
                         AddScoredDocument(/*document_id=*/6),
                         AddScoredDocument(/*document_id=*/7),
                         AddScoredDocument(/*document_id=*/8),
                         AddScoredDocument(/*document_id=*/9),
                         AddScoredDocument(/*document_id=*/10)},
                        /*num_per_page=*/1);
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state3,
      result_state_manager.RankAndPaginate(std::move(result_state3)));

  // GetNextPage for result state 1 and 2 should return NOT_FOUND.
  EXPECT_THAT(
      result_state_manager.GetNextPage(page_result_state1.next_page_token),
      StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));

  EXPECT_THAT(
      result_state_manager.GetNextPage(page_result_state2.next_page_token),
      StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));

  // Only the next four results in state 3 should be retrievable.
  ICING_ASSERT_OK_AND_ASSIGN(
      page_result_state3,
      result_state_manager.GetNextPage(page_result_state3.next_page_token));
  EXPECT_THAT(page_result_state3.scored_document_hits,
              ElementsAre(EqualsScoredDocumentHit(CreateScoredHit(
                  /*document_id=*/9))));

  ICING_ASSERT_OK_AND_ASSIGN(
      page_result_state3,
      result_state_manager.GetNextPage(page_result_state3.next_page_token));
  EXPECT_THAT(page_result_state3.scored_document_hits,
              ElementsAre(EqualsScoredDocumentHit(CreateScoredHit(
                  /*document_id=*/8))));

  ICING_ASSERT_OK_AND_ASSIGN(
      page_result_state3,
      result_state_manager.GetNextPage(page_result_state3.next_page_token));
  EXPECT_THAT(page_result_state3.scored_document_hits,
              ElementsAre(EqualsScoredDocumentHit(CreateScoredHit(
                  /*document_id=*/7))));

  ICING_ASSERT_OK_AND_ASSIGN(
      page_result_state3,
      result_state_manager.GetNextPage(page_result_state3.next_page_token));
  EXPECT_THAT(page_result_state3.scored_document_hits,
              ElementsAre(EqualsScoredDocumentHit(CreateScoredHit(
                  /*document_id=*/6))));

  // The final result should have been dropped because it exceeded the budget.
  EXPECT_THAT(
      result_state_manager.GetNextPage(page_result_state3.next_page_token),
      StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
}

TEST_F(ResultStateManagerTest,
       AddingResultStateShouldEvictOverBudgetResultState) {
  ResultStateManager result_state_manager(/*max_total_hits=*/4,
                                          document_store());
  // Add a result state that is larger than the entire budget. The entire result
  // state will still be cached
  ResultState result_state1 =
      CreateResultState({AddScoredDocument(/*document_id=*/0),
                         AddScoredDocument(/*document_id=*/1),
                         AddScoredDocument(/*document_id=*/2),
                         AddScoredDocument(/*document_id=*/3),
                         AddScoredDocument(/*document_id=*/4),
                         AddScoredDocument(/*document_id=*/5)},
                        /*num_per_page=*/1);
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state1,
      result_state_manager.RankAndPaginate(std::move(result_state1)));

  // Add a result state. Because state2 + state1 is larger than the budget,
  // state1 should be evicted.
  ResultState result_state2 =
      CreateResultState({AddScoredDocument(/*document_id=*/6),
                         AddScoredDocument(/*document_id=*/7)},
                        /*num_per_page=*/1);
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state2,
      result_state_manager.RankAndPaginate(std::move(result_state2)));

  // state1 should have been evicted and state2 should still be retrievable.
  EXPECT_THAT(
      result_state_manager.GetNextPage(page_result_state1.next_page_token),
      StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));

  ICING_ASSERT_OK_AND_ASSIGN(
      page_result_state2,
      result_state_manager.GetNextPage(page_result_state2.next_page_token));
  EXPECT_THAT(page_result_state2.scored_document_hits,
              ElementsAre(EqualsScoredDocumentHit(CreateScoredHit(
                  /*document_id=*/6))));
}

TEST_F(ResultStateManagerTest, ShouldGetSnippetContext) {
  ResultSpecProto result_spec = CreateResultSpec(/*num_per_page=*/1);
  result_spec.mutable_snippet_spec()->set_num_to_snippet(5);
  result_spec.mutable_snippet_spec()->set_num_matches_per_property(5);
  result_spec.mutable_snippet_spec()->set_max_window_bytes(5);

  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);

  SectionRestrictQueryTermsMap query_terms_map;
  query_terms_map.emplace("term1", std::unordered_set<std::string>());

  ResultState original_result_state = ResultState(
      /*scored_document_hits=*/{AddScoredDocument(/*document_id=*/0),
                                AddScoredDocument(/*document_id=*/1)},
      query_terms_map, search_spec, CreateScoringSpec(), result_spec,
      document_store());

  ResultStateManager result_state_manager(
      /*max_total_hits=*/std::numeric_limits<int>::max(), document_store());
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state,
      result_state_manager.RankAndPaginate(std::move(original_result_state)));

  ASSERT_THAT(page_result_state.next_page_token, Gt(kInvalidNextPageToken));

  EXPECT_THAT(page_result_state.snippet_context.match_type,
              Eq(TermMatchType::EXACT_ONLY));
  EXPECT_TRUE(page_result_state.snippet_context.query_terms.find("term1") !=
              page_result_state.snippet_context.query_terms.end());
  EXPECT_THAT(page_result_state.snippet_context.snippet_spec,
              EqualsProto(result_spec.snippet_spec()));
}

TEST_F(ResultStateManagerTest, ShouldGetDefaultSnippetContext) {
  ResultSpecProto result_spec = CreateResultSpec(/*num_per_page=*/1);
  // 0 indicates no snippeting
  result_spec.mutable_snippet_spec()->set_num_to_snippet(0);
  result_spec.mutable_snippet_spec()->set_num_matches_per_property(0);
  result_spec.mutable_snippet_spec()->set_max_window_bytes(0);

  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);

  SectionRestrictQueryTermsMap query_terms_map;
  query_terms_map.emplace("term1", std::unordered_set<std::string>());

  ResultState original_result_state = ResultState(
      /*scored_document_hits=*/{AddScoredDocument(/*document_id=*/0),
                                AddScoredDocument(/*document_id=*/1)},
      query_terms_map, search_spec, CreateScoringSpec(), result_spec,
      document_store());

  ResultStateManager result_state_manager(
      /*max_total_hits=*/std::numeric_limits<int>::max(), document_store());
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state,
      result_state_manager.RankAndPaginate(std::move(original_result_state)));

  ASSERT_THAT(page_result_state.next_page_token, Gt(kInvalidNextPageToken));

  EXPECT_THAT(page_result_state.snippet_context.query_terms, IsEmpty());
  EXPECT_THAT(
      page_result_state.snippet_context.snippet_spec,
      EqualsProto(ResultSpecProto::SnippetSpecProto::default_instance()));
  EXPECT_THAT(page_result_state.snippet_context.match_type,
              Eq(TermMatchType::UNKNOWN));
}

TEST_F(ResultStateManagerTest, ShouldGetCorrectNumPreviouslyReturned) {
  ResultState original_result_state =
      CreateResultState({AddScoredDocument(/*document_id=*/0),
                         AddScoredDocument(/*document_id=*/1),
                         AddScoredDocument(/*document_id=*/2),
                         AddScoredDocument(/*document_id=*/3),
                         AddScoredDocument(/*document_id=*/4)},
                        /*num_per_page=*/2);

  ResultStateManager result_state_manager(
      /*max_total_hits=*/std::numeric_limits<int>::max(), document_store());

  // First page, 2 results
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state1,
      result_state_manager.RankAndPaginate(std::move(original_result_state)));
  ASSERT_THAT(page_result_state1.scored_document_hits.size(), Eq(2));

  // No previously returned results
  EXPECT_THAT(page_result_state1.num_previously_returned, Eq(0));

  uint64_t next_page_token = page_result_state1.next_page_token;

  // Second page, 2 results
  ICING_ASSERT_OK_AND_ASSIGN(PageResultState page_result_state2,
                             result_state_manager.GetNextPage(next_page_token));
  ASSERT_THAT(page_result_state2.scored_document_hits.size(), Eq(2));

  // num_previously_returned = size of first page
  EXPECT_THAT(page_result_state2.num_previously_returned, Eq(2));

  // Third page, 1 result
  ICING_ASSERT_OK_AND_ASSIGN(PageResultState page_result_state3,
                             result_state_manager.GetNextPage(next_page_token));
  ASSERT_THAT(page_result_state3.scored_document_hits.size(), Eq(1));

  // num_previously_returned = size of first and second pages
  EXPECT_THAT(page_result_state3.num_previously_returned, Eq(4));

  // No more results
  EXPECT_THAT(result_state_manager.GetNextPage(next_page_token),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
}

TEST_F(ResultStateManagerTest, ShouldStoreAllHits) {
  ScoredDocumentHit scored_hit_1 = AddScoredDocument(/*document_id=*/0);
  ScoredDocumentHit scored_hit_2 = AddScoredDocument(/*document_id=*/1);
  ScoredDocumentHit scored_hit_3 = AddScoredDocument(/*document_id=*/2);
  ScoredDocumentHit scored_hit_4 = AddScoredDocument(/*document_id=*/3);
  ScoredDocumentHit scored_hit_5 = AddScoredDocument(/*document_id=*/4);

  ResultState original_result_state = CreateResultState(
      {scored_hit_1, scored_hit_2, scored_hit_3, scored_hit_4, scored_hit_5},
      /*num_per_page=*/2);

  ResultStateManager result_state_manager(/*max_total_hits=*/4,
                                          document_store());

  // The 5 input scored document hits will not be truncated. The first page of
  // two hits will be returned immediately and the other three hits will fit
  // within our caching budget.

  // First page, 2 results
  ICING_ASSERT_OK_AND_ASSIGN(
      PageResultState page_result_state1,
      result_state_manager.RankAndPaginate(std::move(original_result_state)));
  EXPECT_THAT(page_result_state1.scored_document_hits,
              ElementsAre(EqualsScoredDocumentHit(scored_hit_5),
                          EqualsScoredDocumentHit(scored_hit_4)));

  uint64_t next_page_token = page_result_state1.next_page_token;

  // Second page, 2 results.
  ICING_ASSERT_OK_AND_ASSIGN(PageResultState page_result_state2,
                             result_state_manager.GetNextPage(next_page_token));
  EXPECT_THAT(page_result_state2.scored_document_hits,
              ElementsAre(EqualsScoredDocumentHit(scored_hit_3),
                          EqualsScoredDocumentHit(scored_hit_2)));

  // Third page, 1 result.
  ICING_ASSERT_OK_AND_ASSIGN(PageResultState page_result_state3,
                             result_state_manager.GetNextPage(next_page_token));
  EXPECT_THAT(page_result_state3.scored_document_hits,
              ElementsAre(EqualsScoredDocumentHit(scored_hit_1)));

  // Fourth page, 0 results.
  EXPECT_THAT(result_state_manager.GetNextPage(next_page_token),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
}

}  // namespace
}  // namespace lib
}  // namespace icing
