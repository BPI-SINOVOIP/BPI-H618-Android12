/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "minikin/Font.h"

#include <gtest/gtest.h>

#include "BufferUtils.h"
#include "FontTestUtils.h"
#include "FreeTypeMinikinFontForTest.h"

namespace minikin {

TEST(FontTest, BufferTest) {
    auto minikinFont = std::make_shared<FreeTypeMinikinFontForTest>(getTestFontPath("Ascii.ttf"));
    std::shared_ptr<Font> original = Font::Builder(minikinFont).build();
    std::vector<uint8_t> buffer = writeToBuffer<Font, writeFreeTypeMinikinFontForTest>(*original);

    BufferReader reader(buffer.data());
    std::shared_ptr<Font> font =
            Font::readFrom<readFreeTypeMinikinFontForTest>(&reader, kEmptyLocaleListId);
    EXPECT_EQ(minikinFont->GetFontPath(), font->typeface()->GetFontPath());
    EXPECT_EQ(original->style(), font->style());
    EXPECT_NE(nullptr, font->baseFont());
    std::vector<uint8_t> newBuffer = writeToBuffer<Font, writeFreeTypeMinikinFontForTest>(*font);
    EXPECT_EQ(buffer, newBuffer);
}

}  // namespace minikin
