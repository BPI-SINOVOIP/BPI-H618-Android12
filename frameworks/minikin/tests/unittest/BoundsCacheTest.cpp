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

#include <gtest/gtest.h>

#include "minikin/BoundsCache.h"

#include "FontTestUtils.h"
#include "LocaleListCache.h"
#include "UnicodeUtils.h"

namespace minikin {

class TestableBoundsCache : public BoundsCache {
public:
    TestableBoundsCache(uint32_t maxEntries) : BoundsCache(maxEntries) {}
};

class BoundsCapture {
public:
    BoundsCapture() {}

    void operator()(const MinikinRect& rect, float advance) {
        mRect = rect;
        mAdvance = advance;
    }

    const MinikinRect& rect() const { return mRect; }
    float advance() const { return mAdvance; }

private:
    MinikinRect mRect;
    float mAdvance;
};

TEST(BoundsCacheTest, cacheHitTest) {
    auto text = utf8ToUtf16("android");
    Range range(0, text.size());
    MinikinPaint paint(buildFontCollection("Ascii.ttf"));

    TestableBoundsCache boundsCache(10);

    BoundsCapture bounds1;
    boundsCache.getOrCreate(text, range, paint, false /* LTR */, StartHyphenEdit::NO_EDIT,
                            EndHyphenEdit::NO_EDIT, bounds1);

    BoundsCapture bounds2;
    boundsCache.getOrCreate(text, range, paint, false /* LTR */, StartHyphenEdit::NO_EDIT,
                            EndHyphenEdit::NO_EDIT, bounds2);

    EXPECT_EQ(bounds1.rect(), bounds2.rect());
    EXPECT_EQ(bounds1.advance(), bounds2.advance());
}

TEST(BoundsCacheTest, cacheMissTest) {
    auto text1 = utf8ToUtf16("android");
    auto text2 = utf8ToUtf16("αβγδζ");
    MinikinPaint paint(buildFontCollection("Ascii.ttf"));

    TestableBoundsCache boundsCache(10);

    BoundsCapture bounds1;
    BoundsCapture bounds2;

    {
        SCOPED_TRACE("Different text");
        boundsCache.getOrCreate(text1, Range(0, text1.size()), paint, false /* LTR */,
                                StartHyphenEdit::NO_EDIT, EndHyphenEdit::NO_EDIT, bounds1);
        boundsCache.getOrCreate(text2, Range(0, text2.size()), paint, false /* LTR */,
                                StartHyphenEdit::NO_EDIT, EndHyphenEdit::NO_EDIT, bounds2);
        EXPECT_NE(bounds1.rect(), bounds2.rect());
        EXPECT_NE(bounds1.advance(), bounds2.advance());
    }
    {
        SCOPED_TRACE("Different range");
        boundsCache.getOrCreate(text1, Range(0, text1.size()), paint, false /* LTR */,
                                StartHyphenEdit::NO_EDIT, EndHyphenEdit::NO_EDIT, bounds1);
        boundsCache.getOrCreate(text1, Range(1, text1.size()), paint, false /* LTR */,
                                StartHyphenEdit::NO_EDIT, EndHyphenEdit::NO_EDIT, bounds2);
        EXPECT_NE(bounds1.rect(), bounds2.rect());
        EXPECT_NE(bounds1.advance(), bounds2.advance());
    }
    {
        SCOPED_TRACE("Different collection");
        MinikinPaint paint1(buildFontCollection("Ascii.ttf"));
        paint1.size = 10.0f;
        paint1.scaleX = 1.0f;
        boundsCache.getOrCreate(text1, Range(0, text1.size()), paint1, false /* LTR */,
                                StartHyphenEdit::NO_EDIT, EndHyphenEdit::NO_EDIT, bounds1);
        MinikinPaint paint2(buildFontCollection("Emoji.ttf"));
        paint2.size = 10.0f;
        paint2.scaleX = 1.0f;
        boundsCache.getOrCreate(text1, Range(0, text1.size()), paint2, false /* LTR */,
                                StartHyphenEdit::NO_EDIT, EndHyphenEdit::NO_EDIT, bounds2);
        EXPECT_NE(bounds1.rect(), bounds2.rect());
        EXPECT_NE(bounds1.advance(), bounds2.advance());
    }
    {
        SCOPED_TRACE("Different size");
        auto collection = buildFontCollection("Ascii.ttf");
        MinikinPaint paint1(collection);
        paint1.size = 10.0f;
        paint1.scaleX = 1.0f;
        boundsCache.getOrCreate(text1, Range(0, text1.size()), paint1, false /* LTR */,
                                StartHyphenEdit::NO_EDIT, EndHyphenEdit::NO_EDIT, bounds1);
        MinikinPaint paint2(collection);
        paint2.size = 20.0f;
        paint2.scaleX = 1.0f;
        boundsCache.getOrCreate(text1, Range(0, text1.size()), paint2, false /* LTR */,
                                StartHyphenEdit::NO_EDIT, EndHyphenEdit::NO_EDIT, bounds2);
        EXPECT_NE(bounds1.rect(), bounds2.rect());
        EXPECT_NE(bounds1.advance(), bounds2.advance());
    }
    {
        SCOPED_TRACE("Different letter spacing");
        auto collection = buildFontCollection("Ascii.ttf");
        MinikinPaint paint1(collection);
        paint1.letterSpacing = 0.0f;
        paint1.size = 10.0f;
        paint1.scaleX = 1.0f;
        boundsCache.getOrCreate(text1, Range(0, text1.size()), paint1, false /* LTR */,
                                StartHyphenEdit::NO_EDIT, EndHyphenEdit::NO_EDIT, bounds1);
        MinikinPaint paint2(collection);
        paint2.letterSpacing = 1.0f;
        paint2.size = 10.0f;
        paint2.scaleX = 1.0f;
        boundsCache.getOrCreate(text1, Range(0, text1.size()), paint2, false /* LTR */,
                                StartHyphenEdit::NO_EDIT, EndHyphenEdit::NO_EDIT, bounds2);
        EXPECT_NE(bounds1.rect(), bounds2.rect());
        EXPECT_NE(bounds1.advance(), bounds2.advance());
    }
}

TEST(BoundsCacheTest, cacheOverflowTest) {
    auto text = utf8ToUtf16("android");
    Range range(0, text.size());
    MinikinPaint paint(buildFontCollection("Ascii.ttf"));

    TestableBoundsCache boundsCache(5);

    BoundsCapture bounds1;
    boundsCache.getOrCreate(text, range, paint, false /* LTR */, StartHyphenEdit::NO_EDIT,
                            EndHyphenEdit::NO_EDIT, bounds1);

    for (char c = 'a'; c <= 'z'; c++) {
        auto text1 = utf8ToUtf16(std::string(10, c));
        BoundsCapture bounds2;
        boundsCache.getOrCreate(text1, Range(0, text1.size()), paint, false /* LTR */,
                                StartHyphenEdit::NO_EDIT, EndHyphenEdit::NO_EDIT, bounds2);
    }

    BoundsCapture bounds3;
    boundsCache.getOrCreate(text, range, paint, false /* LTR */, StartHyphenEdit::NO_EDIT,
                            EndHyphenEdit::NO_EDIT, bounds3);
    EXPECT_EQ(bounds1.rect(), bounds3.rect());
    EXPECT_EQ(bounds1.advance(), bounds3.advance());
}

}  // namespace minikin
