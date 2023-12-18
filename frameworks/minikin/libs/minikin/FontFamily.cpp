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

#define LOG_TAG "Minikin"

#include "minikin/FontFamily.h"

#include <algorithm>
#include <vector>

#include <log/log.h>

#include "minikin/CmapCoverage.h"
#include "minikin/FamilyVariant.h"
#include "minikin/HbUtils.h"
#include "minikin/MinikinFont.h"

#include "FontUtils.h"
#include "Locale.h"
#include "LocaleListCache.h"
#include "MinikinInternal.h"

namespace minikin {

FontFamily::FontFamily(std::vector<std::shared_ptr<Font>>&& fonts)
        : FontFamily(FamilyVariant::DEFAULT, std::move(fonts)) {}

FontFamily::FontFamily(FamilyVariant variant, std::vector<std::shared_ptr<Font>>&& fonts)
        : FontFamily(kEmptyLocaleListId, variant, std::move(fonts), false /* isCustomFallback */) {}

FontFamily::FontFamily(uint32_t localeListId, FamilyVariant variant,
                       std::vector<std::shared_ptr<Font>>&& fonts, bool isCustomFallback)
        : mLocaleListId(localeListId),
          mVariant(variant),
          mFonts(std::move(fonts)),
          mIsColorEmoji(LocaleListCache::getById(localeListId).getEmojiStyle() ==
                        EmojiStyle::EMOJI),
          mIsCustomFallback(isCustomFallback) {
    MINIKIN_ASSERT(!mFonts.empty(), "FontFamily must contain at least one font.");
    computeCoverage();
}

FontFamily::FontFamily(uint32_t localeListId, FamilyVariant variant,
                       std::vector<std::shared_ptr<Font>>&& fonts,
                       std::unordered_set<AxisTag>&& supportedAxes, bool isColorEmoji,
                       bool isCustomFallback, SparseBitSet&& coverage,
                       std::vector<std::unique_ptr<SparseBitSet>>&& cmapFmt14Coverage)
        : mLocaleListId(localeListId),
          mVariant(variant),
          mFonts(std::move(fonts)),
          mSupportedAxes(std::move(supportedAxes)),
          mIsColorEmoji(isColorEmoji),
          mIsCustomFallback(isCustomFallback),
          mCoverage(std::move(coverage)),
          mCmapFmt14Coverage(std::move(cmapFmt14Coverage)) {}

// Read fields other than mFonts, mLocaleList.
// static
std::shared_ptr<FontFamily> FontFamily::readFromInternal(BufferReader* reader,
                                                         std::vector<std::shared_ptr<Font>>&& fonts,
                                                         uint32_t localeListId) {
    // FamilyVariant is uint8_t
    static_assert(sizeof(FamilyVariant) == 1);
    FamilyVariant variant = reader->read<FamilyVariant>();
    // AxisTag is uint32_t
    static_assert(sizeof(AxisTag) == 4);
    const auto& [axesPtr, axesCount] = reader->readArray<AxisTag>();
    std::unordered_set<AxisTag> supportedAxes(axesPtr, axesPtr + axesCount);
    bool isColorEmoji = static_cast<bool>(reader->read<uint8_t>());
    bool isCustomFallback = static_cast<bool>(reader->read<uint8_t>());
    SparseBitSet coverage(reader);
    // Read mCmapFmt14Coverage. As it can have null entries, it is stored in the buffer as a sparse
    // array (size, non-null entry count, array of (index, entry)).
    uint32_t cmapFmt14CoverageSize = reader->read<uint32_t>();
    std::vector<std::unique_ptr<SparseBitSet>> cmapFmt14Coverage(cmapFmt14CoverageSize);
    uint32_t cmapFmt14CoverageEntryCount = reader->read<uint32_t>();
    for (uint32_t i = 0; i < cmapFmt14CoverageEntryCount; i++) {
        uint32_t index = reader->read<uint32_t>();
        cmapFmt14Coverage[index] = std::make_unique<SparseBitSet>(reader);
    }
    return std::shared_ptr<FontFamily>(new FontFamily(
            localeListId, variant, std::move(fonts), std::move(supportedAxes), isColorEmoji,
            isCustomFallback, std::move(coverage), std::move(cmapFmt14Coverage)));
}

// static
uint32_t FontFamily::readLocaleListInternal(BufferReader* reader) {
    return LocaleListCache::readFrom(reader);
}

// Write fields other than mFonts.
void FontFamily::writeToInternal(BufferWriter* writer) const {
    writer->write<FamilyVariant>(mVariant);
    std::vector<AxisTag> axes(mSupportedAxes.begin(), mSupportedAxes.end());
    // Sort axes to be deterministic.
    std::sort(axes.begin(), axes.end());
    writer->writeArray<AxisTag>(axes.data(), axes.size());
    writer->write<uint8_t>(mIsColorEmoji);
    writer->write<uint8_t>(mIsCustomFallback);
    mCoverage.writeTo(writer);
    // Write mCmapFmt14Coverage as a sparse array (size, non-null entry count,
    // array of (index, entry))
    writer->write<uint32_t>(mCmapFmt14Coverage.size());
    uint32_t cmapFmt14CoverageEntryCount = 0;
    for (const std::unique_ptr<SparseBitSet>& coverage : mCmapFmt14Coverage) {
        if (coverage != nullptr) cmapFmt14CoverageEntryCount++;
    }
    writer->write<uint32_t>(cmapFmt14CoverageEntryCount);
    for (size_t i = 0; i < mCmapFmt14Coverage.size(); i++) {
        if (mCmapFmt14Coverage[i] != nullptr) {
            writer->write<uint32_t>(i);
            mCmapFmt14Coverage[i]->writeTo(writer);
        }
    }
}

void FontFamily::writeLocaleListInternal(BufferWriter* writer) const {
    LocaleListCache::writeTo(writer, mLocaleListId);
}
// Compute a matching metric between two styles - 0 is an exact match
static int computeMatch(FontStyle style1, FontStyle style2) {
    if (style1 == style2) return 0;
    int score = abs(style1.weight() / 100 - style2.weight() / 100);
    if (style1.slant() != style2.slant()) {
        score += 2;
    }
    return score;
}

static FontFakery computeFakery(FontStyle wanted, FontStyle actual) {
    // If desired weight is semibold or darker, and 2 or more grades
    // higher than actual (for example, medium 500 -> bold 700), then
    // select fake bold.
    bool isFakeBold = wanted.weight() >= 600 && (wanted.weight() - actual.weight()) >= 200;
    bool isFakeItalic = wanted.slant() == FontStyle::Slant::ITALIC &&
                        actual.slant() == FontStyle::Slant::UPRIGHT;
    return FontFakery(isFakeBold, isFakeItalic);
}

FakedFont FontFamily::getClosestMatch(FontStyle style) const {
    int bestIndex = 0;
    Font* bestFont = mFonts[bestIndex].get();
    int bestMatch = computeMatch(bestFont->style(), style);
    for (size_t i = 1; i < mFonts.size(); i++) {
        Font* font = mFonts[i].get();
        int match = computeMatch(font->style(), style);
        if (i == 0 || match < bestMatch) {
            bestFont = font;
            bestIndex = i;
            bestMatch = match;
        }
    }
    return FakedFont{mFonts[bestIndex], computeFakery(style, bestFont->style())};
}

void FontFamily::computeCoverage() {
    const std::shared_ptr<Font>& font = getClosestMatch(FontStyle()).font;
    HbBlob cmapTable(font->baseFont(), MinikinFont::MakeTag('c', 'm', 'a', 'p'));
    if (cmapTable.get() == nullptr) {
        ALOGE("Could not get cmap table size!\n");
        return;
    }

    mCoverage = CmapCoverage::getCoverage(cmapTable.get(), cmapTable.size(), &mCmapFmt14Coverage);

    for (size_t i = 0; i < mFonts.size(); ++i) {
        std::unordered_set<AxisTag> supportedAxes = mFonts[i]->getSupportedAxes();
        mSupportedAxes.insert(supportedAxes.begin(), supportedAxes.end());
    }
}

bool FontFamily::hasGlyph(uint32_t codepoint, uint32_t variationSelector) const {
    if (variationSelector == 0) {
        return mCoverage.get(codepoint);
    }

    if (mCmapFmt14Coverage.empty()) {
        return false;
    }

    const uint16_t vsIndex = getVsIndex(variationSelector);

    if (vsIndex >= mCmapFmt14Coverage.size()) {
        // Even if vsIndex is INVALID_VS_INDEX, we reach here since INVALID_VS_INDEX is defined to
        // be at the maximum end of the range.
        return false;
    }

    const std::unique_ptr<SparseBitSet>& bitset = mCmapFmt14Coverage[vsIndex];
    if (bitset.get() == nullptr) {
        return false;
    }

    return bitset->get(codepoint);
}

std::shared_ptr<FontFamily> FontFamily::createFamilyWithVariation(
        const std::vector<FontVariation>& variations) const {
    if (variations.empty() || mSupportedAxes.empty()) {
        return nullptr;
    }

    bool hasSupportedAxis = false;
    for (const FontVariation& variation : variations) {
        if (mSupportedAxes.find(variation.axisTag) != mSupportedAxes.end()) {
            hasSupportedAxis = true;
            break;
        }
    }
    if (!hasSupportedAxis) {
        // None of variation axes are suppored by this family.
        return nullptr;
    }

    std::vector<std::shared_ptr<Font>> fonts;
    for (const auto& font : mFonts) {
        bool supportedVariations = false;
        std::unordered_set<AxisTag> supportedAxes = font->getSupportedAxes();
        if (!supportedAxes.empty()) {
            for (const FontVariation& variation : variations) {
                if (supportedAxes.find(variation.axisTag) != supportedAxes.end()) {
                    supportedVariations = true;
                    break;
                }
            }
        }
        std::shared_ptr<MinikinFont> minikinFont;
        if (supportedVariations) {
            minikinFont = font->typeface()->createFontWithVariation(variations);
        }
        if (minikinFont == nullptr) {
            fonts.push_back(font);
        } else {
            fonts.push_back(Font::Builder(minikinFont).setStyle(font->style()).build());
        }
    }

    return std::shared_ptr<FontFamily>(
            new FontFamily(mLocaleListId, mVariant, std::move(fonts), mIsCustomFallback));
}

}  // namespace minikin
