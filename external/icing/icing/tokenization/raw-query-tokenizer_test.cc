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

#include "icing/tokenization/raw-query-tokenizer.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "icing/helpers/icu/icu-data-file-helper.h"
#include "icing/portable/platform.h"
#include "icing/testing/common-matchers.h"
#include "icing/testing/test-data.h"
#include "icing/tokenization/language-segmenter-factory.h"
#include "icing/tokenization/tokenizer-factory.h"
#include "icing/tokenization/tokenizer.h"
#include "unicode/uloc.h"

namespace icing {
namespace lib {
namespace {
using ::testing::ElementsAre;
using ::testing::HasSubstr;

class RawQueryTokenizerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    if (!IsCfStringTokenization() && !IsReverseJniTokenization()) {
      ICING_ASSERT_OK(
          // File generated via icu_data_file rule in //icing/BUILD.
          icu_data_file_helper::SetUpICUDataFile(
              GetTestFilePath("icing/icu.dat")));
    }
  }
};

TEST_F(RawQueryTokenizerTest, CreationWithNullPointerShouldFail) {
  EXPECT_THAT(tokenizer_factory::CreateQueryTokenizer(
                  tokenizer_factory::RAW_QUERY, /*lang_segmenter=*/nullptr),
              StatusIs(libtextclassifier3::StatusCode::FAILED_PRECONDITION));
}

TEST_F(RawQueryTokenizerTest, Simple) {
  language_segmenter_factory::SegmenterOptions options(ULOC_US);
  ICING_ASSERT_OK_AND_ASSIGN(
      auto language_segmenter,
      language_segmenter_factory::Create(std::move(options)));
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<Tokenizer> raw_query_tokenizer,
      tokenizer_factory::CreateQueryTokenizer(tokenizer_factory::RAW_QUERY,
                                              language_segmenter.get()));

  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("Hello World!"),
              IsOkAndHolds(ElementsAre(EqualsToken(Token::REGULAR, "Hello"),
                                       EqualsToken(Token::REGULAR, "World"))));

  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("hElLo WORLD"),
              IsOkAndHolds(ElementsAre(EqualsToken(Token::REGULAR, "hElLo"),
                                       EqualsToken(Token::REGULAR, "WORLD"))));
}

TEST_F(RawQueryTokenizerTest, Parentheses) {
  language_segmenter_factory::SegmenterOptions options(ULOC_US);
  ICING_ASSERT_OK_AND_ASSIGN(
      auto language_segmenter,
      language_segmenter_factory::Create(std::move(options)));
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<Tokenizer> raw_query_tokenizer,
      tokenizer_factory::CreateQueryTokenizer(tokenizer_factory::RAW_QUERY,
                                              language_segmenter.get()));

  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("()"),
              IsOkAndHolds(ElementsAre(
                  EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                  EqualsToken(Token::QUERY_RIGHT_PARENTHESES, ""))));

  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("( )"),
              IsOkAndHolds(ElementsAre(
                  EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                  EqualsToken(Token::QUERY_RIGHT_PARENTHESES, ""))));

  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("(term1 term2)"),
              IsOkAndHolds(ElementsAre(
                  EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                  EqualsToken(Token::REGULAR, "term1"),
                  EqualsToken(Token::REGULAR, "term2"),
                  EqualsToken(Token::QUERY_RIGHT_PARENTHESES, ""))));

  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("((term1 term2) (term3 term4))"),
              IsOkAndHolds(ElementsAre(
                  EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                  EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                  EqualsToken(Token::REGULAR, "term1"),
                  EqualsToken(Token::REGULAR, "term2"),
                  EqualsToken(Token::QUERY_RIGHT_PARENTHESES, ""),
                  EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                  EqualsToken(Token::REGULAR, "term3"),
                  EqualsToken(Token::REGULAR, "term4"),
                  EqualsToken(Token::QUERY_RIGHT_PARENTHESES, ""),
                  EqualsToken(Token::QUERY_RIGHT_PARENTHESES, ""))));

  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("term1(term2)"),
              IsOkAndHolds(ElementsAre(
                  EqualsToken(Token::REGULAR, "term1"),
                  EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                  EqualsToken(Token::REGULAR, "term2"),
                  EqualsToken(Token::QUERY_RIGHT_PARENTHESES, ""))));

  EXPECT_THAT(
      raw_query_tokenizer->TokenizeAll("(term1)term2"),
      IsOkAndHolds(ElementsAre(EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                               EqualsToken(Token::REGULAR, "term1"),
                               EqualsToken(Token::QUERY_RIGHT_PARENTHESES, ""),
                               EqualsToken(Token::REGULAR, "term2"))));

  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("(term1)(term2)"),
              IsOkAndHolds(ElementsAre(
                  EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                  EqualsToken(Token::REGULAR, "term1"),
                  EqualsToken(Token::QUERY_RIGHT_PARENTHESES, ""),
                  EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                  EqualsToken(Token::REGULAR, "term2"),
                  EqualsToken(Token::QUERY_RIGHT_PARENTHESES, ""))));

  EXPECT_THAT(
      raw_query_tokenizer->TokenizeAll("(term1)-term2"),
      IsOkAndHolds(ElementsAre(EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                               EqualsToken(Token::REGULAR, "term1"),
                               EqualsToken(Token::QUERY_RIGHT_PARENTHESES, ""),
                               EqualsToken(Token::QUERY_EXCLUSION, ""),
                               EqualsToken(Token::REGULAR, "term2"))));

  EXPECT_THAT(
      raw_query_tokenizer->TokenizeAll("(term1)OR term2"),
      IsOkAndHolds(ElementsAre(EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                               EqualsToken(Token::REGULAR, "term1"),
                               EqualsToken(Token::QUERY_RIGHT_PARENTHESES, ""),
                               EqualsToken(Token::QUERY_OR, ""),
                               EqualsToken(Token::REGULAR, "term2"))));

  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("(term1)OR(term2)"),
              IsOkAndHolds(ElementsAre(
                  EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                  EqualsToken(Token::REGULAR, "term1"),
                  EqualsToken(Token::QUERY_RIGHT_PARENTHESES, ""),
                  EqualsToken(Token::QUERY_OR, ""),
                  EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                  EqualsToken(Token::REGULAR, "term2"),
                  EqualsToken(Token::QUERY_RIGHT_PARENTHESES, ""))));

  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("(term1):term2"),
              StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT,
                       HasSubstr("Property name can't be a group")));

  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("((term1)"),
              StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT,
                       HasSubstr("Unclosed left parentheses")));

  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("(term1))"),
              StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT,
                       HasSubstr("Too many right parentheses")));
}

TEST_F(RawQueryTokenizerTest, Exclustion) {
  language_segmenter_factory::SegmenterOptions options(ULOC_US);
  ICING_ASSERT_OK_AND_ASSIGN(
      auto language_segmenter,
      language_segmenter_factory::Create(std::move(options)));
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<Tokenizer> raw_query_tokenizer,
      tokenizer_factory::CreateQueryTokenizer(tokenizer_factory::RAW_QUERY,
                                              language_segmenter.get()));

  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("-term1"),
              IsOkAndHolds(ElementsAre(EqualsToken(Token::QUERY_EXCLUSION, ""),
                                       EqualsToken(Token::REGULAR, "term1"))));

  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("(-term1)"),
              IsOkAndHolds(ElementsAre(
                  EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                  EqualsToken(Token::QUERY_EXCLUSION, ""),
                  EqualsToken(Token::REGULAR, "term1"),
                  EqualsToken(Token::QUERY_RIGHT_PARENTHESES, ""))));

  // Exclusion operator is ignored
  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("- term1"),
              IsOkAndHolds(ElementsAre(EqualsToken(Token::REGULAR, "term1"))));

  // Exclusion operator is ignored
  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("term1- term2"),
              IsOkAndHolds(ElementsAre(EqualsToken(Token::REGULAR, "term1"),
                                       EqualsToken(Token::REGULAR, "term2"))));

  // Exclusion operator is ignored
  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("(term1 -)"),
              IsOkAndHolds(ElementsAre(
                  EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                  EqualsToken(Token::REGULAR, "term1"),
                  EqualsToken(Token::QUERY_RIGHT_PARENTHESES, ""))));

  // First exclusion operator is ignored
  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("--term1"),
              IsOkAndHolds(ElementsAre(EqualsToken(Token::QUERY_EXCLUSION, ""),
                                       EqualsToken(Token::REGULAR, "term1"))));

  // First "-" is exclusion operator, second is not and will be discarded.
  // In other words, exclusion only applies to the term right after it.
  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("-term1-term2"),
              IsOkAndHolds(ElementsAre(EqualsToken(Token::QUERY_EXCLUSION, ""),
                                       EqualsToken(Token::REGULAR, "term1"),
                                       EqualsToken(Token::REGULAR, "term2"))));

  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("-(term1)"),
              StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT,
                       HasSubstr("Exclusion on groups is not supported")));

  EXPECT_THAT(
      raw_query_tokenizer->TokenizeAll("-OR"),
      StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT,
               HasSubstr("Exclusion and OR operators can't be used together")));

  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("-:term1"),
              StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT,
                       HasSubstr("Exclusion and property restriction operators "
                                 "can't be used together")));

  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("-property1:term1"),
              StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT,
                       HasSubstr("Exclusion and property restriction operators "
                                 "can't be used together")));
}

TEST_F(RawQueryTokenizerTest, PropertyRestriction) {
  language_segmenter_factory::SegmenterOptions options(ULOC_US);
  ICING_ASSERT_OK_AND_ASSIGN(
      auto language_segmenter,
      language_segmenter_factory::Create(std::move(options)));
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<Tokenizer> raw_query_tokenizer,
      tokenizer_factory::CreateQueryTokenizer(tokenizer_factory::RAW_QUERY,
                                              language_segmenter.get()));

  EXPECT_THAT(
      raw_query_tokenizer->TokenizeAll("property1:term1"),
      IsOkAndHolds(ElementsAre(EqualsToken(Token::QUERY_PROPERTY, "property1"),
                               EqualsToken(Token::REGULAR, "term1"))));

  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("(property1:term1)"),
              IsOkAndHolds(ElementsAre(
                  EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                  EqualsToken(Token::QUERY_PROPERTY, "property1"),
                  EqualsToken(Token::REGULAR, "term1"),
                  EqualsToken(Token::QUERY_RIGHT_PARENTHESES, ""))));

  // Colon is ignored
  EXPECT_THAT(raw_query_tokenizer->TokenizeAll(":term1"),
              IsOkAndHolds(ElementsAre(EqualsToken(Token::REGULAR, "term1"))));

  // Colon is ignored
  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("(:term1)"),
              IsOkAndHolds(ElementsAre(
                  EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                  EqualsToken(Token::REGULAR, "term1"),
                  EqualsToken(Token::QUERY_RIGHT_PARENTHESES, ""))));

  // Colon is ignored
  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("term1:"),
              IsOkAndHolds(ElementsAre(EqualsToken(Token::REGULAR, "term1"))));

  // property name can be a path
  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("email.title:hello"),
              IsOkAndHolds(
                  ElementsAre(EqualsToken(Token::QUERY_PROPERTY, "email.title"),
                              EqualsToken(Token::REGULAR, "hello"))));

  // The first colon ":" triggers property restriction, the second colon is used
  // as a word connector per ICU's rule
  // (https://unicode.org/reports/tr29/#Word_Boundaries).
  EXPECT_THAT(
      raw_query_tokenizer->TokenizeAll("property:foo:bar"),
      IsOkAndHolds(ElementsAre(EqualsToken(Token::QUERY_PROPERTY, "property"),
                               EqualsToken(Token::REGULAR, "foo:bar"))));

  // Property restriction only applies to the term right after it.
  // Note: "term1:term2" is not a term but 2 terms because word connectors
  // don't apply to numbers and alphabets.
  EXPECT_THAT(
      raw_query_tokenizer->TokenizeAll("property1:term1:term2"),
      IsOkAndHolds(ElementsAre(EqualsToken(Token::QUERY_PROPERTY, "property1"),
                               EqualsToken(Token::REGULAR, "term1"),
                               EqualsToken(Token::REGULAR, "term2"))));

  EXPECT_THAT(
      raw_query_tokenizer->TokenizeAll("property1:今天:天气"),
      IsOkAndHolds(ElementsAre(EqualsToken(Token::QUERY_PROPERTY, "property1"),
                               EqualsToken(Token::REGULAR, "今天"),
                               EqualsToken(Token::REGULAR, "天气"))));

  EXPECT_THAT(
      raw_query_tokenizer->TokenizeAll("property1:term1-"),
      IsOkAndHolds(ElementsAre(EqualsToken(Token::QUERY_PROPERTY, "property1"),
                               EqualsToken(Token::REGULAR, "term1"))));

  // Multiple continuous colons will still be recognized as a property
  // restriction operator
  EXPECT_THAT(
      raw_query_tokenizer->TokenizeAll("property1::term1"),
      IsOkAndHolds(ElementsAre(EqualsToken(Token::QUERY_PROPERTY, "property1"),
                               EqualsToken(Token::REGULAR, "term1"))));

  EXPECT_THAT(
      raw_query_tokenizer->TokenizeAll("property1:(term1)"),
      StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT,
               HasSubstr("Property restriction on groups is not supported")));

  EXPECT_THAT(
      raw_query_tokenizer->TokenizeAll("property1:OR"),
      StatusIs(
          libtextclassifier3::StatusCode::INVALID_ARGUMENT,
          HasSubstr(
              "Property restriction and OR operators can't be used together")));

  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("property1:-term1"),
              StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT,
                       HasSubstr("Exclusion and property restriction operators "
                                 "can't be used together")));
}

TEST_F(RawQueryTokenizerTest, OR) {
  language_segmenter_factory::SegmenterOptions options(ULOC_US);
  ICING_ASSERT_OK_AND_ASSIGN(
      auto language_segmenter,
      language_segmenter_factory::Create(std::move(options)));
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<Tokenizer> raw_query_tokenizer,
      tokenizer_factory::CreateQueryTokenizer(tokenizer_factory::RAW_QUERY,
                                              language_segmenter.get()));

  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("term1 OR term2"),
              IsOkAndHolds(ElementsAre(EqualsToken(Token::REGULAR, "term1"),
                                       EqualsToken(Token::QUERY_OR, ""),
                                       EqualsToken(Token::REGULAR, "term2"))));

  // Two continuous "OR"s are treated as one
  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("term1 OR OR term2"),
              IsOkAndHolds(ElementsAre(EqualsToken(Token::REGULAR, "term1"),
                                       EqualsToken(Token::QUERY_OR, ""),
                                       EqualsToken(Token::REGULAR, "term2"))));

  EXPECT_THAT(
      raw_query_tokenizer->TokenizeAll("(term1) OR term2"),
      IsOkAndHolds(ElementsAre(EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                               EqualsToken(Token::REGULAR, "term1"),
                               EqualsToken(Token::QUERY_RIGHT_PARENTHESES, ""),
                               EqualsToken(Token::QUERY_OR, ""),
                               EqualsToken(Token::REGULAR, "term2"))));

  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("term1 OR (term2)"),
              IsOkAndHolds(ElementsAre(
                  EqualsToken(Token::REGULAR, "term1"),
                  EqualsToken(Token::QUERY_OR, ""),
                  EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                  EqualsToken(Token::REGULAR, "term2"),
                  EqualsToken(Token::QUERY_RIGHT_PARENTHESES, ""))));

  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("((term1) OR (term2))"),
              IsOkAndHolds(ElementsAre(
                  EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                  EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                  EqualsToken(Token::REGULAR, "term1"),
                  EqualsToken(Token::QUERY_RIGHT_PARENTHESES, ""),
                  EqualsToken(Token::QUERY_OR, ""),
                  EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                  EqualsToken(Token::REGULAR, "term2"),
                  EqualsToken(Token::QUERY_RIGHT_PARENTHESES, ""),
                  EqualsToken(Token::QUERY_RIGHT_PARENTHESES, ""))));

  // Only "OR" (all in uppercase) is the operator
  EXPECT_THAT(
      raw_query_tokenizer->TokenizeAll("term1 or term2 Or term3 oR term4"),
      IsOkAndHolds(ElementsAre(EqualsToken(Token::REGULAR, "term1"),
                               EqualsToken(Token::REGULAR, "or"),
                               EqualsToken(Token::REGULAR, "term2"),
                               EqualsToken(Token::REGULAR, "Or"),
                               EqualsToken(Token::REGULAR, "term3"),
                               EqualsToken(Token::REGULAR, "oR"),
                               EqualsToken(Token::REGULAR, "term4"))));

  // "OR" is ignored
  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("OR term1"),
              IsOkAndHolds(ElementsAre(EqualsToken(Token::REGULAR, "term1"))));

  // "OR" is ignored
  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("term1 OR"),
              IsOkAndHolds(ElementsAre(EqualsToken(Token::REGULAR, "term1"))));

  // "OR" is ignored
  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("(OR term1)"),
              IsOkAndHolds(ElementsAre(
                  EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                  EqualsToken(Token::REGULAR, "term1"),
                  EqualsToken(Token::QUERY_RIGHT_PARENTHESES, ""))));

  // "OR" is ignored
  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("( OR term1)"),
              IsOkAndHolds(ElementsAre(
                  EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                  EqualsToken(Token::REGULAR, "term1"),
                  EqualsToken(Token::QUERY_RIGHT_PARENTHESES, ""))));

  // "OR" is ignored
  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("(term1 OR)"),
              IsOkAndHolds(ElementsAre(
                  EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                  EqualsToken(Token::REGULAR, "term1"),
                  EqualsToken(Token::QUERY_RIGHT_PARENTHESES, ""))));

  // "OR" is ignored
  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("(term1 OR )"),
              IsOkAndHolds(ElementsAre(
                  EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                  EqualsToken(Token::REGULAR, "term1"),
                  EqualsToken(Token::QUERY_RIGHT_PARENTHESES, ""))));

  // "OR" is ignored
  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("( OR )"),
              IsOkAndHolds(ElementsAre(
                  EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                  EqualsToken(Token::QUERY_RIGHT_PARENTHESES, ""))));

  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("term1 OR(term2)"),
              IsOkAndHolds(ElementsAre(
                  EqualsToken(Token::REGULAR, "term1"),
                  EqualsToken(Token::QUERY_OR, ""),
                  EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                  EqualsToken(Token::REGULAR, "term2"),
                  EqualsToken(Token::QUERY_RIGHT_PARENTHESES, ""))));

  EXPECT_THAT(
      raw_query_tokenizer->TokenizeAll("term1 OR-term2"),
      StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT,
               HasSubstr("No whitespaces before or after OR operator")));

  EXPECT_THAT(
      raw_query_tokenizer->TokenizeAll("term1 OR:term2"),
      StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT,
               HasSubstr("No whitespaces before or after OR operator")));
}

// CJKT are treated the same way by language segmenter and raw tokenizer, so
// here we test Chinese and Japanese to represent CJKT.
TEST_F(RawQueryTokenizerTest, CJKT) {
  language_segmenter_factory::SegmenterOptions options(ULOC_US);
  ICING_ASSERT_OK_AND_ASSIGN(
      auto language_segmenter,
      language_segmenter_factory::Create(std::move(options)));
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<Tokenizer> raw_query_tokenizer,
      tokenizer_factory::CreateQueryTokenizer(tokenizer_factory::RAW_QUERY,
                                              language_segmenter.get()));

  // Exclusion only applies to the term right after it.
  if (IsCfStringTokenization()) {
    EXPECT_THAT(
        raw_query_tokenizer->TokenizeAll("-今天天气很好"),
        IsOkAndHolds(ElementsAre(EqualsToken(Token::QUERY_EXCLUSION, ""),
                                 EqualsToken(Token::REGULAR, "今天"),
                                 EqualsToken(Token::REGULAR, "天气"),
                                 EqualsToken(Token::REGULAR, "很"),
                                 EqualsToken(Token::REGULAR, "好"))));
  } else {
    EXPECT_THAT(
        raw_query_tokenizer->TokenizeAll("-今天天气很好"),
        IsOkAndHolds(ElementsAre(EqualsToken(Token::QUERY_EXCLUSION, ""),
                                 EqualsToken(Token::REGULAR, "今天"),
                                 EqualsToken(Token::REGULAR, "天气"),
                                 EqualsToken(Token::REGULAR, "很好"))));
  }

  if (IsCfStringTokenization()) {
    EXPECT_THAT(raw_query_tokenizer->TokenizeAll("property1:你好"),
                IsOkAndHolds(
                    ElementsAre(EqualsToken(Token::QUERY_PROPERTY, "property1"),
                                EqualsToken(Token::REGULAR, "你"),
                                EqualsToken(Token::REGULAR, "好"))));
  } else {
    EXPECT_THAT(raw_query_tokenizer->TokenizeAll("property1:你好"),
                IsOkAndHolds(
                    ElementsAre(EqualsToken(Token::QUERY_PROPERTY, "property1"),
                                EqualsToken(Token::REGULAR, "你好"))));
  }

  EXPECT_THAT(
      raw_query_tokenizer->TokenizeAll("标题:你好"),
      StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT,
               HasSubstr("Characters in property name must all be ASCII")));

  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("cat OR ねこ"),
              IsOkAndHolds(ElementsAre(EqualsToken(Token::REGULAR, "cat"),
                                       EqualsToken(Token::QUERY_OR, ""),
                                       EqualsToken(Token::REGULAR, "ねこ"))));

  EXPECT_THAT(
      raw_query_tokenizer->TokenizeAll("cat ORねこ"),
      StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT,
               HasSubstr("No whitespaces before or after OR operator")));

  EXPECT_THAT(
      raw_query_tokenizer->TokenizeAll("ねこOR cat"),
      StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT,
               HasSubstr("No whitespaces before or after OR operator")));

  EXPECT_THAT(
      raw_query_tokenizer->TokenizeAll("-ねこOR cat"),
      StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT,
               HasSubstr("No whitespaces before or after OR operator")));

  EXPECT_THAT(
      raw_query_tokenizer->TokenizeAll("property:ねこOR cat"),
      StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT,
               HasSubstr("No whitespaces before or after OR operator")));
}

// Raw tokenizer identifies all characters that it doesn't know as OTHER type,
// so we can choose comma "," to represent all OTHER characters.
TEST_F(RawQueryTokenizerTest, OtherChars) {
  language_segmenter_factory::SegmenterOptions options(ULOC_US);
  ICING_ASSERT_OK_AND_ASSIGN(
      auto language_segmenter,
      language_segmenter_factory::Create(std::move(options)));
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<Tokenizer> raw_query_tokenizer,
      tokenizer_factory::CreateQueryTokenizer(tokenizer_factory::RAW_QUERY,
                                              language_segmenter.get()));

  // Comma is ignored
  EXPECT_THAT(raw_query_tokenizer->TokenizeAll(",term1, ,"),
              IsOkAndHolds(ElementsAre(EqualsToken(Token::REGULAR, "term1"))));

  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("(,term1),"),
              IsOkAndHolds(ElementsAre(
                  EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                  EqualsToken(Token::REGULAR, "term1"),
                  EqualsToken(Token::QUERY_RIGHT_PARENTHESES, ""))));

  // Exclusion operator and comma are ignored
  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("-,term1"),
              IsOkAndHolds(ElementsAre(EqualsToken(Token::REGULAR, "term1"))));

  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("-term1,"),
              IsOkAndHolds(ElementsAre(EqualsToken(Token::QUERY_EXCLUSION, ""),
                                       EqualsToken(Token::REGULAR, "term1"))));

  // Colon and comma are ignored
  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("property1:,term1"),
              IsOkAndHolds(ElementsAre(EqualsToken(Token::REGULAR, "property1"),
                                       EqualsToken(Token::REGULAR, "term1"))));

  EXPECT_THAT(
      raw_query_tokenizer->TokenizeAll("property1:term1,term2"),
      IsOkAndHolds(ElementsAre(EqualsToken(Token::QUERY_PROPERTY, "property1"),
                               EqualsToken(Token::REGULAR, "term1"),
                               EqualsToken(Token::REGULAR, "term2"))));

  // This is a special case for OR, unknown chars are treated the same as
  // whitespaces before and after OR.
  EXPECT_THAT(raw_query_tokenizer->TokenizeAll("term1,OR,term2"),
              IsOkAndHolds(ElementsAre(EqualsToken(Token::REGULAR, "term1"),
                                       EqualsToken(Token::QUERY_OR, ""),
                                       EqualsToken(Token::REGULAR, "term2"))));
}

TEST_F(RawQueryTokenizerTest, Mix) {
  language_segmenter_factory::SegmenterOptions options(ULOC_US);
  ICING_ASSERT_OK_AND_ASSIGN(
      auto language_segmenter,
      language_segmenter_factory::Create(std::move(options)));
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<Tokenizer> raw_query_tokenizer,
      tokenizer_factory::CreateQueryTokenizer(tokenizer_factory::RAW_QUERY,
                                              language_segmenter.get()));

  if (IsCfStringTokenization()) {
    EXPECT_THAT(raw_query_tokenizer->TokenizeAll(
                    "こんにちはgood afternoon, title:今天 OR (ในวันนี้ -B12)"),
                IsOkAndHolds(ElementsAre(
                    EqualsToken(Token::REGULAR, "こんにちは"),
                    EqualsToken(Token::REGULAR, "good"),
                    EqualsToken(Token::REGULAR, "afternoon"),
                    EqualsToken(Token::QUERY_PROPERTY, "title"),
                    EqualsToken(Token::REGULAR, "今天"),
                    EqualsToken(Token::QUERY_OR, ""),
                    EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                    EqualsToken(Token::REGULAR, "ใน"),
                    EqualsToken(Token::REGULAR, "วันนี้"),
                    EqualsToken(Token::QUERY_EXCLUSION, ""),
                    EqualsToken(Token::REGULAR, "B12"),
                    EqualsToken(Token::QUERY_RIGHT_PARENTHESES, ""))));
  } else {
    ICING_ASSERT_OK_AND_ASSIGN(
        std::vector<Token> tokens,
        raw_query_tokenizer->TokenizeAll(
            "こんにちはgood afternoon, title:今天 OR (ในวันนี้ -B12)"));
    EXPECT_THAT(tokens,
                ElementsAre(EqualsToken(Token::REGULAR, "こんにちは"),
                            EqualsToken(Token::REGULAR, "good"),
                            EqualsToken(Token::REGULAR, "afternoon"),
                            EqualsToken(Token::QUERY_PROPERTY, "title"),
                            EqualsToken(Token::REGULAR, "今天"),
                            EqualsToken(Token::QUERY_OR, ""),
                            EqualsToken(Token::QUERY_LEFT_PARENTHESES, ""),
                            EqualsToken(Token::REGULAR, "ใน"),
                            EqualsToken(Token::REGULAR, "วัน"),
                            EqualsToken(Token::REGULAR, "นี้"),
                            EqualsToken(Token::QUERY_EXCLUSION, ""),
                            EqualsToken(Token::REGULAR, "B12"),
                            EqualsToken(Token::QUERY_RIGHT_PARENTHESES, "")));
  }
}

}  // namespace
}  // namespace lib
}  // namespace icing
