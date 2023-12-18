/*
 * Copyright (C) 2021 by Allwinnertech Co. Ltd.
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

#ifndef ANDROID_COMPONENTS_INCLUDE_VENCPARAMETERHELPER_
#define ANDROID_COMPONENTS_INCLUDE_VENCPARAMETERHELPER_

#include <C2Config.h>
#include <C2Debug.h>
#include <C2PlatformSupport.h>
#include <Codec2BufferUtils.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/foundation/AUtils.h>
#include <util/C2InterfaceHelper.h>
#include <utils/Vector.h>

#include <map>
#include <memory>

#include "C2Log.h"

namespace android {

class VencParameterHelper {
 public:
  // unsafe getters
  // TODO(kay): add lock!!
  using size_info = C2StreamPictureSizeInfo::input;
  std::shared_ptr<size_info> getSize_l() const { return mSize; }

  using refresh_tuning = C2StreamIntraRefreshTuning::output;
  std::shared_ptr<refresh_tuning> getIntraRefresh_l() const {
    return mIntraRefresh;
  }

  using frame_rate_info = C2StreamFrameRateInfo::output;
  std::shared_ptr<frame_rate_info> getFrameRate_l() const { return mFrameRate; }

  using bit_rate_info = C2StreamBitrateInfo::output;
  std::shared_ptr<bit_rate_info> getBitrate_l() const { return mBitrate; }

  using frame_tuning = C2StreamRequestSyncFrameTuning::output;
  std::shared_ptr<frame_tuning> getRequestSync_l() const {
    return mRequestSync;
  }

  std::shared_ptr<C2StreamGopTuning::output> getGop_l() const { return mGop; }
  std::shared_ptr<C2StreamColorAspectsInfo::output> getCodedColorAspects_l() const {
      return mCodedColorAspects;
  }

  /***********************For HEVC start***********************/
  std::shared_ptr<C2StreamComplexityTuning::output> getComplexity_l() const {
    return mComplexity;
  }
  std::shared_ptr<C2StreamQualityTuning::output> getQuality_l() const {
    return mQuality;
  }
  //***********************For HEVC end***********************/

  int32_t getHevcLevel_l() const {
    struct Level {
      C2Config::level_t c2Level;
      int32_t hevcLevel;
    };
    constexpr Level levels[] = {
        {LEVEL_HEVC_MAIN_1, 30},    {LEVEL_HEVC_MAIN_2, 60},
        {LEVEL_HEVC_MAIN_2_1, 63},  {LEVEL_HEVC_MAIN_3, 90},
        {LEVEL_HEVC_MAIN_3_1, 93},  {LEVEL_HEVC_MAIN_4, 120},
        {LEVEL_HEVC_MAIN_4_1, 123}, {LEVEL_HEVC_MAIN_5, 150},
        {LEVEL_HEVC_MAIN_5_1, 153}, {LEVEL_HEVC_MAIN_5_2, 156},
        {LEVEL_HEVC_MAIN_6, 180},   {LEVEL_HEVC_MAIN_6_1, 183},
        {LEVEL_HEVC_MAIN_6_2, 186},
    };
    for (const Level &level : levels) {
      if (mProfileLevel->level == level.c2Level) {
        return level.hevcLevel;
      }
    }
    ALOGD("Unrecognized level: %x", mProfileLevel->level);
    return 156;
  }
  uint32_t getSyncFramePeriod_l() const {
    if (mSyncFramePeriod->value < 0 || mSyncFramePeriod->value == INT64_MAX) {
      return 0;
    }
    double period = mSyncFramePeriod->value / 1e6 * mFrameRate->value;
    return (uint32_t)c2_max(
        c2_min(period + 0.5, static_cast<double>(UINT32_MAX)), 1.);
  }

  int32_t getAvcLevel_l() const {
    struct Level {
      C2Config::level_t c2Level;
      int32_t avcLevel;
    };
    constexpr Level levels[] = {
        {LEVEL_AVC_1, 10},   {LEVEL_AVC_1B, 9},   {LEVEL_AVC_1_1, 11},
        {LEVEL_AVC_1_2, 12}, {LEVEL_AVC_1_3, 13}, {LEVEL_AVC_2, 20},
        {LEVEL_AVC_2_1, 21}, {LEVEL_AVC_2_2, 22}, {LEVEL_AVC_3, 30},
        {LEVEL_AVC_3_1, 31}, {LEVEL_AVC_3_2, 32}, {LEVEL_AVC_4, 40},
        {LEVEL_AVC_4_1, 41}, {LEVEL_AVC_4_2, 42}, {LEVEL_AVC_5, 50},
    };
    for (const Level &level : levels) {
      if (mProfileLevel->level == level.c2Level) {
        return level.avcLevel;
      }
    }
    ALOGD("Unrecognized level: %x", mProfileLevel->level);
    return 41;
  }

  /***********************For encoder start***********************/
  std::shared_ptr<C2StreamUsageTuning::input> mUsage;
  std::shared_ptr<C2StreamPictureSizeInfo::input> mSize;
  std::shared_ptr<C2StreamFrameRateInfo::output> mFrameRate;
  std::shared_ptr<C2StreamRequestSyncFrameTuning::output> mRequestSync;
  std::shared_ptr<C2StreamIntraRefreshTuning::output> mIntraRefresh;
  std::shared_ptr<C2StreamBitrateInfo::output> mBitrate;
  std::shared_ptr<C2StreamProfileLevelInfo::output> mProfileLevel;
  std::shared_ptr<C2StreamSyncFrameIntervalTuning::output> mSyncFramePeriod;
  std::shared_ptr<C2StreamGopTuning::output> mGop;
  std::shared_ptr<C2StreamColorAspectsInfo::input> mColorAspects;
  std::shared_ptr<C2StreamColorAspectsInfo::output> mCodedColorAspects;

  /***********************For encoder end***********************/

  /***********************For HEVC start***********************/
  std::shared_ptr<C2StreamComplexityTuning::output> mComplexity;
  std::shared_ptr<C2StreamQualityTuning::output> mQuality;
  std::shared_ptr<C2StreamBitrateModeTuning::output> mBitrateMode;
  /***********************For HEVC end***********************/

  void ParseGop(const C2StreamGopTuning::output &gop, uint32_t *syncInterval,
                uint32_t *iInterval, uint32_t *maxBframes) {
    uint32_t syncInt = 1;
    uint32_t iInt = 1;
    for (size_t i = 0; i < gop.flexCount(); ++i) {
      const C2GopLayerStruct &layer = gop.m.values[i];
      if (layer.count == UINT32_MAX) {
        syncInt = 0;
      } else if (syncInt <= UINT32_MAX / (layer.count + 1)) {
        syncInt *= (layer.count + 1);
      }
      if ((layer.type_ & I_FRAME) == 0) {
        if (layer.count == UINT32_MAX) {
          iInt = 0;
        } else if (iInt <= UINT32_MAX / (layer.count + 1)) {
          iInt *= (layer.count + 1);
        }
      }
      if (layer.type_ == C2Config::picture_type_t(P_FRAME | B_FRAME) &&
          maxBframes) {
        *maxBframes = layer.count;
      }
    }
    if (syncInterval) {
      *syncInterval = syncInt;
    }
    if (iInterval) {
      *iInterval = iInt;
    }
  }
};
}  // namespace android

#endif  // ANDROID_COMPONENTS_INCLUDE_VENCPARAMETERHELPER_
