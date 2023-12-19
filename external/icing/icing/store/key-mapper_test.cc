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

#include "icing/store/key-mapper.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "icing/store/document-id.h"
#include "icing/testing/common-matchers.h"
#include "icing/testing/tmp-directory.h"

using ::testing::_;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::Pair;
using ::testing::UnorderedElementsAre;

namespace icing {
namespace lib {
namespace {
constexpr int kMaxKeyMapperSize = 3 * 1024 * 1024;  // 3 MiB

class KeyMapperTest : public testing::Test {
 protected:
  void SetUp() override { base_dir_ = GetTestTempDir() + "/key_mapper"; }

  void TearDown() override {
    filesystem_.DeleteDirectoryRecursively(base_dir_.c_str());
  }

  std::string base_dir_;
  Filesystem filesystem_;
};

TEST_F(KeyMapperTest, InvalidBaseDir) {
  ASSERT_THAT(
      KeyMapper<DocumentId>::Create(filesystem_, "/dev/null", kMaxKeyMapperSize)
          .status()
          .error_message(),
      HasSubstr("Failed to create KeyMapper"));
}

TEST_F(KeyMapperTest, NegativeMaxKeyMapperSizeReturnsInternalError) {
  ASSERT_THAT(KeyMapper<DocumentId>::Create(filesystem_, base_dir_, -1),
              StatusIs(libtextclassifier3::StatusCode::INTERNAL));
}

TEST_F(KeyMapperTest, TooLargeMaxKeyMapperSizeReturnsInternalError) {
  ASSERT_THAT(KeyMapper<DocumentId>::Create(filesystem_, base_dir_,
                                            std::numeric_limits<int>::max()),
              StatusIs(libtextclassifier3::StatusCode::INTERNAL));
}

TEST_F(KeyMapperTest, CreateNewKeyMapper) {
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<KeyMapper<DocumentId>> key_mapper,
      KeyMapper<DocumentId>::Create(filesystem_, base_dir_, kMaxKeyMapperSize));
  EXPECT_THAT(key_mapper->num_keys(), 0);
}

TEST_F(KeyMapperTest, CanUpdateSameKeyMultipleTimes) {
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<KeyMapper<DocumentId>> key_mapper,
      KeyMapper<DocumentId>::Create(filesystem_, base_dir_, kMaxKeyMapperSize));

  ICING_EXPECT_OK(key_mapper->Put("default-google.com", 100));
  ICING_EXPECT_OK(key_mapper->Put("default-youtube.com", 50));

  EXPECT_THAT(key_mapper->Get("default-google.com"), IsOkAndHolds(100));

  ICING_EXPECT_OK(key_mapper->Put("default-google.com", 200));
  EXPECT_THAT(key_mapper->Get("default-google.com"), IsOkAndHolds(200));
  EXPECT_THAT(key_mapper->num_keys(), 2);

  ICING_EXPECT_OK(key_mapper->Put("default-google.com", 300));
  EXPECT_THAT(key_mapper->Get("default-google.com"), IsOkAndHolds(300));
  EXPECT_THAT(key_mapper->num_keys(), 2);
}

TEST_F(KeyMapperTest, GetOrPutOk) {
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<KeyMapper<DocumentId>> key_mapper,
      KeyMapper<DocumentId>::Create(filesystem_, base_dir_, kMaxKeyMapperSize));

  EXPECT_THAT(key_mapper->Get("foo"),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
  EXPECT_THAT(key_mapper->GetOrPut("foo", 1), IsOkAndHolds(1));
  EXPECT_THAT(key_mapper->Get("foo"), IsOkAndHolds(1));
}

TEST_F(KeyMapperTest, CanPersistToDiskRegularly) {
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<KeyMapper<DocumentId>> key_mapper,
      KeyMapper<DocumentId>::Create(filesystem_, base_dir_, kMaxKeyMapperSize));
  // Can persist an empty KeyMapper.
  ICING_EXPECT_OK(key_mapper->PersistToDisk());
  EXPECT_THAT(key_mapper->num_keys(), 0);

  // Can persist the smallest KeyMapper.
  ICING_EXPECT_OK(key_mapper->Put("default-google.com", 100));
  ICING_EXPECT_OK(key_mapper->PersistToDisk());
  EXPECT_THAT(key_mapper->num_keys(), 1);
  EXPECT_THAT(key_mapper->Get("default-google.com"), IsOkAndHolds(100));

  // Can continue to add keys after PersistToDisk().
  ICING_EXPECT_OK(key_mapper->Put("default-youtube.com", 200));
  EXPECT_THAT(key_mapper->num_keys(), 2);
  EXPECT_THAT(key_mapper->Get("default-youtube.com"), IsOkAndHolds(200));

  // Can continue to update the same key after PersistToDisk().
  ICING_EXPECT_OK(key_mapper->Put("default-google.com", 300));
  EXPECT_THAT(key_mapper->Get("default-google.com"), IsOkAndHolds(300));
  EXPECT_THAT(key_mapper->num_keys(), 2);
}

TEST_F(KeyMapperTest, CanUseAcrossMultipleInstances) {
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<KeyMapper<DocumentId>> key_mapper,
      KeyMapper<DocumentId>::Create(filesystem_, base_dir_, kMaxKeyMapperSize));
  ICING_EXPECT_OK(key_mapper->Put("default-google.com", 100));
  ICING_EXPECT_OK(key_mapper->PersistToDisk());

  key_mapper.reset();
  ICING_ASSERT_OK_AND_ASSIGN(
      key_mapper,
      KeyMapper<DocumentId>::Create(filesystem_, base_dir_, kMaxKeyMapperSize));
  EXPECT_THAT(key_mapper->num_keys(), 1);
  EXPECT_THAT(key_mapper->Get("default-google.com"), IsOkAndHolds(100));

  // Can continue to read/write to the KeyMapper.
  ICING_EXPECT_OK(key_mapper->Put("default-youtube.com", 200));
  ICING_EXPECT_OK(key_mapper->Put("default-google.com", 300));
  EXPECT_THAT(key_mapper->num_keys(), 2);
  EXPECT_THAT(key_mapper->Get("default-youtube.com"), IsOkAndHolds(200));
  EXPECT_THAT(key_mapper->Get("default-google.com"), IsOkAndHolds(300));
}

TEST_F(KeyMapperTest, CanDeleteAndRestartKeyMapping) {
  // Can delete even if there's nothing there
  ICING_EXPECT_OK(KeyMapper<DocumentId>::Delete(filesystem_, base_dir_));

  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<KeyMapper<DocumentId>> key_mapper,
      KeyMapper<DocumentId>::Create(filesystem_, base_dir_, kMaxKeyMapperSize));
  ICING_EXPECT_OK(key_mapper->Put("default-google.com", 100));
  ICING_EXPECT_OK(key_mapper->PersistToDisk());
  ICING_EXPECT_OK(KeyMapper<DocumentId>::Delete(filesystem_, base_dir_));

  key_mapper.reset();
  ICING_ASSERT_OK_AND_ASSIGN(
      key_mapper,
      KeyMapper<DocumentId>::Create(filesystem_, base_dir_, kMaxKeyMapperSize));
  EXPECT_THAT(key_mapper->num_keys(), 0);
  ICING_EXPECT_OK(key_mapper->Put("default-google.com", 100));
  EXPECT_THAT(key_mapper->num_keys(), 1);
}

TEST_F(KeyMapperTest, GetValuesToKeys) {
  ICING_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<KeyMapper<DocumentId>> key_mapper,
      KeyMapper<DocumentId>::Create(filesystem_, base_dir_, kMaxKeyMapperSize));
  EXPECT_THAT(key_mapper->GetValuesToKeys(), IsEmpty());

  ICING_EXPECT_OK(key_mapper->Put("foo", /*value=*/1));
  ICING_EXPECT_OK(key_mapper->Put("bar", /*value=*/2));
  EXPECT_THAT(key_mapper->GetValuesToKeys(),
              UnorderedElementsAre(Pair(1, "foo"), Pair(2, "bar")));

  ICING_EXPECT_OK(key_mapper->Put("baz", /*value=*/3));
  EXPECT_THAT(
      key_mapper->GetValuesToKeys(),
      UnorderedElementsAre(Pair(1, "foo"), Pair(2, "bar"), Pair(3, "baz")));
}

}  // namespace
}  // namespace lib
}  // namespace icing
