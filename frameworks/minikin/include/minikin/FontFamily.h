/*
 * Copyright (C) 2013 The Android Open Source Project
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

#ifndef MINIKIN_FONT_FAMILY_H
#define MINIKIN_FONT_FAMILY_H

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "minikin/FamilyVariant.h"
#include "minikin/Font.h"
#include "minikin/FontStyle.h"
#include "minikin/HbUtils.h"
#include "minikin/Macros.h"
#include "minikin/SparseBitSet.h"

namespace minikin {

class FontFamily {
public:
    explicit FontFamily(std::vector<std::shared_ptr<Font>>&& fonts);
    FontFamily(FamilyVariant variant, std::vector<std::shared_ptr<Font>>&& fonts);
    FontFamily(uint32_t localeListId, FamilyVariant variant,
               std::vector<std::shared_ptr<Font>>&& fonts, bool isCustomFallback);

    template <Font::TypefaceReader typefaceReader>
    static std::shared_ptr<FontFamily> readFrom(BufferReader* reader) {
        uint32_t localeListId = readLocaleListInternal(reader);
        uint32_t fontsCount = reader->read<uint32_t>();
        std::vector<std::shared_ptr<Font>> fonts;
        fonts.reserve(fontsCount);
        for (uint32_t i = 0; i < fontsCount; i++) {
            fonts.emplace_back(Font::readFrom<typefaceReader>(reader, localeListId));
        }
        return readFromInternal(reader, std::move(fonts), localeListId);
    }

    template <Font::TypefaceWriter typefaceWriter>
    void writeTo(BufferWriter* writer) const {
        writeLocaleListInternal(writer);
        writer->write<uint32_t>(mFonts.size());
        for (const std::shared_ptr<Font>& font : mFonts) {
            font->writeTo<typefaceWriter>(writer);
        }
        writeToInternal(writer);
    }

    FakedFont getClosestMatch(FontStyle style) const;

    uint32_t localeListId() const { return mLocaleListId; }
    FamilyVariant variant() const { return mVariant; }

    // API's for enumerating the fonts in a family. These don't guarantee any particular order
    size_t getNumFonts() const { return mFonts.size(); }
    const Font* getFont(size_t index) const { return mFonts[index].get(); }
    const std::shared_ptr<Font>& getFontRef(size_t index) const { return mFonts[index]; }
    FontStyle getStyle(size_t index) const { return mFonts[index]->style(); }
    bool isColorEmojiFamily() const { return mIsColorEmoji; }
    const std::unordered_set<AxisTag>& supportedAxes() const { return mSupportedAxes; }
    bool isCustomFallback() const { return mIsCustomFallback; }

    // Get Unicode coverage.
    const SparseBitSet& getCoverage() const { return mCoverage; }

    // Returns true if the font has a glyph for the code point and variation selector pair.
    // Caller should acquire a lock before calling the method.
    bool hasGlyph(uint32_t codepoint, uint32_t variationSelector) const;

    // Returns true if this font family has a variaion sequence table (cmap format 14 subtable).
    bool hasVSTable() const { return !mCmapFmt14Coverage.empty(); }

    // Creates new FontFamily based on this family while applying font variations. Returns nullptr
    // if none of variations apply to this family.
    std::shared_ptr<FontFamily> createFamilyWithVariation(
            const std::vector<FontVariation>& variations) const;

private:
    FontFamily(uint32_t localeListId, FamilyVariant variant,
               std::vector<std::shared_ptr<Font>>&& fonts,
               std::unordered_set<AxisTag>&& supportedAxes, bool isColorEmoji,
               bool isCustomFallback, SparseBitSet&& coverage,
               std::vector<std::unique_ptr<SparseBitSet>>&& cmapFmt14Coverage);

    static uint32_t readLocaleListInternal(BufferReader* reader);
    static std::shared_ptr<FontFamily> readFromInternal(BufferReader* reader,
                                                        std::vector<std::shared_ptr<Font>>&& fonts,
                                                        uint32_t localeListId);
    void writeLocaleListInternal(BufferWriter* writer) const;
    void writeToInternal(BufferWriter* writer) const;

    void computeCoverage();

    uint32_t mLocaleListId;
    FamilyVariant mVariant;
    std::vector<std::shared_ptr<Font>> mFonts;
    std::unordered_set<AxisTag> mSupportedAxes;
    bool mIsColorEmoji;
    bool mIsCustomFallback;

    SparseBitSet mCoverage;
    std::vector<std::unique_ptr<SparseBitSet>> mCmapFmt14Coverage;

    MINIKIN_PREVENT_COPY_AND_ASSIGN(FontFamily);
};

}  // namespace minikin

#endif  // MINIKIN_FONT_FAMILY_H
