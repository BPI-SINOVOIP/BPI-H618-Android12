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

#include "icing/scoring/scoring-processor.h"

#include <cstdint>

#include "icing/text_classifier/lib3/utils/base/statusor.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "icing/document-builder.h"
#include "icing/index/iterator/doc-hit-info-iterator-test-util.h"
#include "icing/proto/document.pb.h"
#include "icing/proto/schema.pb.h"
#include "icing/proto/scoring.pb.h"
#include "icing/schema-builder.h"
#include "icing/testing/common-matchers.h"
#include "icing/testing/fake-clock.h"
#include "icing/testing/tmp-directory.h"

namespace icing {
namespace lib {

namespace {
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::SizeIs;

constexpr PropertyConfigProto_DataType_Code TYPE_STRING =
    PropertyConfigProto_DataType_Code_STRING;

constexpr PropertyConfigProto_Cardinality_Code CARDINALITY_OPTIONAL =
    PropertyConfigProto_Cardinality_Code_OPTIONAL;

class ScoringProcessorTest : public testing::Test {
 protected:
  ScoringProcessorTest()
      : test_dir_(GetTestTempDir() + "/icing"),
        doc_store_dir_(test_dir_ + "/doc_store"),
        schema_store_dir_(test_dir_ + "/schema_store") {}

  void SetUp() override {
    // Creates file directories
    filesystem_.DeleteDirectoryRecursively(test_dir_.c_str());
    filesystem_.CreateDirectoryRecursively(doc_store_dir_.c_str());
    filesystem_.CreateDirectoryRecursively(schema_store_dir_.c_str());

    ICING_ASSERT_OK_AND_ASSIGN(
        schema_store_,
        SchemaStore::Create(&filesystem_, test_dir_, &fake_clock_));

    ICING_ASSERT_OK_AND_ASSIGN(
        DocumentStore::CreateResult create_result,
        DocumentStore::Create(&filesystem_, doc_store_dir_, &fake_clock_,
                              schema_store_.get()));
    document_store_ = std::move(create_result.document_store);

    // Creates a simple email schema
    SchemaProto test_email_schema =
        SchemaBuilder()
            .AddType(SchemaTypeConfigBuilder().SetType("email").AddProperty(
                PropertyConfigBuilder()
                    .SetName("subject")
                    .SetDataType(TYPE_STRING)
                    .SetCardinality(CARDINALITY_OPTIONAL)))
            .Build();
    ICING_ASSERT_OK(schema_store_->SetSchema(test_email_schema));
  }

  void TearDown() override {
    document_store_.reset();
    schema_store_.reset();
    filesystem_.DeleteDirectoryRecursively(test_dir_.c_str());
  }

  DocumentStore* document_store() { return document_store_.get(); }

 private:
  const std::string test_dir_;
  const std::string doc_store_dir_;
  const std::string schema_store_dir_;
  Filesystem filesystem_;
  FakeClock fake_clock_;
  std::unique_ptr<DocumentStore> document_store_;
  std::unique_ptr<SchemaStore> schema_store_;
};

constexpr int kDefaultScore = 0;
constexpr int64_t kDefaultCreationTimestampMs = 1571100001111;

DocumentProto CreateDocument(const std::string& name_space,
                             const std::string& uri, int score,
                             int64_t creation_timestamp_ms) {
  return DocumentBuilder()
      .SetKey(name_space, uri)
      .SetSchema("email")
      .SetScore(score)
      .SetCreationTimestampMs(creation_timestamp_ms)
      .Build();
}

libtextclassifier3::StatusOr<
    std::pair<std::vector<DocHitInfo>, std::vector<ScoredDocumentHit>>>
CreateAndInsertsDocumentsWithScores(DocumentStore* document_store,
                                    const std::vector<int>& scores) {
  std::vector<DocHitInfo> doc_hit_infos;
  std::vector<ScoredDocumentHit> scored_document_hits;
  for (int i = 0; i < scores.size(); i++) {
    ICING_ASSIGN_OR_RETURN(DocumentId document_id,
                           document_store->Put(CreateDocument(
                               "icing", "email/" + std::to_string(i),
                               scores.at(i), kDefaultCreationTimestampMs)));
    doc_hit_infos.emplace_back(document_id);
    scored_document_hits.emplace_back(document_id, kSectionIdMaskNone,
                                      scores.at(i));
  }
  return std::pair(doc_hit_infos, scored_document_hits);
}

UsageReport CreateUsageReport(std::string name_space, std::string uri,
                              int64 timestamp_ms,
                              UsageReport::UsageType usage_type) {
  UsageReport usage_report;
  usage_report.set_document_namespace(name_space);
  usage_report.set_document_uri(uri);
  usage_report.set_usage_timestamp_ms(timestamp_ms);
  usage_report.set_usage_type(usage_type);
  return usage_report;
}

TEST_F(ScoringProcessorTest, CreationWithNullPointerShouldFail) {
  ScoringSpecProto spec_proto;
  EXPECT_THAT(ScoringProcessor::Create(spec_proto, /*document_store=*/nullptr),
              StatusIs(libtextclassifier3::StatusCode::FAILED_PRECONDITION));
}

TEST_F(ScoringProcessorTest, ShouldCreateInstance) {
  ScoringSpecProto spec_proto;
  spec_proto.set_rank_by(ScoringSpecProto::RankingStrategy::DOCUMENT_SCORE);
  ICING_EXPECT_OK(ScoringProcessor::Create(spec_proto, document_store()));
}

TEST_F(ScoringProcessorTest, ShouldHandleEmptyDocHitIterator) {
  // Creates an empty DocHitInfoIterator
  std::vector<DocHitInfo> doc_hit_infos = {};
  std::unique_ptr<DocHitInfoIterator> doc_hit_info_iterator =
      std::make_unique<DocHitInfoIteratorDummy>(doc_hit_infos);

  ScoringSpecProto spec_proto;
  spec_proto.set_rank_by(ScoringSpecProto::RankingStrategy::DOCUMENT_SCORE);

  // Creates a ScoringProcessor
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ScoringProcessor> scoring_processor,
      ScoringProcessor::Create(spec_proto, document_store()));

  EXPECT_THAT(scoring_processor->Score(std::move(doc_hit_info_iterator),
                                       /*num_to_score=*/5),
              IsEmpty());
}

TEST_F(ScoringProcessorTest, ShouldHandleNonPositiveNumToScore) {
  // Sets up documents
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentId document_id1,
      document_store()->Put(CreateDocument("icing", "email/1", /*score=*/1,
                                           kDefaultCreationTimestampMs)));
  DocHitInfo doc_hit_info1(document_id1);

  // Creates a dummy DocHitInfoIterator
  std::vector<DocHitInfo> doc_hit_infos = {doc_hit_info1};
  std::unique_ptr<DocHitInfoIterator> doc_hit_info_iterator =
      std::make_unique<DocHitInfoIteratorDummy>(doc_hit_infos);

  ScoringSpecProto spec_proto;
  spec_proto.set_rank_by(ScoringSpecProto::RankingStrategy::DOCUMENT_SCORE);

  // Creates a ScoringProcessor
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ScoringProcessor> scoring_processor,
      ScoringProcessor::Create(spec_proto, document_store()));

  EXPECT_THAT(scoring_processor->Score(std::move(doc_hit_info_iterator),
                                       /*num_to_score=*/-1),
              IsEmpty());

  doc_hit_info_iterator =
      std::make_unique<DocHitInfoIteratorDummy>(doc_hit_infos);
  EXPECT_THAT(scoring_processor->Score(std::move(doc_hit_info_iterator),
                                       /*num_to_score=*/0),
              IsEmpty());
}

TEST_F(ScoringProcessorTest, ShouldRespectNumToScore) {
  // Sets up documents
  ICING_ASSERT_OK_AND_ASSIGN(
      auto doc_hit_result_pair,
      CreateAndInsertsDocumentsWithScores(document_store(), {1, 2, 3}));
  std::vector<DocHitInfo> doc_hit_infos = std::move(doc_hit_result_pair.first);

  // Creates a dummy DocHitInfoIterator with 3 results
  std::unique_ptr<DocHitInfoIterator> doc_hit_info_iterator =
      std::make_unique<DocHitInfoIteratorDummy>(doc_hit_infos);

  ScoringSpecProto spec_proto;
  spec_proto.set_rank_by(ScoringSpecProto::RankingStrategy::DOCUMENT_SCORE);

  // Creates a ScoringProcessor
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ScoringProcessor> scoring_processor,
      ScoringProcessor::Create(spec_proto, document_store()));

  EXPECT_THAT(scoring_processor->Score(std::move(doc_hit_info_iterator),
                                       /*num_to_score=*/2),
              SizeIs(2));

  doc_hit_info_iterator =
      std::make_unique<DocHitInfoIteratorDummy>(doc_hit_infos);
  EXPECT_THAT(scoring_processor->Score(std::move(doc_hit_info_iterator),
                                       /*num_to_score=*/4),
              SizeIs(3));
}

TEST_F(ScoringProcessorTest, ShouldScoreByDocumentScore) {
  // Creates input doc_hit_infos and expected output scored_document_hits
  ICING_ASSERT_OK_AND_ASSIGN(
      auto doc_hit_result_pair,
      CreateAndInsertsDocumentsWithScores(document_store(), {1, 3, 2}));
  std::vector<DocHitInfo> doc_hit_infos = std::move(doc_hit_result_pair.first);
  std::vector<ScoredDocumentHit> scored_document_hits =
      std::move(doc_hit_result_pair.second);

  // Creates a dummy DocHitInfoIterator with 3 results
  std::unique_ptr<DocHitInfoIterator> doc_hit_info_iterator =
      std::make_unique<DocHitInfoIteratorDummy>(doc_hit_infos);

  ScoringSpecProto spec_proto;
  spec_proto.set_rank_by(ScoringSpecProto::RankingStrategy::DOCUMENT_SCORE);

  // Creates a ScoringProcessor
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ScoringProcessor> scoring_processor,
      ScoringProcessor::Create(spec_proto, document_store()));

  EXPECT_THAT(scoring_processor->Score(std::move(doc_hit_info_iterator),
                                       /*num_to_score=*/3),
              ElementsAre(EqualsScoredDocumentHit(scored_document_hits.at(0)),
                          EqualsScoredDocumentHit(scored_document_hits.at(1)),
                          EqualsScoredDocumentHit(scored_document_hits.at(2))));
}

TEST_F(ScoringProcessorTest,
       ShouldScoreByRelevanceScore_DocumentsWithDifferentLength) {
  DocumentProto document1 =
      CreateDocument("icing", "email/1", kDefaultScore,
                     /*creation_timestamp_ms=*/kDefaultCreationTimestampMs);
  DocumentProto document2 =
      CreateDocument("icing", "email/2", kDefaultScore,
                     /*creation_timestamp_ms=*/kDefaultCreationTimestampMs);
  DocumentProto document3 =
      CreateDocument("icing", "email/3", kDefaultScore,
                     /*creation_timestamp_ms=*/kDefaultCreationTimestampMs);

  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentId document_id1,
      document_store()->Put(document1, /*num_tokens=*/10));
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentId document_id2,
      document_store()->Put(document2, /*num_tokens=*/100));
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentId document_id3,
      document_store()->Put(document3, /*num_tokens=*/50));

  DocHitInfo doc_hit_info1(document_id1);
  doc_hit_info1.UpdateSection(/*section_id*/ 0, /*hit_term_frequency=*/1);
  DocHitInfo doc_hit_info2(document_id2);
  doc_hit_info2.UpdateSection(/*section_id*/ 0, /*hit_term_frequency=*/1);
  DocHitInfo doc_hit_info3(document_id3);
  doc_hit_info3.UpdateSection(/*section_id*/ 0, /*hit_term_frequency=*/1);

  SectionId section_id = 0;
  SectionIdMask section_id_mask = 1U << section_id;

  // Creates input doc_hit_infos and expected output scored_document_hits
  std::vector<DocHitInfo> doc_hit_infos = {doc_hit_info1, doc_hit_info2,
                                           doc_hit_info3};

  // Creates a dummy DocHitInfoIterator with 3 results for the query "foo"
  std::unique_ptr<DocHitInfoIterator> doc_hit_info_iterator =
      std::make_unique<DocHitInfoIteratorDummy>(doc_hit_infos, "foo");

  ScoringSpecProto spec_proto;
  spec_proto.set_rank_by(ScoringSpecProto::RankingStrategy::RELEVANCE_SCORE);

  // Creates a ScoringProcessor
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ScoringProcessor> scoring_processor,
      ScoringProcessor::Create(spec_proto, document_store()));

  std::unordered_map<std::string, std::unique_ptr<DocHitInfoIterator>>
      query_term_iterators;
  query_term_iterators["foo"] =
      std::make_unique<DocHitInfoIteratorDummy>(doc_hit_infos, "foo");
  // Since the three documents all contain the query term "foo" exactly once,
  // the document's length determines the final score. Document shorter than the
  // average corpus length are slightly boosted.
  ScoredDocumentHit expected_scored_doc_hit1(document_id1, section_id_mask,
                                             /*score=*/0.255482);
  ScoredDocumentHit expected_scored_doc_hit2(document_id2, section_id_mask,
                                             /*score=*/0.115927);
  ScoredDocumentHit expected_scored_doc_hit3(document_id3, section_id_mask,
                                             /*score=*/0.166435);
  EXPECT_THAT(
      scoring_processor->Score(std::move(doc_hit_info_iterator),
                               /*num_to_score=*/3, &query_term_iterators),
      ElementsAre(EqualsScoredDocumentHit(expected_scored_doc_hit1),
                  EqualsScoredDocumentHit(expected_scored_doc_hit2),
                  EqualsScoredDocumentHit(expected_scored_doc_hit3)));
}

TEST_F(ScoringProcessorTest,
       ShouldScoreByRelevanceScore_DocumentsWithSameLength) {
  DocumentProto document1 =
      CreateDocument("icing", "email/1", kDefaultScore,
                     /*creation_timestamp_ms=*/kDefaultCreationTimestampMs);
  DocumentProto document2 =
      CreateDocument("icing", "email/2", kDefaultScore,
                     /*creation_timestamp_ms=*/kDefaultCreationTimestampMs);
  DocumentProto document3 =
      CreateDocument("icing", "email/3", kDefaultScore,
                     /*creation_timestamp_ms=*/kDefaultCreationTimestampMs);

  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentId document_id1,
      document_store()->Put(document1, /*num_tokens=*/10));
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentId document_id2,
      document_store()->Put(document2, /*num_tokens=*/10));
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentId document_id3,
      document_store()->Put(document3, /*num_tokens=*/10));

  DocHitInfo doc_hit_info1(document_id1);
  doc_hit_info1.UpdateSection(/*section_id*/ 0, /*hit_term_frequency=*/1);
  DocHitInfo doc_hit_info2(document_id2);
  doc_hit_info2.UpdateSection(/*section_id*/ 0, /*hit_term_frequency=*/1);
  DocHitInfo doc_hit_info3(document_id3);
  doc_hit_info3.UpdateSection(/*section_id*/ 0, /*hit_term_frequency=*/1);

  SectionId section_id = 0;
  SectionIdMask section_id_mask = 1U << section_id;

  // Creates input doc_hit_infos and expected output scored_document_hits
  std::vector<DocHitInfo> doc_hit_infos = {doc_hit_info1, doc_hit_info2,
                                           doc_hit_info3};

  // Creates a dummy DocHitInfoIterator with 3 results for the query "foo"
  std::unique_ptr<DocHitInfoIterator> doc_hit_info_iterator =
      std::make_unique<DocHitInfoIteratorDummy>(doc_hit_infos, "foo");

  ScoringSpecProto spec_proto;
  spec_proto.set_rank_by(ScoringSpecProto::RankingStrategy::RELEVANCE_SCORE);

  // Creates a ScoringProcessor
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ScoringProcessor> scoring_processor,
      ScoringProcessor::Create(spec_proto, document_store()));

  std::unordered_map<std::string, std::unique_ptr<DocHitInfoIterator>>
      query_term_iterators;
  query_term_iterators["foo"] =
      std::make_unique<DocHitInfoIteratorDummy>(doc_hit_infos, "foo");
  // Since the three documents all contain the query term "foo" exactly once
  // and they have the same length, they will have the same BM25F scoret.
  ScoredDocumentHit expected_scored_doc_hit1(document_id1, section_id_mask,
                                             /*score=*/0.16173716);
  ScoredDocumentHit expected_scored_doc_hit2(document_id2, section_id_mask,
                                             /*score=*/0.16173716);
  ScoredDocumentHit expected_scored_doc_hit3(document_id3, section_id_mask,
                                             /*score=*/0.16173716);
  EXPECT_THAT(
      scoring_processor->Score(std::move(doc_hit_info_iterator),
                               /*num_to_score=*/3, &query_term_iterators),
      ElementsAre(EqualsScoredDocumentHit(expected_scored_doc_hit1),
                  EqualsScoredDocumentHit(expected_scored_doc_hit2),
                  EqualsScoredDocumentHit(expected_scored_doc_hit3)));
}

TEST_F(ScoringProcessorTest,
       ShouldScoreByRelevanceScore_DocumentsWithDifferentQueryFrequency) {
  DocumentProto document1 =
      CreateDocument("icing", "email/1", kDefaultScore,
                     /*creation_timestamp_ms=*/kDefaultCreationTimestampMs);
  DocumentProto document2 =
      CreateDocument("icing", "email/2", kDefaultScore,
                     /*creation_timestamp_ms=*/kDefaultCreationTimestampMs);
  DocumentProto document3 =
      CreateDocument("icing", "email/3", kDefaultScore,
                     /*creation_timestamp_ms=*/kDefaultCreationTimestampMs);

  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentId document_id1,
      document_store()->Put(document1, /*num_tokens=*/10));
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentId document_id2,
      document_store()->Put(document2, /*num_tokens=*/10));
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentId document_id3,
      document_store()->Put(document3, /*num_tokens=*/10));

  DocHitInfo doc_hit_info1(document_id1);
  // Document 1 contains the query term "foo" 5 times
  doc_hit_info1.UpdateSection(/*section_id*/ 0, /*hit_term_frequency=*/5);
  DocHitInfo doc_hit_info2(document_id2);
  // Document 1 contains the query term "foo" 1 time
  doc_hit_info2.UpdateSection(/*section_id*/ 0, /*hit_term_frequency=*/1);
  DocHitInfo doc_hit_info3(document_id3);
  // Document 1 contains the query term "foo" 3 times
  doc_hit_info3.UpdateSection(/*section_id*/ 0, /*hit_term_frequency=*/1);
  doc_hit_info3.UpdateSection(/*section_id*/ 1, /*hit_term_frequency=*/2);

  SectionIdMask section_id_mask1 = 0b00000001;
  SectionIdMask section_id_mask2 = 0b00000001;
  SectionIdMask section_id_mask3 = 0b00000011;

  // Creates input doc_hit_infos and expected output scored_document_hits
  std::vector<DocHitInfo> doc_hit_infos = {doc_hit_info1, doc_hit_info2,
                                           doc_hit_info3};

  // Creates a dummy DocHitInfoIterator with 3 results for the query "foo"
  std::unique_ptr<DocHitInfoIterator> doc_hit_info_iterator =
      std::make_unique<DocHitInfoIteratorDummy>(doc_hit_infos, "foo");

  ScoringSpecProto spec_proto;
  spec_proto.set_rank_by(ScoringSpecProto::RankingStrategy::RELEVANCE_SCORE);

  // Creates a ScoringProcessor
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ScoringProcessor> scoring_processor,
      ScoringProcessor::Create(spec_proto, document_store()));

  std::unordered_map<std::string, std::unique_ptr<DocHitInfoIterator>>
      query_term_iterators;
  query_term_iterators["foo"] =
      std::make_unique<DocHitInfoIteratorDummy>(doc_hit_infos, "foo");
  // Since the three documents all have the same length, the score is decided by
  // the frequency of the query term "foo".
  ScoredDocumentHit expected_scored_doc_hit1(document_id1, section_id_mask1,
                                             /*score=*/0.309497);
  ScoredDocumentHit expected_scored_doc_hit2(document_id2, section_id_mask2,
                                             /*score=*/0.16173716);
  ScoredDocumentHit expected_scored_doc_hit3(document_id3, section_id_mask3,
                                             /*score=*/0.268599);
  EXPECT_THAT(
      scoring_processor->Score(std::move(doc_hit_info_iterator),
                               /*num_to_score=*/3, &query_term_iterators),
      ElementsAre(EqualsScoredDocumentHit(expected_scored_doc_hit1),
                  EqualsScoredDocumentHit(expected_scored_doc_hit2),
                  EqualsScoredDocumentHit(expected_scored_doc_hit3)));
}

TEST_F(ScoringProcessorTest, ShouldScoreByCreationTimestamp) {
  DocumentProto document1 =
      CreateDocument("icing", "email/1", kDefaultScore,
                     /*creation_timestamp_ms=*/1571100001111);
  DocumentProto document2 =
      CreateDocument("icing", "email/2", kDefaultScore,
                     /*creation_timestamp_ms=*/1571100002222);
  DocumentProto document3 =
      CreateDocument("icing", "email/3", kDefaultScore,
                     /*creation_timestamp_ms=*/1571100003333);
  // Intentionally inserts documents in a different order
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             document_store()->Put(document1));
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id3,
                             document_store()->Put(document3));
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             document_store()->Put(document2));
  DocHitInfo doc_hit_info1(document_id1);
  DocHitInfo doc_hit_info2(document_id2);
  DocHitInfo doc_hit_info3(document_id3);
  ScoredDocumentHit scored_document_hit1(document_id1, kSectionIdMaskNone,
                                         document1.creation_timestamp_ms());
  ScoredDocumentHit scored_document_hit2(document_id2, kSectionIdMaskNone,
                                         document2.creation_timestamp_ms());
  ScoredDocumentHit scored_document_hit3(document_id3, kSectionIdMaskNone,
                                         document3.creation_timestamp_ms());

  // Creates a dummy DocHitInfoIterator with 3 results
  std::vector<DocHitInfo> doc_hit_infos = {doc_hit_info2, doc_hit_info3,
                                           doc_hit_info1};
  std::unique_ptr<DocHitInfoIterator> doc_hit_info_iterator =
      std::make_unique<DocHitInfoIteratorDummy>(doc_hit_infos);

  ScoringSpecProto spec_proto;
  spec_proto.set_rank_by(ScoringSpecProto::RankingStrategy::CREATION_TIMESTAMP);

  // Creates a ScoringProcessor which ranks in descending order
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ScoringProcessor> scoring_processor,
      ScoringProcessor::Create(spec_proto, document_store()));

  EXPECT_THAT(scoring_processor->Score(std::move(doc_hit_info_iterator),
                                       /*num_to_score=*/3),
              ElementsAre(EqualsScoredDocumentHit(scored_document_hit2),
                          EqualsScoredDocumentHit(scored_document_hit3),
                          EqualsScoredDocumentHit(scored_document_hit1)));
}

TEST_F(ScoringProcessorTest, ShouldScoreByUsageCount) {
  DocumentProto document1 =
      CreateDocument("icing", "email/1", kDefaultScore,
                     /*creation_timestamp_ms=*/kDefaultCreationTimestampMs);
  DocumentProto document2 =
      CreateDocument("icing", "email/2", kDefaultScore,
                     /*creation_timestamp_ms=*/kDefaultCreationTimestampMs);
  DocumentProto document3 =
      CreateDocument("icing", "email/3", kDefaultScore,
                     /*creation_timestamp_ms=*/kDefaultCreationTimestampMs);

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             document_store()->Put(document1));
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             document_store()->Put(document2));
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id3,
                             document_store()->Put(document3));

  // Report usage for doc1 once and doc2 twice.
  UsageReport usage_report_doc1 = CreateUsageReport(
      /*name_space=*/"icing", /*uri=*/"email/1", /*timestamp_ms=*/0,
      UsageReport::USAGE_TYPE1);
  UsageReport usage_report_doc2 = CreateUsageReport(
      /*name_space=*/"icing", /*uri=*/"email/2", /*timestamp_ms=*/0,
      UsageReport::USAGE_TYPE1);
  ICING_ASSERT_OK(document_store()->ReportUsage(usage_report_doc1));
  ICING_ASSERT_OK(document_store()->ReportUsage(usage_report_doc2));
  ICING_ASSERT_OK(document_store()->ReportUsage(usage_report_doc2));

  DocHitInfo doc_hit_info1(document_id1);
  DocHitInfo doc_hit_info2(document_id2);
  DocHitInfo doc_hit_info3(document_id3);
  ScoredDocumentHit scored_document_hit1(document_id1, kSectionIdMaskNone,
                                         /*score=*/1);
  ScoredDocumentHit scored_document_hit2(document_id2, kSectionIdMaskNone,
                                         /*score=*/2);
  ScoredDocumentHit scored_document_hit3(document_id3, kSectionIdMaskNone,
                                         /*score=*/0);

  // Creates a dummy DocHitInfoIterator with 3 results
  std::vector<DocHitInfo> doc_hit_infos = {doc_hit_info1, doc_hit_info2,
                                           doc_hit_info3};
  std::unique_ptr<DocHitInfoIterator> doc_hit_info_iterator =
      std::make_unique<DocHitInfoIteratorDummy>(doc_hit_infos);

  ScoringSpecProto spec_proto;
  spec_proto.set_rank_by(ScoringSpecProto::RankingStrategy::USAGE_TYPE1_COUNT);

  // Creates a ScoringProcessor which ranks in descending order
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ScoringProcessor> scoring_processor,
      ScoringProcessor::Create(spec_proto, document_store()));

  EXPECT_THAT(scoring_processor->Score(std::move(doc_hit_info_iterator),
                                       /*num_to_score=*/3),
              ElementsAre(EqualsScoredDocumentHit(scored_document_hit1),
                          EqualsScoredDocumentHit(scored_document_hit2),
                          EqualsScoredDocumentHit(scored_document_hit3)));
}

TEST_F(ScoringProcessorTest, ShouldScoreByUsageTimestamp) {
  DocumentProto document1 =
      CreateDocument("icing", "email/1", kDefaultScore,
                     /*creation_timestamp_ms=*/kDefaultCreationTimestampMs);
  DocumentProto document2 =
      CreateDocument("icing", "email/2", kDefaultScore,
                     /*creation_timestamp_ms=*/kDefaultCreationTimestampMs);
  DocumentProto document3 =
      CreateDocument("icing", "email/3", kDefaultScore,
                     /*creation_timestamp_ms=*/kDefaultCreationTimestampMs);

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             document_store()->Put(document1));
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             document_store()->Put(document2));
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id3,
                             document_store()->Put(document3));

  // Report usage for doc1 and doc2.
  UsageReport usage_report_doc1 = CreateUsageReport(
      /*name_space=*/"icing", /*uri=*/"email/1", /*timestamp_ms=*/1000,
      UsageReport::USAGE_TYPE1);
  UsageReport usage_report_doc2 = CreateUsageReport(
      /*name_space=*/"icing", /*uri=*/"email/2", /*timestamp_ms=*/5000,
      UsageReport::USAGE_TYPE1);
  ICING_ASSERT_OK(document_store()->ReportUsage(usage_report_doc1));
  ICING_ASSERT_OK(document_store()->ReportUsage(usage_report_doc2));

  DocHitInfo doc_hit_info1(document_id1);
  DocHitInfo doc_hit_info2(document_id2);
  DocHitInfo doc_hit_info3(document_id3);
  ScoredDocumentHit scored_document_hit1(document_id1, kSectionIdMaskNone,
                                         /*score=*/1000);
  ScoredDocumentHit scored_document_hit2(document_id2, kSectionIdMaskNone,
                                         /*score=*/5000);
  ScoredDocumentHit scored_document_hit3(document_id3, kSectionIdMaskNone,
                                         /*score=*/0);

  // Creates a dummy DocHitInfoIterator with 3 results
  std::vector<DocHitInfo> doc_hit_infos = {doc_hit_info1, doc_hit_info2,
                                           doc_hit_info3};
  std::unique_ptr<DocHitInfoIterator> doc_hit_info_iterator =
      std::make_unique<DocHitInfoIteratorDummy>(doc_hit_infos);

  ScoringSpecProto spec_proto;
  spec_proto.set_rank_by(
      ScoringSpecProto::RankingStrategy::USAGE_TYPE1_LAST_USED_TIMESTAMP);

  // Creates a ScoringProcessor which ranks in descending order
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ScoringProcessor> scoring_processor,
      ScoringProcessor::Create(spec_proto, document_store()));

  EXPECT_THAT(scoring_processor->Score(std::move(doc_hit_info_iterator),
                                       /*num_to_score=*/3),
              ElementsAre(EqualsScoredDocumentHit(scored_document_hit1),
                          EqualsScoredDocumentHit(scored_document_hit2),
                          EqualsScoredDocumentHit(scored_document_hit3)));
}

TEST_F(ScoringProcessorTest, ShouldHandleNoScores) {
  // Creates input doc_hit_infos and corresponding scored_document_hits
  ICING_ASSERT_OK_AND_ASSIGN(
      auto doc_hit_result_pair,
      CreateAndInsertsDocumentsWithScores(document_store(), {1, 2, 3}));
  std::vector<DocHitInfo> doc_hit_infos = std::move(doc_hit_result_pair.first);
  std::vector<ScoredDocumentHit> scored_document_hits =
      std::move(doc_hit_result_pair.second);

  // Creates a dummy DocHitInfoIterator with 4 results one of which doesn't have
  // a score.
  doc_hit_infos.emplace(doc_hit_infos.begin(), /*document_id_in=*/4,
                        kSectionIdMaskNone);
  std::unique_ptr<DocHitInfoIterator> doc_hit_info_iterator =
      std::make_unique<DocHitInfoIteratorDummy>(doc_hit_infos);

  // The document hit without a score will be be assigned the default score 0 in
  // a descending order.
  ScoredDocumentHit scored_document_hit_default =
      ScoredDocumentHit(4, kSectionIdMaskNone, /*score=*/0.0);

  ScoringSpecProto spec_proto;
  spec_proto.set_rank_by(ScoringSpecProto::RankingStrategy::DOCUMENT_SCORE);

  // Creates a ScoringProcessor which ranks in descending order
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ScoringProcessor> scoring_processor,
      ScoringProcessor::Create(spec_proto, document_store()));
  EXPECT_THAT(scoring_processor->Score(std::move(doc_hit_info_iterator),
                                       /*num_to_score=*/4),
              ElementsAre(EqualsScoredDocumentHit(scored_document_hit_default),
                          EqualsScoredDocumentHit(scored_document_hits.at(0)),
                          EqualsScoredDocumentHit(scored_document_hits.at(1)),
                          EqualsScoredDocumentHit(scored_document_hits.at(2))));
}

TEST_F(ScoringProcessorTest, ShouldWrapResultsWhenNoScoring) {
  DocumentProto document1 = CreateDocument("icing", "email/1", /*score=*/1,
                                           kDefaultCreationTimestampMs);
  DocumentProto document2 = CreateDocument("icing", "email/2", /*score=*/2,
                                           kDefaultCreationTimestampMs);
  DocumentProto document3 = CreateDocument("icing", "email/3", /*score=*/3,
                                           kDefaultCreationTimestampMs);

  // Intentionally inserts documents in a different order
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             document_store()->Put(document1));
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id3,
                             document_store()->Put(document3));
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             document_store()->Put(document2));
  DocHitInfo doc_hit_info1(document_id1);
  DocHitInfo doc_hit_info2(document_id2);
  DocHitInfo doc_hit_info3(document_id3);

  // The expected results should all have the default score 0.
  ScoredDocumentHit scored_document_hit1(document_id1, kSectionIdMaskNone,
                                         kDefaultScore);
  ScoredDocumentHit scored_document_hit2(document_id2, kSectionIdMaskNone,
                                         kDefaultScore);
  ScoredDocumentHit scored_document_hit3(document_id3, kSectionIdMaskNone,
                                         kDefaultScore);

  // Creates a dummy DocHitInfoIterator with 3 results
  std::vector<DocHitInfo> doc_hit_infos = {doc_hit_info2, doc_hit_info3,
                                           doc_hit_info1};
  std::unique_ptr<DocHitInfoIterator> doc_hit_info_iterator =
      std::make_unique<DocHitInfoIteratorDummy>(doc_hit_infos);

  // A ScoringSpecProto with no scoring strategy
  ScoringSpecProto spec_proto;
  spec_proto.set_rank_by(ScoringSpecProto::RankingStrategy::NONE);

  // Creates a ScoringProcessor which ranks in descending order
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ScoringProcessor> scoring_processor,
      ScoringProcessor::Create(spec_proto, document_store()));

  EXPECT_THAT(scoring_processor->Score(std::move(doc_hit_info_iterator),
                                       /*num_to_score=*/3),
              ElementsAre(EqualsScoredDocumentHit(scored_document_hit2),
                          EqualsScoredDocumentHit(scored_document_hit3),
                          EqualsScoredDocumentHit(scored_document_hit1)));
}

}  // namespace

}  // namespace lib
}  // namespace icing
