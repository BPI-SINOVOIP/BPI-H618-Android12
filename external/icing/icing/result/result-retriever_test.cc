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

#include "icing/result/result-retriever.h"

#include <limits>
#include <memory>
#include <string_view>
#include <unordered_map>

#include "gtest/gtest.h"
#include "icing/document-builder.h"
#include "icing/file/mock-filesystem.h"
#include "icing/helpers/icu/icu-data-file-helper.h"
#include "icing/portable/equals-proto.h"
#include "icing/portable/platform.h"
#include "icing/proto/document.pb.h"
#include "icing/proto/schema.pb.h"
#include "icing/proto/search.pb.h"
#include "icing/proto/term.pb.h"
#include "icing/result/projection-tree.h"
#include "icing/schema-builder.h"
#include "icing/schema/schema-store.h"
#include "icing/schema/section.h"
#include "icing/store/document-id.h"
#include "icing/testing/common-matchers.h"
#include "icing/testing/fake-clock.h"
#include "icing/testing/snippet-helpers.h"
#include "icing/testing/test-data.h"
#include "icing/testing/tmp-directory.h"
#include "icing/tokenization/language-segmenter-factory.h"
#include "icing/transform/normalizer-factory.h"
#include "icing/transform/normalizer.h"
#include "unicode/uloc.h"

namespace icing {
namespace lib {

namespace {
using ::icing::lib::portable_equals_proto::EqualsProto;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::IsEmpty;
using ::testing::Return;
using ::testing::SizeIs;

constexpr PropertyConfigProto_Cardinality_Code CARDINALITY_OPTIONAL =
    PropertyConfigProto_Cardinality_Code_OPTIONAL;

constexpr StringIndexingConfig_TokenizerType_Code TOKENIZER_PLAIN =
    StringIndexingConfig_TokenizerType_Code_PLAIN;

constexpr TermMatchType_Code MATCH_EXACT = TermMatchType_Code_EXACT_ONLY;
constexpr TermMatchType_Code MATCH_PREFIX = TermMatchType_Code_PREFIX;

class ResultRetrieverTest : public testing::Test {
 protected:
  ResultRetrieverTest() : test_dir_(GetTestTempDir() + "/icing") {
    filesystem_.CreateDirectoryRecursively(test_dir_.c_str());
  }

  void SetUp() override {
    if (!IsCfStringTokenization() && !IsReverseJniTokenization()) {
      ICING_ASSERT_OK(
          // File generated via icu_data_file rule in //icing/BUILD.
          icu_data_file_helper::SetUpICUDataFile(
              GetTestFilePath("icing/icu.dat")));
    }
    language_segmenter_factory::SegmenterOptions options(ULOC_US);
    ICING_ASSERT_OK_AND_ASSIGN(
        language_segmenter_,
        language_segmenter_factory::Create(std::move(options)));

    ICING_ASSERT_OK_AND_ASSIGN(
        schema_store_,
        SchemaStore::Create(&filesystem_, test_dir_, &fake_clock_));
    ICING_ASSERT_OK_AND_ASSIGN(normalizer_, normalizer_factory::Create(
                                                /*max_term_byte_size=*/10000));

    SchemaProto schema =
        SchemaBuilder()
            .AddType(SchemaTypeConfigBuilder()
                         .SetType("Email")
                         .AddProperty(PropertyConfigBuilder()
                                          .SetName("name")
                                          .SetDataTypeString(MATCH_PREFIX,
                                                             TOKENIZER_PLAIN)
                                          .SetCardinality(CARDINALITY_OPTIONAL))
                         .AddProperty(PropertyConfigBuilder()
                                          .SetName("body")
                                          .SetDataTypeString(MATCH_EXACT,
                                                             TOKENIZER_PLAIN)
                                          .SetCardinality(CARDINALITY_OPTIONAL))
                         .AddProperty(
                             PropertyConfigBuilder()
                                 .SetName("sender")
                                 .SetDataTypeDocument(
                                     "Person", /*index_nested_properties=*/true)
                                 .SetCardinality(CARDINALITY_OPTIONAL)))
            .AddType(
                SchemaTypeConfigBuilder()
                    .SetType("Person")
                    .AddProperty(
                        PropertyConfigBuilder()
                            .SetName("name")
                            .SetDataTypeString(MATCH_PREFIX, TOKENIZER_PLAIN)
                            .SetCardinality(CARDINALITY_OPTIONAL))
                    .AddProperty(
                        PropertyConfigBuilder()
                            .SetName("emailAddress")
                            .SetDataTypeString(MATCH_PREFIX, TOKENIZER_PLAIN)
                            .SetCardinality(CARDINALITY_OPTIONAL)))
            .Build();
    ASSERT_THAT(schema_store_->SetSchema(schema), IsOk());
  }

  void TearDown() override {
    filesystem_.DeleteDirectoryRecursively(test_dir_.c_str());
  }

  SectionId GetSectionId(const std::string& type, const std::string& property) {
    auto type_id_or = schema_store_->GetSchemaTypeId(type);
    if (!type_id_or.ok()) {
      return kInvalidSectionId;
    }
    SchemaTypeId type_id = type_id_or.ValueOrDie();
    for (SectionId section_id = 0; section_id <= kMaxSectionId; ++section_id) {
      auto metadata_or = schema_store_->GetSectionMetadata(type_id, section_id);
      if (!metadata_or.ok()) {
        break;
      }
      const SectionMetadata* metadata = metadata_or.ValueOrDie();
      if (metadata->path == property) {
        return metadata->id;
      }
    }
    return kInvalidSectionId;
  }

  const Filesystem filesystem_;
  const std::string test_dir_;
  std::unique_ptr<LanguageSegmenter> language_segmenter_;
  std::unique_ptr<SchemaStore> schema_store_;
  std::unique_ptr<Normalizer> normalizer_;
  FakeClock fake_clock_;
};

ResultSpecProto::SnippetSpecProto CreateSnippetSpec() {
  ResultSpecProto::SnippetSpecProto snippet_spec;
  snippet_spec.set_num_to_snippet(std::numeric_limits<int>::max());
  snippet_spec.set_num_matches_per_property(std::numeric_limits<int>::max());
  snippet_spec.set_max_window_bytes(1024);
  return snippet_spec;
}

DocumentProto CreateDocument(int id) {
  return DocumentBuilder()
      .SetKey("icing", "Email/" + std::to_string(id))
      .SetSchema("Email")
      .AddStringProperty("name", "subject foo " + std::to_string(id))
      .AddStringProperty("body", "body bar " + std::to_string(id))
      .SetCreationTimestampMs(1574365086666 + id)
      .Build();
}

SectionIdMask CreateSectionIdMask(const std::vector<SectionId>& section_ids) {
  SectionIdMask mask = 0;
  for (SectionId section_id : section_ids) {
    mask |= (1u << section_id);
  }
  return mask;
}

TEST_F(ResultRetrieverTest, CreationWithNullPointerShouldFail) {
  EXPECT_THAT(
      ResultRetriever::Create(/*doc_store=*/nullptr, schema_store_.get(),
                              language_segmenter_.get(), normalizer_.get()),
      StatusIs(libtextclassifier3::StatusCode::FAILED_PRECONDITION));

  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, test_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  EXPECT_THAT(
      ResultRetriever::Create(doc_store.get(), /*schema_store=*/nullptr,
                              language_segmenter_.get(), normalizer_.get()),
      StatusIs(libtextclassifier3::StatusCode::FAILED_PRECONDITION));
  EXPECT_THAT(ResultRetriever::Create(doc_store.get(), schema_store_.get(),
                                      /*language_segmenter=*/nullptr,
                                      normalizer_.get()),
              StatusIs(libtextclassifier3::StatusCode::FAILED_PRECONDITION));
  EXPECT_THAT(ResultRetriever::Create(doc_store.get(), schema_store_.get(),
                                      language_segmenter_.get(),
                                      /*normalizer=*/nullptr),
              StatusIs(libtextclassifier3::StatusCode::FAILED_PRECONDITION));
}

TEST_F(ResultRetrieverTest, ShouldRetrieveSimpleResults) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, test_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             doc_store->Put(CreateDocument(/*id=*/1)));
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             doc_store->Put(CreateDocument(/*id=*/2)));
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id3,
                             doc_store->Put(CreateDocument(/*id=*/3)));

  std::vector<SectionId> hit_section_ids = {GetSectionId("Email", "name"),
                                            GetSectionId("Email", "body")};
  SectionIdMask hit_section_id_mask = CreateSectionIdMask(hit_section_ids);
  std::vector<ScoredDocumentHit> scored_document_hits = {
      {document_id1, hit_section_id_mask, /*score=*/19},
      {document_id2, hit_section_id_mask, /*score=*/5},
      {document_id3, hit_section_id_mask, /*score=*/1}};
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ResultRetriever> result_retriever,
      ResultRetriever::Create(doc_store.get(), schema_store_.get(),
                              language_segmenter_.get(), normalizer_.get()));

  SearchResultProto::ResultProto result1;
  *result1.mutable_document() = CreateDocument(/*id=*/1);
  result1.set_score(19);
  SearchResultProto::ResultProto result2;
  *result2.mutable_document() = CreateDocument(/*id=*/2);
  result2.set_score(5);
  SearchResultProto::ResultProto result3;
  *result3.mutable_document() = CreateDocument(/*id=*/3);
  result3.set_score(1);

  SnippetContext snippet_context(
      /*query_terms_in=*/{},
      ResultSpecProto::SnippetSpecProto::default_instance(),
      TermMatchType::EXACT_ONLY);
  PageResultState page_result_state(
      std::move(scored_document_hits), /*next_page_token_in=*/1,
      std::move(snippet_context),
      std::unordered_map<std::string, ProjectionTree>(),
      /*num_previously_returned_in=*/0,
      /*num_per_page_in=*/3);
  EXPECT_THAT(
      result_retriever->RetrieveResults(page_result_state),
      IsOkAndHolds(ElementsAre(EqualsProto(result1), EqualsProto(result2),
                               EqualsProto(result3))));
}

TEST_F(ResultRetrieverTest, IgnoreErrors) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, test_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             doc_store->Put(CreateDocument(/*id=*/1)));
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             doc_store->Put(CreateDocument(/*id=*/2)));

  DocumentId invalid_document_id = -1;
  std::vector<SectionId> hit_section_ids = {GetSectionId("Email", "name"),
                                            GetSectionId("Email", "body")};
  SectionIdMask hit_section_id_mask = CreateSectionIdMask(hit_section_ids);
  std::vector<ScoredDocumentHit> scored_document_hits = {
      {document_id1, hit_section_id_mask, /*score=*/12},
      {document_id2, hit_section_id_mask, /*score=*/4},
      {invalid_document_id, hit_section_id_mask, /*score=*/0}};
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ResultRetriever> result_retriever,
      ResultRetriever::Create(doc_store.get(), schema_store_.get(),
                              language_segmenter_.get(), normalizer_.get(),
                              /*ignore_bad_document_ids=*/true));

  SearchResultProto::ResultProto result1;
  *result1.mutable_document() = CreateDocument(/*id=*/1);
  result1.set_score(12);
  SearchResultProto::ResultProto result2;
  *result2.mutable_document() = CreateDocument(/*id=*/2);
  result2.set_score(4);

  SnippetContext snippet_context(
      /*query_terms_in=*/{},
      ResultSpecProto::SnippetSpecProto::default_instance(),
      TermMatchType::EXACT_ONLY);
  PageResultState page_result_state(
      std::move(scored_document_hits), /*next_page_token_in=*/1,
      std::move(snippet_context),
      std::unordered_map<std::string, ProjectionTree>(),
      /*num_previously_returned_in=*/0,
      /*num_per_page_in=*/3);
  EXPECT_THAT(
      result_retriever->RetrieveResults(page_result_state),
      IsOkAndHolds(ElementsAre(EqualsProto(result1), EqualsProto(result2))));
}

TEST_F(ResultRetrieverTest, NotIgnoreErrors) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, test_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             doc_store->Put(CreateDocument(/*id=*/1)));
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             doc_store->Put(CreateDocument(/*id=*/2)));

  DocumentId invalid_document_id = -1;
  std::vector<SectionId> hit_section_ids = {GetSectionId("Email", "name"),
                                            GetSectionId("Email", "body")};
  SectionIdMask hit_section_id_mask = CreateSectionIdMask(hit_section_ids);
  std::vector<ScoredDocumentHit> scored_document_hits = {
      {document_id1, hit_section_id_mask, /*score=*/0},
      {document_id2, hit_section_id_mask, /*score=*/0},
      {invalid_document_id, hit_section_id_mask, /*score=*/0}};
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ResultRetriever> result_retriever,
      ResultRetriever::Create(doc_store.get(), schema_store_.get(),
                              language_segmenter_.get(), normalizer_.get(),
                              /*ignore_bad_document_ids=*/false));

  SnippetContext snippet_context(
      /*query_terms_in=*/{},
      ResultSpecProto::SnippetSpecProto::default_instance(),
      TermMatchType::EXACT_ONLY);
  PageResultState page_result_state(
      std::move(scored_document_hits), /*next_page_token_in=*/1,
      std::move(snippet_context),
      std::unordered_map<std::string, ProjectionTree>(),
      /*num_previously_returned_in=*/0,
      /*num_per_page_in=*/3);
  EXPECT_THAT(result_retriever->RetrieveResults(page_result_state),
              StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT));

  DocumentId non_existing_document_id = 4;
  page_result_state.scored_document_hits = {
      {document_id1, hit_section_id_mask, /*score=*/0},
      {document_id2, hit_section_id_mask, /*score=*/0},
      {non_existing_document_id, hit_section_id_mask, /*score=*/0}};
  EXPECT_THAT(result_retriever->RetrieveResults(page_result_state),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
}

TEST_F(ResultRetrieverTest, IOErrorShouldReturnInternalError) {
  MockFilesystem mock_filesystem;
  ON_CALL(mock_filesystem, OpenForRead(_)).WillByDefault(Return(false));

  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&mock_filesystem, test_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             doc_store->Put(CreateDocument(/*id=*/1)));
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             doc_store->Put(CreateDocument(/*id=*/2)));

  std::vector<SectionId> hit_section_ids = {GetSectionId("Email", "name"),
                                            GetSectionId("Email", "body")};
  SectionIdMask hit_section_id_mask = CreateSectionIdMask(hit_section_ids);
  std::vector<ScoredDocumentHit> scored_document_hits = {
      {document_id1, hit_section_id_mask, /*score=*/0},
      {document_id2, hit_section_id_mask, /*score=*/0}};

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ResultRetriever> result_retriever,
      ResultRetriever::Create(doc_store.get(), schema_store_.get(),
                              language_segmenter_.get(), normalizer_.get(),
                              /*ignore_bad_document_ids=*/true));

  SnippetContext snippet_context(
      /*query_terms_in=*/{},
      ResultSpecProto::SnippetSpecProto::default_instance(),
      TermMatchType::EXACT_ONLY);
  PageResultState page_result_state(
      std::move(scored_document_hits), /*next_page_token_in=*/1,
      std::move(snippet_context),
      std::unordered_map<std::string, ProjectionTree>(),
      /*num_previously_returned_in=*/0,
      /*num_per_page_in=*/2);
  EXPECT_THAT(result_retriever->RetrieveResults(page_result_state),
              StatusIs(libtextclassifier3::StatusCode::INTERNAL));
}

TEST_F(ResultRetrieverTest, DefaultSnippetSpecShouldDisableSnippeting) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, test_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             doc_store->Put(CreateDocument(/*id=*/1)));
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             doc_store->Put(CreateDocument(/*id=*/2)));
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id3,
                             doc_store->Put(CreateDocument(/*id=*/3)));

  std::vector<SectionId> hit_section_ids = {GetSectionId("Email", "name"),
                                            GetSectionId("Email", "body")};
  SectionIdMask hit_section_id_mask = CreateSectionIdMask(hit_section_ids);
  std::vector<ScoredDocumentHit> scored_document_hits = {
      {document_id1, hit_section_id_mask, /*score=*/0},
      {document_id2, hit_section_id_mask, /*score=*/0},
      {document_id3, hit_section_id_mask, /*score=*/0}};
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ResultRetriever> result_retriever,
      ResultRetriever::Create(doc_store.get(), schema_store_.get(),
                              language_segmenter_.get(), normalizer_.get()));

  SnippetContext snippet_context(
      /*query_terms_in=*/{},
      ResultSpecProto::SnippetSpecProto::default_instance(),
      TermMatchType::EXACT_ONLY);
  PageResultState page_result_state(
      std::move(scored_document_hits), /*next_page_token_in=*/1,
      std::move(snippet_context),
      std::unordered_map<std::string, ProjectionTree>(),
      /*num_previously_returned_in=*/0,
      /*num_per_page_in=*/3);
  ICING_ASSERT_OK_AND_ASSIGN(
      std::vector<SearchResultProto::ResultProto> results,
      result_retriever->RetrieveResults(page_result_state));
  ASSERT_THAT(results, SizeIs(3));
  EXPECT_THAT(results.at(0).snippet(),
              EqualsProto(SnippetProto::default_instance()));
  EXPECT_THAT(results.at(1).snippet(),
              EqualsProto(SnippetProto::default_instance()));
  EXPECT_THAT(results.at(2).snippet(),
              EqualsProto(SnippetProto::default_instance()));
}

TEST_F(ResultRetrieverTest, SimpleSnippeted) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, test_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             doc_store->Put(CreateDocument(/*id=*/1)));
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             doc_store->Put(CreateDocument(/*id=*/2)));
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id3,
                             doc_store->Put(CreateDocument(/*id=*/3)));

  std::vector<SectionId> hit_section_ids = {GetSectionId("Email", "name"),
                                            GetSectionId("Email", "body")};
  SectionIdMask hit_section_id_mask = CreateSectionIdMask(hit_section_ids);
  std::vector<ScoredDocumentHit> scored_document_hits = {
      {document_id1, hit_section_id_mask, /*score=*/0},
      {document_id2, hit_section_id_mask, /*score=*/0},
      {document_id3, hit_section_id_mask, /*score=*/0}};
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ResultRetriever> result_retriever,
      ResultRetriever::Create(doc_store.get(), schema_store_.get(),
                              language_segmenter_.get(), normalizer_.get()));

  SnippetContext snippet_context(
      /*query_terms_in=*/{{"", {"foo", "bar"}}}, CreateSnippetSpec(),
      TermMatchType::EXACT_ONLY);
  PageResultState page_result_state(
      std::move(scored_document_hits), /*next_page_token_in=*/1,
      std::move(snippet_context),
      std::unordered_map<std::string, ProjectionTree>(),
      /*num_previously_returned_in=*/0,
      /*num_per_page_in=*/3);
  ICING_ASSERT_OK_AND_ASSIGN(
      std::vector<SearchResultProto::ResultProto> result,
      result_retriever->RetrieveResults(page_result_state));
  EXPECT_THAT(result, SizeIs(3));

  const DocumentProto& result_document_one = result.at(0).document();
  const SnippetProto& result_snippet_one = result.at(0).snippet();
  EXPECT_THAT(result_document_one, EqualsProto(CreateDocument(/*id=*/1)));
  EXPECT_THAT(result_snippet_one.entries(), SizeIs(2));
  EXPECT_THAT(result_snippet_one.entries(0).property_name(), Eq("body"));
  std::string_view content = GetString(
      &result_document_one, result_snippet_one.entries(0).property_name());
  EXPECT_THAT(GetWindows(content, result_snippet_one.entries(0)),
              ElementsAre("body bar 1"));
  EXPECT_THAT(GetMatches(content, result_snippet_one.entries(0)),
              ElementsAre("bar"));
  EXPECT_THAT(result_snippet_one.entries(1).property_name(), Eq("name"));
  content = GetString(&result_document_one,
                      result_snippet_one.entries(1).property_name());
  EXPECT_THAT(GetWindows(content, result_snippet_one.entries(1)),
              ElementsAre("subject foo 1"));
  EXPECT_THAT(GetMatches(content, result_snippet_one.entries(1)),
              ElementsAre("foo"));

  const DocumentProto& result_document_two = result.at(1).document();
  const SnippetProto& result_snippet_two = result.at(1).snippet();
  EXPECT_THAT(result_document_two, EqualsProto(CreateDocument(/*id=*/2)));
  EXPECT_THAT(result_snippet_two.entries(), SizeIs(2));
  EXPECT_THAT(result_snippet_two.entries(0).property_name(), Eq("body"));
  content = GetString(&result_document_two,
                      result_snippet_two.entries(0).property_name());
  EXPECT_THAT(GetWindows(content, result_snippet_two.entries(0)),
              ElementsAre("body bar 2"));
  EXPECT_THAT(GetMatches(content, result_snippet_two.entries(0)),
              ElementsAre("bar"));
  EXPECT_THAT(result_snippet_two.entries(1).property_name(), Eq("name"));
  content = GetString(&result_document_two,
                      result_snippet_two.entries(1).property_name());
  EXPECT_THAT(GetWindows(content, result_snippet_two.entries(1)),
              ElementsAre("subject foo 2"));
  EXPECT_THAT(GetMatches(content, result_snippet_two.entries(1)),
              ElementsAre("foo"));

  const DocumentProto& result_document_three = result.at(2).document();
  const SnippetProto& result_snippet_three = result.at(2).snippet();
  EXPECT_THAT(result_document_three, EqualsProto(CreateDocument(/*id=*/3)));
  EXPECT_THAT(result_snippet_three.entries(), SizeIs(2));
  EXPECT_THAT(result_snippet_three.entries(0).property_name(), Eq("body"));
  content = GetString(&result_document_three,
                      result_snippet_three.entries(0).property_name());
  EXPECT_THAT(GetWindows(content, result_snippet_three.entries(0)),
              ElementsAre("body bar 3"));
  EXPECT_THAT(GetMatches(content, result_snippet_three.entries(0)),
              ElementsAre("bar"));
  EXPECT_THAT(result_snippet_three.entries(1).property_name(), Eq("name"));
  content = GetString(&result_document_three,
                      result_snippet_three.entries(1).property_name());
  EXPECT_THAT(GetWindows(content, result_snippet_three.entries(1)),
              ElementsAre("subject foo 3"));
  EXPECT_THAT(GetMatches(content, result_snippet_three.entries(1)),
              ElementsAre("foo"));
}

TEST_F(ResultRetrieverTest, OnlyOneDocumentSnippeted) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, test_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             doc_store->Put(CreateDocument(/*id=*/1)));
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             doc_store->Put(CreateDocument(/*id=*/2)));
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id3,
                             doc_store->Put(CreateDocument(/*id=*/3)));

  ResultSpecProto::SnippetSpecProto snippet_spec = CreateSnippetSpec();
  snippet_spec.set_num_to_snippet(1);

  std::vector<SectionId> hit_section_ids = {GetSectionId("Email", "name"),
                                            GetSectionId("Email", "body")};
  SectionIdMask hit_section_id_mask = CreateSectionIdMask(hit_section_ids);
  std::vector<ScoredDocumentHit> scored_document_hits = {
      {document_id1, hit_section_id_mask, /*score=*/0},
      {document_id2, hit_section_id_mask, /*score=*/0},
      {document_id3, hit_section_id_mask, /*score=*/0}};
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ResultRetriever> result_retriever,
      ResultRetriever::Create(doc_store.get(), schema_store_.get(),
                              language_segmenter_.get(), normalizer_.get()));

  SnippetContext snippet_context(/*query_terms_in=*/{{"", {"foo", "bar"}}},
                                 snippet_spec, TermMatchType::EXACT_ONLY);
  PageResultState page_result_state(
      std::move(scored_document_hits), /*next_page_token_in=*/1,
      std::move(snippet_context),
      std::unordered_map<std::string, ProjectionTree>(),
      /*num_previously_returned_in=*/0,
      /*num_per_page_in=*/3);
  ICING_ASSERT_OK_AND_ASSIGN(
      std::vector<SearchResultProto::ResultProto> result,
      result_retriever->RetrieveResults(page_result_state));
  EXPECT_THAT(result, SizeIs(3));

  const DocumentProto& result_document = result.at(0).document();
  const SnippetProto& result_snippet = result.at(0).snippet();
  EXPECT_THAT(result_document, EqualsProto(CreateDocument(/*id=*/1)));
  EXPECT_THAT(result_snippet.entries(), SizeIs(2));
  EXPECT_THAT(result_snippet.entries(0).property_name(), Eq("body"));
  std::string_view content =
      GetString(&result_document, result_snippet.entries(0).property_name());
  EXPECT_THAT(GetWindows(content, result_snippet.entries(0)),
              ElementsAre("body bar 1"));
  EXPECT_THAT(GetMatches(content, result_snippet.entries(0)),
              ElementsAre("bar"));
  EXPECT_THAT(result_snippet.entries(1).property_name(), Eq("name"));
  content =
      GetString(&result_document, result_snippet.entries(1).property_name());
  EXPECT_THAT(GetWindows(content, result_snippet.entries(1)),
              ElementsAre("subject foo 1"));
  EXPECT_THAT(GetMatches(content, result_snippet.entries(1)),
              ElementsAre("foo"));

  EXPECT_THAT(result[1].document(), EqualsProto(CreateDocument(/*id=*/2)));
  EXPECT_THAT(result[1].snippet(),
              EqualsProto(SnippetProto::default_instance()));

  EXPECT_THAT(result[2].document(), EqualsProto(CreateDocument(/*id=*/3)));
  EXPECT_THAT(result[2].snippet(),
              EqualsProto(SnippetProto::default_instance()));
}

TEST_F(ResultRetrieverTest, ShouldSnippetAllResults) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, test_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             doc_store->Put(CreateDocument(/*id=*/1)));
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             doc_store->Put(CreateDocument(/*id=*/2)));
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id3,
                             doc_store->Put(CreateDocument(/*id=*/3)));

  std::vector<SectionId> hit_section_ids = {GetSectionId("Email", "name"),
                                            GetSectionId("Email", "body")};
  SectionIdMask hit_section_id_mask = CreateSectionIdMask(hit_section_ids);
  std::vector<ScoredDocumentHit> scored_document_hits = {
      {document_id1, hit_section_id_mask, /*score=*/0},
      {document_id2, hit_section_id_mask, /*score=*/0},
      {document_id3, hit_section_id_mask, /*score=*/0}};
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ResultRetriever> result_retriever,
      ResultRetriever::Create(doc_store.get(), schema_store_.get(),
                              language_segmenter_.get(), normalizer_.get()));

  ResultSpecProto::SnippetSpecProto snippet_spec = CreateSnippetSpec();
  snippet_spec.set_num_to_snippet(5);
  SnippetContext snippet_context(
      /*query_terms_in=*/{{"", {"foo", "bar"}}}, std::move(snippet_spec),
      TermMatchType::EXACT_ONLY);
  PageResultState page_result_state(
      std::move(scored_document_hits), /*next_page_token_in=*/1,
      std::move(snippet_context),
      std::unordered_map<std::string, ProjectionTree>(),
      /*num_previously_returned_in=*/0,
      /*num_per_page_in=*/3);

  ICING_ASSERT_OK_AND_ASSIGN(
      std::vector<SearchResultProto::ResultProto> result,
      result_retriever->RetrieveResults(page_result_state));
  // num_to_snippet = 5, num_previously_returned_in = 0,
  // We can return 5 - 0 = 5 snippets at most. We're able to return all 3
  // snippets here.
  ASSERT_THAT(result, SizeIs(3));
  EXPECT_THAT(result[0].snippet().entries(), Not(IsEmpty()));
  EXPECT_THAT(result[1].snippet().entries(), Not(IsEmpty()));
  EXPECT_THAT(result[2].snippet().entries(), Not(IsEmpty()));
}

TEST_F(ResultRetrieverTest, ShouldSnippetSomeResults) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, test_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             doc_store->Put(CreateDocument(/*id=*/1)));
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             doc_store->Put(CreateDocument(/*id=*/2)));
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id3,
                             doc_store->Put(CreateDocument(/*id=*/3)));

  std::vector<SectionId> hit_section_ids = {GetSectionId("Email", "name"),
                                            GetSectionId("Email", "body")};
  SectionIdMask hit_section_id_mask = CreateSectionIdMask(hit_section_ids);
  std::vector<ScoredDocumentHit> scored_document_hits = {
      {document_id1, hit_section_id_mask, /*score=*/0},
      {document_id2, hit_section_id_mask, /*score=*/0},
      {document_id3, hit_section_id_mask, /*score=*/0}};
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ResultRetriever> result_retriever,
      ResultRetriever::Create(doc_store.get(), schema_store_.get(),
                              language_segmenter_.get(), normalizer_.get()));

  ResultSpecProto::SnippetSpecProto snippet_spec = CreateSnippetSpec();
  snippet_spec.set_num_to_snippet(5);
  SnippetContext snippet_context(
      /*query_terms_in=*/{{"", {"foo", "bar"}}}, std::move(snippet_spec),
      TermMatchType::EXACT_ONLY);
  PageResultState page_result_state(
      std::move(scored_document_hits), /*next_page_token_in=*/1,
      std::move(snippet_context),
      std::unordered_map<std::string, ProjectionTree>(),
      /*num_previously_returned_in=*/3,
      /*num_per_page_in=*/3);

  // num_to_snippet = 5, num_previously_returned_in = 3,
  // We can return 5 - 3 = 2 snippets.
  ICING_ASSERT_OK_AND_ASSIGN(
      std::vector<SearchResultProto::ResultProto> result,
      result_retriever->RetrieveResults(page_result_state));
  ASSERT_THAT(result, SizeIs(3));
  EXPECT_THAT(result[0].snippet().entries(), Not(IsEmpty()));
  EXPECT_THAT(result[1].snippet().entries(), Not(IsEmpty()));
  EXPECT_THAT(result[2].snippet().entries(), IsEmpty());
}

TEST_F(ResultRetrieverTest, ShouldNotSnippetAnyResults) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, test_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             doc_store->Put(CreateDocument(/*id=*/1)));
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             doc_store->Put(CreateDocument(/*id=*/2)));
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id3,
                             doc_store->Put(CreateDocument(/*id=*/3)));

  std::vector<SectionId> hit_section_ids = {GetSectionId("Email", "name"),
                                            GetSectionId("Email", "body")};
  SectionIdMask hit_section_id_mask = CreateSectionIdMask(hit_section_ids);
  std::vector<ScoredDocumentHit> scored_document_hits = {
      {document_id1, hit_section_id_mask, /*score=*/0},
      {document_id2, hit_section_id_mask, /*score=*/0},
      {document_id3, hit_section_id_mask, /*score=*/0}};
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ResultRetriever> result_retriever,
      ResultRetriever::Create(doc_store.get(), schema_store_.get(),
                              language_segmenter_.get(), normalizer_.get()));

  ResultSpecProto::SnippetSpecProto snippet_spec = CreateSnippetSpec();
  snippet_spec.set_num_to_snippet(5);
  SnippetContext snippet_context(
      /*query_terms_in=*/{{"", {"foo", "bar"}}}, std::move(snippet_spec),
      TermMatchType::EXACT_ONLY);
  PageResultState page_result_state(
      std::move(scored_document_hits), /*next_page_token_in=*/1,
      std::move(snippet_context),
      std::unordered_map<std::string, ProjectionTree>(),
      /*num_previously_returned_in=*/6,
      /*num_per_page_in=*/3);

  // num_to_snippet = 5, num_previously_returned_in = 6,
  // We can't return any snippets for this page.
  ICING_ASSERT_OK_AND_ASSIGN(
      std::vector<SearchResultProto::ResultProto> result,
      result_retriever->RetrieveResults(page_result_state));
  ASSERT_THAT(result, SizeIs(3));
  EXPECT_THAT(result[0].snippet().entries(), IsEmpty());
  EXPECT_THAT(result[1].snippet().entries(), IsEmpty());
  EXPECT_THAT(result[2].snippet().entries(), IsEmpty());
}

TEST_F(ResultRetrieverTest, ProjectionTopLevelLeadNodeFieldPath) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, test_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  // 1. Add two Email documents
  DocumentProto document_one =
      DocumentBuilder()
          .SetKey("namespace", "uri1")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddStringProperty("name", "Hello World!")
          .AddStringProperty(
              "body", "Oh what a beautiful morning! Oh what a beautiful day!")
          .Build();
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             doc_store->Put(document_one));

  DocumentProto document_two =
      DocumentBuilder()
          .SetKey("namespace", "uri2")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddStringProperty("name", "Goodnight Moon!")
          .AddStringProperty("body",
                             "Count all the sheep and tell them 'Hello'.")
          .Build();
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             doc_store->Put(document_two));

  // 2. Setup the scored results.
  std::vector<SectionId> hit_section_ids = {GetSectionId("Email", "name"),
                                            GetSectionId("Email", "body")};
  SectionIdMask hit_section_id_mask = CreateSectionIdMask(hit_section_ids);
  std::vector<ScoredDocumentHit> scored_document_hits = {
      {document_id1, hit_section_id_mask, /*score=*/0},
      {document_id2, hit_section_id_mask, /*score=*/0}};

  TypePropertyMask type_property_mask;
  type_property_mask.set_schema_type("Email");
  type_property_mask.add_paths("name");
  std::unordered_map<std::string, ProjectionTree> type_projection_tree_map;
  type_projection_tree_map.insert(
      {"Email", ProjectionTree(type_property_mask)});

  SnippetContext snippet_context(
      /*query_terms_in=*/{},
      ResultSpecProto::SnippetSpecProto::default_instance(),
      TermMatchType::EXACT_ONLY);
  PageResultState page_result_state(
      std::move(scored_document_hits), /*next_page_token_in=*/1,
      std::move(snippet_context), std::move(type_projection_tree_map),
      /*num_previously_returned_in=*/0,
      /*num_per_page_in=*/2);

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ResultRetriever> result_retriever,
      ResultRetriever::Create(doc_store.get(), schema_store_.get(),
                              language_segmenter_.get(), normalizer_.get()));

  // 3. Verify that the returned results only contain the 'name' property.
  ICING_ASSERT_OK_AND_ASSIGN(
      std::vector<SearchResultProto::ResultProto> result,
      result_retriever->RetrieveResults(page_result_state));
  ASSERT_THAT(result, SizeIs(2));

  DocumentProto projected_document_one =
      DocumentBuilder()
          .SetKey("namespace", "uri1")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddStringProperty("name", "Hello World!")
          .Build();
  EXPECT_THAT(result[0].document(), EqualsProto(projected_document_one));

  DocumentProto projected_document_two =
      DocumentBuilder()
          .SetKey("namespace", "uri2")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddStringProperty("name", "Goodnight Moon!")
          .Build();
  EXPECT_THAT(result[1].document(), EqualsProto(projected_document_two));
}

TEST_F(ResultRetrieverTest, ProjectionNestedLeafNodeFieldPath) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, test_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  // 1. Add two Email documents
  DocumentProto document_one =
      DocumentBuilder()
          .SetKey("namespace", "uri1")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddDocumentProperty(
              "sender",
              DocumentBuilder()
                  .SetKey("namespace", "uri1")
                  .SetSchema("Person")
                  .AddStringProperty("name", "Meg Ryan")
                  .AddStringProperty("emailAddress", "shopgirl@aol.com")
                  .Build())
          .AddStringProperty("name", "Hello World!")
          .AddStringProperty(
              "body", "Oh what a beautiful morning! Oh what a beautiful day!")
          .Build();
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             doc_store->Put(document_one));

  DocumentProto document_two =
      DocumentBuilder()
          .SetKey("namespace", "uri2")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddDocumentProperty(
              "sender", DocumentBuilder()
                            .SetKey("namespace", "uri2")
                            .SetSchema("Person")
                            .AddStringProperty("name", "Tom Hanks")
                            .AddStringProperty("emailAddress", "ny152@aol.com")
                            .Build())
          .AddStringProperty("name", "Goodnight Moon!")
          .AddStringProperty("body",
                             "Count all the sheep and tell them 'Hello'.")
          .Build();
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             doc_store->Put(document_two));

  std::vector<SectionId> hit_section_ids = {GetSectionId("Email", "name"),
                                            GetSectionId("Email", "body")};
  SectionIdMask hit_section_id_mask = CreateSectionIdMask(hit_section_ids);
  std::vector<ScoredDocumentHit> scored_document_hits = {
      {document_id1, hit_section_id_mask, /*score=*/0},
      {document_id2, hit_section_id_mask, /*score=*/0}};

  TypePropertyMask type_property_mask;
  type_property_mask.set_schema_type("Email");
  type_property_mask.add_paths("sender.name");
  std::unordered_map<std::string, ProjectionTree> type_projection_tree_map;
  type_projection_tree_map.insert(
      {"Email", ProjectionTree(type_property_mask)});

  SnippetContext snippet_context(
      /*query_terms_in=*/{},
      ResultSpecProto::SnippetSpecProto::default_instance(),
      TermMatchType::EXACT_ONLY);
  PageResultState page_result_state(
      std::move(scored_document_hits), /*next_page_token_in=*/1,
      std::move(snippet_context), std::move(type_projection_tree_map),
      /*num_previously_returned_in=*/0,
      /*num_per_page_in=*/2);

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ResultRetriever> result_retriever,
      ResultRetriever::Create(doc_store.get(), schema_store_.get(),
                              language_segmenter_.get(), normalizer_.get()));

  // 3. Verify that the returned results only contain the 'sender.name'
  // property.
  ICING_ASSERT_OK_AND_ASSIGN(
      std::vector<SearchResultProto::ResultProto> result,
      result_retriever->RetrieveResults(page_result_state));
  ASSERT_THAT(result, SizeIs(2));

  DocumentProto projected_document_one =
      DocumentBuilder()
          .SetKey("namespace", "uri1")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddDocumentProperty("sender",
                               DocumentBuilder()
                                   .SetKey("namespace", "uri1")
                                   .SetSchema("Person")
                                   .AddStringProperty("name", "Meg Ryan")
                                   .Build())
          .Build();
  EXPECT_THAT(result[0].document(), EqualsProto(projected_document_one));

  DocumentProto projected_document_two =
      DocumentBuilder()
          .SetKey("namespace", "uri2")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddDocumentProperty("sender",
                               DocumentBuilder()
                                   .SetKey("namespace", "uri2")
                                   .SetSchema("Person")
                                   .AddStringProperty("name", "Tom Hanks")
                                   .Build())
          .Build();
  EXPECT_THAT(result[1].document(), EqualsProto(projected_document_two));
}

TEST_F(ResultRetrieverTest, ProjectionIntermediateNodeFieldPath) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, test_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  // 1. Add two Email documents
  DocumentProto document_one =
      DocumentBuilder()
          .SetKey("namespace", "uri1")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddDocumentProperty(
              "sender",
              DocumentBuilder()
                  .SetKey("namespace", "uri1")
                  .SetSchema("Person")
                  .AddStringProperty("name", "Meg Ryan")
                  .AddStringProperty("emailAddress", "shopgirl@aol.com")
                  .Build())
          .AddStringProperty("name", "Hello World!")
          .AddStringProperty(
              "body", "Oh what a beautiful morning! Oh what a beautiful day!")
          .Build();
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             doc_store->Put(document_one));

  DocumentProto document_two =
      DocumentBuilder()
          .SetKey("namespace", "uri2")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddDocumentProperty(
              "sender", DocumentBuilder()
                            .SetKey("namespace", "uri2")
                            .SetSchema("Person")
                            .AddStringProperty("name", "Tom Hanks")
                            .AddStringProperty("emailAddress", "ny152@aol.com")
                            .Build())
          .AddStringProperty("name", "Goodnight Moon!")
          .AddStringProperty("body",
                             "Count all the sheep and tell them 'Hello'.")
          .Build();
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             doc_store->Put(document_two));

  std::vector<SectionId> hit_section_ids = {GetSectionId("Email", "name"),
                                            GetSectionId("Email", "body")};
  SectionIdMask hit_section_id_mask = CreateSectionIdMask(hit_section_ids);
  std::vector<ScoredDocumentHit> scored_document_hits = {
      {document_id1, hit_section_id_mask, /*score=*/0},
      {document_id2, hit_section_id_mask, /*score=*/0}};

  TypePropertyMask type_property_mask;
  type_property_mask.set_schema_type("Email");
  type_property_mask.add_paths("sender");
  std::unordered_map<std::string, ProjectionTree> type_projection_tree_map;
  type_projection_tree_map.insert(
      {"Email", ProjectionTree(type_property_mask)});

  SnippetContext snippet_context(
      /*query_terms_in=*/{},
      ResultSpecProto::SnippetSpecProto::default_instance(),
      TermMatchType::EXACT_ONLY);
  PageResultState page_result_state(
      std::move(scored_document_hits), /*next_page_token_in=*/1,
      std::move(snippet_context), std::move(type_projection_tree_map),
      /*num_previously_returned_in=*/0,
      /*num_per_page_in=*/2);

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ResultRetriever> result_retriever,
      ResultRetriever::Create(doc_store.get(), schema_store_.get(),
                              language_segmenter_.get(), normalizer_.get()));

  // 3. Verify that the returned results only contain the 'sender'
  // property and all of the subproperties of 'sender'.
  ICING_ASSERT_OK_AND_ASSIGN(
      std::vector<SearchResultProto::ResultProto> result,
      result_retriever->RetrieveResults(page_result_state));
  ASSERT_THAT(result, SizeIs(2));

  DocumentProto projected_document_one =
      DocumentBuilder()
          .SetKey("namespace", "uri1")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddDocumentProperty(
              "sender",
              DocumentBuilder()
                  .SetKey("namespace", "uri1")
                  .SetSchema("Person")
                  .AddStringProperty("name", "Meg Ryan")
                  .AddStringProperty("emailAddress", "shopgirl@aol.com")
                  .Build())
          .Build();
  EXPECT_THAT(result[0].document(), EqualsProto(projected_document_one));

  DocumentProto projected_document_two =
      DocumentBuilder()
          .SetKey("namespace", "uri2")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddDocumentProperty(
              "sender", DocumentBuilder()
                            .SetKey("namespace", "uri2")
                            .SetSchema("Person")
                            .AddStringProperty("name", "Tom Hanks")
                            .AddStringProperty("emailAddress", "ny152@aol.com")
                            .Build())
          .Build();
  EXPECT_THAT(result[1].document(), EqualsProto(projected_document_two));
}

TEST_F(ResultRetrieverTest, ProjectionMultipleNestedFieldPaths) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, test_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  // 1. Add two Email documents
  DocumentProto document_one =
      DocumentBuilder()
          .SetKey("namespace", "uri1")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddDocumentProperty(
              "sender",
              DocumentBuilder()
                  .SetKey("namespace", "uri1")
                  .SetSchema("Person")
                  .AddStringProperty("name", "Meg Ryan")
                  .AddStringProperty("emailAddress", "shopgirl@aol.com")
                  .Build())
          .AddStringProperty("name", "Hello World!")
          .AddStringProperty(
              "body", "Oh what a beautiful morning! Oh what a beautiful day!")
          .Build();
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             doc_store->Put(document_one));

  DocumentProto document_two =
      DocumentBuilder()
          .SetKey("namespace", "uri2")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddDocumentProperty(
              "sender", DocumentBuilder()
                            .SetKey("namespace", "uri2")
                            .SetSchema("Person")
                            .AddStringProperty("name", "Tom Hanks")
                            .AddStringProperty("emailAddress", "ny152@aol.com")
                            .Build())
          .AddStringProperty("name", "Goodnight Moon!")
          .AddStringProperty("body",
                             "Count all the sheep and tell them 'Hello'.")
          .Build();
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             doc_store->Put(document_two));

  // 2. Setup the scored results.
  std::vector<SectionId> hit_section_ids = {GetSectionId("Email", "name"),
                                            GetSectionId("Email", "body")};
  SectionIdMask hit_section_id_mask = CreateSectionIdMask(hit_section_ids);
  std::vector<ScoredDocumentHit> scored_document_hits = {
      {document_id1, hit_section_id_mask, /*score=*/0},
      {document_id2, hit_section_id_mask, /*score=*/0}};

  TypePropertyMask type_property_mask;
  type_property_mask.set_schema_type("Email");
  type_property_mask.add_paths("sender.name");
  type_property_mask.add_paths("sender.emailAddress");
  std::unordered_map<std::string, ProjectionTree> type_projection_tree_map;
  type_projection_tree_map.insert(
      {"Email", ProjectionTree(type_property_mask)});

  SnippetContext snippet_context(
      /*query_terms_in=*/{},
      ResultSpecProto::SnippetSpecProto::default_instance(),
      TermMatchType::EXACT_ONLY);
  PageResultState page_result_state(
      std::move(scored_document_hits), /*next_page_token_in=*/1,
      std::move(snippet_context), std::move(type_projection_tree_map),
      /*num_previously_returned_in=*/0,
      /*num_per_page_in=*/2);

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ResultRetriever> result_retriever,
      ResultRetriever::Create(doc_store.get(), schema_store_.get(),
                              language_segmenter_.get(), normalizer_.get()));

  // 3. Verify that the returned results only contain the 'sender.name' and
  // 'sender.address' properties.
  ICING_ASSERT_OK_AND_ASSIGN(
      std::vector<SearchResultProto::ResultProto> result,
      result_retriever->RetrieveResults(page_result_state));
  ASSERT_THAT(result, SizeIs(2));

  DocumentProto projected_document_one =
      DocumentBuilder()
          .SetKey("namespace", "uri1")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddDocumentProperty(
              "sender",
              DocumentBuilder()
                  .SetKey("namespace", "uri1")
                  .SetSchema("Person")
                  .AddStringProperty("name", "Meg Ryan")
                  .AddStringProperty("emailAddress", "shopgirl@aol.com")
                  .Build())
          .Build();
  EXPECT_THAT(result[0].document(), EqualsProto(projected_document_one));

  DocumentProto projected_document_two =
      DocumentBuilder()
          .SetKey("namespace", "uri2")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddDocumentProperty(
              "sender", DocumentBuilder()
                            .SetKey("namespace", "uri2")
                            .SetSchema("Person")
                            .AddStringProperty("name", "Tom Hanks")
                            .AddStringProperty("emailAddress", "ny152@aol.com")
                            .Build())
          .Build();
  EXPECT_THAT(result[1].document(), EqualsProto(projected_document_two));
}

TEST_F(ResultRetrieverTest, ProjectionEmptyFieldPath) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, test_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  // 1. Add two Email documents
  DocumentProto document_one =
      DocumentBuilder()
          .SetKey("namespace", "uri1")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddStringProperty("name", "Hello World!")
          .AddStringProperty(
              "body", "Oh what a beautiful morning! Oh what a beautiful day!")
          .Build();
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             doc_store->Put(document_one));

  DocumentProto document_two =
      DocumentBuilder()
          .SetKey("namespace", "uri2")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddStringProperty("name", "Goodnight Moon!")
          .AddStringProperty("body",
                             "Count all the sheep and tell them 'Hello'.")
          .Build();
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             doc_store->Put(document_two));

  // 2. Setup the scored results.
  std::vector<SectionId> hit_section_ids = {GetSectionId("Email", "name"),
                                            GetSectionId("Email", "body")};
  SectionIdMask hit_section_id_mask = CreateSectionIdMask(hit_section_ids);
  std::vector<ScoredDocumentHit> scored_document_hits = {
      {document_id1, hit_section_id_mask, /*score=*/0},
      {document_id2, hit_section_id_mask, /*score=*/0}};

  TypePropertyMask type_property_mask;
  type_property_mask.set_schema_type("Email");
  std::unordered_map<std::string, ProjectionTree> type_projection_tree_map;
  type_projection_tree_map.insert(
      {"Email", ProjectionTree(type_property_mask)});

  SnippetContext snippet_context(
      /*query_terms_in=*/{},
      ResultSpecProto::SnippetSpecProto::default_instance(),
      TermMatchType::EXACT_ONLY);
  PageResultState page_result_state(
      std::move(scored_document_hits), /*next_page_token_in=*/1,
      std::move(snippet_context), std::move(type_projection_tree_map),
      /*num_previously_returned_in=*/0,
      /*num_per_page_in=*/2);

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ResultRetriever> result_retriever,
      ResultRetriever::Create(doc_store.get(), schema_store_.get(),
                              language_segmenter_.get(), normalizer_.get()));

  // 3. Verify that the returned results contain *no* properties.
  ICING_ASSERT_OK_AND_ASSIGN(
      std::vector<SearchResultProto::ResultProto> result,
      result_retriever->RetrieveResults(page_result_state));
  ASSERT_THAT(result, SizeIs(2));

  DocumentProto projected_document_one = DocumentBuilder()
                                             .SetKey("namespace", "uri1")
                                             .SetCreationTimestampMs(1000)
                                             .SetSchema("Email")
                                             .Build();
  EXPECT_THAT(result[0].document(), EqualsProto(projected_document_one));

  DocumentProto projected_document_two = DocumentBuilder()
                                             .SetKey("namespace", "uri2")
                                             .SetCreationTimestampMs(1000)
                                             .SetSchema("Email")
                                             .Build();
  EXPECT_THAT(result[1].document(), EqualsProto(projected_document_two));
}

TEST_F(ResultRetrieverTest, ProjectionInvalidFieldPath) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, test_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  // 1. Add two Email documents
  DocumentProto document_one =
      DocumentBuilder()
          .SetKey("namespace", "uri1")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddStringProperty("name", "Hello World!")
          .AddStringProperty(
              "body", "Oh what a beautiful morning! Oh what a beautiful day!")
          .Build();
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             doc_store->Put(document_one));

  DocumentProto document_two =
      DocumentBuilder()
          .SetKey("namespace", "uri2")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddStringProperty("name", "Goodnight Moon!")
          .AddStringProperty("body",
                             "Count all the sheep and tell them 'Hello'.")
          .Build();
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             doc_store->Put(document_two));

  // 2. Setup the scored results.
  std::vector<SectionId> hit_section_ids = {GetSectionId("Email", "name"),
                                            GetSectionId("Email", "body")};
  SectionIdMask hit_section_id_mask = CreateSectionIdMask(hit_section_ids);
  std::vector<ScoredDocumentHit> scored_document_hits = {
      {document_id1, hit_section_id_mask, /*score=*/0},
      {document_id2, hit_section_id_mask, /*score=*/0}};

  TypePropertyMask type_property_mask;
  type_property_mask.set_schema_type("Email");
  type_property_mask.add_paths("nonExistentProperty");
  std::unordered_map<std::string, ProjectionTree> type_projection_tree_map;
  type_projection_tree_map.insert(
      {"Email", ProjectionTree(type_property_mask)});

  SnippetContext snippet_context(
      /*query_terms_in=*/{},
      ResultSpecProto::SnippetSpecProto::default_instance(),
      TermMatchType::EXACT_ONLY);
  PageResultState page_result_state(
      std::move(scored_document_hits), /*next_page_token_in=*/1,
      std::move(snippet_context), std::move(type_projection_tree_map),
      /*num_previously_returned_in=*/0,
      /*num_per_page_in=*/2);

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ResultRetriever> result_retriever,
      ResultRetriever::Create(doc_store.get(), schema_store_.get(),
                              language_segmenter_.get(), normalizer_.get()));

  // 3. Verify that the returned results contain *no* properties.
  ICING_ASSERT_OK_AND_ASSIGN(
      std::vector<SearchResultProto::ResultProto> result,
      result_retriever->RetrieveResults(page_result_state));
  ASSERT_THAT(result, SizeIs(2));

  DocumentProto projected_document_one = DocumentBuilder()
                                             .SetKey("namespace", "uri1")
                                             .SetCreationTimestampMs(1000)
                                             .SetSchema("Email")
                                             .Build();
  EXPECT_THAT(result[0].document(), EqualsProto(projected_document_one));

  DocumentProto projected_document_two = DocumentBuilder()
                                             .SetKey("namespace", "uri2")
                                             .SetCreationTimestampMs(1000)
                                             .SetSchema("Email")
                                             .Build();
  EXPECT_THAT(result[1].document(), EqualsProto(projected_document_two));
}

TEST_F(ResultRetrieverTest, ProjectionValidAndInvalidFieldPath) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, test_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  // 1. Add two Email documents
  DocumentProto document_one =
      DocumentBuilder()
          .SetKey("namespace", "uri1")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddStringProperty("name", "Hello World!")
          .AddStringProperty(
              "body", "Oh what a beautiful morning! Oh what a beautiful day!")
          .Build();
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             doc_store->Put(document_one));

  DocumentProto document_two =
      DocumentBuilder()
          .SetKey("namespace", "uri2")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddStringProperty("name", "Goodnight Moon!")
          .AddStringProperty("body",
                             "Count all the sheep and tell them 'Hello'.")
          .Build();
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             doc_store->Put(document_two));

  // 2. Setup the scored results.
  std::vector<SectionId> hit_section_ids = {GetSectionId("Email", "name"),
                                            GetSectionId("Email", "body")};
  SectionIdMask hit_section_id_mask = CreateSectionIdMask(hit_section_ids);
  std::vector<ScoredDocumentHit> scored_document_hits = {
      {document_id1, hit_section_id_mask, /*score=*/0},
      {document_id2, hit_section_id_mask, /*score=*/0}};

  TypePropertyMask type_property_mask;
  type_property_mask.set_schema_type("Email");
  type_property_mask.add_paths("name");
  type_property_mask.add_paths("nonExistentProperty");
  std::unordered_map<std::string, ProjectionTree> type_projection_tree_map;
  type_projection_tree_map.insert(
      {"Email", ProjectionTree(type_property_mask)});

  SnippetContext snippet_context(
      /*query_terms_in=*/{},
      ResultSpecProto::SnippetSpecProto::default_instance(),
      TermMatchType::EXACT_ONLY);
  PageResultState page_result_state(
      std::move(scored_document_hits), /*next_page_token_in=*/1,
      std::move(snippet_context), std::move(type_projection_tree_map),
      /*num_previously_returned_in=*/0,
      /*num_per_page_in=*/2);

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ResultRetriever> result_retriever,
      ResultRetriever::Create(doc_store.get(), schema_store_.get(),
                              language_segmenter_.get(), normalizer_.get()));

  // 3. Verify that the returned results only contain the 'name' property.
  ICING_ASSERT_OK_AND_ASSIGN(
      std::vector<SearchResultProto::ResultProto> result,
      result_retriever->RetrieveResults(page_result_state));
  ASSERT_THAT(result, SizeIs(2));

  DocumentProto projected_document_one =
      DocumentBuilder()
          .SetKey("namespace", "uri1")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddStringProperty("name", "Hello World!")
          .Build();
  EXPECT_THAT(result[0].document(), EqualsProto(projected_document_one));

  DocumentProto projected_document_two =
      DocumentBuilder()
          .SetKey("namespace", "uri2")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddStringProperty("name", "Goodnight Moon!")
          .Build();
  EXPECT_THAT(result[1].document(), EqualsProto(projected_document_two));
}

TEST_F(ResultRetrieverTest, ProjectionMultipleTypesNoWildcards) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, test_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  // 1. Add two documents
  DocumentProto document_one =
      DocumentBuilder()
          .SetKey("namespace", "uri1")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddStringProperty("name", "Hello World!")
          .AddStringProperty(
              "body", "Oh what a beautiful morning! Oh what a beautiful day!")
          .Build();
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             doc_store->Put(document_one));

  DocumentProto document_two =
      DocumentBuilder()
          .SetKey("namespace", "uri2")
          .SetCreationTimestampMs(1000)
          .SetSchema("Person")
          .AddStringProperty("name", "Joe Fox")
          .AddStringProperty("emailAddress", "ny152@aol.com")
          .Build();
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             doc_store->Put(document_two));

  // 2. Setup the scored results.
  std::vector<SectionId> hit_section_ids = {GetSectionId("Email", "name"),
                                            GetSectionId("Email", "body")};
  SectionIdMask hit_section_id_mask = CreateSectionIdMask(hit_section_ids);
  std::vector<ScoredDocumentHit> scored_document_hits = {
      {document_id1, hit_section_id_mask, /*score=*/0},
      {document_id2, hit_section_id_mask, /*score=*/0}};

  TypePropertyMask type_property_mask;
  type_property_mask.set_schema_type("Email");
  type_property_mask.add_paths("name");
  std::unordered_map<std::string, ProjectionTree> type_projection_tree_map;
  type_projection_tree_map.insert(
      {"Email", ProjectionTree(type_property_mask)});

  SnippetContext snippet_context(
      /*query_terms_in=*/{},
      ResultSpecProto::SnippetSpecProto::default_instance(),
      TermMatchType::EXACT_ONLY);
  PageResultState page_result_state(
      std::move(scored_document_hits), /*next_page_token_in=*/1,
      std::move(snippet_context), std::move(type_projection_tree_map),
      /*num_previously_returned_in=*/0,
      /*num_per_page_in=*/2);

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ResultRetriever> result_retriever,
      ResultRetriever::Create(doc_store.get(), schema_store_.get(),
                              language_segmenter_.get(), normalizer_.get()));

  // 3. Verify that the returned Email results only contain the 'name'
  // property and the returned Person results have all of their properties.
  ICING_ASSERT_OK_AND_ASSIGN(
      std::vector<SearchResultProto::ResultProto> result,
      result_retriever->RetrieveResults(page_result_state));
  ASSERT_THAT(result, SizeIs(2));

  DocumentProto projected_document_one =
      DocumentBuilder()
          .SetKey("namespace", "uri1")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddStringProperty("name", "Hello World!")
          .Build();
  EXPECT_THAT(result[0].document(), EqualsProto(projected_document_one));

  DocumentProto projected_document_two =
      DocumentBuilder()
          .SetKey("namespace", "uri2")
          .SetCreationTimestampMs(1000)
          .SetSchema("Person")
          .AddStringProperty("name", "Joe Fox")
          .AddStringProperty("emailAddress", "ny152@aol.com")
          .Build();
  EXPECT_THAT(result[1].document(), EqualsProto(projected_document_two));
}

TEST_F(ResultRetrieverTest, ProjectionMultipleTypesWildcard) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, test_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  // 1. Add two documents
  DocumentProto document_one =
      DocumentBuilder()
          .SetKey("namespace", "uri1")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddStringProperty("name", "Hello World!")
          .AddStringProperty(
              "body", "Oh what a beautiful morning! Oh what a beautiful day!")
          .Build();
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             doc_store->Put(document_one));

  DocumentProto document_two =
      DocumentBuilder()
          .SetKey("namespace", "uri2")
          .SetCreationTimestampMs(1000)
          .SetSchema("Person")
          .AddStringProperty("name", "Joe Fox")
          .AddStringProperty("emailAddress", "ny152@aol.com")
          .Build();
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             doc_store->Put(document_two));

  // 2. Setup the scored results.
  std::vector<SectionId> hit_section_ids = {GetSectionId("Email", "name"),
                                            GetSectionId("Email", "body")};
  SectionIdMask hit_section_id_mask = CreateSectionIdMask(hit_section_ids);
  std::vector<ScoredDocumentHit> scored_document_hits = {
      {document_id1, hit_section_id_mask, /*score=*/0},
      {document_id2, hit_section_id_mask, /*score=*/0}};

  TypePropertyMask wildcard_type_property_mask;
  wildcard_type_property_mask.set_schema_type(
      std::string(ProjectionTree::kSchemaTypeWildcard));
  wildcard_type_property_mask.add_paths("name");
  std::unordered_map<std::string, ProjectionTree> type_projection_tree_map;
  type_projection_tree_map.insert(
      {std::string(ProjectionTree::kSchemaTypeWildcard),
       ProjectionTree(wildcard_type_property_mask)});

  SnippetContext snippet_context(
      /*query_terms_in=*/{},
      ResultSpecProto::SnippetSpecProto::default_instance(),
      TermMatchType::EXACT_ONLY);
  PageResultState page_result_state(
      std::move(scored_document_hits), /*next_page_token_in=*/1,
      std::move(snippet_context), std::move(type_projection_tree_map),
      /*num_previously_returned_in=*/0,
      /*num_per_page_in=*/2);

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ResultRetriever> result_retriever,
      ResultRetriever::Create(doc_store.get(), schema_store_.get(),
                              language_segmenter_.get(), normalizer_.get()));

  // 3. Verify that the returned Email results only contain the 'name'
  // property and the returned Person results only contain the 'name' property.
  ICING_ASSERT_OK_AND_ASSIGN(
      std::vector<SearchResultProto::ResultProto> result,
      result_retriever->RetrieveResults(page_result_state));
  ASSERT_THAT(result, SizeIs(2));

  DocumentProto projected_document_one =
      DocumentBuilder()
          .SetKey("namespace", "uri1")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddStringProperty("name", "Hello World!")
          .Build();
  EXPECT_THAT(result[0].document(), EqualsProto(projected_document_one));

  DocumentProto projected_document_two =
      DocumentBuilder()
          .SetKey("namespace", "uri2")
          .SetCreationTimestampMs(1000)
          .SetSchema("Person")
          .AddStringProperty("name", "Joe Fox")
          .Build();
  EXPECT_THAT(result[1].document(), EqualsProto(projected_document_two));
}

TEST_F(ResultRetrieverTest, ProjectionMultipleTypesWildcardWithOneOverride) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, test_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  // 1. Add two documents
  DocumentProto document_one =
      DocumentBuilder()
          .SetKey("namespace", "uri1")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddStringProperty("name", "Hello World!")
          .AddStringProperty(
              "body", "Oh what a beautiful morning! Oh what a beautiful day!")
          .Build();
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             doc_store->Put(document_one));

  DocumentProto document_two =
      DocumentBuilder()
          .SetKey("namespace", "uri2")
          .SetCreationTimestampMs(1000)
          .SetSchema("Person")
          .AddStringProperty("name", "Joe Fox")
          .AddStringProperty("emailAddress", "ny152@aol.com")
          .Build();
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             doc_store->Put(document_two));

  // 2. Setup the scored results.
  std::vector<SectionId> hit_section_ids = {GetSectionId("Email", "name"),
                                            GetSectionId("Email", "body")};
  SectionIdMask hit_section_id_mask = CreateSectionIdMask(hit_section_ids);
  std::vector<ScoredDocumentHit> scored_document_hits = {
      {document_id1, hit_section_id_mask, /*score=*/0},
      {document_id2, hit_section_id_mask, /*score=*/0}};

  TypePropertyMask email_type_property_mask;
  email_type_property_mask.set_schema_type("Email");
  email_type_property_mask.add_paths("body");
  TypePropertyMask wildcard_type_property_mask;
  wildcard_type_property_mask.set_schema_type(
      std::string(ProjectionTree::kSchemaTypeWildcard));
  wildcard_type_property_mask.add_paths("name");
  std::unordered_map<std::string, ProjectionTree> type_projection_tree_map;
  type_projection_tree_map.insert(
      {"Email", ProjectionTree(email_type_property_mask)});
  type_projection_tree_map.insert(
      {std::string(ProjectionTree::kSchemaTypeWildcard),
       ProjectionTree(wildcard_type_property_mask)});

  SnippetContext snippet_context(
      /*query_terms_in=*/{},
      ResultSpecProto::SnippetSpecProto::default_instance(),
      TermMatchType::EXACT_ONLY);
  PageResultState page_result_state(
      std::move(scored_document_hits), /*next_page_token_in=*/1,
      std::move(snippet_context), std::move(type_projection_tree_map),
      /*num_previously_returned_in=*/0,
      /*num_per_page_in=*/2);

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ResultRetriever> result_retriever,
      ResultRetriever::Create(doc_store.get(), schema_store_.get(),
                              language_segmenter_.get(), normalizer_.get()));

  // 3. Verify that the returned Email results only contain the 'body'
  // property and the returned Person results  only contain the 'name' property.
  ICING_ASSERT_OK_AND_ASSIGN(
      std::vector<SearchResultProto::ResultProto> result,
      result_retriever->RetrieveResults(page_result_state));
  ASSERT_THAT(result, SizeIs(2));

  DocumentProto projected_document_one =
      DocumentBuilder()
          .SetKey("namespace", "uri1")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddStringProperty(
              "body", "Oh what a beautiful morning! Oh what a beautiful day!")
          .Build();
  EXPECT_THAT(result[0].document(), EqualsProto(projected_document_one));

  DocumentProto projected_document_two =
      DocumentBuilder()
          .SetKey("namespace", "uri2")
          .SetCreationTimestampMs(1000)
          .SetSchema("Person")
          .AddStringProperty("name", "Joe Fox")
          .Build();
  EXPECT_THAT(result[1].document(), EqualsProto(projected_document_two));
}

TEST_F(ResultRetrieverTest, ProjectionSingleTypesWildcardAndOverride) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, test_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  // 1. Add two documents
  DocumentProto document_one =
      DocumentBuilder()
          .SetKey("namespace", "uri1")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddStringProperty("name", "Hello World!")
          .AddStringProperty(
              "body", "Oh what a beautiful morning! Oh what a beautiful day!")
          .AddDocumentProperty(
              "sender",
              DocumentBuilder()
                  .SetKey("namespace", "uri")
                  .SetSchema("Person")
                  .AddStringProperty("name", "Mr. Body")
                  .AddStringProperty("emailAddress", "mr.body123@gmail.com")
                  .Build())
          .Build();
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             doc_store->Put(document_one));

  DocumentProto document_two =
      DocumentBuilder()
          .SetKey("namespace", "uri2")
          .SetCreationTimestampMs(1000)
          .SetSchema("Person")
          .AddStringProperty("name", "Joe Fox")
          .AddStringProperty("emailAddress", "ny152@aol.com")
          .Build();
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             doc_store->Put(document_two));

  // 2. Setup the scored results.
  std::vector<SectionId> hit_section_ids = {GetSectionId("Email", "name"),
                                            GetSectionId("Email", "body")};
  SectionIdMask hit_section_id_mask = CreateSectionIdMask(hit_section_ids);
  std::vector<ScoredDocumentHit> scored_document_hits = {
      {document_id1, hit_section_id_mask, /*score=*/0},
      {document_id2, hit_section_id_mask, /*score=*/0}};

  TypePropertyMask email_type_property_mask;
  email_type_property_mask.set_schema_type("Email");
  email_type_property_mask.add_paths("sender.name");
  TypePropertyMask wildcard_type_property_mask;
  wildcard_type_property_mask.set_schema_type(
      std::string(ProjectionTree::kSchemaTypeWildcard));
  wildcard_type_property_mask.add_paths("name");
  std::unordered_map<std::string, ProjectionTree> type_projection_tree_map;
  type_projection_tree_map.insert(
      {"Email", ProjectionTree(email_type_property_mask)});
  type_projection_tree_map.insert(
      {std::string(ProjectionTree::kSchemaTypeWildcard),
       ProjectionTree(wildcard_type_property_mask)});

  SnippetContext snippet_context(
      /*query_terms_in=*/{},
      ResultSpecProto::SnippetSpecProto::default_instance(),
      TermMatchType::EXACT_ONLY);
  PageResultState page_result_state(
      std::move(scored_document_hits), /*next_page_token_in=*/1,
      std::move(snippet_context), std::move(type_projection_tree_map),
      /*num_previously_returned_in=*/0,
      /*num_per_page_in=*/2);

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ResultRetriever> result_retriever,
      ResultRetriever::Create(doc_store.get(), schema_store_.get(),
                              language_segmenter_.get(), normalizer_.get()));

  // 3. Verify that the returned Email results only contain the 'sender.name'
  // property and the returned Person results only contain the 'name' property.
  ICING_ASSERT_OK_AND_ASSIGN(
      std::vector<SearchResultProto::ResultProto> result,
      result_retriever->RetrieveResults(page_result_state));
  ASSERT_THAT(result, SizeIs(2));

  DocumentProto projected_document_one =
      DocumentBuilder()
          .SetKey("namespace", "uri1")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddDocumentProperty("sender",
                               DocumentBuilder()
                                   .SetKey("namespace", "uri")
                                   .SetSchema("Person")
                                   .AddStringProperty("name", "Mr. Body")
                                   .Build())
          .Build();
  EXPECT_THAT(result[0].document(), EqualsProto(projected_document_one));

  DocumentProto projected_document_two =
      DocumentBuilder()
          .SetKey("namespace", "uri2")
          .SetCreationTimestampMs(1000)
          .SetSchema("Person")
          .AddStringProperty("name", "Joe Fox")
          .Build();
  EXPECT_THAT(result[1].document(), EqualsProto(projected_document_two));
}

TEST_F(ResultRetrieverTest,
       ProjectionSingleTypesWildcardAndOverrideNestedProperty) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, test_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  // 1. Add two documents
  DocumentProto document_one =
      DocumentBuilder()
          .SetKey("namespace", "uri1")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddStringProperty("name", "Hello World!")
          .AddStringProperty(
              "body", "Oh what a beautiful morning! Oh what a beautiful day!")
          .AddDocumentProperty(
              "sender",
              DocumentBuilder()
                  .SetKey("namespace", "uri")
                  .SetSchema("Person")
                  .AddStringProperty("name", "Mr. Body")
                  .AddStringProperty("emailAddress", "mr.body123@gmail.com")
                  .Build())
          .Build();
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             doc_store->Put(document_one));

  DocumentProto document_two =
      DocumentBuilder()
          .SetKey("namespace", "uri2")
          .SetCreationTimestampMs(1000)
          .SetSchema("Person")
          .AddStringProperty("name", "Joe Fox")
          .AddStringProperty("emailAddress", "ny152@aol.com")
          .Build();
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             doc_store->Put(document_two));

  // 2. Setup the scored results.
  std::vector<SectionId> hit_section_ids = {GetSectionId("Email", "name"),
                                            GetSectionId("Email", "body")};
  SectionIdMask hit_section_id_mask = CreateSectionIdMask(hit_section_ids);
  std::vector<ScoredDocumentHit> scored_document_hits = {
      {document_id1, hit_section_id_mask, /*score=*/0},
      {document_id2, hit_section_id_mask, /*score=*/0}};

  TypePropertyMask email_type_property_mask;
  email_type_property_mask.set_schema_type("Email");
  email_type_property_mask.add_paths("sender.name");
  TypePropertyMask wildcard_type_property_mask;
  wildcard_type_property_mask.set_schema_type(
      std::string(ProjectionTree::kSchemaTypeWildcard));
  wildcard_type_property_mask.add_paths("sender");
  std::unordered_map<std::string, ProjectionTree> type_projection_tree_map;
  type_projection_tree_map.insert(
      {"Email", ProjectionTree(email_type_property_mask)});
  type_projection_tree_map.insert(
      {std::string(ProjectionTree::kSchemaTypeWildcard),
       ProjectionTree(wildcard_type_property_mask)});

  SnippetContext snippet_context(
      /*query_terms_in=*/{},
      ResultSpecProto::SnippetSpecProto::default_instance(),
      TermMatchType::EXACT_ONLY);
  PageResultState page_result_state(
      std::move(scored_document_hits), /*next_page_token_in=*/1,
      std::move(snippet_context), std::move(type_projection_tree_map),
      /*num_previously_returned_in=*/0,
      /*num_per_page_in=*/2);

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ResultRetriever> result_retriever,
      ResultRetriever::Create(doc_store.get(), schema_store_.get(),
                              language_segmenter_.get(), normalizer_.get()));

  // 3. Verify that the returned Email results only contain the 'sender.name'
  // property and the returned Person results contain no properties.
  ICING_ASSERT_OK_AND_ASSIGN(
      std::vector<SearchResultProto::ResultProto> result,
      result_retriever->RetrieveResults(page_result_state));
  ASSERT_THAT(result, SizeIs(2));

  DocumentProto projected_document_one =
      DocumentBuilder()
          .SetKey("namespace", "uri1")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddDocumentProperty("sender",
                               DocumentBuilder()
                                   .SetKey("namespace", "uri")
                                   .SetSchema("Person")
                                   .AddStringProperty("name", "Mr. Body")
                                   .Build())
          .Build();
  EXPECT_THAT(result[0].document(), EqualsProto(projected_document_one));

  DocumentProto projected_document_two = DocumentBuilder()
                                             .SetKey("namespace", "uri2")
                                             .SetCreationTimestampMs(1000)
                                             .SetSchema("Person")
                                             .Build();
  EXPECT_THAT(result[1].document(), EqualsProto(projected_document_two));
}

}  // namespace

}  // namespace lib
}  // namespace icing
