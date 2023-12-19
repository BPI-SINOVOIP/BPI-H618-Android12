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

#include "icing/schema/section-manager.h"

#include <limits>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "icing/document-builder.h"
#include "icing/file/filesystem.h"
#include "icing/proto/schema.pb.h"
#include "icing/proto/term.pb.h"
#include "icing/schema/schema-util.h"
#include "icing/store/key-mapper.h"
#include "icing/testing/common-matchers.h"
#include "icing/testing/tmp-directory.h"

namespace icing {
namespace lib {

using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::SizeIs;

// type and property names of EmailMessage
constexpr char kTypeEmail[] = "EmailMessage";
constexpr char kPropertySubject[] = "subject";
constexpr char kPropertyText[] = "text";
constexpr char kPropertyAttachment[] = "attachment";
constexpr char kPropertyRecipients[] = "recipients";
// type and property names of Conversation
constexpr char kTypeConversation[] = "Conversation";
constexpr char kPropertyName[] = "name";
constexpr char kPropertyEmails[] = "emails";

class SectionManagerTest : public ::testing::Test {
 protected:
  SectionManagerTest() : test_dir_(GetTestTempDir() + "/icing") {
    auto email_type = CreateEmailTypeConfig();
    auto conversation_type = CreateConversationTypeConfig();
    type_config_map_.emplace(email_type.schema_type(), email_type);
    type_config_map_.emplace(conversation_type.schema_type(),
                             conversation_type);

    email_document_ =
        DocumentBuilder()
            .SetKey("icing", "email/1")
            .SetSchema(kTypeEmail)
            .AddStringProperty(kPropertySubject, "the subject")
            .AddStringProperty(kPropertyText, "the text")
            .AddBytesProperty(kPropertyAttachment, "attachment bytes")
            .AddStringProperty(kPropertyRecipients, "recipient1", "recipient2",
                               "recipient3")
            .Build();

    conversation_document_ =
        DocumentBuilder()
            .SetKey("icing", "conversation/1")
            .SetSchema(kTypeConversation)
            .AddDocumentProperty(kPropertyEmails,
                                 DocumentProto(email_document_),
                                 DocumentProto(email_document_))
            .Build();
  }

  void SetUp() override {
    // KeyMapper uses 3 internal arrays for bookkeeping. Give each one 128KiB so
    // the total KeyMapper should get 384KiB
    int key_mapper_size = 3 * 128 * 1024;
    ICING_ASSERT_OK_AND_ASSIGN(schema_type_mapper_,
                               KeyMapper<SchemaTypeId>::Create(
                                   filesystem_, test_dir_, key_mapper_size));
    ICING_ASSERT_OK(schema_type_mapper_->Put(kTypeEmail, 0));
    ICING_ASSERT_OK(schema_type_mapper_->Put(kTypeConversation, 1));
  }

  static SchemaTypeConfigProto CreateEmailTypeConfig() {
    SchemaTypeConfigProto type;
    type.set_schema_type(kTypeEmail);

    auto subject = type.add_properties();
    subject->set_property_name(kPropertySubject);
    subject->set_data_type(PropertyConfigProto::DataType::STRING);
    subject->set_cardinality(PropertyConfigProto::Cardinality::REQUIRED);
    subject->mutable_string_indexing_config()->set_term_match_type(
        TermMatchType::EXACT_ONLY);
    subject->mutable_string_indexing_config()->set_tokenizer_type(
        StringIndexingConfig::TokenizerType::PLAIN);

    auto text = type.add_properties();
    text->set_property_name(kPropertyText);
    text->set_data_type(PropertyConfigProto::DataType::STRING);
    text->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);
    text->mutable_string_indexing_config()->set_term_match_type(
        TermMatchType::UNKNOWN);

    auto attachment = type.add_properties();
    attachment->set_property_name(kPropertyAttachment);
    attachment->set_data_type(PropertyConfigProto::DataType::BYTES);
    attachment->set_cardinality(PropertyConfigProto::Cardinality::REQUIRED);

    auto recipients = type.add_properties();
    recipients->set_property_name(kPropertyRecipients);
    recipients->set_data_type(PropertyConfigProto::DataType::STRING);
    recipients->set_cardinality(PropertyConfigProto::Cardinality::REPEATED);
    recipients->mutable_string_indexing_config()->set_term_match_type(
        TermMatchType::EXACT_ONLY);
    recipients->mutable_string_indexing_config()->set_tokenizer_type(
        StringIndexingConfig::TokenizerType::PLAIN);

    return type;
  }

  static SchemaTypeConfigProto CreateConversationTypeConfig() {
    SchemaTypeConfigProto type;
    type.set_schema_type(kTypeConversation);

    auto name = type.add_properties();
    name->set_property_name(kPropertyName);
    name->set_data_type(PropertyConfigProto::DataType::STRING);
    name->set_cardinality(PropertyConfigProto::Cardinality::OPTIONAL);
    name->mutable_string_indexing_config()->set_term_match_type(
        TermMatchType::EXACT_ONLY);

    auto emails = type.add_properties();
    emails->set_property_name(kPropertyEmails);
    emails->set_data_type(PropertyConfigProto::DataType::DOCUMENT);
    emails->set_cardinality(PropertyConfigProto::Cardinality::REPEATED);
    emails->set_schema_type(kTypeEmail);
    emails->mutable_document_indexing_config()->set_index_nested_properties(
        true);

    return type;
  }

  Filesystem filesystem_;
  const std::string test_dir_;
  SchemaUtil::TypeConfigMap type_config_map_;
  std::unique_ptr<KeyMapper<SchemaTypeId>> schema_type_mapper_;

  DocumentProto email_document_;
  DocumentProto conversation_document_;
};

TEST_F(SectionManagerTest, CreationWithNullPointerShouldFail) {
  EXPECT_THAT(
      SectionManager::Create(type_config_map_, /*schema_type_mapper=*/nullptr),
      StatusIs(libtextclassifier3::StatusCode::FAILED_PRECONDITION));
}

TEST_F(SectionManagerTest, CreationWithTooManyPropertiesShouldFail) {
  SchemaTypeConfigProto type_config;
  type_config.set_schema_type("type");
  // Adds more properties than allowed
  int max_num_sections_allowed = kMaxSectionId - kMinSectionId + 1;
  for (int i = 0; i < max_num_sections_allowed + 1; i++) {
    auto property = type_config.add_properties();
    property->set_property_name("property" + std::to_string(i));
    property->set_data_type(PropertyConfigProto::DataType::STRING);
    property->set_cardinality(PropertyConfigProto::Cardinality::REQUIRED);
    property->mutable_string_indexing_config()->set_term_match_type(
        TermMatchType::EXACT_ONLY);
  }

  SchemaUtil::TypeConfigMap type_config_map;
  type_config_map.emplace("type", type_config);

  EXPECT_THAT(
      SectionManager::Create(type_config_map, schema_type_mapper_.get()),
      StatusIs(libtextclassifier3::StatusCode::OUT_OF_RANGE,
               HasSubstr("Too many properties")));
}

TEST_F(SectionManagerTest, GetStringSectionContent) {
  ICING_ASSERT_OK_AND_ASSIGN(
      auto section_manager,
      SectionManager::Create(type_config_map_, schema_type_mapper_.get()));

  // Test simple section paths
  EXPECT_THAT(
      section_manager->GetStringSectionContent(email_document_,
                                               /*section_path*/ "subject"),
      IsOkAndHolds(ElementsAre("the subject")));
  EXPECT_THAT(section_manager->GetStringSectionContent(email_document_,
                                                       /*section_path*/ "text"),
              IsOkAndHolds(ElementsAre("the text")));

  // Test repeated values, they are joined into one string
  ICING_ASSERT_OK_AND_ASSIGN(
      auto content,
      section_manager->GetStringSectionContent(email_document_,
                                               /*section_path*/ "recipients"));
  EXPECT_THAT(content, ElementsAre("recipient1", "recipient2", "recipient3"));

  // Test concatenated section paths: "property1.property2"
  ICING_ASSERT_OK_AND_ASSIGN(content, section_manager->GetStringSectionContent(
                                          conversation_document_,
                                          /*section_path*/ "emails.subject"));
  EXPECT_THAT(content, ElementsAre("the subject", "the subject"));

  ICING_ASSERT_OK_AND_ASSIGN(content, section_manager->GetStringSectionContent(
                                          conversation_document_,
                                          /*section_path*/ "emails.text"));
  EXPECT_THAT(content, ElementsAre("the text", "the text"));

  ICING_ASSERT_OK_AND_ASSIGN(content,
                             section_manager->GetStringSectionContent(
                                 conversation_document_,
                                 /*section_path*/ "emails.recipients"));
  EXPECT_THAT(content, ElementsAre("recipient1", "recipient2", "recipient3",
                                   "recipient1", "recipient2", "recipient3"));

  // Test non-existing paths
  EXPECT_THAT(section_manager->GetStringSectionContent(email_document_,
                                                       /*section_path*/ "name"),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
  EXPECT_THAT(
      section_manager->GetStringSectionContent(email_document_,
                                               /*section_path*/ "invalid"),
      StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
  EXPECT_THAT(section_manager->GetStringSectionContent(
                  conversation_document_,
                  /*section_path*/ "emails.invalid"),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));

  // Test other data types
  // BYTES type can't be indexed, so content won't be returned
  EXPECT_THAT(
      section_manager->GetStringSectionContent(email_document_,
                                               /*section_path*/ "attachment"),
      StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));

  // The following tests are similar to the ones above but use section ids
  // instead of section paths

  // EmailMessage (section id -> section path):
  SectionId recipients_section_id = 0;
  SectionId subject_section_id = 1;
  SectionId invalid_email_section_id = 2;
  ICING_ASSERT_OK_AND_ASSIGN(
      content, section_manager->GetStringSectionContent(email_document_,
                                                        recipients_section_id));
  EXPECT_THAT(content, ElementsAre("recipient1", "recipient2", "recipient3"));

  EXPECT_THAT(section_manager->GetStringSectionContent(email_document_,
                                                       subject_section_id),
              IsOkAndHolds(ElementsAre("the subject")));

  EXPECT_THAT(section_manager->GetStringSectionContent(
                  email_document_, invalid_email_section_id),
              StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT));

  // Conversation (section id -> section path):
  //   0 -> emails.recipients
  //   1 -> emails.subject
  //   2 -> name
  SectionId emails_recipients_section_id = 0;
  SectionId emails_subject_section_id = 1;
  SectionId name_section_id = 2;
  SectionId invalid_conversation_section_id = 3;
  ICING_ASSERT_OK_AND_ASSIGN(
      content, section_manager->GetStringSectionContent(
                   conversation_document_, emails_recipients_section_id));
  EXPECT_THAT(content, ElementsAre("recipient1", "recipient2", "recipient3",
                                   "recipient1", "recipient2", "recipient3"));

  ICING_ASSERT_OK_AND_ASSIGN(
      content, section_manager->GetStringSectionContent(
                   conversation_document_, emails_subject_section_id));
  EXPECT_THAT(content, ElementsAre("the subject", "the subject"));

  EXPECT_THAT(section_manager->GetStringSectionContent(conversation_document_,
                                                       name_section_id),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));

  EXPECT_THAT(section_manager->GetStringSectionContent(
                  conversation_document_, invalid_conversation_section_id),
              StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT));
}

TEST_F(SectionManagerTest, ExtractSections) {
  ICING_ASSERT_OK_AND_ASSIGN(
      auto section_manager,
      SectionManager::Create(type_config_map_, schema_type_mapper_.get()));

  // Extracts all sections from 'EmailMessage' document
  ICING_ASSERT_OK_AND_ASSIGN(auto sections,
                             section_manager->ExtractSections(email_document_));
  EXPECT_THAT(sections.size(), Eq(2));

  EXPECT_THAT(sections[0].metadata.id, Eq(0));
  EXPECT_THAT(sections[0].metadata.path, Eq("recipients"));
  EXPECT_THAT(sections[0].content,
              ElementsAre("recipient1", "recipient2", "recipient3"));

  EXPECT_THAT(sections[1].metadata.id, Eq(1));
  EXPECT_THAT(sections[1].metadata.path, Eq("subject"));
  EXPECT_THAT(sections[1].content, ElementsAre("the subject"));

  // Extracts all sections from 'Conversation' document
  ICING_ASSERT_OK_AND_ASSIGN(
      sections, section_manager->ExtractSections(conversation_document_));
  EXPECT_THAT(sections.size(), Eq(2));

  // Section id 3 (name) not found in document, so the first section id found
  // is 1 below.
  EXPECT_THAT(sections[0].metadata.id, Eq(0));
  EXPECT_THAT(sections[0].metadata.path, Eq("emails.recipients"));
  EXPECT_THAT(sections[0].content,
              ElementsAre("recipient1", "recipient2", "recipient3",
                          "recipient1", "recipient2", "recipient3"));

  EXPECT_THAT(sections[1].metadata.id, Eq(1));
  EXPECT_THAT(sections[1].metadata.path, Eq("emails.subject"));
  EXPECT_THAT(sections[1].content, ElementsAre("the subject", "the subject"));
}

TEST_F(SectionManagerTest,
       NonStringFieldsWithStringIndexingConfigDontCreateSections) {
  // Create a schema for an empty document.
  SchemaTypeConfigProto empty_type;
  empty_type.set_schema_type("EmptySchema");

  // Create a schema with all the non-string fields
  SchemaTypeConfigProto type_with_non_string_properties;
  type_with_non_string_properties.set_schema_type("Schema");

  // Create an int property with a string_indexing_config
  auto int_property = type_with_non_string_properties.add_properties();
  int_property->set_property_name("int");
  int_property->set_data_type(PropertyConfigProto::DataType::INT64);
  int_property->set_cardinality(PropertyConfigProto::Cardinality::REQUIRED);
  int_property->mutable_string_indexing_config()->set_term_match_type(
      TermMatchType::EXACT_ONLY);
  int_property->mutable_string_indexing_config()->set_tokenizer_type(
      StringIndexingConfig::TokenizerType::PLAIN);

  // Create a double property with a string_indexing_config
  auto double_property = type_with_non_string_properties.add_properties();
  double_property->set_property_name("double");
  double_property->set_data_type(PropertyConfigProto::DataType::DOUBLE);
  double_property->set_cardinality(PropertyConfigProto::Cardinality::REQUIRED);
  double_property->mutable_string_indexing_config()->set_term_match_type(
      TermMatchType::EXACT_ONLY);
  double_property->mutable_string_indexing_config()->set_tokenizer_type(
      StringIndexingConfig::TokenizerType::PLAIN);

  // Create a boolean property with a string_indexing_config
  auto boolean_property = type_with_non_string_properties.add_properties();
  boolean_property->set_property_name("boolean");
  boolean_property->set_data_type(PropertyConfigProto::DataType::BOOLEAN);
  boolean_property->set_cardinality(PropertyConfigProto::Cardinality::REQUIRED);
  boolean_property->mutable_string_indexing_config()->set_term_match_type(
      TermMatchType::EXACT_ONLY);
  boolean_property->mutable_string_indexing_config()->set_tokenizer_type(
      StringIndexingConfig::TokenizerType::PLAIN);

  // Create a bytes property with a string_indexing_config
  auto bytes_property = type_with_non_string_properties.add_properties();
  bytes_property->set_property_name("bytes");
  bytes_property->set_data_type(PropertyConfigProto::DataType::BYTES);
  bytes_property->set_cardinality(PropertyConfigProto::Cardinality::REQUIRED);
  bytes_property->mutable_string_indexing_config()->set_term_match_type(
      TermMatchType::EXACT_ONLY);
  bytes_property->mutable_string_indexing_config()->set_tokenizer_type(
      StringIndexingConfig::TokenizerType::PLAIN);

  // Create a document property with a string_indexing_config
  auto document_property = type_with_non_string_properties.add_properties();
  document_property->set_property_name("document");
  document_property->set_data_type(PropertyConfigProto::DataType::DOCUMENT);
  document_property->set_schema_type(empty_type.schema_type());
  document_property->set_cardinality(
      PropertyConfigProto::Cardinality::REQUIRED);
  document_property->mutable_string_indexing_config()->set_term_match_type(
      TermMatchType::EXACT_ONLY);
  document_property->mutable_string_indexing_config()->set_tokenizer_type(
      StringIndexingConfig::TokenizerType::PLAIN);

  // Setup classes to create the section manager
  SchemaUtil::TypeConfigMap type_config_map;
  type_config_map.emplace(type_with_non_string_properties.schema_type(),
                          type_with_non_string_properties);
  type_config_map.emplace(empty_type.schema_type(), empty_type);

  // KeyMapper uses 3 internal arrays for bookkeeping. Give each one 128KiB so
  // the total KeyMapper should get 384KiB
  int key_mapper_size = 3 * 128 * 1024;
  std::string dir = GetTestTempDir() + "/non_string_fields";
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<KeyMapper<SchemaTypeId>> schema_type_mapper,
      KeyMapper<SchemaTypeId>::Create(filesystem_, dir, key_mapper_size));
  ICING_ASSERT_OK(schema_type_mapper->Put(
      type_with_non_string_properties.schema_type(), /*schema_type_id=*/0));
  ICING_ASSERT_OK(schema_type_mapper->Put(empty_type.schema_type(),
                                          /*schema_type_id=*/1));

  ICING_ASSERT_OK_AND_ASSIGN(
      auto section_manager,
      SectionManager::Create(type_config_map, schema_type_mapper.get()));

  // Create an empty document to be nested
  DocumentProto empty_document = DocumentBuilder()
                                     .SetKey("icing", "uri1")
                                     .SetSchema(empty_type.schema_type())
                                     .Build();

  // Create a document that follows "Schema"
  DocumentProto document =
      DocumentBuilder()
          .SetKey("icing", "uri2")
          .SetSchema(type_with_non_string_properties.schema_type())
          .AddInt64Property("int", 1)
          .AddDoubleProperty("double", 0.2)
          .AddBooleanProperty("boolean", true)
          .AddBytesProperty("bytes", "attachment bytes")
          .AddDocumentProperty("document", empty_document)
          .Build();

  // Extracts sections from 'Schema' document
  ICING_ASSERT_OK_AND_ASSIGN(auto sections,
                             section_manager->ExtractSections(document));
  EXPECT_THAT(sections.size(), Eq(0));
}

TEST_F(SectionManagerTest, AssignSectionsRecursivelyForDocumentFields) {
  // Create the inner schema that the document property is.
  SchemaTypeConfigProto document_type;
  document_type.set_schema_type("DocumentSchema");

  auto string_property = document_type.add_properties();
  string_property->set_property_name("string");
  string_property->set_data_type(PropertyConfigProto::DataType::STRING);
  string_property->set_cardinality(PropertyConfigProto::Cardinality::REQUIRED);
  string_property->mutable_string_indexing_config()->set_term_match_type(
      TermMatchType::EXACT_ONLY);
  string_property->mutable_string_indexing_config()->set_tokenizer_type(
      StringIndexingConfig::TokenizerType::PLAIN);

  // Create the outer schema which has the document property.
  SchemaTypeConfigProto type;
  type.set_schema_type("Schema");

  auto document_property = type.add_properties();
  document_property->set_property_name("document");
  document_property->set_data_type(PropertyConfigProto::DataType::DOCUMENT);
  document_property->set_schema_type(document_type.schema_type());
  document_property->set_cardinality(
      PropertyConfigProto::Cardinality::REQUIRED);

  // Opt into recursing into the document fields.
  document_property->mutable_document_indexing_config()
      ->set_index_nested_properties(true);

  // Create the inner document.
  DocumentProto inner_document = DocumentBuilder()
                                     .SetKey("icing", "uri1")
                                     .SetSchema(document_type.schema_type())
                                     .AddStringProperty("string", "foo")
                                     .Build();

  // Create the outer document that holds the inner document
  DocumentProto outer_document =
      DocumentBuilder()
          .SetKey("icing", "uri2")
          .SetSchema(type.schema_type())
          .AddDocumentProperty("document", inner_document)
          .Build();

  // Setup classes to create the section manager
  SchemaUtil::TypeConfigMap type_config_map;
  type_config_map.emplace(type.schema_type(), type);
  type_config_map.emplace(document_type.schema_type(), document_type);

  // KeyMapper uses 3 internal arrays for bookkeeping. Give each one 128KiB so
  // the total KeyMapper should get 384KiB
  int key_mapper_size = 3 * 128 * 1024;
  std::string dir = GetTestTempDir() + "/recurse_into_document";
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<KeyMapper<SchemaTypeId>> schema_type_mapper,
      KeyMapper<SchemaTypeId>::Create(filesystem_, dir, key_mapper_size));
  int type_schema_type_id = 0;
  int document_type_schema_type_id = 1;
  ICING_ASSERT_OK(
      schema_type_mapper->Put(type.schema_type(), type_schema_type_id));
  ICING_ASSERT_OK(schema_type_mapper->Put(document_type.schema_type(),
                                          document_type_schema_type_id));

  ICING_ASSERT_OK_AND_ASSIGN(
      auto section_manager,
      SectionManager::Create(type_config_map, schema_type_mapper.get()));

  // Extracts sections from 'Schema' document; there should be the 1 string
  // property inside the document.
  ICING_ASSERT_OK_AND_ASSIGN(std::vector<Section> sections,
                             section_manager->ExtractSections(outer_document));
  EXPECT_THAT(sections, SizeIs(1));
}

TEST_F(SectionManagerTest, DontAssignSectionsRecursivelyForDocumentFields) {
  // Create the inner schema that the document property is.
  SchemaTypeConfigProto document_type;
  document_type.set_schema_type("DocumentSchema");

  auto string_property = document_type.add_properties();
  string_property->set_property_name("string");
  string_property->set_data_type(PropertyConfigProto::DataType::STRING);
  string_property->set_cardinality(PropertyConfigProto::Cardinality::REQUIRED);
  string_property->mutable_string_indexing_config()->set_term_match_type(
      TermMatchType::EXACT_ONLY);
  string_property->mutable_string_indexing_config()->set_tokenizer_type(
      StringIndexingConfig::TokenizerType::PLAIN);

  // Create the outer schema which has the document property.
  SchemaTypeConfigProto type;
  type.set_schema_type("Schema");

  auto document_property = type.add_properties();
  document_property->set_property_name("document");
  document_property->set_data_type(PropertyConfigProto::DataType::DOCUMENT);
  document_property->set_schema_type(document_type.schema_type());
  document_property->set_cardinality(
      PropertyConfigProto::Cardinality::REQUIRED);

  // Opt into recursing into the document fields.
  document_property->mutable_document_indexing_config()
      ->set_index_nested_properties(false);

  // Create the inner document.
  DocumentProto inner_document = DocumentBuilder()
                                     .SetKey("icing", "uri1")
                                     .SetSchema(document_type.schema_type())
                                     .AddStringProperty("string", "foo")
                                     .Build();

  // Create the outer document that holds the inner document
  DocumentProto outer_document =
      DocumentBuilder()
          .SetKey("icing", "uri2")
          .SetSchema(type.schema_type())
          .AddDocumentProperty("document", inner_document)
          .Build();

  // Setup classes to create the section manager
  SchemaUtil::TypeConfigMap type_config_map;
  type_config_map.emplace(type.schema_type(), type);
  type_config_map.emplace(document_type.schema_type(), document_type);

  // KeyMapper uses 3 internal arrays for bookkeeping. Give each one 128KiB so
  // the total KeyMapper should get 384KiB
  int key_mapper_size = 3 * 128 * 1024;
  std::string dir = GetTestTempDir() + "/recurse_into_document";
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<KeyMapper<SchemaTypeId>> schema_type_mapper,
      KeyMapper<SchemaTypeId>::Create(filesystem_, dir, key_mapper_size));
  int type_schema_type_id = 0;
  int document_type_schema_type_id = 1;
  ICING_ASSERT_OK(
      schema_type_mapper->Put(type.schema_type(), type_schema_type_id));
  ICING_ASSERT_OK(schema_type_mapper->Put(document_type.schema_type(),
                                          document_type_schema_type_id));

  ICING_ASSERT_OK_AND_ASSIGN(
      auto section_manager,
      SectionManager::Create(type_config_map, schema_type_mapper.get()));

  // Extracts sections from 'Schema' document; there won't be any since we
  // didn't recurse into the document to see the inner string property
  ICING_ASSERT_OK_AND_ASSIGN(std::vector<Section> sections,
                             section_manager->ExtractSections(outer_document));
  EXPECT_THAT(sections, IsEmpty());
}

}  // namespace lib
}  // namespace icing
