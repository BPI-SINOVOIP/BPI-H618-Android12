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

//#define LOG_NDEBUG 0
#define LOG_TAG "HevcEncComponent"
#include "HevcEncComponent.h"

#include <C2Debug.h>
#include <C2PlatformSupport.h>
#include <Codec2BufferUtils.h>
#include <media/hardware/VideoAPI.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/foundation/AUtils.h>
#include <util/C2InterfaceHelper.h>
#include <utils/misc.h>

#include <memory>

#include "C2HwSupport.h"
#include "C2Log.h"
#include "HwC2Interface.h"
#include "VencParameterHelper.h"

namespace android {

namespace {

constexpr char COMPONENT_NAME[] = "c2.allwinner.hevc.encoder";

}  // namespace
#define MAX_RC_LOOKAHEAD 1
#define DEFAULT_RC_LOOKAHEAD 0
class HevcEncComponent::IntfImpl : public HwC2Interface<void>::BaseParams {
 public:
  explicit IntfImpl(const std::shared_ptr<C2ReflectorHelper> &helper)
      : HwC2Interface<void>::BaseParams(
            helper, COMPONENT_NAME, C2Component::KIND_ENCODER,
            C2Component::DOMAIN_VIDEO, MEDIA_MIMETYPE_VIDEO_AVC) {
    noPrivateBuffers();
    noInputReferences();
    noOutputReferences();
    noTimeStretch();
    setDerivedInstance(this);
    mParaHelper = std::make_shared<VencParameterHelper>();

    addParameter(
        DefineParam(mActualInputDelay, C2_PARAMKEY_INPUT_DELAY)
            .withDefault(new C2PortActualDelayTuning::input(
                DEFAULT_B_FRAMES + DEFAULT_RC_LOOKAHEAD))
            .withFields({C2F(mActualInputDelay, value)
                             .inRange(0, MAX_B_FRAMES + MAX_RC_LOOKAHEAD)})
            .withSetter(
                Setter<decltype(*mActualInputDelay)>::StrictValueWithNoDeps)
            .build());

    addParameter(DefineParam(mAttrib, C2_PARAMKEY_COMPONENT_ATTRIBUTES)
                     .withConstValue(new C2ComponentAttributesSetting(
                         C2Component::ATTRIB_IS_TEMPORAL))
                     .build());

    addParameter(
        DefineParam(mParaHelper->mUsage, C2_PARAMKEY_INPUT_STREAM_USAGE)
            .withConstValue(new C2StreamUsageTuning::input(
                0u, (uint64_t)C2MemoryUsage::CPU_READ))
            .build());

    // matches size limits in codec library
    addParameter(
        DefineParam(mParaHelper->mSize, C2_PARAMKEY_PICTURE_SIZE)
            .withDefault(new C2StreamPictureSizeInfo::input(0u, 320, 240))
            .withFields({
                C2F(mParaHelper->mSize, width).inRange(2, 1920, 2),
                C2F(mParaHelper->mSize, height).inRange(2, 1088, 2),
            })
            .withSetter(SizeSetter)
            .build());

    addParameter(
        DefineParam(mParaHelper->mFrameRate, C2_PARAMKEY_FRAME_RATE)
            .withDefault(new C2StreamFrameRateInfo::output(0u, 30.))
            .withFields({C2F(mParaHelper->mFrameRate, value).greaterThan(0.)})
            .withSetter(Setter<decltype(
                            *mParaHelper->mFrameRate)>::StrictValueWithNoDeps)
            .build());

    addParameter(
        DefineParam(mParaHelper->mBitrateMode, C2_PARAMKEY_BITRATE_MODE)
            .withDefault(new C2StreamBitrateModeTuning::output(
                0u, C2Config::BITRATE_VARIABLE))
            .withFields({C2F(mParaHelper->mBitrateMode, value)
                             .oneOf({C2Config::BITRATE_CONST,
                                     C2Config::BITRATE_VARIABLE,
                                     C2Config::BITRATE_IGNORE})})
            .withSetter(Setter<decltype(
                            *mParaHelper->mBitrateMode)>::StrictValueWithNoDeps)
            .build());

    addParameter(
        DefineParam(mParaHelper->mBitrate, C2_PARAMKEY_BITRATE)
            .withDefault(new C2StreamBitrateInfo::output(0u, 64000))
            .withFields(
                {C2F(mParaHelper->mBitrate, value).inRange(4096, 12000000)})
            .withSetter(BitrateSetter)
            .build());

    // matches levels allowed within codec library
    addParameter(
        DefineParam(mParaHelper->mComplexity, C2_PARAMKEY_COMPLEXITY)
            .withDefault(new C2StreamComplexityTuning::output(0u, 0))
            .withFields({C2F(mParaHelper->mComplexity, value).inRange(0, 10)})
            .withSetter(
                Setter<decltype(
                    *mParaHelper->mComplexity)>::NonStrictValueWithNoDeps)
            .build());

    addParameter(
        DefineParam(mParaHelper->mQuality, C2_PARAMKEY_QUALITY)
            .withDefault(new C2StreamQualityTuning::output(0u, 80))
            .withFields({C2F(mParaHelper->mQuality, value).inRange(0, 100)})
            .withSetter(Setter<decltype(
                            *mParaHelper->mQuality)>::NonStrictValueWithNoDeps)
            .build());

    addParameter(
        DefineParam(mParaHelper->mProfileLevel, C2_PARAMKEY_PROFILE_LEVEL)
            .withDefault(new C2StreamProfileLevelInfo::output(
                0u, PROFILE_HEVC_MAIN, LEVEL_HEVC_MAIN_1))
            .withFields({
                C2F(mParaHelper->mProfileLevel, profile)
                    .oneOf({C2Config::PROFILE_HEVC_MAIN,
                            C2Config::PROFILE_HEVC_MAIN_STILL}),
                C2F(mParaHelper->mProfileLevel, level)
                    .oneOf({LEVEL_HEVC_MAIN_1, LEVEL_HEVC_MAIN_2,
                            LEVEL_HEVC_MAIN_2_1, LEVEL_HEVC_MAIN_3,
                            LEVEL_HEVC_MAIN_3_1, LEVEL_HEVC_MAIN_4,
                            LEVEL_HEVC_MAIN_4_1, LEVEL_HEVC_MAIN_5,
                            LEVEL_HEVC_MAIN_5_1, LEVEL_HEVC_MAIN_5_2}),
            })
            .withSetter(ProfileLevelSetter, mParaHelper->mSize,
                        mParaHelper->mFrameRate, mParaHelper->mBitrate)
            .build());

    addParameter(
        DefineParam(mParaHelper->mRequestSync, C2_PARAMKEY_REQUEST_SYNC_FRAME)
            .withDefault(
                new C2StreamRequestSyncFrameTuning::output(0u, C2_FALSE))
            .withFields({C2F(mParaHelper->mRequestSync, value)
                             .oneOf({C2_FALSE, C2_TRUE})})
            .withSetter(
                Setter<decltype(
                    *mParaHelper->mRequestSync)>::NonStrictValueWithNoDeps)
            .build());

    addParameter(
        DefineParam(mParaHelper->mSyncFramePeriod,
                    C2_PARAMKEY_SYNC_FRAME_INTERVAL)
            .withDefault(
                new C2StreamSyncFrameIntervalTuning::output(0u, 1000000))
            .withFields({C2F(mParaHelper->mSyncFramePeriod, value).any()})
            .withSetter(
                Setter<decltype(
                    *mParaHelper->mSyncFramePeriod)>::StrictValueWithNoDeps)
            .build());
  }

  static C2R BitrateSetter(bool /*mayBlock*/,
                           C2P<C2StreamBitrateInfo::output> &me) {
    C2R res = C2R::Ok();
    if (me.v.value < 4096) {
      me.set().value = 4096;
    }
    return res;
  }

  static C2R SizeSetter(bool /*mayBlock*/,
                        const C2P<C2StreamPictureSizeInfo::input> &oldMe,
                        C2P<C2StreamPictureSizeInfo::input> &me) {
    C2R res = C2R::Ok();
    if (!me.F(me.v.width).supportsAtAll(me.v.width)) {
      res = res.plus(C2SettingResultBuilder::BadValue(me.F(me.v.width)));
      me.set().width = oldMe.v.width;
    }
    if (!me.F(me.v.height).supportsAtAll(me.v.height)) {
      res = res.plus(C2SettingResultBuilder::BadValue(me.F(me.v.height)));
      me.set().height = oldMe.v.height;
    }
    return res;
  }

  static C2R ProfileLevelSetter(
      bool /*mayBlock*/, C2P<C2StreamProfileLevelInfo::output> &me,
      const C2P<C2StreamPictureSizeInfo::input> &size,
      const C2P<C2StreamFrameRateInfo::output> &frameRate,
      const C2P<C2StreamBitrateInfo::output> &bitrate) {
    if (!me.F(me.v.profile).supportsAtAll(me.v.profile)) {
      me.set().profile = PROFILE_HEVC_MAIN;
    }

    struct LevelLimits {
      C2Config::level_t level;
      uint64_t samplesPerSec;
      uint64_t samples;
      uint32_t bitrate;
    };

    constexpr LevelLimits kLimits[] = {
        {LEVEL_HEVC_MAIN_1, 552960, 36864, 128000},
        {LEVEL_HEVC_MAIN_2, 3686400, 122880, 1500000},
        {LEVEL_HEVC_MAIN_2_1, 7372800, 245760, 3000000},
        {LEVEL_HEVC_MAIN_3, 16588800, 552960, 6000000},
        {LEVEL_HEVC_MAIN_3_1, 33177600, 983040, 10000000},
        {LEVEL_HEVC_MAIN_4, 66846720, 2228224, 12000000},
        {LEVEL_HEVC_MAIN_4_1, 133693440, 2228224, 20000000},
        {LEVEL_HEVC_MAIN_5, 267386880, 8912896, 25000000},
        {LEVEL_HEVC_MAIN_5_1, 534773760, 8912896, 40000000},
        {LEVEL_HEVC_MAIN_5_2, 1069547520, 8912896, 60000000},
        {LEVEL_HEVC_MAIN_6, 1069547520, 35651584, 60000000},
        {LEVEL_HEVC_MAIN_6_1, 2139095040, 35651584, 120000000},
        {LEVEL_HEVC_MAIN_6_2, 4278190080, 35651584, 240000000},
    };

    uint64_t samples = size.v.width * size.v.height;
    uint64_t samplesPerSec = samples * frameRate.v.value;

    // Check if the supplied level meets the MB / bitrate requirements. If
    // not, update the level with the lowest level meeting the requirements.

    bool found = false;
    // By default needsUpdate = false in case the supplied level does meet
    // the requirements.
    bool needsUpdate = false;
    for (const LevelLimits &limit : kLimits) {
      if (samples <= limit.samples && samplesPerSec <= limit.samplesPerSec &&
          bitrate.v.value <= limit.bitrate) {
        // This is the lowest level that meets the requirements, and if
        // we haven't seen the supplied level yet, that means we don't
        // need the update.
        if (needsUpdate) {
          // c2_logd("Given level %x does not cover current configuration: "
          //    "adjusting to %x", me.v.level, limit.level);
          // me.set().level = limit.level;
        }
        found = true;
        break;
      }
      if (me.v.level == limit.level) {
        // We break out of the loop when the lowest feasible level is
        // found. The fact that we're here means that our level doesn't
        // meet the requirement and needs to be updated.
        needsUpdate = true;
      }
    }
    if (!found) {
      // We set to the highest supported level.
      me.set().level = LEVEL_HEVC_MAIN_5_2;
    }
    return C2R::Ok();
  }

  static C2R IntraRefreshSetter(bool /*mayBlock*/,
                                C2P<C2StreamIntraRefreshTuning::output> &me) {
    C2R res = C2R::Ok();
    if (me.v.period < 1) {
      me.set().mode = C2Config::INTRA_REFRESH_DISABLED;
      me.set().period = 0;
    } else {
      // only support arbitrary mode (cyclic in our case)
      me.set().mode = C2Config::INTRA_REFRESH_ARBITRARY;
    }
    return res;
  }

  static C2R GopSetter(bool /*mayBlock*/, C2P<C2StreamGopTuning::output> &me) {
    for (size_t i = 0; i < me.v.flexCount(); ++i) {
      const C2GopLayerStruct &layer = me.v.m.values[0];
      if (layer.type_ == C2Config::picture_type_t(P_FRAME | B_FRAME) &&
          layer.count > MAX_B_FRAMES) {
        me.set().m.values[i].count = MAX_B_FRAMES;
      }
    }
    return C2R::Ok();
  }

  std::shared_ptr<VencParameterHelper> mParaHelper;
};

HevcEncComponent::HevcEncComponent(const char *name, c2_node_id_t id,
                                   const std::shared_ptr<IntfImpl> &intfImpl)
    : HwC2Component(
          std::make_shared<HwC2Interface<IntfImpl>>(name, id, intfImpl)),
      mIntf(intfImpl) {
  mCodec = std::make_unique<VencComponent>(name, mIntf->mParaHelper);
  mCodec->setIntf(mIntf);
}

HevcEncComponent::~HevcEncComponent() {}

c2_status_t HevcEncComponent::onInit() { return mCodec->onInit(); }

c2_status_t HevcEncComponent::onStart() { return mCodec->onStart(); }

c2_status_t HevcEncComponent::onStop() { return mCodec->onStop(); }

void HevcEncComponent::onReset() { mCodec->onReset(); }

void HevcEncComponent::onRelease() { mCodec->onRelease(); }

c2_status_t HevcEncComponent::onFlush_sm() { return mCodec->onFlush_sm(); }

void HevcEncComponent::process(const std::unique_ptr<C2Work> &work,
                               const std::shared_ptr<C2BlockPool> &pool) {
  mCodec->process(work, pool);
}

c2_status_t HevcEncComponent::drain(uint32_t drainMode,
                                    const std::shared_ptr<C2BlockPool> &pool) {
  return mCodec->drain(drainMode, pool);
}

class HevcEncComponentFactory : public C2ComponentFactory {
 public:
  HevcEncComponentFactory()
      : mHelper(std::static_pointer_cast<C2ReflectorHelper>(
            GetCodec2HwComponentStore()->getParamReflector())) {}

  c2_status_t createComponent(
      c2_node_id_t id, std::shared_ptr<C2Component> *const component,
      std::function<void(C2Component *)> deleter) override {
    *component = std::shared_ptr<C2Component>(
        new HevcEncComponent(
            COMPONENT_NAME, id,
            std::make_shared<HevcEncComponent::IntfImpl>(mHelper)),
        deleter);
    return C2_OK;
  }

  c2_status_t createInterface(
      c2_node_id_t id, std::shared_ptr<C2ComponentInterface> *const interface,
      std::function<void(C2ComponentInterface *)> deleter) override {
    *interface = std::shared_ptr<C2ComponentInterface>(
        new HwC2Interface<HevcEncComponent::IntfImpl>(
            COMPONENT_NAME, id,
            std::make_shared<HevcEncComponent::IntfImpl>(mHelper)),
        deleter);
    return C2_OK;
  }

  ~HevcEncComponentFactory() override = default;

 private:
  std::shared_ptr<C2ReflectorHelper> mHelper;
};

}  // namespace android

extern "C" ::C2ComponentFactory *CreateCodec2Factory() {
  return new ::android::HevcEncComponentFactory();
}

extern "C" void DestroyCodec2Factory(::C2ComponentFactory *factory) {
  delete factory;
}
