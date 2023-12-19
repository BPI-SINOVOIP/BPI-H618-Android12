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

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "icing/testing/common-matchers.h"
#include "icing/transform/normalizer-factory.h"
#include "icing/transform/normalizer.h"

namespace icing {
namespace lib {
namespace {

using ::testing::Eq;

TEST(NoneNormalizerTest, Creation) {
  EXPECT_THAT(normalizer_factory::Create(
                  /*max_term_byte_size=*/5),
              IsOk());
  EXPECT_THAT(normalizer_factory::Create(
                  /*max_term_byte_size=*/0),
              StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT));
  EXPECT_THAT(normalizer_factory::Create(
                  /*max_term_byte_size=*/-1),
              StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT));
}

TEST(IcuNormalizerTest, NoNormalizationDone) {
  ICING_ASSERT_OK_AND_ASSIGN(auto normalizer, normalizer_factory::Create(
                                                  /*max_term_byte_size=*/1000));
  EXPECT_THAT(normalizer->NormalizeTerm(""), Eq(""));
  EXPECT_THAT(normalizer->NormalizeTerm("hello world"), Eq("hello world"));

  // Capitalization
  EXPECT_THAT(normalizer->NormalizeTerm("MDI"), Eq("MDI"));

  // Accents
  EXPECT_THAT(normalizer->NormalizeTerm("Zürich"), Eq("Zürich"));

  // Full-width punctuation to ASCII punctuation
  EXPECT_THAT(normalizer->NormalizeTerm("。，！？：”"), Eq("。，！？：”"));

  // Half-width katakana
  EXPECT_THAT(normalizer->NormalizeTerm("ｶ"), Eq("ｶ"));
}

TEST(NoneNormalizerTest, Truncate) {
  ICING_ASSERT_OK_AND_ASSIGN(auto normalizer, normalizer_factory::Create(
                                                  /*max_term_byte_size=*/5));

  // Won't be truncated
  EXPECT_THAT(normalizer->NormalizeTerm("hi"), Eq("hi"));
  EXPECT_THAT(normalizer->NormalizeTerm("hello"), Eq("hello"));

  // Truncated to length 5.
  EXPECT_THAT(normalizer->NormalizeTerm("hello!"), Eq("hello"));
}

}  // namespace
}  // namespace lib
}  // namespace icing
