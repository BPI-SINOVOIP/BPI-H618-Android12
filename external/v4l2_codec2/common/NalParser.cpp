// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//#define LOG_NDEBUG 0
#define LOG_TAG "NalParser"

#include <v4l2_codec2/common/NalParser.h>

#include <algorithm>

#include <media/stagefright/foundation/ABitReader.h>
#include <utils/Log.h>

namespace android {

namespace {

enum H264ProfileIDC {
    kProfileIDCAVLC444 = 44,
    kProfileIDScalableBaseline = 83,
    kProfileIDScalableHigh = 86,
    kProfileIDCHigh = 100,
    kProfileIDHigh10 = 110,
    kProfileIDSMultiviewHigh = 118,
    kProfileIDHigh422 = 122,
    kProfileIDStereoHigh = 128,
    kProfileIDHigh444Predictive = 244,
};

constexpr uint32_t kYUV444Idc = 3;

// Read unsigned int encoded with exponential-golomb.
uint32_t parseUE(ABitReader* br) {
    uint32_t numZeroes = 0;
    while (br->getBits(1) == 0) {
        ++numZeroes;
    }
    uint32_t val = br->getBits(numZeroes);
    return val + (1u << numZeroes) - 1;
}

// Read signed int encoded with exponential-golomb.
int32_t parseSE(ABitReader* br) {
    uint32_t codeNum = parseUE(br);
    return (codeNum & 1) ? (codeNum + 1) >> 1 : -static_cast<int32_t>(codeNum >> 1);
}

// Skip a H.264 sequence scaling list in the specified bitstream.
void skipScalingList(ABitReader* br, size_t scalingListSize) {
    size_t nextScale = 8;
    size_t lastScale = 8;
    for (size_t j = 0; j < scalingListSize; ++j) {
        if (nextScale != 0) {
            int32_t deltaScale = parseSE(br);  // delta_sl
            if (deltaScale < -128) {
                ALOGW("delta scale (%d) is below range, capping to -128", deltaScale);
                deltaScale = -128;
            } else if (deltaScale > 127) {
                ALOGW("delta scale (%d) is above range, capping to 127", deltaScale);
                deltaScale = 127;
            }
            nextScale = (lastScale + (deltaScale + 256)) % 256;
        }
        lastScale = (nextScale == 0) ? lastScale : nextScale;
    }
}

// Skip the H.264 sequence scaling matrix in the specified bitstream.
void skipScalingMatrix(ABitReader* br, size_t numScalingLists) {
    for (size_t i = 0; i < numScalingLists; ++i) {
        if (br->getBits(1)) {  // seq_scaling_list_present_flag
            if (i < 6) {
                skipScalingList(br, 16);
            } else {
                skipScalingList(br, 64);
            }
        }
    }
}

}  // namespace

NalParser::NalParser(const uint8_t* data, size_t length)
      : mCurrNalDataPos(data), mDataEnd(data + length) {
    mNextNalStartCodePos = findNextStartCodePos();
}

bool NalParser::locateNextNal() {
    if (mNextNalStartCodePos == mDataEnd) return false;
    mCurrNalDataPos = mNextNalStartCodePos + kNalStartCodeLength;  // skip start code.
    mNextNalStartCodePos = findNextStartCodePos();
    return true;
}

bool NalParser::locateSPS() {
    while (locateNextNal()) {
        if (length() == 0) continue;
        if (type() != kSPSType) continue;
        return true;
    }

    return false;
}

const uint8_t* NalParser::data() const {
    return mCurrNalDataPos;
}

size_t NalParser::length() const {
    if (mNextNalStartCodePos == mDataEnd) return mDataEnd - mCurrNalDataPos;
    size_t length = mNextNalStartCodePos - mCurrNalDataPos;
    // The start code could be 3 or 4 bytes, i.e., 0x000001 or 0x00000001.
    return *(mNextNalStartCodePos - 1) == 0x00 ? length - 1 : length;
}

uint8_t NalParser::type() const {
    // First byte is forbidden_zero_bit (1) + nal_ref_idc (2) + nal_unit_type (5)
    constexpr uint8_t kNALTypeMask = 0x1f;
    return *mCurrNalDataPos & kNALTypeMask;
}

const uint8_t* NalParser::findNextStartCodePos() const {
    return std::search(mCurrNalDataPos, mDataEnd, kNalStartCode,
                       kNalStartCode + kNalStartCodeLength);
}

bool NalParser::findCodedColorAspects(ColorAspects* colorAspects) {
    ALOG_ASSERT(colorAspects);
    ALOG_ASSERT(type() == kSPSType);

    // Unfortunately we can't directly jump to the Video Usability Information (VUI) parameters that
    // contain the color aspects. We need to parse the entire SPS header up until the values we
    // need.

    // Skip first byte containing type.
    ABitReader br(mCurrNalDataPos + 1, length() - 1);

    uint32_t profileIDC = br.getBits(8);  // profile_idc
    br.skipBits(16);                      // constraint flags + reserved bits + level_idc
    parseUE(&br);                         // seq_parameter_set_id

    if (profileIDC == kProfileIDCHigh || profileIDC == kProfileIDHigh10 ||
        profileIDC == kProfileIDHigh422 || profileIDC == kProfileIDHigh444Predictive ||
        profileIDC == kProfileIDCAVLC444 || profileIDC == kProfileIDScalableBaseline ||
        profileIDC == kProfileIDScalableHigh || profileIDC == kProfileIDSMultiviewHigh ||
        profileIDC == kProfileIDStereoHigh) {
        uint32_t chromaFormatIDC = parseUE(&br);
        if (chromaFormatIDC == kYUV444Idc) {  // chroma_format_idc
            br.skipBits(1);                   // separate_colour_plane_flag
        }
        parseUE(&br);    // bit_depth_luma_minus8
        parseUE(&br);    // bit_depth_chroma_minus8
        br.skipBits(1);  // lossless_qpprime_y_zero_flag

        if (br.getBits(1)) {  // seq_scaling_matrix_present_flag
            const size_t numScalingLists = (chromaFormatIDC != kYUV444Idc) ? 8 : 12;
            skipScalingMatrix(&br, numScalingLists);
        }
    }

    parseUE(&br);                                   // log2_max_frame_num_minus4
    uint32_t pictureOrderCountType = parseUE(&br);  // pic_order_cnt_type
    if (pictureOrderCountType == 0) {
        parseUE(&br);  // log2_max_pic_order_cnt_lsb_minus4
    } else if (pictureOrderCountType == 1) {
        br.skipBits(1);                              // delta_pic_order_always_zero_flag
        parseSE(&br);                                // offset_for_non_ref_pic
        parseSE(&br);                                // offset_for_top_to_bottom_field
        uint32_t numReferenceFrames = parseUE(&br);  // num_ref_frames_in_pic_order_cnt_cycle
        for (uint32_t i = 0; i < numReferenceFrames; ++i) {
            parseUE(&br);  // offset_for_ref_frame
        }
    }

    parseUE(&br);          // num_ref_frames
    br.skipBits(1);        // gaps_in_frame_num_value_allowed_flag
    parseUE(&br);          // pic_width_in_mbs_minus1
    parseUE(&br);          // pic_height_in_map_units_minus1
    if (!br.getBits(1)) {  // frame_mbs_only_flag
        br.skipBits(1);    // mb_adaptive_frame_field_flag
    }
    br.skipBits(1);  // direct_8x8_inference_flag

    if (br.getBits(1)) {  // frame_cropping_flag
        parseUE(&br);     // frame_cropping_rect_left_offset
        parseUE(&br);     // frame_cropping_rect_right_offset
        parseUE(&br);     // frame_cropping_rect_top_offset
        parseUE(&br);     // frame_cropping_rect_bottom_offset
    }

    if (br.getBits(1)) {                 // vui_parameters_present_flag
        if (br.getBits(1)) {             // VUI aspect_ratio_info_present_flag
            if (br.getBits(8) == 255) {  // VUI aspect_ratio_idc == extended sample aspect ratio
                br.skipBits(32);         // VUI sar_width + sar_height
            }
        }

        if (br.getBits(1)) {  // VUI overscan_info_present_flag
            br.skipBits(1);   // VUI overscan_appropriate_flag
        }
        if (br.getBits(1)) {                              // VUI video_signal_type_present_flag
            br.skipBits(3);                               // VUI video_format
            colorAspects->fullRange = br.getBits(1);      // VUI video_full_range_flag
            if (br.getBits(1)) {                          // VUI color_description_present_flag
                colorAspects->primaries = br.getBits(8);  // VUI colour_primaries
                colorAspects->transfer = br.getBits(8);   // VUI transfer_characteristics
                colorAspects->coeffs = br.getBits(8);     // VUI matrix_coefficients
                return !br.overRead();
            }
        }
    }

    return false;
}

}  // namespace android
