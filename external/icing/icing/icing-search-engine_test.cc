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

#include "icing/icing-search-engine.h"

#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <utility>

#include "icing/jni/jni-cache.h"
#include "icing/text_classifier/lib3/utils/base/status.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "icing/document-builder.h"
#include "icing/file/filesystem.h"
#include "icing/file/mock-filesystem.h"
#include "icing/helpers/icu/icu-data-file-helper.h"
#include "icing/legacy/index/icing-mock-filesystem.h"
#include "icing/portable/equals-proto.h"
#include "icing/portable/platform.h"
#include "icing/proto/document.pb.h"
#include "icing/proto/initialize.pb.h"
#include "icing/proto/optimize.pb.h"
#include "icing/proto/persist.pb.h"
#include "icing/proto/schema.pb.h"
#include "icing/proto/scoring.pb.h"
#include "icing/proto/search.pb.h"
#include "icing/proto/status.pb.h"
#include "icing/schema-builder.h"
#include "icing/schema/schema-store.h"
#include "icing/schema/section.h"
#include "icing/store/document-log-creator.h"
#include "icing/testing/common-matchers.h"
#include "icing/testing/fake-clock.h"
#include "icing/testing/jni-test-helpers.h"
#include "icing/testing/random-string.h"
#include "icing/testing/snippet-helpers.h"
#include "icing/testing/test-data.h"
#include "icing/testing/tmp-directory.h"

namespace icing {
namespace lib {

namespace {

using ::icing::lib::portable_equals_proto::EqualsProto;
using ::testing::_;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Ge;
using ::testing::Gt;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::Le;
using ::testing::Lt;
using ::testing::Matcher;
using ::testing::Ne;
using ::testing::Return;
using ::testing::SizeIs;
using ::testing::StrEq;
using ::testing::UnorderedElementsAre;

constexpr std::string_view kIpsumText =
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla convallis "
    "scelerisque orci quis hendrerit. Sed augue turpis, sodales eu gravida "
    "nec, scelerisque nec leo. Maecenas accumsan interdum commodo. Aliquam "
    "mattis sapien est, sit amet interdum risus dapibus sed. Maecenas leo "
    "erat, fringilla in nisl a, venenatis gravida metus. Phasellus venenatis, "
    "orci in aliquet mattis, lectus sapien volutpat arcu, sed hendrerit ligula "
    "arcu nec mauris. Integer dolor mi, rhoncus eget gravida et, pulvinar et "
    "nunc. Aliquam ac sollicitudin nisi. Vivamus sit amet urna vestibulum, "
    "tincidunt eros sed, efficitur nisl. Fusce non neque accumsan, sagittis "
    "nisi eget, sagittis turpis. Ut pulvinar nibh eu purus feugiat faucibus. "
    "Donec tellus nulla, tincidunt vel lacus id, bibendum fermentum turpis. "
    "Nullam ultrices sed nibh vitae aliquet. Ut risus neque, consectetur "
    "vehicula posuere vitae, convallis eu lorem. Donec semper augue eu nibh "
    "placerat semper.";

constexpr PropertyConfigProto_Cardinality_Code CARDINALITY_OPTIONAL =
    PropertyConfigProto_Cardinality_Code_OPTIONAL;
constexpr PropertyConfigProto_Cardinality_Code CARDINALITY_REQUIRED =
    PropertyConfigProto_Cardinality_Code_REQUIRED;
constexpr PropertyConfigProto_Cardinality_Code CARDINALITY_REPEATED =
    PropertyConfigProto_Cardinality_Code_REPEATED;

constexpr StringIndexingConfig_TokenizerType_Code TOKENIZER_PLAIN =
    StringIndexingConfig_TokenizerType_Code_PLAIN;
constexpr StringIndexingConfig_TokenizerType_Code TOKENIZER_NONE =
    StringIndexingConfig_TokenizerType_Code_NONE;

constexpr TermMatchType_Code MATCH_EXACT = TermMatchType_Code_EXACT_ONLY;
constexpr TermMatchType_Code MATCH_PREFIX = TermMatchType_Code_PREFIX;
constexpr TermMatchType_Code MATCH_NONE = TermMatchType_Code_UNKNOWN;

PortableFileBackedProtoLog<DocumentWrapper>::Header ReadDocumentLogHeader(
    Filesystem filesystem, const std::string& file_path) {
  PortableFileBackedProtoLog<DocumentWrapper>::Header header;
  filesystem.PRead(file_path.c_str(), &header,
                   sizeof(PortableFileBackedProtoLog<DocumentWrapper>::Header),
                   /*offset=*/0);
  return header;
}

void WriteDocumentLogHeader(
    Filesystem filesystem, const std::string& file_path,
    PortableFileBackedProtoLog<DocumentWrapper>::Header& header) {
  filesystem.Write(file_path.c_str(), &header,
                   sizeof(PortableFileBackedProtoLog<DocumentWrapper>::Header));
}

// For mocking purpose, we allow tests to provide a custom Filesystem.
class TestIcingSearchEngine : public IcingSearchEngine {
 public:
  TestIcingSearchEngine(const IcingSearchEngineOptions& options,
                        std::unique_ptr<const Filesystem> filesystem,
                        std::unique_ptr<const IcingFilesystem> icing_filesystem,
                        std::unique_ptr<Clock> clock,
                        std::unique_ptr<JniCache> jni_cache)
      : IcingSearchEngine(options, std::move(filesystem),
                          std::move(icing_filesystem), std::move(clock),
                          std::move(jni_cache)) {}
};

std::string GetTestBaseDir() { return GetTestTempDir() + "/icing"; }

class IcingSearchEngineTest : public testing::Test {
 protected:
  void SetUp() override {
    if (!IsCfStringTokenization() && !IsReverseJniTokenization()) {
      // If we've specified using the reverse-JNI method for segmentation (i.e.
      // not ICU), then we won't have the ICU data file included to set up.
      // Technically, we could choose to use reverse-JNI for segmentation AND
      // include an ICU data file, but that seems unlikely and our current BUILD
      // setup doesn't do this.
      // File generated via icu_data_file rule in //icing/BUILD.
      std::string icu_data_file_path =
          GetTestFilePath("icing/icu.dat");
      ICING_ASSERT_OK(
          icu_data_file_helper::SetUpICUDataFile(icu_data_file_path));
    }
    filesystem_.CreateDirectoryRecursively(GetTestBaseDir().c_str());
  }

  void TearDown() override {
    filesystem_.DeleteDirectoryRecursively(GetTestBaseDir().c_str());
  }

  const Filesystem* filesystem() const { return &filesystem_; }

 private:
  Filesystem filesystem_;
};

constexpr int kMaxSupportedDocumentSize = (1u << 24) - 1;

// Non-zero value so we don't override it to be the current time
constexpr int64_t kDefaultCreationTimestampMs = 1575492852000;

std::string GetDocumentDir() { return GetTestBaseDir() + "/document_dir"; }

std::string GetIndexDir() { return GetTestBaseDir() + "/index_dir"; }

std::string GetSchemaDir() { return GetTestBaseDir() + "/schema_dir"; }

std::string GetHeaderFilename() {
  return GetTestBaseDir() + "/icing_search_engine_header";
}

IcingSearchEngineOptions GetDefaultIcingOptions() {
  IcingSearchEngineOptions icing_options;
  icing_options.set_base_dir(GetTestBaseDir());
  return icing_options;
}

DocumentProto CreateMessageDocument(std::string name_space, std::string uri) {
  return DocumentBuilder()
      .SetKey(std::move(name_space), std::move(uri))
      .SetSchema("Message")
      .AddStringProperty("body", "message body")
      .SetCreationTimestampMs(kDefaultCreationTimestampMs)
      .Build();
}

DocumentProto CreateEmailDocument(const std::string& name_space,
                                  const std::string& uri, int score,
                                  const std::string& subject_content,
                                  const std::string& body_content) {
  return DocumentBuilder()
      .SetKey(name_space, uri)
      .SetSchema("Email")
      .SetScore(score)
      .AddStringProperty("subject", subject_content)
      .AddStringProperty("body", body_content)
      .Build();
}

SchemaProto CreateMessageSchema() {
  return SchemaBuilder()
      .AddType(SchemaTypeConfigBuilder().SetType("Message").AddProperty(
          PropertyConfigBuilder()
              .SetName("body")
              .SetDataTypeString(MATCH_PREFIX, TOKENIZER_PLAIN)
              .SetCardinality(CARDINALITY_REQUIRED)))
      .Build();
}

SchemaProto CreateEmailSchema() {
  return SchemaBuilder()
      .AddType(
          SchemaTypeConfigBuilder()
              .SetType("Email")
              .AddProperty(PropertyConfigBuilder()
                               .SetName("body")
                               .SetDataTypeString(MATCH_PREFIX, TOKENIZER_PLAIN)
                               .SetCardinality(CARDINALITY_REQUIRED))
              .AddProperty(PropertyConfigBuilder()
                               .SetName("subject")
                               .SetDataTypeString(MATCH_PREFIX, TOKENIZER_PLAIN)
                               .SetCardinality(CARDINALITY_REQUIRED)))
      .Build();
}

SchemaProto CreatePersonAndEmailSchema() {
  return SchemaBuilder()
      .AddType(
          SchemaTypeConfigBuilder()
              .SetType("Person")
              .AddProperty(PropertyConfigBuilder()
                               .SetName("name")
                               .SetDataTypeString(MATCH_PREFIX, TOKENIZER_PLAIN)
                               .SetCardinality(CARDINALITY_OPTIONAL))
              .AddProperty(PropertyConfigBuilder()
                               .SetName("emailAddress")
                               .SetDataTypeString(MATCH_PREFIX, TOKENIZER_PLAIN)
                               .SetCardinality(CARDINALITY_OPTIONAL)))
      .AddType(
          SchemaTypeConfigBuilder()
              .SetType("Email")
              .AddProperty(PropertyConfigBuilder()
                               .SetName("body")
                               .SetDataTypeString(MATCH_PREFIX, TOKENIZER_PLAIN)
                               .SetCardinality(CARDINALITY_OPTIONAL))
              .AddProperty(PropertyConfigBuilder()
                               .SetName("subject")
                               .SetDataTypeString(MATCH_PREFIX, TOKENIZER_PLAIN)
                               .SetCardinality(CARDINALITY_OPTIONAL))
              .AddProperty(PropertyConfigBuilder()
                               .SetName("sender")
                               .SetDataTypeDocument(
                                   "Person", /*index_nested_properties=*/true)
                               .SetCardinality(CARDINALITY_OPTIONAL)))
      .Build();
}

ScoringSpecProto GetDefaultScoringSpec() {
  ScoringSpecProto scoring_spec;
  scoring_spec.set_rank_by(ScoringSpecProto::RankingStrategy::DOCUMENT_SCORE);
  return scoring_spec;
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

std::vector<std::string> GetUrisFromSearchResults(
    SearchResultProto& search_result_proto) {
  std::vector<std::string> result_uris;
  result_uris.reserve(search_result_proto.results_size());
  for (int i = 0; i < search_result_proto.results_size(); i++) {
    result_uris.push_back(
        search_result_proto.mutable_results(i)->document().uri());
  }
  return result_uris;
}

TEST_F(IcingSearchEngineTest, SimpleInitialization) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  DocumentProto document = CreateMessageDocument("namespace", "uri");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(DocumentProto(document)).status(), ProtoIsOk());
}

TEST_F(IcingSearchEngineTest, InitializingAgainSavesNonPersistedData) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  DocumentProto document = CreateMessageDocument("namespace", "uri");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());

  GetResultProto expected_get_result_proto;
  expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_get_result_proto.mutable_document() = document;

  ASSERT_THAT(
      icing.Get("namespace", "uri", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(
      icing.Get("namespace", "uri", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));
}

TEST_F(IcingSearchEngineTest, MaxIndexMergeSizeReturnsInvalidArgument) {
  IcingSearchEngineOptions options = GetDefaultIcingOptions();
  options.set_index_merge_size(std::numeric_limits<int32_t>::max());
  IcingSearchEngine icing(options, GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(),
              ProtoStatusIs(StatusProto::INVALID_ARGUMENT));
}

TEST_F(IcingSearchEngineTest, NegativeMergeSizeReturnsInvalidArgument) {
  IcingSearchEngineOptions options = GetDefaultIcingOptions();
  options.set_index_merge_size(-1);
  IcingSearchEngine icing(options, GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(),
              ProtoStatusIs(StatusProto::INVALID_ARGUMENT));
}

TEST_F(IcingSearchEngineTest, ZeroMergeSizeReturnsInvalidArgument) {
  IcingSearchEngineOptions options = GetDefaultIcingOptions();
  options.set_index_merge_size(0);
  IcingSearchEngine icing(options, GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(),
              ProtoStatusIs(StatusProto::INVALID_ARGUMENT));
}

TEST_F(IcingSearchEngineTest, GoodIndexMergeSizeReturnsOk) {
  IcingSearchEngineOptions options = GetDefaultIcingOptions();
  // One is fine, if a bit weird. It just means that the lite index will be
  // smaller and will request a merge any time content is added to it.
  options.set_index_merge_size(1);
  IcingSearchEngine icing(options, GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
}

TEST_F(IcingSearchEngineTest,
       NegativeMaxTokensPerDocSizeReturnsInvalidArgument) {
  IcingSearchEngineOptions options = GetDefaultIcingOptions();
  options.set_max_tokens_per_doc(-1);
  IcingSearchEngine icing(options, GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(),
              ProtoStatusIs(StatusProto::INVALID_ARGUMENT));
}

TEST_F(IcingSearchEngineTest, ZeroMaxTokensPerDocSizeReturnsInvalidArgument) {
  IcingSearchEngineOptions options = GetDefaultIcingOptions();
  options.set_max_tokens_per_doc(0);
  IcingSearchEngine icing(options, GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(),
              ProtoStatusIs(StatusProto::INVALID_ARGUMENT));
}

TEST_F(IcingSearchEngineTest, GoodMaxTokensPerDocSizeReturnsOk) {
  IcingSearchEngineOptions options = GetDefaultIcingOptions();
  // INT_MAX is valid - it just means that we shouldn't limit the number of
  // tokens per document. It would be pretty inconceivable that anyone would
  // produce such a document - the text being indexed alone would take up at
  // least ~4.3 GiB! - and the document would be rejected before indexing
  // for exceeding max_document_size, but there's no reason to explicitly
  // bar it.
  options.set_max_tokens_per_doc(std::numeric_limits<int32_t>::max());
  IcingSearchEngine icing(options, GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
}

TEST_F(IcingSearchEngineTest, NegativeMaxTokenLenReturnsInvalidArgument) {
  IcingSearchEngineOptions options = GetDefaultIcingOptions();
  options.set_max_token_length(-1);
  IcingSearchEngine icing(options, GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(),
              ProtoStatusIs(StatusProto::INVALID_ARGUMENT));
}

TEST_F(IcingSearchEngineTest, ZeroMaxTokenLenReturnsInvalidArgument) {
  IcingSearchEngineOptions options = GetDefaultIcingOptions();
  options.set_max_token_length(0);
  IcingSearchEngine icing(options, GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(),
              ProtoStatusIs(StatusProto::INVALID_ARGUMENT));
}

TEST_F(IcingSearchEngineTest, MaxTokenLenReturnsOkAndTruncatesTokens) {
  IcingSearchEngineOptions options = GetDefaultIcingOptions();
  // A length of 1 is allowed - even though it would be strange to want
  // this.
  options.set_max_token_length(1);
  IcingSearchEngine icing(options, GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  DocumentProto document = CreateMessageDocument("namespace", "uri");
  EXPECT_THAT(icing.Put(document).status(), ProtoIsOk());

  // "message" should have been truncated to "m"
  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  // The indexed tokens were  truncated to length of 1, so "m" will match
  search_spec.set_query("m");

  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document;

  SearchResultProto actual_results =
      icing.Search(search_spec, GetDefaultScoringSpec(),
                   ResultSpecProto::default_instance());
  EXPECT_THAT(actual_results, EqualsSearchResultIgnoreStatsAndScores(
                                  expected_search_result_proto));

  // The query token is also truncated to length of 1, so "me"->"m" matches "m"
  search_spec.set_query("me");
  actual_results = icing.Search(search_spec, GetDefaultScoringSpec(),
                                ResultSpecProto::default_instance());
  EXPECT_THAT(actual_results, EqualsSearchResultIgnoreStatsAndScores(
                                  expected_search_result_proto));

  // The query token is still truncated to length of 1, so "massage"->"m"
  // matches "m"
  search_spec.set_query("massage");
  actual_results = icing.Search(search_spec, GetDefaultScoringSpec(),
                                ResultSpecProto::default_instance());
  EXPECT_THAT(actual_results, EqualsSearchResultIgnoreStatsAndScores(
                                  expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest,
       MaxIntMaxTokenLenReturnsOkTooLargeTokenReturnsResourceExhausted) {
  IcingSearchEngineOptions options = GetDefaultIcingOptions();
  // Set token length to max. This is allowed (it just means never to
  // truncate tokens). However, this does mean that tokens that exceed the
  // size of the lexicon will cause indexing to fail.
  options.set_max_token_length(std::numeric_limits<int32_t>::max());
  IcingSearchEngine icing(options, GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  // Add a document that just barely fits under the max document limit.
  // This will still fail to index because we won't actually have enough
  // room in the lexicon to fit this content.
  std::string enormous_string(kMaxSupportedDocumentSize - 256, 'p');
  DocumentProto document =
      DocumentBuilder()
          .SetKey("namespace", "uri")
          .SetSchema("Message")
          .AddStringProperty("body", std::move(enormous_string))
          .Build();
  EXPECT_THAT(icing.Put(document).status(),
              ProtoStatusIs(StatusProto::OUT_OF_SPACE));

  SearchSpecProto search_spec;
  search_spec.set_query("p");
  search_spec.set_term_match_type(TermMatchType::PREFIX);

  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  SearchResultProto actual_results =
      icing.Search(search_spec, GetDefaultScoringSpec(),
                   ResultSpecProto::default_instance());
  EXPECT_THAT(actual_results, EqualsSearchResultIgnoreStatsAndScores(
                                  expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, FailToCreateDocStore) {
  auto mock_filesystem = std::make_unique<MockFilesystem>();
  // This fails DocumentStore::Create()
  ON_CALL(*mock_filesystem, CreateDirectoryRecursively(_))
      .WillByDefault(Return(false));

  TestIcingSearchEngine icing(GetDefaultIcingOptions(),
                              std::move(mock_filesystem),
                              std::make_unique<IcingFilesystem>(),
                              std::make_unique<FakeClock>(), GetTestJniCache());

  InitializeResultProto initialize_result_proto = icing.Initialize();
  EXPECT_THAT(initialize_result_proto.status(),
              ProtoStatusIs(StatusProto::INTERNAL));
  EXPECT_THAT(initialize_result_proto.status().message(),
              HasSubstr("Could not create directory"));
}

TEST_F(IcingSearchEngineTest,
       CircularReferenceCreateSectionManagerReturnsInvalidArgument) {
  // Create a type config with a circular reference.
  SchemaProto schema;
  auto* type = schema.add_types();
  type->set_schema_type("Message");

  auto* body = type->add_properties();
  body->set_property_name("recipient");
  body->set_schema_type("Person");
  body->set_data_type(PropertyConfigProto::DataType::DOCUMENT);
  body->set_cardinality(PropertyConfigProto::Cardinality::REQUIRED);
  body->mutable_document_indexing_config()->set_index_nested_properties(true);

  type = schema.add_types();
  type->set_schema_type("Person");

  body = type->add_properties();
  body->set_property_name("recipient");
  body->set_schema_type("Message");
  body->set_data_type(PropertyConfigProto::DataType::DOCUMENT);
  body->set_cardinality(PropertyConfigProto::Cardinality::REQUIRED);
  body->mutable_document_indexing_config()->set_index_nested_properties(true);

  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.SetSchema(schema).status(),
              ProtoStatusIs(StatusProto::INVALID_ARGUMENT));
}

TEST_F(IcingSearchEngineTest, PutWithoutSchemaFailedPrecondition) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

  DocumentProto document = CreateMessageDocument("namespace", "uri");
  PutResultProto put_result_proto = icing.Put(document);
  EXPECT_THAT(put_result_proto.status(),
              ProtoStatusIs(StatusProto::FAILED_PRECONDITION));
  EXPECT_THAT(put_result_proto.status().message(), HasSubstr("Schema not set"));
}

TEST_F(IcingSearchEngineTest, FailToReadSchema) {
  IcingSearchEngineOptions icing_options = GetDefaultIcingOptions();

  {
    // Successfully initialize and set a schema
    IcingSearchEngine icing(icing_options, GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
    ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
  }

  auto mock_filesystem = std::make_unique<MockFilesystem>();

  // This fails FileBackedProto::Read() when we try to check the schema we
  // had previously set
  ON_CALL(*mock_filesystem,
          OpenForRead(Eq(icing_options.base_dir() + "/schema_dir/schema.pb")))
      .WillByDefault(Return(-1));

  TestIcingSearchEngine test_icing(icing_options, std::move(mock_filesystem),
                                   std::make_unique<IcingFilesystem>(),
                                   std::make_unique<FakeClock>(),
                                   GetTestJniCache());

  InitializeResultProto initialize_result_proto = test_icing.Initialize();
  EXPECT_THAT(initialize_result_proto.status(),
              ProtoStatusIs(StatusProto::INTERNAL));
  EXPECT_THAT(initialize_result_proto.status().message(),
              HasSubstr("Unable to open file for read"));
}

TEST_F(IcingSearchEngineTest, FailToWriteSchema) {
  IcingSearchEngineOptions icing_options = GetDefaultIcingOptions();

  auto mock_filesystem = std::make_unique<MockFilesystem>();
  // This fails FileBackedProto::Write()
  ON_CALL(*mock_filesystem,
          OpenForWrite(Eq(icing_options.base_dir() + "/schema_dir/schema.pb")))
      .WillByDefault(Return(-1));

  TestIcingSearchEngine icing(icing_options, std::move(mock_filesystem),
                              std::make_unique<IcingFilesystem>(),
                              std::make_unique<FakeClock>(), GetTestJniCache());

  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());

  SetSchemaResultProto set_schema_result_proto =
      icing.SetSchema(CreateMessageSchema());
  EXPECT_THAT(set_schema_result_proto.status(),
              ProtoStatusIs(StatusProto::INTERNAL));
  EXPECT_THAT(set_schema_result_proto.status().message(),
              HasSubstr("Unable to open file for write"));
}

TEST_F(IcingSearchEngineTest, SetSchemaIncompatibleFails) {
  {
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

    // 1. Create a schema with an Email type with properties { "title", "body"}
    SchemaProto schema;
    SchemaTypeConfigProto* type = schema.add_types();
    type->set_schema_type("Email");
    PropertyConfigProto* property = type->add_properties();
    property->set_property_name("title");
    property->set_data_type(PropertyConfigProto::DataType::STRING);
    property->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);
    property = type->add_properties();
    property->set_property_name("body");
    property->set_data_type(PropertyConfigProto::DataType::STRING);
    property->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);

    EXPECT_THAT(icing.SetSchema(schema).status(), ProtoIsOk());

    // 2. Add an email document
    DocumentProto doc = DocumentBuilder()
                            .SetKey("emails", "email#1")
                            .SetSchema("Email")
                            .AddStringProperty("title", "Hello world.")
                            .AddStringProperty("body", "Goodnight Moon.")
                            .Build();
    EXPECT_THAT(icing.Put(std::move(doc)).status(), ProtoIsOk());
  }

  {
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

    // 3. Set a schema that deletes email. This should fail.
    SchemaProto schema;
    SchemaTypeConfigProto* type = schema.add_types();
    type->set_schema_type("Message");
    PropertyConfigProto* property = type->add_properties();
    property->set_property_name("body");
    property->set_data_type(PropertyConfigProto::DataType::STRING);
    property->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);

    EXPECT_THAT(
        icing.SetSchema(schema, /*ignore_errors_and_delete_documents=*/false)
            .status(),
        ProtoStatusIs(StatusProto::FAILED_PRECONDITION));

    // 4. Try to delete by email type. This should succeed because email wasn't
    // deleted in step 3.
    EXPECT_THAT(icing.DeleteBySchemaType("Email").status(), ProtoIsOk());
  }
}

TEST_F(IcingSearchEngineTest, SetSchemaIncompatibleForceOverrideSucceeds) {
  {
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

    // 1. Create a schema with an Email type with properties { "title", "body"}
    SchemaProto schema;
    SchemaTypeConfigProto* type = schema.add_types();
    type->set_schema_type("Email");
    PropertyConfigProto* property = type->add_properties();
    property->set_property_name("title");
    property->set_data_type(PropertyConfigProto::DataType::STRING);
    property->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);
    property = type->add_properties();
    property->set_property_name("body");
    property->set_data_type(PropertyConfigProto::DataType::STRING);
    property->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);

    EXPECT_THAT(icing.SetSchema(schema).status(), ProtoIsOk());

    // 2. Add an email document
    DocumentProto doc = DocumentBuilder()
                            .SetKey("emails", "email#1")
                            .SetSchema("Email")
                            .AddStringProperty("title", "Hello world.")
                            .AddStringProperty("body", "Goodnight Moon.")
                            .Build();
    EXPECT_THAT(icing.Put(std::move(doc)).status(), ProtoIsOk());
  }

  {
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

    // 3. Set a schema that deletes email with force override. This should
    // succeed and delete the email type.
    SchemaProto schema;
    SchemaTypeConfigProto* type = schema.add_types();
    type->set_schema_type("Message");
    PropertyConfigProto* property = type->add_properties();
    property->set_property_name("body");
    property->set_data_type(PropertyConfigProto::DataType::STRING);
    property->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);

    EXPECT_THAT(icing.SetSchema(schema, true).status(), ProtoIsOk());

    // 4. Try to delete by email type. This should fail because email was
    // already deleted.
    EXPECT_THAT(icing.DeleteBySchemaType("Email").status(),
                ProtoStatusIs(StatusProto::NOT_FOUND));
  }
}

TEST_F(IcingSearchEngineTest, SetSchemaUnsetVersionIsZero) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

  // 1. Create a schema with an Email type with version 1
  SchemaProto schema;
  SchemaTypeConfigProto* type = schema.add_types();
  type->set_schema_type("Email");
  PropertyConfigProto* property = type->add_properties();
  property->set_property_name("title");
  property->set_data_type(PropertyConfigProto::DataType::STRING);
  property->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);

  EXPECT_THAT(icing.SetSchema(schema).status(), ProtoIsOk());

  EXPECT_THAT(icing.GetSchema().schema().types(0).version(), Eq(0));
}

TEST_F(IcingSearchEngineTest, SetSchemaCompatibleVersionUpdateSucceeds) {
  {
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

    // 1. Create a schema with an Email type with version 1
    SchemaProto schema;
    SchemaTypeConfigProto* type = schema.add_types();
    type->set_version(1);
    type->set_schema_type("Email");
    PropertyConfigProto* property = type->add_properties();
    property->set_property_name("title");
    property->set_data_type(PropertyConfigProto::DataType::STRING);
    property->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);

    EXPECT_THAT(icing.SetSchema(schema).status(), ProtoIsOk());

    EXPECT_THAT(icing.GetSchema().schema().types(0).version(), Eq(1));
  }

  {
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

    // 2. Create schema that adds a new optional property and updates version.
    SchemaProto schema;
    SchemaTypeConfigProto* type = schema.add_types();
    type->set_version(2);
    type->set_schema_type("Email");
    PropertyConfigProto* property = type->add_properties();
    property->set_property_name("title");
    property->set_data_type(PropertyConfigProto::DataType::STRING);
    property->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);
    property->set_property_name("body");
    property->set_data_type(PropertyConfigProto::DataType::STRING);
    property->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);

    // 3. SetSchema should succeed and the version number should be updated.
    EXPECT_THAT(icing.SetSchema(schema, true).status(), ProtoIsOk());

    EXPECT_THAT(icing.GetSchema().schema().types(0).version(), Eq(2));
  }
}

TEST_F(IcingSearchEngineTest, SetSchemaIncompatibleVersionUpdateFails) {
  {
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

    // 1. Create a schema with an Email type with version 1
    SchemaProto schema;
    SchemaTypeConfigProto* type = schema.add_types();
    type->set_version(1);
    type->set_schema_type("Email");
    PropertyConfigProto* property = type->add_properties();
    property->set_property_name("title");
    property->set_data_type(PropertyConfigProto::DataType::STRING);
    property->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);

    EXPECT_THAT(icing.SetSchema(schema).status(), ProtoIsOk());

    EXPECT_THAT(icing.GetSchema().schema().types(0).version(), Eq(1));
  }

  {
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

    // 2. Create schema that makes an incompatible change (OPTIONAL -> REQUIRED)
    SchemaProto schema;
    SchemaTypeConfigProto* type = schema.add_types();
    type->set_version(2);
    type->set_schema_type("Email");
    PropertyConfigProto* property = type->add_properties();
    property->set_property_name("title");
    property->set_data_type(PropertyConfigProto::DataType::STRING);
    property->set_cardinality(PropertyConfigProto::Cardinality::REQUIRED);

    // 3. SetSchema should fail and the version number should NOT be updated.
    EXPECT_THAT(icing.SetSchema(schema).status(),
                ProtoStatusIs(StatusProto::FAILED_PRECONDITION));

    EXPECT_THAT(icing.GetSchema().schema().types(0).version(), Eq(1));
  }
}

TEST_F(IcingSearchEngineTest,
       SetSchemaIncompatibleVersionUpdateForceOverrideSucceeds) {
  {
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

    // 1. Create a schema with an Email type with version 1
    SchemaProto schema;
    SchemaTypeConfigProto* type = schema.add_types();
    type->set_version(1);
    type->set_schema_type("Email");
    PropertyConfigProto* property = type->add_properties();
    property->set_property_name("title");
    property->set_data_type(PropertyConfigProto::DataType::STRING);
    property->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);

    EXPECT_THAT(icing.SetSchema(schema).status(), ProtoIsOk());

    EXPECT_THAT(icing.GetSchema().schema().types(0).version(), Eq(1));
  }

  {
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

    // 2. Create schema that makes an incompatible change (OPTIONAL -> REQUIRED)
    // with force override to true.
    SchemaProto schema;
    SchemaTypeConfigProto* type = schema.add_types();
    type->set_version(2);
    type->set_schema_type("Email");
    PropertyConfigProto* property = type->add_properties();
    property->set_property_name("title");
    property->set_data_type(PropertyConfigProto::DataType::STRING);
    property->set_cardinality(PropertyConfigProto::Cardinality::REQUIRED);

    // 3. SetSchema should succeed and the version number should be updated.
    EXPECT_THAT(icing.SetSchema(schema, true).status(), ProtoIsOk());

    EXPECT_THAT(icing.GetSchema().schema().types(0).version(), Eq(2));
  }
}

TEST_F(IcingSearchEngineTest, SetSchemaNoChangeVersionUpdateSucceeds) {
  {
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

    // 1. Create a schema with an Email type with version 1
    SchemaProto schema;
    SchemaTypeConfigProto* type = schema.add_types();
    type->set_version(1);
    type->set_schema_type("Email");
    PropertyConfigProto* property = type->add_properties();
    property->set_property_name("title");
    property->set_data_type(PropertyConfigProto::DataType::STRING);
    property->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);

    EXPECT_THAT(icing.SetSchema(schema).status(), ProtoIsOk());

    EXPECT_THAT(icing.GetSchema().schema().types(0).version(), Eq(1));
  }

  {
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

    // 2. Create schema that only changes the version.
    SchemaProto schema;
    SchemaTypeConfigProto* type = schema.add_types();
    type->set_version(2);
    type->set_schema_type("Email");
    PropertyConfigProto* property = type->add_properties();
    property->set_property_name("title");
    property->set_data_type(PropertyConfigProto::DataType::STRING);
    property->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);

    // 3. SetSchema should succeed and the version number should be updated.
    EXPECT_THAT(icing.SetSchema(schema).status(), ProtoIsOk());

    EXPECT_THAT(icing.GetSchema().schema().types(0).version(), Eq(2));
  }
}

TEST_F(IcingSearchEngineTest, SetSchemaDuplicateTypesReturnsAlreadyExists) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

  // Create a schema with types { "Email", "Message" and "Email" }
  SchemaProto schema;
  SchemaTypeConfigProto* type = schema.add_types();
  type->set_schema_type("Email");
  PropertyConfigProto* property = type->add_properties();
  property->set_property_name("title");
  property->set_data_type(PropertyConfigProto::DataType::STRING);
  property->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);

  type = schema.add_types();
  type->set_schema_type("Message");
  property = type->add_properties();
  property->set_property_name("body");
  property->set_data_type(PropertyConfigProto::DataType::STRING);
  property->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);

  *schema.add_types() = schema.types(0);

  EXPECT_THAT(icing.SetSchema(schema).status(),
              ProtoStatusIs(StatusProto::ALREADY_EXISTS));
}

TEST_F(IcingSearchEngineTest,
       SetSchemaDuplicatePropertiesReturnsAlreadyExists) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

  // Create a schema with an Email type with properties { "title", "body" and
  // "title" }
  SchemaProto schema;
  SchemaTypeConfigProto* type = schema.add_types();
  type->set_schema_type("Email");
  PropertyConfigProto* property = type->add_properties();
  property->set_property_name("title");
  property->set_data_type(PropertyConfigProto::DataType::STRING);
  property->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);
  property = type->add_properties();
  property->set_property_name("body");
  property->set_data_type(PropertyConfigProto::DataType::STRING);
  property->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);
  property = type->add_properties();
  property->set_property_name("title");
  property->set_data_type(PropertyConfigProto::DataType::STRING);
  property->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);

  EXPECT_THAT(icing.SetSchema(schema).status(),
              ProtoStatusIs(StatusProto::ALREADY_EXISTS));
}

TEST_F(IcingSearchEngineTest, SetSchema) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

  auto message_document = CreateMessageDocument("namespace", "uri");

  auto schema_with_message = CreateMessageSchema();

  SchemaProto schema_with_email;
  SchemaTypeConfigProto* type = schema_with_email.add_types();
  type->set_schema_type("Email");
  PropertyConfigProto* property = type->add_properties();
  property->set_property_name("title");
  property->set_data_type(PropertyConfigProto::DataType::STRING);
  property->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);

  SchemaProto schema_with_email_and_message = schema_with_email;
  type = schema_with_email_and_message.add_types();
  type->set_schema_type("Message");
  property = type->add_properties();
  property->set_property_name("body");
  property->set_data_type(PropertyConfigProto::DataType::STRING);
  property->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);

  // Create an arbitrary invalid schema
  SchemaProto invalid_schema;
  SchemaTypeConfigProto* empty_type = invalid_schema.add_types();
  empty_type->set_schema_type("");

  // Make sure we can't set invalid schemas
  EXPECT_THAT(icing.SetSchema(invalid_schema).status(),
              ProtoStatusIs(StatusProto::INVALID_ARGUMENT));

  // Can add an document of a set schema
  EXPECT_THAT(icing.SetSchema(schema_with_message).status(), ProtoIsOk());
  EXPECT_THAT(icing.Put(message_document).status(), ProtoIsOk());

  // Schema with Email doesn't have Message, so would result incompatible
  // data
  EXPECT_THAT(icing.SetSchema(schema_with_email).status(),
              ProtoStatusIs(StatusProto::FAILED_PRECONDITION));

  // Can expand the set of schema types and add an document of a new
  // schema type
  EXPECT_THAT(icing.SetSchema(SchemaProto(schema_with_email_and_message))
                  .status()
                  .code(),
              Eq(StatusProto::OK));
  EXPECT_THAT(icing.Put(message_document).status(), ProtoIsOk());

  // Can't add an document whose schema isn't set
  auto photo_document = DocumentBuilder()
                            .SetKey("namespace", "uri")
                            .SetSchema("Photo")
                            .AddStringProperty("creator", "icing")
                            .Build();
  PutResultProto put_result_proto = icing.Put(photo_document);
  EXPECT_THAT(put_result_proto.status(), ProtoStatusIs(StatusProto::NOT_FOUND));
  EXPECT_THAT(put_result_proto.status().message(),
              HasSubstr("'Photo' not found"));
}

TEST_F(IcingSearchEngineTest,
       SetSchemaTriggersIndexRestorationAndReturnsOk) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

  SchemaProto schema_with_no_indexed_property = CreateMessageSchema();
  schema_with_no_indexed_property.mutable_types(0)
      ->mutable_properties(0)
      ->clear_string_indexing_config();

  EXPECT_THAT(icing.SetSchema(schema_with_no_indexed_property).status(),
              ProtoIsOk());
  // Nothing will be index and Search() won't return anything.
  EXPECT_THAT(icing.Put(CreateMessageDocument("namespace", "uri")).status(),
              ProtoIsOk());

  SearchSpecProto search_spec;
  search_spec.set_query("message");
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);

  SearchResultProto empty_result;
  empty_result.mutable_status()->set_code(StatusProto::OK);

  SearchResultProto actual_results =
      icing.Search(search_spec, GetDefaultScoringSpec(),
                   ResultSpecProto::default_instance());
  EXPECT_THAT(actual_results,
              EqualsSearchResultIgnoreStatsAndScores(empty_result));

  SchemaProto schema_with_indexed_property = CreateMessageSchema();
  // Index restoration should be triggered here because new schema requires more
  // properties to be indexed.
  EXPECT_THAT(icing.SetSchema(schema_with_indexed_property).status(),
              ProtoIsOk());

  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      CreateMessageDocument("namespace", "uri");
  actual_results = icing.Search(search_spec, GetDefaultScoringSpec(),
                                ResultSpecProto::default_instance());
  EXPECT_THAT(actual_results, EqualsSearchResultIgnoreStatsAndScores(
                                  expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest,
       SetSchemaChangeNestedPropertiesTriggersIndexRestorationAndReturnsOk) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

  SchemaTypeConfigProto person_proto =
      SchemaTypeConfigBuilder()
          .SetType("Person")
          .AddProperty(PropertyConfigBuilder()
                           .SetName("name")
                           .SetDataTypeString(MATCH_PREFIX, TOKENIZER_PLAIN)
                           .SetCardinality(CARDINALITY_OPTIONAL))
          .Build();
  SchemaProto nested_schema =
      SchemaBuilder()
          .AddType(person_proto)
          .AddType(SchemaTypeConfigBuilder()
                       .SetType("Email")
                       .AddProperty(PropertyConfigBuilder()
                                        .SetName("sender")
                                        .SetDataTypeDocument(
                                            "Person",
                                            /*index_nested_properties=*/true)
                                        .SetCardinality(CARDINALITY_OPTIONAL))
                       .AddProperty(
                           PropertyConfigBuilder()
                               .SetName("subject")
                               .SetDataTypeString(MATCH_PREFIX, TOKENIZER_PLAIN)
                               .SetCardinality(CARDINALITY_OPTIONAL)))
          .Build();

  SetSchemaResultProto set_schema_result = icing.SetSchema(nested_schema);
  SetSchemaResultProto expected_set_schema_result;
  expected_set_schema_result.mutable_status()->set_code(StatusProto::OK);
  EXPECT_THAT(set_schema_result, EqualsProto(expected_set_schema_result));

  DocumentProto document =
      DocumentBuilder()
          .SetKey("namespace1", "uri1")
          .SetSchema("Email")
          .SetCreationTimestampMs(1000)
          .AddStringProperty("subject",
                             "Did you get the memo about TPS reports?")
          .AddDocumentProperty("sender",
                               DocumentBuilder()
                                   .SetKey("namespace1", "uri1")
                                   .SetSchema("Person")
                                   .AddStringProperty("name", "Bill Lundbergh")
                                   .Build())
          .Build();

  // "sender.name" should get assigned property id 0 and subject should get
  // property id 1.
  EXPECT_THAT(icing.Put(document).status(), ProtoIsOk());

  // document should match a query for 'Bill' in 'sender.name', but not in
  // 'subject'
  SearchSpecProto search_spec;
  search_spec.set_query("sender.name:Bill");
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);

  SearchResultProto result;
  result.mutable_status()->set_code(StatusProto::OK);
  *result.mutable_results()->Add()->mutable_document() = document;

  SearchResultProto actual_results =
      icing.Search(search_spec, GetDefaultScoringSpec(),
                   ResultSpecProto::default_instance());
  EXPECT_THAT(actual_results, EqualsSearchResultIgnoreStatsAndScores(result));

  SearchResultProto empty_result;
  empty_result.mutable_status()->set_code(StatusProto::OK);
  search_spec.set_query("subject:Bill");
  actual_results = icing.Search(search_spec, GetDefaultScoringSpec(),
                                ResultSpecProto::default_instance());
  EXPECT_THAT(actual_results,
              EqualsSearchResultIgnoreStatsAndScores(empty_result));

  // Now update the schema with index_nested_properties=false. This should
  // reassign property ids, lead to an index rebuild and ensure that nothing
  // match a query for "Bill".
  SchemaProto no_nested_schema =
      SchemaBuilder()
          .AddType(person_proto)
          .AddType(SchemaTypeConfigBuilder()
                       .SetType("Email")
                       .AddProperty(PropertyConfigBuilder()
                                        .SetName("sender")
                                        .SetDataTypeDocument(
                                            "Person",
                                            /*index_nested_properties=*/false)
                                        .SetCardinality(CARDINALITY_OPTIONAL))
                       .AddProperty(
                           PropertyConfigBuilder()
                               .SetName("subject")
                               .SetDataTypeString(MATCH_PREFIX, TOKENIZER_PLAIN)
                               .SetCardinality(CARDINALITY_OPTIONAL)))
          .Build();

  set_schema_result = icing.SetSchema(no_nested_schema);
  expected_set_schema_result = SetSchemaResultProto();
  expected_set_schema_result.mutable_status()->set_code(StatusProto::OK);
  EXPECT_THAT(set_schema_result, EqualsProto(expected_set_schema_result));

  // document shouldn't match a query for 'Bill' in either 'sender.name' or
  // 'subject'
  search_spec.set_query("sender.name:Bill");
  actual_results = icing.Search(search_spec, GetDefaultScoringSpec(),
                                ResultSpecProto::default_instance());
  EXPECT_THAT(actual_results,
              EqualsSearchResultIgnoreStatsAndScores(empty_result));

  search_spec.set_query("subject:Bill");
  actual_results = icing.Search(search_spec, GetDefaultScoringSpec(),
                                ResultSpecProto::default_instance());
  EXPECT_THAT(actual_results,
              EqualsSearchResultIgnoreStatsAndScores(empty_result));
}

TEST_F(IcingSearchEngineTest,
       ForceSetSchemaPropertyDeletionTriggersIndexRestorationAndReturnsOk) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

  // 'body' should have a property id of 0 and 'subject' should have a property
  // id of 1.
  SchemaProto email_with_body_schema =
      SchemaBuilder()
          .AddType(SchemaTypeConfigBuilder()
                       .SetType("Email")
                       .AddProperty(
                           PropertyConfigBuilder()
                               .SetName("subject")
                               .SetDataTypeString(MATCH_PREFIX, TOKENIZER_PLAIN)
                               .SetCardinality(CARDINALITY_OPTIONAL))
                       .AddProperty(
                           PropertyConfigBuilder()
                               .SetName("body")
                               .SetDataTypeString(MATCH_PREFIX, TOKENIZER_PLAIN)
                               .SetCardinality(CARDINALITY_OPTIONAL)))
          .Build();

  SetSchemaResultProto set_schema_result =
      icing.SetSchema(email_with_body_schema);
  SetSchemaResultProto expected_set_schema_result;
  expected_set_schema_result.mutable_status()->set_code(StatusProto::OK);
  EXPECT_THAT(set_schema_result, EqualsProto(expected_set_schema_result));

  // Create a document with only a subject property.
  DocumentProto document =
      DocumentBuilder()
          .SetKey("namespace1", "uri1")
          .SetSchema("Email")
          .SetCreationTimestampMs(1000)
          .AddStringProperty("subject",
                             "Did you get the memo about TPS reports?")
          .Build();
  EXPECT_THAT(icing.Put(document).status(), ProtoIsOk());

  // We should be able to retrieve the document by searching for 'tps' in
  // 'subject'.
  SearchSpecProto search_spec;
  search_spec.set_query("subject:tps");
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);

  SearchResultProto result;
  result.mutable_status()->set_code(StatusProto::OK);
  *result.mutable_results()->Add()->mutable_document() = document;

  SearchResultProto actual_results =
      icing.Search(search_spec, GetDefaultScoringSpec(),
                   ResultSpecProto::default_instance());
  EXPECT_THAT(actual_results, EqualsSearchResultIgnoreStatsAndScores(result));

  // Now update the schema to remove the 'body' field. This is backwards
  // incompatible, but document should be preserved because it doesn't contain a
  // 'body' field. If the index is correctly rebuilt, then 'subject' will now
  // have a property id of 0. If not, then the hits in the index will still have
  // have a property id of 1 and therefore it won't be found.
  SchemaProto email_no_body_schema =
      SchemaBuilder()
          .AddType(SchemaTypeConfigBuilder().SetType("Email").AddProperty(
              PropertyConfigBuilder()
                  .SetName("subject")
                  .SetDataTypeString(MATCH_PREFIX, TOKENIZER_PLAIN)
                  .SetCardinality(CARDINALITY_OPTIONAL)))
          .Build();

  set_schema_result = icing.SetSchema(
      email_no_body_schema, /*ignore_errors_and_delete_documents=*/true);
  expected_set_schema_result = SetSchemaResultProto();
  expected_set_schema_result.mutable_incompatible_schema_types()->Add("Email");
  expected_set_schema_result.mutable_status()->set_code(StatusProto::OK);
  EXPECT_THAT(set_schema_result, EqualsProto(expected_set_schema_result));

  // We should be able to retrieve the document by searching for 'tps' in
  // 'subject'.
  search_spec.set_query("subject:tps");
  actual_results = icing.Search(search_spec, GetDefaultScoringSpec(),
                                ResultSpecProto::default_instance());
  EXPECT_THAT(actual_results, EqualsSearchResultIgnoreStatsAndScores(result));
}

TEST_F(
    IcingSearchEngineTest,
    ForceSetSchemaPropertyDeletionAndAdditionTriggersIndexRestorationAndReturnsOk) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

  // 'body' should have a property id of 0 and 'subject' should have a property
  // id of 1.
  SchemaProto email_with_body_schema =
      SchemaBuilder()
          .AddType(SchemaTypeConfigBuilder()
                       .SetType("Email")
                       .AddProperty(
                           PropertyConfigBuilder()
                               .SetName("subject")
                               .SetDataTypeString(MATCH_PREFIX, TOKENIZER_PLAIN)
                               .SetCardinality(CARDINALITY_OPTIONAL))
                       .AddProperty(
                           PropertyConfigBuilder()
                               .SetName("body")
                               .SetDataTypeString(MATCH_PREFIX, TOKENIZER_PLAIN)
                               .SetCardinality(CARDINALITY_OPTIONAL)))
          .Build();

  SetSchemaResultProto set_schema_result =
      icing.SetSchema(email_with_body_schema);
  SetSchemaResultProto expected_set_schema_result;
  expected_set_schema_result.mutable_status()->set_code(StatusProto::OK);
  EXPECT_THAT(set_schema_result, EqualsProto(expected_set_schema_result));

  // Create a document with only a subject property.
  DocumentProto document =
      DocumentBuilder()
          .SetKey("namespace1", "uri1")
          .SetSchema("Email")
          .SetCreationTimestampMs(1000)
          .AddStringProperty("subject",
                             "Did you get the memo about TPS reports?")
          .Build();
  EXPECT_THAT(icing.Put(document).status(), ProtoIsOk());

  // We should be able to retrieve the document by searching for 'tps' in
  // 'subject'.
  SearchSpecProto search_spec;
  search_spec.set_query("subject:tps");
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);

  SearchResultProto result;
  result.mutable_status()->set_code(StatusProto::OK);
  *result.mutable_results()->Add()->mutable_document() = document;

  SearchResultProto actual_results =
      icing.Search(search_spec, GetDefaultScoringSpec(),
                   ResultSpecProto::default_instance());
  EXPECT_THAT(actual_results, EqualsSearchResultIgnoreStatsAndScores(result));

  // Now update the schema to remove the 'body' field. This is backwards
  // incompatible, but document should be preserved because it doesn't contain a
  // 'body' field. If the index is correctly rebuilt, then 'subject' and 'to'
  // will now have property ids of 0 and 1 respectively. If not, then the hits
  // in the index will still have have a property id of 1 and therefore it won't
  // be found.
  SchemaProto email_no_body_schema =
      SchemaBuilder()
          .AddType(SchemaTypeConfigBuilder()
                       .SetType("Email")
                       .AddProperty(
                           PropertyConfigBuilder()
                               .SetName("subject")
                               .SetDataTypeString(MATCH_PREFIX, TOKENIZER_PLAIN)
                               .SetCardinality(CARDINALITY_OPTIONAL))
                       .AddProperty(
                           PropertyConfigBuilder()
                               .SetName("to")
                               .SetDataTypeString(MATCH_PREFIX, TOKENIZER_PLAIN)
                               .SetCardinality(CARDINALITY_OPTIONAL)))
          .Build();

  set_schema_result = icing.SetSchema(
      email_no_body_schema, /*ignore_errors_and_delete_documents=*/true);
  expected_set_schema_result = SetSchemaResultProto();
  expected_set_schema_result.mutable_incompatible_schema_types()->Add("Email");
  expected_set_schema_result.mutable_status()->set_code(StatusProto::OK);
  EXPECT_THAT(set_schema_result, EqualsProto(expected_set_schema_result));

  // We should be able to retrieve the document by searching for 'tps' in
  // 'subject'.
  search_spec.set_query("subject:tps");
  actual_results = icing.Search(search_spec, GetDefaultScoringSpec(),
                                ResultSpecProto::default_instance());
  EXPECT_THAT(actual_results, EqualsSearchResultIgnoreStatsAndScores(result));
}

TEST_F(IcingSearchEngineTest, ForceSetSchemaIncompatibleNestedDocsAreDeleted) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

  SchemaTypeConfigProto email_schema_type =
      SchemaTypeConfigBuilder()
          .SetType("Email")
          .AddProperty(
              PropertyConfigBuilder()
                  .SetName("sender")
                  .SetDataTypeDocument("Person",
                                       /*index_nested_properties=*/true)
                  .SetCardinality(CARDINALITY_OPTIONAL))
          .AddProperty(PropertyConfigBuilder()
                           .SetName("subject")
                           .SetDataTypeString(MATCH_PREFIX, TOKENIZER_PLAIN)
                           .SetCardinality(CARDINALITY_OPTIONAL))
          .Build();
  SchemaProto nested_schema =
      SchemaBuilder()
          .AddType(SchemaTypeConfigBuilder()
                       .SetType("Person")
                       .AddProperty(
                           PropertyConfigBuilder()
                               .SetName("name")
                               .SetDataTypeString(MATCH_PREFIX, TOKENIZER_PLAIN)
                               .SetCardinality(CARDINALITY_OPTIONAL))
                       .AddProperty(
                           PropertyConfigBuilder()
                               .SetName("company")
                               .SetDataTypeString(MATCH_PREFIX, TOKENIZER_PLAIN)
                               .SetCardinality(CARDINALITY_OPTIONAL)))
          .AddType(email_schema_type)
          .Build();

  SetSchemaResultProto set_schema_result = icing.SetSchema(nested_schema);
  SetSchemaResultProto expected_set_schema_result;
  expected_set_schema_result.mutable_status()->set_code(StatusProto::OK);
  EXPECT_THAT(set_schema_result, EqualsProto(expected_set_schema_result));

  // Create two documents - a person document and an email document - both docs
  // should be deleted when we remove the 'company' field from the person type.
  DocumentProto person_document =
      DocumentBuilder()
          .SetKey("namespace1", "uri1")
          .SetSchema("Person")
          .SetCreationTimestampMs(1000)
          .AddStringProperty("name", "Bill Lundbergh")
          .AddStringProperty("company", "Initech Corp.")
          .Build();
  EXPECT_THAT(icing.Put(person_document).status(), ProtoIsOk());

  DocumentProto email_document =
      DocumentBuilder()
          .SetKey("namespace1", "uri2")
          .SetSchema("Email")
          .SetCreationTimestampMs(1000)
          .AddStringProperty("subject",
                             "Did you get the memo about TPS reports?")
          .AddDocumentProperty("sender", person_document)
          .Build();
  EXPECT_THAT(icing.Put(email_document).status(), ProtoIsOk());

  // We should be able to retrieve both documents.
  GetResultProto get_result =
      icing.Get("namespace1", "uri1", GetResultSpecProto::default_instance());
  EXPECT_THAT(get_result.status(), ProtoIsOk());
  EXPECT_THAT(get_result.document(), EqualsProto(person_document));

  get_result =
      icing.Get("namespace1", "uri2", GetResultSpecProto::default_instance());
  EXPECT_THAT(get_result.status(), ProtoIsOk());
  EXPECT_THAT(get_result.document(), EqualsProto(email_document));

  // Now update the schema to remove the 'company' field. This is backwards
  // incompatible, *both* documents should be deleted because both fail
  // validation (they each contain a 'Person' that has a non-existent property).
  nested_schema =
      SchemaBuilder()
          .AddType(SchemaTypeConfigBuilder().SetType("Person").AddProperty(
              PropertyConfigBuilder()
                  .SetName("name")
                  .SetDataTypeString(MATCH_PREFIX, TOKENIZER_PLAIN)
                  .SetCardinality(CARDINALITY_OPTIONAL)))
          .AddType(email_schema_type)
          .Build();

  set_schema_result = icing.SetSchema(
      nested_schema, /*ignore_errors_and_delete_documents=*/true);
  expected_set_schema_result = SetSchemaResultProto();
  expected_set_schema_result.mutable_incompatible_schema_types()->Add("Person");
  expected_set_schema_result.mutable_incompatible_schema_types()->Add("Email");
  expected_set_schema_result.mutable_status()->set_code(StatusProto::OK);
  EXPECT_THAT(set_schema_result, EqualsProto(expected_set_schema_result));

  // Both documents should be deleted now.
  get_result =
      icing.Get("namespace1", "uri1", GetResultSpecProto::default_instance());
  EXPECT_THAT(get_result.status(), ProtoStatusIs(StatusProto::NOT_FOUND));

  get_result =
      icing.Get("namespace1", "uri2", GetResultSpecProto::default_instance());
  EXPECT_THAT(get_result.status(), ProtoStatusIs(StatusProto::NOT_FOUND));
}

TEST_F(IcingSearchEngineTest, SetSchemaRevalidatesDocumentsAndReturnsOk) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

  SchemaProto schema_with_optional_subject;
  auto type = schema_with_optional_subject.add_types();
  type->set_schema_type("email");

  // Add a OPTIONAL property
  auto property = type->add_properties();
  property->set_property_name("subject");
  property->set_data_type(PropertyConfigProto::DataType::STRING);
  property->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);

  EXPECT_THAT(icing.SetSchema(schema_with_optional_subject).status(),
              ProtoIsOk());

  DocumentProto email_document_without_subject =
      DocumentBuilder()
          .SetKey("namespace", "without_subject")
          .SetSchema("email")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  DocumentProto email_document_with_subject =
      DocumentBuilder()
          .SetKey("namespace", "with_subject")
          .SetSchema("email")
          .AddStringProperty("subject", "foo")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();

  EXPECT_THAT(icing.Put(email_document_without_subject).status(), ProtoIsOk());
  EXPECT_THAT(icing.Put(email_document_with_subject).status(), ProtoIsOk());

  SchemaProto schema_with_required_subject;
  type = schema_with_required_subject.add_types();
  type->set_schema_type("email");

  // Add a REQUIRED property
  property = type->add_properties();
  property->set_property_name("subject");
  property->set_data_type(PropertyConfigProto::DataType::STRING);
  property->set_cardinality(PropertyConfigProto::Cardinality::REQUIRED);

  // Can't set the schema since it's incompatible
  SetSchemaResultProto expected_set_schema_result_proto;
  expected_set_schema_result_proto.mutable_status()->set_code(
      StatusProto::FAILED_PRECONDITION);
  expected_set_schema_result_proto.mutable_status()->set_message(
      "Schema is incompatible.");
  expected_set_schema_result_proto.add_incompatible_schema_types("email");

  EXPECT_THAT(icing.SetSchema(schema_with_required_subject),
              EqualsProto(expected_set_schema_result_proto));

  // Force set it
  expected_set_schema_result_proto.mutable_status()->set_code(StatusProto::OK);
  expected_set_schema_result_proto.mutable_status()->clear_message();
  EXPECT_THAT(icing.SetSchema(schema_with_required_subject,
                              /*ignore_errors_and_delete_documents=*/true),
              EqualsProto(expected_set_schema_result_proto));

  GetResultProto expected_get_result_proto;
  expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_get_result_proto.mutable_document() = email_document_with_subject;

  EXPECT_THAT(icing.Get("namespace", "with_subject",
                        GetResultSpecProto::default_instance()),
              EqualsProto(expected_get_result_proto));

  // The document without a subject got deleted because it failed validation
  // against the new schema
  expected_get_result_proto.mutable_status()->set_code(StatusProto::NOT_FOUND);
  expected_get_result_proto.mutable_status()->set_message(
      "Document (namespace, without_subject) not found.");
  expected_get_result_proto.clear_document();

  EXPECT_THAT(icing.Get("namespace", "without_subject",
                        GetResultSpecProto::default_instance()),
              EqualsProto(expected_get_result_proto));
}

TEST_F(IcingSearchEngineTest, SetSchemaDeletesDocumentsAndReturnsOk) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

  SchemaProto schema;
  auto type = schema.add_types();
  type->set_schema_type("email");
  type = schema.add_types();
  type->set_schema_type("message");

  EXPECT_THAT(icing.SetSchema(schema).status(), ProtoIsOk());

  DocumentProto email_document =
      DocumentBuilder()
          .SetKey("namespace", "email_uri")
          .SetSchema("email")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  DocumentProto message_document =
      DocumentBuilder()
          .SetKey("namespace", "message_uri")
          .SetSchema("message")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();

  EXPECT_THAT(icing.Put(email_document).status(), ProtoIsOk());
  EXPECT_THAT(icing.Put(message_document).status(), ProtoIsOk());

  // Clear the schema and only add the "email" type, essentially deleting the
  // "message" type
  SchemaProto new_schema;
  type = new_schema.add_types();
  type->set_schema_type("email");

  // Can't set the schema since it's incompatible
  SetSchemaResultProto expected_result;
  expected_result.mutable_status()->set_code(StatusProto::FAILED_PRECONDITION);
  expected_result.mutable_status()->set_message("Schema is incompatible.");
  expected_result.add_deleted_schema_types("message");

  EXPECT_THAT(icing.SetSchema(new_schema), EqualsProto(expected_result));

  // Force set it
  expected_result.mutable_status()->set_code(StatusProto::OK);
  expected_result.mutable_status()->clear_message();
  EXPECT_THAT(icing.SetSchema(new_schema,
                              /*ignore_errors_and_delete_documents=*/true),
              EqualsProto(expected_result));

  // "email" document is still there
  GetResultProto expected_get_result_proto;
  expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_get_result_proto.mutable_document() = email_document;

  EXPECT_THAT(icing.Get("namespace", "email_uri",
                        GetResultSpecProto::default_instance()),
              EqualsProto(expected_get_result_proto));

  // "message" document got deleted
  expected_get_result_proto.mutable_status()->set_code(StatusProto::NOT_FOUND);
  expected_get_result_proto.mutable_status()->set_message(
      "Document (namespace, message_uri) not found.");
  expected_get_result_proto.clear_document();

  EXPECT_THAT(icing.Get("namespace", "message_uri",
                        GetResultSpecProto::default_instance()),
              EqualsProto(expected_get_result_proto));
}

TEST_F(IcingSearchEngineTest, GetSchemaNotFound) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

  EXPECT_THAT(icing.GetSchema().status(),
              ProtoStatusIs(StatusProto::NOT_FOUND));
}

TEST_F(IcingSearchEngineTest, GetSchemaOk) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

  EXPECT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  GetSchemaResultProto expected_get_schema_result_proto;
  expected_get_schema_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_get_schema_result_proto.mutable_schema() = CreateMessageSchema();
  EXPECT_THAT(icing.GetSchema(), EqualsProto(expected_get_schema_result_proto));
}

TEST_F(IcingSearchEngineTest, GetSchemaTypeFailedPrecondition) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

  GetSchemaTypeResultProto get_schema_type_result_proto =
      icing.GetSchemaType("nonexistent_schema");
  EXPECT_THAT(get_schema_type_result_proto.status(),
              ProtoStatusIs(StatusProto::FAILED_PRECONDITION));
  EXPECT_THAT(get_schema_type_result_proto.status().message(),
              HasSubstr("Schema not set"));
}

TEST_F(IcingSearchEngineTest, GetSchemaTypeOk) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

  EXPECT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  GetSchemaTypeResultProto expected_get_schema_type_result_proto;
  expected_get_schema_type_result_proto.mutable_status()->set_code(
      StatusProto::OK);
  *expected_get_schema_type_result_proto.mutable_schema_type_config() =
      CreateMessageSchema().types(0);
  EXPECT_THAT(icing.GetSchemaType(CreateMessageSchema().types(0).schema_type()),
              EqualsProto(expected_get_schema_type_result_proto));
}

TEST_F(IcingSearchEngineTest, GetDocument) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  // Simple put and get
  ASSERT_THAT(icing.Put(CreateMessageDocument("namespace", "uri")).status(),
              ProtoIsOk());

  GetResultProto expected_get_result_proto;
  expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_get_result_proto.mutable_document() =
      CreateMessageDocument("namespace", "uri");
  ASSERT_THAT(
      icing.Get("namespace", "uri", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  // Put an invalid document
  PutResultProto put_result_proto = icing.Put(DocumentProto());
  EXPECT_THAT(put_result_proto.status(),
              ProtoStatusIs(StatusProto::INVALID_ARGUMENT));
  EXPECT_THAT(put_result_proto.status().message(),
              HasSubstr("'namespace' is empty"));

  // Get a non-existing key
  expected_get_result_proto.mutable_status()->set_code(StatusProto::NOT_FOUND);
  expected_get_result_proto.mutable_status()->set_message(
      "Document (wrong, uri) not found.");
  expected_get_result_proto.clear_document();
  ASSERT_THAT(icing.Get("wrong", "uri", GetResultSpecProto::default_instance()),
              EqualsProto(expected_get_result_proto));
}

TEST_F(IcingSearchEngineTest, GetDocumentProjectionEmpty) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  DocumentProto document = CreateMessageDocument("namespace", "uri");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());

  GetResultSpecProto result_spec;
  TypePropertyMask* mask = result_spec.add_type_property_masks();
  mask->set_schema_type(document.schema());
  mask->add_paths("");

  GetResultProto expected_get_result_proto;
  expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_get_result_proto.mutable_document() = document;
  expected_get_result_proto.mutable_document()->clear_properties();
  ASSERT_THAT(icing.Get("namespace", "uri", result_spec),
              EqualsProto(expected_get_result_proto));
}

TEST_F(IcingSearchEngineTest, GetDocumentWildCardProjectionEmpty) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  DocumentProto document = CreateMessageDocument("namespace", "uri");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());

  GetResultSpecProto result_spec;
  TypePropertyMask* mask = result_spec.add_type_property_masks();
  mask->set_schema_type("*");
  mask->add_paths("");

  GetResultProto expected_get_result_proto;
  expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_get_result_proto.mutable_document() = document;
  expected_get_result_proto.mutable_document()->clear_properties();
  ASSERT_THAT(icing.Get("namespace", "uri", result_spec),
              EqualsProto(expected_get_result_proto));
}

TEST_F(IcingSearchEngineTest, GetDocumentProjectionMultipleFieldPaths) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreatePersonAndEmailSchema()).status(),
              ProtoIsOk());

  // 1. Add an email document
  DocumentProto document =
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
          .AddStringProperty("subject", "Hello World!")
          .AddStringProperty(
              "body", "Oh what a beautiful morning! Oh what a beautiful day!")
          .Build();
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());

  GetResultSpecProto result_spec;
  TypePropertyMask* mask = result_spec.add_type_property_masks();
  mask->set_schema_type("Email");
  mask->add_paths("sender.name");
  mask->add_paths("subject");

  // 2. Verify that the returned result only contains the 'sender.name'
  // property and the 'subject' property.
  GetResultProto expected_get_result_proto;
  expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_get_result_proto.mutable_document() =
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
          .AddStringProperty("subject", "Hello World!")
          .Build();
  ASSERT_THAT(icing.Get("namespace", "uri1", result_spec),
              EqualsProto(expected_get_result_proto));
}

TEST_F(IcingSearchEngineTest, GetDocumentWildcardProjectionMultipleFieldPaths) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreatePersonAndEmailSchema()).status(),
              ProtoIsOk());

  // 1. Add an email document
  DocumentProto document =
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
          .AddStringProperty("subject", "Hello World!")
          .AddStringProperty(
              "body", "Oh what a beautiful morning! Oh what a beautiful day!")
          .Build();
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());

  GetResultSpecProto result_spec;
  TypePropertyMask* mask = result_spec.add_type_property_masks();
  mask->set_schema_type("*");
  mask->add_paths("sender.name");
  mask->add_paths("subject");

  // 2. Verify that the returned result only contains the 'sender.name'
  // property and the 'subject' property.
  GetResultProto expected_get_result_proto;
  expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_get_result_proto.mutable_document() =
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
          .AddStringProperty("subject", "Hello World!")
          .Build();
  ASSERT_THAT(icing.Get("namespace", "uri1", result_spec),
              EqualsProto(expected_get_result_proto));
}

TEST_F(IcingSearchEngineTest,
       GetDocumentSpecificProjectionOverridesWildcardProjection) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreatePersonAndEmailSchema()).status(),
              ProtoIsOk());

  // 1. Add an email document
  DocumentProto document =
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
          .AddStringProperty("subject", "Hello World!")
          .AddStringProperty(
              "body", "Oh what a beautiful morning! Oh what a beautiful day!")
          .Build();
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());

  // 2. Add type property masks for the wildcard and the specific type of the
  // document 'Email'. The wildcard should be ignored and only the 'Email'
  // projection should apply.
  GetResultSpecProto result_spec;
  TypePropertyMask* mask = result_spec.add_type_property_masks();
  mask->set_schema_type("*");
  mask->add_paths("subject");
  mask = result_spec.add_type_property_masks();
  mask->set_schema_type("Email");
  mask->add_paths("body");

  // 3. Verify that the returned result only contains the 'body' property.
  GetResultProto expected_get_result_proto;
  expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_get_result_proto.mutable_document() =
      DocumentBuilder()
          .SetKey("namespace", "uri1")
          .SetCreationTimestampMs(1000)
          .SetSchema("Email")
          .AddStringProperty(
              "body", "Oh what a beautiful morning! Oh what a beautiful day!")
          .Build();
  ASSERT_THAT(icing.Get("namespace", "uri1", result_spec),
              EqualsProto(expected_get_result_proto));
}

TEST_F(IcingSearchEngineTest, SearchReturnsValidResults) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  DocumentProto document_one = CreateMessageDocument("namespace", "uri1");
  ASSERT_THAT(icing.Put(document_one).status(), ProtoIsOk());

  DocumentProto document_two = CreateMessageDocument("namespace", "uri2");
  ASSERT_THAT(icing.Put(document_two).status(), ProtoIsOk());

  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  search_spec.set_query("message");

  ResultSpecProto result_spec;
  result_spec.mutable_snippet_spec()->set_max_window_bytes(64);
  result_spec.mutable_snippet_spec()->set_num_matches_per_property(1);
  result_spec.mutable_snippet_spec()->set_num_to_snippet(1);

  SearchResultProto results =
      icing.Search(search_spec, GetDefaultScoringSpec(), result_spec);
  EXPECT_THAT(results.status(), ProtoIsOk());
  EXPECT_THAT(results.results(), SizeIs(2));

  const DocumentProto& document = results.results(0).document();
  EXPECT_THAT(document, EqualsProto(document_two));

  const SnippetProto& snippet = results.results(0).snippet();
  EXPECT_THAT(snippet.entries(), SizeIs(1));
  EXPECT_THAT(snippet.entries(0).property_name(), Eq("body"));
  std::string_view content =
      GetString(&document, snippet.entries(0).property_name());
  EXPECT_THAT(GetWindows(content, snippet.entries(0)),
              ElementsAre("message body"));
  EXPECT_THAT(GetMatches(content, snippet.entries(0)), ElementsAre("message"));

  EXPECT_THAT(results.results(1).document(), EqualsProto(document_one));
  EXPECT_THAT(results.results(1).snippet().entries(), IsEmpty());

  search_spec.set_query("foo");

  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  SearchResultProto actual_results =
      icing.Search(search_spec, GetDefaultScoringSpec(),
                   ResultSpecProto::default_instance());
  EXPECT_THAT(actual_results, EqualsSearchResultIgnoreStatsAndScores(
                                  expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, SearchReturnsScoresDocumentScore) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  DocumentProto document_one = CreateMessageDocument("namespace", "uri1");
  document_one.set_score(93);
  document_one.set_creation_timestamp_ms(10000);
  ASSERT_THAT(icing.Put(document_one).status(), ProtoIsOk());

  DocumentProto document_two = CreateMessageDocument("namespace", "uri2");
  document_two.set_score(15);
  document_two.set_creation_timestamp_ms(12000);
  ASSERT_THAT(icing.Put(document_two).status(), ProtoIsOk());

  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  search_spec.set_query("message");

  // Rank by DOCUMENT_SCORE and ensure that the score field is populated with
  // document score.
  ScoringSpecProto scoring_spec;
  scoring_spec.set_rank_by(ScoringSpecProto::RankingStrategy::DOCUMENT_SCORE);

  SearchResultProto results = icing.Search(search_spec, scoring_spec,
                                           ResultSpecProto::default_instance());
  EXPECT_THAT(results.status(), ProtoIsOk());
  EXPECT_THAT(results.results(), SizeIs(2));

  EXPECT_THAT(results.results(0).document(), EqualsProto(document_one));
  EXPECT_THAT(results.results(0).score(), 93);
  EXPECT_THAT(results.results(1).document(), EqualsProto(document_two));
  EXPECT_THAT(results.results(1).score(), 15);
}

TEST_F(IcingSearchEngineTest, SearchReturnsScoresCreationTimestamp) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  DocumentProto document_one = CreateMessageDocument("namespace", "uri1");
  document_one.set_score(93);
  document_one.set_creation_timestamp_ms(10000);
  ASSERT_THAT(icing.Put(document_one).status(), ProtoIsOk());

  DocumentProto document_two = CreateMessageDocument("namespace", "uri2");
  document_two.set_score(15);
  document_two.set_creation_timestamp_ms(12000);
  ASSERT_THAT(icing.Put(document_two).status(), ProtoIsOk());

  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  search_spec.set_query("message");

  // Rank by CREATION_TS and ensure that the score field is populated with
  // creation ts.
  ScoringSpecProto scoring_spec;
  scoring_spec.set_rank_by(
      ScoringSpecProto::RankingStrategy::CREATION_TIMESTAMP);

  SearchResultProto results = icing.Search(search_spec, scoring_spec,
                                           ResultSpecProto::default_instance());
  EXPECT_THAT(results.status(), ProtoIsOk());
  EXPECT_THAT(results.results(), SizeIs(2));

  EXPECT_THAT(results.results(0).document(), EqualsProto(document_two));
  EXPECT_THAT(results.results(0).score(), 12000);
  EXPECT_THAT(results.results(1).document(), EqualsProto(document_one));
  EXPECT_THAT(results.results(1).score(), 10000);
}

TEST_F(IcingSearchEngineTest, SearchReturnsOneResult) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  DocumentProto document_one = CreateMessageDocument("namespace", "uri1");
  ASSERT_THAT(icing.Put(document_one).status(), ProtoIsOk());

  DocumentProto document_two = CreateMessageDocument("namespace", "uri2");
  ASSERT_THAT(icing.Put(document_two).status(), ProtoIsOk());

  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  search_spec.set_query("message");

  ResultSpecProto result_spec;
  result_spec.set_num_per_page(1);

  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document_two;

  SearchResultProto search_result_proto =
      icing.Search(search_spec, GetDefaultScoringSpec(), result_spec);
  EXPECT_THAT(search_result_proto.status(), ProtoIsOk());
  // The token is a random number so we don't verify it.
  expected_search_result_proto.set_next_page_token(
      search_result_proto.next_page_token());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, SearchZeroResultLimitReturnsEmptyResults) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  search_spec.set_query("");

  ResultSpecProto result_spec;
  result_spec.set_num_per_page(0);

  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  SearchResultProto actual_results =
      icing.Search(search_spec, GetDefaultScoringSpec(), result_spec);
  EXPECT_THAT(actual_results, EqualsSearchResultIgnoreStatsAndScores(
                                  expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, SearchNegativeResultLimitReturnsInvalidArgument) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  search_spec.set_query("");

  ResultSpecProto result_spec;
  result_spec.set_num_per_page(-5);

  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(
      StatusProto::INVALID_ARGUMENT);
  expected_search_result_proto.mutable_status()->set_message(
      "ResultSpecProto.num_per_page cannot be negative.");
  SearchResultProto actual_results =
      icing.Search(search_spec, GetDefaultScoringSpec(), result_spec);
  EXPECT_THAT(actual_results, EqualsSearchResultIgnoreStatsAndScores(
                                  expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, SearchWithPersistenceReturnsValidResults) {
  IcingSearchEngineOptions icing_options = GetDefaultIcingOptions();

  {
    // Set the schema up beforehand.
    IcingSearchEngine icing(icing_options, GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
    ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
    // Schema will be persisted to disk when icing goes out of scope.
  }

  {
    // Ensure that icing initializes the schema and section_manager
    // properly from the pre-existing file.
    IcingSearchEngine icing(icing_options, GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

    EXPECT_THAT(icing.Put(CreateMessageDocument("namespace", "uri")).status(),
                ProtoIsOk());
    // The index and document store will be persisted to disk when icing goes
    // out of scope.
  }

  {
    // Ensure that the index is brought back up without problems and we
    // can query for the content that we expect.
    IcingSearchEngine icing(icing_options, GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

    SearchSpecProto search_spec;
    search_spec.set_term_match_type(TermMatchType::PREFIX);
    search_spec.set_query("message");

    SearchResultProto expected_search_result_proto;
    expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
    *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
        CreateMessageDocument("namespace", "uri");

    SearchResultProto actual_results =
        icing.Search(search_spec, GetDefaultScoringSpec(),
                     ResultSpecProto::default_instance());
    EXPECT_THAT(actual_results, EqualsSearchResultIgnoreStatsAndScores(
                                    expected_search_result_proto));

    search_spec.set_query("foo");

    SearchResultProto empty_result;
    empty_result.mutable_status()->set_code(StatusProto::OK);
    actual_results = icing.Search(search_spec, GetDefaultScoringSpec(),
                                  ResultSpecProto::default_instance());
    EXPECT_THAT(actual_results,
                EqualsSearchResultIgnoreStatsAndScores(empty_result));
  }
}

TEST_F(IcingSearchEngineTest, SearchShouldReturnEmpty) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  search_spec.set_query("message");

  // Empty result, no next-page token
  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);

  SearchResultProto search_result_proto =
      icing.Search(search_spec, GetDefaultScoringSpec(),
                   ResultSpecProto::default_instance());

  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, SearchShouldReturnMultiplePages) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  // Creates and inserts 5 documents
  DocumentProto document1 = CreateMessageDocument("namespace", "uri1");
  DocumentProto document2 = CreateMessageDocument("namespace", "uri2");
  DocumentProto document3 = CreateMessageDocument("namespace", "uri3");
  DocumentProto document4 = CreateMessageDocument("namespace", "uri4");
  DocumentProto document5 = CreateMessageDocument("namespace", "uri5");
  ASSERT_THAT(icing.Put(document1).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document2).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document3).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document4).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document5).status(), ProtoIsOk());

  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  search_spec.set_query("message");

  ResultSpecProto result_spec;
  result_spec.set_num_per_page(2);

  // Searches and gets the first page, 2 results
  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document5;
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document4;
  SearchResultProto search_result_proto =
      icing.Search(search_spec, GetDefaultScoringSpec(), result_spec);
  EXPECT_THAT(search_result_proto.next_page_token(), Gt(kInvalidNextPageToken));
  uint64_t next_page_token = search_result_proto.next_page_token();
  // Since the token is a random number, we don't need to verify
  expected_search_result_proto.set_next_page_token(next_page_token);
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));

  // Second page, 2 results
  expected_search_result_proto.clear_results();
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document3;
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document2;
  search_result_proto = icing.GetNextPage(next_page_token);
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));

  // Third page, 1 result
  expected_search_result_proto.clear_results();
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document1;
  // Because there are no more results, we should not return the next page
  // token.
  expected_search_result_proto.clear_next_page_token();
  search_result_proto = icing.GetNextPage(next_page_token);
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));

  // No more results
  expected_search_result_proto.clear_results();
  search_result_proto = icing.GetNextPage(next_page_token);
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, SearchWithNoScoringShouldReturnMultiplePages) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  // Creates and inserts 5 documents
  DocumentProto document1 = CreateMessageDocument("namespace", "uri1");
  DocumentProto document2 = CreateMessageDocument("namespace", "uri2");
  DocumentProto document3 = CreateMessageDocument("namespace", "uri3");
  DocumentProto document4 = CreateMessageDocument("namespace", "uri4");
  DocumentProto document5 = CreateMessageDocument("namespace", "uri5");
  ASSERT_THAT(icing.Put(document1).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document2).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document3).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document4).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document5).status(), ProtoIsOk());

  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  search_spec.set_query("message");

  ScoringSpecProto scoring_spec;
  scoring_spec.set_rank_by(ScoringSpecProto::RankingStrategy::NONE);

  ResultSpecProto result_spec;
  result_spec.set_num_per_page(2);

  // Searches and gets the first page, 2 results
  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document5;
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document4;
  SearchResultProto search_result_proto =
      icing.Search(search_spec, scoring_spec, result_spec);
  EXPECT_THAT(search_result_proto.next_page_token(), Gt(kInvalidNextPageToken));
  uint64_t next_page_token = search_result_proto.next_page_token();
  // Since the token is a random number, we don't need to verify
  expected_search_result_proto.set_next_page_token(next_page_token);
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));

  // Second page, 2 results
  expected_search_result_proto.clear_results();
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document3;
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document2;
  search_result_proto = icing.GetNextPage(next_page_token);
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));

  // Third page, 1 result
  expected_search_result_proto.clear_results();
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document1;
  // Because there are no more results, we should not return the next page
  // token.
  expected_search_result_proto.clear_next_page_token();
  search_result_proto = icing.GetNextPage(next_page_token);
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));

  // No more results
  expected_search_result_proto.clear_results();
  search_result_proto = icing.GetNextPage(next_page_token);
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, ShouldReturnMultiplePagesWithSnippets) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  // Creates and inserts 5 documents
  DocumentProto document1 = CreateMessageDocument("namespace", "uri1");
  DocumentProto document2 = CreateMessageDocument("namespace", "uri2");
  DocumentProto document3 = CreateMessageDocument("namespace", "uri3");
  DocumentProto document4 = CreateMessageDocument("namespace", "uri4");
  DocumentProto document5 = CreateMessageDocument("namespace", "uri5");
  ASSERT_THAT(icing.Put(document1).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document2).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document3).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document4).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document5).status(), ProtoIsOk());

  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  search_spec.set_query("message");

  ResultSpecProto result_spec;
  result_spec.set_num_per_page(2);
  result_spec.mutable_snippet_spec()->set_max_window_bytes(64);
  result_spec.mutable_snippet_spec()->set_num_matches_per_property(1);
  result_spec.mutable_snippet_spec()->set_num_to_snippet(3);

  // Searches and gets the first page, 2 results with 2 snippets
  SearchResultProto search_result =
      icing.Search(search_spec, GetDefaultScoringSpec(), result_spec);
  ASSERT_THAT(search_result.status(), ProtoIsOk());
  ASSERT_THAT(search_result.results(), SizeIs(2));
  ASSERT_THAT(search_result.next_page_token(), Gt(kInvalidNextPageToken));

  const DocumentProto& document_result_1 = search_result.results(0).document();
  EXPECT_THAT(document_result_1, EqualsProto(document5));
  const SnippetProto& snippet_result_1 = search_result.results(0).snippet();
  EXPECT_THAT(snippet_result_1.entries(), SizeIs(1));
  EXPECT_THAT(snippet_result_1.entries(0).property_name(), Eq("body"));
  std::string_view content = GetString(
      &document_result_1, snippet_result_1.entries(0).property_name());
  EXPECT_THAT(GetWindows(content, snippet_result_1.entries(0)),
              ElementsAre("message body"));
  EXPECT_THAT(GetMatches(content, snippet_result_1.entries(0)),
              ElementsAre("message"));

  const DocumentProto& document_result_2 = search_result.results(1).document();
  EXPECT_THAT(document_result_2, EqualsProto(document4));
  const SnippetProto& snippet_result_2 = search_result.results(1).snippet();
  EXPECT_THAT(snippet_result_2.entries(0).property_name(), Eq("body"));
  content = GetString(&document_result_2,
                      snippet_result_2.entries(0).property_name());
  EXPECT_THAT(GetWindows(content, snippet_result_2.entries(0)),
              ElementsAre("message body"));
  EXPECT_THAT(GetMatches(content, snippet_result_2.entries(0)),
              ElementsAre("message"));

  // Second page, 2 result with 1 snippet
  search_result = icing.GetNextPage(search_result.next_page_token());
  ASSERT_THAT(search_result.status(), ProtoIsOk());
  ASSERT_THAT(search_result.results(), SizeIs(2));
  ASSERT_THAT(search_result.next_page_token(), Gt(kInvalidNextPageToken));

  const DocumentProto& document_result_3 = search_result.results(0).document();
  EXPECT_THAT(document_result_3, EqualsProto(document3));
  const SnippetProto& snippet_result_3 = search_result.results(0).snippet();
  EXPECT_THAT(snippet_result_3.entries(0).property_name(), Eq("body"));
  content = GetString(&document_result_3,
                      snippet_result_3.entries(0).property_name());
  EXPECT_THAT(GetWindows(content, snippet_result_3.entries(0)),
              ElementsAre("message body"));
  EXPECT_THAT(GetMatches(content, snippet_result_3.entries(0)),
              ElementsAre("message"));

  EXPECT_THAT(search_result.results(1).document(), EqualsProto(document2));
  EXPECT_THAT(search_result.results(1).snippet().entries(), IsEmpty());

  // Third page, 1 result with 0 snippets
  search_result = icing.GetNextPage(search_result.next_page_token());
  ASSERT_THAT(search_result.status(), ProtoIsOk());
  ASSERT_THAT(search_result.results(), SizeIs(1));
  ASSERT_THAT(search_result.next_page_token(), Eq(kInvalidNextPageToken));

  EXPECT_THAT(search_result.results(0).document(), EqualsProto(document1));
  EXPECT_THAT(search_result.results(0).snippet().entries(), IsEmpty());
}

TEST_F(IcingSearchEngineTest, ShouldInvalidateNextPageToken) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  DocumentProto document1 = CreateMessageDocument("namespace", "uri1");
  DocumentProto document2 = CreateMessageDocument("namespace", "uri2");
  ASSERT_THAT(icing.Put(document1).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document2).status(), ProtoIsOk());

  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  search_spec.set_query("message");

  ResultSpecProto result_spec;
  result_spec.set_num_per_page(1);

  // Searches and gets the first page, 1 result
  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document2;
  SearchResultProto search_result_proto =
      icing.Search(search_spec, GetDefaultScoringSpec(), result_spec);
  EXPECT_THAT(search_result_proto.next_page_token(), Gt(kInvalidNextPageToken));
  uint64_t next_page_token = search_result_proto.next_page_token();
  // Since the token is a random number, we don't need to verify
  expected_search_result_proto.set_next_page_token(next_page_token);
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
  // Now document1 is still to be fetched.

  // Invalidates token
  icing.InvalidateNextPageToken(next_page_token);

  // Tries to fetch the second page, no result since it's invalidated
  expected_search_result_proto.clear_results();
  expected_search_result_proto.clear_next_page_token();
  search_result_proto = icing.GetNextPage(next_page_token);
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest,
       AllPageTokensShouldBeInvalidatedAfterOptimization) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  DocumentProto document1 = CreateMessageDocument("namespace", "uri1");
  DocumentProto document2 = CreateMessageDocument("namespace", "uri2");
  ASSERT_THAT(icing.Put(document1).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document2).status(), ProtoIsOk());

  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  search_spec.set_query("message");

  ResultSpecProto result_spec;
  result_spec.set_num_per_page(1);

  // Searches and gets the first page, 1 result
  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document2;
  SearchResultProto search_result_proto =
      icing.Search(search_spec, GetDefaultScoringSpec(), result_spec);
  EXPECT_THAT(search_result_proto.next_page_token(), Gt(kInvalidNextPageToken));
  uint64_t next_page_token = search_result_proto.next_page_token();
  // Since the token is a random number, we don't need to verify
  expected_search_result_proto.set_next_page_token(next_page_token);
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
  // Now document1 is still to be fetched.

  OptimizeResultProto optimize_result_proto;
  optimize_result_proto.mutable_status()->set_code(StatusProto::OK);
  optimize_result_proto.mutable_status()->set_message("");
  OptimizeResultProto actual_result = icing.Optimize();
  actual_result.clear_optimize_stats();
  ASSERT_THAT(actual_result, EqualsProto(optimize_result_proto));

  // Tries to fetch the second page, no results since all tokens have been
  // invalidated during Optimize()
  expected_search_result_proto.clear_results();
  expected_search_result_proto.clear_next_page_token();
  search_result_proto = icing.GetNextPage(next_page_token);
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, OptimizationShouldRemoveDeletedDocs) {
  IcingSearchEngineOptions icing_options = GetDefaultIcingOptions();

  DocumentProto document1 = CreateMessageDocument("namespace", "uri1");

  GetResultProto expected_get_result_proto;
  expected_get_result_proto.mutable_status()->set_code(StatusProto::NOT_FOUND);
  expected_get_result_proto.mutable_status()->set_message(
      "Document (namespace, uri1) not found.");
  {
    IcingSearchEngine icing(icing_options, GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
    ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
    ASSERT_THAT(icing.Put(document1).status(), ProtoIsOk());

    // Deletes document1
    ASSERT_THAT(icing.Delete("namespace", "uri1").status(), ProtoIsOk());
    const std::string document_log_path =
        icing_options.base_dir() + "/document_dir/" +
        DocumentLogCreator::GetDocumentLogFilename();
    int64_t document_log_size_before =
        filesystem()->GetFileSize(document_log_path.c_str());
    ASSERT_THAT(icing.Optimize().status(), ProtoIsOk());
    int64_t document_log_size_after =
        filesystem()->GetFileSize(document_log_path.c_str());

    // Validates that document can't be found right after Optimize()
    EXPECT_THAT(
        icing.Get("namespace", "uri1", GetResultSpecProto::default_instance()),
        EqualsProto(expected_get_result_proto));
    // Validates that document is actually removed from document log
    EXPECT_THAT(document_log_size_after, Lt(document_log_size_before));
  }  // Destroys IcingSearchEngine to make sure nothing is cached.

  IcingSearchEngine icing(icing_options, GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(
      icing.Get("namespace", "uri1", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));
}

TEST_F(IcingSearchEngineTest, OptimizationShouldDeleteTemporaryDirectory) {
  IcingSearchEngineOptions icing_options = GetDefaultIcingOptions();
  IcingSearchEngine icing(icing_options, GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  // Create a tmp dir that will be used in Optimize() to swap files,
  // this validates that any tmp dirs will be deleted before using.
  const std::string tmp_dir =
      icing_options.base_dir() + "/document_dir_optimize_tmp";

  const std::string tmp_file = tmp_dir + "/file";
  ASSERT_TRUE(filesystem()->CreateDirectory(tmp_dir.c_str()));
  ScopedFd fd(filesystem()->OpenForWrite(tmp_file.c_str()));
  ASSERT_TRUE(fd.is_valid());
  ASSERT_TRUE(filesystem()->Write(fd.get(), "1234", 4));
  fd.reset();

  EXPECT_THAT(icing.Optimize().status(), ProtoIsOk());

  EXPECT_FALSE(filesystem()->DirectoryExists(tmp_dir.c_str()));
  EXPECT_FALSE(filesystem()->FileExists(tmp_file.c_str()));
}

TEST_F(IcingSearchEngineTest, GetOptimizeInfoHasCorrectStats) {
  DocumentProto document1 = CreateMessageDocument("namespace", "uri1");
  DocumentProto document2 = DocumentBuilder()
                                .SetKey("namespace", "uri2")
                                .SetSchema("Message")
                                .AddStringProperty("body", "message body")
                                .SetCreationTimestampMs(100)
                                .SetTtlMs(500)
                                .Build();

  {
    auto fake_clock = std::make_unique<FakeClock>();
    fake_clock->SetSystemTimeMilliseconds(1000);

    TestIcingSearchEngine icing(GetDefaultIcingOptions(),
                                std::make_unique<Filesystem>(),
                                std::make_unique<IcingFilesystem>(),
                                std::move(fake_clock), GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

    // Just initialized, nothing is optimizable yet.
    GetOptimizeInfoResultProto optimize_info = icing.GetOptimizeInfo();
    EXPECT_THAT(optimize_info.status(), ProtoIsOk());
    EXPECT_THAT(optimize_info.optimizable_docs(), Eq(0));
    EXPECT_THAT(optimize_info.estimated_optimizable_bytes(), Eq(0));
    EXPECT_THAT(optimize_info.time_since_last_optimize_ms(), Eq(0));

    ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
    ASSERT_THAT(icing.Put(document1).status(), ProtoIsOk());

    // Only have active documents, nothing is optimizable yet.
    optimize_info = icing.GetOptimizeInfo();
    EXPECT_THAT(optimize_info.status(), ProtoIsOk());
    EXPECT_THAT(optimize_info.optimizable_docs(), Eq(0));
    EXPECT_THAT(optimize_info.estimated_optimizable_bytes(), Eq(0));
    EXPECT_THAT(optimize_info.time_since_last_optimize_ms(), Eq(0));

    // Deletes document1
    ASSERT_THAT(icing.Delete("namespace", "uri1").status(), ProtoIsOk());

    optimize_info = icing.GetOptimizeInfo();
    EXPECT_THAT(optimize_info.status(), ProtoIsOk());
    EXPECT_THAT(optimize_info.optimizable_docs(), Eq(1));
    EXPECT_THAT(optimize_info.estimated_optimizable_bytes(), Gt(0));
    EXPECT_THAT(optimize_info.time_since_last_optimize_ms(), Eq(0));
    int64_t first_estimated_optimizable_bytes =
        optimize_info.estimated_optimizable_bytes();

    // Add a second document, but it'll be expired since the time (1000) is
    // greater than the document's creation timestamp (100) + the document's ttl
    // (500)
    ASSERT_THAT(icing.Put(document2).status(), ProtoIsOk());

    optimize_info = icing.GetOptimizeInfo();
    EXPECT_THAT(optimize_info.status(), ProtoIsOk());
    EXPECT_THAT(optimize_info.optimizable_docs(), Eq(2));
    EXPECT_THAT(optimize_info.estimated_optimizable_bytes(),
                Gt(first_estimated_optimizable_bytes));
    EXPECT_THAT(optimize_info.time_since_last_optimize_ms(), Eq(0));

    // Optimize
    ASSERT_THAT(icing.Optimize().status(), ProtoIsOk());
  }

  {
    // Recreate with new time
    auto fake_clock = std::make_unique<FakeClock>();
    fake_clock->SetSystemTimeMilliseconds(5000);

    TestIcingSearchEngine icing(GetDefaultIcingOptions(),
                                std::make_unique<Filesystem>(),
                                std::make_unique<IcingFilesystem>(),
                                std::move(fake_clock), GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

    // Nothing is optimizable now that everything has been optimized away.
    GetOptimizeInfoResultProto optimize_info = icing.GetOptimizeInfo();
    EXPECT_THAT(optimize_info.status(), ProtoIsOk());
    EXPECT_THAT(optimize_info.optimizable_docs(), Eq(0));
    EXPECT_THAT(optimize_info.estimated_optimizable_bytes(), Eq(0));
    EXPECT_THAT(optimize_info.time_since_last_optimize_ms(), Eq(4000));
  }
}

TEST_F(IcingSearchEngineTest, GetAndPutShouldWorkAfterOptimization) {
  DocumentProto document1 = CreateMessageDocument("namespace", "uri1");
  DocumentProto document2 = CreateMessageDocument("namespace", "uri2");
  DocumentProto document3 = CreateMessageDocument("namespace", "uri3");

  GetResultProto expected_get_result_proto;
  expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_get_result_proto.mutable_document() = document1;

  {
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
    ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

    ASSERT_THAT(icing.Put(document1).status(), ProtoIsOk());
    ASSERT_THAT(icing.Optimize().status(), ProtoIsOk());

    // Validates that Get() and Put() are good right after Optimize()
    EXPECT_THAT(
        icing.Get("namespace", "uri1", GetResultSpecProto::default_instance()),
        EqualsProto(expected_get_result_proto));
    EXPECT_THAT(icing.Put(document2).status(), ProtoIsOk());
  }  // Destroys IcingSearchEngine to make sure nothing is cached.

  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(
      icing.Get("namespace", "uri1", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  *expected_get_result_proto.mutable_document() = document2;
  EXPECT_THAT(
      icing.Get("namespace", "uri2", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  EXPECT_THAT(icing.Put(document3).status(), ProtoIsOk());
}

TEST_F(IcingSearchEngineTest, DeleteShouldWorkAfterOptimization) {
  DocumentProto document1 = CreateMessageDocument("namespace", "uri1");
  DocumentProto document2 = CreateMessageDocument("namespace", "uri2");
  {
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
    ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
    ASSERT_THAT(icing.Put(document1).status(), ProtoIsOk());
    ASSERT_THAT(icing.Put(document2).status(), ProtoIsOk());
    ASSERT_THAT(icing.Optimize().status(), ProtoIsOk());

    // Validates that Delete() works right after Optimize()
    EXPECT_THAT(icing.Delete("namespace", "uri1").status(), ProtoIsOk());

    GetResultProto expected_get_result_proto;
    expected_get_result_proto.mutable_status()->set_code(
        StatusProto::NOT_FOUND);
    expected_get_result_proto.mutable_status()->set_message(
        "Document (namespace, uri1) not found.");
    EXPECT_THAT(
        icing.Get("namespace", "uri1", GetResultSpecProto::default_instance()),
        EqualsProto(expected_get_result_proto));

    expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
    expected_get_result_proto.mutable_status()->clear_message();
    *expected_get_result_proto.mutable_document() = document2;
    EXPECT_THAT(
        icing.Get("namespace", "uri2", GetResultSpecProto::default_instance()),
        EqualsProto(expected_get_result_proto));
  }  // Destroys IcingSearchEngine to make sure nothing is cached.

  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.Delete("namespace", "uri2").status(), ProtoIsOk());

  GetResultProto expected_get_result_proto;
  expected_get_result_proto.mutable_status()->set_code(StatusProto::NOT_FOUND);
  expected_get_result_proto.mutable_status()->set_message(
      "Document (namespace, uri1) not found.");
  EXPECT_THAT(
      icing.Get("namespace", "uri1", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  expected_get_result_proto.mutable_status()->set_message(
      "Document (namespace, uri2) not found.");
  EXPECT_THAT(
      icing.Get("namespace", "uri2", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));
}

TEST_F(IcingSearchEngineTest, OptimizationFailureUninitializesIcing) {
  // Setup filesystem to fail
  auto mock_filesystem = std::make_unique<MockFilesystem>();
  bool just_swapped_files = false;
  auto create_dir_lambda = [this, &just_swapped_files](const char* dir_name) {
    if (just_swapped_files) {
      // We should fail the first call immediately after swapping files.
      just_swapped_files = false;
      return false;
    }
    return filesystem()->CreateDirectoryRecursively(dir_name);
  };
  ON_CALL(*mock_filesystem, CreateDirectoryRecursively)
      .WillByDefault(create_dir_lambda);
  auto swap_lambda = [&just_swapped_files](const char* first_dir,
                                           const char* second_dir) {
    just_swapped_files = true;
    return false;
  };
  ON_CALL(*mock_filesystem, SwapFiles).WillByDefault(swap_lambda);
  TestIcingSearchEngine icing(GetDefaultIcingOptions(),
                              std::move(mock_filesystem),
                              std::make_unique<IcingFilesystem>(),
                              std::make_unique<FakeClock>(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

  // The mocks should cause an unrecoverable error during Optimize - returning
  // INTERNAL.
  ASSERT_THAT(icing.Optimize().status(), ProtoStatusIs(StatusProto::INTERNAL));

  // Ordinary operations should fail safely.
  SchemaProto simple_schema;
  auto type = simple_schema.add_types();
  type->set_schema_type("type0");
  auto property = type->add_properties();
  property->set_property_name("prop0");
  property->set_data_type(PropertyConfigProto::DataType::STRING);
  property->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);

  DocumentProto simple_doc = DocumentBuilder()
                                 .SetKey("namespace0", "uri0")
                                 .SetSchema("type0")
                                 .AddStringProperty("prop0", "foo")
                                 .Build();

  SearchSpecProto search_spec;
  search_spec.set_query("foo");
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);
  ResultSpecProto result_spec;
  ScoringSpecProto scoring_spec;
  scoring_spec.set_rank_by(
      ScoringSpecProto::RankingStrategy::CREATION_TIMESTAMP);

  EXPECT_THAT(icing.SetSchema(simple_schema).status(),
              ProtoStatusIs(StatusProto::FAILED_PRECONDITION));
  EXPECT_THAT(icing.Put(simple_doc).status(),
              ProtoStatusIs(StatusProto::FAILED_PRECONDITION));
  EXPECT_THAT(icing
                  .Get(simple_doc.namespace_(), simple_doc.uri(),
                       GetResultSpecProto::default_instance())
                  .status(),
              ProtoStatusIs(StatusProto::FAILED_PRECONDITION));
  EXPECT_THAT(icing.Search(search_spec, scoring_spec, result_spec).status(),
              ProtoStatusIs(StatusProto::FAILED_PRECONDITION));

  // Reset should get icing back to a safe (empty) and working state.
  EXPECT_THAT(icing.Reset().status(), ProtoIsOk());
  EXPECT_THAT(icing.SetSchema(simple_schema).status(), ProtoIsOk());
  EXPECT_THAT(icing.Put(simple_doc).status(), ProtoIsOk());
  EXPECT_THAT(icing
                  .Get(simple_doc.namespace_(), simple_doc.uri(),
                       GetResultSpecProto::default_instance())
                  .status(),
              ProtoIsOk());
  EXPECT_THAT(icing.Search(search_spec, scoring_spec, result_spec).status(),
              ProtoIsOk());
}

TEST_F(IcingSearchEngineTest, DeleteBySchemaType) {
  SchemaProto schema;
  // Add an email type
  auto type = schema.add_types();
  type->set_schema_type("email");
  auto property = type->add_properties();
  property->set_property_name("subject");
  property->set_data_type(PropertyConfigProto::DataType::STRING);
  property->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);
  property->mutable_string_indexing_config()->set_term_match_type(
      TermMatchType::EXACT_ONLY);
  property->mutable_string_indexing_config()->set_tokenizer_type(
      StringIndexingConfig::TokenizerType::PLAIN);
  // Add an message type
  type = schema.add_types();
  type->set_schema_type("message");
  property = type->add_properties();
  property->set_property_name("body");
  property->set_data_type(PropertyConfigProto::DataType::STRING);
  property->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);
  property->mutable_string_indexing_config()->set_term_match_type(
      TermMatchType::EXACT_ONLY);
  property->mutable_string_indexing_config()->set_tokenizer_type(
      StringIndexingConfig::TokenizerType::PLAIN);
  DocumentProto document1 =
      DocumentBuilder()
          .SetKey("namespace1", "uri1")
          .SetSchema("message")
          .AddStringProperty("body", "message body1")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  DocumentProto document2 =
      DocumentBuilder()
          .SetKey("namespace2", "uri2")
          .SetSchema("email")
          .AddStringProperty("subject", "message body2")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();

  auto fake_clock = std::make_unique<FakeClock>();
  fake_clock->SetTimerElapsedMilliseconds(7);
  TestIcingSearchEngine icing(GetDefaultIcingOptions(),
                              std::make_unique<Filesystem>(),
                              std::make_unique<IcingFilesystem>(),
                              std::move(fake_clock), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(schema).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document1).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document2).status(), ProtoIsOk());

  GetResultProto expected_get_result_proto;
  expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_get_result_proto.mutable_document() = document1;
  EXPECT_THAT(
      icing.Get("namespace1", "uri1", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  *expected_get_result_proto.mutable_document() = document2;
  EXPECT_THAT(
      icing.Get("namespace2", "uri2", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  // Delete the first type. The first doc should be irretrievable. The
  // second should still be present.
  DeleteBySchemaTypeResultProto result_proto =
      icing.DeleteBySchemaType("message");
  EXPECT_THAT(result_proto.status(), ProtoIsOk());
  DeleteStatsProto exp_stats;
  exp_stats.set_delete_type(DeleteStatsProto::DeleteType::SCHEMA_TYPE);
  exp_stats.set_latency_ms(7);
  exp_stats.set_num_documents_deleted(1);
  EXPECT_THAT(result_proto.delete_stats(), EqualsProto(exp_stats));

  expected_get_result_proto.mutable_status()->set_code(StatusProto::NOT_FOUND);
  expected_get_result_proto.mutable_status()->set_message(
      "Document (namespace1, uri1) not found.");
  expected_get_result_proto.clear_document();
  EXPECT_THAT(
      icing.Get("namespace1", "uri1", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
  expected_get_result_proto.mutable_status()->clear_message();
  *expected_get_result_proto.mutable_document() = document2;
  EXPECT_THAT(
      icing.Get("namespace2", "uri2", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  // Search for "message", only document2 should show up.
  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document2;
  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);
  search_spec.set_query("message");
  SearchResultProto search_result_proto =
      icing.Search(search_spec, GetDefaultScoringSpec(),
                   ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, DeleteSchemaTypeByQuery) {
  SchemaProto schema = CreateMessageSchema();
  // Add an email type
  SchemaProto tmp = CreateEmailSchema();
  *schema.add_types() = tmp.types(0);

  DocumentProto document1 =
      DocumentBuilder()
          .SetKey("namespace1", "uri1")
          .SetSchema(schema.types(0).schema_type())
          .AddStringProperty("body", "message body1")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  DocumentProto document2 =
      DocumentBuilder()
          .SetKey("namespace2", "uri2")
          .SetSchema(schema.types(1).schema_type())
          .AddStringProperty("subject", "subject subject2")
          .AddStringProperty("body", "message body2")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.SetSchema(schema).status(), ProtoIsOk());
  EXPECT_THAT(icing.Put(document1).status(), ProtoIsOk());
  EXPECT_THAT(icing.Put(document2).status(), ProtoIsOk());

  GetResultProto expected_get_result_proto;
  expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_get_result_proto.mutable_document() = document1;
  EXPECT_THAT(
      icing.Get("namespace1", "uri1", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  *expected_get_result_proto.mutable_document() = document2;
  EXPECT_THAT(
      icing.Get("namespace2", "uri2", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  // Delete the first type. The first doc should be irretrievable. The
  // second should still be present.
  SearchSpecProto search_spec;
  search_spec.add_schema_type_filters(schema.types(0).schema_type());
  EXPECT_THAT(icing.DeleteByQuery(search_spec).status(), ProtoIsOk());

  expected_get_result_proto.mutable_status()->set_code(StatusProto::NOT_FOUND);
  expected_get_result_proto.mutable_status()->set_message(
      "Document (namespace1, uri1) not found.");
  expected_get_result_proto.clear_document();
  EXPECT_THAT(
      icing.Get("namespace1", "uri1", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
  expected_get_result_proto.mutable_status()->clear_message();
  *expected_get_result_proto.mutable_document() = document2;
  EXPECT_THAT(
      icing.Get("namespace2", "uri2", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  search_spec = SearchSpecProto::default_instance();
  search_spec.set_query("message");
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);

  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document2;
  SearchResultProto search_result_proto =
      icing.Search(search_spec, GetDefaultScoringSpec(),
                   ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, DeleteByNamespace) {
  DocumentProto document1 =
      DocumentBuilder()
          .SetKey("namespace1", "uri1")
          .SetSchema("Message")
          .AddStringProperty("body", "message body1")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  DocumentProto document2 =
      DocumentBuilder()
          .SetKey("namespace1", "uri2")
          .SetSchema("Message")
          .AddStringProperty("body", "message body2")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  DocumentProto document3 =
      DocumentBuilder()
          .SetKey("namespace3", "uri3")
          .SetSchema("Message")
          .AddStringProperty("body", "message body2")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();

  auto fake_clock = std::make_unique<FakeClock>();
  fake_clock->SetTimerElapsedMilliseconds(7);
  TestIcingSearchEngine icing(GetDefaultIcingOptions(),
                              std::make_unique<Filesystem>(),
                              std::make_unique<IcingFilesystem>(),
                              std::move(fake_clock), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document1).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document2).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document3).status(), ProtoIsOk());

  GetResultProto expected_get_result_proto;
  expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_get_result_proto.mutable_document() = document1;
  EXPECT_THAT(
      icing.Get("namespace1", "uri1", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  *expected_get_result_proto.mutable_document() = document2;
  EXPECT_THAT(
      icing.Get("namespace1", "uri2", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  *expected_get_result_proto.mutable_document() = document3;
  EXPECT_THAT(
      icing.Get("namespace3", "uri3", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  // Delete namespace1. Document1 and document2 should be irretrievable.
  // Document3 should still be present.
  DeleteByNamespaceResultProto result_proto =
      icing.DeleteByNamespace("namespace1");
  EXPECT_THAT(result_proto.status(), ProtoIsOk());
  DeleteStatsProto exp_stats;
  exp_stats.set_delete_type(DeleteStatsProto::DeleteType::NAMESPACE);
  exp_stats.set_latency_ms(7);
  exp_stats.set_num_documents_deleted(2);
  EXPECT_THAT(result_proto.delete_stats(), EqualsProto(exp_stats));

  expected_get_result_proto.mutable_status()->set_code(StatusProto::NOT_FOUND);
  expected_get_result_proto.mutable_status()->set_message(
      "Document (namespace1, uri1) not found.");
  expected_get_result_proto.clear_document();
  EXPECT_THAT(
      icing.Get("namespace1", "uri1", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  expected_get_result_proto.mutable_status()->set_code(StatusProto::NOT_FOUND);
  expected_get_result_proto.mutable_status()->set_message(
      "Document (namespace1, uri2) not found.");
  expected_get_result_proto.clear_document();
  EXPECT_THAT(
      icing.Get("namespace1", "uri2", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
  expected_get_result_proto.mutable_status()->clear_message();
  *expected_get_result_proto.mutable_document() = document3;
  EXPECT_THAT(
      icing.Get("namespace3", "uri3", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  // Search for "message", only document3 should show up.
  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document3;
  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);
  search_spec.set_query("message");
  SearchResultProto search_result_proto =
      icing.Search(search_spec, GetDefaultScoringSpec(),
                   ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, DeleteNamespaceByQuery) {
  DocumentProto document1 =
      DocumentBuilder()
          .SetKey("namespace1", "uri1")
          .SetSchema("Message")
          .AddStringProperty("body", "message body1")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  DocumentProto document2 =
      DocumentBuilder()
          .SetKey("namespace2", "uri2")
          .SetSchema("Message")
          .AddStringProperty("body", "message body2")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();

  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
  EXPECT_THAT(icing.Put(document1).status(), ProtoIsOk());
  EXPECT_THAT(icing.Put(document2).status(), ProtoIsOk());

  GetResultProto expected_get_result_proto;
  expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_get_result_proto.mutable_document() = document1;
  EXPECT_THAT(
      icing.Get("namespace1", "uri1", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  *expected_get_result_proto.mutable_document() = document2;
  EXPECT_THAT(
      icing.Get("namespace2", "uri2", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  // Delete the first namespace. The first doc should be irretrievable. The
  // second should still be present.
  SearchSpecProto search_spec;
  search_spec.add_namespace_filters("namespace1");
  EXPECT_THAT(icing.DeleteByQuery(search_spec).status(), ProtoIsOk());

  expected_get_result_proto.mutable_status()->set_code(StatusProto::NOT_FOUND);
  expected_get_result_proto.mutable_status()->set_message(
      "Document (namespace1, uri1) not found.");
  expected_get_result_proto.clear_document();
  EXPECT_THAT(
      icing.Get("namespace1", "uri1", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
  expected_get_result_proto.mutable_status()->clear_message();
  *expected_get_result_proto.mutable_document() = document2;
  EXPECT_THAT(
      icing.Get("namespace2", "uri2", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  search_spec = SearchSpecProto::default_instance();
  search_spec.set_query("message");
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);

  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document2;
  SearchResultProto search_result_proto =
      icing.Search(search_spec, GetDefaultScoringSpec(),
                   ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, DeleteByQuery) {
  DocumentProto document1 =
      DocumentBuilder()
          .SetKey("namespace1", "uri1")
          .SetSchema("Message")
          .AddStringProperty("body", "message body1")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  DocumentProto document2 =
      DocumentBuilder()
          .SetKey("namespace2", "uri2")
          .SetSchema("Message")
          .AddStringProperty("body", "message body2")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();

  auto fake_clock = std::make_unique<FakeClock>();
  fake_clock->SetTimerElapsedMilliseconds(7);
  TestIcingSearchEngine icing(GetDefaultIcingOptions(),
                              std::make_unique<Filesystem>(),
                              std::make_unique<IcingFilesystem>(),
                              std::move(fake_clock), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
  EXPECT_THAT(icing.Put(document1).status(), ProtoIsOk());
  EXPECT_THAT(icing.Put(document2).status(), ProtoIsOk());

  GetResultProto expected_get_result_proto;
  expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_get_result_proto.mutable_document() = document1;
  EXPECT_THAT(
      icing.Get("namespace1", "uri1", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  *expected_get_result_proto.mutable_document() = document2;
  EXPECT_THAT(
      icing.Get("namespace2", "uri2", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  // Delete all docs containing 'body1'. The first doc should be irretrievable.
  // The second should still be present.
  SearchSpecProto search_spec;
  search_spec.set_query("body1");
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);
  DeleteByQueryResultProto result_proto = icing.DeleteByQuery(search_spec);
  EXPECT_THAT(result_proto.status(), ProtoIsOk());
  DeleteStatsProto exp_stats;
  exp_stats.set_delete_type(DeleteStatsProto::DeleteType::QUERY);
  exp_stats.set_latency_ms(7);
  exp_stats.set_num_documents_deleted(1);
  EXPECT_THAT(result_proto.delete_stats(), EqualsProto(exp_stats));

  expected_get_result_proto.mutable_status()->set_code(StatusProto::NOT_FOUND);
  expected_get_result_proto.mutable_status()->set_message(
      "Document (namespace1, uri1) not found.");
  expected_get_result_proto.clear_document();
  EXPECT_THAT(
      icing.Get("namespace1", "uri1", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
  expected_get_result_proto.mutable_status()->clear_message();
  *expected_get_result_proto.mutable_document() = document2;
  EXPECT_THAT(
      icing.Get("namespace2", "uri2", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  search_spec = SearchSpecProto::default_instance();
  search_spec.set_query("message");
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);

  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document2;
  SearchResultProto search_result_proto =
      icing.Search(search_spec, GetDefaultScoringSpec(),
                   ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, DeleteByQueryNotFound) {
  DocumentProto document1 =
      DocumentBuilder()
          .SetKey("namespace1", "uri1")
          .SetSchema("Message")
          .AddStringProperty("body", "message body1")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  DocumentProto document2 =
      DocumentBuilder()
          .SetKey("namespace2", "uri2")
          .SetSchema("Message")
          .AddStringProperty("body", "message body2")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();

  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
  EXPECT_THAT(icing.Put(document1).status(), ProtoIsOk());
  EXPECT_THAT(icing.Put(document2).status(), ProtoIsOk());

  GetResultProto expected_get_result_proto;
  expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_get_result_proto.mutable_document() = document1;
  EXPECT_THAT(
      icing.Get("namespace1", "uri1", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  *expected_get_result_proto.mutable_document() = document2;
  EXPECT_THAT(
      icing.Get("namespace2", "uri2", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  // Delete all docs containing 'foo', which should be none of them. Both docs
  // should still be present.
  SearchSpecProto search_spec;
  search_spec.set_query("foo");
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);
  EXPECT_THAT(icing.DeleteByQuery(search_spec).status(),
              ProtoStatusIs(StatusProto::NOT_FOUND));

  expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
  expected_get_result_proto.mutable_status()->clear_message();
  *expected_get_result_proto.mutable_document() = document1;
  EXPECT_THAT(
      icing.Get("namespace1", "uri1", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
  expected_get_result_proto.mutable_status()->clear_message();
  *expected_get_result_proto.mutable_document() = document2;
  EXPECT_THAT(
      icing.Get("namespace2", "uri2", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  search_spec = SearchSpecProto::default_instance();
  search_spec.set_query("message");
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);

  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document2;
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document1;
  SearchResultProto search_result_proto =
      icing.Search(search_spec, GetDefaultScoringSpec(),
                   ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, SetSchemaShouldWorkAfterOptimization) {
  // Creates 3 test schemas
  SchemaProto schema1 = SchemaProto(CreateMessageSchema());

  SchemaProto schema2 = SchemaProto(schema1);
  auto new_property2 = schema2.mutable_types(0)->add_properties();
  new_property2->set_property_name("property2");
  new_property2->set_data_type(PropertyConfigProto::DataType::STRING);
  new_property2->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);
  new_property2->mutable_string_indexing_config()->set_term_match_type(
      TermMatchType::PREFIX);
  new_property2->mutable_string_indexing_config()->set_tokenizer_type(
      StringIndexingConfig::TokenizerType::PLAIN);

  SchemaProto schema3 = SchemaProto(schema2);
  auto new_property3 = schema3.mutable_types(0)->add_properties();
  new_property3->set_property_name("property3");
  new_property3->set_data_type(PropertyConfigProto::DataType::STRING);
  new_property3->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);
  new_property3->mutable_string_indexing_config()->set_term_match_type(
      TermMatchType::PREFIX);
  new_property3->mutable_string_indexing_config()->set_tokenizer_type(
      StringIndexingConfig::TokenizerType::PLAIN);

  {
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
    ASSERT_THAT(icing.SetSchema(schema1).status(), ProtoIsOk());
    ASSERT_THAT(icing.Optimize().status(), ProtoIsOk());

    // Validates that SetSchema() works right after Optimize()
    EXPECT_THAT(icing.SetSchema(schema2).status(), ProtoIsOk());
  }  // Destroys IcingSearchEngine to make sure nothing is cached.

  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.SetSchema(schema3).status(), ProtoIsOk());
}

TEST_F(IcingSearchEngineTest, SearchShouldWorkAfterOptimization) {
  DocumentProto document = CreateMessageDocument("namespace", "uri");
  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  search_spec.set_query("m");
  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document;

  {
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
    ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
    ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
    ASSERT_THAT(icing.Optimize().status(), ProtoIsOk());

    // Validates that Search() works right after Optimize()
    SearchResultProto search_result_proto =
        icing.Search(search_spec, GetDefaultScoringSpec(),
                     ResultSpecProto::default_instance());
    EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                         expected_search_result_proto));
  }  // Destroys IcingSearchEngine to make sure nothing is cached.

  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  SearchResultProto search_result_proto =
      icing.Search(search_spec, GetDefaultScoringSpec(),
                   ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, IcingShouldWorkFineIfOptimizationIsAborted) {
  DocumentProto document1 = CreateMessageDocument("namespace", "uri1");
  {
    // Initializes a normal icing to create files needed
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
    ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
    ASSERT_THAT(icing.Put(document1).status(), ProtoIsOk());
  }

  // Creates a mock filesystem in which DeleteDirectoryRecursively() always
  // fails. This will fail IcingSearchEngine::OptimizeDocumentStore() and makes
  // it return ABORTED_ERROR.
  auto mock_filesystem = std::make_unique<MockFilesystem>();
  ON_CALL(*mock_filesystem, DeleteDirectoryRecursively)
      .WillByDefault(Return(false));

  TestIcingSearchEngine icing(GetDefaultIcingOptions(),
                              std::move(mock_filesystem),
                              std::make_unique<IcingFilesystem>(),
                              std::make_unique<FakeClock>(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.Optimize().status(), ProtoStatusIs(StatusProto::ABORTED));

  // Now optimization is aborted, we verify that document-related functions
  // still work as expected.

  GetResultProto expected_get_result_proto;
  expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_get_result_proto.mutable_document() = document1;
  EXPECT_THAT(
      icing.Get("namespace", "uri1", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  DocumentProto document2 = CreateMessageDocument("namespace", "uri2");

  EXPECT_THAT(icing.Put(document2).status(), ProtoIsOk());

  SearchSpecProto search_spec;
  search_spec.set_query("m");
  search_spec.set_term_match_type(TermMatchType::PREFIX);

  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document2;
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document1;

  SearchResultProto search_result_proto =
      icing.Search(search_spec, GetDefaultScoringSpec(),
                   ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest,
       OptimizationShouldRecoverIfFileDirectoriesAreMissing) {
  // Creates a mock filesystem in which SwapFiles() always fails and deletes the
  // directories. This will fail IcingSearchEngine::OptimizeDocumentStore().
  auto mock_filesystem = std::make_unique<MockFilesystem>();
  ON_CALL(*mock_filesystem, SwapFiles)
      .WillByDefault([this](const char* one, const char* two) {
        filesystem()->DeleteDirectoryRecursively(one);
        filesystem()->DeleteDirectoryRecursively(two);
        return false;
      });

  TestIcingSearchEngine icing(GetDefaultIcingOptions(),
                              std::move(mock_filesystem),
                              std::make_unique<IcingFilesystem>(),
                              std::make_unique<FakeClock>(), GetTestJniCache());

  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(CreateMessageDocument("namespace", "uri")).status(),
              ProtoIsOk());

  // Optimize() fails due to filesystem error
  EXPECT_THAT(icing.Optimize().status(),
              ProtoStatusIs(StatusProto::WARNING_DATA_LOSS));

  // Document is not found because original file directory is missing
  GetResultProto expected_get_result_proto;
  expected_get_result_proto.mutable_status()->set_code(StatusProto::NOT_FOUND);
  expected_get_result_proto.mutable_status()->set_message(
      "Document (namespace, uri) not found.");
  EXPECT_THAT(
      icing.Get("namespace", "uri", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  DocumentProto new_document =
      DocumentBuilder()
          .SetKey("namespace", "uri2")
          .SetSchema("Message")
          .AddStringProperty("body", "new body")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();

  EXPECT_THAT(icing.Put(new_document).status(), ProtoIsOk());

  SearchSpecProto search_spec;
  search_spec.set_query("m");
  search_spec.set_term_match_type(TermMatchType::PREFIX);

  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);

  // Searching old content returns nothing because original file directory is
  // missing
  SearchResultProto search_result_proto =
      icing.Search(search_spec, GetDefaultScoringSpec(),
                   ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));

  search_spec.set_query("n");

  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      new_document;

  // Searching new content returns the new document
  search_result_proto = icing.Search(search_spec, GetDefaultScoringSpec(),
                                     ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, OptimizationShouldRecoverIfDataFilesAreMissing) {
  // Creates a mock filesystem in which SwapFiles() always fails and empties the
  // directories. This will fail IcingSearchEngine::OptimizeDocumentStore().
  auto mock_filesystem = std::make_unique<MockFilesystem>();
  ON_CALL(*mock_filesystem, SwapFiles)
      .WillByDefault([this](const char* one, const char* two) {
        filesystem()->DeleteDirectoryRecursively(one);
        filesystem()->CreateDirectoryRecursively(one);
        filesystem()->DeleteDirectoryRecursively(two);
        filesystem()->CreateDirectoryRecursively(two);
        return false;
      });

  TestIcingSearchEngine icing(GetDefaultIcingOptions(),
                              std::move(mock_filesystem),
                              std::make_unique<IcingFilesystem>(),
                              std::make_unique<FakeClock>(), GetTestJniCache());

  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(CreateMessageDocument("namespace", "uri")).status(),
              ProtoIsOk());

  // Optimize() fails due to filesystem error
  EXPECT_THAT(icing.Optimize().status(),
              ProtoStatusIs(StatusProto::WARNING_DATA_LOSS));

  // Document is not found because original files are missing
  GetResultProto expected_get_result_proto;
  expected_get_result_proto.mutable_status()->set_code(StatusProto::NOT_FOUND);
  expected_get_result_proto.mutable_status()->set_message(
      "Document (namespace, uri) not found.");
  EXPECT_THAT(
      icing.Get("namespace", "uri", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  DocumentProto new_document =
      DocumentBuilder()
          .SetKey("namespace", "uri2")
          .SetSchema("Message")
          .AddStringProperty("body", "new body")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();

  EXPECT_THAT(icing.Put(new_document).status(), ProtoIsOk());

  SearchSpecProto search_spec;
  search_spec.set_query("m");
  search_spec.set_term_match_type(TermMatchType::PREFIX);

  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);

  // Searching old content returns nothing because original files are missing
  SearchResultProto search_result_proto =
      icing.Search(search_spec, GetDefaultScoringSpec(),
                   ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));

  search_spec.set_query("n");

  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      new_document;

  // Searching new content returns the new document
  search_result_proto = icing.Search(search_spec, GetDefaultScoringSpec(),
                                     ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, SearchIncludesDocumentsBeforeTtl) {
  SchemaProto schema;
  auto type = schema.add_types();
  type->set_schema_type("Message");

  auto body = type->add_properties();
  body->set_property_name("body");
  body->set_data_type(PropertyConfigProto::DataType::STRING);
  body->set_cardinality(PropertyConfigProto::Cardinality::REQUIRED);
  body->mutable_string_indexing_config()->set_term_match_type(
      TermMatchType::PREFIX);
  body->mutable_string_indexing_config()->set_tokenizer_type(
      StringIndexingConfig::TokenizerType::PLAIN);

  DocumentProto document = DocumentBuilder()
                               .SetKey("namespace", "uri")
                               .SetSchema("Message")
                               .AddStringProperty("body", "message body")
                               .SetCreationTimestampMs(100)
                               .SetTtlMs(500)
                               .Build();

  SearchSpecProto search_spec;
  search_spec.set_query("message");
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);

  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document;

  // Time just has to be less than the document's creation timestamp (100) + the
  // document's ttl (500)
  auto fake_clock = std::make_unique<FakeClock>();
  fake_clock->SetSystemTimeMilliseconds(400);

  TestIcingSearchEngine icing(GetDefaultIcingOptions(),
                              std::make_unique<Filesystem>(),
                              std::make_unique<IcingFilesystem>(),
                              std::move(fake_clock), GetTestJniCache());

  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.SetSchema(schema).status(), ProtoIsOk());
  EXPECT_THAT(icing.Put(document).status(), ProtoIsOk());

  // Check that the document is returned as part of search results
  SearchResultProto search_result_proto =
      icing.Search(search_spec, GetDefaultScoringSpec(),
                   ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, SearchDoesntIncludeDocumentsPastTtl) {
  SchemaProto schema;
  auto type = schema.add_types();
  type->set_schema_type("Message");

  auto body = type->add_properties();
  body->set_property_name("body");
  body->set_data_type(PropertyConfigProto::DataType::STRING);
  body->set_cardinality(PropertyConfigProto::Cardinality::REQUIRED);
  body->mutable_string_indexing_config()->set_term_match_type(
      TermMatchType::PREFIX);
  body->mutable_string_indexing_config()->set_tokenizer_type(
      StringIndexingConfig::TokenizerType::PLAIN);

  DocumentProto document = DocumentBuilder()
                               .SetKey("namespace", "uri")
                               .SetSchema("Message")
                               .AddStringProperty("body", "message body")
                               .SetCreationTimestampMs(100)
                               .SetTtlMs(500)
                               .Build();

  SearchSpecProto search_spec;
  search_spec.set_query("message");
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);

  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);

  // Time just has to be greater than the document's creation timestamp (100) +
  // the document's ttl (500)
  auto fake_clock = std::make_unique<FakeClock>();
  fake_clock->SetSystemTimeMilliseconds(700);

  TestIcingSearchEngine icing(GetDefaultIcingOptions(),
                              std::make_unique<Filesystem>(),
                              std::make_unique<IcingFilesystem>(),
                              std::move(fake_clock), GetTestJniCache());

  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.SetSchema(schema).status(), ProtoIsOk());
  EXPECT_THAT(icing.Put(document).status(), ProtoIsOk());

  // Check that the document is not returned as part of search results
  SearchResultProto search_result_proto =
      icing.Search(search_spec, GetDefaultScoringSpec(),
                   ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, SearchWorksAfterSchemaTypesCompatiblyModified) {
  SchemaProto schema;
  auto type_config = schema.add_types();
  type_config->set_schema_type("message");

  auto property = type_config->add_properties();
  property->set_property_name("body");
  property->set_data_type(PropertyConfigProto::DataType::STRING);
  property->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);

  DocumentProto message_document =
      DocumentBuilder()
          .SetKey("namespace", "message_uri")
          .SetSchema("message")
          .AddStringProperty("body", "foo")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();

  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.SetSchema(schema).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(message_document).status(), ProtoIsOk());

  // Make sure we can search for message document
  SearchSpecProto search_spec;
  search_spec.set_query("foo");
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);

  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);

  // The message isn't indexed, so we get nothing
  SearchResultProto search_result_proto =
      icing.Search(search_spec, GetDefaultScoringSpec(),
                   ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));

  // With just the schema type filter, we can search for the message
  search_spec.Clear();
  search_spec.add_schema_type_filters("message");

  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      message_document;

  search_result_proto = icing.Search(search_spec, GetDefaultScoringSpec(),
                                     ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));

  // Since SchemaTypeIds are assigned based on order in the SchemaProto, this
  // will force a change in the DocumentStore's cached SchemaTypeIds
  schema.clear_types();
  type_config = schema.add_types();
  type_config->set_schema_type("email");

  // Adding a new indexed property will require reindexing
  type_config = schema.add_types();
  type_config->set_schema_type("message");

  property = type_config->add_properties();
  property->set_property_name("body");
  property->set_data_type(PropertyConfigProto::DataType::STRING);
  property->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);
  property->mutable_string_indexing_config()->set_term_match_type(
      TermMatchType::PREFIX);
  property->mutable_string_indexing_config()->set_tokenizer_type(
      StringIndexingConfig::TokenizerType::PLAIN);

  EXPECT_THAT(icing.SetSchema(schema).status(), ProtoIsOk());

  search_spec.Clear();
  search_spec.set_query("foo");
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);
  search_spec.add_schema_type_filters("message");

  // We can still search for the message document
  search_result_proto = icing.Search(search_spec, GetDefaultScoringSpec(),
                                     ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, RecoverFromMissingHeaderFile) {
  SearchSpecProto search_spec;
  search_spec.set_query("message");
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);

  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      CreateMessageDocument("namespace", "uri");

  GetResultProto expected_get_result_proto;
  expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_get_result_proto.mutable_document() =
      CreateMessageDocument("namespace", "uri");

  {
    // Basic initialization/setup
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
    EXPECT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
    EXPECT_THAT(icing.Put(CreateMessageDocument("namespace", "uri")).status(),
                ProtoIsOk());
    EXPECT_THAT(
        icing.Get("namespace", "uri", GetResultSpecProto::default_instance()),
        EqualsProto(expected_get_result_proto));
    SearchResultProto search_result_proto =
        icing.Search(search_spec, GetDefaultScoringSpec(),
                     ResultSpecProto::default_instance());
    EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                         expected_search_result_proto));
  }  // This should shut down IcingSearchEngine and persist anything it needs to

  EXPECT_TRUE(filesystem()->DeleteFile(GetHeaderFilename().c_str()));

  // We should be able to recover from this and access all our previous data
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());

  // Checks that DocumentLog is still ok
  EXPECT_THAT(
      icing.Get("namespace", "uri", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  // Checks that the index is still ok so we can search over it
  SearchResultProto search_result_proto =
      icing.Search(search_spec, GetDefaultScoringSpec(),
                   ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));

  // Checks that Schema is still since it'll be needed to validate the document
  EXPECT_THAT(icing.Put(CreateMessageDocument("namespace", "uri")).status(),
              ProtoIsOk());
}

TEST_F(IcingSearchEngineTest, UnableToRecoverFromCorruptSchema) {
  {
    // Basic initialization/setup
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
    EXPECT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
    EXPECT_THAT(icing.Put(CreateMessageDocument("namespace", "uri")).status(),
                ProtoIsOk());

    GetResultProto expected_get_result_proto;
    expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
    *expected_get_result_proto.mutable_document() =
        CreateMessageDocument("namespace", "uri");

    EXPECT_THAT(
        icing.Get("namespace", "uri", GetResultSpecProto::default_instance()),
        EqualsProto(expected_get_result_proto));
  }  // This should shut down IcingSearchEngine and persist anything it needs to

  const std::string schema_file =
      absl_ports::StrCat(GetSchemaDir(), "/schema.pb");
  const std::string corrupt_data = "1234";
  EXPECT_TRUE(filesystem()->Write(schema_file.c_str(), corrupt_data.data(),
                                  corrupt_data.size()));

  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(),
              ProtoStatusIs(StatusProto::INTERNAL));
}

TEST_F(IcingSearchEngineTest, UnableToRecoverFromCorruptDocumentLog) {
  {
    // Basic initialization/setup
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
    EXPECT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
    EXPECT_THAT(icing.Put(CreateMessageDocument("namespace", "uri")).status(),
                ProtoIsOk());

    GetResultProto expected_get_result_proto;
    expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
    *expected_get_result_proto.mutable_document() =
        CreateMessageDocument("namespace", "uri");

    EXPECT_THAT(
        icing.Get("namespace", "uri", GetResultSpecProto::default_instance()),
        EqualsProto(expected_get_result_proto));
  }  // This should shut down IcingSearchEngine and persist anything it needs to

  const std::string document_log_file = absl_ports::StrCat(
      GetDocumentDir(), "/", DocumentLogCreator::GetDocumentLogFilename());
  const std::string corrupt_data = "1234";
  EXPECT_TRUE(filesystem()->Write(document_log_file.c_str(),
                                  corrupt_data.data(), corrupt_data.size()));

  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(),
              ProtoStatusIs(StatusProto::INTERNAL));
}

TEST_F(IcingSearchEngineTest, RecoverFromInconsistentSchemaStore) {
  DocumentProto document1 = CreateMessageDocument("namespace", "uri1");
  DocumentProto document2_with_additional_property =
      DocumentBuilder()
          .SetKey("namespace", "uri2")
          .SetSchema("Message")
          .AddStringProperty("additional", "content")
          .AddStringProperty("body", "message body")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();

  IcingSearchEngineOptions options = GetDefaultIcingOptions();
  {
    // Initializes folder and schema
    IcingSearchEngine icing(options, GetTestJniCache());
    EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());

    SchemaProto schema;
    auto type = schema.add_types();
    type->set_schema_type("Message");

    auto property = type->add_properties();
    property->set_property_name("body");
    property->set_data_type(PropertyConfigProto::DataType::STRING);
    property->set_cardinality(PropertyConfigProto::Cardinality::REQUIRED);
    property->mutable_string_indexing_config()->set_term_match_type(
        TermMatchType::PREFIX);
    property->mutable_string_indexing_config()->set_tokenizer_type(
        StringIndexingConfig::TokenizerType::PLAIN);

    property = type->add_properties();
    property->set_property_name("additional");
    property->set_data_type(PropertyConfigProto::DataType::STRING);
    property->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);

    EXPECT_THAT(icing.SetSchema(schema).status(), ProtoIsOk());
    EXPECT_THAT(icing.Put(document1).status(), ProtoIsOk());
    EXPECT_THAT(icing.Put(document2_with_additional_property).status(),
                ProtoIsOk());

    // Won't get us anything because "additional" isn't marked as an indexed
    // property in the schema
    SearchSpecProto search_spec;
    search_spec.set_query("additional:content");
    search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);

    SearchResultProto expected_search_result_proto;
    expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
    SearchResultProto search_result_proto =
        icing.Search(search_spec, GetDefaultScoringSpec(),
                     ResultSpecProto::default_instance());
    EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                         expected_search_result_proto));
  }  // This should shut down IcingSearchEngine and persist anything it needs to

  {
    // This schema will change the SchemaTypeIds from the previous schema_
    // (since SchemaTypeIds are assigned based on order of the types, and this
    // new schema changes the ordering of previous types)
    SchemaProto new_schema;
    auto type = new_schema.add_types();
    type->set_schema_type("Email");

    type = new_schema.add_types();
    type->set_schema_type("Message");

    // Adding a new property changes the SectionIds (since SectionIds are
    // assigned based on alphabetical order of indexed sections, marking
    // "additional" as an indexed property will push the "body" property to a
    // different SectionId)
    auto property = type->add_properties();
    property->set_property_name("body");
    property->set_data_type(PropertyConfigProto::DataType::STRING);
    property->set_cardinality(PropertyConfigProto::Cardinality::REQUIRED);
    property->mutable_string_indexing_config()->set_term_match_type(
        TermMatchType::PREFIX);
    property->mutable_string_indexing_config()->set_tokenizer_type(
        StringIndexingConfig::TokenizerType::PLAIN);

    property = type->add_properties();
    property->set_property_name("additional");
    property->set_data_type(PropertyConfigProto::DataType::STRING);
    property->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);
    property->mutable_string_indexing_config()->set_term_match_type(
        TermMatchType::PREFIX);
    property->mutable_string_indexing_config()->set_tokenizer_type(
        StringIndexingConfig::TokenizerType::PLAIN);

    // Write the marker file
    std::string marker_filepath =
        absl_ports::StrCat(options.base_dir(), "/set_schema_marker");
    ScopedFd sfd(filesystem()->OpenForWrite(marker_filepath.c_str()));
    ASSERT_TRUE(sfd.is_valid());

    // Write the new schema
    FakeClock fake_clock;
    ICING_ASSERT_OK_AND_ASSIGN(
        std::unique_ptr<SchemaStore> schema_store,
        SchemaStore::Create(filesystem(), GetSchemaDir(), &fake_clock));
    ICING_EXPECT_OK(schema_store->SetSchema(new_schema));
  }  // Will persist new schema

  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());

  // We can insert a Email document since we kept the new schema
  DocumentProto email_document =
      DocumentBuilder()
          .SetKey("namespace", "email_uri")
          .SetSchema("Email")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  EXPECT_THAT(icing.Put(email_document).status(), ProtoIsOk());

  GetResultProto expected_get_result_proto;
  expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_get_result_proto.mutable_document() = email_document;

  EXPECT_THAT(icing.Get("namespace", "email_uri",
                        GetResultSpecProto::default_instance()),
              EqualsProto(expected_get_result_proto));

  SearchSpecProto search_spec;

  // The section restrict will ensure we are using the correct, updated
  // SectionId in the Index
  search_spec.set_query("additional:content");

  // Schema type filter will ensure we're using the correct, updated
  // SchemaTypeId in the DocumentStore
  search_spec.add_schema_type_filters("Message");
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);

  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document2_with_additional_property;

  SearchResultProto search_result_proto =
      icing.Search(search_spec, GetDefaultScoringSpec(),
                   ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, RecoverFromInconsistentDocumentStore) {
  DocumentProto document1 = CreateMessageDocument("namespace", "uri1");
  DocumentProto document2 = CreateMessageDocument("namespace", "uri2");

  {
    // Initializes folder and schema, index one document
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
    EXPECT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
    EXPECT_THAT(icing.Put(document1).status(), ProtoIsOk());
  }  // This should shut down IcingSearchEngine and persist anything it needs to

  {
    FakeClock fake_clock;
    ICING_ASSERT_OK_AND_ASSIGN(
        std::unique_ptr<SchemaStore> schema_store,
        SchemaStore::Create(filesystem(), GetSchemaDir(), &fake_clock));
    ICING_EXPECT_OK(schema_store->SetSchema(CreateMessageSchema()));

    // Puts a second document into DocumentStore but doesn't index it.
    ICING_ASSERT_OK_AND_ASSIGN(
        DocumentStore::CreateResult create_result,
        DocumentStore::Create(filesystem(), GetDocumentDir(), &fake_clock,
                              schema_store.get()));
    std::unique_ptr<DocumentStore> document_store =
        std::move(create_result.document_store);

    ICING_EXPECT_OK(document_store->Put(document2));
  }

  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  // Index Restoration should be triggered here and document2 should be
  // indexed.
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());

  GetResultProto expected_get_result_proto;
  expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_get_result_proto.mutable_document() = document1;

  // DocumentStore kept the additional document
  EXPECT_THAT(
      icing.Get("namespace", "uri1", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  *expected_get_result_proto.mutable_document() = document2;
  EXPECT_THAT(
      icing.Get("namespace", "uri2", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  // We indexed the additional document
  SearchSpecProto search_spec;
  search_spec.set_query("message");
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);

  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document2;

  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document1;

  SearchResultProto search_result_proto =
      icing.Search(search_spec, GetDefaultScoringSpec(),
                   ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, RecoverFromInconsistentIndex) {
  SearchSpecProto search_spec;
  search_spec.set_query("message");
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);

  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      CreateMessageDocument("namespace", "uri");

  {
    // Initializes folder and schema, index one document
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
    EXPECT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
    EXPECT_THAT(icing.Put(CreateMessageDocument("namespace", "uri")).status(),
                ProtoIsOk());
    SearchResultProto search_result_proto =
        icing.Search(search_spec, GetDefaultScoringSpec(),
                     ResultSpecProto::default_instance());
    EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                         expected_search_result_proto));
  }  // This should shut down IcingSearchEngine and persist anything it needs to

  // Pretend we lost the entire index
  EXPECT_TRUE(filesystem()->DeleteDirectoryRecursively(
      absl_ports::StrCat(GetIndexDir(), "/idx/lite.").c_str()));

  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());

  // Check that our index is ok by searching over the restored index
  SearchResultProto search_result_proto =
      icing.Search(search_spec, GetDefaultScoringSpec(),
                   ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, RecoverFromCorruptIndex) {
  SearchSpecProto search_spec;
  search_spec.set_query("message");
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);

  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      CreateMessageDocument("namespace", "uri");

  {
    // Initializes folder and schema, index one document
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
    EXPECT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
    EXPECT_THAT(icing.Put(CreateMessageDocument("namespace", "uri")).status(),
                ProtoIsOk());
    SearchResultProto search_result_proto =
        icing.Search(search_spec, GetDefaultScoringSpec(),
                     ResultSpecProto::default_instance());
    EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                         expected_search_result_proto));
  }  // This should shut down IcingSearchEngine and persist anything it needs to

  // Pretend index is corrupted
  const std::string index_hit_buffer_file = GetIndexDir() + "/idx/lite.hb";
  ScopedFd fd(filesystem()->OpenForWrite(index_hit_buffer_file.c_str()));
  ASSERT_TRUE(fd.is_valid());
  ASSERT_TRUE(filesystem()->Write(fd.get(), "1234", 4));

  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());

  // Check that our index is ok by searching over the restored index
  SearchResultProto search_result_proto =
      icing.Search(search_spec, GetDefaultScoringSpec(),
                   ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, SearchResultShouldBeRankedByDocumentScore) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  // Creates 3 documents and ensures the relationship in terms of document
  // score is: document1 < document2 < document3
  DocumentProto document1 =
      DocumentBuilder()
          .SetKey("namespace", "uri/1")
          .SetSchema("Message")
          .AddStringProperty("body", "message1")
          .SetScore(1)
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  DocumentProto document2 =
      DocumentBuilder()
          .SetKey("namespace", "uri/2")
          .SetSchema("Message")
          .AddStringProperty("body", "message2")
          .SetScore(2)
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  DocumentProto document3 =
      DocumentBuilder()
          .SetKey("namespace", "uri/3")
          .SetSchema("Message")
          .AddStringProperty("body", "message3")
          .SetScore(3)
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();

  // Intentionally inserts the documents in the order that is different than
  // their score order
  ASSERT_THAT(icing.Put(document2).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document3).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document1).status(), ProtoIsOk());

  // "m" will match all 3 documents
  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  search_spec.set_query("m");

  // Result should be in descending score order
  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document3;
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document2;
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document1;

  ScoringSpecProto scoring_spec = GetDefaultScoringSpec();
  scoring_spec.set_rank_by(ScoringSpecProto::RankingStrategy::DOCUMENT_SCORE);
  SearchResultProto search_result_proto = icing.Search(
      search_spec, scoring_spec, ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, SearchShouldAllowNoScoring) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  // Creates 3 documents and ensures the relationship of them is:
  // document1 < document2 < document3
  DocumentProto document1 = DocumentBuilder()
                                .SetKey("namespace", "uri/1")
                                .SetSchema("Message")
                                .AddStringProperty("body", "message1")
                                .SetScore(1)
                                .SetCreationTimestampMs(1571111111111)
                                .Build();
  DocumentProto document2 = DocumentBuilder()
                                .SetKey("namespace", "uri/2")
                                .SetSchema("Message")
                                .AddStringProperty("body", "message2")
                                .SetScore(2)
                                .SetCreationTimestampMs(1572222222222)
                                .Build();
  DocumentProto document3 = DocumentBuilder()
                                .SetKey("namespace", "uri/3")
                                .SetSchema("Message")
                                .AddStringProperty("body", "message3")
                                .SetScore(3)
                                .SetCreationTimestampMs(1573333333333)
                                .Build();

  // Intentionally inserts the documents in the order that is different than
  // their score order
  ASSERT_THAT(icing.Put(document3).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document1).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document2).status(), ProtoIsOk());

  // "m" will match all 3 documents
  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  search_spec.set_query("m");

  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document2;
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document1;
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document3;

  // Results should not be ranked by score but returned in reverse insertion
  // order.
  ScoringSpecProto scoring_spec = GetDefaultScoringSpec();
  scoring_spec.set_rank_by(ScoringSpecProto::RankingStrategy::NONE);
  SearchResultProto search_result_proto = icing.Search(
      search_spec, scoring_spec, ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, SearchResultShouldBeRankedByCreationTimestamp) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  // Creates 3 documents and ensures the relationship in terms of creation
  // timestamp score is: document1 < document2 < document3
  DocumentProto document1 = DocumentBuilder()
                                .SetKey("namespace", "uri/1")
                                .SetSchema("Message")
                                .AddStringProperty("body", "message1")
                                .SetCreationTimestampMs(1571111111111)
                                .Build();
  DocumentProto document2 = DocumentBuilder()
                                .SetKey("namespace", "uri/2")
                                .SetSchema("Message")
                                .AddStringProperty("body", "message2")
                                .SetCreationTimestampMs(1572222222222)
                                .Build();
  DocumentProto document3 = DocumentBuilder()
                                .SetKey("namespace", "uri/3")
                                .SetSchema("Message")
                                .AddStringProperty("body", "message3")
                                .SetCreationTimestampMs(1573333333333)
                                .Build();

  // Intentionally inserts the documents in the order that is different than
  // their score order
  ASSERT_THAT(icing.Put(document3).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document1).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document2).status(), ProtoIsOk());

  // "m" will match all 3 documents
  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  search_spec.set_query("m");

  // Result should be in descending timestamp order
  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document3;
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document2;
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document1;

  ScoringSpecProto scoring_spec = GetDefaultScoringSpec();
  scoring_spec.set_rank_by(
      ScoringSpecProto::RankingStrategy::CREATION_TIMESTAMP);
  SearchResultProto search_result_proto = icing.Search(
      search_spec, scoring_spec, ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, SearchResultShouldBeRankedByUsageCount) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  // Creates 3 test documents
  DocumentProto document1 =
      DocumentBuilder()
          .SetKey("namespace", "uri/1")
          .SetSchema("Message")
          .AddStringProperty("body", "message1")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  DocumentProto document2 =
      DocumentBuilder()
          .SetKey("namespace", "uri/2")
          .SetSchema("Message")
          .AddStringProperty("body", "message2")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  DocumentProto document3 =
      DocumentBuilder()
          .SetKey("namespace", "uri/3")
          .SetSchema("Message")
          .AddStringProperty("body", "message3")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();

  // Intentionally inserts the documents in a different order to eliminate the
  // possibility that the following results are sorted in the default reverse
  // insertion order.
  ASSERT_THAT(icing.Put(document3).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document1).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document2).status(), ProtoIsOk());

  // Report usage for doc3 twice and doc2 once. The order will be doc3 > doc2 >
  // doc1 when ranked by USAGE_TYPE1_COUNT.
  UsageReport usage_report_doc3 = CreateUsageReport(
      /*name_space=*/"namespace", /*uri=*/"uri/3", /*timestamp_ms=*/0,
      UsageReport::USAGE_TYPE1);
  UsageReport usage_report_doc2 = CreateUsageReport(
      /*name_space=*/"namespace", /*uri=*/"uri/2", /*timestamp_ms=*/0,
      UsageReport::USAGE_TYPE1);
  ASSERT_THAT(icing.ReportUsage(usage_report_doc3).status(), ProtoIsOk());
  ASSERT_THAT(icing.ReportUsage(usage_report_doc3).status(), ProtoIsOk());
  ASSERT_THAT(icing.ReportUsage(usage_report_doc2).status(), ProtoIsOk());

  // "m" will match all 3 documents
  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  search_spec.set_query("m");

  // Result should be in descending USAGE_TYPE1_COUNT order
  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document3;
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document2;
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document1;

  ScoringSpecProto scoring_spec;
  scoring_spec.set_rank_by(
      ScoringSpecProto::RankingStrategy::USAGE_TYPE1_COUNT);
  SearchResultProto search_result_proto = icing.Search(
      search_spec, scoring_spec, ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest,
       SearchResultShouldHaveDefaultOrderWithoutUsageCounts) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  // Creates 3 test documents
  DocumentProto document1 =
      DocumentBuilder()
          .SetKey("namespace", "uri/1")
          .SetSchema("Message")
          .AddStringProperty("body", "message1")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  DocumentProto document2 =
      DocumentBuilder()
          .SetKey("namespace", "uri/2")
          .SetSchema("Message")
          .AddStringProperty("body", "message2")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  DocumentProto document3 =
      DocumentBuilder()
          .SetKey("namespace", "uri/3")
          .SetSchema("Message")
          .AddStringProperty("body", "message3")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();

  ASSERT_THAT(icing.Put(document1).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document2).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document3).status(), ProtoIsOk());

  // "m" will match all 3 documents
  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  search_spec.set_query("m");

  // None of the documents have usage reports. Result should be in the default
  // reverse insertion order.
  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document3;
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document2;
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document1;

  ScoringSpecProto scoring_spec;
  scoring_spec.set_rank_by(
      ScoringSpecProto::RankingStrategy::USAGE_TYPE1_COUNT);
  SearchResultProto search_result_proto = icing.Search(
      search_spec, scoring_spec, ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, SearchResultShouldBeRankedByUsageTimestamp) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  // Creates 3 test documents
  DocumentProto document1 =
      DocumentBuilder()
          .SetKey("namespace", "uri/1")
          .SetSchema("Message")
          .AddStringProperty("body", "message1")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  DocumentProto document2 =
      DocumentBuilder()
          .SetKey("namespace", "uri/2")
          .SetSchema("Message")
          .AddStringProperty("body", "message2")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  DocumentProto document3 =
      DocumentBuilder()
          .SetKey("namespace", "uri/3")
          .SetSchema("Message")
          .AddStringProperty("body", "message3")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();

  // Intentionally inserts the documents in a different order to eliminate the
  // possibility that the following results are sorted in the default reverse
  // insertion order.
  ASSERT_THAT(icing.Put(document3).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document1).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document2).status(), ProtoIsOk());

  // Report usage for doc2 and doc3. The order will be doc3 > doc2 > doc1 when
  // ranked by USAGE_TYPE1_LAST_USED_TIMESTAMP.
  UsageReport usage_report_doc2 = CreateUsageReport(
      /*name_space=*/"namespace", /*uri=*/"uri/2", /*timestamp_ms=*/1000,
      UsageReport::USAGE_TYPE1);
  UsageReport usage_report_doc3 = CreateUsageReport(
      /*name_space=*/"namespace", /*uri=*/"uri/3", /*timestamp_ms=*/5000,
      UsageReport::USAGE_TYPE1);
  ASSERT_THAT(icing.ReportUsage(usage_report_doc2).status(), ProtoIsOk());
  ASSERT_THAT(icing.ReportUsage(usage_report_doc3).status(), ProtoIsOk());

  // "m" will match all 3 documents
  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  search_spec.set_query("m");

  // Result should be in descending USAGE_TYPE1_LAST_USED_TIMESTAMP order
  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document3;
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document2;
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document1;

  ScoringSpecProto scoring_spec;
  scoring_spec.set_rank_by(
      ScoringSpecProto::RankingStrategy::USAGE_TYPE1_LAST_USED_TIMESTAMP);
  SearchResultProto search_result_proto = icing.Search(
      search_spec, scoring_spec, ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, Bm25fRelevanceScoringOneNamespace) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.SetSchema(CreateEmailSchema()).status(), ProtoIsOk());

  // Create and index documents in namespace "namespace1".
  DocumentProto document = CreateEmailDocument(
      "namespace1", "namespace1/uri0", /*score=*/10, "sushi belmont",
      "fresh fish. inexpensive. good sushi.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument(
      "namespace1", "namespace1/uri1", /*score=*/13, "peacock koriander",
      "indian food. buffet. spicy food. kadai chicken.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument("namespace1", "namespace1/uri2", /*score=*/4,
                                 "panda express",
                                 "chinese food. cheap. inexpensive. kung pao.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument("namespace1", "namespace1/uri3", /*score=*/23,
                                 "speederia pizza",
                                 "thin-crust pizza. good and fast.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument("namespace1", "namespace1/uri4", /*score=*/8,
                                 "whole foods",
                                 "salads. pizza. organic food. expensive.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument(
      "namespace1", "namespace1/uri5", /*score=*/18, "peets coffee",
      "espresso. decaf. brewed coffee. whole beans. excellent coffee.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument(
      "namespace1", "namespace1/uri6", /*score=*/4, "costco",
      "bulk. cheap whole beans. frozen fish. food samples.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument("namespace1", "namespace1/uri7", /*score=*/4,
                                 "starbucks coffee",
                                 "habit. birthday rewards. good coffee");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());

  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);
  search_spec.set_query("coffee OR food");
  ScoringSpecProto scoring_spec = GetDefaultScoringSpec();
  scoring_spec.set_rank_by(ScoringSpecProto::RankingStrategy::RELEVANCE_SCORE);
  SearchResultProto search_result_proto = icing.Search(
      search_spec, scoring_spec, ResultSpecProto::default_instance());

  // Result should be in descending score order
  EXPECT_THAT(search_result_proto.status(), ProtoIsOk());
  // Both doc5 and doc7 have "coffee" in name and text sections.
  // However, doc5 has more matches in the text section.
  // Documents with "food" are ranked lower as the term "food" is commonly
  // present in this corpus, and thus, has a lower IDF.
  EXPECT_THAT(GetUrisFromSearchResults(search_result_proto),
              ElementsAre("namespace1/uri5",    // 'coffee' 3 times
                          "namespace1/uri7",    // 'coffee' 2 times
                          "namespace1/uri1",    // 'food' 2 times
                          "namespace1/uri4",    // 'food' 2 times
                          "namespace1/uri2",    // 'food' 1 time
                          "namespace1/uri6"));  // 'food' 1 time
}

TEST_F(IcingSearchEngineTest, Bm25fRelevanceScoringOneNamespaceNotOperator) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.SetSchema(CreateEmailSchema()).status(), ProtoIsOk());

  // Create and index documents in namespace "namespace1".
  DocumentProto document = CreateEmailDocument(
      "namespace1", "namespace1/uri0", /*score=*/10, "sushi belmont",
      "fresh fish. inexpensive. good sushi.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument(
      "namespace1", "namespace1/uri1", /*score=*/13, "peacock koriander",
      "indian food. buffet. spicy food. kadai chicken.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument("namespace1", "namespace1/uri2", /*score=*/4,
                                 "panda express",
                                 "chinese food. cheap. inexpensive. kung pao.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument(
      "namespace1", "namespace1/uri3", /*score=*/23, "speederia pizza",
      "thin-crust pizza. good and fast. nice coffee");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument("namespace1", "namespace1/uri4", /*score=*/8,
                                 "whole foods",
                                 "salads. pizza. organic food. expensive.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument(
      "namespace1", "namespace1/uri5", /*score=*/18, "peets coffee",
      "espresso. decaf. brewed coffee. whole beans. excellent coffee.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument(
      "namespace1", "namespace1/uri6", /*score=*/4, "costco",
      "bulk. cheap whole beans. frozen fish. food samples.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument("namespace1", "namespace1/uri7", /*score=*/4,
                                 "starbucks coffee",
                                 "habit. birthday rewards. good coffee");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());

  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);
  search_spec.set_query("coffee -starbucks");
  ScoringSpecProto scoring_spec = GetDefaultScoringSpec();
  scoring_spec.set_rank_by(ScoringSpecProto::RankingStrategy::RELEVANCE_SCORE);
  SearchResultProto search_result_proto = icing.Search(
      search_spec, scoring_spec, ResultSpecProto::default_instance());

  // Result should be in descending score order
  EXPECT_THAT(search_result_proto.status(), ProtoIsOk());
  EXPECT_THAT(
      GetUrisFromSearchResults(search_result_proto),
      ElementsAre("namespace1/uri5",    // 'coffee' 3 times, 'starbucks' 0 times
                  "namespace1/uri3"));  // 'coffee' 1 times, 'starbucks' 0 times
}

TEST_F(IcingSearchEngineTest,
       Bm25fRelevanceScoringOneNamespaceSectionRestrict) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.SetSchema(CreateEmailSchema()).status(), ProtoIsOk());

  // Create and index documents in namespace "namespace1".
  DocumentProto document = CreateEmailDocument(
      "namespace1", "namespace1/uri0", /*score=*/10, "sushi belmont",
      "fresh fish. inexpensive. good sushi.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument(
      "namespace1", "namespace1/uri1", /*score=*/13, "peacock koriander",
      "indian food. buffet. spicy food. kadai chicken.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument("namespace1", "namespace1/uri2", /*score=*/4,
                                 "panda express",
                                 "chinese food. cheap. inexpensive. kung pao.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument("namespace1", "namespace1/uri3", /*score=*/23,
                                 "speederia pizza",
                                 "thin-crust pizza. good and fast.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument("namespace1", "namespace1/uri4", /*score=*/8,
                                 "whole foods",
                                 "salads. pizza. organic food. expensive.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document =
      CreateEmailDocument("namespace1", "namespace1/uri5", /*score=*/18,
                          "peets coffee, best coffee",
                          "espresso. decaf. whole beans. excellent coffee.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument(
      "namespace1", "namespace1/uri6", /*score=*/4, "costco",
      "bulk. cheap whole beans. frozen fish. food samples.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument(
      "namespace1", "namespace1/uri7", /*score=*/4, "starbucks",
      "habit. birthday rewards. good coffee. brewed coffee");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());

  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);
  search_spec.set_query("subject:coffee OR body:food");
  ScoringSpecProto scoring_spec = GetDefaultScoringSpec();
  scoring_spec.set_rank_by(ScoringSpecProto::RankingStrategy::RELEVANCE_SCORE);
  SearchResultProto search_result_proto = icing.Search(
      search_spec, scoring_spec, ResultSpecProto::default_instance());

  // Result should be in descending score order
  EXPECT_THAT(search_result_proto.status(), ProtoIsOk());
  // The term frequencies of "coffee" and "food" are calculated respectively
  // from the subject section and the body section.
  // Documents with "food" are ranked lower as the term "food" is commonly
  // present in this corpus, and thus, has a lower IDF.
  EXPECT_THAT(
      GetUrisFromSearchResults(search_result_proto),
      ElementsAre("namespace1/uri5",    // 'coffee' 2 times in section subject
                  "namespace1/uri1",    // 'food' 2 times in section body
                  "namespace1/uri4",    // 'food' 2 times in section body
                  "namespace1/uri2",    // 'food' 1 time in section body
                  "namespace1/uri6"));  // 'food' 1 time in section body
}

TEST_F(IcingSearchEngineTest, Bm25fRelevanceScoringTwoNamespaces) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.SetSchema(CreateEmailSchema()).status(), ProtoIsOk());

  // Create and index documents in namespace "namespace1".
  DocumentProto document = CreateEmailDocument(
      "namespace1", "namespace1/uri0", /*score=*/10, "sushi belmont",
      "fresh fish. inexpensive. good sushi.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument(
      "namespace1", "namespace1/uri1", /*score=*/13, "peacock koriander",
      "indian food. buffet. spicy food. kadai chicken.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument("namespace1", "namespace1/uri2", /*score=*/4,
                                 "panda express",
                                 "chinese food. cheap. inexpensive. kung pao.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument("namespace1", "namespace1/uri3", /*score=*/23,
                                 "speederia pizza",
                                 "thin-crust pizza. good and fast.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument("namespace1", "namespace1/uri4", /*score=*/8,
                                 "whole foods",
                                 "salads. pizza. organic food. expensive.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument(
      "namespace1", "namespace1/uri5", /*score=*/18, "peets coffee",
      "espresso. decaf. brewed coffee. whole beans. excellent coffee.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument(
      "namespace1", "namespace1/uri6", /*score=*/4, "costco",
      "bulk. cheap whole beans. frozen fish. food samples.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument("namespace1", "namespace1/uri7", /*score=*/4,
                                 "starbucks coffee",
                                 "habit. birthday rewards. good coffee");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());

  // Create and index documents in namespace "namespace2".
  document = CreateEmailDocument("namespace2", "namespace2/uri0", /*score=*/10,
                                 "sushi belmont",
                                 "fresh fish. inexpensive. good sushi.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument(
      "namespace2", "namespace2/uri1", /*score=*/13, "peacock koriander",
      "indian food. buffet. spicy food. kadai chicken.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument("namespace2", "namespace2/uri2", /*score=*/4,
                                 "panda express",
                                 "chinese food. cheap. inexpensive. kung pao.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument("namespace2", "namespace2/uri3", /*score=*/23,
                                 "speederia pizza",
                                 "thin-crust pizza. good and fast.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument("namespace2", "namespace2/uri4", /*score=*/8,
                                 "whole foods",
                                 "salads. pizza. organic food. expensive.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument(
      "namespace2", "namespace2/uri5", /*score=*/18, "peets coffee",
      "espresso. decaf. brewed coffee. whole beans. excellent coffee.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument(
      "namespace2", "namespace2/uri6", /*score=*/4, "costco",
      "bulk. cheap whole beans. frozen fish. food samples.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument("namespace2", "namespace2/uri7", /*score=*/4,
                                 "starbucks coffee", "good coffee");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());

  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);
  search_spec.set_query("coffee OR food");
  ScoringSpecProto scoring_spec = GetDefaultScoringSpec();
  scoring_spec.set_rank_by(ScoringSpecProto::RankingStrategy::RELEVANCE_SCORE);
  ResultSpecProto result_spec_proto;
  result_spec_proto.set_num_per_page(16);
  SearchResultProto search_result_proto =
      icing.Search(search_spec, scoring_spec, result_spec_proto);

  // Result should be in descending score order
  EXPECT_THAT(search_result_proto.status(), ProtoIsOk());
  // The two corpora have the same documents except for document 7, which in
  // "namespace2" is much shorter than the average dcoument length, so it is
  // boosted.
  EXPECT_THAT(GetUrisFromSearchResults(search_result_proto),
              ElementsAre("namespace2/uri7",    // 'coffee' 2 times, short doc
                          "namespace1/uri5",    // 'coffee' 3 times
                          "namespace2/uri5",    // 'coffee' 3 times
                          "namespace1/uri7",    // 'coffee' 2 times
                          "namespace1/uri1",    // 'food' 2 times
                          "namespace2/uri1",    // 'food' 2 times
                          "namespace1/uri4",    // 'food' 2 times
                          "namespace2/uri4",    // 'food' 2 times
                          "namespace1/uri2",    // 'food' 1 time
                          "namespace2/uri2",    // 'food' 1 time
                          "namespace1/uri6",    // 'food' 1 time
                          "namespace2/uri6"));  // 'food' 1 time
}

TEST_F(IcingSearchEngineTest, Bm25fRelevanceScoringWithNamespaceFilter) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.SetSchema(CreateEmailSchema()).status(), ProtoIsOk());

  // Create and index documents in namespace "namespace1".
  DocumentProto document = CreateEmailDocument(
      "namespace1", "namespace1/uri0", /*score=*/10, "sushi belmont",
      "fresh fish. inexpensive. good sushi.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument(
      "namespace1", "namespace1/uri1", /*score=*/13, "peacock koriander",
      "indian food. buffet. spicy food. kadai chicken.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument("namespace1", "namespace1/uri2", /*score=*/4,
                                 "panda express",
                                 "chinese food. cheap. inexpensive. kung pao.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument("namespace1", "namespace1/uri3", /*score=*/23,
                                 "speederia pizza",
                                 "thin-crust pizza. good and fast.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument("namespace1", "namespace1/uri4", /*score=*/8,
                                 "whole foods",
                                 "salads. pizza. organic food. expensive.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument(
      "namespace1", "namespace1/uri5", /*score=*/18, "peets coffee",
      "espresso. decaf. brewed coffee. whole beans. excellent coffee.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument(
      "namespace1", "namespace1/uri6", /*score=*/4, "costco",
      "bulk. cheap whole beans. frozen fish. food samples.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument("namespace1", "namespace1/uri7", /*score=*/4,
                                 "starbucks coffee",
                                 "habit. birthday rewards. good coffee");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());

  // Create and index documents in namespace "namespace2".
  document = CreateEmailDocument("namespace2", "namespace2/uri0", /*score=*/10,
                                 "sushi belmont",
                                 "fresh fish. inexpensive. good sushi.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument(
      "namespace2", "namespace2/uri1", /*score=*/13, "peacock koriander",
      "indian food. buffet. spicy food. kadai chicken.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument("namespace2", "namespace2/uri2", /*score=*/4,
                                 "panda express",
                                 "chinese food. cheap. inexpensive. kung pao.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument("namespace2", "namespace2/uri3", /*score=*/23,
                                 "speederia pizza",
                                 "thin-crust pizza. good and fast.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument("namespace2", "namespace2/uri4", /*score=*/8,
                                 "whole foods",
                                 "salads. pizza. organic food. expensive.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument(
      "namespace2", "namespace2/uri5", /*score=*/18, "peets coffee",
      "espresso. decaf. brewed coffee. whole beans. excellent coffee.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument(
      "namespace2", "namespace2/uri6", /*score=*/4, "costco",
      "bulk. cheap whole beans. frozen fish. food samples.");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  document = CreateEmailDocument("namespace2", "namespace2/uri7", /*score=*/4,
                                 "starbucks coffee", "good coffee");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());

  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);
  search_spec.set_query("coffee OR food");
  // Now query only corpus 2
  search_spec.add_namespace_filters("namespace2");
  ScoringSpecProto scoring_spec = GetDefaultScoringSpec();
  scoring_spec.set_rank_by(ScoringSpecProto::RankingStrategy::RELEVANCE_SCORE);
  SearchResultProto search_result_proto = icing.Search(
      search_spec, scoring_spec, ResultSpecProto::default_instance());
  search_result_proto = icing.Search(search_spec, scoring_spec,
                                     ResultSpecProto::default_instance());

  // Result from namespace "namespace2" should be in descending score order
  EXPECT_THAT(search_result_proto.status(), ProtoIsOk());
  // Both doc5 and doc7 have "coffee" in name and text sections.
  // Even though doc5 has more matches in the text section, doc7's length is
  // much shorter than the average corpus's length, so it's being boosted.
  // Documents with "food" are ranked lower as the term "food" is commonly
  // present in this corpus, and thus, has a lower IDF.
  EXPECT_THAT(GetUrisFromSearchResults(search_result_proto),
              ElementsAre("namespace2/uri7",    // 'coffee' 2 times, short doc
                          "namespace2/uri5",    // 'coffee' 3 times
                          "namespace2/uri1",    // 'food' 2 times
                          "namespace2/uri4",    // 'food' 2 times
                          "namespace2/uri2",    // 'food' 1 time
                          "namespace2/uri6"));  // 'food' 1 time
}

TEST_F(IcingSearchEngineTest,
       SearchResultShouldHaveDefaultOrderWithoutUsageTimestamp) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  // Creates 3 test documents
  DocumentProto document1 =
      DocumentBuilder()
          .SetKey("namespace", "uri/1")
          .SetSchema("Message")
          .AddStringProperty("body", "message1")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  DocumentProto document2 =
      DocumentBuilder()
          .SetKey("namespace", "uri/2")
          .SetSchema("Message")
          .AddStringProperty("body", "message2")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  DocumentProto document3 =
      DocumentBuilder()
          .SetKey("namespace", "uri/3")
          .SetSchema("Message")
          .AddStringProperty("body", "message3")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();

  ASSERT_THAT(icing.Put(document1).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document2).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document3).status(), ProtoIsOk());

  // "m" will match all 3 documents
  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  search_spec.set_query("m");

  // None of the documents have usage reports. Result should be in the default
  // reverse insertion order.
  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document3;
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document2;
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document1;

  ScoringSpecProto scoring_spec;
  scoring_spec.set_rank_by(
      ScoringSpecProto::RankingStrategy::USAGE_TYPE1_LAST_USED_TIMESTAMP);
  SearchResultProto search_result_proto = icing.Search(
      search_spec, scoring_spec, ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, OlderUsageTimestampShouldNotOverrideNewerOnes) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  // Creates 3 test documents
  DocumentProto document1 =
      DocumentBuilder()
          .SetKey("namespace", "uri/1")
          .SetSchema("Message")
          .AddStringProperty("body", "message1")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  DocumentProto document2 =
      DocumentBuilder()
          .SetKey("namespace", "uri/2")
          .SetSchema("Message")
          .AddStringProperty("body", "message2")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  DocumentProto document3 =
      DocumentBuilder()
          .SetKey("namespace", "uri/3")
          .SetSchema("Message")
          .AddStringProperty("body", "message3")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();

  ASSERT_THAT(icing.Put(document1).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document2).status(), ProtoIsOk());

  // Report usage for doc1 and doc2. The older timestamp 5000 shouldn't be
  // overridden by 1000. The order will be doc1 > doc2 when ranked by
  // USAGE_TYPE1_LAST_USED_TIMESTAMP.
  UsageReport usage_report_doc1_time1 = CreateUsageReport(
      /*name_space=*/"namespace", /*uri=*/"uri/1", /*timestamp_ms=*/1000,
      UsageReport::USAGE_TYPE1);
  UsageReport usage_report_doc1_time5 = CreateUsageReport(
      /*name_space=*/"namespace", /*uri=*/"uri/1", /*timestamp_ms=*/5000,
      UsageReport::USAGE_TYPE1);
  UsageReport usage_report_doc2_time3 = CreateUsageReport(
      /*name_space=*/"namespace", /*uri=*/"uri/2", /*timestamp_ms=*/3000,
      UsageReport::USAGE_TYPE1);
  ASSERT_THAT(icing.ReportUsage(usage_report_doc1_time5).status(), ProtoIsOk());
  ASSERT_THAT(icing.ReportUsage(usage_report_doc2_time3).status(), ProtoIsOk());
  ASSERT_THAT(icing.ReportUsage(usage_report_doc1_time1).status(), ProtoIsOk());

  // "m" will match both documents
  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  search_spec.set_query("m");

  // Result should be in descending USAGE_TYPE1_LAST_USED_TIMESTAMP order
  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document1;
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document2;

  ScoringSpecProto scoring_spec;
  scoring_spec.set_rank_by(
      ScoringSpecProto::RankingStrategy::USAGE_TYPE1_LAST_USED_TIMESTAMP);
  SearchResultProto search_result_proto = icing.Search(
      search_spec, scoring_spec, ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest, SearchResultShouldBeRankedAscendingly) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  // Creates 3 documents and ensures the relationship in terms of document
  // score is: document1 < document2 < document3
  DocumentProto document1 =
      DocumentBuilder()
          .SetKey("namespace", "uri/1")
          .SetSchema("Message")
          .AddStringProperty("body", "message1")
          .SetScore(1)
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  DocumentProto document2 =
      DocumentBuilder()
          .SetKey("namespace", "uri/2")
          .SetSchema("Message")
          .AddStringProperty("body", "message2")
          .SetScore(2)
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  DocumentProto document3 =
      DocumentBuilder()
          .SetKey("namespace", "uri/3")
          .SetSchema("Message")
          .AddStringProperty("body", "message3")
          .SetScore(3)
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();

  // Intentionally inserts the documents in the order that is different than
  // their score order
  ASSERT_THAT(icing.Put(document2).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document3).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document1).status(), ProtoIsOk());

  // "m" will match all 3 documents
  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  search_spec.set_query("m");

  // Result should be in ascending score order
  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document1;
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document2;
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document3;

  ScoringSpecProto scoring_spec = GetDefaultScoringSpec();
  scoring_spec.set_rank_by(ScoringSpecProto::RankingStrategy::DOCUMENT_SCORE);
  scoring_spec.set_order_by(ScoringSpecProto::Order::ASC);
  SearchResultProto search_result_proto = icing.Search(
      search_spec, scoring_spec, ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest,
       SearchResultGroupingDuplicateNamespaceShouldReturnError) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  // Creates 2 documents and ensures the relationship in terms of document
  // score is: document1 < document2
  DocumentProto document1 =
      DocumentBuilder()
          .SetKey("namespace1", "uri/1")
          .SetSchema("Message")
          .AddStringProperty("body", "message1")
          .SetScore(1)
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  DocumentProto document2 =
      DocumentBuilder()
          .SetKey("namespace2", "uri/2")
          .SetSchema("Message")
          .AddStringProperty("body", "message2")
          .SetScore(2)
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();

  ASSERT_THAT(icing.Put(document1).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document2).status(), ProtoIsOk());

  // "m" will match all 2 documents
  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  search_spec.set_query("m");

  ScoringSpecProto scoring_spec = GetDefaultScoringSpec();
  scoring_spec.set_rank_by(ScoringSpecProto::RankingStrategy::DOCUMENT_SCORE);

  // Specify "namespace1" twice. This should result in an error.
  ResultSpecProto result_spec;
  ResultSpecProto::ResultGrouping* result_grouping =
      result_spec.add_result_groupings();
  result_grouping->set_max_results(1);
  result_grouping->add_namespaces("namespace1");
  result_grouping->add_namespaces("namespace2");
  result_grouping = result_spec.add_result_groupings();
  result_grouping->set_max_results(1);
  result_grouping->add_namespaces("namespace1");

  SearchResultProto search_result_proto =
      icing.Search(search_spec, scoring_spec, result_spec);
  EXPECT_THAT(search_result_proto.status(),
              ProtoStatusIs(StatusProto::INVALID_ARGUMENT));
}

TEST_F(IcingSearchEngineTest,
       SearchResultGroupingNonPositiveMaxResultsShouldReturnError) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  // Creates 2 documents and ensures the relationship in terms of document
  // score is: document1 < document2
  DocumentProto document1 =
      DocumentBuilder()
          .SetKey("namespace1", "uri/1")
          .SetSchema("Message")
          .AddStringProperty("body", "message1")
          .SetScore(1)
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  DocumentProto document2 =
      DocumentBuilder()
          .SetKey("namespace2", "uri/2")
          .SetSchema("Message")
          .AddStringProperty("body", "message2")
          .SetScore(2)
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();

  ASSERT_THAT(icing.Put(document1).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document2).status(), ProtoIsOk());

  // "m" will match all 2 documents
  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  search_spec.set_query("m");

  ScoringSpecProto scoring_spec = GetDefaultScoringSpec();
  scoring_spec.set_rank_by(ScoringSpecProto::RankingStrategy::DOCUMENT_SCORE);

  // Specify zero results. This should result in an error.
  ResultSpecProto result_spec;
  ResultSpecProto::ResultGrouping* result_grouping =
      result_spec.add_result_groupings();
  result_grouping->set_max_results(0);
  result_grouping->add_namespaces("namespace1");
  result_grouping->add_namespaces("namespace2");

  SearchResultProto search_result_proto =
      icing.Search(search_spec, scoring_spec, result_spec);
  EXPECT_THAT(search_result_proto.status(),
              ProtoStatusIs(StatusProto::INVALID_ARGUMENT));

  // Specify negative results. This should result in an error.
  result_spec.mutable_result_groupings(0)->set_max_results(-1);
  EXPECT_THAT(search_result_proto.status(),
              ProtoStatusIs(StatusProto::INVALID_ARGUMENT));
}

TEST_F(IcingSearchEngineTest, SearchResultGroupingMultiNamespaceGrouping) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  // Creates 3 documents and ensures the relationship in terms of document
  // score is: document1 < document2 < document3 < document4 < document5 <
  // document6
  DocumentProto document1 =
      DocumentBuilder()
          .SetKey("namespace1", "uri/1")
          .SetSchema("Message")
          .AddStringProperty("body", "message1")
          .SetScore(1)
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  DocumentProto document2 =
      DocumentBuilder()
          .SetKey("namespace1", "uri/2")
          .SetSchema("Message")
          .AddStringProperty("body", "message2")
          .SetScore(2)
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  DocumentProto document3 =
      DocumentBuilder()
          .SetKey("namespace2", "uri/3")
          .SetSchema("Message")
          .AddStringProperty("body", "message3")
          .SetScore(3)
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  DocumentProto document4 =
      DocumentBuilder()
          .SetKey("namespace2", "uri/4")
          .SetSchema("Message")
          .AddStringProperty("body", "message1")
          .SetScore(4)
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  DocumentProto document5 =
      DocumentBuilder()
          .SetKey("namespace3", "uri/5")
          .SetSchema("Message")
          .AddStringProperty("body", "message3")
          .SetScore(5)
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  DocumentProto document6 =
      DocumentBuilder()
          .SetKey("namespace3", "uri/6")
          .SetSchema("Message")
          .AddStringProperty("body", "message1")
          .SetScore(6)
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();

  ASSERT_THAT(icing.Put(document1).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document2).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document3).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document4).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document5).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document6).status(), ProtoIsOk());

  // "m" will match all 6 documents
  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  search_spec.set_query("m");

  ScoringSpecProto scoring_spec = GetDefaultScoringSpec();
  scoring_spec.set_rank_by(ScoringSpecProto::RankingStrategy::DOCUMENT_SCORE);

  ResultSpecProto result_spec;
  ResultSpecProto::ResultGrouping* result_grouping =
      result_spec.add_result_groupings();
  result_grouping->set_max_results(1);
  result_grouping->add_namespaces("namespace1");
  result_grouping = result_spec.add_result_groupings();
  result_grouping->set_max_results(2);
  result_grouping->add_namespaces("namespace2");
  result_grouping->add_namespaces("namespace3");

  SearchResultProto search_result_proto =
      icing.Search(search_spec, scoring_spec, result_spec);

  // The last result (document1) in namespace "namespace1" should not be
  // included. "namespace2" and "namespace3" are grouped together. So only the
  // two highest scored documents between the two (both of which are in
  // "namespace3") should be returned.
  SearchResultProto expected_search_result_proto;
  expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document6;
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document5;
  *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
      document2;

  EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                       expected_search_result_proto));
}

TEST_F(IcingSearchEngineTest,
       SetSchemaCanNotDetectPreviousSchemaWasLostWithoutDocuments) {
  SchemaProto schema;
  auto type = schema.add_types();
  type->set_schema_type("Message");

  auto body = type->add_properties();
  body->set_property_name("body");
  body->set_data_type(PropertyConfigProto::DataType::STRING);
  body->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);

  // Make an incompatible schema, a previously OPTIONAL field is REQUIRED
  SchemaProto incompatible_schema = schema;
  incompatible_schema.mutable_types(0)->mutable_properties(0)->set_cardinality(
      PropertyConfigProto::Cardinality::REQUIRED);

  {
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
    ASSERT_THAT(icing.SetSchema(schema).status(), ProtoIsOk());
  }  // This should shut down IcingSearchEngine and persist anything it needs to

  ASSERT_TRUE(filesystem()->DeleteDirectoryRecursively(GetSchemaDir().c_str()));

  // Since we don't have any documents yet, we can't detect this edge-case.  But
  // it should be fine since there aren't any documents to be invalidated.
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.SetSchema(incompatible_schema).status(), ProtoIsOk());
}

TEST_F(IcingSearchEngineTest, SetSchemaCanDetectPreviousSchemaWasLost) {
  SchemaProto schema;
  auto type = schema.add_types();
  type->set_schema_type("Message");

  auto body = type->add_properties();
  body->set_property_name("body");
  body->set_data_type(PropertyConfigProto::DataType::STRING);
  body->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);
  body->mutable_string_indexing_config()->set_term_match_type(
      TermMatchType::PREFIX);
  body->mutable_string_indexing_config()->set_tokenizer_type(
      StringIndexingConfig::TokenizerType::PLAIN);

  // Make an incompatible schema, a previously OPTIONAL field is REQUIRED
  SchemaProto incompatible_schema = schema;
  incompatible_schema.mutable_types(0)->mutable_properties(0)->set_cardinality(
      PropertyConfigProto::Cardinality::REQUIRED);

  SearchSpecProto search_spec;
  search_spec.set_query("message");
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);

  {
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
    ASSERT_THAT(icing.SetSchema(schema).status(), ProtoIsOk());

    DocumentProto document = CreateMessageDocument("namespace", "uri");
    ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());

    // Can retrieve by namespace/uri
    GetResultProto expected_get_result_proto;
    expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
    *expected_get_result_proto.mutable_document() = document;

    ASSERT_THAT(
        icing.Get("namespace", "uri", GetResultSpecProto::default_instance()),
        EqualsProto(expected_get_result_proto));

    // Can search for it
    SearchResultProto expected_search_result_proto;
    expected_search_result_proto.mutable_status()->set_code(StatusProto::OK);
    *expected_search_result_proto.mutable_results()->Add()->mutable_document() =
        CreateMessageDocument("namespace", "uri");
    SearchResultProto search_result_proto =
        icing.Search(search_spec, GetDefaultScoringSpec(),
                     ResultSpecProto::default_instance());
    EXPECT_THAT(search_result_proto, EqualsSearchResultIgnoreStatsAndScores(
                                         expected_search_result_proto));
  }  // This should shut down IcingSearchEngine and persist anything it needs to

  ASSERT_TRUE(filesystem()->DeleteDirectoryRecursively(GetSchemaDir().c_str()));

  // Setting the new, different schema will remove incompatible documents
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing.SetSchema(incompatible_schema).status(), ProtoIsOk());

  // Can't retrieve by namespace/uri
  GetResultProto expected_get_result_proto;
  expected_get_result_proto.mutable_status()->set_code(StatusProto::NOT_FOUND);
  expected_get_result_proto.mutable_status()->set_message(
      "Document (namespace, uri) not found.");

  EXPECT_THAT(
      icing.Get("namespace", "uri", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));

  // Can't search for it
  SearchResultProto empty_result;
  empty_result.mutable_status()->set_code(StatusProto::OK);
  SearchResultProto search_result_proto =
      icing.Search(search_spec, GetDefaultScoringSpec(),
                   ResultSpecProto::default_instance());
  EXPECT_THAT(search_result_proto,
              EqualsSearchResultIgnoreStatsAndScores(empty_result));
}

TEST_F(IcingSearchEngineTest, PersistToDisk) {
  GetResultProto expected_get_result_proto;
  expected_get_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_get_result_proto.mutable_document() =
      CreateMessageDocument("namespace", "uri");

  {
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
    EXPECT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
    EXPECT_THAT(icing.Put(CreateMessageDocument("namespace", "uri")).status(),
                ProtoIsOk());

    // Persisting shouldn't affect anything
    EXPECT_THAT(icing.PersistToDisk(PersistType::FULL).status(), ProtoIsOk());

    EXPECT_THAT(
        icing.Get("namespace", "uri", GetResultSpecProto::default_instance()),
        EqualsProto(expected_get_result_proto));
  }  // Destructing persists as well

  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(
      icing.Get("namespace", "uri", GetResultSpecProto::default_instance()),
      EqualsProto(expected_get_result_proto));
}

TEST_F(IcingSearchEngineTest, NoPersistToDiskLiteDoesntPersistPut) {
  IcingSearchEngine icing1(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing1.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing1.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
  DocumentProto document1 = CreateMessageDocument("namespace", "uri");
  EXPECT_THAT(icing1.Put(document1).status(), ProtoIsOk());
  EXPECT_THAT(
      icing1.Get("namespace", "uri", GetResultSpecProto::default_instance())
          .document(),
      EqualsProto(document1));

  IcingSearchEngine icing2(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing2.Initialize().status(), ProtoIsOk());
  // The document shouldn't be found because we forgot to call
  // PersistToDisk(LITE)!
  EXPECT_THAT(
      icing2.Get("namespace", "uri", GetResultSpecProto::default_instance())
          .status(),
      ProtoStatusIs(StatusProto::NOT_FOUND));
}

TEST_F(IcingSearchEngineTest, PersistToDiskLitePersistsPut) {
  IcingSearchEngine icing1(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing1.Initialize().status(), ProtoIsOk());
  EXPECT_THAT(icing1.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
  DocumentProto document1 = CreateMessageDocument("namespace", "uri");
  EXPECT_THAT(icing1.Put(document1).status(), ProtoIsOk());
  EXPECT_THAT(icing1.PersistToDisk(PersistType::LITE).status(), ProtoIsOk());
  EXPECT_THAT(
      icing1.Get("namespace", "uri", GetResultSpecProto::default_instance())
          .document(),
      EqualsProto(document1));

  IcingSearchEngine icing2(GetDefaultIcingOptions(), GetTestJniCache());
  EXPECT_THAT(icing2.Initialize().status(), ProtoIsOk());
  // The document should be found because we called PersistToDisk(LITE)!
  EXPECT_THAT(
      icing2.Get("namespace", "uri", GetResultSpecProto::default_instance())
          .document(),
      EqualsProto(document1));
}

TEST_F(IcingSearchEngineTest, ResetOk) {
  SchemaProto message_schema = CreateMessageSchema();
  SchemaProto empty_schema = SchemaProto(message_schema);
  empty_schema.clear_types();

  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(message_schema).status(), ProtoIsOk());

  int64_t empty_state_size =
      filesystem()->GetFileDiskUsage(GetTestBaseDir().c_str());

  DocumentProto document = CreateMessageDocument("namespace", "uri");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());

  // Check that things have been added
  EXPECT_THAT(filesystem()->GetDiskUsage(GetTestBaseDir().c_str()),
              Gt(empty_state_size));

  EXPECT_THAT(icing.Reset().status(), ProtoIsOk());

  // Check that we're back to an empty state
  EXPECT_EQ(filesystem()->GetFileDiskUsage(GetTestBaseDir().c_str()),
            empty_state_size);

  // Sanity check that we can still call other APIs. If things aren't cleared,
  // then this should raise an error since the empty schema is incompatible with
  // the old message_schema.
  EXPECT_THAT(icing.SetSchema(empty_schema).status(), ProtoIsOk());
}

TEST_F(IcingSearchEngineTest, ResetDeleteFailureCausesInternalError) {
  auto mock_filesystem = std::make_unique<MockFilesystem>();

  // This fails IcingSearchEngine::Reset() with status code INTERNAL and leaves
  // the IcingSearchEngine instance in an uninitialized state.
  ON_CALL(*mock_filesystem,
          DeleteDirectoryRecursively(StrEq(GetTestBaseDir().c_str())))
      .WillByDefault(Return(false));

  TestIcingSearchEngine icing(GetDefaultIcingOptions(),
                              std::move(mock_filesystem),
                              std::make_unique<IcingFilesystem>(),
                              std::make_unique<FakeClock>(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  DocumentProto document = CreateMessageDocument("namespace", "uri");
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  EXPECT_THAT(icing.Reset().status(), ProtoStatusIs(StatusProto::INTERNAL));

  GetResultProto expected_get_result_proto;
  expected_get_result_proto.mutable_status()->set_code(
      StatusProto::FAILED_PRECONDITION);
  *expected_get_result_proto.mutable_document() = document;
  EXPECT_THAT(icing
                  .Get(document.namespace_(), document.uri(),
                       GetResultSpecProto::default_instance())
                  .status(),
              ProtoStatusIs(StatusProto::FAILED_PRECONDITION));
}

TEST_F(IcingSearchEngineTest, SnippetNormalization) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  DocumentProto document_one =
      DocumentBuilder()
          .SetKey("namespace", "uri1")
          .SetSchema("Message")
          .AddStringProperty("body", "MDI zurich Team Meeting")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  ASSERT_THAT(icing.Put(document_one).status(), ProtoIsOk());

  DocumentProto document_two =
      DocumentBuilder()
          .SetKey("namespace", "uri2")
          .SetSchema("Message")
          .AddStringProperty("body", "mdi Zürich Team Meeting")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  ASSERT_THAT(icing.Put(document_two).status(), ProtoIsOk());

  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);
  search_spec.set_query("mdi Zürich");

  ResultSpecProto result_spec;
  result_spec.mutable_snippet_spec()->set_max_window_bytes(64);
  result_spec.mutable_snippet_spec()->set_num_matches_per_property(2);
  result_spec.mutable_snippet_spec()->set_num_to_snippet(2);

  SearchResultProto results =
      icing.Search(search_spec, GetDefaultScoringSpec(), result_spec);
  EXPECT_THAT(results.status(), ProtoIsOk());
  ASSERT_THAT(results.results(), SizeIs(2));
  const DocumentProto& result_document_1 = results.results(0).document();
  const SnippetProto& result_snippet_1 = results.results(0).snippet();
  EXPECT_THAT(result_document_1, EqualsProto(document_two));
  EXPECT_THAT(result_snippet_1.entries(), SizeIs(1));
  EXPECT_THAT(result_snippet_1.entries(0).property_name(), Eq("body"));
  std::string_view content = GetString(
      &result_document_1, result_snippet_1.entries(0).property_name());
  EXPECT_THAT(
      GetWindows(content, result_snippet_1.entries(0)),
      ElementsAre("mdi Zürich Team Meeting", "mdi Zürich Team Meeting"));
  EXPECT_THAT(GetMatches(content, result_snippet_1.entries(0)),
              ElementsAre("mdi", "Zürich"));

  const DocumentProto& result_document_2 = results.results(1).document();
  const SnippetProto& result_snippet_2 = results.results(1).snippet();
  EXPECT_THAT(result_document_2, EqualsProto(document_one));
  EXPECT_THAT(result_snippet_2.entries(), SizeIs(1));
  EXPECT_THAT(result_snippet_2.entries(0).property_name(), Eq("body"));
  content = GetString(&result_document_2,
                      result_snippet_2.entries(0).property_name());
  EXPECT_THAT(
      GetWindows(content, result_snippet_2.entries(0)),
      ElementsAre("MDI zurich Team Meeting", "MDI zurich Team Meeting"));
  EXPECT_THAT(GetMatches(content, result_snippet_2.entries(0)),
              ElementsAre("MDI", "zurich"));
}

TEST_F(IcingSearchEngineTest, SnippetNormalizationPrefix) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  DocumentProto document_one =
      DocumentBuilder()
          .SetKey("namespace", "uri1")
          .SetSchema("Message")
          .AddStringProperty("body", "MDI zurich Team Meeting")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  ASSERT_THAT(icing.Put(document_one).status(), ProtoIsOk());

  DocumentProto document_two =
      DocumentBuilder()
          .SetKey("namespace", "uri2")
          .SetSchema("Message")
          .AddStringProperty("body", "mdi Zürich Team Meeting")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  ASSERT_THAT(icing.Put(document_two).status(), ProtoIsOk());

  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  search_spec.set_query("md Zür");

  ResultSpecProto result_spec;
  result_spec.mutable_snippet_spec()->set_max_window_bytes(64);
  result_spec.mutable_snippet_spec()->set_num_matches_per_property(2);
  result_spec.mutable_snippet_spec()->set_num_to_snippet(2);

  SearchResultProto results =
      icing.Search(search_spec, GetDefaultScoringSpec(), result_spec);
  EXPECT_THAT(results.status(), ProtoIsOk());
  ASSERT_THAT(results.results(), SizeIs(2));
  const DocumentProto& result_document_1 = results.results(0).document();
  const SnippetProto& result_snippet_1 = results.results(0).snippet();
  EXPECT_THAT(result_document_1, EqualsProto(document_two));
  EXPECT_THAT(result_snippet_1.entries(), SizeIs(1));
  EXPECT_THAT(result_snippet_1.entries(0).property_name(), Eq("body"));
  std::string_view content = GetString(
      &result_document_1, result_snippet_1.entries(0).property_name());
  EXPECT_THAT(
      GetWindows(content, result_snippet_1.entries(0)),
      ElementsAre("mdi Zürich Team Meeting", "mdi Zürich Team Meeting"));
  EXPECT_THAT(GetMatches(content, result_snippet_1.entries(0)),
              ElementsAre("mdi", "Zürich"));

  const DocumentProto& result_document_2 = results.results(1).document();
  const SnippetProto& result_snippet_2 = results.results(1).snippet();
  EXPECT_THAT(result_document_2, EqualsProto(document_one));
  EXPECT_THAT(result_snippet_2.entries(), SizeIs(1));
  EXPECT_THAT(result_snippet_2.entries(0).property_name(), Eq("body"));
  content = GetString(&result_document_2,
                      result_snippet_2.entries(0).property_name());
  EXPECT_THAT(
      GetWindows(content, result_snippet_2.entries(0)),
      ElementsAre("MDI zurich Team Meeting", "MDI zurich Team Meeting"));
  EXPECT_THAT(GetMatches(content, result_snippet_2.entries(0)),
              ElementsAre("MDI", "zurich"));
}

TEST_F(IcingSearchEngineTest, SnippetSectionRestrict) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreateEmailSchema()).status(), ProtoIsOk());

  DocumentProto document_one =
      DocumentBuilder()
          .SetKey("namespace", "uri1")
          .SetSchema("Email")
          .AddStringProperty("subject", "MDI zurich Team Meeting")
          .AddStringProperty("body", "MDI zurich Team Meeting")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  ASSERT_THAT(icing.Put(document_one).status(), ProtoIsOk());

  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  search_spec.set_query("body:Zür");

  ResultSpecProto result_spec;
  result_spec.mutable_snippet_spec()->set_max_window_bytes(64);
  result_spec.mutable_snippet_spec()->set_num_matches_per_property(10);
  result_spec.mutable_snippet_spec()->set_num_to_snippet(10);

  SearchResultProto results =
      icing.Search(search_spec, GetDefaultScoringSpec(), result_spec);
  EXPECT_THAT(results.status(), ProtoIsOk());
  ASSERT_THAT(results.results(), SizeIs(1));

  const DocumentProto& result_document = results.results(0).document();
  const SnippetProto& result_snippet = results.results(0).snippet();
  EXPECT_THAT(result_document, EqualsProto(document_one));
  EXPECT_THAT(result_snippet.entries(), SizeIs(1));
  EXPECT_THAT(result_snippet.entries(0).property_name(), Eq("body"));
  std::string_view content =
      GetString(&result_document, result_snippet.entries(0).property_name());
  EXPECT_THAT(GetWindows(content, result_snippet.entries(0)),
              ElementsAre("MDI zurich Team Meeting"));
  EXPECT_THAT(GetMatches(content, result_snippet.entries(0)),
              ElementsAre("zurich"));
}

TEST_F(IcingSearchEngineTest, UninitializedInstanceFailsSafely) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());

  SchemaProto email_schema = CreateMessageSchema();
  EXPECT_THAT(icing.SetSchema(email_schema).status(),
              ProtoStatusIs(StatusProto::FAILED_PRECONDITION));
  EXPECT_THAT(icing.GetSchema().status(),
              ProtoStatusIs(StatusProto::FAILED_PRECONDITION));
  EXPECT_THAT(icing.GetSchemaType(email_schema.types(0).schema_type()).status(),
              ProtoStatusIs(StatusProto::FAILED_PRECONDITION));

  DocumentProto doc = CreateMessageDocument("namespace", "uri");
  EXPECT_THAT(icing.Put(doc).status(),
              ProtoStatusIs(StatusProto::FAILED_PRECONDITION));
  EXPECT_THAT(icing
                  .Get(doc.namespace_(), doc.uri(),
                       GetResultSpecProto::default_instance())
                  .status(),
              ProtoStatusIs(StatusProto::FAILED_PRECONDITION));
  EXPECT_THAT(icing.Delete(doc.namespace_(), doc.uri()).status(),
              ProtoStatusIs(StatusProto::FAILED_PRECONDITION));
  EXPECT_THAT(icing.DeleteByNamespace(doc.namespace_()).status(),
              ProtoStatusIs(StatusProto::FAILED_PRECONDITION));
  EXPECT_THAT(icing.DeleteBySchemaType(email_schema.types(0).schema_type())
                  .status()
                  .code(),
              Eq(StatusProto::FAILED_PRECONDITION));

  SearchSpecProto search_spec = SearchSpecProto::default_instance();
  ScoringSpecProto scoring_spec = ScoringSpecProto::default_instance();
  ResultSpecProto result_spec = ResultSpecProto::default_instance();
  EXPECT_THAT(icing.Search(search_spec, scoring_spec, result_spec).status(),
              ProtoStatusIs(StatusProto::FAILED_PRECONDITION));
  constexpr int kSomePageToken = 12;
  EXPECT_THAT(icing.GetNextPage(kSomePageToken).status(),
              ProtoStatusIs(StatusProto::FAILED_PRECONDITION));
  icing.InvalidateNextPageToken(kSomePageToken);  // Verify this doesn't crash.

  EXPECT_THAT(icing.PersistToDisk(PersistType::FULL).status(),
              ProtoStatusIs(StatusProto::FAILED_PRECONDITION));
  EXPECT_THAT(icing.Optimize().status(),
              ProtoStatusIs(StatusProto::FAILED_PRECONDITION));
}

TEST_F(IcingSearchEngineTest, GetAllNamespaces) {
  DocumentProto namespace1 = DocumentBuilder()
                                 .SetKey("namespace1", "uri")
                                 .SetSchema("Message")
                                 .AddStringProperty("body", "message body")
                                 .SetCreationTimestampMs(100)
                                 .SetTtlMs(1000)
                                 .Build();
  DocumentProto namespace2_uri1 = DocumentBuilder()
                                      .SetKey("namespace2", "uri1")
                                      .SetSchema("Message")
                                      .AddStringProperty("body", "message body")
                                      .SetCreationTimestampMs(100)
                                      .SetTtlMs(1000)
                                      .Build();
  DocumentProto namespace2_uri2 = DocumentBuilder()
                                      .SetKey("namespace2", "uri2")
                                      .SetSchema("Message")
                                      .AddStringProperty("body", "message body")
                                      .SetCreationTimestampMs(100)
                                      .SetTtlMs(1000)
                                      .Build();

  DocumentProto namespace3 = DocumentBuilder()
                                 .SetKey("namespace3", "uri")
                                 .SetSchema("Message")
                                 .AddStringProperty("body", "message body")
                                 .SetCreationTimestampMs(100)
                                 .SetTtlMs(500)
                                 .Build();
  {
    // Some arbitrary time that's less than all the document's creation time +
    // ttl
    auto fake_clock = std::make_unique<FakeClock>();
    fake_clock->SetSystemTimeMilliseconds(500);

    TestIcingSearchEngine icing(GetDefaultIcingOptions(),
                                std::make_unique<Filesystem>(),
                                std::make_unique<IcingFilesystem>(),
                                std::move(fake_clock), GetTestJniCache());

    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
    ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

    // No namespaces exist yet
    GetAllNamespacesResultProto result = icing.GetAllNamespaces();
    EXPECT_THAT(result.status(), ProtoIsOk());
    EXPECT_THAT(result.namespaces(), IsEmpty());

    ASSERT_THAT(icing.Put(namespace1).status(), ProtoIsOk());
    ASSERT_THAT(icing.Put(namespace2_uri1).status(), ProtoIsOk());
    ASSERT_THAT(icing.Put(namespace2_uri2).status(), ProtoIsOk());
    ASSERT_THAT(icing.Put(namespace3).status(), ProtoIsOk());

    // All namespaces should exist now
    result = icing.GetAllNamespaces();
    EXPECT_THAT(result.status(), ProtoIsOk());
    EXPECT_THAT(result.namespaces(),
                UnorderedElementsAre("namespace1", "namespace2", "namespace3"));

    // After deleting namespace2_uri1 document, we still have namespace2_uri2 in
    // "namespace2" so it should still show up
    ASSERT_THAT(icing.Delete("namespace2", "uri1").status(), ProtoIsOk());

    result = icing.GetAllNamespaces();
    EXPECT_THAT(result.status(), ProtoIsOk());
    EXPECT_THAT(result.namespaces(),
                UnorderedElementsAre("namespace1", "namespace2", "namespace3"));

    // After deleting namespace2_uri2 document, we no longer have any documents
    // in "namespace2"
    ASSERT_THAT(icing.Delete("namespace2", "uri2").status(), ProtoIsOk());

    result = icing.GetAllNamespaces();
    EXPECT_THAT(result.status(), ProtoIsOk());
    EXPECT_THAT(result.namespaces(),
                UnorderedElementsAre("namespace1", "namespace3"));
  }

  // We reinitialize here so we can feed in a fake clock this time
  {
    // Time needs to be past namespace3's creation time (100) + ttl (500) for it
    // to count as "expired"
    auto fake_clock = std::make_unique<FakeClock>();
    fake_clock->SetSystemTimeMilliseconds(1000);

    TestIcingSearchEngine icing(GetDefaultIcingOptions(),
                                std::make_unique<Filesystem>(),
                                std::make_unique<IcingFilesystem>(),
                                std::move(fake_clock), GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

    // Only valid document left is the one in "namespace1"
    GetAllNamespacesResultProto result = icing.GetAllNamespaces();
    EXPECT_THAT(result.status(), ProtoIsOk());
    EXPECT_THAT(result.namespaces(), UnorderedElementsAre("namespace1"));
  }
}

TEST_F(IcingSearchEngineTest, Hyphens) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

  SchemaProto schema;
  SchemaTypeConfigProto* type = schema.add_types();
  type->set_schema_type("MyType");
  PropertyConfigProto* prop = type->add_properties();
  prop->set_property_name("foo");
  prop->set_data_type(PropertyConfigProto::DataType::STRING);
  prop->set_cardinality(PropertyConfigProto::Cardinality::REQUIRED);
  prop->mutable_string_indexing_config()->set_term_match_type(
      TermMatchType::EXACT_ONLY);
  prop->mutable_string_indexing_config()->set_tokenizer_type(
      StringIndexingConfig::TokenizerType::PLAIN);
  ASSERT_THAT(icing.SetSchema(schema).status(), ProtoIsOk());

  DocumentProto document_one =
      DocumentBuilder()
          .SetKey("namespace", "uri1")
          .SetSchema("MyType")
          .AddStringProperty("foo", "foo bar-baz bat")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  ASSERT_THAT(icing.Put(document_one).status(), ProtoIsOk());

  DocumentProto document_two =
      DocumentBuilder()
          .SetKey("namespace", "uri2")
          .SetSchema("MyType")
          .AddStringProperty("foo", "bar for baz bat-man")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .Build();
  ASSERT_THAT(icing.Put(document_two).status(), ProtoIsOk());

  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);
  search_spec.set_query("foo:bar-baz");

  ResultSpecProto result_spec;
  SearchResultProto results =
      icing.Search(search_spec, GetDefaultScoringSpec(), result_spec);

  EXPECT_THAT(results.status(), ProtoIsOk());
  ASSERT_THAT(results.results(), SizeIs(2));
  EXPECT_THAT(results.results(0).document(), EqualsProto(document_two));
  EXPECT_THAT(results.results(1).document(), EqualsProto(document_one));
}

TEST_F(IcingSearchEngineTest, RestoreIndex) {
  DocumentProto document = DocumentBuilder()
                               .SetKey("icing", "fake_type/0")
                               .SetSchema("Message")
                               .AddStringProperty("body", kIpsumText)
                               .Build();
  // 1. Create an index with a LiteIndex that will only allow one document
  // before needing a merge.
  {
    IcingSearchEngineOptions options = GetDefaultIcingOptions();
    options.set_index_merge_size(document.ByteSizeLong());
    IcingSearchEngine icing(options, GetTestJniCache());

    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
    ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

    // Add two documents. These should get merged into the main index.
    EXPECT_THAT(icing.Put(document).status(), ProtoIsOk());
    document = DocumentBuilder(document).SetUri("fake_type/1").Build();
    EXPECT_THAT(icing.Put(document).status(), ProtoIsOk());
    // Add one document. This one should get remain in the lite index.
    document = DocumentBuilder(document).SetUri("fake_type/2").Build();
    EXPECT_THAT(icing.Put(document).status(), ProtoIsOk());
  }

  // 2. Delete the index file to trigger RestoreIndexIfNeeded.
  std::string idx_subdir = GetIndexDir() + "/idx";
  filesystem()->DeleteDirectoryRecursively(idx_subdir.c_str());

  // 3. Create the index again. This should trigger index restoration.
  {
    IcingSearchEngineOptions options = GetDefaultIcingOptions();
    options.set_index_merge_size(document.ByteSizeLong());
    IcingSearchEngine icing(options, GetTestJniCache());

    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

    SearchSpecProto search_spec;
    search_spec.set_query("consectetur");
    search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);
    SearchResultProto results =
        icing.Search(search_spec, ScoringSpecProto::default_instance(),
                     ResultSpecProto::default_instance());
    EXPECT_THAT(results.status(), ProtoIsOk());
    EXPECT_THAT(results.next_page_token(), Eq(0));
    // All documents should be retrievable.
    ASSERT_THAT(results.results(), SizeIs(3));
    EXPECT_THAT(results.results(0).document().uri(), Eq("fake_type/2"));
    EXPECT_THAT(results.results(1).document().uri(), Eq("fake_type/1"));
    EXPECT_THAT(results.results(2).document().uri(), Eq("fake_type/0"));
  }
}

TEST_F(IcingSearchEngineTest, RestoreIndexLoseLiteIndex) {
  DocumentProto document = DocumentBuilder()
                               .SetKey("icing", "fake_type/0")
                               .SetSchema("Message")
                               .AddStringProperty("body", kIpsumText)
                               .Build();
  // 1. Create an index with a LiteIndex that will only allow one document
  // before needing a merge.
  {
    IcingSearchEngineOptions options = GetDefaultIcingOptions();
    options.set_index_merge_size(document.ByteSizeLong());
    IcingSearchEngine icing(options, GetTestJniCache());

    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
    ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

    // Add two documents. These should get merged into the main index.
    EXPECT_THAT(icing.Put(document).status(), ProtoIsOk());
    document = DocumentBuilder(document).SetUri("fake_type/1").Build();
    EXPECT_THAT(icing.Put(document).status(), ProtoIsOk());
    // Add one document. This one should get remain in the lite index.
    document = DocumentBuilder(document).SetUri("fake_type/2").Build();
    EXPECT_THAT(icing.Put(document).status(), ProtoIsOk());
  }

  // 2. Delete the last document from the document log
  {
    const std::string document_log_file = absl_ports::StrCat(
        GetDocumentDir(), "/", DocumentLogCreator::GetDocumentLogFilename());
    filesystem()->DeleteFile(document_log_file.c_str());
    ICING_ASSERT_OK_AND_ASSIGN(
        auto create_result,
        PortableFileBackedProtoLog<DocumentWrapper>::Create(
            filesystem(), document_log_file.c_str(),
            PortableFileBackedProtoLog<DocumentWrapper>::Options(
                /*compress_in=*/true)));
    std::unique_ptr<PortableFileBackedProtoLog<DocumentWrapper>> document_log =
        std::move(create_result.proto_log);

    document = DocumentBuilder(document).SetUri("fake_type/0").Build();
    DocumentWrapper wrapper;
    *wrapper.mutable_document() = document;
    ASSERT_THAT(document_log->WriteProto(wrapper), IsOk());

    document = DocumentBuilder(document).SetUri("fake_type/1").Build();
    *wrapper.mutable_document() = document;
    ASSERT_THAT(document_log->WriteProto(wrapper), IsOk());
  }

  // 3. Create the index again. This should throw out the lite index and trigger
  // index restoration which will only restore the two documents in the main
  // index.
  {
    IcingSearchEngineOptions options = GetDefaultIcingOptions();
    options.set_index_merge_size(document.ByteSizeLong());
    IcingSearchEngine icing(options, GetTestJniCache());

    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

    SearchSpecProto search_spec;
    search_spec.set_query("consectetur");
    search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);
    SearchResultProto results =
        icing.Search(search_spec, ScoringSpecProto::default_instance(),
                     ResultSpecProto::default_instance());
    EXPECT_THAT(results.status(), ProtoIsOk());
    EXPECT_THAT(results.next_page_token(), Eq(0));
    // Only the documents that were in the main index should be retrievable.
    ASSERT_THAT(results.results(), SizeIs(2));
    EXPECT_THAT(results.results(0).document().uri(), Eq("fake_type/1"));
    EXPECT_THAT(results.results(1).document().uri(), Eq("fake_type/0"));
  }
}

TEST_F(IcingSearchEngineTest, RestoreIndexLoseIndex) {
  DocumentProto document = DocumentBuilder()
                               .SetKey("icing", "fake_type/0")
                               .SetSchema("Message")
                               .AddStringProperty("body", kIpsumText)
                               .Build();
  // 1. Create an index with a LiteIndex that will only allow one document
  // before needing a merge.
  {
    IcingSearchEngineOptions options = GetDefaultIcingOptions();
    options.set_index_merge_size(document.ByteSizeLong());
    IcingSearchEngine icing(options, GetTestJniCache());

    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
    ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

    // Add two documents. These should get merged into the main index.
    EXPECT_THAT(icing.Put(document).status(), ProtoIsOk());
    document = DocumentBuilder(document).SetUri("fake_type/1").Build();
    EXPECT_THAT(icing.Put(document).status(), ProtoIsOk());
    // Add one document. This one should get remain in the lite index.
    document = DocumentBuilder(document).SetUri("fake_type/2").Build();
    EXPECT_THAT(icing.Put(document).status(), ProtoIsOk());
  }

  // 2. Delete the last two documents from the document log.
  {
    const std::string document_log_file = absl_ports::StrCat(
        GetDocumentDir(), "/", DocumentLogCreator::GetDocumentLogFilename());
    filesystem()->DeleteFile(document_log_file.c_str());
    ICING_ASSERT_OK_AND_ASSIGN(
        auto create_result,
        PortableFileBackedProtoLog<DocumentWrapper>::Create(
            filesystem(), document_log_file.c_str(),
            PortableFileBackedProtoLog<DocumentWrapper>::Options(
                /*compress_in=*/true)));
    std::unique_ptr<PortableFileBackedProtoLog<DocumentWrapper>> document_log =
        std::move(create_result.proto_log);

    document = DocumentBuilder(document).SetUri("fake_type/0").Build();
    DocumentWrapper wrapper;
    *wrapper.mutable_document() = document;
    ASSERT_THAT(document_log->WriteProto(wrapper), IsOk());
  }

  // 3. Create the index again. This should throw out the lite and main index
  // and trigger index restoration.
  {
    IcingSearchEngineOptions options = GetDefaultIcingOptions();
    options.set_index_merge_size(document.ByteSizeLong());
    IcingSearchEngine icing(options, GetTestJniCache());

    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

    SearchSpecProto search_spec;
    search_spec.set_query("consectetur");
    search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);
    SearchResultProto results =
        icing.Search(search_spec, ScoringSpecProto::default_instance(),
                     ResultSpecProto::default_instance());
    EXPECT_THAT(results.status(), ProtoIsOk());
    EXPECT_THAT(results.next_page_token(), Eq(0));
    // Only the first document should be retrievable.
    ASSERT_THAT(results.results(), SizeIs(1));
    EXPECT_THAT(results.results(0).document().uri(), Eq("fake_type/0"));
  }
}

TEST_F(IcingSearchEngineTest,
       DocumentWithNoIndexedContentDoesntCauseRestoreIndex) {
  // 1. Create an index with a single document in it that has no indexed
  // content.
  {
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

    // Set a schema for a single type that has no indexed properties.
    SchemaProto schema =
        SchemaBuilder()
            .AddType(SchemaTypeConfigBuilder().SetType("Message").AddProperty(
                PropertyConfigBuilder()
                    .SetName("unindexedField")
                    .SetDataTypeString(MATCH_NONE, TOKENIZER_NONE)
                    .SetCardinality(CARDINALITY_REQUIRED)))
            .Build();
    ASSERT_THAT(icing.SetSchema(schema).status(), ProtoIsOk());

    // Add a document that contains no indexed content.
    DocumentProto document =
        DocumentBuilder()
            .SetKey("icing", "fake_type/0")
            .SetSchema("Message")
            .AddStringProperty("unindexedField",
                               "Don't you dare search over this!")
            .Build();
    EXPECT_THAT(icing.Put(document).status(), ProtoIsOk());
  }

  // 2. Create the index again. This should NOT trigger a recovery of any kind.
  {
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    InitializeResultProto init_result = icing.Initialize();
    EXPECT_THAT(init_result.status(), ProtoIsOk());
    EXPECT_THAT(init_result.initialize_stats().document_store_data_status(),
                Eq(InitializeStatsProto::NO_DATA_LOSS));
    EXPECT_THAT(init_result.initialize_stats().document_store_recovery_cause(),
                Eq(InitializeStatsProto::NONE));
    EXPECT_THAT(init_result.initialize_stats().schema_store_recovery_cause(),
                Eq(InitializeStatsProto::NONE));
    EXPECT_THAT(init_result.initialize_stats().index_restoration_cause(),
                Eq(InitializeStatsProto::NONE));
  }
}

TEST_F(IcingSearchEngineTest,
       DocumentWithNoValidIndexedContentDoesntCauseRestoreIndex) {
  // 1. Create an index with a single document in it that has no valid indexed
  // tokens in its content.
  {
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

    // Set a schema for a single type that has no indexed properties.
    ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

    // Add a document that contains no valid indexed content - just punctuation.
    DocumentProto document = DocumentBuilder()
                                 .SetKey("icing", "fake_type/0")
                                 .SetSchema("Message")
                                 .AddStringProperty("body", "?...!")
                                 .Build();
    EXPECT_THAT(icing.Put(document).status(), ProtoIsOk());
  }

  // 2. Create the index again. This should NOT trigger a recovery of any kind.
  {
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    InitializeResultProto init_result = icing.Initialize();
    EXPECT_THAT(init_result.status(), ProtoIsOk());
    EXPECT_THAT(init_result.initialize_stats().document_store_data_status(),
                Eq(InitializeStatsProto::NO_DATA_LOSS));
    EXPECT_THAT(init_result.initialize_stats().document_store_recovery_cause(),
                Eq(InitializeStatsProto::NONE));
    EXPECT_THAT(init_result.initialize_stats().schema_store_recovery_cause(),
                Eq(InitializeStatsProto::NONE));
    EXPECT_THAT(init_result.initialize_stats().index_restoration_cause(),
                Eq(InitializeStatsProto::NONE));
  }
}

TEST_F(IcingSearchEngineTest, IndexingDocMergeFailureResets) {
  DocumentProto document = DocumentBuilder()
                               .SetKey("icing", "fake_type/0")
                               .SetSchema("Message")
                               .AddStringProperty("body", kIpsumText)
                               .Build();
  // 1. Create an index with a LiteIndex that will only allow one document
  // before needing a merge.
  {
    IcingSearchEngineOptions options = GetDefaultIcingOptions();
    options.set_index_merge_size(document.ByteSizeLong());
    IcingSearchEngine icing(options, GetTestJniCache());

    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
    ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

    // Add two documents. These should get merged into the main index.
    EXPECT_THAT(icing.Put(document).status(), ProtoIsOk());
    document = DocumentBuilder(document).SetUri("fake_type/1").Build();
    EXPECT_THAT(icing.Put(document).status(), ProtoIsOk());
    // Add one document. This one should get remain in the lite index.
    document = DocumentBuilder(document).SetUri("fake_type/2").Build();
    EXPECT_THAT(icing.Put(document).status(), ProtoIsOk());
  }

  // 2. Delete the index file to trigger RestoreIndexIfNeeded.
  std::string idx_subdir = GetIndexDir() + "/idx";
  filesystem()->DeleteDirectoryRecursively(idx_subdir.c_str());

  // 3. Setup a mock filesystem to fail to grow the main index once.
  bool has_failed_already = false;
  auto open_write_lambda = [this, &has_failed_already](const char* filename) {
    std::string main_lexicon_suffix = "/main-lexicon.prop.2";
    std::string filename_string(filename);
    if (!has_failed_already &&
        filename_string.length() >= main_lexicon_suffix.length() &&
        filename_string.substr(
            filename_string.length() - main_lexicon_suffix.length(),
            main_lexicon_suffix.length()) == main_lexicon_suffix) {
      has_failed_already = true;
      return -1;
    }
    return this->filesystem()->OpenForWrite(filename);
  };
  auto mock_icing_filesystem = std::make_unique<IcingMockFilesystem>();
  ON_CALL(*mock_icing_filesystem, OpenForWrite)
      .WillByDefault(open_write_lambda);

  // 4. Create the index again. This should trigger index restoration.
  {
    IcingSearchEngineOptions options = GetDefaultIcingOptions();
    options.set_index_merge_size(document.ByteSizeLong());
    TestIcingSearchEngine icing(options, std::make_unique<Filesystem>(),
                                std::move(mock_icing_filesystem),
                                std::make_unique<FakeClock>(),
                                GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(),
                ProtoStatusIs(StatusProto::WARNING_DATA_LOSS));

    SearchSpecProto search_spec;
    search_spec.set_query("consectetur");
    search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);
    SearchResultProto results =
        icing.Search(search_spec, ScoringSpecProto::default_instance(),
                     ResultSpecProto::default_instance());
    EXPECT_THAT(results.status(), ProtoIsOk());
    EXPECT_THAT(results.next_page_token(), Eq(0));
    // Only the last document that was added should still be retrievable.
    ASSERT_THAT(results.results(), SizeIs(1));
    EXPECT_THAT(results.results(0).document().uri(), Eq("fake_type/2"));
  }
}

TEST_F(IcingSearchEngineTest, InitializeShouldLogFunctionLatency) {
  auto fake_clock = std::make_unique<FakeClock>();
  fake_clock->SetTimerElapsedMilliseconds(10);
  TestIcingSearchEngine icing(GetDefaultIcingOptions(),
                              std::make_unique<Filesystem>(),
                              std::make_unique<IcingFilesystem>(),
                              std::move(fake_clock), GetTestJniCache());
  InitializeResultProto initialize_result_proto = icing.Initialize();
  EXPECT_THAT(initialize_result_proto.status(), ProtoIsOk());
  EXPECT_THAT(initialize_result_proto.initialize_stats().latency_ms(), Eq(10));
}

TEST_F(IcingSearchEngineTest, InitializeShouldLogNumberOfDocuments) {
  DocumentProto document1 = DocumentBuilder()
                                .SetKey("icing", "fake_type/1")
                                .SetSchema("Message")
                                .AddStringProperty("body", "message body")
                                .Build();
  DocumentProto document2 = DocumentBuilder()
                                .SetKey("icing", "fake_type/2")
                                .SetSchema("Message")
                                .AddStringProperty("body", "message body")
                                .Build();

  {
    // Initialize and put a document.
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    InitializeResultProto initialize_result_proto = icing.Initialize();
    EXPECT_THAT(initialize_result_proto.status(), ProtoIsOk());
    EXPECT_THAT(initialize_result_proto.initialize_stats().num_documents(),
                Eq(0));

    ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
    ASSERT_THAT(icing.Put(document1).status(), ProtoIsOk());
  }

  {
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    InitializeResultProto initialize_result_proto = icing.Initialize();
    EXPECT_THAT(initialize_result_proto.status(), ProtoIsOk());
    EXPECT_THAT(initialize_result_proto.initialize_stats().num_documents(),
                Eq(1));

    // Put another document.
    ASSERT_THAT(icing.Put(document2).status(), ProtoIsOk());
  }

  {
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    InitializeResultProto initialize_result_proto = icing.Initialize();
    EXPECT_THAT(initialize_result_proto.status(), ProtoIsOk());
    EXPECT_THAT(initialize_result_proto.initialize_stats().num_documents(),
                Eq(2));
  }
}

TEST_F(IcingSearchEngineTest,
       InitializeShouldNotLogRecoveryCauseForFirstTimeInitialize) {
  // Even though the fake timer will return 10, all the latency numbers related
  // to recovery / restoration should be 0 during the first-time initialization.
  auto fake_clock = std::make_unique<FakeClock>();
  fake_clock->SetTimerElapsedMilliseconds(10);
  TestIcingSearchEngine icing(GetDefaultIcingOptions(),
                              std::make_unique<Filesystem>(),
                              std::make_unique<IcingFilesystem>(),
                              std::move(fake_clock), GetTestJniCache());
  InitializeResultProto initialize_result_proto = icing.Initialize();
  EXPECT_THAT(initialize_result_proto.status(), ProtoIsOk());
  EXPECT_THAT(initialize_result_proto.initialize_stats()
                  .document_store_recovery_cause(),
              Eq(InitializeStatsProto::NONE));
  EXPECT_THAT(initialize_result_proto.initialize_stats()
                  .document_store_recovery_latency_ms(),
              Eq(0));
  EXPECT_THAT(
      initialize_result_proto.initialize_stats().document_store_data_status(),
      Eq(InitializeStatsProto::NO_DATA_LOSS));
  EXPECT_THAT(
      initialize_result_proto.initialize_stats().index_restoration_cause(),
      Eq(InitializeStatsProto::NONE));
  EXPECT_THAT(
      initialize_result_proto.initialize_stats().index_restoration_latency_ms(),
      Eq(0));
  EXPECT_THAT(
      initialize_result_proto.initialize_stats().schema_store_recovery_cause(),
      Eq(InitializeStatsProto::NONE));
  EXPECT_THAT(initialize_result_proto.initialize_stats()
                  .schema_store_recovery_latency_ms(),
              Eq(0));
}

TEST_F(IcingSearchEngineTest, InitializeShouldLogRecoveryCausePartialDataLoss) {
  DocumentProto document = DocumentBuilder()
                               .SetKey("icing", "fake_type/0")
                               .SetSchema("Message")
                               .AddStringProperty("body", "message body")
                               .Build();

  {
    // Initialize and put a document.
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
    ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
    EXPECT_THAT(icing.Put(document).status(), ProtoIsOk());
  }

  {
    // Append a non-checksummed document. This will mess up the checksum of the
    // proto log, forcing it to rewind and later return a DATA_LOSS error.
    const std::string serialized_document = document.SerializeAsString();
    const std::string document_log_file = absl_ports::StrCat(
        GetDocumentDir(), "/", DocumentLogCreator::GetDocumentLogFilename());

    int64_t file_size = filesystem()->GetFileSize(document_log_file.c_str());
    filesystem()->PWrite(document_log_file.c_str(), file_size,
                         serialized_document.data(),
                         serialized_document.size());
  }

  {
    // Document store will rewind to previous checkpoint. The cause should be
    // DATA_LOSS and the data status should be PARTIAL_LOSS.
    auto fake_clock = std::make_unique<FakeClock>();
    fake_clock->SetTimerElapsedMilliseconds(10);
    TestIcingSearchEngine icing(GetDefaultIcingOptions(),
                                std::make_unique<Filesystem>(),
                                std::make_unique<IcingFilesystem>(),
                                std::move(fake_clock), GetTestJniCache());
    InitializeResultProto initialize_result_proto = icing.Initialize();
    EXPECT_THAT(initialize_result_proto.status(), ProtoIsOk());
    EXPECT_THAT(initialize_result_proto.initialize_stats()
                    .document_store_recovery_cause(),
                Eq(InitializeStatsProto::DATA_LOSS));
    EXPECT_THAT(initialize_result_proto.initialize_stats()
                    .document_store_recovery_latency_ms(),
                Eq(10));
    EXPECT_THAT(
        initialize_result_proto.initialize_stats().document_store_data_status(),
        Eq(InitializeStatsProto::PARTIAL_LOSS));
    EXPECT_THAT(
        initialize_result_proto.initialize_stats().index_restoration_cause(),
        Eq(InitializeStatsProto::NONE));
    EXPECT_THAT(initialize_result_proto.initialize_stats()
                    .index_restoration_latency_ms(),
                Eq(0));
    EXPECT_THAT(initialize_result_proto.initialize_stats()
                    .schema_store_recovery_cause(),
                Eq(InitializeStatsProto::NONE));
    EXPECT_THAT(initialize_result_proto.initialize_stats()
                    .schema_store_recovery_latency_ms(),
                Eq(0));
  }
}

TEST_F(IcingSearchEngineTest,
       InitializeShouldLogRecoveryCauseCompleteDataLoss) {
  DocumentProto document1 = DocumentBuilder()
                                .SetKey("icing", "fake_type/1")
                                .SetSchema("Message")
                                .AddStringProperty("body", "message body")
                                .Build();

  const std::string document_log_file = absl_ports::StrCat(
      GetDocumentDir(), "/", DocumentLogCreator::GetDocumentLogFilename());
  int64_t corruptible_offset;

  {
    // Initialize and put a document.
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());

    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());

    // There's some space at the beginning of the file (e.g. header, kmagic,
    // etc) that is necessary to initialize the FileBackedProtoLog. We can't
    // corrupt that region, so we need to figure out the offset at which
    // documents will be written to - which is the file size after
    // initialization.
    corruptible_offset = filesystem()->GetFileSize(document_log_file.c_str());

    ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
    EXPECT_THAT(icing.Put(document1).status(), ProtoIsOk());
  }

  {
    // "Corrupt" the content written in the log. Make the corrupt document
    // smaller than our original one so we don't accidentally write past our
    // file.
    DocumentProto document =
        DocumentBuilder().SetKey("invalid_namespace", "invalid_uri").Build();
    std::string serialized_document = document.SerializeAsString();
    ASSERT_TRUE(filesystem()->PWrite(
        document_log_file.c_str(), corruptible_offset,
        serialized_document.data(), serialized_document.size()));

    PortableFileBackedProtoLog<DocumentWrapper>::Header header =
        ReadDocumentLogHeader(*filesystem(), document_log_file);

    // Set dirty bit to true to reflect that something changed in the log.
    header.SetDirtyFlag(true);
    header.SetHeaderChecksum(header.CalculateHeaderChecksum());

    WriteDocumentLogHeader(*filesystem(), document_log_file, header);
  }

  {
    // Document store will completely rewind. The cause should be DATA_LOSS and
    // the data status should be COMPLETE_LOSS.
    auto fake_clock = std::make_unique<FakeClock>();
    fake_clock->SetTimerElapsedMilliseconds(10);
    TestIcingSearchEngine icing(GetDefaultIcingOptions(),
                                std::make_unique<Filesystem>(),
                                std::make_unique<IcingFilesystem>(),
                                std::move(fake_clock), GetTestJniCache());
    InitializeResultProto initialize_result_proto = icing.Initialize();
    EXPECT_THAT(initialize_result_proto.status(), ProtoIsOk());
    EXPECT_THAT(initialize_result_proto.initialize_stats()
                    .document_store_recovery_cause(),
                Eq(InitializeStatsProto::DATA_LOSS));
    EXPECT_THAT(initialize_result_proto.initialize_stats()
                    .document_store_recovery_latency_ms(),
                Eq(10));
    EXPECT_THAT(
        initialize_result_proto.initialize_stats().document_store_data_status(),
        Eq(InitializeStatsProto::COMPLETE_LOSS));
    // The complete rewind of ground truth causes us to clear the index, but
    // that's not considered a restoration.
    EXPECT_THAT(
        initialize_result_proto.initialize_stats().index_restoration_cause(),
        Eq(InitializeStatsProto::NONE));
    EXPECT_THAT(initialize_result_proto.initialize_stats()
                    .index_restoration_latency_ms(),
                Eq(0));
    EXPECT_THAT(initialize_result_proto.initialize_stats()
                    .schema_store_recovery_cause(),
                Eq(InitializeStatsProto::NONE));
    EXPECT_THAT(initialize_result_proto.initialize_stats()
                    .schema_store_recovery_latency_ms(),
                Eq(0));
  }
}

TEST_F(IcingSearchEngineTest,
       InitializeShouldLogRecoveryCauseInconsistentWithGroundTruth) {
  DocumentProto document = DocumentBuilder()
                               .SetKey("icing", "fake_type/0")
                               .SetSchema("Message")
                               .AddStringProperty("body", "message body")
                               .Build();
  {
    // Initialize and put a document.
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
    ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
    EXPECT_THAT(icing.Put(document).status(), ProtoIsOk());
  }

  {
    // Delete the index file to trigger RestoreIndexIfNeeded.
    std::string idx_subdir = GetIndexDir() + "/idx";
    filesystem()->DeleteDirectoryRecursively(idx_subdir.c_str());
  }

  {
    // Index is empty but ground truth is not. Index should be restored due to
    // the inconsistency.
    auto fake_clock = std::make_unique<FakeClock>();
    fake_clock->SetTimerElapsedMilliseconds(10);
    TestIcingSearchEngine icing(GetDefaultIcingOptions(),
                                std::make_unique<Filesystem>(),
                                std::make_unique<IcingFilesystem>(),
                                std::move(fake_clock), GetTestJniCache());
    InitializeResultProto initialize_result_proto = icing.Initialize();
    EXPECT_THAT(initialize_result_proto.status(), ProtoIsOk());
    EXPECT_THAT(
        initialize_result_proto.initialize_stats().index_restoration_cause(),
        Eq(InitializeStatsProto::INCONSISTENT_WITH_GROUND_TRUTH));
    EXPECT_THAT(initialize_result_proto.initialize_stats()
                    .index_restoration_latency_ms(),
                Eq(10));
    EXPECT_THAT(initialize_result_proto.initialize_stats()
                    .document_store_recovery_cause(),
                Eq(InitializeStatsProto::NONE));
    EXPECT_THAT(initialize_result_proto.initialize_stats()
                    .document_store_recovery_latency_ms(),
                Eq(0));
    EXPECT_THAT(
        initialize_result_proto.initialize_stats().document_store_data_status(),
        Eq(InitializeStatsProto::NO_DATA_LOSS));
    EXPECT_THAT(initialize_result_proto.initialize_stats()
                    .schema_store_recovery_cause(),
                Eq(InitializeStatsProto::NONE));
    EXPECT_THAT(initialize_result_proto.initialize_stats()
                    .schema_store_recovery_latency_ms(),
                Eq(0));
  }
}

TEST_F(IcingSearchEngineTest,
       InitializeShouldLogRecoveryCauseSchemaChangesOutofSync) {
  DocumentProto document = DocumentBuilder()
                               .SetKey("icing", "fake_type/0")
                               .SetSchema("Message")
                               .AddStringProperty("body", "message body")
                               .Build();
  IcingSearchEngineOptions options = GetDefaultIcingOptions();
  {
    // Initialize and put one document.
    IcingSearchEngine icing(options, GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
    ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
    ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  }

  {
    // Simulate a schema change where power is lost after the schema is written.
    SchemaProto new_schema =
        SchemaBuilder()
            .AddType(
                SchemaTypeConfigBuilder()
                    .SetType("Message")
                    .AddProperty(
                        PropertyConfigBuilder()
                            .SetName("body")
                            .SetDataTypeString(MATCH_PREFIX, TOKENIZER_PLAIN)
                            .SetCardinality(CARDINALITY_REQUIRED))
                    .AddProperty(
                        PropertyConfigBuilder()
                            .SetName("subject")
                            .SetDataTypeString(MATCH_PREFIX, TOKENIZER_PLAIN)
                            .SetCardinality(CARDINALITY_OPTIONAL)))
            .Build();
    // Write the marker file
    std::string marker_filepath =
        absl_ports::StrCat(options.base_dir(), "/set_schema_marker");
    ScopedFd sfd(filesystem()->OpenForWrite(marker_filepath.c_str()));
    ASSERT_TRUE(sfd.is_valid());

    // Write the new schema
    FakeClock fake_clock;
    ICING_ASSERT_OK_AND_ASSIGN(
        std::unique_ptr<SchemaStore> schema_store,
        SchemaStore::Create(filesystem(), GetSchemaDir(), &fake_clock));
    ICING_EXPECT_OK(schema_store->SetSchema(new_schema));
  }

  {
    // Both document store and index should be recovered from checksum mismatch.
    auto fake_clock = std::make_unique<FakeClock>();
    fake_clock->SetTimerElapsedMilliseconds(10);
    TestIcingSearchEngine icing(GetDefaultIcingOptions(),
                                std::make_unique<Filesystem>(),
                                std::make_unique<IcingFilesystem>(),
                                std::move(fake_clock), GetTestJniCache());
    InitializeResultProto initialize_result_proto = icing.Initialize();
    EXPECT_THAT(initialize_result_proto.status(), ProtoIsOk());
    EXPECT_THAT(
        initialize_result_proto.initialize_stats().index_restoration_cause(),
        Eq(InitializeStatsProto::SCHEMA_CHANGES_OUT_OF_SYNC));
    EXPECT_THAT(initialize_result_proto.initialize_stats()
                    .index_restoration_latency_ms(),
                Eq(10));
    EXPECT_THAT(initialize_result_proto.initialize_stats()
                    .document_store_recovery_cause(),
                Eq(InitializeStatsProto::SCHEMA_CHANGES_OUT_OF_SYNC));
    EXPECT_THAT(initialize_result_proto.initialize_stats()
                    .document_store_recovery_latency_ms(),
                Eq(10));
    EXPECT_THAT(
        initialize_result_proto.initialize_stats().document_store_data_status(),
        Eq(InitializeStatsProto::NO_DATA_LOSS));
    EXPECT_THAT(initialize_result_proto.initialize_stats()
                    .schema_store_recovery_cause(),
                Eq(InitializeStatsProto::NONE));
    EXPECT_THAT(initialize_result_proto.initialize_stats()
                    .schema_store_recovery_latency_ms(),
                Eq(0));
  }

  {
    // No recovery should be needed.
    auto fake_clock = std::make_unique<FakeClock>();
    fake_clock->SetTimerElapsedMilliseconds(10);
    TestIcingSearchEngine icing(GetDefaultIcingOptions(),
                                std::make_unique<Filesystem>(),
                                std::make_unique<IcingFilesystem>(),
                                std::move(fake_clock), GetTestJniCache());
    InitializeResultProto initialize_result_proto = icing.Initialize();
    EXPECT_THAT(initialize_result_proto.status(), ProtoIsOk());
    EXPECT_THAT(
        initialize_result_proto.initialize_stats().index_restoration_cause(),
        Eq(InitializeStatsProto::NONE));
    EXPECT_THAT(initialize_result_proto.initialize_stats()
                    .index_restoration_latency_ms(),
                Eq(0));
    EXPECT_THAT(initialize_result_proto.initialize_stats()
                    .document_store_recovery_cause(),
                Eq(InitializeStatsProto::NONE));
    EXPECT_THAT(initialize_result_proto.initialize_stats()
                    .document_store_recovery_latency_ms(),
                Eq(0));
    EXPECT_THAT(
        initialize_result_proto.initialize_stats().document_store_data_status(),
        Eq(InitializeStatsProto::NO_DATA_LOSS));
    EXPECT_THAT(initialize_result_proto.initialize_stats()
                    .schema_store_recovery_cause(),
                Eq(InitializeStatsProto::NONE));
    EXPECT_THAT(initialize_result_proto.initialize_stats()
                    .schema_store_recovery_latency_ms(),
                Eq(0));
  }
}

TEST_F(IcingSearchEngineTest, InitializeShouldLogRecoveryCauseIndexIOError) {
  DocumentProto document = DocumentBuilder()
                               .SetKey("icing", "fake_type/0")
                               .SetSchema("Message")
                               .AddStringProperty("body", "message body")
                               .Build();
  {
    // Initialize and put one document.
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
    ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
    ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  }

  // lambda to fail OpenForWrite on lite index hit buffer once.
  bool has_failed_already = false;
  auto open_write_lambda = [this, &has_failed_already](const char* filename) {
    std::string lite_index_buffer_file_path =
        absl_ports::StrCat(GetIndexDir(), "/idx/lite.hb");
    std::string filename_string(filename);
    if (!has_failed_already && filename_string == lite_index_buffer_file_path) {
      has_failed_already = true;
      return -1;
    }
    return this->filesystem()->OpenForWrite(filename);
  };

  auto mock_icing_filesystem = std::make_unique<IcingMockFilesystem>();
  // This fails Index::Create() once.
  ON_CALL(*mock_icing_filesystem, OpenForWrite)
      .WillByDefault(open_write_lambda);

  auto fake_clock = std::make_unique<FakeClock>();
  fake_clock->SetTimerElapsedMilliseconds(10);
  TestIcingSearchEngine icing(GetDefaultIcingOptions(),
                              std::make_unique<Filesystem>(),
                              std::move(mock_icing_filesystem),
                              std::move(fake_clock), GetTestJniCache());

  InitializeResultProto initialize_result_proto = icing.Initialize();
  EXPECT_THAT(initialize_result_proto.status(), ProtoIsOk());
  EXPECT_THAT(
      initialize_result_proto.initialize_stats().index_restoration_cause(),
      Eq(InitializeStatsProto::IO_ERROR));
  EXPECT_THAT(
      initialize_result_proto.initialize_stats().index_restoration_latency_ms(),
      Eq(10));
  EXPECT_THAT(initialize_result_proto.initialize_stats()
                  .document_store_recovery_cause(),
              Eq(InitializeStatsProto::NONE));
  EXPECT_THAT(initialize_result_proto.initialize_stats()
                  .document_store_recovery_latency_ms(),
              Eq(0));
  EXPECT_THAT(
      initialize_result_proto.initialize_stats().document_store_data_status(),
      Eq(InitializeStatsProto::NO_DATA_LOSS));
  EXPECT_THAT(
      initialize_result_proto.initialize_stats().schema_store_recovery_cause(),
      Eq(InitializeStatsProto::NONE));
  EXPECT_THAT(initialize_result_proto.initialize_stats()
                  .schema_store_recovery_latency_ms(),
              Eq(0));
}

TEST_F(IcingSearchEngineTest, InitializeShouldLogRecoveryCauseDocStoreIOError) {
  DocumentProto document = DocumentBuilder()
                               .SetKey("icing", "fake_type/0")
                               .SetSchema("Message")
                               .AddStringProperty("body", "message body")
                               .Build();
  {
    // Initialize and put one document.
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
    ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
    ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());
  }

  // lambda to fail Read on document store header once.
  bool has_failed_already = false;
  auto read_lambda = [this, &has_failed_already](const char* filename,
                                                 void* buf, size_t buf_size) {
    std::string document_store_header_file_path =
        absl_ports::StrCat(GetDocumentDir(), "/document_store_header");
    std::string filename_string(filename);
    if (!has_failed_already &&
        filename_string == document_store_header_file_path) {
      has_failed_already = true;
      return false;
    }
    return this->filesystem()->Read(filename, buf, buf_size);
  };

  auto mock_filesystem = std::make_unique<MockFilesystem>();
  // This fails DocumentStore::InitializeDerivedFiles() once.
  ON_CALL(*mock_filesystem, Read(A<const char*>(), _, _))
      .WillByDefault(read_lambda);

  auto fake_clock = std::make_unique<FakeClock>();
  fake_clock->SetTimerElapsedMilliseconds(10);
  TestIcingSearchEngine icing(GetDefaultIcingOptions(),
                              std::move(mock_filesystem),
                              std::make_unique<IcingFilesystem>(),
                              std::move(fake_clock), GetTestJniCache());

  InitializeResultProto initialize_result_proto = icing.Initialize();
  EXPECT_THAT(initialize_result_proto.status(), ProtoIsOk());
  EXPECT_THAT(initialize_result_proto.initialize_stats()
                  .document_store_recovery_cause(),
              Eq(InitializeStatsProto::IO_ERROR));
  EXPECT_THAT(initialize_result_proto.initialize_stats()
                  .document_store_recovery_latency_ms(),
              Eq(10));
  EXPECT_THAT(
      initialize_result_proto.initialize_stats().document_store_data_status(),
      Eq(InitializeStatsProto::NO_DATA_LOSS));
  EXPECT_THAT(
      initialize_result_proto.initialize_stats().index_restoration_cause(),
      Eq(InitializeStatsProto::NONE));
  EXPECT_THAT(
      initialize_result_proto.initialize_stats().index_restoration_latency_ms(),
      Eq(0));
  EXPECT_THAT(
      initialize_result_proto.initialize_stats().schema_store_recovery_cause(),
      Eq(InitializeStatsProto::NONE));
  EXPECT_THAT(initialize_result_proto.initialize_stats()
                  .schema_store_recovery_latency_ms(),
              Eq(0));
}

TEST_F(IcingSearchEngineTest,
       InitializeShouldLogRecoveryCauseSchemaStoreIOError) {
  {
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
    ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
  }

  {
    // Delete the schema store header file to trigger an I/O error.
    std::string schema_store_header_file_path =
        GetSchemaDir() + "/schema_store_header";
    filesystem()->DeleteFile(schema_store_header_file_path.c_str());
  }

  {
    auto fake_clock = std::make_unique<FakeClock>();
    fake_clock->SetTimerElapsedMilliseconds(10);
    TestIcingSearchEngine icing(GetDefaultIcingOptions(),
                                std::make_unique<Filesystem>(),
                                std::make_unique<IcingFilesystem>(),
                                std::move(fake_clock), GetTestJniCache());
    InitializeResultProto initialize_result_proto = icing.Initialize();
    EXPECT_THAT(initialize_result_proto.status(), ProtoIsOk());
    EXPECT_THAT(initialize_result_proto.initialize_stats()
                    .schema_store_recovery_cause(),
                Eq(InitializeStatsProto::IO_ERROR));
    EXPECT_THAT(initialize_result_proto.initialize_stats()
                    .schema_store_recovery_latency_ms(),
                Eq(10));
    EXPECT_THAT(initialize_result_proto.initialize_stats()
                    .document_store_recovery_cause(),
                Eq(InitializeStatsProto::NONE));
    EXPECT_THAT(initialize_result_proto.initialize_stats()
                    .document_store_recovery_latency_ms(),
                Eq(0));
    EXPECT_THAT(
        initialize_result_proto.initialize_stats().document_store_data_status(),
        Eq(InitializeStatsProto::NO_DATA_LOSS));
    EXPECT_THAT(
        initialize_result_proto.initialize_stats().index_restoration_cause(),
        Eq(InitializeStatsProto::NONE));
    EXPECT_THAT(initialize_result_proto.initialize_stats()
                    .index_restoration_latency_ms(),
                Eq(0));
  }
}

TEST_F(IcingSearchEngineTest, InitializeShouldLogNumberOfSchemaTypes) {
  {
    // Initialize an empty storage.
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    InitializeResultProto initialize_result_proto = icing.Initialize();
    EXPECT_THAT(initialize_result_proto.status(), ProtoIsOk());
    // There should be 0 schema types.
    EXPECT_THAT(initialize_result_proto.initialize_stats().num_schema_types(),
                Eq(0));

    // Set a schema with one type config.
    ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
  }

  {
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    InitializeResultProto initialize_result_proto = icing.Initialize();
    EXPECT_THAT(initialize_result_proto.status(), ProtoIsOk());
    // There should be 1 schema type.
    EXPECT_THAT(initialize_result_proto.initialize_stats().num_schema_types(),
                Eq(1));

    // Create and set a schema with two type configs: Email and Message.
    SchemaProto schema = CreateEmailSchema();

    auto type = schema.add_types();
    type->set_schema_type("Message");
    auto body = type->add_properties();
    body->set_property_name("body");
    body->set_data_type(PropertyConfigProto::DataType::STRING);
    body->set_cardinality(PropertyConfigProto::Cardinality::REQUIRED);
    body->mutable_string_indexing_config()->set_term_match_type(
        TermMatchType::PREFIX);
    body->mutable_string_indexing_config()->set_tokenizer_type(
        StringIndexingConfig::TokenizerType::PLAIN);

    ASSERT_THAT(icing.SetSchema(schema).status(), ProtoIsOk());
  }

  {
    IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
    InitializeResultProto initialize_result_proto = icing.Initialize();
    EXPECT_THAT(initialize_result_proto.status(), ProtoIsOk());
    EXPECT_THAT(initialize_result_proto.initialize_stats().num_schema_types(),
                Eq(2));
  }
}

TEST_F(IcingSearchEngineTest, PutDocumentShouldLogFunctionLatency) {
  DocumentProto document = DocumentBuilder()
                               .SetKey("icing", "fake_type/0")
                               .SetSchema("Message")
                               .AddStringProperty("body", "message body")
                               .Build();

  auto fake_clock = std::make_unique<FakeClock>();
  fake_clock->SetTimerElapsedMilliseconds(10);
  TestIcingSearchEngine icing(GetDefaultIcingOptions(),
                              std::make_unique<Filesystem>(),
                              std::make_unique<IcingFilesystem>(),
                              std::move(fake_clock), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  PutResultProto put_result_proto = icing.Put(document);
  EXPECT_THAT(put_result_proto.status(), ProtoIsOk());
  EXPECT_THAT(put_result_proto.put_document_stats().latency_ms(), Eq(10));
}

TEST_F(IcingSearchEngineTest, PutDocumentShouldLogDocumentStoreStats) {
  DocumentProto document =
      DocumentBuilder()
          .SetKey("icing", "fake_type/0")
          .SetSchema("Message")
          .SetCreationTimestampMs(kDefaultCreationTimestampMs)
          .AddStringProperty("body", "message body")
          .Build();

  auto fake_clock = std::make_unique<FakeClock>();
  fake_clock->SetTimerElapsedMilliseconds(10);
  TestIcingSearchEngine icing(GetDefaultIcingOptions(),
                              std::make_unique<Filesystem>(),
                              std::make_unique<IcingFilesystem>(),
                              std::move(fake_clock), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  PutResultProto put_result_proto = icing.Put(document);
  EXPECT_THAT(put_result_proto.status(), ProtoIsOk());
  EXPECT_THAT(put_result_proto.put_document_stats().document_store_latency_ms(),
              Eq(10));
  size_t document_size = put_result_proto.put_document_stats().document_size();
  EXPECT_THAT(document_size, Ge(document.ByteSizeLong()));
  EXPECT_THAT(document_size, Le(document.ByteSizeLong() +
                                sizeof(DocumentProto::InternalFields)));
}

TEST_F(IcingSearchEngineTest, PutDocumentShouldLogIndexingStats) {
  DocumentProto document = DocumentBuilder()
                               .SetKey("icing", "fake_type/0")
                               .SetSchema("Message")
                               .AddStringProperty("body", "message body")
                               .Build();

  auto fake_clock = std::make_unique<FakeClock>();
  fake_clock->SetTimerElapsedMilliseconds(10);
  TestIcingSearchEngine icing(GetDefaultIcingOptions(),
                              std::make_unique<Filesystem>(),
                              std::make_unique<IcingFilesystem>(),
                              std::move(fake_clock), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  PutResultProto put_result_proto = icing.Put(document);
  EXPECT_THAT(put_result_proto.status(), ProtoIsOk());
  EXPECT_THAT(put_result_proto.put_document_stats().index_latency_ms(), Eq(10));
  // No merge should happen.
  EXPECT_THAT(put_result_proto.put_document_stats().index_merge_latency_ms(),
              Eq(0));
  // Number of tokens should not exceed.
  EXPECT_FALSE(put_result_proto.put_document_stats()
                   .tokenization_stats()
                   .exceeded_max_token_num());
  // The input document has 2 tokens.
  EXPECT_THAT(put_result_proto.put_document_stats()
                  .tokenization_stats()
                  .num_tokens_indexed(),
              Eq(2));
}

TEST_F(IcingSearchEngineTest, PutDocumentShouldLogWhetherNumTokensExceeds) {
  // Create a document with 2 tokens.
  DocumentProto document = DocumentBuilder()
                               .SetKey("icing", "fake_type/0")
                               .SetSchema("Message")
                               .AddStringProperty("body", "message body")
                               .Build();

  // Create an icing instance with max_tokens_per_doc = 1.
  IcingSearchEngineOptions icing_options = GetDefaultIcingOptions();
  icing_options.set_max_tokens_per_doc(1);
  IcingSearchEngine icing(icing_options, GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  PutResultProto put_result_proto = icing.Put(document);
  EXPECT_THAT(put_result_proto.status(), ProtoIsOk());
  // Number of tokens(2) exceeds the max allowed value(1).
  EXPECT_TRUE(put_result_proto.put_document_stats()
                  .tokenization_stats()
                  .exceeded_max_token_num());
  EXPECT_THAT(put_result_proto.put_document_stats()
                  .tokenization_stats()
                  .num_tokens_indexed(),
              Eq(1));
}

TEST_F(IcingSearchEngineTest, PutDocumentShouldLogIndexMergeLatency) {
  DocumentProto document1 = DocumentBuilder()
                                .SetKey("icing", "fake_type/1")
                                .SetSchema("Message")
                                .AddStringProperty("body", kIpsumText)
                                .Build();
  DocumentProto document2 = DocumentBuilder()
                                .SetKey("icing", "fake_type/2")
                                .SetSchema("Message")
                                .AddStringProperty("body", kIpsumText)
                                .Build();

  // Create an icing instance with index_merge_size = document1's size.
  IcingSearchEngineOptions icing_options = GetDefaultIcingOptions();
  icing_options.set_index_merge_size(document1.ByteSizeLong());

  auto fake_clock = std::make_unique<FakeClock>();
  fake_clock->SetTimerElapsedMilliseconds(10);
  TestIcingSearchEngine icing(icing_options, std::make_unique<Filesystem>(),
                              std::make_unique<IcingFilesystem>(),
                              std::move(fake_clock), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());
  EXPECT_THAT(icing.Put(document1).status(), ProtoIsOk());

  // Putting document2 should trigger an index merge.
  PutResultProto put_result_proto = icing.Put(document2);
  EXPECT_THAT(put_result_proto.status(), ProtoIsOk());
  EXPECT_THAT(put_result_proto.put_document_stats().index_merge_latency_ms(),
              Eq(10));
}

TEST_F(IcingSearchEngineTest, SearchWithProjectionEmptyFieldPath) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreatePersonAndEmailSchema()).status(),
              ProtoIsOk());

  // 1. Add two email documents
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
          .AddStringProperty("subject", "Hello World!")
          .AddStringProperty(
              "body", "Oh what a beautiful morning! Oh what a beautiful day!")
          .Build();
  ASSERT_THAT(icing.Put(document_one).status(), ProtoIsOk());

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
          .AddStringProperty("subject", "Goodnight Moon!")
          .AddStringProperty("body",
                             "Count all the sheep and tell them 'Hello'.")
          .Build();
  ASSERT_THAT(icing.Put(document_two).status(), ProtoIsOk());

  // 2. Issue a query that will match those documents and use an empty field
  // mask to request NO properties.
  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  search_spec.set_query("hello");

  ResultSpecProto result_spec;
  // Retrieve only one result at a time to make sure that projection works when
  // retrieving all pages.
  result_spec.set_num_per_page(1);
  TypePropertyMask* email_field_mask = result_spec.add_type_property_masks();
  email_field_mask->set_schema_type("Email");
  email_field_mask->add_paths("");

  SearchResultProto results =
      icing.Search(search_spec, GetDefaultScoringSpec(), result_spec);
  EXPECT_THAT(results.status(), ProtoIsOk());
  EXPECT_THAT(results.results(), SizeIs(1));

  // 3. Verify that the returned results contain no properties.
  DocumentProto projected_document_two = DocumentBuilder()
                                             .SetKey("namespace", "uri2")
                                             .SetCreationTimestampMs(1000)
                                             .SetSchema("Email")
                                             .Build();
  EXPECT_THAT(results.results(0).document(),
              EqualsProto(projected_document_two));

  results = icing.GetNextPage(results.next_page_token());
  EXPECT_THAT(results.status(), ProtoIsOk());
  EXPECT_THAT(results.results(), SizeIs(1));
  DocumentProto projected_document_one = DocumentBuilder()
                                             .SetKey("namespace", "uri1")
                                             .SetCreationTimestampMs(1000)
                                             .SetSchema("Email")
                                             .Build();
  EXPECT_THAT(results.results(0).document(),
              EqualsProto(projected_document_one));
}

TEST_F(IcingSearchEngineTest, SearchWithProjectionMultipleFieldPaths) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreatePersonAndEmailSchema()).status(),
              ProtoIsOk());

  // 1. Add two email documents
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
          .AddStringProperty("subject", "Hello World!")
          .AddStringProperty(
              "body", "Oh what a beautiful morning! Oh what a beautiful day!")
          .Build();
  ASSERT_THAT(icing.Put(document_one).status(), ProtoIsOk());

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
          .AddStringProperty("subject", "Goodnight Moon!")
          .AddStringProperty("body",
                             "Count all the sheep and tell them 'Hello'.")
          .Build();
  ASSERT_THAT(icing.Put(document_two).status(), ProtoIsOk());

  // 2. Issue a query that will match those documents and request only
  // 'sender.name' and 'subject' properties.
  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  search_spec.set_query("hello");

  ResultSpecProto result_spec;
  // Retrieve only one result at a time to make sure that projection works when
  // retrieving all pages.
  result_spec.set_num_per_page(1);
  TypePropertyMask* email_field_mask = result_spec.add_type_property_masks();
  email_field_mask->set_schema_type("Email");
  email_field_mask->add_paths("sender.name");
  email_field_mask->add_paths("subject");

  SearchResultProto results =
      icing.Search(search_spec, GetDefaultScoringSpec(), result_spec);
  EXPECT_THAT(results.status(), ProtoIsOk());
  EXPECT_THAT(results.results(), SizeIs(1));

  // 3. Verify that the returned results only contain the 'sender.name'
  // property.
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
          .AddStringProperty("subject", "Goodnight Moon!")
          .Build();
  EXPECT_THAT(results.results(0).document(),
              EqualsProto(projected_document_two));

  results = icing.GetNextPage(results.next_page_token());
  EXPECT_THAT(results.status(), ProtoIsOk());
  EXPECT_THAT(results.results(), SizeIs(1));
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
          .AddStringProperty("subject", "Hello World!")
          .Build();
  EXPECT_THAT(results.results(0).document(),
              EqualsProto(projected_document_one));
}

TEST_F(IcingSearchEngineTest, QueryStatsProtoTest) {
  auto fake_clock = std::make_unique<FakeClock>();
  fake_clock->SetTimerElapsedMilliseconds(5);
  TestIcingSearchEngine icing(GetDefaultIcingOptions(),
                              std::make_unique<Filesystem>(),
                              std::make_unique<IcingFilesystem>(),
                              std::move(fake_clock), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  // Creates and inserts 5 documents
  DocumentProto document1 = CreateMessageDocument("namespace", "uri1");
  DocumentProto document2 = CreateMessageDocument("namespace", "uri2");
  DocumentProto document3 = CreateMessageDocument("namespace", "uri3");
  DocumentProto document4 = CreateMessageDocument("namespace", "uri4");
  DocumentProto document5 = CreateMessageDocument("namespace", "uri5");
  ASSERT_THAT(icing.Put(document1).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document2).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document3).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document4).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document5).status(), ProtoIsOk());

  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  search_spec.add_namespace_filters("namespace");
  search_spec.add_schema_type_filters(document1.schema());
  search_spec.set_query("message");

  ResultSpecProto result_spec;
  result_spec.set_num_per_page(2);
  result_spec.mutable_snippet_spec()->set_max_window_bytes(64);
  result_spec.mutable_snippet_spec()->set_num_matches_per_property(1);
  result_spec.mutable_snippet_spec()->set_num_to_snippet(3);

  ScoringSpecProto scoring_spec;
  scoring_spec.set_rank_by(
      ScoringSpecProto::RankingStrategy::CREATION_TIMESTAMP);

  // Searches and gets the first page, 2 results with 2 snippets
  SearchResultProto search_result =
      icing.Search(search_spec, scoring_spec, result_spec);
  ASSERT_THAT(search_result.status(), ProtoIsOk());
  ASSERT_THAT(search_result.results(), SizeIs(2));
  ASSERT_THAT(search_result.next_page_token(), Ne(kInvalidNextPageToken));

  // Check the stats
  QueryStatsProto exp_stats;
  exp_stats.set_query_length(7);
  exp_stats.set_num_terms(1);
  exp_stats.set_num_namespaces_filtered(1);
  exp_stats.set_num_schema_types_filtered(1);
  exp_stats.set_ranking_strategy(
      ScoringSpecProto::RankingStrategy::CREATION_TIMESTAMP);
  exp_stats.set_is_first_page(true);
  exp_stats.set_requested_page_size(2);
  exp_stats.set_num_results_returned_current_page(2);
  exp_stats.set_num_documents_scored(5);
  exp_stats.set_num_results_with_snippets(2);
  exp_stats.set_latency_ms(5);
  exp_stats.set_parse_query_latency_ms(5);
  exp_stats.set_scoring_latency_ms(5);
  exp_stats.set_ranking_latency_ms(5);
  exp_stats.set_document_retrieval_latency_ms(5);
  EXPECT_THAT(search_result.query_stats(), EqualsProto(exp_stats));

  // Second page, 2 result with 1 snippet
  search_result = icing.GetNextPage(search_result.next_page_token());
  ASSERT_THAT(search_result.status(), ProtoIsOk());
  ASSERT_THAT(search_result.results(), SizeIs(2));
  ASSERT_THAT(search_result.next_page_token(), Gt(kInvalidNextPageToken));

  exp_stats = QueryStatsProto();
  exp_stats.set_is_first_page(false);
  exp_stats.set_requested_page_size(2);
  exp_stats.set_num_results_returned_current_page(2);
  exp_stats.set_num_results_with_snippets(1);
  exp_stats.set_latency_ms(5);
  exp_stats.set_document_retrieval_latency_ms(5);
  EXPECT_THAT(search_result.query_stats(), EqualsProto(exp_stats));

  // Third page, 1 result with 0 snippets
  search_result = icing.GetNextPage(search_result.next_page_token());
  ASSERT_THAT(search_result.status(), ProtoIsOk());
  ASSERT_THAT(search_result.results(), SizeIs(1));
  ASSERT_THAT(search_result.next_page_token(), Eq(kInvalidNextPageToken));

  exp_stats = QueryStatsProto();
  exp_stats.set_is_first_page(false);
  exp_stats.set_requested_page_size(2);
  exp_stats.set_num_results_returned_current_page(1);
  exp_stats.set_num_results_with_snippets(0);
  exp_stats.set_latency_ms(5);
  exp_stats.set_document_retrieval_latency_ms(5);
  EXPECT_THAT(search_result.query_stats(), EqualsProto(exp_stats));
}

TEST_F(IcingSearchEngineTest, OptimizeStatsProtoTest) {
  auto fake_clock = std::make_unique<FakeClock>();
  fake_clock->SetTimerElapsedMilliseconds(5);
  fake_clock->SetSystemTimeMilliseconds(10000);
  auto icing = std::make_unique<TestIcingSearchEngine>(
      GetDefaultIcingOptions(), std::make_unique<Filesystem>(),
      std::make_unique<IcingFilesystem>(), std::move(fake_clock),
      GetTestJniCache());
  ASSERT_THAT(icing->Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing->SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  // Create three documents.
  DocumentProto document1 = CreateMessageDocument("namespace", "uri1");
  DocumentProto document2 = CreateMessageDocument("namespace", "uri2");
  document2.set_creation_timestamp_ms(9000);
  document2.set_ttl_ms(500);
  DocumentProto document3 = CreateMessageDocument("namespace", "uri3");
  ASSERT_THAT(icing->Put(document1).status(), ProtoIsOk());
  ASSERT_THAT(icing->Put(document2).status(), ProtoIsOk());
  ASSERT_THAT(icing->Put(document3).status(), ProtoIsOk());

  // Delete the first document.
  ASSERT_THAT(icing->Delete(document1.namespace_(), document1.uri()).status(),
              ProtoIsOk());
  ASSERT_THAT(icing->PersistToDisk(PersistType::FULL).status(), ProtoIsOk());

  OptimizeStatsProto expected;
  expected.set_latency_ms(5);
  expected.set_document_store_optimize_latency_ms(5);
  expected.set_index_restoration_latency_ms(5);
  expected.set_num_original_documents(3);
  expected.set_num_deleted_documents(1);
  expected.set_num_expired_documents(1);

  // Run Optimize
  OptimizeResultProto result = icing->Optimize();
  // Depending on how many blocks the documents end up spread across, it's
  // possible that Optimize can remove documents without shrinking storage. The
  // first Optimize call will also write the OptimizeStatusProto for the first
  // time which will take up 1 block. So make sure that before_size is no less
  // than after_size - 1 block.
  uint32_t page_size = getpagesize();
  EXPECT_THAT(result.optimize_stats().storage_size_before(),
              Ge(result.optimize_stats().storage_size_after() - page_size));
  result.mutable_optimize_stats()->clear_storage_size_before();
  result.mutable_optimize_stats()->clear_storage_size_after();
  EXPECT_THAT(result.optimize_stats(), EqualsProto(expected));

  fake_clock = std::make_unique<FakeClock>();
  fake_clock->SetTimerElapsedMilliseconds(5);
  fake_clock->SetSystemTimeMilliseconds(20000);
  icing = std::make_unique<TestIcingSearchEngine>(
      GetDefaultIcingOptions(), std::make_unique<Filesystem>(),
      std::make_unique<IcingFilesystem>(), std::move(fake_clock),
      GetTestJniCache());
  ASSERT_THAT(icing->Initialize().status(), ProtoIsOk());

  expected = OptimizeStatsProto();
  expected.set_latency_ms(5);
  expected.set_document_store_optimize_latency_ms(5);
  expected.set_index_restoration_latency_ms(5);
  expected.set_num_original_documents(1);
  expected.set_num_deleted_documents(0);
  expected.set_num_expired_documents(0);
  expected.set_time_since_last_optimize_ms(10000);

  // Run Optimize
  result = icing->Optimize();
  EXPECT_THAT(result.optimize_stats().storage_size_before(),
              Eq(result.optimize_stats().storage_size_after()));
  result.mutable_optimize_stats()->clear_storage_size_before();
  result.mutable_optimize_stats()->clear_storage_size_after();
  EXPECT_THAT(result.optimize_stats(), EqualsProto(expected));
}

TEST_F(IcingSearchEngineTest, StorageInfoTest) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  // Create three documents.
  DocumentProto document1 = CreateMessageDocument("namespace", "uri1");
  DocumentProto document2 = CreateMessageDocument("namespace", "uri2");
  DocumentProto document3 = CreateMessageDocument("namespace", "uri3");
  ASSERT_THAT(icing.Put(document1).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document2).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document3).status(), ProtoIsOk());

  // Ensure that total_storage_size is set. All the other stats are covered by
  // the classes that generate them.
  StorageInfoResultProto result = icing.GetStorageInfo();
  EXPECT_THAT(result.status(), ProtoIsOk());
  EXPECT_THAT(result.storage_info().total_storage_size(), Ge(0));
}

TEST_F(IcingSearchEngineTest, SnippetErrorTest) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  SchemaProto schema =
      SchemaBuilder()
          .AddType(SchemaTypeConfigBuilder().SetType("Generic").AddProperty(
              PropertyConfigBuilder()
                  .SetName("subject")
                  .SetDataTypeString(MATCH_PREFIX, TOKENIZER_PLAIN)
                  .SetCardinality(CARDINALITY_REPEATED)))
          .Build();
  ASSERT_THAT(icing.SetSchema(schema).status(), ProtoIsOk());

  DocumentProto document1 =
      DocumentBuilder()
          .SetKey("namespace", "uri1")
          .SetScore(10)
          .SetSchema("Generic")
          .AddStringProperty("subject", "I like cats", "I like dogs",
                             "I like birds", "I like fish")
          .Build();
  DocumentProto document2 =
      DocumentBuilder()
          .SetKey("namespace", "uri2")
          .SetScore(20)
          .SetSchema("Generic")
          .AddStringProperty("subject", "I like red", "I like green",
                             "I like blue", "I like yellow")
          .Build();
  DocumentProto document3 =
      DocumentBuilder()
          .SetKey("namespace", "uri3")
          .SetScore(5)
          .SetSchema("Generic")
          .AddStringProperty("subject", "I like cupcakes", "I like donuts",
                             "I like eclairs", "I like froyo")
          .Build();
  ASSERT_THAT(icing.Put(document1).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document2).status(), ProtoIsOk());
  ASSERT_THAT(icing.Put(document3).status(), ProtoIsOk());

  SearchSpecProto search_spec;
  search_spec.add_schema_type_filters("Generic");
  search_spec.set_term_match_type(TermMatchType::EXACT_ONLY);
  search_spec.set_query("like");
  ScoringSpecProto scoring_spec;
  scoring_spec.set_rank_by(ScoringSpecProto::RankingStrategy::DOCUMENT_SCORE);
  ResultSpecProto result_spec;
  result_spec.mutable_snippet_spec()->set_num_to_snippet(2);
  result_spec.mutable_snippet_spec()->set_num_matches_per_property(3);
  result_spec.mutable_snippet_spec()->set_max_window_bytes(4);
  SearchResultProto search_results =
      icing.Search(search_spec, scoring_spec, result_spec);

  ASSERT_THAT(search_results.results(), SizeIs(3));
  const SearchResultProto::ResultProto* result = &search_results.results(0);
  EXPECT_THAT(result->document().uri(), Eq("uri2"));
  ASSERT_THAT(result->snippet().entries(), SizeIs(3));
  const SnippetProto::EntryProto* entry = &result->snippet().entries(0);
  EXPECT_THAT(entry->property_name(), "subject[0]");
  std::string_view content = GetString(&result->document(), "subject[0]");
  EXPECT_THAT(GetMatches(content, *entry), ElementsAre("like"));

  entry = &result->snippet().entries(1);
  EXPECT_THAT(entry->property_name(), "subject[1]");
  content = GetString(&result->document(), "subject[1]");
  EXPECT_THAT(GetMatches(content, *entry), ElementsAre("like"));

  entry = &result->snippet().entries(2);
  EXPECT_THAT(entry->property_name(), "subject[2]");
  content = GetString(&result->document(), "subject[2]");
  EXPECT_THAT(GetMatches(content, *entry), ElementsAre("like"));

  result = &search_results.results(1);
  EXPECT_THAT(result->document().uri(), Eq("uri1"));
  ASSERT_THAT(result->snippet().entries(), SizeIs(3));
  entry = &result->snippet().entries(0);
  EXPECT_THAT(entry->property_name(), "subject[0]");
  content = GetString(&result->document(), "subject[0]");
  EXPECT_THAT(GetMatches(content, *entry), ElementsAre("like"));

  entry = &result->snippet().entries(1);
  ASSERT_THAT(entry->property_name(), "subject[1]");
  content = GetString(&result->document(), "subject[1]");
  EXPECT_THAT(GetMatches(content, *entry), ElementsAre("like"));

  entry = &result->snippet().entries(2);
  ASSERT_THAT(entry->property_name(), "subject[2]");
  content = GetString(&result->document(), "subject[2]");
  EXPECT_THAT(GetMatches(content, *entry), ElementsAre("like"));

  result = &search_results.results(2);
  ASSERT_THAT(result->document().uri(), Eq("uri3"));
  ASSERT_THAT(result->snippet().entries(), IsEmpty());
}

TEST_F(IcingSearchEngineTest, CJKSnippetTest) {
  IcingSearchEngine icing(GetDefaultIcingOptions(), GetTestJniCache());
  ASSERT_THAT(icing.Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing.SetSchema(CreateMessageSchema()).status(), ProtoIsOk());

  // String:     "我每天走路去上班。"
  //              ^ ^  ^   ^^
  // UTF8 idx:    0 3  9  15 18
  // UTF16 idx:   0 1  3   5 6
  // Breaks into segments: "我", "每天", "走路", "去", "上班"
  constexpr std::string_view kChinese = "我每天走路去上班。";
  DocumentProto document = DocumentBuilder()
                               .SetKey("namespace", "uri1")
                               .SetSchema("Message")
                               .AddStringProperty("body", kChinese)
                               .Build();
  ASSERT_THAT(icing.Put(document).status(), ProtoIsOk());

  // Search and request snippet matching but no windowing.
  SearchSpecProto search_spec;
  search_spec.set_query("走");
  search_spec.set_term_match_type(MATCH_PREFIX);

  ResultSpecProto result_spec;
  result_spec.mutable_snippet_spec()->set_num_to_snippet(
      std::numeric_limits<int>::max());
  result_spec.mutable_snippet_spec()->set_num_matches_per_property(
      std::numeric_limits<int>::max());

  // Search and make sure that we got a single successful result
  SearchResultProto search_results = icing.Search(
      search_spec, ScoringSpecProto::default_instance(), result_spec);
  ASSERT_THAT(search_results.status(), ProtoIsOk());
  ASSERT_THAT(search_results.results(), SizeIs(1));
  const SearchResultProto::ResultProto* result = &search_results.results(0);
  EXPECT_THAT(result->document().uri(), Eq("uri1"));

  // Ensure that one and only one property was matched and it was "body"
  ASSERT_THAT(result->snippet().entries(), SizeIs(1));
  const SnippetProto::EntryProto* entry = &result->snippet().entries(0);
  EXPECT_THAT(entry->property_name(), Eq("body"));

  // Get the content for "subject" and see what the match is.
  std::string_view content = GetString(&result->document(), "body");
  ASSERT_THAT(content, Eq(kChinese));

  // Ensure that there is one and only one match within "subject"
  ASSERT_THAT(entry->snippet_matches(), SizeIs(1));
  const SnippetMatchProto& match_proto = entry->snippet_matches(0);

  EXPECT_THAT(match_proto.exact_match_byte_position(), Eq(9));
  EXPECT_THAT(match_proto.exact_match_byte_length(), Eq(6));
  std::string_view match =
      content.substr(match_proto.exact_match_byte_position(),
                     match_proto.exact_match_byte_length());
  ASSERT_THAT(match, Eq("走路"));

  // Ensure that the utf-16 values are also as expected
  EXPECT_THAT(match_proto.exact_match_utf16_position(), Eq(3));
  EXPECT_THAT(match_proto.exact_match_utf16_length(), Eq(2));
}

// We skip this test case when we're running in a jni_test since the data files
// will be stored in the android-instrumented storage location, rather than the
// normal cc_library runfiles directory. To get that storage location, it's
// recommended to use the TestStorage APIs which handles different API
// levels/absolute vs relative/etc differences. Since that's only accessible on
// the java-side, and I haven't figured out a way to pass that directory path to
// this native side yet, we're just going to disable this. The functionality is
// already well-tested across 4 different emulated OS's so we're not losing much
// test coverage here.
#ifndef ICING_JNI_TEST
// Disable backwards compat test. This test is enabled in google3, but disabled
// in jetpack/framework because we didn't want to keep the binary testdata files
// in our repo.
#define DISABLE_BACKWARDS_COMPAT_TEST
#ifndef DISABLE_BACKWARDS_COMPAT_TEST
TEST_F(IcingSearchEngineTest, MigrateToPortableFileBackedProtoLog) {
  // Copy the testdata files into our IcingSearchEngine directory
  std::string dir_without_portable_log;
  if (IsAndroidX86()) {
    dir_without_portable_log = GetTestFilePath(
        "icing/testdata/not_portable_log/"
        "icing_search_engine_android_x86");
  } else if (IsAndroidArm()) {
    dir_without_portable_log = GetTestFilePath(
        "icing/testdata/not_portable_log/"
        "icing_search_engine_android_arm");
  } else if (IsIosPlatform()) {
    dir_without_portable_log = GetTestFilePath(
        "icing/testdata/not_portable_log/"
        "icing_search_engine_ios");
  } else {
    dir_without_portable_log = GetTestFilePath(
        "icing/testdata/not_portable_log/"
        "icing_search_engine_linux");
  }

  // Create dst directory that we'll initialize the IcingSearchEngine over.
  std::string base_dir = GetTestBaseDir() + "_migrate";
  ASSERT_THAT(filesystem()->DeleteDirectoryRecursively(base_dir.c_str()), true);
  ASSERT_THAT(filesystem()->CreateDirectoryRecursively(base_dir.c_str()), true);

  ASSERT_TRUE(filesystem()->CopyDirectory(dir_without_portable_log.c_str(),
                                          base_dir.c_str(),
                                          /*recursive=*/true));

  IcingSearchEngineOptions icing_options;
  icing_options.set_base_dir(base_dir);

  IcingSearchEngine icing(icing_options, GetTestJniCache());
  InitializeResultProto init_result = icing.Initialize();
  EXPECT_THAT(init_result.status(), ProtoIsOk());
  EXPECT_THAT(init_result.initialize_stats().document_store_data_status(),
              Eq(InitializeStatsProto::NO_DATA_LOSS));
  EXPECT_THAT(init_result.initialize_stats().document_store_recovery_cause(),
              Eq(InitializeStatsProto::NONE));
  EXPECT_THAT(init_result.initialize_stats().schema_store_recovery_cause(),
              Eq(InitializeStatsProto::NONE));
  EXPECT_THAT(init_result.initialize_stats().index_restoration_cause(),
              Eq(InitializeStatsProto::NONE));

  // Set up schema, this is the one used to validate documents in the testdata
  // files. Do not change unless you're also updating the testdata files.
  SchemaProto schema =
      SchemaBuilder()
          .AddType(SchemaTypeConfigBuilder()
                       .SetType("email")
                       .AddProperty(
                           PropertyConfigBuilder()
                               .SetName("subject")
                               .SetDataTypeString(MATCH_EXACT, TOKENIZER_PLAIN)
                               .SetCardinality(CARDINALITY_OPTIONAL))
                       .AddProperty(
                           PropertyConfigBuilder()
                               .SetName("body")
                               .SetDataTypeString(MATCH_EXACT, TOKENIZER_PLAIN)
                               .SetCardinality(CARDINALITY_OPTIONAL)))
          .Build();

  // Make sure our schema is still the same as we expect. If not, there's
  // definitely no way we're getting the documents back that we expect.
  GetSchemaResultProto expected_get_schema_result_proto;
  expected_get_schema_result_proto.mutable_status()->set_code(StatusProto::OK);
  *expected_get_schema_result_proto.mutable_schema() = schema;
  ASSERT_THAT(icing.GetSchema(), EqualsProto(expected_get_schema_result_proto));

  // These are the documents that are stored in the testdata files. Do not
  // change unless you're also updating the testdata files.
  DocumentProto document1 = DocumentBuilder()
                                .SetKey("namespace1", "uri1")
                                .SetSchema("email")
                                .SetCreationTimestampMs(10)
                                .AddStringProperty("subject", "foo")
                                .AddStringProperty("body", "bar")
                                .Build();

  DocumentProto document2 = DocumentBuilder()
                                .SetKey("namespace1", "uri2")
                                .SetSchema("email")
                                .SetCreationTimestampMs(20)
                                .SetScore(321)
                                .AddStringProperty("body", "baz bat")
                                .Build();

  DocumentProto document3 = DocumentBuilder()
                                .SetKey("namespace2", "uri1")
                                .SetSchema("email")
                                .SetCreationTimestampMs(30)
                                .SetScore(123)
                                .AddStringProperty("subject", "phoo")
                                .Build();

  // Document 1 and 3 were put normally, and document 2 was deleted in our
  // testdata files.
  EXPECT_THAT(icing
                  .Get(document1.namespace_(), document1.uri(),
                       GetResultSpecProto::default_instance())
                  .document(),
              EqualsProto(document1));
  EXPECT_THAT(icing
                  .Get(document2.namespace_(), document2.uri(),
                       GetResultSpecProto::default_instance())
                  .status(),
              ProtoStatusIs(StatusProto::NOT_FOUND));
  EXPECT_THAT(icing
                  .Get(document3.namespace_(), document3.uri(),
                       GetResultSpecProto::default_instance())
                  .document(),
              EqualsProto(document3));

  // Searching for "foo" should get us document1.
  SearchSpecProto search_spec;
  search_spec.set_term_match_type(TermMatchType::PREFIX);
  search_spec.set_query("foo");

  SearchResultProto expected_document1;
  expected_document1.mutable_status()->set_code(StatusProto::OK);
  *expected_document1.mutable_results()->Add()->mutable_document() = document1;

  SearchResultProto actual_results =
      icing.Search(search_spec, GetDefaultScoringSpec(),
                   ResultSpecProto::default_instance());
  EXPECT_THAT(actual_results,
              EqualsSearchResultIgnoreStatsAndScores(expected_document1));

  // Searching for "baz" would've gotten us document2, except it got deleted.
  // Make sure that it's cleared from our index too.
  search_spec.set_query("baz");

  SearchResultProto expected_no_documents;
  expected_no_documents.mutable_status()->set_code(StatusProto::OK);

  actual_results = icing.Search(search_spec, GetDefaultScoringSpec(),
                                ResultSpecProto::default_instance());
  EXPECT_THAT(actual_results,
              EqualsSearchResultIgnoreStatsAndScores(expected_no_documents));

  // Searching for "phoo" should get us document3.
  search_spec.set_query("phoo");

  SearchResultProto expected_document3;
  expected_document3.mutable_status()->set_code(StatusProto::OK);
  *expected_document3.mutable_results()->Add()->mutable_document() = document3;

  actual_results = icing.Search(search_spec, GetDefaultScoringSpec(),
                                ResultSpecProto::default_instance());
  EXPECT_THAT(actual_results,
              EqualsSearchResultIgnoreStatsAndScores(expected_document3));
}
#endif  // DISABLE_BACKWARDS_COMPAT_TEST
#endif  // !ICING_JNI_TEST

}  // namespace
}  // namespace lib
}  // namespace icing
