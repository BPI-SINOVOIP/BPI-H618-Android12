/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include "minikin/Font.h"

#include <vector>

#include <hb-ot.h>
#include <hb.h>
#include <log/log.h>

#include "minikin/HbUtils.h"
#include "minikin/MinikinFont.h"

#include "FontUtils.h"
#include "MinikinInternal.h"

namespace minikin {

std::shared_ptr<Font> Font::Builder::build() {
    if (mIsWeightSet && mIsSlantSet) {
        // No need to read OS/2 header of the font file.
        return std::shared_ptr<Font>(new Font(std::move(mTypeface), FontStyle(mWeight, mSlant),
                                              prepareFont(mTypeface), mLocaleListId));
    }

    HbFontUniquePtr font = prepareFont(mTypeface);
    FontStyle styleFromFont = analyzeStyle(font);
    if (!mIsWeightSet) {
        mWeight = styleFromFont.weight();
    }
    if (!mIsSlantSet) {
        mSlant = styleFromFont.slant();
    }
    return std::shared_ptr<Font>(new Font(std::move(mTypeface), FontStyle(mWeight, mSlant),
                                          std::move(font), mLocaleListId));
}

const std::shared_ptr<MinikinFont>& Font::typeface() const {
    std::lock_guard lock(mTypefaceMutex);
    if (mTypeface) return mTypeface;
    initTypefaceLocked();
    return mTypeface;
}

const HbFontUniquePtr& Font::baseFont() const {
    std::lock_guard lock(mTypefaceMutex);
    if (mBaseFont) return mBaseFont;
    initTypefaceLocked();
    mBaseFont = prepareFont(mTypeface);
    return mBaseFont;
}

void Font::initTypefaceLocked() const {
    if (mTypeface) return;
    MINIKIN_ASSERT(mTypefaceLoader, "mTypefaceLoader should not be empty when mTypeface is null");
    mTypeface = mTypefaceLoader(mTypefaceMetadataReader);
}

// static
HbFontUniquePtr Font::prepareFont(const std::shared_ptr<MinikinFont>& typeface) {
    const char* buf = reinterpret_cast<const char*>(typeface->GetFontData());
    size_t size = typeface->GetFontSize();
    uint32_t ttcIndex = typeface->GetFontIndex();

    HbBlobUniquePtr blob(hb_blob_create(buf, size, HB_MEMORY_MODE_READONLY, nullptr, nullptr));
    HbFaceUniquePtr face(hb_face_create(blob.get(), ttcIndex));
    HbFontUniquePtr parent(hb_font_create(face.get()));
    hb_ot_font_set_funcs(parent.get());

    uint32_t upem = hb_face_get_upem(face.get());
    hb_font_set_scale(parent.get(), upem, upem);

    HbFontUniquePtr font(hb_font_create_sub_font(parent.get()));
    std::vector<hb_variation_t> variations;
    variations.reserve(typeface->GetAxes().size());
    for (const FontVariation& variation : typeface->GetAxes()) {
        variations.push_back({variation.axisTag, variation.value});
    }
    hb_font_set_variations(font.get(), variations.data(), variations.size());
    return font;
}

// static
FontStyle Font::analyzeStyle(const HbFontUniquePtr& font) {
    HbBlob os2Table(font, MinikinFont::MakeTag('O', 'S', '/', '2'));
    if (!os2Table) {
        return FontStyle();
    }

    int weight;
    bool italic;
    if (!::minikin::analyzeStyle(os2Table.get(), os2Table.size(), &weight, &italic)) {
        return FontStyle();
    }
    // TODO: Update weight/italic based on fvar value.
    return FontStyle(static_cast<uint16_t>(weight), static_cast<FontStyle::Slant>(italic));
}

std::unordered_set<AxisTag> Font::getSupportedAxes() const {
    HbBlob fvarTable(baseFont(), MinikinFont::MakeTag('f', 'v', 'a', 'r'));
    if (!fvarTable) {
        return std::unordered_set<AxisTag>();
    }
    std::unordered_set<AxisTag> supportedAxes;
    analyzeAxes(fvarTable.get(), fvarTable.size(), &supportedAxes);
    return supportedAxes;
}

}  // namespace minikin
