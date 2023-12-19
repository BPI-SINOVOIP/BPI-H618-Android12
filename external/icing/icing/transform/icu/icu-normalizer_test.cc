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
#include "icing/helpers/icu/icu-data-file-helper.h"
#include "icing/testing/common-matchers.h"
#include "icing/testing/icu-i18n-test-utils.h"
#include "icing/testing/test-data.h"
#include "icing/transform/normalizer-factory.h"
#include "icing/transform/normalizer.h"

namespace icing {
namespace lib {
namespace {
using ::testing::Eq;

class IcuNormalizerTest : public testing::Test {
 protected:
  void SetUp() override {
    ICING_ASSERT_OK(
        // File generated via icu_data_file rule in //icing/BUILD.
        icu_data_file_helper::SetUpICUDataFile(
            GetTestFilePath("icing/icu.dat")));

    ICING_ASSERT_OK_AND_ASSIGN(normalizer_, normalizer_factory::Create(
                                                /*max_term_byte_size=*/1024));
  }

  std::unique_ptr<Normalizer> normalizer_;
};

TEST_F(IcuNormalizerTest, Creation) {
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

// Strings that are already normalized won't change if normalized again.
TEST_F(IcuNormalizerTest, AlreadyNormalized) {
  EXPECT_THAT(normalizer_->NormalizeTerm(""), Eq(""));
  EXPECT_THAT(normalizer_->NormalizeTerm("hello world"), Eq("hello world"));
  EXPECT_THAT(normalizer_->NormalizeTerm("你好"), Eq("你好"));
  EXPECT_THAT(normalizer_->NormalizeTerm("キャンパス"), Eq("キャンパス"));
  EXPECT_THAT(normalizer_->NormalizeTerm("안녕하세요"), Eq("안녕하세요"));
}

TEST_F(IcuNormalizerTest, UppercaseToLowercase) {
  EXPECT_THAT(normalizer_->NormalizeTerm("MDI"), Eq("mdi"));
  EXPECT_THAT(normalizer_->NormalizeTerm("Icing"), Eq("icing"));
}

TEST_F(IcuNormalizerTest, LatinLetterRemoveAccent) {
  EXPECT_THAT(normalizer_->NormalizeTerm("Zürich"), Eq("zurich"));
  EXPECT_THAT(normalizer_->NormalizeTerm("après-midi"), Eq("apres-midi"));
  EXPECT_THAT(normalizer_->NormalizeTerm("Buenos días"), Eq("buenos dias"));
  EXPECT_THAT(normalizer_->NormalizeTerm("ÀÁÂÃÄÅĀĂĄḀḁàáâãäåāăą"),
              Eq("aaaaaaaaaaaaaaaaaaaa"));
  EXPECT_THAT(normalizer_->NormalizeTerm("ḂḄḆḃḅḇ"), Eq("bbbbbb"));
  EXPECT_THAT(normalizer_->NormalizeTerm("ÇĆĈĊČḈḉćĉċčç"), Eq("cccccccccccc"));
  EXPECT_THAT(normalizer_->NormalizeTerm("ÐĎĐḊḌḎḐḒḋḍḏḑḓďđ"),
              Eq("ddddddddddddddd"));
  EXPECT_THAT(normalizer_->NormalizeTerm("ÈÉÊËĒĔĖĘḔḖḘḚḜḕḗḙḛḝèéêëēĕėęě"),
              Eq("eeeeeeeeeeeeeeeeeeeeeeeeeee"));
  EXPECT_THAT(normalizer_->NormalizeTerm("Ḟḟ"), Eq("ff"));
  EXPECT_THAT(normalizer_->NormalizeTerm("ĜĞĠĢḠḡĝğġģ"), Eq("gggggggggg"));
  EXPECT_THAT(normalizer_->NormalizeTerm("ĤḢḤḦḨḪḣḥḧḩḫĥẖ"),
              Eq("hhhhhhhhhhhhh"));
  EXPECT_THAT(normalizer_->NormalizeTerm("ÌÍÎÏĨĪĬḬḭḯìíîïĩīĭ"),
              Eq("iiiiiiiiiiiiiiiii"));
  EXPECT_THAT(normalizer_->NormalizeTerm("Ĵĵ"), Eq("jj"));
  EXPECT_THAT(normalizer_->NormalizeTerm("ĶḰḲḴḵḱḳķ"), Eq("kkkkkkkk"));
  EXPECT_THAT(normalizer_->NormalizeTerm("ĹĻĽḶḸḼḷḹḻḽĺļľ"),
              Eq("lllllllllllll"));
  EXPECT_THAT(normalizer_->NormalizeTerm("ḾṀṂḿṁṃ"), Eq("mmmmmm"));
  EXPECT_THAT(normalizer_->NormalizeTerm("ÑŃŅŇṄṆṈṊṅṇṉṋñńņň"),
              Eq("nnnnnnnnnnnnnnnn"));
  EXPECT_THAT(normalizer_->NormalizeTerm("ŌŎŐÒÓÔÕÖṌṎṐṒṍṏṑṓòóôõöōŏő"),
              Eq("oooooooooooooooooooooooo"));
  EXPECT_THAT(normalizer_->NormalizeTerm("ṔṖṕṗ"), Eq("pppp"));
  EXPECT_THAT(normalizer_->NormalizeTerm("ŔŖŘṘṚṜṞṙṛṝṟŕŗř"),
              Eq("rrrrrrrrrrrrrr"));
  EXPECT_THAT(normalizer_->NormalizeTerm("ŚŜŞŠȘṠṢṤṦṨṡṣṥṧṩșśŝşš"),
              Eq("ssssssssssssssssssss"));
  EXPECT_THAT(normalizer_->NormalizeTerm("ŢŤȚṪṬṮṰṫṭṯṱțţť"),
              Eq("tttttttttttttt"));
  EXPECT_THAT(normalizer_->NormalizeTerm("ŨŪŬÙÚÛÜṲṴṶṸṺṳṵṷṹṻùúûüũūŭ"),
              Eq("uuuuuuuuuuuuuuuuuuuuuuuu"));
  EXPECT_THAT(normalizer_->NormalizeTerm("ṼṾṽṿ"), Eq("vvvv"));
  EXPECT_THAT(normalizer_->NormalizeTerm("ŴẀẂẄẆẈẁẃẅẇẉŵ"), Eq("wwwwwwwwwwww"));
  EXPECT_THAT(normalizer_->NormalizeTerm("ẊẌẋẍ"), Eq("xxxx"));
  EXPECT_THAT(normalizer_->NormalizeTerm("ÝŶŸẎẏŷýÿ"), Eq("yyyyyyyy"));
  EXPECT_THAT(normalizer_->NormalizeTerm("ŹŻŽẐẒẔẑẓẕźżž"),
              Eq("zzzzzzzzzzzz"));
}

// Accent / diacritic marks won't be removed in non-latin chars, e.g. in
// Japanese and Greek
TEST_F(IcuNormalizerTest, NonLatinLetterNotRemoveAccent) {
  // Katakana
  EXPECT_THAT(normalizer_->NormalizeTerm("ダヂヅデド"), Eq("ダヂヅデド"));
  // Greek
  EXPECT_THAT(normalizer_->NormalizeTerm("kαλημέρα"), Eq("kαλημέρα"));
  EXPECT_THAT(normalizer_->NormalizeTerm("εγγραφή"), Eq("εγγραφή"));

  // Our current ICU rules can't handle Hebrew properly, e.g. the accents in
  // "אָלֶף־בֵּית עִבְרִי"
  // will be removed.
}

TEST_F(IcuNormalizerTest, FullWidthCharsToASCII) {
  // Full-width punctuation to ASCII punctuation
  EXPECT_THAT(normalizer_->NormalizeTerm("‘’．，！？：“”"), Eq("''.,!?:\"\""));
  // Full-width 0-9
  EXPECT_THAT(normalizer_->NormalizeTerm("０１２３４５６７８９"),
              Eq("0123456789"));
  // Full-width A-Z
  EXPECT_THAT(normalizer_->NormalizeTerm(
                  "ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺ"),
              Eq("abcdefghijklmnopqrstuvwxyz"));
  // Full-width a-z
  EXPECT_THAT(normalizer_->NormalizeTerm(
                  "ａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚ"),
              Eq("abcdefghijklmnopqrstuvwxyz"));
}

TEST_F(IcuNormalizerTest, IdeographicToASCII) {
  ICING_ASSERT_OK_AND_ASSIGN(auto normalizer, normalizer_factory::Create(
                                                  /*max_term_byte_size=*/1000));

  EXPECT_THAT(normalizer->NormalizeTerm("，。"), Eq(",."));
}

// For Katakana, each character is normalized to its full-width version.
TEST_F(IcuNormalizerTest, KatakanaHalfWidthToFullWidth) {
  EXPECT_THAT(normalizer_->NormalizeTerm("ｶ"), Eq("カ"));
  EXPECT_THAT(normalizer_->NormalizeTerm("ｫ"), Eq("ォ"));
  EXPECT_THAT(normalizer_->NormalizeTerm("ｻ"), Eq("サ"));
  EXPECT_THAT(normalizer_->NormalizeTerm("ﾎ"), Eq("ホ"));
}

TEST_F(IcuNormalizerTest, HiraganaToKatakana) {
  EXPECT_THAT(normalizer_->NormalizeTerm("あいうえお"), Eq("アイウエオ"));
  EXPECT_THAT(normalizer_->NormalizeTerm("かきくけこ"), Eq("カキクケコ"));
  EXPECT_THAT(normalizer_->NormalizeTerm("さしすせそ"), Eq("サシスセソ"));
  EXPECT_THAT(normalizer_->NormalizeTerm("たちつてと"), Eq("タチツテト"));
  EXPECT_THAT(normalizer_->NormalizeTerm("なにぬねの"), Eq("ナニヌネノ"));
  EXPECT_THAT(normalizer_->NormalizeTerm("はひふへほ"), Eq("ハヒフヘホ"));
  EXPECT_THAT(normalizer_->NormalizeTerm("まみむめも"), Eq("マミムメモ"));
  EXPECT_THAT(normalizer_->NormalizeTerm("やゆよ"), Eq("ヤユヨ"));
  EXPECT_THAT(normalizer_->NormalizeTerm("らりるれろ"), Eq("ラリルレロ"));
  EXPECT_THAT(normalizer_->NormalizeTerm("わゐゑを"), Eq("ワヰヱヲ"));
  EXPECT_THAT(normalizer_->NormalizeTerm("ん"), Eq("ン"));
  EXPECT_THAT(normalizer_->NormalizeTerm("がぎぐげご"), Eq("ガギグゲゴ"));
  EXPECT_THAT(normalizer_->NormalizeTerm("ざじずぜぞ"), Eq("ザジズゼゾ"));
  EXPECT_THAT(normalizer_->NormalizeTerm("だぢづでど"), Eq("ダヂヅデド"));
  EXPECT_THAT(normalizer_->NormalizeTerm("ばびぶべぼ"), Eq("バビブベボ"));
  EXPECT_THAT(normalizer_->NormalizeTerm("ぱぴぷぺぽ"), Eq("パピプペポ"));
}

TEST_F(IcuNormalizerTest, SuperscriptAndSubscriptToASCII) {
  EXPECT_THAT(normalizer_->NormalizeTerm("⁹"), Eq("9"));
  EXPECT_THAT(normalizer_->NormalizeTerm("₉"), Eq("9"));
}

TEST_F(IcuNormalizerTest, CircledCharsToASCII) {
  EXPECT_THAT(normalizer_->NormalizeTerm("①"), Eq("1"));
  EXPECT_THAT(normalizer_->NormalizeTerm("Ⓐ"), Eq("a"));
}

TEST_F(IcuNormalizerTest, RotatedCharsToASCII) {
  EXPECT_THAT(normalizer_->NormalizeTerm("︷"), Eq("{"));
  EXPECT_THAT(normalizer_->NormalizeTerm("︸"), Eq("}"));
}

TEST_F(IcuNormalizerTest, SquaredCharsToASCII) {
  EXPECT_THAT(normalizer_->NormalizeTerm("㌀"), Eq("アパート"));
}

TEST_F(IcuNormalizerTest, FractionsToASCII) {
  EXPECT_THAT(normalizer_->NormalizeTerm("¼"), Eq(" 1/4"));
  EXPECT_THAT(normalizer_->NormalizeTerm("⅚"), Eq(" 5/6"));
}

TEST_F(IcuNormalizerTest, Truncate) {
  {
    ICING_ASSERT_OK_AND_ASSIGN(auto normalizer, normalizer_factory::Create(
                                                    /*max_term_byte_size=*/5));

    // Won't be truncated
    EXPECT_THAT(normalizer->NormalizeTerm("hi"), Eq("hi"));
    EXPECT_THAT(normalizer->NormalizeTerm("hello"), Eq("hello"));

    // Truncated to length 5.
    EXPECT_THAT(normalizer->NormalizeTerm("hello!"), Eq("hello"));

    // Each Japanese character has 3 bytes, so truncating to length 5 results in
    // only 1 character.
    EXPECT_THAT(normalizer->NormalizeTerm("キャンパス"), Eq("キ"));

    // Each Greek character has 2 bytes, so truncating to length 5 results in 2
    // character.
    EXPECT_THAT(normalizer->NormalizeTerm("αβγδε"), Eq("αβ"));
  }

  {
    ICING_ASSERT_OK_AND_ASSIGN(auto normalizer, normalizer_factory::Create(
                                                    /*max_term_byte_size=*/2));
    // The Japanese character has 3 bytes, truncating it results in an empty
    // string.
    EXPECT_THAT(normalizer->NormalizeTerm("キ"), Eq(""));
  }
}

}  // namespace
}  // namespace lib
}  // namespace icing
