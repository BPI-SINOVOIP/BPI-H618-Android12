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

#include "icing/legacy/index/icing-dynamic-trie.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "icing/text_classifier/lib3/utils/hash/farmhash.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "icing/legacy/core/icing-string-util.h"
#include "icing/legacy/index/icing-filesystem.h"
#include "icing/testing/tmp-directory.h"

using testing::ElementsAre;

namespace icing {
namespace lib {

namespace {

constexpr std::string_view kKeys[] = {
    "", "ab", "ac", "abd", "bac", "bb", "bacd", "abbb", "abcdefg",
};
constexpr uint32_t kNumKeys = ABSL_ARRAYSIZE(kKeys);

class IcingDynamicTrieTest : public ::testing::Test {
 protected:
  class NewValueMap : public IcingDynamicTrie::NewValueMap {
   public:
    const void* GetNewValue(uint32_t old_value_index) const override {
      const_cast<NewValueMap*>(this)->buf_ = old_value_index;
      return &buf_;
    }

   private:
    uint32_t buf_;
  };

  // Output trie stats to stderr.
  static void StatsDump(const IcingDynamicTrie& trie) {
    IcingDynamicTrie::Stats stats;
    trie.CollectStats(&stats);
    DLOG(INFO) << "Stats:\n" << stats.DumpStats(true);
  }

  static void AddToTrie(IcingDynamicTrie* trie, uint32_t num_keys) {
    std::string key;
    for (uint32_t i = 0; i < kNumKeys; i++) {
      key.clear();
      IcingStringUtil::SStringAppendF(&key, 0, "%u+%010u", i % 2, i);
      bool inserted = trie->Insert(key.c_str(), &i);
      ASSERT_TRUE(inserted);
    }
  }

  static void CheckTrie(const IcingDynamicTrie& trie, uint32_t num_keys) {
    std::string key;
    for (uint32_t i = 0; i < kNumKeys; i++) {
      key.clear();
      IcingStringUtil::SStringAppendF(&key, 0, "%u+%010u", i % 2, i);
      uint32_t val;
      bool found = trie.Find(key.c_str(), &val);
      EXPECT_TRUE(found);
      EXPECT_EQ(i, val);
    }
  }

  static void PrintTrie(const IcingDynamicTrie& trie) {
    std::vector<std::string> keys;
    std::ostringstream os;
    DLOG(INFO) << "Trie:\n";
    trie.DumpTrie(&os, &keys);
    DLOG(INFO) << os.str();
  }

  void SetUp() override {
    trie_files_dir_ = GetTestTempDir() + "/trie_files";
    trie_files_prefix_ = trie_files_dir_ + "/test_file_";
  }

  void TearDown() override {
    IcingFilesystem filesystem;
    filesystem.DeleteDirectoryRecursively(trie_files_dir_.c_str());
  }

  std::string trie_files_dir_;
  std::string trie_files_prefix_;
};

constexpr std::string_view kCommonEnglishWords[] = {
    "that", "was",  "for",  "on",   "are",  "with",  "they", "be",    "at",
    "one",  "have", "this", "from", "word", "but",   "what", "some",  "you",
    "had",  "the",  "and",  "can",  "out",  "other", "were", "which", "their",
    "time", "will", "how",  "said", "each", "tell",  "may",  "three"};
constexpr uint32_t kCommonEnglishWordArrayLen =
    sizeof(kCommonEnglishWords) / sizeof(std::string_view);

TEST_F(IcingDynamicTrieTest, Simple) {
  // Test simple key insertions.
  IcingFilesystem filesystem;
  IcingDynamicTrie trie(trie_files_prefix_, IcingDynamicTrie::RuntimeOptions(),
                        &filesystem);
  ASSERT_TRUE(trie.CreateIfNotExist(IcingDynamicTrie::Options()));
  ASSERT_TRUE(trie.Init());

  for (uint32_t i = 0; i < kNumKeys; i++) {
    ASSERT_TRUE(trie.Insert(kKeys[i].data(), &i));

    uint32_t val;
    bool found = trie.Find(kKeys[i].data(), &val);
    EXPECT_TRUE(found) << kKeys[i];
    if (found) EXPECT_EQ(i, val) << kKeys[i] << " " << val;
  }

  EXPECT_EQ(trie.size(), kNumKeys);

  StatsDump(trie);
  std::vector<std::string> keys;
  std::ostringstream os;
  DLOG(INFO) << "Trie:\n";
  trie.DumpTrie(&os, &keys);
  DLOG(INFO) << os.str();
  EXPECT_EQ(keys.size(), kNumKeys);
}

TEST_F(IcingDynamicTrieTest, Init) {
  // Test create/init behavior.
  IcingFilesystem filesystem;
  IcingDynamicTrie trie(trie_files_prefix_, IcingDynamicTrie::RuntimeOptions(),
                        &filesystem);
  EXPECT_FALSE(trie.is_initialized());
  EXPECT_FALSE(trie.Init());

  ASSERT_TRUE(trie.CreateIfNotExist(IcingDynamicTrie::Options()));
  EXPECT_TRUE(trie.Init());
  EXPECT_TRUE(trie.is_initialized());
}

TEST_F(IcingDynamicTrieTest, Iterator) {
  // Test iterator.
  IcingFilesystem filesystem;
  uint32_t val;
  IcingDynamicTrie trie(trie_files_prefix_, IcingDynamicTrie::RuntimeOptions(),
                        &filesystem);
  ASSERT_TRUE(trie.CreateIfNotExist(IcingDynamicTrie::Options()));
  ASSERT_TRUE(trie.Init());

  for (uint32_t i = 0; i < kNumKeys; i++) {
    ASSERT_TRUE(trie.Insert(kKeys[i].data(), &i));
  }

  // We try everything twice to test that Reset also works.

  // Should get the entire trie.
  IcingDynamicTrie::Iterator it_all(trie, "");
  for (int i = 0; i < 2; i++) {
    uint32_t count = 0;
    for (; it_all.IsValid(); it_all.Advance()) {
      uint32_t val_idx = it_all.GetValueIndex();
      EXPECT_EQ(it_all.GetValue(), trie.GetValueAtIndex(val_idx));
      count++;
    }
    EXPECT_EQ(count, kNumKeys);
    it_all.Reset();
  }

  // Get everything under "a".
  IcingDynamicTrie::Iterator it1(trie, "a");
  for (int i = 0; i < 2; i++) {
    ASSERT_TRUE(it1.IsValid());
    EXPECT_STREQ(it1.GetKey(), "ab");
    static const uint32_t kOne = 1;
    ASSERT_TRUE(it1.GetValue() != nullptr);
    EXPECT_TRUE(!memcmp(it1.GetValue(), &kOne, sizeof(kOne)));

    ASSERT_TRUE(it1.Advance());
    ASSERT_TRUE(it1.IsValid());
    EXPECT_STREQ(it1.GetKey(), "abbb");

    ASSERT_TRUE(it1.Advance());
    ASSERT_TRUE(it1.IsValid());
    EXPECT_STREQ(it1.GetKey(), "abcdefg");

    ASSERT_TRUE(it1.Advance());
    ASSERT_TRUE(it1.IsValid());
    EXPECT_STREQ(it1.GetKey(), "abd");

    ASSERT_TRUE(it1.Advance());
    ASSERT_TRUE(it1.IsValid());
    EXPECT_STREQ(it1.GetKey(), "ac");

    EXPECT_FALSE(it1.Advance());
    EXPECT_FALSE(it1.IsValid());

    it1.Reset();
  }

  // Now "b".
  IcingDynamicTrie::Iterator it2(trie, "b");
  for (int i = 0; i < 2; i++) {
    ASSERT_TRUE(it2.IsValid());
    EXPECT_STREQ(it2.GetKey(), "bac");
    val = 1;
    ASSERT_TRUE(it1.GetValue() != nullptr);
    EXPECT_TRUE(!memcmp(it1.GetValue(), &val, sizeof(val)));
    val = 4;
    ASSERT_TRUE(it2.GetValue() != nullptr);
    EXPECT_TRUE(!memcmp(it2.GetValue(), &val, sizeof(val)));

    ASSERT_TRUE(it2.Advance());
    ASSERT_TRUE(it2.IsValid());
    EXPECT_STREQ(it2.GetKey(), "bacd");

    ASSERT_TRUE(it2.Advance());
    ASSERT_TRUE(it2.IsValid());
    EXPECT_STREQ(it2.GetKey(), "bb");

    EXPECT_FALSE(it2.Advance());
    EXPECT_FALSE(it2.IsValid());

    it2.Reset();
  }

  // Get everything under "ab".
  IcingDynamicTrie::Iterator it3(trie, "ab");
  for (int i = 0; i < 2; i++) {
    ASSERT_TRUE(it3.IsValid());
    EXPECT_STREQ(it3.GetKey(), "ab");
    val = 1;
    ASSERT_TRUE(it3.GetValue() != nullptr);
    EXPECT_TRUE(!memcmp(it3.GetValue(), &val, sizeof(val)));

    ASSERT_TRUE(it3.Advance());
    ASSERT_TRUE(it3.IsValid());
    EXPECT_STREQ(it3.GetKey(), "abbb");

    ASSERT_TRUE(it3.Advance());
    ASSERT_TRUE(it3.IsValid());
    EXPECT_STREQ(it3.GetKey(), "abcdefg");

    ASSERT_TRUE(it3.Advance());
    ASSERT_TRUE(it3.IsValid());
    EXPECT_STREQ(it3.GetKey(), "abd");

    EXPECT_FALSE(it3.Advance());
    EXPECT_FALSE(it3.IsValid());

    it3.Reset();
  }

  // Should match only one key exactly.
  constexpr std::string_view kOneMatch[] = {
      "abd",
      "abcd",
      "abcdef",
      "abcdefg",
  };
  // With the following match:
  constexpr std::string_view kOneMatchMatched[] = {
      "abd",
      "abcdefg",
      "abcdefg",
      "abcdefg",
  };

  for (size_t k = 0; k < ABSL_ARRAYSIZE(kOneMatch); k++) {
    IcingDynamicTrie::Iterator it_single(trie, kOneMatch[k].data());
    for (int i = 0; i < 2; i++) {
      ASSERT_TRUE(it_single.IsValid()) << kOneMatch[k];
      EXPECT_STREQ(it_single.GetKey(), kOneMatchMatched[k].data());
      EXPECT_FALSE(it_single.Advance()) << kOneMatch[k];
      EXPECT_FALSE(it_single.IsValid()) << kOneMatch[k];

      it_single.Reset();
    }
  }

  // Matches nothing.
  constexpr std::string_view kNoMatch[] = {
      "abbd",
      "abcdeg",
      "abcdefh",
  };
  for (size_t k = 0; k < ABSL_ARRAYSIZE(kNoMatch); k++) {
    IcingDynamicTrie::Iterator it_empty(trie, kNoMatch[k].data());
    for (int i = 0; i < 2; i++) {
      EXPECT_FALSE(it_empty.IsValid());

      it_empty.Reset();
    }
  }

  // Clear.
  trie.Clear();
  EXPECT_FALSE(IcingDynamicTrie::Iterator(trie, "").IsValid());
  EXPECT_EQ(0u, trie.size());
  EXPECT_EQ(1.0, trie.min_free_fraction());
}

TEST_F(IcingDynamicTrieTest, Persistence) {
  // Test persistence on the English dictionary.
  IcingFilesystem filesystem;
  {
    // Test with a trie including strings in words. Test will fail if
    // words are not unique.
    IcingDynamicTrie trie(trie_files_prefix_,
                          IcingDynamicTrie::RuntimeOptions(), &filesystem);
    EXPECT_FALSE(trie.Init());
    ASSERT_TRUE(trie.CreateIfNotExist(IcingDynamicTrie::Options()));
    ASSERT_TRUE(trie.Init());

    for (uint32_t i = 0; i < kCommonEnglishWordArrayLen; i++) {
      ASSERT_TRUE(trie.Insert(kCommonEnglishWords[i].data(), &i));
    }
    // Explicitly omit sync.

    StatsDump(trie);
  }

  {
    IcingDynamicTrie trie(trie_files_prefix_,
                          IcingDynamicTrie::RuntimeOptions(), &filesystem);
    ASSERT_TRUE(trie.Init());
    EXPECT_EQ(0U, trie.size());

    for (uint32_t i = 0; i < kCommonEnglishWordArrayLen; i++) {
      ASSERT_TRUE(trie.Insert(kCommonEnglishWords[i].data(), &i));
    }
    trie.Sync();

    StatsDump(trie);
  }

  {
    IcingDynamicTrie trie(trie_files_prefix_,
                          IcingDynamicTrie::RuntimeOptions(), &filesystem);
    ASSERT_TRUE(trie.Init());

    // Make sure we can find everything with the right value.
    uint32_t found_count = 0;
    uint32_t matched_count = 0;
    for (size_t i = 0; i < kCommonEnglishWordArrayLen; i++) {
      uint32_t val;
      bool found = trie.Find(kCommonEnglishWords[i].data(), &val);
      if (found) {
        found_count++;
        if (i == val) {
          matched_count++;
        }
      }
    }
    EXPECT_EQ(found_count, kCommonEnglishWordArrayLen);
    EXPECT_EQ(matched_count, kCommonEnglishWordArrayLen);

    StatsDump(trie);
  }
}

TEST_F(IcingDynamicTrieTest, PersistenceShared) {
  // Test persistence on the English dictionary.
  IcingFilesystem filesystem;
  IcingDynamicTrie::RuntimeOptions ropt;

  {
    // Test with a trie including strings in words. Test will fail if
    // words are not unique.
    ropt.storage_policy = IcingDynamicTrie::RuntimeOptions::kMapSharedWithCrc;
    IcingDynamicTrie trie(trie_files_prefix_, ropt, &filesystem);
    EXPECT_FALSE(trie.Init());
    ASSERT_TRUE(trie.CreateIfNotExist(IcingDynamicTrie::Options()));
    ASSERT_TRUE(trie.Init());

    uint32_t next_reopen = kCommonEnglishWordArrayLen / 16;
    for (uint32_t i = 0; i < kCommonEnglishWordArrayLen; i++) {
      ASSERT_TRUE(trie.Insert(kCommonEnglishWords[i].data(), &i));

      if (i == next_reopen) {
        ASSERT_NE(0u, trie.UpdateCrc());
        trie.Close();
        ASSERT_TRUE(trie.Init());

        next_reopen += next_reopen / 2;
      }
    }
    // Explicitly omit sync. Shared should automatically persist.

    StatsDump(trie);
  }

  // Go back and forth between the two policies.
  for (int i = 0; i < 5; i++) {
    if (i % 2 == 0) {
      DLOG(INFO) << "Opening with map shared";
      ropt.storage_policy = IcingDynamicTrie::RuntimeOptions::kMapSharedWithCrc;
    } else {
      DLOG(INFO) << "Opening with explicit flush";
      ropt.storage_policy = IcingDynamicTrie::RuntimeOptions::kExplicitFlush;
    }
    IcingDynamicTrie trie(trie_files_prefix_, ropt, &filesystem);
    ASSERT_TRUE(trie.Init());

    // Make sure we can find everything with the right value.
    uint32_t found_count = 0;
    uint32_t matched_count = 0;
    for (size_t i = 0; i < kCommonEnglishWordArrayLen; i++) {
      uint32_t val;
      bool found = trie.Find(kCommonEnglishWords[i].data(), &val);
      if (found) {
        found_count++;
        if (i == val) {
          matched_count++;
        }
      }
    }
    EXPECT_EQ(found_count, kCommonEnglishWordArrayLen);
    EXPECT_EQ(matched_count, kCommonEnglishWordArrayLen);

    StatsDump(trie);
  }

  // Clear and re-open.
  ropt.storage_policy = IcingDynamicTrie::RuntimeOptions::kMapSharedWithCrc;
  IcingDynamicTrie trie(trie_files_prefix_, ropt, &filesystem);
  ASSERT_TRUE(trie.Init());
  trie.Clear();
  trie.Close();
  ASSERT_TRUE(trie.Init());
}

TEST_F(IcingDynamicTrieTest, Sync) {
  IcingFilesystem filesystem;
  {
    IcingDynamicTrie trie(trie_files_prefix_,
                          IcingDynamicTrie::RuntimeOptions(), &filesystem);
    ASSERT_TRUE(trie.CreateIfNotExist(IcingDynamicTrie::Options()));
    ASSERT_TRUE(trie.Init());

    for (uint32_t i = 0; i < kNumKeys; i++) {
      ASSERT_TRUE(trie.Insert(kKeys[i].data(), &i));

      uint32_t val;
      bool found = trie.Find(kKeys[i].data(), &val);
      EXPECT_TRUE(found) << kKeys[i];
      if (found) EXPECT_EQ(i, val) << kKeys[i] << " " << val;
    }

    StatsDump(trie);
    PrintTrie(trie);

    trie.Sync();

    for (uint32_t i = 0; i < kNumKeys; i++) {
      uint32_t val;
      bool found = trie.Find(kKeys[i].data(), &val);
      EXPECT_TRUE(found) << kKeys[i];
      if (found) EXPECT_EQ(i, val) << kKeys[i] << " " << val;
    }
  }

  {
    IcingDynamicTrie trie(trie_files_prefix_,
                          IcingDynamicTrie::RuntimeOptions(), &filesystem);
    ASSERT_TRUE(trie.Init());

    for (uint32_t i = 0; i < kNumKeys; i++) {
      uint32_t val;
      bool found = trie.Find(kKeys[i].data(), &val);
      EXPECT_TRUE(found) << kKeys[i];
      if (found) EXPECT_EQ(i, val) << kKeys[i] << " " << val;
    }

    StatsDump(trie);
    PrintTrie(trie);
  }
}

TEST_F(IcingDynamicTrieTest, LimitsZero) {
  // Don't crash if we set limits to 0.
  IcingFilesystem filesystem;
  IcingDynamicTrie trie(trie_files_prefix_, IcingDynamicTrie::RuntimeOptions(),
                        &filesystem);
  ASSERT_FALSE(trie.CreateIfNotExist(IcingDynamicTrie::Options(0, 0, 0, 0)));
}

TEST_F(IcingDynamicTrieTest, LimitsSmall) {
  // Test limits with a few keys.
  IcingFilesystem filesystem;
  IcingDynamicTrie trie(trie_files_prefix_, IcingDynamicTrie::RuntimeOptions(),
                        &filesystem);
  ASSERT_TRUE(trie.CreateIfNotExist(
      IcingDynamicTrie::Options(10, 300, 30, sizeof(uint32_t))));
  ASSERT_TRUE(trie.Init());

  ASSERT_LT(3U, kNumKeys);

  for (uint32_t i = 0; i < 3; i++) {
    ASSERT_TRUE(trie.Insert(kKeys[i].data(), &i)) << i;

    uint32_t val;
    bool found = trie.Find(kKeys[i].data(), &val);
    EXPECT_TRUE(found) << kKeys[i];
    if (found) EXPECT_EQ(i, val) << kKeys[i] << " " << val;
  }

  uint32_t val = 3;
  EXPECT_FALSE(trie.Insert(kKeys[3].data(), &val));

  StatsDump(trie);
  PrintTrie(trie);
}

TEST_F(IcingDynamicTrieTest, DISABLEDFingerprintedKeys) {
  IcingFilesystem filesystem;
  IcingDynamicTrie::Options options(4 << 20, 4 << 20, 20 << 20,
                                    sizeof(uint32_t));
  IcingDynamicTrie trie(trie_files_prefix_, IcingDynamicTrie::RuntimeOptions(),
                        &filesystem);
  ASSERT_TRUE(trie.CreateIfNotExist(options));
  ASSERT_TRUE(trie.Init());
  IcingDynamicTrie triefp(trie_files_prefix_ + ".fps",
                          IcingDynamicTrie::RuntimeOptions(), &filesystem);
  ASSERT_TRUE(triefp.CreateIfNotExist(options));
  ASSERT_TRUE(triefp.Init());

  static const uint32_t kNumKeys = 1000000;
  std::string key;
  for (uint32_t i = 0; i < kNumKeys; i++) {
    key.clear();
    IcingStringUtil::SStringAppendF(
        &key, 1000, "content://gmail-ls/account/conversation/%u/message/%u", i,
        10 * i);
    ASSERT_TRUE(trie.Insert(key.c_str(), &i));

    // Now compute a fingerprint.
    uint64_t fpkey = tc3farmhash::Fingerprint64(key);

    // Convert to base255 since keys in trie cannot contain 0.
    uint8_t fpkey_base255[9];
    for (int j = 0; j < 8; j++) {
      fpkey_base255[j] = (fpkey % 255) + 1;
      fpkey /= 255;
    }
    fpkey_base255[8] = '\0';
    ASSERT_TRUE(
        triefp.Insert(reinterpret_cast<const char*>(fpkey_base255), &i));

    // Sync periodically to gauge write locality.
    if ((i + 1) % (kNumKeys / 10) == 0) {
      DLOG(INFO) << "Trie sync";
      trie.Sync();
      DLOG(INFO) << "Trie fp sync";
      triefp.Sync();
    }
  }

  DLOG(INFO) << "Trie stats";
  StatsDump(trie);
  DLOG(INFO) << "Trie fp stats";
  StatsDump(triefp);
}

TEST_F(IcingDynamicTrieTest, AddDups) {
  IcingFilesystem filesystem;
  IcingDynamicTrie trie(trie_files_prefix_, IcingDynamicTrie::RuntimeOptions(),
                        &filesystem);
  ASSERT_TRUE(trie.CreateIfNotExist(IcingDynamicTrie::Options()));
  ASSERT_TRUE(trie.Init());

  static const uint32_t kNumKeys = 5000;
  AddToTrie(&trie, kNumKeys);
  CheckTrie(trie, kNumKeys);

  DLOG(INFO) << "Trie stats";
  StatsDump(trie);

  AddToTrie(&trie, kNumKeys);
  CheckTrie(trie, kNumKeys);
  DLOG(INFO) << "Trie stats";
  StatsDump(trie);
}

TEST_F(IcingDynamicTrieTest, Properties) {
  IcingFilesystem filesystem;
  IcingDynamicTrie trie(trie_files_prefix_, IcingDynamicTrie::RuntimeOptions(),
                        &filesystem);
  ASSERT_TRUE(trie.CreateIfNotExist(IcingDynamicTrie::Options()));
  ASSERT_TRUE(trie.Init());

  static const uint32_t kOne = 1;
  uint32_t val_idx;
  trie.Insert("abcd", &kOne, &val_idx, false);
  trie.SetProperty(val_idx, 0);
  trie.SetProperty(val_idx, 3);

  {
    IcingDynamicTrie::PropertyReader reader(trie, 3);
    ASSERT_TRUE(reader.Exists());
    EXPECT_TRUE(reader.HasProperty(val_idx));
    EXPECT_FALSE(reader.HasProperty(1000));
  }

  // Disappear after close.
  trie.Close();
  ASSERT_TRUE(trie.Init());
  {
    IcingDynamicTrie::PropertyReader reader(trie, 3);
    EXPECT_FALSE(reader.HasProperty(val_idx));
  }

  // Persist after sync.
  trie.Insert("abcd", &kOne, &val_idx, false);
  trie.SetProperty(val_idx, 1);
  ASSERT_TRUE(trie.Sync());
  trie.Close();
  ASSERT_TRUE(trie.Init());

  uint32_t val;
  ASSERT_TRUE(trie.Find("abcd", &val, &val_idx));
  EXPECT_EQ(1u, val);
  {
    IcingDynamicTrie::PropertyReader reader(trie, 1);
    EXPECT_TRUE(reader.HasProperty(val_idx));
  }

  // Get all.
  {
    IcingDynamicTrie::PropertyReadersAll readers(trie);
    ASSERT_EQ(4u, readers.size());
    EXPECT_TRUE(readers.Exists(0));
    EXPECT_TRUE(readers.Exists(1));
    EXPECT_FALSE(readers.Exists(2));
    EXPECT_TRUE(readers.Exists(3));
  }
}

TEST_F(IcingDynamicTrieTest, ClearSingleProperty) {
  IcingFilesystem filesystem;
  IcingDynamicTrie trie(trie_files_prefix_, IcingDynamicTrie::RuntimeOptions(),
                        &filesystem);
  ASSERT_TRUE(trie.CreateIfNotExist(IcingDynamicTrie::Options()));
  ASSERT_TRUE(trie.Init());

  static const uint32_t kOne = 1;
  uint32_t val_idx[3];
  trie.Insert("abcd", &kOne, &val_idx[0], false);
  trie.SetProperty(val_idx[0], 0);
  trie.SetProperty(val_idx[0], 3);

  trie.Insert("efgh", &kOne, &val_idx[1], false);
  trie.SetProperty(val_idx[1], 0);
  trie.SetProperty(val_idx[1], 3);

  trie.Insert("ijkl", &kOne, &val_idx[2], false);
  trie.SetProperty(val_idx[2], 0);
  trie.SetProperty(val_idx[2], 3);

  {
    IcingDynamicTrie::PropertyReadersAll readers(trie);
    ASSERT_EQ(4u, readers.size());
    EXPECT_TRUE(readers.Exists(0));
    EXPECT_FALSE(readers.Exists(1));
    EXPECT_FALSE(readers.Exists(2));
    EXPECT_TRUE(readers.Exists(3));
    for (size_t i = 0; i < readers.size(); i++) {
      if (readers.Exists(i)) {
        for (size_t j = 0; j < sizeof(val_idx) / sizeof(uint32_t); ++j) {
          EXPECT_TRUE(readers.HasProperty(i, val_idx[j]));
        }
      }
    }
  }

  EXPECT_TRUE(trie.ClearPropertyForAllValues(3));

  {
    IcingDynamicTrie::PropertyReadersAll readers(trie);
    ASSERT_EQ(4u, readers.size());
    EXPECT_TRUE(readers.Exists(0));
    EXPECT_FALSE(readers.Exists(1));
    EXPECT_FALSE(readers.Exists(2));
    // Clearing the property causes all values to be deleted.
    EXPECT_FALSE(readers.Exists(3));
    for (size_t i = 0; i < readers.size(); i++) {
      for (size_t j = 0; j < sizeof(val_idx) / sizeof(uint32_t); ++j) {
        if (i == 0) {
          EXPECT_TRUE(readers.HasProperty(i, val_idx[j]));
        } else {
          EXPECT_FALSE(readers.HasProperty(i, val_idx[j]));
        }
      }
    }
  }
}

TEST_F(IcingDynamicTrieTest, Compact) {
  IcingFilesystem filesystem;
  IcingDynamicTrie trie(trie_files_prefix_, IcingDynamicTrie::RuntimeOptions(),
                        &filesystem);
  ASSERT_TRUE(trie.CreateIfNotExist(IcingDynamicTrie::Options()));
  ASSERT_TRUE(trie.Init());

  for (uint32_t i = 0; i < kNumKeys; i++) {
    ASSERT_TRUE(trie.Insert(kKeys[i].data(), &i));

    uint32_t val;
    bool found = trie.Find(kKeys[i].data(), &val);
    EXPECT_TRUE(found) << kKeys[i];
    if (found) EXPECT_EQ(i, val) << kKeys[i] << " " << val;
  }

  EXPECT_EQ(trie.size(), kNumKeys);

  StatsDump(trie);
  PrintTrie(trie);

  IcingDynamicTrie trie2(trie_files_prefix_ + "-2",
                         IcingDynamicTrie::RuntimeOptions(), &filesystem);
  ASSERT_TRUE(trie2.CreateIfNotExist(IcingDynamicTrie::Options()));
  ASSERT_TRUE(trie2.Init());

  std::unordered_map<uint32_t, uint32_t> old_to_new_tvi;
  trie.Compact(NewValueMap(), &trie2, &old_to_new_tvi);
  for (uint32_t i = 0; i < kNumKeys; i++) {
    uint32_t val;
    bool found = trie2.Find(kKeys[i].data(), &val);
    EXPECT_TRUE(found) << kKeys[i];
    EXPECT_TRUE(old_to_new_tvi.find(val) != old_to_new_tvi.end());
  }
}

TEST_F(IcingDynamicTrieTest, DeletionShouldWorkWhenRootIsLeaf) {
  IcingFilesystem filesystem;
  IcingDynamicTrie trie(trie_files_prefix_, IcingDynamicTrie::RuntimeOptions(),
                        &filesystem);
  ASSERT_TRUE(trie.CreateIfNotExist(IcingDynamicTrie::Options()));
  ASSERT_TRUE(trie.Init());

  // Inserts a key, the root is a leaf.
  uint32_t value = 1;
  ASSERT_TRUE(trie.Insert("foo", &value));
  ASSERT_TRUE(trie.Find("foo", &value));

  // Deletes the key.
  EXPECT_TRUE(trie.Delete("foo"));
  EXPECT_FALSE(trie.Find("foo", &value));
}

TEST_F(IcingDynamicTrieTest, DeletionShouldWorkWhenLastCharIsLeaf) {
  IcingFilesystem filesystem;
  IcingDynamicTrie trie(trie_files_prefix_, IcingDynamicTrie::RuntimeOptions(),
                        &filesystem);
  ASSERT_TRUE(trie.CreateIfNotExist(IcingDynamicTrie::Options()));
  ASSERT_TRUE(trie.Init());

  // Inserts "bar" and "ba", the trie structure looks like:
  //       root
  //         |
  //         b
  //         |
  //         a
  //        / \
  //     null  r
  uint32_t value = 1;
  ASSERT_TRUE(trie.Insert("bar", &value));
  ASSERT_TRUE(trie.Insert("ba", &value));
  ASSERT_TRUE(trie.Find("bar", &value));
  ASSERT_TRUE(trie.Find("ba", &value));

  // Deletes "bar". "r" is a leaf node in the trie.
  EXPECT_TRUE(trie.Delete("bar"));
  EXPECT_FALSE(trie.Find("bar", &value));
  EXPECT_TRUE(trie.Find("ba", &value));
}

TEST_F(IcingDynamicTrieTest, DeletionShouldWorkWithTerminationNode) {
  IcingFilesystem filesystem;
  IcingDynamicTrie trie(trie_files_prefix_, IcingDynamicTrie::RuntimeOptions(),
                        &filesystem);
  ASSERT_TRUE(trie.CreateIfNotExist(IcingDynamicTrie::Options()));
  ASSERT_TRUE(trie.Init());

  // Inserts "bar" and "ba", the trie structure looks like:
  //       root
  //         |
  //         b
  //         |
  //         a
  //        / \
  //     null  r
  uint32_t value = 1;
  ASSERT_TRUE(trie.Insert("bar", &value));
  ASSERT_TRUE(trie.Insert("ba", &value));
  ASSERT_TRUE(trie.Find("bar", &value));
  ASSERT_TRUE(trie.Find("ba", &value));

  // Deletes "ba" which is a key with termination node in the trie.
  EXPECT_TRUE(trie.Delete("ba"));
  EXPECT_FALSE(trie.Find("ba", &value));
  EXPECT_TRUE(trie.Find("bar", &value));
}

TEST_F(IcingDynamicTrieTest, DeletionShouldWorkWithMultipleNexts) {
  IcingFilesystem filesystem;
  IcingDynamicTrie trie(trie_files_prefix_, IcingDynamicTrie::RuntimeOptions(),
                        &filesystem);
  ASSERT_TRUE(trie.CreateIfNotExist(IcingDynamicTrie::Options()));
  ASSERT_TRUE(trie.Init());

  // Inserts "ba", "bb", "bc", and "bd", the trie structure looks like:
  //       root
  //         |
  //         b
  //      / | | \
  //     a  b c  d
  uint32_t value = 1;
  ASSERT_TRUE(trie.Insert("ba", &value));
  ASSERT_TRUE(trie.Insert("bb", &value));
  ASSERT_TRUE(trie.Insert("bc", &value));
  ASSERT_TRUE(trie.Insert("bd", &value));
  ASSERT_TRUE(trie.Find("ba", &value));
  ASSERT_TRUE(trie.Find("bb", &value));
  ASSERT_TRUE(trie.Find("bc", &value));
  ASSERT_TRUE(trie.Find("bd", &value));

  // Deletes "bc".
  EXPECT_TRUE(trie.Delete("bc"));
  EXPECT_FALSE(trie.Find("bc", &value));
  EXPECT_TRUE(trie.Find("ba", &value));
  EXPECT_TRUE(trie.Find("bb", &value));
  EXPECT_TRUE(trie.Find("bd", &value));
}

TEST_F(IcingDynamicTrieTest, DeletionShouldWorkWithMultipleTrieBranches) {
  IcingFilesystem filesystem;
  IcingDynamicTrie trie(trie_files_prefix_, IcingDynamicTrie::RuntimeOptions(),
                        &filesystem);
  ASSERT_TRUE(trie.CreateIfNotExist(IcingDynamicTrie::Options()));
  ASSERT_TRUE(trie.Init());

  // Inserts "batter", "battle", and "bar", the trie structure looks like:
  //       root
  //         |
  //         b
  //         |
  //         a
  //        / \
  //       t   r
  //       |
  //       t
  //      / \
  //     e   l
  //     |   |
  //     r   e
  uint32_t value = 1;
  ASSERT_TRUE(trie.Insert("batter", &value));
  ASSERT_TRUE(trie.Insert("battle", &value));
  ASSERT_TRUE(trie.Insert("bar", &value));
  ASSERT_TRUE(trie.Find("batter", &value));
  ASSERT_TRUE(trie.Find("battle", &value));
  ASSERT_TRUE(trie.Find("bar", &value));

  // Deletes "batter".
  EXPECT_TRUE(trie.Delete("batter"));
  EXPECT_FALSE(trie.Find("batter", &value));
  EXPECT_TRUE(trie.Find("battle", &value));
  EXPECT_TRUE(trie.Find("bar", &value));
}

TEST_F(IcingDynamicTrieTest, InsertionShouldWorkAfterDeletion) {
  IcingFilesystem filesystem;
  IcingDynamicTrie trie(trie_files_prefix_, IcingDynamicTrie::RuntimeOptions(),
                        &filesystem);
  ASSERT_TRUE(trie.CreateIfNotExist(IcingDynamicTrie::Options()));
  ASSERT_TRUE(trie.Init());

  // Inserts some keys.
  uint32_t value = 1;
  ASSERT_TRUE(trie.Insert("bar", &value));
  ASSERT_TRUE(trie.Insert("bed", &value));
  ASSERT_TRUE(trie.Insert("foo", &value));

  // Deletes a key
  ASSERT_TRUE(trie.Delete("bed"));
  ASSERT_FALSE(trie.Find("bed", &value));

  // Inserts after deletion
  EXPECT_TRUE(trie.Insert("bed", &value));
  EXPECT_TRUE(trie.Insert("bedroom", &value));
  EXPECT_TRUE(trie.Find("bed", &value));
  EXPECT_TRUE(trie.Find("bedroom", &value));
}

TEST_F(IcingDynamicTrieTest, IteratorShouldWorkAfterDeletion) {
  IcingFilesystem filesystem;
  IcingDynamicTrie trie(trie_files_prefix_, IcingDynamicTrie::RuntimeOptions(),
                        &filesystem);
  ASSERT_TRUE(trie.CreateIfNotExist(IcingDynamicTrie::Options()));
  ASSERT_TRUE(trie.Init());

  // Inserts some keys.
  uint32_t value = 1;
  ASSERT_TRUE(trie.Insert("bar", &value));
  ASSERT_TRUE(trie.Insert("bed", &value));
  ASSERT_TRUE(trie.Insert("foo", &value));

  // Deletes a key
  ASSERT_TRUE(trie.Delete("bed"));

  // Iterates through all keys
  IcingDynamicTrie::Iterator iterator_all(trie, "");
  std::vector<std::string> results;
  for (; iterator_all.IsValid(); iterator_all.Advance()) {
    results.emplace_back(iterator_all.GetKey());
  }
  EXPECT_THAT(results, ElementsAre("bar", "foo"));

  // Iterates through keys that start with "b"
  IcingDynamicTrie::Iterator iterator_b(trie, "b");
  results.clear();
  for (; iterator_b.IsValid(); iterator_b.Advance()) {
    results.emplace_back(iterator_b.GetKey());
  }
  EXPECT_THAT(results, ElementsAre("bar"));
}

TEST_F(IcingDynamicTrieTest, DeletingNonExistingKeyShouldReturnTrue) {
  IcingFilesystem filesystem;
  IcingDynamicTrie trie(trie_files_prefix_, IcingDynamicTrie::RuntimeOptions(),
                        &filesystem);
  ASSERT_TRUE(trie.CreateIfNotExist(IcingDynamicTrie::Options()));
  ASSERT_TRUE(trie.Init());

  // Inserts some keys.
  uint32_t value = 1;
  ASSERT_TRUE(trie.Insert("bar", &value));
  ASSERT_TRUE(trie.Insert("bed", &value));

  // "ba" and bedroom are not keys in the trie.
  EXPECT_TRUE(trie.Delete("ba"));
  EXPECT_TRUE(trie.Delete("bedroom"));

  // The original keys are not affected.
  EXPECT_TRUE(trie.Find("bar", &value));
  EXPECT_TRUE(trie.Find("bed", &value));
}

}  // namespace

// The tests below are accessing private methods and fields of IcingDynamicTrie
// so can't be in the anonymous namespace.

TEST_F(IcingDynamicTrieTest, TrieShouldRespectLimits) {
  // Test limits on numbers of nodes, nexts, and suffixes size.
  IcingFilesystem filesystem;

  // These 3 numbers are the entities we need in order to insert all the test
  // words before the last one.
  uint32_t num_nodes_enough;
  uint32_t num_nexts_enough;
  uint32_t suffixes_size_enough;

  // First, try to fill the 3 numbers above.
  {
    IcingDynamicTrie trie(trie_files_prefix_,
                          IcingDynamicTrie::RuntimeOptions(), &filesystem);
    ASSERT_TRUE(trie.Remove());
    // Creates a trie with enough numbers of nodes, nexts, and suffix file size.
    ASSERT_TRUE(trie.CreateIfNotExist(IcingDynamicTrie::Options(
        /*max_nodes_in=*/1000, /*max_nexts_in=*/1000,
        /*max_suffixes_size_in=*/1000, sizeof(uint32_t))));
    ASSERT_TRUE(trie.Init());

    // Inserts all the test words before the last one.
    uint32_t value = 0;
    for (size_t i = 0; i < kCommonEnglishWordArrayLen - 1; ++i) {
      ASSERT_TRUE(trie.Insert(kCommonEnglishWords[i].data(), &value));
    }

    IcingDynamicTrieHeader header;
    trie.GetHeader(&header);

    // Before each insertion, it requires that there're (2 + 1 + key_length)
    // nodes left, so we need 8 nodes to insert the last word. +7 here will make
    // it just enough to insert the word before the last one.
    num_nodes_enough = header.num_nodes() + 7;

    // Before each insertion, it requires that there're (2 + 1 + key_length +
    // kMaxNextArraySize) nexts left, so we need (8 + kMaxNextArraySize) nexts
    // to insert the last word. (7 + kMaxNextArraySize) here will make it just
    // enough to insert the word before the last one.
    num_nexts_enough =
        header.num_nexts() + 7 + IcingDynamicTrie::kMaxNextArraySize;

    // Before each insertion, it requires that there're (1 + key_length +
    // value_size) bytes left for suffixes, so we need (6 + sizeof(uint32_t))
    // bytes to insert the last word. (5 + sizeof(uint32_t)) here will make it
    // just enough to insert the word before the last one.
    suffixes_size_enough = header.suffixes_size() + 5 + sizeof(uint32_t);
  }

  // Test a trie with just enough number of nodes.
  {
    IcingDynamicTrie trie(trie_files_prefix_,
                          IcingDynamicTrie::RuntimeOptions(), &filesystem);
    ASSERT_TRUE(trie.Remove());
    ASSERT_TRUE(trie.CreateIfNotExist(IcingDynamicTrie::Options(
        num_nodes_enough, /*max_nexts_in=*/1000,
        /*max_suffixes_size_in=*/1000, sizeof(uint32_t))));
    ASSERT_TRUE(trie.Init());

    // Inserts all the test words before the last one.
    uint32_t value = 0;
    for (size_t i = 0; i < kCommonEnglishWordArrayLen - 1; ++i) {
      ASSERT_TRUE(trie.Insert(kCommonEnglishWords[i].data(), &value));
    }

    // Fails to insert the last word because no enough nodes left.
    EXPECT_FALSE(trie.Insert(
        kCommonEnglishWords[kCommonEnglishWordArrayLen - 1].data(), &value));
  }

  // Test a trie with just enough number of nexts.
  {
    IcingDynamicTrie trie(trie_files_prefix_,
                          IcingDynamicTrie::RuntimeOptions(), &filesystem);
    ASSERT_TRUE(trie.Remove());
    ASSERT_TRUE(trie.CreateIfNotExist(IcingDynamicTrie::Options(
        /*max_nodes_in=*/1000, num_nexts_enough,
        /*max_suffixes_size_in=*/1000, sizeof(uint32_t))));
    ASSERT_TRUE(trie.Init());

    // Inserts all the test words before the last one.
    uint32_t value = 0;
    for (size_t i = 0; i < kCommonEnglishWordArrayLen - 1; ++i) {
      ASSERT_TRUE(trie.Insert(kCommonEnglishWords[i].data(), &value));
    }

    // Fails to insert the last word because no enough nexts left.
    EXPECT_FALSE(trie.Insert(
        kCommonEnglishWords[kCommonEnglishWordArrayLen - 1].data(), &value));
  }

  // Test a trie with just enough suffixes size.
  {
    IcingDynamicTrie trie(trie_files_prefix_,
                          IcingDynamicTrie::RuntimeOptions(), &filesystem);
    ASSERT_TRUE(trie.Remove());
    ASSERT_TRUE(trie.CreateIfNotExist(IcingDynamicTrie::Options(
        /*max_nodes_in=*/1000, /*max_nexts_in=*/1000, suffixes_size_enough,
        sizeof(uint32_t))));
    ASSERT_TRUE(trie.Init());

    // Inserts all the test words before the last one.
    uint32_t value = 0;
    for (size_t i = 0; i < kCommonEnglishWordArrayLen - 1; ++i) {
      ASSERT_TRUE(trie.Insert(kCommonEnglishWords[i].data(), &value));
    }

    // Fails to insert the last word because no enough space for more suffixes.
    EXPECT_FALSE(trie.Insert(
        kCommonEnglishWords[kCommonEnglishWordArrayLen - 1].data(), &value));
  }
}

TEST_F(IcingDynamicTrieTest, SyncErrorRecovery) {
  IcingFilesystem filesystem;
  IcingDynamicTrie trie(trie_files_prefix_, IcingDynamicTrie::RuntimeOptions(),
                        &filesystem);
  ASSERT_TRUE(trie.CreateIfNotExist(IcingDynamicTrie::Options()));
  ASSERT_TRUE(trie.Init());

  static const uint32_t kNumKeys = 5000;
  AddToTrie(&trie, kNumKeys);
  CheckTrie(trie, kNumKeys);

  trie.Sync();
  trie.Close();

  // Reach into the file and set the value_size.
  ASSERT_TRUE(trie.Init());
  IcingDynamicTrieHeader hdr;
  trie.GetHeader(&hdr);
  hdr.set_value_size(hdr.value_size() + 123);
  trie.SetHeader(hdr);
  trie.Close();

  ASSERT_FALSE(trie.Init());
}

TEST_F(IcingDynamicTrieTest, BitmapsClosedWhenInitFails) {
  // Create trie with one property.
  IcingFilesystem filesystem;
  IcingDynamicTrie trie(
      trie_files_prefix_,
      IcingDynamicTrie::RuntimeOptions().set_storage_policy(
          IcingDynamicTrie::RuntimeOptions::kMapSharedWithCrc),
      &filesystem);
  ASSERT_TRUE(trie.CreateIfNotExist(IcingDynamicTrie::Options()));
  ASSERT_TRUE(trie.Init());
  ASSERT_TRUE(trie.deleted_bitmap_);
  trie.SetProperty(0, 0);
  ASSERT_EQ(1, trie.property_bitmaps_.size());
  ASSERT_TRUE(trie.property_bitmaps_[0]);
  trie.Close();

  // Intentionally corrupt deleted_bitmap file to make Init() fail.
  FILE* fp = fopen(trie.deleted_bitmap_filename_.c_str(), "r+");
  ASSERT_TRUE(fp);
  ASSERT_EQ(16, fwrite("################", 1, 16, fp));
  fclose(fp);
  ASSERT_FALSE(trie.Init());

  // Check that both the bitmap and the property files have been closed.
  ASSERT_FALSE(trie.deleted_bitmap_);
  ASSERT_EQ(0, trie.property_bitmaps_.size());
}

}  // namespace lib
}  // namespace icing
