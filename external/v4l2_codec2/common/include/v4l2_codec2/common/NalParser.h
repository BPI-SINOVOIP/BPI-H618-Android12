// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_V4L2_CODEC2_COMMON_NALPARSER_H
#define ANDROID_V4L2_CODEC2_COMMON_NALPARSER_H

#include <stdint.h>

namespace android {

// Helper class to parse H264 NAL units from data.
class NalParser {
public:
    // Type of a SPS NAL unit.
    static constexpr uint8_t kSPSType = 7;
    // Type of a PPS NAL unit.
    static constexpr uint8_t kPPSType = 8;

    // Parameters related to a video's color aspects.
    struct ColorAspects {
        int32_t primaries;
        int32_t transfer;
        int32_t coeffs;
        bool fullRange;
    };

    NalParser(const uint8_t* data, size_t length);

    // Locates the next NAL after |mNextNalStartCodePos|. If there is one, updates |mCurrNalDataPos|
    // to the first byte of the NAL data (start code is not included), and |mNextNalStartCodePos| to
    // the position of the next start code, and returns true.
    // If there is no more NAL, returns false.
    //
    // Note: This method must be called prior to data() and length().
    bool locateNextNal();

    // Locate the sequence parameter set (SPS).
    bool locateSPS();

    // Gets current NAL data (start code is not included).
    const uint8_t* data() const;

    // Gets the byte length of current NAL data (start code is not included).
    size_t length() const;

    // Get the type of the current NAL unit.
    uint8_t type() const;

    // Find the H.264 video's color aspects in the current SPS NAL.
    bool findCodedColorAspects(ColorAspects* colorAspects);

private:
    const uint8_t* findNextStartCodePos() const;

    // The byte pattern for the start of a H264 NAL unit.
    const uint8_t kNalStartCode[3] = {0x00, 0x00, 0x01};
    // The length in bytes of the NAL-unit start pattern.
    const size_t kNalStartCodeLength = 3;

    const uint8_t* mCurrNalDataPos;
    const uint8_t* mDataEnd;
    const uint8_t* mNextNalStartCodePos;
};

}  // namespace android

#endif  // ANDROID_V4L2_CODEC2_COMMON_NALPARSER_H
