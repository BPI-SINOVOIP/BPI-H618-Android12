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

#include "icing/result/result-state.h"

#include "gtest/gtest.h"
#include "icing/document-builder.h"
#include "icing/file/filesystem.h"
#include "icing/portable/equals-proto.h"
#include "icing/schema/schema-store.h"
#include "icing/scoring/scored-document-hit.h"
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
using ::testing::IsEmpty;

ScoredDocumentHit CreateScoredDocumentHit(DocumentId document_id) {
  return ScoredDocumentHit(document_id, kSectionIdMaskNone, /*score=*/1);
}

SearchSpecProto CreateSearchSpec(TermMatchType::Code match_type) {
  SearchSpecProto search_spec;
  search_spec.set_term_match_type(match_type);
  return search_spec;
}

ScoringSpecProto CreateScoringSpec(bool is_descending_order) {
  ScoringSpecProto scoring_spec;
  scoring_spec.set_order_by(is_descending_order ? ScoringSpecProto::Order::DESC
                                                : ScoringSpecProto::Order::ASC);
  return scoring_spec;
}

ResultSpecProto CreateResultSpec(int num_per_page) {
  ResultSpecProto result_spec;
  result_spec.set_num_per_page(num_per_page);
  return result_spec;
}

class ResultStateTest : public testing::Test {
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

  ScoredDocumentHit AddScoredDocument(DocumentId document_id) {
    DocumentProto document;
    document.set_namespace_("namespace");
    document.set_uri(std::to_string(document_id));
    document.set_schema("Document");
    document_store_->Put(std::move(document));
    return ScoredDocumentHit(document_id, kSectionIdMaskNone, /*score=*/1);
  }

  DocumentStore& document_store() { return *document_store_; }

 private:
  Filesystem filesystem_;
  std::string doc_store_base_dir_;
  std::string schema_store_base_dir_;
  Clock clock_;
  std::unique_ptr<DocumentStore> document_store_;
  std::unique_ptr<SchemaStore> schema_store_;
};

// ResultState::ResultState() and ResultState::GetNextPage() are calling
// Ranker::BuildHeapInPlace() and Ranker::PopTopResultsFromHeap() directly, so
// we don't need to test much on what order is returned as that is tested in
// Ranker's tests. Here we just need one sanity test to make sure that the
// correct functions are called.
TEST_F(ResultStateTest, ShouldReturnNextPage) {
  ScoredDocumentHit scored_hit_0 = AddScoredDocument(/*document_id=*/0);
  ScoredDocumentHit scored_hit_1 = AddScoredDocument(/*document_id=*/1);
  ScoredDocumentHit scored_hit_2 = AddScoredDocument(/*document_id=*/2);
  ScoredDocumentHit scored_hit_3 = AddScoredDocument(/*document_id=*/3);
  ScoredDocumentHit scored_hit_4 = AddScoredDocument(/*document_id=*/4);
  std::vector<ScoredDocumentHit> scored_document_hits = {
      scored_hit_1, scored_hit_0, scored_hit_2, scored_hit_4, scored_hit_3};

  ResultState result_state(scored_document_hits, /*query_terms=*/{},
                           CreateSearchSpec(TermMatchType::EXACT_ONLY),
                           CreateScoringSpec(/*is_descending_order=*/true),
                           CreateResultSpec(/*num_per_page=*/2),
                           document_store());

  EXPECT_THAT(
      result_state.GetNextPage(document_store()),
      ElementsAre(
          EqualsScoredDocumentHit(CreateScoredDocumentHit(/*document_id=*/4)),
          EqualsScoredDocumentHit(CreateScoredDocumentHit(/*document_id=*/3))));

  EXPECT_THAT(
      result_state.GetNextPage(document_store()),
      ElementsAre(
          EqualsScoredDocumentHit(CreateScoredDocumentHit(/*document_id=*/2)),
          EqualsScoredDocumentHit(CreateScoredDocumentHit(/*document_id=*/1))));

  EXPECT_THAT(result_state.GetNextPage(document_store()),
              ElementsAre(EqualsScoredDocumentHit(
                  CreateScoredDocumentHit(/*document_id=*/0))));
}

TEST_F(ResultStateTest, ShouldReturnSnippetContextAccordingToSpecs) {
  ResultSpecProto result_spec = CreateResultSpec(/*num_per_page=*/2);
  result_spec.mutable_snippet_spec()->set_num_to_snippet(5);
  result_spec.mutable_snippet_spec()->set_num_matches_per_property(5);
  result_spec.mutable_snippet_spec()->set_max_window_bytes(5);

  SectionRestrictQueryTermsMap query_terms_map;
  query_terms_map.emplace("term1", std::unordered_set<std::string>());

  ResultState result_state(
      /*scored_document_hits=*/{}, query_terms_map,
      CreateSearchSpec(TermMatchType::EXACT_ONLY),
      CreateScoringSpec(/*is_descending_order=*/true), result_spec,
      document_store());

  const SnippetContext& snippet_context = result_state.snippet_context();

  // Snippet context should be derived from the specs above.
  EXPECT_TRUE(snippet_context.query_terms.find("term1") !=
              snippet_context.query_terms.end());
  EXPECT_THAT(snippet_context.snippet_spec,
              EqualsProto(result_spec.snippet_spec()));
  EXPECT_THAT(snippet_context.match_type, Eq(TermMatchType::EXACT_ONLY));

  // The same copy can be fetched multiple times.
  const SnippetContext& snippet_context2 = result_state.snippet_context();
  EXPECT_TRUE(snippet_context2.query_terms.find("term1") !=
              snippet_context2.query_terms.end());
  EXPECT_THAT(snippet_context2.snippet_spec,
              EqualsProto(result_spec.snippet_spec()));
  EXPECT_THAT(snippet_context2.match_type, Eq(TermMatchType::EXACT_ONLY));
}

TEST_F(ResultStateTest, NoSnippetingShouldReturnNull) {
  ResultSpecProto result_spec = CreateResultSpec(/*num_per_page=*/2);
  // Setting num_to_snippet to 0 so that snippeting info won't be
  // stored.
  result_spec.mutable_snippet_spec()->set_num_to_snippet(0);
  result_spec.mutable_snippet_spec()->set_num_matches_per_property(5);
  result_spec.mutable_snippet_spec()->set_max_window_bytes(5);

  SectionRestrictQueryTermsMap query_terms_map;
  query_terms_map.emplace("term1", std::unordered_set<std::string>());

  ResultState result_state(/*scored_document_hits=*/{}, query_terms_map,
                           CreateSearchSpec(TermMatchType::EXACT_ONLY),
                           CreateScoringSpec(/*is_descending_order=*/true),
                           result_spec, document_store());

  const SnippetContext& snippet_context = result_state.snippet_context();
  EXPECT_THAT(snippet_context.query_terms, IsEmpty());
  EXPECT_THAT(
      snippet_context.snippet_spec,
      EqualsProto(ResultSpecProto::SnippetSpecProto::default_instance()));
  EXPECT_THAT(snippet_context.match_type, TermMatchType::UNKNOWN);
}

TEST_F(ResultStateTest, ShouldTruncateToNewSize) {
  ScoredDocumentHit scored_hit_0 = AddScoredDocument(/*document_id=*/0);
  ScoredDocumentHit scored_hit_1 = AddScoredDocument(/*document_id=*/1);
  ScoredDocumentHit scored_hit_2 = AddScoredDocument(/*document_id=*/2);
  ScoredDocumentHit scored_hit_3 = AddScoredDocument(/*document_id=*/3);
  ScoredDocumentHit scored_hit_4 = AddScoredDocument(/*document_id=*/4);
  std::vector<ScoredDocumentHit> scored_document_hits = {
      scored_hit_1, scored_hit_0, scored_hit_2, scored_hit_4, scored_hit_3};

  // Creates a ResultState with 5 ScoredDocumentHits.
  ResultState result_state(scored_document_hits, /*query_terms=*/{},
                           CreateSearchSpec(TermMatchType::EXACT_ONLY),
                           CreateScoringSpec(/*is_descending_order=*/true),
                           CreateResultSpec(/*num_per_page=*/5),
                           document_store());

  result_state.TruncateHitsTo(/*new_size=*/3);
  // The best 3 are left.
  EXPECT_THAT(
      result_state.GetNextPage(document_store()),
      ElementsAre(
          EqualsScoredDocumentHit(CreateScoredDocumentHit(/*document_id=*/4)),
          EqualsScoredDocumentHit(CreateScoredDocumentHit(/*document_id=*/3)),
          EqualsScoredDocumentHit(CreateScoredDocumentHit(/*document_id=*/2))));
}

TEST_F(ResultStateTest, ShouldTruncateToZero) {
  ScoredDocumentHit scored_hit_0 = AddScoredDocument(/*document_id=*/0);
  ScoredDocumentHit scored_hit_1 = AddScoredDocument(/*document_id=*/1);
  ScoredDocumentHit scored_hit_2 = AddScoredDocument(/*document_id=*/2);
  ScoredDocumentHit scored_hit_3 = AddScoredDocument(/*document_id=*/3);
  ScoredDocumentHit scored_hit_4 = AddScoredDocument(/*document_id=*/4);
  std::vector<ScoredDocumentHit> scored_document_hits = {
      scored_hit_1, scored_hit_0, scored_hit_2, scored_hit_4, scored_hit_3};

  // Creates a ResultState with 5 ScoredDocumentHits.
  ResultState result_state(scored_document_hits, /*query_terms=*/{},
                           CreateSearchSpec(TermMatchType::EXACT_ONLY),
                           CreateScoringSpec(/*is_descending_order=*/true),
                           CreateResultSpec(/*num_per_page=*/5),
                           document_store());

  result_state.TruncateHitsTo(/*new_size=*/0);
  EXPECT_THAT(result_state.GetNextPage(document_store()), IsEmpty());
}

TEST_F(ResultStateTest, ShouldNotTruncateToNegative) {
  ScoredDocumentHit scored_hit_0 = AddScoredDocument(/*document_id=*/0);
  ScoredDocumentHit scored_hit_1 = AddScoredDocument(/*document_id=*/1);
  ScoredDocumentHit scored_hit_2 = AddScoredDocument(/*document_id=*/2);
  ScoredDocumentHit scored_hit_3 = AddScoredDocument(/*document_id=*/3);
  ScoredDocumentHit scored_hit_4 = AddScoredDocument(/*document_id=*/4);
  std::vector<ScoredDocumentHit> scored_document_hits = {
      scored_hit_1, scored_hit_0, scored_hit_2, scored_hit_4, scored_hit_3};

  // Creates a ResultState with 5 ScoredDocumentHits.
  ResultState result_state(scored_document_hits, /*query_terms=*/{},
                           CreateSearchSpec(TermMatchType::EXACT_ONLY),
                           CreateScoringSpec(/*is_descending_order=*/true),
                           CreateResultSpec(/*num_per_page=*/5),
                           document_store());

  result_state.TruncateHitsTo(/*new_size=*/-1);
  // Results are not affected.
  EXPECT_THAT(
      result_state.GetNextPage(document_store()),
      ElementsAre(
          EqualsScoredDocumentHit(CreateScoredDocumentHit(/*document_id=*/4)),
          EqualsScoredDocumentHit(CreateScoredDocumentHit(/*document_id=*/3)),
          EqualsScoredDocumentHit(CreateScoredDocumentHit(/*document_id=*/2)),
          EqualsScoredDocumentHit(CreateScoredDocumentHit(/*document_id=*/1)),
          EqualsScoredDocumentHit(CreateScoredDocumentHit(/*document_id=*/0))));
}

TEST_F(ResultStateTest, ResultGroupingShouldLimitResults) {
  // Creates 2 documents and ensures the relationship in terms of document
  // score is: document1 < document2
  DocumentProto document1 = DocumentBuilder()
                                .SetKey("namespace", "uri/1")
                                .SetSchema("Document")
                                .SetScore(1)
                                .Build();
  DocumentProto document2 = DocumentBuilder()
                                .SetKey("namespace", "uri/2")
                                .SetSchema("Document")
                                .SetScore(2)
                                .Build();

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             document_store().Put(document1));
  ScoredDocumentHit scored_hit_1(document_id1, kSectionIdMaskNone,
                                 document1.score());
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             document_store().Put(document2));
  ScoredDocumentHit scored_hit_2(document_id2, kSectionIdMaskNone,
                                 document2.score());
  std::vector<ScoredDocumentHit> scored_document_hits = {scored_hit_2,
                                                         scored_hit_1};

  // Create a ResultSpec that limits "namespace" to a single result.
  ResultSpecProto result_spec;
  result_spec.set_num_per_page(5);
  ResultSpecProto::ResultGrouping* result_grouping =
      result_spec.add_result_groupings();
  result_grouping->set_max_results(1);
  result_grouping->add_namespaces("namespace");

  // Creates a ResultState with 2 ScoredDocumentHits.
  ResultState result_state(scored_document_hits, /*query_terms=*/{},
                           CreateSearchSpec(TermMatchType::EXACT_ONLY),
                           CreateScoringSpec(/*is_descending_order=*/true),
                           result_spec, document_store());

  // Only the top ranked document in "namespace" (document2), should be
  // returned.
  EXPECT_THAT(result_state.GetNextPage(document_store()),
              ElementsAre(EqualsScoredDocumentHit(scored_hit_2)));
}

TEST_F(ResultStateTest, ResultGroupingDoesNotLimitOtherNamespaceResults) {
  // Creates 4 documents and ensures the relationship in terms of document
  // score is: document1 < document2 < document3 < document4
  DocumentProto document1 = DocumentBuilder()
                                .SetKey("namespace1", "uri/1")
                                .SetSchema("Document")
                                .SetScore(1)
                                .Build();
  DocumentProto document2 = DocumentBuilder()
                                .SetKey("namespace1", "uri/2")
                                .SetSchema("Document")
                                .SetScore(2)
                                .Build();
  DocumentProto document3 = DocumentBuilder()
                                .SetKey("namespace2", "uri/3")
                                .SetSchema("Document")
                                .SetScore(3)
                                .Build();
  DocumentProto document4 = DocumentBuilder()
                                .SetKey("namespace2", "uri/4")
                                .SetSchema("Document")
                                .SetScore(4)
                                .Build();

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             document_store().Put(document1));
  ScoredDocumentHit scored_hit_1(document_id1, kSectionIdMaskNone,
                                 document1.score());
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             document_store().Put(document2));
  ScoredDocumentHit scored_hit_2(document_id2, kSectionIdMaskNone,
                                 document2.score());
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id3,
                             document_store().Put(document3));
  ScoredDocumentHit scored_hit_3(document_id3, kSectionIdMaskNone,
                                 document3.score());
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id4,
                             document_store().Put(document4));
  ScoredDocumentHit scored_hit_4(document_id4, kSectionIdMaskNone,
                                 document4.score());
  std::vector<ScoredDocumentHit> scored_document_hits = {
      scored_hit_4, scored_hit_3, scored_hit_2, scored_hit_1};

  // Create a ResultSpec that limits "namespace1" to a single result, but
  // doesn't limit "namespace2".
  ResultSpecProto result_spec;
  result_spec.set_num_per_page(5);
  ResultSpecProto::ResultGrouping* result_grouping =
      result_spec.add_result_groupings();
  result_grouping->set_max_results(1);
  result_grouping->add_namespaces("namespace1");

  // Creates a ResultState with 4 ScoredDocumentHits.
  ResultState result_state(scored_document_hits, /*query_terms=*/{},
                           CreateSearchSpec(TermMatchType::EXACT_ONLY),
                           CreateScoringSpec(/*is_descending_order=*/true),
                           result_spec, document_store());

  // Only the top ranked document in "namespace" (document2), should be
  // returned.
  EXPECT_THAT(result_state.GetNextPage(document_store()),
              ElementsAre(EqualsScoredDocumentHit(scored_hit_4),
                          EqualsScoredDocumentHit(scored_hit_3),
                          EqualsScoredDocumentHit(scored_hit_2)));
}

TEST_F(ResultStateTest, ResultGroupingNonexistentNamespaceShouldBeIgnored) {
  // Creates 2 documents and ensures the relationship in terms of document
  // score is: document1 < document2
  DocumentProto document1 = DocumentBuilder()
                                .SetKey("namespace", "uri/1")
                                .SetSchema("Document")
                                .SetScore(1)
                                .Build();
  DocumentProto document2 = DocumentBuilder()
                                .SetKey("namespace", "uri/2")
                                .SetSchema("Document")
                                .SetScore(2)
                                .Build();

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             document_store().Put(document1));
  ScoredDocumentHit scored_hit_1(document_id1, kSectionIdMaskNone,
                                 document1.score());
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             document_store().Put(document2));
  ScoredDocumentHit scored_hit_2(document_id2, kSectionIdMaskNone,
                                 document2.score());
  std::vector<ScoredDocumentHit> scored_document_hits = {scored_hit_2,
                                                         scored_hit_1};

  // Create a ResultSpec that limits "namespace"+"nonExistentNamespace" to a
  // single result.
  ResultSpecProto result_spec;
  result_spec.set_num_per_page(5);
  ResultSpecProto::ResultGrouping* result_grouping =
      result_spec.add_result_groupings();
  result_grouping->set_max_results(1);
  result_grouping->add_namespaces("namespace");
  result_grouping->add_namespaces("nonexistentNamespace");

  // Creates a ResultState with 2 ScoredDocumentHits.
  ResultState result_state(scored_document_hits, /*query_terms=*/{},
                           CreateSearchSpec(TermMatchType::EXACT_ONLY),
                           CreateScoringSpec(/*is_descending_order=*/true),
                           result_spec, document_store());

  // Only the top ranked document in "namespace" (document2), should be
  // returned. The presence of "nonexistentNamespace" in the same result
  // grouping should have no effect.
  EXPECT_THAT(result_state.GetNextPage(document_store()),
              ElementsAre(EqualsScoredDocumentHit(scored_hit_2)));
}

TEST_F(ResultStateTest, ResultGroupingMultiNamespaceGrouping) {
  // Creates 6 documents and ensures the relationship in terms of document
  // score is: document1 < document2 < document3 < document4 < document5 <
  // document6
  DocumentProto document1 = DocumentBuilder()
                                .SetKey("namespace1", "uri/1")
                                .SetSchema("Document")
                                .SetScore(1)
                                .Build();
  DocumentProto document2 = DocumentBuilder()
                                .SetKey("namespace1", "uri/2")
                                .SetSchema("Document")
                                .SetScore(2)
                                .Build();
  DocumentProto document3 = DocumentBuilder()
                                .SetKey("namespace2", "uri/3")
                                .SetSchema("Document")
                                .SetScore(3)
                                .Build();
  DocumentProto document4 = DocumentBuilder()
                                .SetKey("namespace2", "uri/4")
                                .SetSchema("Document")
                                .SetScore(4)
                                .Build();
  DocumentProto document5 = DocumentBuilder()
                                .SetKey("namespace3", "uri/5")
                                .SetSchema("Document")
                                .SetScore(5)
                                .Build();
  DocumentProto document6 = DocumentBuilder()
                                .SetKey("namespace3", "uri/6")
                                .SetSchema("Document")
                                .SetScore(6)
                                .Build();

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             document_store().Put(document1));
  ScoredDocumentHit scored_hit_1(document_id1, kSectionIdMaskNone,
                                 document1.score());
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             document_store().Put(document2));
  ScoredDocumentHit scored_hit_2(document_id2, kSectionIdMaskNone,
                                 document2.score());
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id3,
                             document_store().Put(document3));
  ScoredDocumentHit scored_hit_3(document_id3, kSectionIdMaskNone,
                                 document3.score());
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id4,
                             document_store().Put(document4));
  ScoredDocumentHit scored_hit_4(document_id4, kSectionIdMaskNone,
                                 document4.score());
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id5,
                             document_store().Put(document5));
  ScoredDocumentHit scored_hit_5(document_id5, kSectionIdMaskNone,
                                 document5.score());
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id6,
                             document_store().Put(document6));
  ScoredDocumentHit scored_hit_6(document_id6, kSectionIdMaskNone,
                                 document6.score());
  std::vector<ScoredDocumentHit> scored_document_hits = {
      scored_hit_6, scored_hit_5, scored_hit_4,
      scored_hit_3, scored_hit_2, scored_hit_1};

  // Create a ResultSpec that limits "namespace1" to a single result and limits
  // "namespace2"+"namespace3" to a total of two results.
  ResultSpecProto result_spec;
  result_spec.set_num_per_page(5);
  ResultSpecProto::ResultGrouping* result_grouping =
      result_spec.add_result_groupings();
  result_grouping->set_max_results(1);
  result_grouping->add_namespaces("namespace1");
  result_grouping = result_spec.add_result_groupings();
  result_grouping->set_max_results(2);
  result_grouping->add_namespaces("namespace2");
  result_grouping->add_namespaces("namespace3");

  // Creates a ResultState with 4 ScoredDocumentHits.
  ResultState result_state(scored_document_hits, /*query_terms=*/{},
                           CreateSearchSpec(TermMatchType::EXACT_ONLY),
                           CreateScoringSpec(/*is_descending_order=*/true),
                           result_spec, document_store());

  // Only the top-ranked result in "namespace1" (document2) should be returned.
  // Only the top-ranked results across "namespace2" and "namespace3"
  // (document6, document5) should be returned.
  EXPECT_THAT(result_state.GetNextPage(document_store()),
              ElementsAre(EqualsScoredDocumentHit(scored_hit_6),
                          EqualsScoredDocumentHit(scored_hit_5),
                          EqualsScoredDocumentHit(scored_hit_2)));
}

TEST_F(ResultStateTest, ResultGroupingOnlyNonexistentNamespaces) {
  // Creates 2 documents and ensures the relationship in terms of document
  // score is: document1 < document2
  DocumentProto document1 = DocumentBuilder()
                                .SetKey("namespace", "uri/1")
                                .SetSchema("Document")
                                .SetScore(1)
                                .Build();
  DocumentProto document2 = DocumentBuilder()
                                .SetKey("namespace", "uri/2")
                                .SetSchema("Document")
                                .SetScore(2)
                                .Build();

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             document_store().Put(document1));
  ScoredDocumentHit scored_hit_1(document_id1, kSectionIdMaskNone,
                                 document1.score());
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             document_store().Put(document2));
  ScoredDocumentHit scored_hit_2(document_id2, kSectionIdMaskNone,
                                 document2.score());
  std::vector<ScoredDocumentHit> scored_document_hits = {scored_hit_2,
                                                         scored_hit_1};

  // Create a ResultSpec that limits "nonexistentNamespace" to a single result.
  // but doesn't limit "namespace"
  ResultSpecProto result_spec;
  result_spec.set_num_per_page(5);
  ResultSpecProto::ResultGrouping* result_grouping =
      result_spec.add_result_groupings();
  result_grouping->set_max_results(1);
  result_grouping->add_namespaces("nonexistentNamespace");

  // Creates a ResultState with 2 ScoredDocumentHits.
  ResultState result_state(scored_document_hits, /*query_terms=*/{},
                           CreateSearchSpec(TermMatchType::EXACT_ONLY),
                           CreateScoringSpec(/*is_descending_order=*/true),
                           result_spec, document_store());

  // All documents in "namespace" should be returned. The presence of
  // "nonexistentNamespace" should have no effect.
  EXPECT_THAT(result_state.GetNextPage(document_store()),
              ElementsAre(EqualsScoredDocumentHit(scored_hit_2),
                          EqualsScoredDocumentHit(scored_hit_1)));
}

}  // namespace
}  // namespace lib
}  // namespace icing
