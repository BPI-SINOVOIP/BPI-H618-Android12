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

#include "icing/store/document-store.h"

#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <string>

#include "icing/text_classifier/lib3/utils/base/status.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "icing/absl_ports/str_cat.h"
#include "icing/document-builder.h"
#include "icing/file/file-backed-vector.h"
#include "icing/file/filesystem.h"
#include "icing/file/memory-mapped-file.h"
#include "icing/file/mock-filesystem.h"
#include "icing/helpers/icu/icu-data-file-helper.h"
#include "icing/portable/equals-proto.h"
#include "icing/portable/platform.h"
#include "icing/proto/document.pb.h"
#include "icing/proto/schema.pb.h"
#include "icing/proto/storage.pb.h"
#include "icing/schema-builder.h"
#include "icing/schema/schema-store.h"
#include "icing/store/corpus-associated-scoring-data.h"
#include "icing/store/corpus-id.h"
#include "icing/store/document-filter-data.h"
#include "icing/store/document-id.h"
#include "icing/store/document-log-creator.h"
#include "icing/store/namespace-id.h"
#include "icing/testing/common-matchers.h"
#include "icing/testing/fake-clock.h"
#include "icing/testing/test-data.h"
#include "icing/testing/tmp-directory.h"
#include "icing/tokenization/language-segmenter-factory.h"
#include "icing/tokenization/language-segmenter.h"
#include "icing/util/crc32.h"
#include "unicode/uloc.h"

namespace icing {
namespace lib {

namespace {

using ::icing::lib::portable_equals_proto::EqualsProto;
using ::testing::_;
using ::testing::Eq;
using ::testing::Ge;
using ::testing::Gt;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::IsFalse;
using ::testing::IsTrue;
using ::testing::Not;
using ::testing::Return;
using ::testing::UnorderedElementsAre;

const NamespaceStorageInfoProto& GetNamespaceStorageInfo(
    const DocumentStorageInfoProto& storage_info,
    const std::string& name_space) {
  for (const NamespaceStorageInfoProto& namespace_storage_info :
       storage_info.namespace_storage_info()) {
    if (namespace_storage_info.namespace_() == name_space) {
      return namespace_storage_info;
    }
  }
  // Didn't find our namespace, fail the test.
  EXPECT_TRUE(false) << "Failed to find namespace '" << name_space
                     << "' in DocumentStorageInfoProto.";
  return std::move(NamespaceStorageInfoProto());
}

constexpr PropertyConfigProto_Cardinality_Code CARDINALITY_OPTIONAL =
    PropertyConfigProto_Cardinality_Code_OPTIONAL;

constexpr StringIndexingConfig_TokenizerType_Code TOKENIZER_PLAIN =
    StringIndexingConfig_TokenizerType_Code_PLAIN;

constexpr TermMatchType_Code MATCH_EXACT = TermMatchType_Code_EXACT_ONLY;

constexpr PropertyConfigProto_DataType_Code TYPE_INT =
    PropertyConfigProto_DataType_Code_INT64;

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

class DocumentStoreTest : public ::testing::Test {
 protected:
  DocumentStoreTest()
      : test_dir_(GetTestTempDir() + "/icing"),
        document_store_dir_(test_dir_ + "/document_store"),
        schema_store_dir_(test_dir_ + "/schema_store") {
    test_document1_ =
        DocumentBuilder()
            .SetKey("icing", "email/1")
            .SetSchema("email")
            .AddStringProperty("subject", "subject foo")
            .AddStringProperty("body", "body bar")
            .SetScore(document1_score_)
            .SetCreationTimestampMs(
                document1_creation_timestamp_)  // A random timestamp
            .SetTtlMs(document1_ttl_)
            .Build();
    test_document2_ =
        DocumentBuilder()
            .SetKey("icing", "email/2")
            .SetSchema("email")
            .AddStringProperty("subject", "subject foo 2")
            .AddStringProperty("body", "body bar 2")
            .SetScore(document2_score_)
            .SetCreationTimestampMs(
                document2_creation_timestamp_)  // A random timestamp
            .SetTtlMs(document2_ttl_)
            .Build();
  }

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

    filesystem_.DeleteDirectoryRecursively(test_dir_.c_str());
    filesystem_.CreateDirectoryRecursively(test_dir_.c_str());
    filesystem_.CreateDirectoryRecursively(document_store_dir_.c_str());
    filesystem_.CreateDirectoryRecursively(schema_store_dir_.c_str());

    SchemaProto schema =
        SchemaBuilder()
            .AddType(
                SchemaTypeConfigBuilder()
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
    ICING_ASSERT_OK_AND_ASSIGN(
        schema_store_,
        SchemaStore::Create(&filesystem_, schema_store_dir_, &fake_clock_));
    ASSERT_THAT(schema_store_->SetSchema(schema), IsOk());

    language_segmenter_factory::SegmenterOptions segmenter_options(ULOC_US);
    ICING_ASSERT_OK_AND_ASSIGN(
        lang_segmenter_,
        language_segmenter_factory::Create(std::move(segmenter_options)));
  }

  void TearDown() override {
    filesystem_.DeleteDirectoryRecursively(test_dir_.c_str());
  }

  void CorruptDocStoreHeaderChecksumFile() {
    // Change the DocStore's header combined checksum so that it won't match the
    // recalculated checksum on initialization. This will force a regeneration
    // of derived files from ground truth.
    const std::string header_file =
        absl_ports::StrCat(document_store_dir_, "/document_store_header");
    DocumentStore::Header header;
    header.magic = DocumentStore::Header::kMagic;
    header.checksum = 10;  // Arbitrary garbage checksum
    filesystem_.DeleteFile(header_file.c_str());
    filesystem_.Write(header_file.c_str(), &header, sizeof(header));
  }

  const Filesystem filesystem_;
  const std::string test_dir_;
  FakeClock fake_clock_;
  const std::string document_store_dir_;
  const std::string schema_store_dir_;
  DocumentProto test_document1_;
  DocumentProto test_document2_;
  std::unique_ptr<SchemaStore> schema_store_;
  std::unique_ptr<LanguageSegmenter> lang_segmenter_;

  // Document1 values
  const int document1_score_ = 1;
  const int64_t document1_creation_timestamp_ = 1;
  const int64_t document1_ttl_ = 0;
  const int64_t document1_expiration_timestamp_ =
      std::numeric_limits<int64_t>::max();  // special_case where ttl=0

  // Document2 values
  const int document2_score_ = 2;
  const int64_t document2_creation_timestamp_ = 2;
  const int64_t document2_ttl_ = 1;
  const int64_t document2_expiration_timestamp_ = 3;  // creation + ttl
};

TEST_F(DocumentStoreTest, CreationWithNullPointerShouldFail) {
  EXPECT_THAT(DocumentStore::Create(/*filesystem=*/nullptr, document_store_dir_,
                                    &fake_clock_, schema_store_.get()),
              StatusIs(libtextclassifier3::StatusCode::FAILED_PRECONDITION));

  EXPECT_THAT(DocumentStore::Create(&filesystem_, document_store_dir_,
                                    /*clock=*/nullptr, schema_store_.get()),
              StatusIs(libtextclassifier3::StatusCode::FAILED_PRECONDITION));

  EXPECT_THAT(DocumentStore::Create(&filesystem_, document_store_dir_,
                                    &fake_clock_, /*schema_store=*/nullptr),
              StatusIs(libtextclassifier3::StatusCode::FAILED_PRECONDITION));
}

TEST_F(DocumentStoreTest, CreationWithBadFilesystemShouldFail) {
  MockFilesystem mock_filesystem;
  ON_CALL(mock_filesystem, OpenForWrite(_)).WillByDefault(Return(false));

  EXPECT_THAT(DocumentStore::Create(&mock_filesystem, document_store_dir_,
                                    &fake_clock_, schema_store_.get()),
              StatusIs(libtextclassifier3::StatusCode::INTERNAL));
}

TEST_F(DocumentStoreTest, PutAndGetInSameNamespaceOk) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  // Both documents have namespace of "icing"
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             doc_store->Put(test_document1_));
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             doc_store->Put(DocumentProto(test_document2_)));

  EXPECT_THAT(doc_store->Get(document_id1),
              IsOkAndHolds(EqualsProto(test_document1_)));
  EXPECT_THAT(doc_store->Get(document_id2),
              IsOkAndHolds(EqualsProto(test_document2_)));
}

TEST_F(DocumentStoreTest, PutAndGetAcrossNamespacesOk) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  // Can handle different namespaces with same url
  DocumentProto foo_document = DocumentBuilder()
                                   .SetKey("foo", "1")
                                   .SetSchema("email")
                                   .SetCreationTimestampMs(0)
                                   .Build();
  DocumentProto bar_document = DocumentBuilder()
                                   .SetKey("bar", "1")
                                   .SetSchema("email")
                                   .SetCreationTimestampMs(0)
                                   .Build();

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             doc_store->Put(foo_document));
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             doc_store->Put(DocumentProto(bar_document)));

  EXPECT_THAT(doc_store->Get(document_id1),
              IsOkAndHolds(EqualsProto(foo_document)));
  EXPECT_THAT(doc_store->Get(document_id2),
              IsOkAndHolds(EqualsProto(bar_document)));
}

// Validates that putting an document with the same key will overwrite previous
// document and old doc ids are not getting reused.
TEST_F(DocumentStoreTest, PutSameKey) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  // Creates two documents with the same key (namespace + uri)
  DocumentProto document1 = DocumentProto(test_document1_);
  DocumentProto document2 = DocumentProto(test_document1_);

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             doc_store->Put(document1));
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             doc_store->Put(document2));
  EXPECT_THAT(document_id1, Not(document_id2));
  // document2 overrides document1, so document_id1 becomes invalid
  EXPECT_THAT(doc_store->Get(document_id1),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
  EXPECT_THAT(doc_store->Get(document_id2),
              IsOkAndHolds(EqualsProto(document2)));

  // Makes sure that old doc ids are not getting reused.
  DocumentProto document3 = DocumentProto(test_document1_);
  document3.set_uri("another/uri/1");
  EXPECT_THAT(doc_store->Put(document3), IsOkAndHolds(Not(document_id1)));
}

TEST_F(DocumentStoreTest, IsDocumentExistingWithoutStatus) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             doc_store->Put(DocumentProto(test_document1_)));
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             doc_store->Put(DocumentProto(test_document2_)));

  EXPECT_THAT(doc_store->DoesDocumentExist(document_id1), IsTrue());
  EXPECT_THAT(doc_store->DoesDocumentExist(document_id2), IsTrue());

  DocumentId invalid_document_id_negative = -1;
  EXPECT_THAT(doc_store->DoesDocumentExist(invalid_document_id_negative),
              IsFalse());

  DocumentId invalid_document_id_greater_than_max = kMaxDocumentId + 2;
  EXPECT_THAT(
      doc_store->DoesDocumentExist(invalid_document_id_greater_than_max),
      IsFalse());

  EXPECT_THAT(doc_store->DoesDocumentExist(kInvalidDocumentId), IsFalse());

  DocumentId invalid_document_id_out_of_range = document_id2 + 1;
  EXPECT_THAT(doc_store->DoesDocumentExist(invalid_document_id_out_of_range),
              IsFalse());
}

TEST_F(DocumentStoreTest, GetDeletedDocumentNotFound) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> document_store =
      std::move(create_result.document_store);

  ICING_EXPECT_OK(document_store->Put(DocumentProto(test_document1_)));
  EXPECT_THAT(
      document_store->Get(test_document1_.namespace_(), test_document1_.uri()),
      IsOkAndHolds(EqualsProto(test_document1_)));

  ICING_EXPECT_OK(document_store->Delete(test_document1_.namespace_(),
                                         test_document1_.uri()));
  EXPECT_THAT(
      document_store->Get(test_document1_.namespace_(), test_document1_.uri()),
      StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
}

TEST_F(DocumentStoreTest, GetExpiredDocumentNotFound) {
  DocumentProto document = DocumentBuilder()
                               .SetKey("namespace", "uri")
                               .SetSchema("email")
                               .SetCreationTimestampMs(10)
                               .SetTtlMs(100)
                               .Build();

  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> document_store =
      std::move(create_result.document_store);

  ICING_EXPECT_OK(document_store->Put(document));
  EXPECT_THAT(document_store->Get("namespace", "uri"),
              IsOkAndHolds(EqualsProto(document)));

  // Some arbitrary time before the document's creation time (10) + ttl (100)
  fake_clock_.SetSystemTimeMilliseconds(109);
  EXPECT_THAT(document_store->Get("namespace", "uri"),
              IsOkAndHolds(EqualsProto(document)));

  // Some arbitrary time equal to the document's creation time (10) + ttl (100)
  fake_clock_.SetSystemTimeMilliseconds(110);
  EXPECT_THAT(document_store->Get("namespace", "uri"),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));

  // Some arbitrary time past the document's creation time (10) + ttl (100)
  fake_clock_.SetSystemTimeMilliseconds(200);
  EXPECT_THAT(document_store->Get("namespace", "uri"),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
}

TEST_F(DocumentStoreTest, GetInvalidDocumentId) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id,
                             doc_store->Put(DocumentProto(test_document1_)));

  DocumentId invalid_document_id_negative = -1;
  EXPECT_THAT(doc_store->Get(invalid_document_id_negative),
              StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT));

  DocumentId invalid_document_id_greater_than_max = kMaxDocumentId + 2;
  EXPECT_THAT(doc_store->Get(invalid_document_id_greater_than_max),
              StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT));

  EXPECT_THAT(doc_store->Get(kInvalidDocumentId),
              StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT));

  DocumentId invalid_document_id_out_of_range = document_id + 1;
  EXPECT_THAT(doc_store->Get(invalid_document_id_out_of_range),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
}

TEST_F(DocumentStoreTest, DeleteNonexistentDocumentNotFound) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> document_store =
      std::move(create_result.document_store);

  // Validates that deleting something non-existing won't append anything to
  // ground truth
  int64_t document_log_size_before = filesystem_.GetFileSize(
      absl_ports::StrCat(document_store_dir_, "/",
                         DocumentLogCreator::GetDocumentLogFilename())
          .c_str());

  EXPECT_THAT(
      document_store->Delete("nonexistent_namespace", "nonexistent_uri"),
      StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));

  int64_t document_log_size_after = filesystem_.GetFileSize(
      absl_ports::StrCat(document_store_dir_, "/",
                         DocumentLogCreator::GetDocumentLogFilename())
          .c_str());
  EXPECT_THAT(document_log_size_before, Eq(document_log_size_after));
}

TEST_F(DocumentStoreTest, DeleteAlreadyDeletedDocumentNotFound) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> document_store =
      std::move(create_result.document_store);

  ICING_EXPECT_OK(document_store->Put(test_document1_));

  // First time is OK
  ICING_EXPECT_OK(document_store->Delete(test_document1_.namespace_(),
                                         test_document1_.uri()));

  // Deleting it again is NOT_FOUND
  EXPECT_THAT(document_store->Delete(test_document1_.namespace_(),
                                     test_document1_.uri()),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
}

TEST_F(DocumentStoreTest, DeleteByNamespaceOk) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  DocumentProto document1 = test_document1_;
  document1.set_namespace_("namespace.1");
  document1.set_uri("uri1");
  ICING_ASSERT_OK(doc_store->Put(document1));

  DocumentProto document2 = test_document1_;
  document2.set_namespace_("namespace.2");
  document2.set_uri("uri1");
  ICING_ASSERT_OK(doc_store->Put(document2));

  DocumentProto document3 = test_document1_;
  document3.set_namespace_("namespace.3");
  document3.set_uri("uri1");
  ICING_ASSERT_OK(doc_store->Put(document3));

  DocumentProto document4 = test_document1_;
  document4.set_namespace_("namespace.1");
  document4.set_uri("uri2");
  ICING_ASSERT_OK(doc_store->Put(document4));

  // DELETE namespace.1. document1 and document 4 should be deleted. document2
  // and document3 should still be retrievable.
  DocumentStore::DeleteByGroupResult group_result =
      doc_store->DeleteByNamespace("namespace.1");
  EXPECT_THAT(group_result.status, IsOk());
  EXPECT_THAT(group_result.num_docs_deleted, Eq(2));
  EXPECT_THAT(doc_store->Get(document1.namespace_(), document1.uri()),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
  EXPECT_THAT(doc_store->Get(document2.namespace_(), document2.uri()),
              IsOkAndHolds(EqualsProto(document2)));
  EXPECT_THAT(doc_store->Get(document3.namespace_(), document3.uri()),
              IsOkAndHolds(EqualsProto(document3)));
  EXPECT_THAT(doc_store->Get(document4.namespace_(), document4.uri()),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
}

TEST_F(DocumentStoreTest, DeleteByNamespaceNonexistentNamespaceNotFound) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  // Validates that deleting something non-existing won't append anything to
  // ground truth
  int64_t document_log_size_before = filesystem_.GetFileSize(
      absl_ports::StrCat(document_store_dir_, "/",
                         DocumentLogCreator::GetDocumentLogFilename())
          .c_str());

  EXPECT_THAT(doc_store->DeleteByNamespace("nonexistent_namespace").status,
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));

  int64_t document_log_size_after = filesystem_.GetFileSize(
      absl_ports::StrCat(document_store_dir_, "/",
                         DocumentLogCreator::GetDocumentLogFilename())
          .c_str());
  EXPECT_THAT(document_log_size_before, Eq(document_log_size_after));
}

TEST_F(DocumentStoreTest, DeleteByNamespaceNoExistingDocumentsNotFound) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> document_store =
      std::move(create_result.document_store);

  ICING_EXPECT_OK(document_store->Put(test_document1_));
  ICING_EXPECT_OK(document_store->Delete(test_document1_.namespace_(),
                                         test_document1_.uri()));

  // At this point, there are no existing documents with the namespace, even
  // though Icing's derived files know about this namespace. We should still
  // return NOT_FOUND since nothing existing has this namespace.
  EXPECT_THAT(
      document_store->DeleteByNamespace(test_document1_.namespace_()).status,
      StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
}

TEST_F(DocumentStoreTest, DeleteByNamespaceRecoversOk) {
  DocumentProto document1 = test_document1_;
  document1.set_namespace_("namespace.1");
  document1.set_uri("uri1");

  DocumentProto document2 = test_document1_;
  document2.set_namespace_("namespace.2");
  document2.set_uri("uri1");

  DocumentProto document3 = test_document1_;
  document3.set_namespace_("namespace.3");
  document3.set_uri("uri1");

  DocumentProto document4 = test_document1_;
  document4.set_namespace_("namespace.1");
  document4.set_uri("uri2");

  int64_t document_log_size_before;
  {
    ICING_ASSERT_OK_AND_ASSIGN(
        DocumentStore::CreateResult create_result,
        DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                              schema_store_.get()));
    std::unique_ptr<DocumentStore> doc_store =
        std::move(create_result.document_store);

    ICING_ASSERT_OK(doc_store->Put(document1));
    ICING_ASSERT_OK(doc_store->Put(document2));
    ICING_ASSERT_OK(doc_store->Put(document3));
    ICING_ASSERT_OK(doc_store->Put(document4));

    // DELETE namespace.1. document1 and document 4 should be deleted. document2
    // and document3 should still be retrievable.
    DocumentStore::DeleteByGroupResult group_result =
        doc_store->DeleteByNamespace("namespace.1");
    EXPECT_THAT(group_result.status, IsOk());
    EXPECT_THAT(group_result.num_docs_deleted, Eq(2));

    document_log_size_before = filesystem_.GetFileSize(
        absl_ports::StrCat(document_store_dir_, "/",
                           DocumentLogCreator::GetDocumentLogFilename())
            .c_str());
  }  // Destructors should update checksum and persist all data to file.

  CorruptDocStoreHeaderChecksumFile();
  // Successfully recover from a corrupt derived file issue.
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  // Make sure we didn't add anything to the ground truth after we recovered.
  int64_t document_log_size_after = filesystem_.GetFileSize(
      absl_ports::StrCat(document_store_dir_, "/",
                         DocumentLogCreator::GetDocumentLogFilename())
          .c_str());
  EXPECT_EQ(document_log_size_before, document_log_size_after);

  EXPECT_THAT(doc_store->Get(document1.namespace_(), document1.uri()),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
  EXPECT_THAT(doc_store->Get(document2.namespace_(), document2.uri()),
              IsOkAndHolds(EqualsProto(document2)));
  EXPECT_THAT(doc_store->Get(document3.namespace_(), document3.uri()),
              IsOkAndHolds(EqualsProto(document3)));
  EXPECT_THAT(doc_store->Get(document4.namespace_(), document4.uri()),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
}

TEST_F(DocumentStoreTest, DeleteBySchemaTypeOk) {
  SchemaProto schema =
      SchemaBuilder()
          .AddType(SchemaTypeConfigBuilder().SetType("email"))
          .AddType(SchemaTypeConfigBuilder().SetType("message"))
          .AddType(SchemaTypeConfigBuilder().SetType("person"))
          .Build();

  std::string schema_store_dir = schema_store_dir_ + "_custom";
  filesystem_.DeleteDirectoryRecursively(schema_store_dir.c_str());
  filesystem_.CreateDirectoryRecursively(schema_store_dir.c_str());
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<SchemaStore> schema_store,
      SchemaStore::Create(&filesystem_, schema_store_dir, &fake_clock_));

  ICING_ASSERT_OK(schema_store->SetSchema(schema));

  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store.get()));
  std::unique_ptr<DocumentStore> document_store =
      std::move(create_result.document_store);

  DocumentProto email_document_1 = DocumentBuilder()
                                       .SetKey("namespace1", "1")
                                       .SetSchema("email")
                                       .SetCreationTimestampMs(1)
                                       .Build();
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId email_1_document_id,
                             document_store->Put(email_document_1));

  DocumentProto email_document_2 = DocumentBuilder()
                                       .SetKey("namespace2", "2")
                                       .SetSchema("email")
                                       .SetCreationTimestampMs(1)
                                       .Build();
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId email_2_document_id,
                             document_store->Put(email_document_2));

  DocumentProto message_document = DocumentBuilder()
                                       .SetKey("namespace", "3")
                                       .SetSchema("message")
                                       .SetCreationTimestampMs(1)
                                       .Build();
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId message_document_id,
                             document_store->Put(message_document));

  DocumentProto person_document = DocumentBuilder()
                                      .SetKey("namespace", "4")
                                      .SetSchema("person")
                                      .SetCreationTimestampMs(1)
                                      .Build();
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId person_document_id,
                             document_store->Put(person_document));

  // Delete the "email" type and ensure that it works across both
  // email_document's namespaces. And that other documents aren't affected.
  DocumentStore::DeleteByGroupResult group_result =
      document_store->DeleteBySchemaType("email");
  EXPECT_THAT(group_result.status, IsOk());
  EXPECT_THAT(group_result.num_docs_deleted, Eq(2));
  EXPECT_THAT(document_store->Get(email_1_document_id),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
  EXPECT_THAT(document_store->Get(email_2_document_id),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
  EXPECT_THAT(document_store->Get(message_document_id),
              IsOkAndHolds(EqualsProto(message_document)));
  EXPECT_THAT(document_store->Get(person_document_id),
              IsOkAndHolds(EqualsProto(person_document)));

  // Delete the "message" type and check that other documents aren't affected
  group_result = document_store->DeleteBySchemaType("message");
  EXPECT_THAT(group_result.status, IsOk());
  EXPECT_THAT(group_result.num_docs_deleted, Eq(1));
  EXPECT_THAT(document_store->Get(email_1_document_id),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
  EXPECT_THAT(document_store->Get(email_2_document_id),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
  EXPECT_THAT(document_store->Get(message_document_id),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
  EXPECT_THAT(document_store->Get(person_document_id),
              IsOkAndHolds(EqualsProto(person_document)));
}

TEST_F(DocumentStoreTest, DeleteBySchemaTypeNonexistentSchemaTypeNotFound) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> document_store =
      std::move(create_result.document_store);

  // Validates that deleting something non-existing won't append anything to
  // ground truth
  int64_t document_log_size_before = filesystem_.GetFileSize(
      absl_ports::StrCat(document_store_dir_, "/",
                         DocumentLogCreator::GetDocumentLogFilename())
          .c_str());

  EXPECT_THAT(document_store->DeleteBySchemaType("nonexistent_type").status,
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));

  int64_t document_log_size_after = filesystem_.GetFileSize(
      absl_ports::StrCat(document_store_dir_, "/",
                         DocumentLogCreator::GetDocumentLogFilename())
          .c_str());

  EXPECT_THAT(document_log_size_before, Eq(document_log_size_after));
}

TEST_F(DocumentStoreTest, DeleteBySchemaTypeNoExistingDocumentsNotFound) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> document_store =
      std::move(create_result.document_store);

  ICING_EXPECT_OK(document_store->Put(test_document1_));
  ICING_EXPECT_OK(document_store->Delete(test_document1_.namespace_(),
                                         test_document1_.uri()));

  EXPECT_THAT(
      document_store->DeleteBySchemaType(test_document1_.schema()).status,
      StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
}

TEST_F(DocumentStoreTest, DeleteBySchemaTypeRecoversOk) {
  SchemaProto schema =
      SchemaBuilder()
          .AddType(SchemaTypeConfigBuilder().SetType("email"))
          .AddType(SchemaTypeConfigBuilder().SetType("message"))
          .Build();

  std::string schema_store_dir = schema_store_dir_ + "_custom";
  filesystem_.DeleteDirectoryRecursively(schema_store_dir.c_str());
  filesystem_.CreateDirectoryRecursively(schema_store_dir.c_str());
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<SchemaStore> schema_store,
      SchemaStore::Create(&filesystem_, schema_store_dir, &fake_clock_));

  ICING_ASSERT_OK(schema_store->SetSchema(schema));

  DocumentId email_document_id;
  DocumentId message_document_id;

  DocumentProto email_document = DocumentBuilder()
                                     .SetKey("namespace", "1")
                                     .SetSchema("email")
                                     .SetCreationTimestampMs(1)
                                     .Build();

  DocumentProto message_document = DocumentBuilder()
                                       .SetKey("namespace", "2")
                                       .SetSchema("message")
                                       .SetCreationTimestampMs(1)
                                       .Build();
  int64_t document_log_size_before;
  {
    ICING_ASSERT_OK_AND_ASSIGN(
        DocumentStore::CreateResult create_result,
        DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                              schema_store.get()));
    std::unique_ptr<DocumentStore> document_store =
        std::move(create_result.document_store);

    ICING_ASSERT_OK_AND_ASSIGN(email_document_id,
                               document_store->Put(email_document));
    ICING_ASSERT_OK_AND_ASSIGN(message_document_id,
                               document_store->Put(message_document));

    // Delete "email". "message" documents should still be retrievable.
    DocumentStore::DeleteByGroupResult group_result =
        document_store->DeleteBySchemaType("email");
    EXPECT_THAT(group_result.status, IsOk());
    EXPECT_THAT(group_result.num_docs_deleted, Eq(1));

    document_log_size_before = filesystem_.GetFileSize(
        absl_ports::StrCat(document_store_dir_, "/",
                           DocumentLogCreator::GetDocumentLogFilename())
            .c_str());
  }  // Destructors should update checksum and persist all data to file.

  CorruptDocStoreHeaderChecksumFile();
  // Successfully recover from a corrupt derived file issue.
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store.get()));
  std::unique_ptr<DocumentStore> document_store =
      std::move(create_result.document_store);

  // Make sure we didn't add anything to the ground truth after we recovered.
  int64_t document_log_size_after = filesystem_.GetFileSize(
      absl_ports::StrCat(document_store_dir_, "/",
                         DocumentLogCreator::GetDocumentLogFilename())
          .c_str());
  EXPECT_EQ(document_log_size_before, document_log_size_after);

  EXPECT_THAT(document_store->Get(email_document_id),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
  EXPECT_THAT(document_store->Get(message_document_id),
              IsOkAndHolds(EqualsProto(message_document)));
}

TEST_F(DocumentStoreTest, PutDeleteThenPut) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);
  ICING_EXPECT_OK(doc_store->Put(test_document1_));
  ICING_EXPECT_OK(
      doc_store->Delete(test_document1_.namespace_(), test_document1_.uri()));
  ICING_EXPECT_OK(doc_store->Put(test_document1_));
}

TEST_F(DocumentStoreTest, DeletedSchemaTypeFromSchemaStoreRecoversOk) {
  SchemaProto schema =
      SchemaBuilder()
          .AddType(SchemaTypeConfigBuilder().SetType("email"))
          .AddType(SchemaTypeConfigBuilder().SetType("message"))
          .Build();

  std::string schema_store_dir = schema_store_dir_ + "_custom";
  filesystem_.DeleteDirectoryRecursively(schema_store_dir.c_str());
  filesystem_.CreateDirectoryRecursively(schema_store_dir.c_str());
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<SchemaStore> schema_store,
      SchemaStore::Create(&filesystem_, schema_store_dir, &fake_clock_));

  ICING_ASSERT_OK(schema_store->SetSchema(schema));

  DocumentId email_document_id;
  DocumentId message_document_id;

  DocumentProto email_document = DocumentBuilder()
                                     .SetKey("namespace", "email")
                                     .SetSchema("email")
                                     .SetCreationTimestampMs(1)
                                     .Build();

  DocumentProto message_document = DocumentBuilder()
                                       .SetKey("namespace", "message")
                                       .SetSchema("message")
                                       .SetCreationTimestampMs(1)
                                       .Build();
  int64_t document_log_size_before;
  {
    ICING_ASSERT_OK_AND_ASSIGN(
        DocumentStore::CreateResult create_result,
        DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                              schema_store.get()));
    std::unique_ptr<DocumentStore> document_store =
        std::move(create_result.document_store);

    ICING_ASSERT_OK_AND_ASSIGN(email_document_id,
                               document_store->Put(email_document));
    ICING_ASSERT_OK_AND_ASSIGN(message_document_id,
                               document_store->Put(message_document));

    // Delete "email". "message" documents should still be retrievable.
    DocumentStore::DeleteByGroupResult group_result =
        document_store->DeleteBySchemaType("email");
    EXPECT_THAT(group_result.status, IsOk());
    EXPECT_THAT(group_result.num_docs_deleted, Eq(1));

    EXPECT_THAT(document_store->Get(email_document_id),
                StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
    EXPECT_THAT(document_store->Get(message_document_id),
                IsOkAndHolds(EqualsProto(message_document)));

    document_log_size_before = filesystem_.GetFileSize(
        absl_ports::StrCat(document_store_dir_, "/",
                           DocumentLogCreator::GetDocumentLogFilename())
            .c_str());
  }  // Destructors should update checksum and persist all data to file.

  CorruptDocStoreHeaderChecksumFile();

  SchemaProto new_schema =
      SchemaBuilder()
          .AddType(SchemaTypeConfigBuilder().SetType("message"))
          .Build();
  ICING_EXPECT_OK(schema_store->SetSchema(
      new_schema, /*ignore_errors_and_delete_documents=*/true));

  // Successfully recover from a corrupt derived file issue.
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store.get()));
  std::unique_ptr<DocumentStore> document_store =
      std::move(create_result.document_store);

  // Make sure we didn't add anything to the ground truth after we recovered.
  int64_t document_log_size_after = filesystem_.GetFileSize(
      absl_ports::StrCat(document_store_dir_, "/",
                         DocumentLogCreator::GetDocumentLogFilename())
          .c_str());
  EXPECT_EQ(document_log_size_before, document_log_size_after);

  EXPECT_THAT(document_store->Get(email_document_id),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
  EXPECT_THAT(document_store->Get(message_document_id),
              IsOkAndHolds(EqualsProto(message_document)));
}

TEST_F(DocumentStoreTest, OptimizeInto) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  DocumentProto document1 = DocumentBuilder()
                                .SetKey("namespace", "uri1")
                                .SetSchema("email")
                                .SetCreationTimestampMs(100)
                                .SetTtlMs(1000)
                                .Build();

  DocumentProto document2 = DocumentBuilder()
                                .SetKey("namespace", "uri2")
                                .SetSchema("email")
                                .SetCreationTimestampMs(100)
                                .SetTtlMs(1000)
                                .Build();

  DocumentProto document3 = DocumentBuilder()
                                .SetKey("namespace", "uri3")
                                .SetSchema("email")
                                .SetCreationTimestampMs(100)
                                .SetTtlMs(100)
                                .Build();

  // Nothing should have expired yet.
  fake_clock_.SetSystemTimeMilliseconds(100);

  ICING_ASSERT_OK(doc_store->Put(document1));
  ICING_ASSERT_OK(doc_store->Put(document2));
  ICING_ASSERT_OK(doc_store->Put(document3));

  std::string original_document_log = absl_ports::StrCat(
      document_store_dir_, "/", DocumentLogCreator::GetDocumentLogFilename());

  int64_t original_size =
      filesystem_.GetFileSize(original_document_log.c_str());

  // Optimizing into the same directory is not allowed
  EXPECT_THAT(
      doc_store->OptimizeInto(document_store_dir_, lang_segmenter_.get()),
      StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT,
               HasSubstr("directory is the same")));

  std::string optimized_dir = document_store_dir_ + "_optimize";
  std::string optimized_document_log =
      optimized_dir + "/" + DocumentLogCreator::GetDocumentLogFilename();

  // Validates that the optimized document log has the same size if nothing is
  // deleted
  ASSERT_TRUE(filesystem_.DeleteDirectoryRecursively(optimized_dir.c_str()));
  ASSERT_TRUE(filesystem_.CreateDirectoryRecursively(optimized_dir.c_str()));
  ICING_ASSERT_OK(
      doc_store->OptimizeInto(optimized_dir, lang_segmenter_.get()));
  int64_t optimized_size1 =
      filesystem_.GetFileSize(optimized_document_log.c_str());
  EXPECT_EQ(original_size, optimized_size1);

  // Validates that the optimized document log has a smaller size if something
  // is deleted
  ASSERT_TRUE(filesystem_.DeleteDirectoryRecursively(optimized_dir.c_str()));
  ASSERT_TRUE(filesystem_.CreateDirectoryRecursively(optimized_dir.c_str()));
  ICING_ASSERT_OK(doc_store->Delete("namespace", "uri1"));
  ICING_ASSERT_OK(
      doc_store->OptimizeInto(optimized_dir, lang_segmenter_.get()));
  int64_t optimized_size2 =
      filesystem_.GetFileSize(optimized_document_log.c_str());
  EXPECT_THAT(original_size, Gt(optimized_size2));

  // Document3 has expired since this is past its creation (100) + ttl (100).
  // But document1 and document2 should be fine since their ttl's were 1000.
  fake_clock_.SetSystemTimeMilliseconds(300);

  // Validates that the optimized document log has a smaller size if something
  // expired
  ASSERT_TRUE(filesystem_.DeleteDirectoryRecursively(optimized_dir.c_str()));
  ASSERT_TRUE(filesystem_.CreateDirectoryRecursively(optimized_dir.c_str()));
  ICING_ASSERT_OK(
      doc_store->OptimizeInto(optimized_dir, lang_segmenter_.get()));
  int64_t optimized_size3 =
      filesystem_.GetFileSize(optimized_document_log.c_str());
  EXPECT_THAT(optimized_size2, Gt(optimized_size3));
}

TEST_F(DocumentStoreTest, ShouldRecoverFromDataLoss) {
  DocumentId document_id1, document_id2;
  {
    // Can put and delete fine.
    ICING_ASSERT_OK_AND_ASSIGN(
        DocumentStore::CreateResult create_result,
        DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                              schema_store_.get()));
    std::unique_ptr<DocumentStore> doc_store =
        std::move(create_result.document_store);

    ICING_ASSERT_OK_AND_ASSIGN(
        document_id1,
        doc_store->Put(DocumentProto(test_document1_), /*num_tokens=*/4));
    ICING_ASSERT_OK_AND_ASSIGN(
        document_id2,
        doc_store->Put(DocumentProto(test_document2_), /*num_tokens=*/4));
    EXPECT_THAT(doc_store->Get(document_id1),
                IsOkAndHolds(EqualsProto(test_document1_)));
    EXPECT_THAT(doc_store->Get(document_id2),
                IsOkAndHolds(EqualsProto(test_document2_)));
    // Checks derived score cache
    EXPECT_THAT(
        doc_store->GetDocumentAssociatedScoreData(document_id1),
        IsOkAndHolds(DocumentAssociatedScoreData(
            /*corpus_id=*/0, document1_score_, document1_creation_timestamp_,
            /*length_in_tokens=*/4)));
    EXPECT_THAT(
        doc_store->GetDocumentAssociatedScoreData(document_id2),
        IsOkAndHolds(DocumentAssociatedScoreData(
            /*corpus_id=*/0, document2_score_, document2_creation_timestamp_,
            /*length_in_tokens=*/4)));
    EXPECT_THAT(doc_store->GetCorpusAssociatedScoreData(/*corpus_id=*/0),
                IsOkAndHolds(CorpusAssociatedScoreData(
                    /*num_docs=*/2, /*sum_length_in_tokens=*/8)));

    // Delete document 1
    EXPECT_THAT(doc_store->Delete("icing", "email/1"), IsOk());
    EXPECT_THAT(doc_store->Get(document_id1),
                StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
    EXPECT_THAT(doc_store->Get(document_id2),
                IsOkAndHolds(EqualsProto(test_document2_)));
  }

  // "Corrupt" the content written in the log by adding non-checksummed data to
  // it. This will mess up the checksum of the proto log, forcing it to rewind
  // to the last saved point.
  DocumentProto document = DocumentBuilder().SetKey("namespace", "uri").Build();
  const std::string serialized_document = document.SerializeAsString();

  const std::string document_log_file = absl_ports::StrCat(
      document_store_dir_, "/", DocumentLogCreator::GetDocumentLogFilename());
  int64_t file_size = filesystem_.GetFileSize(document_log_file.c_str());
  filesystem_.PWrite(document_log_file.c_str(), file_size,
                     serialized_document.data(), serialized_document.size());

  // Successfully recover from a data loss issue.
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  EXPECT_THAT(doc_store->Get(document_id1),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
  EXPECT_THAT(doc_store->Get(document_id2),
              IsOkAndHolds(EqualsProto(test_document2_)));

  // Checks derived filter cache
  EXPECT_THAT(doc_store->GetDocumentFilterData(document_id2),
              IsOkAndHolds(DocumentFilterData(
                  /*namespace_id=*/0,
                  /*schema_type_id=*/0, document2_expiration_timestamp_)));
  // Checks derived score cache
  EXPECT_THAT(
      doc_store->GetDocumentAssociatedScoreData(document_id2),
      IsOkAndHolds(DocumentAssociatedScoreData(
          /*corpus_id=*/0, document2_score_, document2_creation_timestamp_,
          /*length_in_tokens=*/4)));
  EXPECT_THAT(doc_store->GetCorpusAssociatedScoreData(/*corpus_id=*/0),
              IsOkAndHolds(CorpusAssociatedScoreData(
                  /*num_docs=*/1, /*sum_length_in_tokens=*/4)));
}

TEST_F(DocumentStoreTest, ShouldRecoverFromCorruptDerivedFile) {
  DocumentId document_id1, document_id2;
  {
    // Can put and delete fine.
    ICING_ASSERT_OK_AND_ASSIGN(
        DocumentStore::CreateResult create_result,
        DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                              schema_store_.get()));
    std::unique_ptr<DocumentStore> doc_store =
        std::move(create_result.document_store);

    ICING_ASSERT_OK_AND_ASSIGN(
        document_id1,
        doc_store->Put(DocumentProto(test_document1_), /*num_tokens=*/4));
    ICING_ASSERT_OK_AND_ASSIGN(
        document_id2,
        doc_store->Put(DocumentProto(test_document2_), /*num_tokens=*/4));
    EXPECT_THAT(doc_store->Get(document_id1),
                IsOkAndHolds(EqualsProto(test_document1_)));
    EXPECT_THAT(doc_store->Get(document_id2),
                IsOkAndHolds(EqualsProto(test_document2_)));
    // Checks derived score cache
    EXPECT_THAT(
        doc_store->GetDocumentAssociatedScoreData(document_id1),
        IsOkAndHolds(DocumentAssociatedScoreData(
            /*corpus_id=*/0, document1_score_, document1_creation_timestamp_,
            /*length_in_tokens=*/4)));
    EXPECT_THAT(
        doc_store->GetDocumentAssociatedScoreData(document_id2),
        IsOkAndHolds(DocumentAssociatedScoreData(
            /*corpus_id=*/0, document2_score_, document2_creation_timestamp_,
            /*length_in_tokens=*/4)));
    EXPECT_THAT(doc_store->GetCorpusAssociatedScoreData(/*corpus_id=*/0),
                IsOkAndHolds(CorpusAssociatedScoreData(
                    /*num_docs=*/2, /*sum_length_in_tokens=*/8)));
    // Delete document 1
    EXPECT_THAT(doc_store->Delete("icing", "email/1"), IsOk());
    EXPECT_THAT(doc_store->Get(document_id1),
                StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
    EXPECT_THAT(doc_store->Get(document_id2),
                IsOkAndHolds(EqualsProto(test_document2_)));
  }

  // "Corrupt" one of the derived files by adding non-checksummed data to
  // it. This will mess up the checksum and throw an error on the derived file's
  // initialization.
  const std::string document_id_mapper_file =
      absl_ports::StrCat(document_store_dir_, "/document_id_mapper");
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<FileBackedVector<int64_t>> document_id_mapper,
      FileBackedVector<int64_t>::Create(
          filesystem_, document_id_mapper_file,
          MemoryMappedFile::READ_WRITE_AUTO_SYNC));
  int64_t corrupt_document_id = 3;
  int64_t corrupt_offset = 3;
  EXPECT_THAT(document_id_mapper->Set(corrupt_document_id, corrupt_offset),
              IsOk());

  // Successfully recover from a corrupt derived file issue.
  // NOTE: this doesn't trigger RegenerateDerivedFiles.
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  EXPECT_THAT(doc_store->Get(document_id1),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
  EXPECT_THAT(doc_store->Get(document_id2),
              IsOkAndHolds(EqualsProto(test_document2_)));

  // Checks derived filter cache
  EXPECT_THAT(doc_store->GetDocumentFilterData(document_id2),
              IsOkAndHolds(DocumentFilterData(
                  /*namespace_id=*/0,
                  /*schema_type_id=*/0, document2_expiration_timestamp_)));
  // Checks derived score cache - note that they aren't regenerated from
  // scratch.
  EXPECT_THAT(
      doc_store->GetDocumentAssociatedScoreData(document_id2),
      IsOkAndHolds(DocumentAssociatedScoreData(
          /*corpus_id=*/0, document2_score_, document2_creation_timestamp_,
          /*length_in_tokens=*/4)));
  EXPECT_THAT(doc_store->GetCorpusAssociatedScoreData(/*corpus_id=*/0),
              IsOkAndHolds(CorpusAssociatedScoreData(
                  /*num_docs=*/2, /*sum_length_in_tokens=*/8)));
}

TEST_F(DocumentStoreTest, ShouldRecoverFromBadChecksum) {
  DocumentId document_id1, document_id2;
  {
    // Can put and delete fine.
    ICING_ASSERT_OK_AND_ASSIGN(
        DocumentStore::CreateResult create_result,
        DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                              schema_store_.get()));
    std::unique_ptr<DocumentStore> doc_store =
        std::move(create_result.document_store);

    ICING_ASSERT_OK_AND_ASSIGN(
        document_id1,
        doc_store->Put(DocumentProto(test_document1_), /*num_tokens=*/4));
    ICING_ASSERT_OK_AND_ASSIGN(
        document_id2,
        doc_store->Put(DocumentProto(test_document2_), /*num_tokens=*/4));
    EXPECT_THAT(doc_store->Get(document_id1),
                IsOkAndHolds(EqualsProto(test_document1_)));
    EXPECT_THAT(doc_store->Get(document_id2),
                IsOkAndHolds(EqualsProto(test_document2_)));
    // Checks derived score cache
    EXPECT_THAT(
        doc_store->GetDocumentAssociatedScoreData(document_id1),
        IsOkAndHolds(DocumentAssociatedScoreData(
            /*corpus_id=*/0, document1_score_, document1_creation_timestamp_,
            /*length_in_tokens=*/4)));
    EXPECT_THAT(
        doc_store->GetDocumentAssociatedScoreData(document_id2),
        IsOkAndHolds(DocumentAssociatedScoreData(
            /*corpus_id=*/0, document2_score_, document2_creation_timestamp_,
            /*length_in_tokens=*/4)));
    EXPECT_THAT(doc_store->GetCorpusAssociatedScoreData(/*corpus_id=*/0),
                IsOkAndHolds(CorpusAssociatedScoreData(
                    /*num_docs=*/2, /*sum_length_in_tokens=*/8)));
    EXPECT_THAT(doc_store->Delete("icing", "email/1"), IsOk());
    EXPECT_THAT(doc_store->Get(document_id1),
                StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
    EXPECT_THAT(doc_store->Get(document_id2),
                IsOkAndHolds(EqualsProto(test_document2_)));
  }

  CorruptDocStoreHeaderChecksumFile();
  // Successfully recover from a corrupt derived file issue.
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  EXPECT_THAT(doc_store->Get(document_id1),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
  EXPECT_THAT(doc_store->Get(document_id2),
              IsOkAndHolds(EqualsProto(test_document2_)));

  // Checks derived filter cache
  EXPECT_THAT(doc_store->GetDocumentFilterData(document_id2),
              IsOkAndHolds(DocumentFilterData(
                  /*namespace_id=*/0,
                  /*schema_type_id=*/0, document2_expiration_timestamp_)));
  // Checks derived score cache
  EXPECT_THAT(
      doc_store->GetDocumentAssociatedScoreData(document_id2),
      IsOkAndHolds(DocumentAssociatedScoreData(
          /*corpus_id=*/0, document2_score_, document2_creation_timestamp_,
          /*length_in_tokens=*/4)));
  EXPECT_THAT(doc_store->GetCorpusAssociatedScoreData(/*corpus_id=*/0),
              IsOkAndHolds(CorpusAssociatedScoreData(
                  /*num_docs=*/1, /*sum_length_in_tokens=*/4)));
}

TEST_F(DocumentStoreTest, GetStorageInfo) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  DocumentStorageInfoProto doc_store_storage_info = doc_store->GetStorageInfo();
  int64_t empty_doc_store_size = doc_store_storage_info.document_store_size();
  EXPECT_THAT(empty_doc_store_size, Gt(0));

  DocumentProto document = DocumentBuilder()
                               .SetKey("icing", "email/1")
                               .SetSchema("email")
                               .AddStringProperty("subject", "foo")
                               .Build();

  // Since GetStorageInfo can only get sizes in increments of block_size, we
  // need to insert enough documents so the disk usage will increase by at least
  // 1 block size. The number 100 is a bit arbitrary, gotten from manually
  // testing.
  for (int i = 0; i < 100; ++i) {
    ICING_ASSERT_OK(doc_store->Put(document));
  }
  doc_store_storage_info = doc_store->GetStorageInfo();
  EXPECT_THAT(doc_store_storage_info.document_store_size(),
              Gt(empty_doc_store_size));

  // Bad file system
  MockFilesystem mock_filesystem;
  ON_CALL(mock_filesystem, GetDiskUsage(A<const char*>()))
      .WillByDefault(Return(Filesystem::kBadFileSize));
  ICING_ASSERT_OK_AND_ASSIGN(
      create_result,
      DocumentStore::Create(&mock_filesystem, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store_with_mock_filesystem =
      std::move(create_result.document_store);

  doc_store_storage_info = doc_store_with_mock_filesystem->GetStorageInfo();
  EXPECT_THAT(doc_store_storage_info.document_store_size(), Eq(-1));
}

TEST_F(DocumentStoreTest, MaxDocumentId) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  // Since the DocumentStore is empty, we get an invalid DocumentId
  EXPECT_THAT(doc_store->last_added_document_id(), Eq(kInvalidDocumentId));

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             doc_store->Put(DocumentProto(test_document1_)));
  EXPECT_THAT(doc_store->last_added_document_id(), Eq(document_id1));

  // Still returns the last DocumentId even if it was deleted
  ICING_ASSERT_OK(doc_store->Delete("icing", "email/1"));
  EXPECT_THAT(doc_store->last_added_document_id(), Eq(document_id1));

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             doc_store->Put(DocumentProto(test_document2_)));
  EXPECT_THAT(doc_store->last_added_document_id(), Eq(document_id2));
}

TEST_F(DocumentStoreTest, GetNamespaceId) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  DocumentProto document_namespace1 =
      DocumentBuilder().SetKey("namespace1", "1").SetSchema("email").Build();
  DocumentProto document_namespace2 =
      DocumentBuilder().SetKey("namespace2", "2").SetSchema("email").Build();

  ICING_ASSERT_OK(doc_store->Put(DocumentProto(document_namespace1)));
  ICING_ASSERT_OK(doc_store->Put(DocumentProto(document_namespace2)));

  // NamespaceId of 0 since it was the first namespace seen by the DocumentStore
  EXPECT_THAT(doc_store->GetNamespaceId("namespace1"), IsOkAndHolds(Eq(0)));

  // NamespaceId of 1 since it was the second namespace seen by the
  // DocumentStore
  EXPECT_THAT(doc_store->GetNamespaceId("namespace2"), IsOkAndHolds(Eq(1)));

  // DELETE namespace1 - document_namespace1 is deleted.
  DocumentStore::DeleteByGroupResult group_result =
      doc_store->DeleteByNamespace("namespace1");
  EXPECT_THAT(group_result.status, IsOk());
  EXPECT_THAT(group_result.num_docs_deleted, Eq(1));

  // NamespaceMapper doesn't care if the document has been deleted
  EXPECT_THAT(doc_store->GetNamespaceId("namespace1"), IsOkAndHolds(Eq(0)));
}

TEST_F(DocumentStoreTest, GetDuplicateNamespaceId) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  DocumentProto document1 =
      DocumentBuilder().SetKey("namespace", "1").SetSchema("email").Build();
  DocumentProto document2 =
      DocumentBuilder().SetKey("namespace", "2").SetSchema("email").Build();

  ICING_ASSERT_OK(doc_store->Put(document1));
  ICING_ASSERT_OK(doc_store->Put(document2));

  // NamespaceId of 0 since it was the first namespace seen by the DocumentStore
  EXPECT_THAT(doc_store->GetNamespaceId("namespace"), IsOkAndHolds(Eq(0)));
}

TEST_F(DocumentStoreTest, NonexistentNamespaceNotFound) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  EXPECT_THAT(doc_store->GetNamespaceId("nonexistent_namespace"),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
}

TEST_F(DocumentStoreTest, GetCorpusDuplicateCorpusId) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  DocumentProto document1 =
      DocumentBuilder().SetKey("namespace", "1").SetSchema("email").Build();
  DocumentProto document2 =
      DocumentBuilder().SetKey("namespace", "2").SetSchema("email").Build();

  ICING_ASSERT_OK(doc_store->Put(document1));
  ICING_ASSERT_OK(doc_store->Put(document2));

  // CorpusId of 0 since it was the first namespace seen by the DocumentStore
  EXPECT_THAT(doc_store->GetCorpusId("namespace", "email"),
              IsOkAndHolds(Eq(0)));
}

TEST_F(DocumentStoreTest, GetCorpusId) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  DocumentProto document_corpus1 =
      DocumentBuilder().SetKey("namespace1", "1").SetSchema("email").Build();
  DocumentProto document_corpus2 =
      DocumentBuilder().SetKey("namespace2", "2").SetSchema("email").Build();

  ICING_ASSERT_OK(doc_store->Put(DocumentProto(document_corpus1)));
  ICING_ASSERT_OK(doc_store->Put(DocumentProto(document_corpus2)));

  // CorpusId of 0 since it was the first corpus seen by the DocumentStore
  EXPECT_THAT(doc_store->GetCorpusId("namespace1", "email"),
              IsOkAndHolds(Eq(0)));

  // CorpusId of 1 since it was the second corpus seen by the
  // DocumentStore
  EXPECT_THAT(doc_store->GetCorpusId("namespace2", "email"),
              IsOkAndHolds(Eq(1)));

  // DELETE namespace1 - document_corpus1 is deleted.
  DocumentStore::DeleteByGroupResult group_result =
      doc_store->DeleteByNamespace("namespace1");
  EXPECT_THAT(group_result.status, IsOk());
  EXPECT_THAT(group_result.num_docs_deleted, Eq(1));

  // CorpusMapper doesn't care if the document has been deleted
  EXPECT_THAT(doc_store->GetNamespaceId("namespace1"), IsOkAndHolds(Eq(0)));
}

TEST_F(DocumentStoreTest, NonexistentCorpusNotFound) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  EXPECT_THAT(
      doc_store->GetCorpusId("nonexistent_namespace", "nonexistent_schema"),
      StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));

  DocumentProto document_corpus =
      DocumentBuilder().SetKey("namespace1", "1").SetSchema("email").Build();
  ICING_ASSERT_OK(doc_store->Put(DocumentProto(document_corpus)));

  EXPECT_THAT(doc_store->GetCorpusId("nonexistent_namespace", "email"),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
  EXPECT_THAT(doc_store->GetCorpusId("namespace1", "nonexistent_schema"),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
  EXPECT_THAT(doc_store->GetCorpusAssociatedScoreData(/*corpus_id=*/1),
              StatusIs(libtextclassifier3::StatusCode::OUT_OF_RANGE));
}

TEST_F(DocumentStoreTest, GetCorpusAssociatedScoreDataSameCorpus) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  DocumentProto document1 =
      DocumentBuilder().SetKey("namespace", "1").SetSchema("email").Build();
  DocumentProto document2 =
      DocumentBuilder().SetKey("namespace", "2").SetSchema("email").Build();

  ICING_ASSERT_OK(doc_store->Put(document1, /*num_tokens=*/5));
  ICING_ASSERT_OK(doc_store->Put(document2, /*num_tokens=*/7));

  // CorpusId of 0 since it was the first namespace seen by the DocumentStore
  EXPECT_THAT(doc_store->GetCorpusAssociatedScoreData(/*corpus_id=*/0),
              IsOkAndHolds(CorpusAssociatedScoreData(
                  /*num_docs=*/2, /*sum_length_in_tokens=*/12)));
  // Only one corpus exists
  EXPECT_THAT(doc_store->GetCorpusAssociatedScoreData(/*corpus_id=*/1),
              StatusIs(libtextclassifier3::StatusCode::OUT_OF_RANGE));
}

TEST_F(DocumentStoreTest, GetCorpusAssociatedScoreData) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  DocumentProto document_corpus1 =
      DocumentBuilder().SetKey("namespace1", "1").SetSchema("email").Build();
  DocumentProto document_corpus2 =
      DocumentBuilder().SetKey("namespace2", "2").SetSchema("email").Build();

  ICING_ASSERT_OK(
      doc_store->Put(DocumentProto(document_corpus1), /*num_tokens=*/5));
  ICING_ASSERT_OK(
      doc_store->Put(DocumentProto(document_corpus2), /*num_tokens=*/7));

  // CorpusId of 0 since it was the first corpus seen by the DocumentStore
  EXPECT_THAT(doc_store->GetCorpusAssociatedScoreData(/*corpus_id=*/0),
              IsOkAndHolds(CorpusAssociatedScoreData(
                  /*num_docs=*/1, /*sum_length_in_tokens=*/5)));

  // CorpusId of 1 since it was the second corpus seen by the
  // DocumentStore
  EXPECT_THAT(doc_store->GetCorpusAssociatedScoreData(/*corpus_id=*/1),
              IsOkAndHolds(CorpusAssociatedScoreData(
                  /*num_docs=*/1, /*sum_length_in_tokens=*/7)));

  // DELETE namespace1 - document_corpus1 is deleted.
  ICING_EXPECT_OK(doc_store->DeleteByNamespace("namespace1").status);

  // Corpus score cache doesn't care if the document has been deleted
  EXPECT_THAT(doc_store->GetCorpusAssociatedScoreData(/*corpus_id=*/0),
              IsOkAndHolds(CorpusAssociatedScoreData(
                  /*num_docs=*/1, /*sum_length_in_tokens=*/5)));
}

TEST_F(DocumentStoreTest, NonexistentCorpusAssociatedScoreDataOutOfRange) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  EXPECT_THAT(doc_store->GetCorpusAssociatedScoreData(/*corpus_id=*/0),
              StatusIs(libtextclassifier3::StatusCode::OUT_OF_RANGE));
}

TEST_F(DocumentStoreTest, GetDocumentAssociatedScoreDataSameCorpus) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  DocumentProto document1 =
      DocumentBuilder()
          .SetKey("namespace", "1")
          .SetSchema("email")
          .SetScore(document1_score_)
          .SetCreationTimestampMs(
              document1_creation_timestamp_)  // A random timestamp
          .Build();
  DocumentProto document2 =
      DocumentBuilder()
          .SetKey("namespace", "2")
          .SetSchema("email")
          .SetScore(document2_score_)
          .SetCreationTimestampMs(
              document2_creation_timestamp_)  // A random timestamp
          .Build();

  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentId document_id1,
      doc_store->Put(DocumentProto(document1), /*num_tokens=*/5));
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentId document_id2,
      doc_store->Put(DocumentProto(document2), /*num_tokens=*/7));

  EXPECT_THAT(
      doc_store->GetDocumentAssociatedScoreData(document_id1),
      IsOkAndHolds(DocumentAssociatedScoreData(
          /*corpus_id=*/0, document1_score_, document1_creation_timestamp_,
          /*length_in_tokens=*/5)));
  EXPECT_THAT(
      doc_store->GetDocumentAssociatedScoreData(document_id2),
      IsOkAndHolds(DocumentAssociatedScoreData(
          /*corpus_id=*/0, document2_score_, document2_creation_timestamp_,
          /*length_in_tokens=*/7)));
}

TEST_F(DocumentStoreTest, GetDocumentAssociatedScoreDataDifferentCorpus) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  DocumentProto document1 =
      DocumentBuilder()
          .SetKey("namespace1", "1")
          .SetSchema("email")
          .SetScore(document1_score_)
          .SetCreationTimestampMs(
              document1_creation_timestamp_)  // A random timestamp
          .Build();
  DocumentProto document2 =
      DocumentBuilder()
          .SetKey("namespace2", "2")
          .SetSchema("email")
          .SetScore(document2_score_)
          .SetCreationTimestampMs(
              document2_creation_timestamp_)  // A random timestamp
          .Build();

  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentId document_id1,
      doc_store->Put(DocumentProto(document1), /*num_tokens=*/5));
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentId document_id2,
      doc_store->Put(DocumentProto(document2), /*num_tokens=*/7));

  EXPECT_THAT(
      doc_store->GetDocumentAssociatedScoreData(document_id1),
      IsOkAndHolds(DocumentAssociatedScoreData(
          /*corpus_id=*/0, document1_score_, document1_creation_timestamp_,
          /*length_in_tokens=*/5)));
  EXPECT_THAT(
      doc_store->GetDocumentAssociatedScoreData(document_id2),
      IsOkAndHolds(DocumentAssociatedScoreData(
          /*corpus_id=*/1, document2_score_, document2_creation_timestamp_,
          /*length_in_tokens=*/7)));
}

TEST_F(DocumentStoreTest, NonexistentDocumentAssociatedScoreDataNotFound) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  EXPECT_THAT(doc_store->GetDocumentAssociatedScoreData(/*document_id=*/0),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
}

TEST_F(DocumentStoreTest, NonexistentDocumentFilterDataNotFound) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  EXPECT_THAT(doc_store->GetDocumentFilterData(/*document_id=*/0),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
}

TEST_F(DocumentStoreTest, DeleteClearsFilterCache) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id,
                             doc_store->Put(test_document1_));

  EXPECT_THAT(
      doc_store->GetDocumentFilterData(document_id),
      IsOkAndHolds(DocumentFilterData(
          /*namespace_id=*/0,
          /*schema_type_id=*/0,
          /*expiration_timestamp_ms=*/document1_expiration_timestamp_)));

  ICING_ASSERT_OK(doc_store->Delete("icing", "email/1"));
  // Associated entry of the deleted document is removed.
  EXPECT_THAT(doc_store->GetDocumentFilterData(document_id),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
}

TEST_F(DocumentStoreTest, DeleteClearsScoreCache) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id,
                             doc_store->Put(test_document1_, /*num_tokens=*/4));

  EXPECT_THAT(doc_store->GetDocumentAssociatedScoreData(document_id),
              IsOkAndHolds(DocumentAssociatedScoreData(
                  /*corpus_id=*/0,
                  /*document_score=*/document1_score_,
                  /*creation_timestamp_ms=*/document1_creation_timestamp_,
                  /*length_in_tokens=*/4)));

  ICING_ASSERT_OK(doc_store->Delete("icing", "email/1"));
  // Associated entry of the deleted document is removed.
  EXPECT_THAT(doc_store->GetDocumentAssociatedScoreData(document_id),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
}

TEST_F(DocumentStoreTest, DeleteShouldPreventUsageScores) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id,
                             doc_store->Put(test_document1_));

  // Report usage with type 1.
  UsageReport usage_report_type1 = CreateUsageReport(
      /*name_space=*/"icing", /*uri=*/"email/1", /*timestamp_ms=*/0,
      UsageReport::USAGE_TYPE1);
  ICING_ASSERT_OK(doc_store->ReportUsage(usage_report_type1));

  UsageStore::UsageScores expected_scores;
  expected_scores.usage_type1_count = 1;
  ASSERT_THAT(doc_store->GetUsageScores(document_id),
              IsOkAndHolds(expected_scores));

  // Delete the document.
  ICING_ASSERT_OK(doc_store->Delete("icing", "email/1"));

  // Can't report or get usage scores on the deleted document
  ASSERT_THAT(
      doc_store->ReportUsage(usage_report_type1),
      StatusIs(libtextclassifier3::StatusCode::NOT_FOUND,
               HasSubstr("Couldn't report usage on a nonexistent document")));

  ASSERT_THAT(doc_store->GetUsageScores(document_id),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND,
                       HasSubstr("Can't get usage scores")));
}

TEST_F(DocumentStoreTest, ExpirationShouldPreventUsageScores) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  DocumentProto document = DocumentBuilder()
                               .SetKey("icing", "email/1")
                               .SetSchema("email")
                               .AddStringProperty("subject", "subject foo")
                               .AddStringProperty("body", "body bar")
                               .SetScore(document1_score_)
                               .SetCreationTimestampMs(10)
                               .SetTtlMs(100)
                               .Build();

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id, doc_store->Put(document));

  // Some arbitrary time before the document's creation time (10) + ttl (100)
  fake_clock_.SetSystemTimeMilliseconds(109);

  // Report usage with type 1.
  UsageReport usage_report_type1 = CreateUsageReport(
      /*name_space=*/"icing", /*uri=*/"email/1", /*timestamp_ms=*/0,
      UsageReport::USAGE_TYPE1);
  ICING_ASSERT_OK(doc_store->ReportUsage(usage_report_type1));

  UsageStore::UsageScores expected_scores;
  expected_scores.usage_type1_count = 1;
  ASSERT_THAT(doc_store->GetUsageScores(document_id),
              IsOkAndHolds(expected_scores));

  // Some arbitrary time past the document's creation time (10) + ttl (100)
  fake_clock_.SetSystemTimeMilliseconds(200);

  // Can't report or get usage scores on the expired document
  ASSERT_THAT(
      doc_store->ReportUsage(usage_report_type1),
      StatusIs(libtextclassifier3::StatusCode::NOT_FOUND,
               HasSubstr("Couldn't report usage on a nonexistent document")));

  ASSERT_THAT(doc_store->GetUsageScores(document_id),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND,
                       HasSubstr("Can't get usage scores")));
}

TEST_F(DocumentStoreTest,
       ExpirationTimestampIsSumOfNonZeroTtlAndCreationTimestamp) {
  DocumentProto document = DocumentBuilder()
                               .SetKey("namespace1", "1")
                               .SetSchema("email")
                               .SetCreationTimestampMs(100)
                               .SetTtlMs(1000)
                               .Build();

  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id, doc_store->Put(document));

  EXPECT_THAT(
      doc_store->GetDocumentFilterData(document_id),
      IsOkAndHolds(DocumentFilterData(/*namespace_id=*/0,
                                      /*schema_type_id=*/0,
                                      /*expiration_timestamp_ms=*/1100)));
}

TEST_F(DocumentStoreTest, ExpirationTimestampIsInt64MaxIfTtlIsZero) {
  DocumentProto document = DocumentBuilder()
                               .SetKey("namespace1", "1")
                               .SetSchema("email")
                               .SetCreationTimestampMs(100)
                               .SetTtlMs(0)
                               .Build();

  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id, doc_store->Put(document));

  EXPECT_THAT(
      doc_store->GetDocumentFilterData(document_id),
      IsOkAndHolds(DocumentFilterData(
          /*namespace_id=*/0,
          /*schema_type_id=*/0,
          /*expiration_timestamp_ms=*/std::numeric_limits<int64_t>::max())));
}

TEST_F(DocumentStoreTest, ExpirationTimestampIsInt64MaxOnOverflow) {
  DocumentProto document =
      DocumentBuilder()
          .SetKey("namespace1", "1")
          .SetSchema("email")
          .SetCreationTimestampMs(std::numeric_limits<int64_t>::max() - 1)
          .SetTtlMs(std::numeric_limits<int64_t>::max() - 1)
          .Build();

  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id, doc_store->Put(document));

  EXPECT_THAT(
      doc_store->GetDocumentFilterData(document_id),
      IsOkAndHolds(DocumentFilterData(
          /*namespace_id=*/0,
          /*schema_type_id=*/0,
          /*expiration_timestamp_ms=*/std::numeric_limits<int64_t>::max())));
}

TEST_F(DocumentStoreTest, CreationTimestampShouldBePopulated) {
  // Creates a document without a given creation timestamp
  DocumentProto document_without_creation_timestamp =
      DocumentBuilder()
          .SetKey("icing", "email/1")
          .SetSchema("email")
          .AddStringProperty("subject", "subject foo")
          .AddStringProperty("body", "body bar")
          .Build();

  int64_t fake_real_time = 100;
  fake_clock_.SetSystemTimeMilliseconds(fake_real_time);
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentId document_id,
      doc_store->Put(document_without_creation_timestamp));

  ICING_ASSERT_OK_AND_ASSIGN(DocumentProto document_with_creation_timestamp,
                             doc_store->Get(document_id));

  // Now the creation timestamp should be set by document store.
  EXPECT_THAT(document_with_creation_timestamp.creation_timestamp_ms(),
              Eq(fake_real_time));
}

TEST_F(DocumentStoreTest, ShouldWriteAndReadScoresCorrectly) {
  DocumentProto document1 = DocumentBuilder()
                                .SetKey("icing", "email/1")
                                .SetSchema("email")
                                .AddStringProperty("subject", "subject foo")
                                // With default doc score 0
                                .Build();
  DocumentProto document2 = DocumentBuilder()
                                .SetKey("icing", "email/2")
                                .SetSchema("email")
                                .AddStringProperty("subject", "subject foo")
                                .SetScore(5)
                                .Build();

  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id1,
                             doc_store->Put(document1));
  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id2,
                             doc_store->Put(document2));

  EXPECT_THAT(doc_store->GetDocumentAssociatedScoreData(document_id1),
              IsOkAndHolds(DocumentAssociatedScoreData(
                  /*corpus_id=*/0,
                  /*document_score=*/0, /*creation_timestamp_ms=*/0,
                  /*length_in_tokens=*/0)));

  EXPECT_THAT(doc_store->GetDocumentAssociatedScoreData(document_id2),
              IsOkAndHolds(DocumentAssociatedScoreData(
                  /*corpus_id=*/0,
                  /*document_score=*/5, /*creation_timestamp_ms=*/0,
                  /*length_in_tokens=*/0)));
}

TEST_F(DocumentStoreTest, ComputeChecksumSameBetweenCalls) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> document_store =
      std::move(create_result.document_store);

  ICING_EXPECT_OK(document_store->Put(test_document1_));
  ICING_ASSERT_OK_AND_ASSIGN(Crc32 checksum, document_store->ComputeChecksum());

  // Calling ComputeChecksum again shouldn't change anything
  EXPECT_THAT(document_store->ComputeChecksum(), IsOkAndHolds(checksum));
}

TEST_F(DocumentStoreTest, ComputeChecksumSameAcrossInstances) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> document_store =
      std::move(create_result.document_store);

  ICING_EXPECT_OK(document_store->Put(test_document1_));
  ICING_ASSERT_OK_AND_ASSIGN(Crc32 checksum, document_store->ComputeChecksum());

  // Destroy the previous instance and recreate DocumentStore
  document_store.reset();
  ICING_ASSERT_OK_AND_ASSIGN(
      create_result, DocumentStore::Create(&filesystem_, document_store_dir_,
                                           &fake_clock_, schema_store_.get()));
  document_store = std::move(create_result.document_store);

  EXPECT_THAT(document_store->ComputeChecksum(), IsOkAndHolds(checksum));
}

TEST_F(DocumentStoreTest, ComputeChecksumChangesOnNewDocument) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> document_store =
      std::move(create_result.document_store);

  ICING_EXPECT_OK(document_store->Put(test_document1_));
  ICING_ASSERT_OK_AND_ASSIGN(Crc32 checksum, document_store->ComputeChecksum());

  ICING_EXPECT_OK(document_store->Put(test_document2_));
  EXPECT_THAT(document_store->ComputeChecksum(),
              IsOkAndHolds(Not(Eq(checksum))));
}

TEST_F(DocumentStoreTest, ComputeChecksumDoesntChangeOnNewUsage) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> document_store =
      std::move(create_result.document_store);

  ICING_EXPECT_OK(document_store->Put(test_document1_));
  ICING_ASSERT_OK_AND_ASSIGN(Crc32 checksum, document_store->ComputeChecksum());

  UsageReport usage_report =
      CreateUsageReport(test_document1_.namespace_(), test_document1_.uri(),
                        /*timestamp_ms=*/1000, UsageReport::USAGE_TYPE1);
  ICING_EXPECT_OK(document_store->ReportUsage(usage_report));
  EXPECT_THAT(document_store->ComputeChecksum(), IsOkAndHolds(Eq(checksum)));
}

TEST_F(DocumentStoreTest, RegenerateDerivedFilesSkipsUnknownSchemaTypeIds) {
  const std::string schema_store_dir = schema_store_dir_ + "_custom";

  DocumentId email_document_id;
  NamespaceId email_namespace_id;
  int64_t email_expiration_timestamp;
  DocumentProto email_document = DocumentBuilder()
                                     .SetKey("namespace", "email_uri")
                                     .SetSchema("email")
                                     .SetCreationTimestampMs(0)
                                     .Build();

  DocumentId message_document_id;
  NamespaceId message_namespace_id;
  int64_t message_expiration_timestamp;
  DocumentProto message_document = DocumentBuilder()
                                       .SetKey("namespace", "message_uri")
                                       .SetSchema("message")
                                       .SetCreationTimestampMs(0)
                                       .Build();

  {
    // Set a schema with "email" and "message"
    filesystem_.DeleteDirectoryRecursively(schema_store_dir.c_str());
    filesystem_.CreateDirectoryRecursively(schema_store_dir.c_str());
    ICING_ASSERT_OK_AND_ASSIGN(
        std::unique_ptr<SchemaStore> schema_store,
        SchemaStore::Create(&filesystem_, schema_store_dir, &fake_clock_));
    SchemaProto schema =
        SchemaBuilder()
            .AddType(SchemaTypeConfigBuilder().SetType("email"))
            .AddType(SchemaTypeConfigBuilder().SetType("message"))
            .Build();
    ICING_EXPECT_OK(schema_store->SetSchema(schema));

    ICING_ASSERT_OK_AND_ASSIGN(SchemaTypeId email_schema_type_id,
                               schema_store->GetSchemaTypeId("email"));
    ICING_ASSERT_OK_AND_ASSIGN(SchemaTypeId message_schema_type_id,
                               schema_store->GetSchemaTypeId("message"));

    ICING_ASSERT_OK_AND_ASSIGN(
        DocumentStore::CreateResult create_result,
        DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                              schema_store.get()));
    std::unique_ptr<DocumentStore> document_store =
        std::move(create_result.document_store);

    // Insert and verify a "email "document
    ICING_ASSERT_OK_AND_ASSIGN(
        email_document_id, document_store->Put(DocumentProto(email_document)));
    EXPECT_THAT(document_store->Get(email_document_id),
                IsOkAndHolds(EqualsProto(email_document)));
    ICING_ASSERT_OK_AND_ASSIGN(
        DocumentFilterData email_data,
        document_store->GetDocumentFilterData(email_document_id));
    EXPECT_THAT(email_data.schema_type_id(), Eq(email_schema_type_id));
    email_namespace_id = email_data.namespace_id();
    email_expiration_timestamp = email_data.expiration_timestamp_ms();

    // Insert and verify a "message" document
    ICING_ASSERT_OK_AND_ASSIGN(
        message_document_id,
        document_store->Put(DocumentProto(message_document)));
    EXPECT_THAT(document_store->Get(message_document_id),
                IsOkAndHolds(EqualsProto(message_document)));
    ICING_ASSERT_OK_AND_ASSIGN(
        DocumentFilterData message_data,
        document_store->GetDocumentFilterData(message_document_id));
    EXPECT_THAT(message_data.schema_type_id(), Eq(message_schema_type_id));
    message_namespace_id = message_data.namespace_id();
    message_expiration_timestamp = message_data.expiration_timestamp_ms();
  }  // Everything destructs and commits changes to file

  CorruptDocStoreHeaderChecksumFile();

  // Change the schema so that we don't know of the Document's type anymore.
  // Since we can't set backwards incompatible changes, we do some file-level
  // hacks to "reset" the schema. Without a previously existing schema, the new
  // schema isn't considered backwards incompatible
  filesystem_.DeleteDirectoryRecursively(schema_store_dir.c_str());
  filesystem_.CreateDirectoryRecursively(schema_store_dir.c_str());
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<SchemaStore> schema_store,
      SchemaStore::Create(&filesystem_, schema_store_dir, &fake_clock_));

  SchemaProto schema = SchemaBuilder()
                           .AddType(SchemaTypeConfigBuilder().SetType("email"))
                           .Build();
  ICING_EXPECT_OK(schema_store->SetSchema(schema));

  ICING_ASSERT_OK_AND_ASSIGN(SchemaTypeId email_schema_type_id,
                             schema_store->GetSchemaTypeId("email"));

  // Successfully recover from a corrupt derived file issue. We don't fail just
  // because the "message" schema type is missing
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store.get()));
  std::unique_ptr<DocumentStore> document_store =
      std::move(create_result.document_store);

  // "email" document is fine
  EXPECT_THAT(document_store->Get(email_document_id),
              IsOkAndHolds(EqualsProto(email_document)));
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentFilterData email_data,
      document_store->GetDocumentFilterData(email_document_id));
  EXPECT_THAT(email_data.schema_type_id(), Eq(email_schema_type_id));
  // Make sure that all the other fields are stll valid/the same
  EXPECT_THAT(email_data.namespace_id(), Eq(email_namespace_id));
  EXPECT_THAT(email_data.expiration_timestamp_ms(),
              Eq(email_expiration_timestamp));

  // "message" document has an invalid SchemaTypeId
  EXPECT_THAT(document_store->Get(message_document_id),
              IsOkAndHolds(EqualsProto(message_document)));
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentFilterData message_data,
      document_store->GetDocumentFilterData(message_document_id));
  EXPECT_THAT(message_data.schema_type_id(), Eq(-1));
  // Make sure that all the other fields are stll valid/the same
  EXPECT_THAT(message_data.namespace_id(), Eq(message_namespace_id));
  EXPECT_THAT(message_data.expiration_timestamp_ms(),
              Eq(message_expiration_timestamp));
}

TEST_F(DocumentStoreTest, UpdateSchemaStoreUpdatesSchemaTypeIds) {
  const std::string schema_store_dir = test_dir_ + "_custom";
  filesystem_.DeleteDirectoryRecursively(schema_store_dir.c_str());
  filesystem_.CreateDirectoryRecursively(schema_store_dir.c_str());

  // Set a schema
  SchemaProto schema =
      SchemaBuilder()
          .AddType(SchemaTypeConfigBuilder().SetType("email"))
          .AddType(SchemaTypeConfigBuilder().SetType("message"))
          .Build();

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<SchemaStore> schema_store,
      SchemaStore::Create(&filesystem_, schema_store_dir, &fake_clock_));
  ICING_EXPECT_OK(schema_store->SetSchema(schema));

  ICING_ASSERT_OK_AND_ASSIGN(SchemaTypeId old_email_schema_type_id,
                             schema_store->GetSchemaTypeId("email"));
  ICING_ASSERT_OK_AND_ASSIGN(SchemaTypeId old_message_schema_type_id,
                             schema_store->GetSchemaTypeId("message"));

  DocumentProto email_document = DocumentBuilder()
                                     .SetNamespace("namespace")
                                     .SetUri("email_uri")
                                     .SetSchema("email")
                                     .Build();

  DocumentProto message_document = DocumentBuilder()
                                       .SetNamespace("namespace")
                                       .SetUri("message_uri")
                                       .SetSchema("message")
                                       .Build();

  // Add the documents and check SchemaTypeIds match
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store.get()));
  std::unique_ptr<DocumentStore> document_store =
      std::move(create_result.document_store);

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId email_document_id,
                             document_store->Put(email_document));
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentFilterData email_data,
      document_store->GetDocumentFilterData(email_document_id));
  EXPECT_THAT(email_data.schema_type_id(), Eq(old_email_schema_type_id));

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId message_document_id,
                             document_store->Put(message_document));
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentFilterData message_data,
      document_store->GetDocumentFilterData(message_document_id));
  EXPECT_THAT(message_data.schema_type_id(), Eq(old_message_schema_type_id));

  // Rearrange the schema types. Since SchemaTypeId is assigned based on order,
  // this should change the SchemaTypeIds.
  schema = SchemaBuilder()
               .AddType(SchemaTypeConfigBuilder().SetType("message"))
               .AddType(SchemaTypeConfigBuilder().SetType("email"))
               .Build();

  ICING_EXPECT_OK(schema_store->SetSchema(schema));

  ICING_ASSERT_OK_AND_ASSIGN(SchemaTypeId new_email_schema_type_id,
                             schema_store->GetSchemaTypeId("email"));
  ICING_ASSERT_OK_AND_ASSIGN(SchemaTypeId new_message_schema_type_id,
                             schema_store->GetSchemaTypeId("message"));

  // SchemaTypeIds should have changed.
  EXPECT_NE(old_email_schema_type_id, new_email_schema_type_id);
  EXPECT_NE(old_message_schema_type_id, new_message_schema_type_id);

  ICING_EXPECT_OK(document_store->UpdateSchemaStore(schema_store.get()));

  // Check that the FilterCache holds the new SchemaTypeIds
  ICING_ASSERT_OK_AND_ASSIGN(
      email_data, document_store->GetDocumentFilterData(email_document_id));
  EXPECT_THAT(email_data.schema_type_id(), Eq(new_email_schema_type_id));

  ICING_ASSERT_OK_AND_ASSIGN(
      message_data, document_store->GetDocumentFilterData(message_document_id));
  EXPECT_THAT(message_data.schema_type_id(), Eq(new_message_schema_type_id));
}

TEST_F(DocumentStoreTest, UpdateSchemaStoreDeletesInvalidDocuments) {
  const std::string schema_store_dir = test_dir_ + "_custom";
  filesystem_.DeleteDirectoryRecursively(schema_store_dir.c_str());
  filesystem_.CreateDirectoryRecursively(schema_store_dir.c_str());

  // Set a schema
  SchemaProto schema =
      SchemaBuilder()
          .AddType(SchemaTypeConfigBuilder().SetType("email").AddProperty(
              PropertyConfigBuilder()
                  .SetName("subject")
                  .SetDataTypeString(MATCH_EXACT, TOKENIZER_PLAIN)
                  .SetCardinality(CARDINALITY_OPTIONAL)))
          .Build();

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<SchemaStore> schema_store,
      SchemaStore::Create(&filesystem_, schema_store_dir, &fake_clock_));
  ICING_EXPECT_OK(schema_store->SetSchema(schema));

  // Add two documents, with and without a subject
  DocumentProto email_without_subject = DocumentBuilder()
                                            .SetNamespace("namespace")
                                            .SetUri("email_uri_without_subject")
                                            .SetSchema("email")
                                            .SetCreationTimestampMs(0)
                                            .Build();

  DocumentProto email_with_subject = DocumentBuilder()
                                         .SetNamespace("namespace")
                                         .SetUri("email_uri_with_subject")
                                         .SetSchema("email")
                                         .AddStringProperty("subject", "foo")
                                         .SetCreationTimestampMs(0)
                                         .Build();

  // Insert documents and check they're ok
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store.get()));
  std::unique_ptr<DocumentStore> document_store =
      std::move(create_result.document_store);

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId email_without_subject_document_id,
                             document_store->Put(email_without_subject));
  EXPECT_THAT(document_store->Get(email_without_subject_document_id),
              IsOkAndHolds(EqualsProto(email_without_subject)));

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId email_with_subject_document_id,
                             document_store->Put(email_with_subject));
  EXPECT_THAT(document_store->Get(email_with_subject_document_id),
              IsOkAndHolds(EqualsProto(email_with_subject)));

  // Changing an OPTIONAL field to REQUIRED is backwards incompatible, and will
  // invalidate all documents that don't have this property set
  schema.mutable_types(0)->mutable_properties(0)->set_cardinality(
      PropertyConfigProto::Cardinality::REQUIRED);

  ICING_EXPECT_OK(schema_store->SetSchema(
      schema, /*ignore_errors_and_delete_documents=*/true));

  ICING_EXPECT_OK(document_store->UpdateSchemaStore(schema_store.get()));

  // The email without a subject should be marked as deleted
  EXPECT_THAT(document_store->Get(email_without_subject_document_id),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));

  // The email with a subject should be unaffected
  EXPECT_THAT(document_store->Get(email_with_subject_document_id),
              IsOkAndHolds(EqualsProto(email_with_subject)));
}

TEST_F(DocumentStoreTest,
       UpdateSchemaStoreDeletesDocumentsByDeletedSchemaType) {
  const std::string schema_store_dir = test_dir_ + "_custom";
  filesystem_.DeleteDirectoryRecursively(schema_store_dir.c_str());
  filesystem_.CreateDirectoryRecursively(schema_store_dir.c_str());

  // Set a schema
  SchemaProto schema =
      SchemaBuilder()
          .AddType(SchemaTypeConfigBuilder().SetType("email"))
          .AddType(SchemaTypeConfigBuilder().SetType("message"))
          .Build();

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<SchemaStore> schema_store,
      SchemaStore::Create(&filesystem_, schema_store_dir, &fake_clock_));
  ICING_EXPECT_OK(schema_store->SetSchema(schema));

  // Add a "email" and "message" document
  DocumentProto email_document = DocumentBuilder()
                                     .SetNamespace("namespace")
                                     .SetUri("email_uri")
                                     .SetSchema("email")
                                     .SetCreationTimestampMs(0)
                                     .Build();

  DocumentProto message_document = DocumentBuilder()
                                       .SetNamespace("namespace")
                                       .SetUri("message_uri")
                                       .SetSchema("message")
                                       .SetCreationTimestampMs(0)
                                       .Build();

  // Insert documents and check they're ok
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store.get()));
  std::unique_ptr<DocumentStore> document_store =
      std::move(create_result.document_store);

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId email_document_id,
                             document_store->Put(email_document));
  EXPECT_THAT(document_store->Get(email_document_id),
              IsOkAndHolds(EqualsProto(email_document)));

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId message_document_id,
                             document_store->Put(message_document));
  EXPECT_THAT(document_store->Get(message_document_id),
              IsOkAndHolds(EqualsProto(message_document)));

  SchemaProto new_schema =
      SchemaBuilder()
          .AddType(SchemaTypeConfigBuilder().SetType("message"))
          .Build();

  ICING_EXPECT_OK(
      schema_store->SetSchema(new_schema,
                              /*ignore_errors_and_delete_documents=*/true));

  ICING_EXPECT_OK(document_store->UpdateSchemaStore(schema_store.get()));

  // The "email" type is unknown now, so the "email" document should be deleted
  EXPECT_THAT(document_store->Get(email_document_id),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));

  // The "message" document should be unaffected
  EXPECT_THAT(document_store->Get(message_document_id),
              IsOkAndHolds(EqualsProto(message_document)));
}

TEST_F(DocumentStoreTest, OptimizedUpdateSchemaStoreUpdatesSchemaTypeIds) {
  const std::string schema_store_dir = test_dir_ + "_custom";
  filesystem_.DeleteDirectoryRecursively(schema_store_dir.c_str());
  filesystem_.CreateDirectoryRecursively(schema_store_dir.c_str());

  // Set a schema
  SchemaProto schema =
      SchemaBuilder()
          .AddType(SchemaTypeConfigBuilder().SetType("email"))
          .AddType(SchemaTypeConfigBuilder().SetType("message"))
          .Build();

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<SchemaStore> schema_store,
      SchemaStore::Create(&filesystem_, schema_store_dir, &fake_clock_));
  ICING_EXPECT_OK(schema_store->SetSchema(schema));

  ICING_ASSERT_OK_AND_ASSIGN(SchemaTypeId old_email_schema_type_id,
                             schema_store->GetSchemaTypeId("email"));
  ICING_ASSERT_OK_AND_ASSIGN(SchemaTypeId old_message_schema_type_id,
                             schema_store->GetSchemaTypeId("message"));

  DocumentProto email_document = DocumentBuilder()
                                     .SetNamespace("namespace")
                                     .SetUri("email_uri")
                                     .SetSchema("email")
                                     .Build();

  DocumentProto message_document = DocumentBuilder()
                                       .SetNamespace("namespace")
                                       .SetUri("message_uri")
                                       .SetSchema("message")
                                       .Build();

  // Add the documents and check SchemaTypeIds match
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store.get()));
  std::unique_ptr<DocumentStore> document_store =
      std::move(create_result.document_store);

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId email_document_id,
                             document_store->Put(email_document));
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentFilterData email_data,
      document_store->GetDocumentFilterData(email_document_id));
  EXPECT_THAT(email_data.schema_type_id(), Eq(old_email_schema_type_id));

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId message_document_id,
                             document_store->Put(message_document));
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentFilterData message_data,
      document_store->GetDocumentFilterData(message_document_id));
  EXPECT_THAT(message_data.schema_type_id(), Eq(old_message_schema_type_id));

  // Rearrange the schema types. Since SchemaTypeId is assigned based on order,
  // this should change the SchemaTypeIds.
  schema = SchemaBuilder()
               .AddType(SchemaTypeConfigBuilder().SetType("message"))
               .AddType(SchemaTypeConfigBuilder().SetType("email"))
               .Build();

  ICING_ASSERT_OK_AND_ASSIGN(SchemaStore::SetSchemaResult set_schema_result,
                             schema_store->SetSchema(schema));

  ICING_ASSERT_OK_AND_ASSIGN(SchemaTypeId new_email_schema_type_id,
                             schema_store->GetSchemaTypeId("email"));
  ICING_ASSERT_OK_AND_ASSIGN(SchemaTypeId new_message_schema_type_id,
                             schema_store->GetSchemaTypeId("message"));

  // SchemaTypeIds should have changed.
  EXPECT_NE(old_email_schema_type_id, new_email_schema_type_id);
  EXPECT_NE(old_message_schema_type_id, new_message_schema_type_id);

  ICING_EXPECT_OK(document_store->OptimizedUpdateSchemaStore(
      schema_store.get(), set_schema_result));

  // Check that the FilterCache holds the new SchemaTypeIds
  ICING_ASSERT_OK_AND_ASSIGN(
      email_data, document_store->GetDocumentFilterData(email_document_id));
  EXPECT_THAT(email_data.schema_type_id(), Eq(new_email_schema_type_id));

  ICING_ASSERT_OK_AND_ASSIGN(
      message_data, document_store->GetDocumentFilterData(message_document_id));
  EXPECT_THAT(message_data.schema_type_id(), Eq(new_message_schema_type_id));
}

TEST_F(DocumentStoreTest, OptimizedUpdateSchemaStoreDeletesInvalidDocuments) {
  const std::string schema_store_dir = test_dir_ + "_custom";
  filesystem_.DeleteDirectoryRecursively(schema_store_dir.c_str());
  filesystem_.CreateDirectoryRecursively(schema_store_dir.c_str());

  // Set a schema
  SchemaProto schema =
      SchemaBuilder()
          .AddType(SchemaTypeConfigBuilder().SetType("email").AddProperty(
              PropertyConfigBuilder()
                  .SetName("subject")
                  .SetDataTypeString(MATCH_EXACT, TOKENIZER_PLAIN)
                  .SetCardinality(CARDINALITY_OPTIONAL)))
          .Build();

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<SchemaStore> schema_store,
      SchemaStore::Create(&filesystem_, schema_store_dir, &fake_clock_));
  ICING_EXPECT_OK(schema_store->SetSchema(schema));

  // Add two documents, with and without a subject
  DocumentProto email_without_subject = DocumentBuilder()
                                            .SetNamespace("namespace")
                                            .SetUri("email_uri_without_subject")
                                            .SetSchema("email")
                                            .SetCreationTimestampMs(0)
                                            .Build();

  DocumentProto email_with_subject = DocumentBuilder()
                                         .SetNamespace("namespace")
                                         .SetUri("email_uri_with_subject")
                                         .SetSchema("email")
                                         .AddStringProperty("subject", "foo")
                                         .SetCreationTimestampMs(0)
                                         .Build();

  // Insert documents and check they're ok
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store.get()));
  std::unique_ptr<DocumentStore> document_store =
      std::move(create_result.document_store);

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId email_without_subject_document_id,
                             document_store->Put(email_without_subject));
  EXPECT_THAT(document_store->Get(email_without_subject_document_id),
              IsOkAndHolds(EqualsProto(email_without_subject)));

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId email_with_subject_document_id,
                             document_store->Put(email_with_subject));
  EXPECT_THAT(document_store->Get(email_with_subject_document_id),
              IsOkAndHolds(EqualsProto(email_with_subject)));

  // Changing an OPTIONAL field to REQUIRED is backwards incompatible, and will
  // invalidate all documents that don't have this property set
  schema.mutable_types(0)->mutable_properties(0)->set_cardinality(
      PropertyConfigProto::Cardinality::REQUIRED);

  ICING_ASSERT_OK_AND_ASSIGN(
      SchemaStore::SetSchemaResult set_schema_result,
      schema_store->SetSchema(schema,
                              /*ignore_errors_and_delete_documents=*/true));

  ICING_EXPECT_OK(document_store->OptimizedUpdateSchemaStore(
      schema_store.get(), set_schema_result));

  // The email without a subject should be marked as deleted
  EXPECT_THAT(document_store->Get(email_without_subject_document_id),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));

  // The email with a subject should be unaffected
  EXPECT_THAT(document_store->Get(email_with_subject_document_id),
              IsOkAndHolds(EqualsProto(email_with_subject)));
}

TEST_F(DocumentStoreTest,
       OptimizedUpdateSchemaStoreDeletesDocumentsByDeletedSchemaType) {
  const std::string schema_store_dir = test_dir_ + "_custom";
  filesystem_.DeleteDirectoryRecursively(schema_store_dir.c_str());
  filesystem_.CreateDirectoryRecursively(schema_store_dir.c_str());

  // Set a schema
  SchemaProto schema =
      SchemaBuilder()
          .AddType(SchemaTypeConfigBuilder().SetType("email"))
          .AddType(SchemaTypeConfigBuilder().SetType("message"))
          .Build();

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<SchemaStore> schema_store,
      SchemaStore::Create(&filesystem_, schema_store_dir, &fake_clock_));
  ICING_EXPECT_OK(schema_store->SetSchema(schema));

  // Add a "email" and "message" document
  DocumentProto email_document = DocumentBuilder()
                                     .SetNamespace("namespace")
                                     .SetUri("email_uri")
                                     .SetSchema("email")
                                     .SetCreationTimestampMs(0)
                                     .Build();

  DocumentProto message_document = DocumentBuilder()
                                       .SetNamespace("namespace")
                                       .SetUri("message_uri")
                                       .SetSchema("message")
                                       .SetCreationTimestampMs(0)
                                       .Build();

  // Insert documents and check they're ok
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store.get()));
  std::unique_ptr<DocumentStore> document_store =
      std::move(create_result.document_store);

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId email_document_id,
                             document_store->Put(email_document));
  EXPECT_THAT(document_store->Get(email_document_id),
              IsOkAndHolds(EqualsProto(email_document)));

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId message_document_id,
                             document_store->Put(message_document));
  EXPECT_THAT(document_store->Get(message_document_id),
              IsOkAndHolds(EqualsProto(message_document)));

  SchemaProto new_schema =
      SchemaBuilder()
          .AddType(SchemaTypeConfigBuilder().SetType("message"))
          .Build();

  ICING_ASSERT_OK_AND_ASSIGN(
      SchemaStore::SetSchemaResult set_schema_result,
      schema_store->SetSchema(new_schema,
                              /*ignore_errors_and_delete_documents=*/true));

  ICING_EXPECT_OK(document_store->OptimizedUpdateSchemaStore(
      schema_store.get(), set_schema_result));

  // The "email" type is unknown now, so the "email" document should be deleted
  EXPECT_THAT(document_store->Get(email_document_id),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));

  // The "message" document should be unaffected
  EXPECT_THAT(document_store->Get(message_document_id),
              IsOkAndHolds(EqualsProto(message_document)));
}

TEST_F(DocumentStoreTest, GetOptimizeInfo) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> document_store =
      std::move(create_result.document_store);

  // Nothing should be optimizable yet
  ICING_ASSERT_OK_AND_ASSIGN(DocumentStore::OptimizeInfo optimize_info,
                             document_store->GetOptimizeInfo());
  EXPECT_THAT(optimize_info.total_docs, Eq(0));
  EXPECT_THAT(optimize_info.optimizable_docs, Eq(0));
  EXPECT_THAT(optimize_info.estimated_optimizable_bytes, Eq(0));

  ICING_EXPECT_OK(document_store->Put(DocumentProto(test_document1_)));

  // Adding a document, still nothing is optimizable
  ICING_ASSERT_OK_AND_ASSIGN(optimize_info, document_store->GetOptimizeInfo());
  EXPECT_THAT(optimize_info.total_docs, Eq(1));
  EXPECT_THAT(optimize_info.optimizable_docs, Eq(0));
  EXPECT_THAT(optimize_info.estimated_optimizable_bytes, Eq(0));

  // Delete a document. Now something is optimizable
  ICING_EXPECT_OK(document_store->Delete(test_document1_.namespace_(),
                                         test_document1_.uri()));
  ICING_ASSERT_OK_AND_ASSIGN(optimize_info, document_store->GetOptimizeInfo());
  EXPECT_THAT(optimize_info.total_docs, Eq(1));
  EXPECT_THAT(optimize_info.optimizable_docs, Eq(1));
  EXPECT_THAT(optimize_info.estimated_optimizable_bytes, Gt(0));

  // Optimize it into a different directory, should bring us back to nothing
  // since all documents were optimized away.
  std::string optimized_dir = document_store_dir_ + "_optimize";
  EXPECT_TRUE(filesystem_.DeleteDirectoryRecursively(optimized_dir.c_str()));
  EXPECT_TRUE(filesystem_.CreateDirectoryRecursively(optimized_dir.c_str()));
  ICING_ASSERT_OK(
      document_store->OptimizeInto(optimized_dir, lang_segmenter_.get()));
  document_store.reset();
  ICING_ASSERT_OK_AND_ASSIGN(
      create_result, DocumentStore::Create(&filesystem_, optimized_dir,
                                           &fake_clock_, schema_store_.get()));
  std::unique_ptr<DocumentStore> optimized_document_store =
      std::move(create_result.document_store);

  ICING_ASSERT_OK_AND_ASSIGN(optimize_info,
                             optimized_document_store->GetOptimizeInfo());
  EXPECT_THAT(optimize_info.total_docs, Eq(0));
  EXPECT_THAT(optimize_info.optimizable_docs, Eq(0));
  EXPECT_THAT(optimize_info.estimated_optimizable_bytes, Eq(0));
}

TEST_F(DocumentStoreTest, GetAllNamespaces) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> document_store =
      std::move(create_result.document_store);

  // Empty namespaces to start with
  EXPECT_THAT(document_store->GetAllNamespaces(), IsEmpty());

  DocumentProto namespace1 = DocumentBuilder()
                                 .SetKey("namespace1", "uri")
                                 .SetSchema("email")
                                 .SetCreationTimestampMs(0)
                                 .SetTtlMs(500)
                                 .Build();
  DocumentProto namespace2_uri1 = DocumentBuilder()
                                      .SetKey("namespace2", "uri1")
                                      .SetSchema("email")
                                      .SetCreationTimestampMs(0)
                                      .SetTtlMs(500)
                                      .Build();
  DocumentProto namespace2_uri2 = DocumentBuilder()
                                      .SetKey("namespace2", "uri2")
                                      .SetSchema("email")
                                      .SetCreationTimestampMs(0)
                                      .SetTtlMs(500)
                                      .Build();
  DocumentProto namespace3 = DocumentBuilder()
                                 .SetKey("namespace3", "uri")
                                 .SetSchema("email")
                                 .SetCreationTimestampMs(0)
                                 .SetTtlMs(100)
                                 .Build();

  ICING_ASSERT_OK(document_store->Put(namespace1));
  ICING_ASSERT_OK(document_store->Put(namespace2_uri1));
  ICING_ASSERT_OK(document_store->Put(namespace2_uri2));
  ICING_ASSERT_OK(document_store->Put(namespace3));

  auto get_result = document_store->Get("namespace1", "uri");
  get_result = document_store->Get("namespace2", "uri1");
  get_result = document_store->Get("namespace2", "uri2");
  get_result = document_store->Get("namespace3", "uri");

  // Have all the namespaces now
  EXPECT_THAT(document_store->GetAllNamespaces(),
              UnorderedElementsAre("namespace1", "namespace2", "namespace3"));

  // After deleting namespace2_uri1, there's still namespace2_uri2, so
  // "namespace2" still shows up in results
  ICING_EXPECT_OK(document_store->Delete("namespace2", "uri1"));

  EXPECT_THAT(document_store->GetAllNamespaces(),
              UnorderedElementsAre("namespace1", "namespace2", "namespace3"));

  // After deleting namespace2_uri2, there's no more documents in "namespace2"
  ICING_EXPECT_OK(document_store->Delete("namespace2", "uri2"));

  EXPECT_THAT(document_store->GetAllNamespaces(),
              UnorderedElementsAre("namespace1", "namespace3"));

  // Some arbitrary time past namespace3's creation time (0) and ttl (100)
  fake_clock_.SetSystemTimeMilliseconds(110);

  EXPECT_THAT(document_store->GetAllNamespaces(),
              UnorderedElementsAre("namespace1"));
}

TEST_F(DocumentStoreTest, ReportUsageWithDifferentTimestampsAndGetUsageScores) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> document_store =
      std::move(create_result.document_store);

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id,
                             document_store->Put(test_document1_));

  // Report usage with type 1 and time 1.
  UsageReport usage_report_type1_time1 = CreateUsageReport(
      /*name_space=*/"icing", /*uri=*/"email/1", /*timestamp_ms=*/1000,
      UsageReport::USAGE_TYPE1);
  ICING_ASSERT_OK(document_store->ReportUsage(usage_report_type1_time1));

  UsageStore::UsageScores expected_scores;
  expected_scores.usage_type1_last_used_timestamp_s = 1;
  ++expected_scores.usage_type1_count;
  ASSERT_THAT(document_store->GetUsageScores(document_id),
              IsOkAndHolds(expected_scores));

  // Report usage with type 1 and time 5, time should be updated.
  UsageReport usage_report_type1_time5 = CreateUsageReport(
      /*name_space=*/"icing", /*uri=*/"email/1", /*timestamp_ms=*/5000,
      UsageReport::USAGE_TYPE1);
  ICING_ASSERT_OK(document_store->ReportUsage(usage_report_type1_time5));

  expected_scores.usage_type1_last_used_timestamp_s = 5;
  ++expected_scores.usage_type1_count;
  ASSERT_THAT(document_store->GetUsageScores(document_id),
              IsOkAndHolds(expected_scores));

  // Report usage with type 2 and time 1.
  UsageReport usage_report_type2_time1 = CreateUsageReport(
      /*name_space=*/"icing", /*uri=*/"email/1", /*timestamp_ms=*/1000,
      UsageReport::USAGE_TYPE2);
  ICING_ASSERT_OK(document_store->ReportUsage(usage_report_type2_time1));

  expected_scores.usage_type2_last_used_timestamp_s = 1;
  ++expected_scores.usage_type2_count;
  ASSERT_THAT(document_store->GetUsageScores(document_id),
              IsOkAndHolds(expected_scores));

  // Report usage with type 2 and time 5.
  UsageReport usage_report_type2_time5 = CreateUsageReport(
      /*name_space=*/"icing", /*uri=*/"email/1", /*timestamp_ms=*/5000,
      UsageReport::USAGE_TYPE2);
  ICING_ASSERT_OK(document_store->ReportUsage(usage_report_type2_time5));

  expected_scores.usage_type2_last_used_timestamp_s = 5;
  ++expected_scores.usage_type2_count;
  ASSERT_THAT(document_store->GetUsageScores(document_id),
              IsOkAndHolds(expected_scores));

  // Report usage with type 3 and time 1.
  UsageReport usage_report_type3_time1 = CreateUsageReport(
      /*name_space=*/"icing", /*uri=*/"email/1", /*timestamp_ms=*/1000,
      UsageReport::USAGE_TYPE3);
  ICING_ASSERT_OK(document_store->ReportUsage(usage_report_type3_time1));

  expected_scores.usage_type3_last_used_timestamp_s = 1;
  ++expected_scores.usage_type3_count;
  ASSERT_THAT(document_store->GetUsageScores(document_id),
              IsOkAndHolds(expected_scores));

  // Report usage with type 3 and time 5.
  UsageReport usage_report_type3_time5 = CreateUsageReport(
      /*name_space=*/"icing", /*uri=*/"email/1", /*timestamp_ms=*/5000,
      UsageReport::USAGE_TYPE3);
  ICING_ASSERT_OK(document_store->ReportUsage(usage_report_type3_time5));

  expected_scores.usage_type3_last_used_timestamp_s = 5;
  ++expected_scores.usage_type3_count;
  ASSERT_THAT(document_store->GetUsageScores(document_id),
              IsOkAndHolds(expected_scores));
}

TEST_F(DocumentStoreTest, ReportUsageWithDifferentTypesAndGetUsageScores) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> document_store =
      std::move(create_result.document_store);

  ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id,
                             document_store->Put(test_document1_));

  // Report usage with type 1.
  UsageReport usage_report_type1 = CreateUsageReport(
      /*name_space=*/"icing", /*uri=*/"email/1", /*timestamp_ms=*/0,
      UsageReport::USAGE_TYPE1);
  ICING_ASSERT_OK(document_store->ReportUsage(usage_report_type1));

  UsageStore::UsageScores expected_scores;
  ++expected_scores.usage_type1_count;
  ASSERT_THAT(document_store->GetUsageScores(document_id),
              IsOkAndHolds(expected_scores));

  // Report usage with type 2.
  UsageReport usage_report_type2 = CreateUsageReport(
      /*name_space=*/"icing", /*uri=*/"email/1", /*timestamp_ms=*/0,
      UsageReport::USAGE_TYPE2);
  ICING_ASSERT_OK(document_store->ReportUsage(usage_report_type2));

  ++expected_scores.usage_type2_count;
  ASSERT_THAT(document_store->GetUsageScores(document_id),
              IsOkAndHolds(expected_scores));

  // Report usage with type 3.
  UsageReport usage_report_type3 = CreateUsageReport(
      /*name_space=*/"icing", /*uri=*/"email/1", /*timestamp_ms=*/0,
      UsageReport::USAGE_TYPE3);
  ICING_ASSERT_OK(document_store->ReportUsage(usage_report_type3));

  ++expected_scores.usage_type3_count;
  ASSERT_THAT(document_store->GetUsageScores(document_id),
              IsOkAndHolds(expected_scores));
}

TEST_F(DocumentStoreTest, UsageScoresShouldNotBeClearedOnChecksumMismatch) {
  UsageStore::UsageScores expected_scores;
  DocumentId document_id;
  {
    ICING_ASSERT_OK_AND_ASSIGN(
        DocumentStore::CreateResult create_result,
        DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                              schema_store_.get()));
    std::unique_ptr<DocumentStore> document_store =
        std::move(create_result.document_store);

    ICING_ASSERT_OK_AND_ASSIGN(document_id,
                               document_store->Put(test_document1_));

    // Report usage with type 1.
    UsageReport usage_report_type1 = CreateUsageReport(
        /*name_space=*/"icing", /*uri=*/"email/1", /*timestamp_ms=*/0,
        UsageReport::USAGE_TYPE1);
    ICING_ASSERT_OK(document_store->ReportUsage(usage_report_type1));

    ++expected_scores.usage_type1_count;
    ASSERT_THAT(document_store->GetUsageScores(document_id),
                IsOkAndHolds(expected_scores));
  }

  CorruptDocStoreHeaderChecksumFile();
  // Successfully recover from a corrupt derived file issue.
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> document_store =
      std::move(create_result.document_store);

  // Usage scores should be the same.
  ASSERT_THAT(document_store->GetUsageScores(document_id),
              IsOkAndHolds(expected_scores));
}

TEST_F(DocumentStoreTest, UsageScoresShouldBeAvailableAfterDataLoss) {
  UsageStore::UsageScores expected_scores;
  DocumentId document_id;
  {
    ICING_ASSERT_OK_AND_ASSIGN(
        DocumentStore::CreateResult create_result,
        DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                              schema_store_.get()));
    std::unique_ptr<DocumentStore> document_store =
        std::move(create_result.document_store);

    ICING_ASSERT_OK_AND_ASSIGN(
        document_id, document_store->Put(DocumentProto(test_document1_)));

    // Report usage with type 1.
    UsageReport usage_report_type1 = CreateUsageReport(
        /*name_space=*/"icing", /*uri=*/"email/1", /*timestamp_ms=*/0,
        UsageReport::USAGE_TYPE1);
    ICING_ASSERT_OK(document_store->ReportUsage(usage_report_type1));

    ++expected_scores.usage_type1_count;
    ASSERT_THAT(document_store->GetUsageScores(document_id),
                IsOkAndHolds(expected_scores));
  }

  // "Corrupt" the content written in the log by adding non-checksummed data to
  // it. This will mess up the checksum of the proto log, forcing it to rewind
  // to the last saved point.
  DocumentProto document = DocumentBuilder().SetKey("namespace", "uri").Build();
  const std::string serialized_document = document.SerializeAsString();

  const std::string document_log_file = absl_ports::StrCat(
      document_store_dir_, "/", DocumentLogCreator::GetDocumentLogFilename());
  int64_t file_size = filesystem_.GetFileSize(document_log_file.c_str());
  filesystem_.PWrite(document_log_file.c_str(), file_size,
                     serialized_document.data(), serialized_document.size());

  // Successfully recover from a data loss issue.
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> document_store =
      std::move(create_result.document_store);

  // Usage scores should still be available.
  ASSERT_THAT(document_store->GetUsageScores(document_id),
              IsOkAndHolds(expected_scores));
}

TEST_F(DocumentStoreTest, UsageScoresShouldBeCopiedOverToUpdatedDocument) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> document_store =
      std::move(create_result.document_store);

  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentId document_id,
      document_store->Put(DocumentProto(test_document1_)));

  // Report usage with type 1.
  UsageReport usage_report_type1 = CreateUsageReport(
      /*name_space=*/"icing", /*uri=*/"email/1", /*timestamp_ms=*/0,
      UsageReport::USAGE_TYPE1);
  ICING_ASSERT_OK(document_store->ReportUsage(usage_report_type1));

  UsageStore::UsageScores expected_scores;
  ++expected_scores.usage_type1_count;
  ASSERT_THAT(document_store->GetUsageScores(document_id),
              IsOkAndHolds(expected_scores));

  // Update the document.
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentId updated_document_id,
      document_store->Put(DocumentProto(test_document1_)));
  // We should get a different document id.
  ASSERT_THAT(updated_document_id, Not(Eq(document_id)));

  // Usage scores should be the same.
  EXPECT_THAT(document_store->GetUsageScores(updated_document_id),
              IsOkAndHolds(expected_scores));
}

TEST_F(DocumentStoreTest, UsageScoresShouldPersistOnOptimize) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> document_store =
      std::move(create_result.document_store);

  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentId document_id1,
      document_store->Put(DocumentProto(test_document1_)));
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentId document_id2,
      document_store->Put(DocumentProto(test_document2_)));
  ICING_ASSERT_OK(document_store->Delete(document_id1));

  // Report usage of document 2.
  UsageReport usage_report = CreateUsageReport(
      /*name_space=*/"icing", /*uri=*/"email/2", /*timestamp_ms=*/0,
      UsageReport::USAGE_TYPE1);
  ICING_ASSERT_OK(document_store->ReportUsage(usage_report));

  UsageStore::UsageScores expected_scores;
  ++expected_scores.usage_type1_count;
  ASSERT_THAT(document_store->GetUsageScores(document_id2),
              IsOkAndHolds(expected_scores));

  // Run optimize
  std::string optimized_dir = document_store_dir_ + "/optimize_test";
  filesystem_.CreateDirectoryRecursively(optimized_dir.c_str());
  ICING_ASSERT_OK(
      document_store->OptimizeInto(optimized_dir, lang_segmenter_.get()));

  // Get optimized document store
  ICING_ASSERT_OK_AND_ASSIGN(
      create_result, DocumentStore::Create(&filesystem_, optimized_dir,
                                           &fake_clock_, schema_store_.get()));
  std::unique_ptr<DocumentStore> optimized_document_store =
      std::move(create_result.document_store);

  // Usage scores should be the same.
  // The original document_id2 should have become document_id2 - 1.
  ASSERT_THAT(optimized_document_store->GetUsageScores(document_id2 - 1),
              IsOkAndHolds(expected_scores));
}

TEST_F(DocumentStoreTest, DetectPartialDataLoss) {
  {
    // Can put and delete fine.
    ICING_ASSERT_OK_AND_ASSIGN(
        DocumentStore::CreateResult create_result,
        DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                              schema_store_.get()));
    std::unique_ptr<DocumentStore> doc_store =
        std::move(create_result.document_store);
    EXPECT_THAT(create_result.data_loss, Eq(DataLoss::NONE));

    ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id,
                               doc_store->Put(DocumentProto(test_document1_)));
    EXPECT_THAT(doc_store->Get(document_id),
                IsOkAndHolds(EqualsProto(test_document1_)));
  }

  // "Corrupt" the content written in the log by adding non-checksummed data to
  // it. This will mess up the checksum of the proto log, forcing it to rewind
  // to the last saved point and triggering data loss.
  DocumentProto document = DocumentBuilder().SetKey("namespace", "uri").Build();
  const std::string serialized_document = document.SerializeAsString();

  const std::string document_log_file =
      absl_ports::StrCat(document_store_dir_, "/",
                         DocumentLogCreator::GetDocumentLogFilename())
          .c_str();
  int64_t file_size = filesystem_.GetFileSize(document_log_file.c_str());
  filesystem_.PWrite(document_log_file.c_str(), file_size,
                     serialized_document.data(), serialized_document.size());

  // Successfully recover from a data loss issue.
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);
  ASSERT_THAT(create_result.data_loss, Eq(DataLoss::PARTIAL));
}

TEST_F(DocumentStoreTest, DetectCompleteDataLoss) {
  int64_t corruptible_offset;
  const std::string document_log_file = absl_ports::StrCat(
      document_store_dir_, "/", DocumentLogCreator::GetDocumentLogFilename());
  {
    // Can put and delete fine.
    ICING_ASSERT_OK_AND_ASSIGN(
        DocumentStore::CreateResult create_result,
        DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                              schema_store_.get()));
    std::unique_ptr<DocumentStore> doc_store =
        std::move(create_result.document_store);
    EXPECT_THAT(create_result.data_loss, Eq(DataLoss::NONE));

    // There's some space at the beginning of the file (e.g. header, kmagic,
    // etc) that is necessary to initialize the FileBackedProtoLog. We can't
    // corrupt that region, so we need to figure out the offset at which
    // documents will be written to - which is the file size after
    // initialization.
    corruptible_offset = filesystem_.GetFileSize(document_log_file.c_str());

    ICING_ASSERT_OK_AND_ASSIGN(DocumentId document_id,
                               doc_store->Put(DocumentProto(test_document1_)));
    EXPECT_THAT(doc_store->Get(document_id),
                IsOkAndHolds(EqualsProto(test_document1_)));
  }

  // "Corrupt" the persisted content written in the log. We can't recover if
  // the persisted data was corrupted.
  std::string corruption = "abc";
  filesystem_.PWrite(document_log_file.c_str(),
                     /*offset=*/corruptible_offset, corruption.data(),
                     corruption.size());

  {
    // "Corrupt" the content written in the log. Make the corrupt document
    // smaller than our original one so we don't accidentally write past our
    // file.
    DocumentProto document =
        DocumentBuilder().SetKey("invalid_namespace", "invalid_uri").Build();
    std::string serialized_document = document.SerializeAsString();
    ASSERT_TRUE(filesystem_.PWrite(
        document_log_file.c_str(), corruptible_offset,
        serialized_document.data(), serialized_document.size()));

    PortableFileBackedProtoLog<DocumentWrapper>::Header header =
        ReadDocumentLogHeader(filesystem_, document_log_file);

    // Set dirty bit to true to reflect that something changed in the log.
    header.SetDirtyFlag(true);
    header.SetHeaderChecksum(header.CalculateHeaderChecksum());

    WriteDocumentLogHeader(filesystem_, document_log_file, header);
  }

  // Successfully recover from a data loss issue.
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);
  ASSERT_THAT(create_result.data_loss, Eq(DataLoss::COMPLETE));
}

// TODO(b/185845269) Re-enable this test by copying over a full valid set of
// document store files. Right now this test only includes the score_cache and
// the document store header.
//
// This causes a problem now because this cl changes behavior to not consider an
// InitializeExistingDerivedFiles failure to be a recovery if there is nothing
// to recover because the doocument store is empty.
#define DISABLE_BACKWARDS_COMPAT_TEST
#ifndef DISABLE_BACKWARDS_COMPAT_TEST
TEST_F(DocumentStoreTest, LoadScoreCacheAndInitializeSuccessfully) {
  // The directory testdata/score_cache_without_length_in_tokens/document_store
  // contains only the scoring_cache and the document_store_header (holding the
  // crc for the scoring_cache). If the current code is compatible with the
  // format of the v0 scoring_cache, then an empty document store should be
  // initialized, but the non-empty scoring_cache should be retained. The
  // current document-asscoiated-score-data has a new field with respect to the
  // ones stored in testdata/score_cache_Without_length_in_tokens, hence the
  // document store's initialization requires regenerating its derived files.

  // Create dst directory
  ASSERT_THAT(filesystem_.CreateDirectory(document_store_dir_.c_str()), true);

  // Get src files
  std::string document_store_without_length_in_tokens;
  if (IsAndroidPlatform() || IsIosPlatform()) {
    document_store_without_length_in_tokens = GetTestFilePath(
        "icing/testdata/score_cache_without_length_in_tokens/"
        "document_store_android_ios_compatible");
  } else {
    document_store_without_length_in_tokens = GetTestFilePath(
        "icing/testdata/score_cache_without_length_in_tokens/"
        "document_store");
  }
  std::vector<std::string> document_store_files;
  Filesystem filesystem;
  filesystem.ListDirectory(document_store_without_length_in_tokens.c_str(),
                           &document_store_files);

  ICING_LOG(INFO) << "Copying files " << document_store_without_length_in_tokens
                  << ' ' << document_store_files.size();
  for (size_t i = 0; i != document_store_files.size(); i++) {
    std::string src = absl_ports::StrCat(
        document_store_without_length_in_tokens, "/", document_store_files[i]);
    std::string dst =
        absl_ports::StrCat(document_store_dir_, "/", document_store_files[i]);
    ASSERT_THAT(filesystem_.CopyFile(src.c_str(), dst.c_str()), true);
  }

  InitializeStatsProto initialize_stats;
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get(),
                            /*force_recovery_and_revalidate_documents=*/false,
                            &initialize_stats));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);
  // The store_cache trigger regeneration because its element size is
  // inconsistent: expected 20 (current new size), actual 12 (as per the v0
  // score_cache).
  EXPECT_TRUE(initialize_stats.has_document_store_recovery_cause());
}
#endif  // DISABLE_BACKWARDS_COMPAT_TEST

TEST_F(DocumentStoreTest, DocumentStoreStorageInfo) {
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                            schema_store_.get()));
  std::unique_ptr<DocumentStore> doc_store =
      std::move(create_result.document_store);

  // Add three documents.
  DocumentProto document1 = test_document1_;
  document1.set_namespace_("namespace.1");
  document1.set_uri("uri1");
  ICING_ASSERT_OK(doc_store->Put(document1));

  DocumentProto document2 = test_document1_;
  document2.set_namespace_("namespace.1");
  document2.set_uri("uri2");
  document2.set_creation_timestamp_ms(fake_clock_.GetSystemTimeMilliseconds());
  document2.set_ttl_ms(100);
  ICING_ASSERT_OK(doc_store->Put(document2));

  DocumentProto document3 = test_document1_;
  document3.set_namespace_("namespace.1");
  document3.set_uri("uri3");
  ICING_ASSERT_OK(doc_store->Put(document3));

  DocumentProto document4 = test_document1_;
  document4.set_namespace_("namespace.2");
  document4.set_uri("uri1");
  ICING_ASSERT_OK(doc_store->Put(document4));

  // Report usage with type 1 on document1
  UsageReport usage_report_type1 = CreateUsageReport(
      /*name_space=*/"namespace.1", /*uri=*/"uri1", /*timestamp_ms=*/1000,
      UsageReport::USAGE_TYPE1);
  ICING_ASSERT_OK(doc_store->ReportUsage(usage_report_type1));

  // Report usage with type 2 on document2
  UsageReport usage_report_type2 = CreateUsageReport(
      /*name_space=*/"namespace.1", /*uri=*/"uri2", /*timestamp_ms=*/1000,
      UsageReport::USAGE_TYPE2);
  ICING_ASSERT_OK(doc_store->ReportUsage(usage_report_type2));

  // Report usage with type 3 on document3
  UsageReport usage_report_type3 = CreateUsageReport(
      /*name_space=*/"namespace.1", /*uri=*/"uri3", /*timestamp_ms=*/1000,
      UsageReport::USAGE_TYPE3);
  ICING_ASSERT_OK(doc_store->ReportUsage(usage_report_type3));

  // Report usage with type 1 on document4
  usage_report_type1 = CreateUsageReport(
      /*name_space=*/"namespace.2", /*uri=*/"uri1", /*timestamp_ms=*/1000,
      UsageReport::USAGE_TYPE1);
  ICING_ASSERT_OK(doc_store->ReportUsage(usage_report_type1));

  // Delete the first doc.
  ICING_ASSERT_OK(doc_store->Delete(document1.namespace_(), document1.uri()));

  // Expire the second doc.
  fake_clock_.SetSystemTimeMilliseconds(document2.creation_timestamp_ms() +
                                        document2.ttl_ms() + 1);

  // Check high level info
  DocumentStorageInfoProto storage_info = doc_store->GetStorageInfo();
  EXPECT_THAT(storage_info.num_alive_documents(), Eq(2));
  EXPECT_THAT(storage_info.num_deleted_documents(), Eq(1));
  EXPECT_THAT(storage_info.num_expired_documents(), Eq(1));
  EXPECT_THAT(storage_info.document_store_size(), Ge(0));
  EXPECT_THAT(storage_info.document_log_size(), Ge(0));
  EXPECT_THAT(storage_info.key_mapper_size(), Ge(0));
  EXPECT_THAT(storage_info.document_id_mapper_size(), Ge(0));
  EXPECT_THAT(storage_info.score_cache_size(), Ge(0));
  EXPECT_THAT(storage_info.filter_cache_size(), Ge(0));
  EXPECT_THAT(storage_info.corpus_mapper_size(), Ge(0));
  EXPECT_THAT(storage_info.corpus_score_cache_size(), Ge(0));
  EXPECT_THAT(storage_info.namespace_id_mapper_size(), Ge(0));
  EXPECT_THAT(storage_info.num_namespaces(), Eq(2));

  // Check per-namespace info
  EXPECT_THAT(storage_info.namespace_storage_info_size(), Eq(2));

  NamespaceStorageInfoProto namespace_storage_info =
      GetNamespaceStorageInfo(storage_info, "namespace.1");
  EXPECT_THAT(namespace_storage_info.num_alive_documents(), Eq(1));
  EXPECT_THAT(namespace_storage_info.num_expired_documents(), Eq(1));
  EXPECT_THAT(namespace_storage_info.num_alive_documents_usage_type1(), Eq(0));
  EXPECT_THAT(namespace_storage_info.num_alive_documents_usage_type2(), Eq(0));
  EXPECT_THAT(namespace_storage_info.num_alive_documents_usage_type3(), Eq(1));
  EXPECT_THAT(namespace_storage_info.num_expired_documents_usage_type1(),
              Eq(0));
  EXPECT_THAT(namespace_storage_info.num_expired_documents_usage_type2(),
              Eq(1));
  EXPECT_THAT(namespace_storage_info.num_expired_documents_usage_type3(),
              Eq(0));

  namespace_storage_info = GetNamespaceStorageInfo(storage_info, "namespace.2");
  EXPECT_THAT(namespace_storage_info.num_alive_documents(), Eq(1));
  EXPECT_THAT(namespace_storage_info.num_expired_documents(), Eq(0));
  EXPECT_THAT(namespace_storage_info.num_alive_documents_usage_type1(), Eq(1));
  EXPECT_THAT(namespace_storage_info.num_alive_documents_usage_type2(), Eq(0));
  EXPECT_THAT(namespace_storage_info.num_alive_documents_usage_type3(), Eq(0));
  EXPECT_THAT(namespace_storage_info.num_expired_documents_usage_type1(),
              Eq(0));
  EXPECT_THAT(namespace_storage_info.num_expired_documents_usage_type2(),
              Eq(0));
  EXPECT_THAT(namespace_storage_info.num_expired_documents_usage_type3(),
              Eq(0));
}

TEST_F(DocumentStoreTest, InitializeForceRecoveryUpdatesTypeIds) {
  // Start fresh and set the schema with one type.
  filesystem_.DeleteDirectoryRecursively(test_dir_.c_str());
  filesystem_.CreateDirectoryRecursively(test_dir_.c_str());
  filesystem_.CreateDirectoryRecursively(document_store_dir_.c_str());
  filesystem_.CreateDirectoryRecursively(schema_store_dir_.c_str());

  SchemaTypeConfigProto email_type_config =
      SchemaTypeConfigBuilder()
          .SetType("email")
          .AddProperty(PropertyConfigBuilder()
                           .SetName("subject")
                           .SetDataTypeString(MATCH_EXACT, TOKENIZER_PLAIN)
                           .SetCardinality(CARDINALITY_OPTIONAL))
          .AddProperty(PropertyConfigBuilder()
                           .SetName("body")
                           .SetDataTypeString(MATCH_EXACT, TOKENIZER_PLAIN)
                           .SetCardinality(CARDINALITY_OPTIONAL))
          .Build();
  SchemaProto schema = SchemaBuilder().AddType(email_type_config).Build();
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<SchemaStore> schema_store,
      SchemaStore::Create(&filesystem_, schema_store_dir_, &fake_clock_));
  ASSERT_THAT(schema_store->SetSchema(schema), IsOk());
  // The typeid for "email" should be 0.
  ASSERT_THAT(schema_store->GetSchemaTypeId("email"), IsOkAndHolds(0));

  DocumentId docid = kInvalidDocumentId;
  {
    // Create the document store the first time and add an email document.
    ICING_ASSERT_OK_AND_ASSIGN(
        DocumentStore::CreateResult create_result,
        DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                              schema_store.get()));
    std::unique_ptr<DocumentStore> doc_store =
        std::move(create_result.document_store);

    DocumentProto doc =
        DocumentBuilder()
            .SetKey("icing", "email/1")
            .SetSchema("email")
            .AddStringProperty("subject", "subject foo")
            .AddStringProperty("body", "body bar")
            .SetScore(document1_score_)
            .SetCreationTimestampMs(
                document1_creation_timestamp_)  // A random timestamp
            .SetTtlMs(document1_ttl_)
            .Build();
    ICING_ASSERT_OK_AND_ASSIGN(docid, doc_store->Put(doc));
    ICING_ASSERT_OK_AND_ASSIGN(DocumentFilterData filter_data,
                               doc_store->GetDocumentFilterData(docid));

    ASSERT_THAT(filter_data.schema_type_id(), Eq(0));
  }

  // Add another type to the schema before the email type.
  schema =
      SchemaBuilder()
          .AddType(SchemaTypeConfigBuilder()
                       .SetType("alarm")
                       .AddProperty(
                           PropertyConfigBuilder()
                               .SetName("name")
                               .SetDataTypeString(MATCH_EXACT, TOKENIZER_PLAIN)
                               .SetCardinality(CARDINALITY_OPTIONAL))
                       .AddProperty(PropertyConfigBuilder()
                                        .SetName("time")
                                        .SetDataType(TYPE_INT)
                                        .SetCardinality(CARDINALITY_OPTIONAL)))
          .AddType(email_type_config)
          .Build();
  ASSERT_THAT(schema_store->SetSchema(schema), IsOk());
  // Adding a new type should cause ids to be reassigned. Ids are assigned in
  // order of appearance so 'alarm' should be 0 and 'email' should be 1.
  ASSERT_THAT(schema_store->GetSchemaTypeId("alarm"), IsOkAndHolds(0));
  ASSERT_THAT(schema_store->GetSchemaTypeId("email"), IsOkAndHolds(1));

  {
    // Create the document store the second time and force recovery
    ICING_ASSERT_OK_AND_ASSIGN(
        DocumentStore::CreateResult create_result,
        DocumentStore::Create(
            &filesystem_, document_store_dir_, &fake_clock_, schema_store.get(),
            /*force_recovery_and_revalidate_documents=*/true));
    std::unique_ptr<DocumentStore> doc_store =
        std::move(create_result.document_store);

    // Ensure that the type id of the email document has been correctly updated.
    ICING_ASSERT_OK_AND_ASSIGN(DocumentFilterData filter_data,
                               doc_store->GetDocumentFilterData(docid));
    ASSERT_THAT(filter_data.schema_type_id(), Eq(1));
  }
}

TEST_F(DocumentStoreTest, InitializeDontForceRecoveryDoesntUpdateTypeIds) {
  // Start fresh and set the schema with one type.
  filesystem_.DeleteDirectoryRecursively(test_dir_.c_str());
  filesystem_.CreateDirectoryRecursively(test_dir_.c_str());
  filesystem_.CreateDirectoryRecursively(document_store_dir_.c_str());
  filesystem_.CreateDirectoryRecursively(schema_store_dir_.c_str());

  SchemaTypeConfigProto email_type_config =
      SchemaTypeConfigBuilder()
          .SetType("email")
          .AddProperty(PropertyConfigBuilder()
                           .SetName("subject")
                           .SetDataTypeString(MATCH_EXACT, TOKENIZER_PLAIN)
                           .SetCardinality(CARDINALITY_OPTIONAL))
          .AddProperty(PropertyConfigBuilder()
                           .SetName("body")
                           .SetDataTypeString(MATCH_EXACT, TOKENIZER_PLAIN)
                           .SetCardinality(CARDINALITY_OPTIONAL))
          .Build();
  SchemaProto schema = SchemaBuilder().AddType(email_type_config).Build();
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<SchemaStore> schema_store,
      SchemaStore::Create(&filesystem_, schema_store_dir_, &fake_clock_));
  ASSERT_THAT(schema_store->SetSchema(schema), IsOk());
  // The typeid for "email" should be 0.
  ASSERT_THAT(schema_store->GetSchemaTypeId("email"), IsOkAndHolds(0));

  DocumentId docid = kInvalidDocumentId;
  {
    // Create the document store the first time and add an email document.
    ICING_ASSERT_OK_AND_ASSIGN(
        DocumentStore::CreateResult create_result,
        DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                              schema_store.get()));
    std::unique_ptr<DocumentStore> doc_store =
        std::move(create_result.document_store);

    DocumentProto doc =
        DocumentBuilder()
            .SetKey("icing", "email/1")
            .SetSchema("email")
            .AddStringProperty("subject", "subject foo")
            .AddStringProperty("body", "body bar")
            .SetScore(document1_score_)
            .SetCreationTimestampMs(
                document1_creation_timestamp_)  // A random timestamp
            .SetTtlMs(document1_ttl_)
            .Build();
    ICING_ASSERT_OK_AND_ASSIGN(docid, doc_store->Put(doc));
    ICING_ASSERT_OK_AND_ASSIGN(DocumentFilterData filter_data,
                               doc_store->GetDocumentFilterData(docid));

    ASSERT_THAT(filter_data.schema_type_id(), Eq(0));
  }

  // Add another type to the schema.
  schema =
      SchemaBuilder()
          .AddType(SchemaTypeConfigBuilder()
                       .SetType("alarm")
                       .AddProperty(
                           PropertyConfigBuilder()
                               .SetName("name")
                               .SetDataTypeString(MATCH_EXACT, TOKENIZER_PLAIN)
                               .SetCardinality(CARDINALITY_OPTIONAL))
                       .AddProperty(PropertyConfigBuilder()
                                        .SetName("time")
                                        .SetDataType(TYPE_INT)
                                        .SetCardinality(CARDINALITY_OPTIONAL)))
          .AddType(email_type_config)
          .Build();
  ASSERT_THAT(schema_store->SetSchema(schema), IsOk());
  // Adding a new type should cause ids to be reassigned. Ids are assigned in
  // order of appearance so 'alarm' should be 0 and 'email' should be 1.
  ASSERT_THAT(schema_store->GetSchemaTypeId("alarm"), IsOkAndHolds(0));
  ASSERT_THAT(schema_store->GetSchemaTypeId("email"), IsOkAndHolds(1));

  {
    // Create the document store the second time. Don't force recovery.
    ICING_ASSERT_OK_AND_ASSIGN(
        DocumentStore::CreateResult create_result,
        DocumentStore::Create(
            &filesystem_, document_store_dir_, &fake_clock_, schema_store.get(),
            /*force_recovery_and_revalidate_documents=*/false));
    std::unique_ptr<DocumentStore> doc_store =
        std::move(create_result.document_store);

    // Check that the type id of the email document has not been updated.
    ICING_ASSERT_OK_AND_ASSIGN(DocumentFilterData filter_data,
                               doc_store->GetDocumentFilterData(docid));
    ASSERT_THAT(filter_data.schema_type_id(), Eq(0));
  }
}

TEST_F(DocumentStoreTest, InitializeForceRecoveryDeletesInvalidDocument) {
  // Start fresh and set the schema with one type.
  filesystem_.DeleteDirectoryRecursively(test_dir_.c_str());
  filesystem_.CreateDirectoryRecursively(test_dir_.c_str());
  filesystem_.CreateDirectoryRecursively(document_store_dir_.c_str());
  filesystem_.CreateDirectoryRecursively(schema_store_dir_.c_str());

  SchemaTypeConfigProto email_type_config =
      SchemaTypeConfigBuilder()
          .SetType("email")
          .AddProperty(PropertyConfigBuilder()
                           .SetName("subject")
                           .SetDataTypeString(MATCH_EXACT, TOKENIZER_PLAIN)
                           .SetCardinality(CARDINALITY_OPTIONAL))
          .AddProperty(PropertyConfigBuilder()
                           .SetName("body")
                           .SetDataTypeString(MATCH_EXACT, TOKENIZER_PLAIN)
                           .SetCardinality(CARDINALITY_OPTIONAL))
          .Build();
  SchemaProto schema = SchemaBuilder().AddType(email_type_config).Build();
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<SchemaStore> schema_store,
      SchemaStore::Create(&filesystem_, schema_store_dir_, &fake_clock_));
  ASSERT_THAT(schema_store->SetSchema(schema), IsOk());

  DocumentId docid = kInvalidDocumentId;
  DocumentProto docWithBody =
      DocumentBuilder()
          .SetKey("icing", "email/1")
          .SetSchema("email")
          .AddStringProperty("subject", "subject foo")
          .AddStringProperty("body", "body bar")
          .SetScore(document1_score_)
          .SetCreationTimestampMs(
              document1_creation_timestamp_)  // A random timestamp
          .SetTtlMs(document1_ttl_)
          .Build();
  DocumentProto docWithoutBody =
      DocumentBuilder()
          .SetKey("icing", "email/2")
          .SetSchema("email")
          .AddStringProperty("subject", "subject foo")
          .SetScore(document1_score_)
          .SetCreationTimestampMs(
              document1_creation_timestamp_)  // A random timestamp
          .SetTtlMs(document1_ttl_)
          .Build();

  {
    // Create the document store the first time and add two email documents: one
    // that has the 'body' section and one that doesn't.
    ICING_ASSERT_OK_AND_ASSIGN(
        DocumentStore::CreateResult create_result,
        DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                              schema_store.get()));
    std::unique_ptr<DocumentStore> doc_store =
        std::move(create_result.document_store);

    ICING_ASSERT_OK_AND_ASSIGN(docid, doc_store->Put(docWithBody));
    ICING_ASSERT_OK_AND_ASSIGN(docid, doc_store->Put(docWithoutBody));

    ASSERT_THAT(doc_store->Get(docWithBody.namespace_(), docWithBody.uri()),
                IsOkAndHolds(EqualsProto(docWithBody)));
    ASSERT_THAT(
        doc_store->Get(docWithoutBody.namespace_(), docWithoutBody.uri()),
        IsOkAndHolds(EqualsProto(docWithoutBody)));
  }

  // Delete the 'body' property from the 'email' type, making all pre-existing
  // documents with the 'body' property invalid.
  email_type_config =
      SchemaTypeConfigBuilder()
          .SetType("email")
          .AddProperty(PropertyConfigBuilder()
                           .SetName("subject")
                           .SetDataTypeString(MATCH_EXACT, TOKENIZER_PLAIN)
                           .SetCardinality(CARDINALITY_OPTIONAL))
          .Build();
  schema = SchemaBuilder().AddType(email_type_config).Build();
  ASSERT_THAT(schema_store->SetSchema(
                  schema, /*ignore_errors_and_delete_documents=*/true),
              IsOk());

  {
    // Create the document store the second time and force recovery
    CorruptDocStoreHeaderChecksumFile();
    ICING_ASSERT_OK_AND_ASSIGN(
        DocumentStore::CreateResult create_result,
        DocumentStore::Create(
            &filesystem_, document_store_dir_, &fake_clock_, schema_store.get(),
            /*force_recovery_and_revalidate_documents=*/true));
    std::unique_ptr<DocumentStore> doc_store =
        std::move(create_result.document_store);

    ASSERT_THAT(doc_store->Get(docWithBody.namespace_(), docWithBody.uri()),
                StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
    ASSERT_THAT(
        doc_store->Get(docWithoutBody.namespace_(), docWithoutBody.uri()),
        IsOkAndHolds(EqualsProto(docWithoutBody)));
  }
}

TEST_F(DocumentStoreTest, InitializeDontForceRecoveryKeepsInvalidDocument) {
  // Start fresh and set the schema with one type.
  filesystem_.DeleteDirectoryRecursively(test_dir_.c_str());
  filesystem_.CreateDirectoryRecursively(test_dir_.c_str());
  filesystem_.CreateDirectoryRecursively(document_store_dir_.c_str());
  filesystem_.CreateDirectoryRecursively(schema_store_dir_.c_str());

  SchemaTypeConfigProto email_type_config =
      SchemaTypeConfigBuilder()
          .SetType("email")
          .AddProperty(PropertyConfigBuilder()
                           .SetName("subject")
                           .SetDataTypeString(MATCH_EXACT, TOKENIZER_PLAIN)
                           .SetCardinality(CARDINALITY_OPTIONAL))
          .AddProperty(PropertyConfigBuilder()
                           .SetName("body")
                           .SetDataTypeString(MATCH_EXACT, TOKENIZER_PLAIN)
                           .SetCardinality(CARDINALITY_OPTIONAL))
          .Build();
  SchemaProto schema = SchemaBuilder().AddType(email_type_config).Build();
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<SchemaStore> schema_store,
      SchemaStore::Create(&filesystem_, schema_store_dir_, &fake_clock_));
  ASSERT_THAT(schema_store->SetSchema(schema), IsOk());

  DocumentId docid = kInvalidDocumentId;
  DocumentProto docWithBody =
      DocumentBuilder()
          .SetKey("icing", "email/1")
          .SetSchema("email")
          .AddStringProperty("subject", "subject foo")
          .AddStringProperty("body", "body bar")
          .SetScore(document1_score_)
          .SetCreationTimestampMs(
              document1_creation_timestamp_)  // A random timestamp
          .SetTtlMs(document1_ttl_)
          .Build();
  DocumentProto docWithoutBody =
      DocumentBuilder()
          .SetKey("icing", "email/2")
          .SetSchema("email")
          .AddStringProperty("subject", "subject foo")
          .SetScore(document1_score_)
          .SetCreationTimestampMs(
              document1_creation_timestamp_)  // A random timestamp
          .SetTtlMs(document1_ttl_)
          .Build();

  {
    // Create the document store the first time and add two email documents: one
    // that has the 'body' section and one that doesn't.
    ICING_ASSERT_OK_AND_ASSIGN(
        DocumentStore::CreateResult create_result,
        DocumentStore::Create(&filesystem_, document_store_dir_, &fake_clock_,
                              schema_store.get()));
    std::unique_ptr<DocumentStore> doc_store =
        std::move(create_result.document_store);

    ICING_ASSERT_OK_AND_ASSIGN(docid, doc_store->Put(docWithBody));
    ICING_ASSERT_OK_AND_ASSIGN(docid, doc_store->Put(docWithoutBody));

    ASSERT_THAT(doc_store->Get(docWithBody.namespace_(), docWithBody.uri()),
                IsOkAndHolds(EqualsProto(docWithBody)));
    ASSERT_THAT(
        doc_store->Get(docWithoutBody.namespace_(), docWithoutBody.uri()),
        IsOkAndHolds(EqualsProto(docWithoutBody)));
  }

  // Delete the 'body' property from the 'email' type, making all pre-existing
  // documents with the 'body' property invalid.
  email_type_config =
      SchemaTypeConfigBuilder()
          .SetType("email")
          .AddProperty(PropertyConfigBuilder()
                           .SetName("subject")
                           .SetDataTypeString(MATCH_EXACT, TOKENIZER_PLAIN)
                           .SetCardinality(CARDINALITY_OPTIONAL))
          .Build();
  schema = SchemaBuilder().AddType(email_type_config).Build();
  ASSERT_THAT(schema_store->SetSchema(
                  schema, /*ignore_errors_and_delete_documents=*/true),
              IsOk());

  {
    // Corrupt the document store header checksum so that we will perform
    // recovery, but without revalidation.
    CorruptDocStoreHeaderChecksumFile();
    ICING_ASSERT_OK_AND_ASSIGN(
        DocumentStore::CreateResult create_result,
        DocumentStore::Create(
            &filesystem_, document_store_dir_, &fake_clock_, schema_store.get(),
            /*force_recovery_and_revalidate_documents=*/false));
    std::unique_ptr<DocumentStore> doc_store =
        std::move(create_result.document_store);

    ASSERT_THAT(doc_store->Get(docWithBody.namespace_(), docWithBody.uri()),
                IsOkAndHolds(EqualsProto(docWithBody)));
    ASSERT_THAT(
        doc_store->Get(docWithoutBody.namespace_(), docWithoutBody.uri()),
        IsOkAndHolds(EqualsProto(docWithoutBody)));
  }
}

#ifndef DISABLE_BACKWARDS_COMPAT_TEST
TEST_F(DocumentStoreTest, MigrateToPortableFileBackedProtoLog) {
  // Set up schema.
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

  std::string schema_store_dir = schema_store_dir_ + "_migrate";
  filesystem_.DeleteDirectoryRecursively(schema_store_dir.c_str());
  filesystem_.CreateDirectoryRecursively(schema_store_dir.c_str());
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<SchemaStore> schema_store,
      SchemaStore::Create(&filesystem_, schema_store_dir, &fake_clock_));

  ASSERT_THAT(schema_store->SetSchema(schema), IsOk());

  // Create dst directory that we'll initialize the DocumentStore over.
  std::string document_store_dir = document_store_dir_ + "_migrate";
  ASSERT_THAT(
      filesystem_.DeleteDirectoryRecursively(document_store_dir.c_str()), true);
  ASSERT_THAT(
      filesystem_.CreateDirectoryRecursively(document_store_dir.c_str()), true);

  // Copy the testdata files into our DocumentStore directory
  std::string document_store_without_portable_log;
  if (IsAndroidX86()) {
    document_store_without_portable_log = GetTestFilePath(
        "icing/testdata/not_portable_log/"
        "icing_search_engine_android_x86/document_dir");
  } else if (IsAndroidArm()) {
    document_store_without_portable_log = GetTestFilePath(
        "icing/testdata/not_portable_log/"
        "icing_search_engine_android_arm/document_dir");
  } else if (IsIosPlatform()) {
    document_store_without_portable_log = GetTestFilePath(
        "icing/testdata/not_portable_log/"
        "icing_search_engine_ios/document_dir");
  } else {
    document_store_without_portable_log = GetTestFilePath(
        "icing/testdata/not_portable_log/"
        "icing_search_engine_linux/document_dir");
  }

  ASSERT_TRUE(filesystem_.CopyDirectory(
      document_store_without_portable_log.c_str(), document_store_dir.c_str(),
      /*recursive=*/true));

  // Initialize the DocumentStore over our copied files.
  InitializeStatsProto initialize_stats;
  ICING_ASSERT_OK_AND_ASSIGN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(&filesystem_, document_store_dir, &fake_clock_,
                            schema_store.get(),
                            /*force_recovery_and_revalidate_documents=*/false,
                            &initialize_stats));
  std::unique_ptr<DocumentStore> document_store =
      std::move(create_result.document_store);

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

  // Check that we didn't lose anything. A migration also doesn't technically
  // count as a recovery.
  EXPECT_THAT(create_result.data_loss, Eq(DataLoss::NONE));
  EXPECT_FALSE(initialize_stats.has_document_store_recovery_cause());

  // Document 1 and 3 were put normally, and document 2 was deleted in our
  // testdata files.
  //
  // Check by namespace, uri
  EXPECT_THAT(document_store->Get(document1.namespace_(), document1.uri()),
              IsOkAndHolds(EqualsProto(document1)));
  EXPECT_THAT(document_store->Get(document2.namespace_(), document2.uri()),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
  EXPECT_THAT(document_store->Get(document3.namespace_(), document3.uri()),
              IsOkAndHolds(EqualsProto(document3)));

  // Check by document_id
  EXPECT_THAT(document_store->Get(/*document_id=*/0),
              IsOkAndHolds(EqualsProto(document1)));
  EXPECT_THAT(document_store->Get(/*document_id=*/1),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
  EXPECT_THAT(document_store->Get(/*document_id=*/2),
              IsOkAndHolds(EqualsProto(document3)));
}
#endif  // DISABLE_BACKWARDS_COMPAT_TEST

}  // namespace

}  // namespace lib
}  // namespace icing
