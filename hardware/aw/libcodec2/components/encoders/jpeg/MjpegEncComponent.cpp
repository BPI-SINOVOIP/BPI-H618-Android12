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
#define LOG_TAG "MjpegEncComponent"
#include "MjpegEncComponent.h"

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

constexpr char COMPONENT_NAME[] = "c2.allwinner.mjpeg.encoder";

}  // namespace

class MjpegEncComponent::IntfImpl : public HwC2Interface<void>::BaseParams {
 public:
  explicit IntfImpl(const std::shared_ptr<C2ReflectorHelper> &helper)
      : HwC2Interface<void>::BaseParams(
            helper, COMPONENT_NAME, C2Component::KIND_ENCODER,
            C2Component::DOMAIN_VIDEO, MEDIA_MIMETYPE_VIDEO_MJPEG) {
    noPrivateBuffers();
    noInputReferences();
    noOutputReferences();
    noTimeStretch();
    setDerivedInstance(this);
    mParaHelper = std::make_shared<VencParameterHelper>();
    addParameter(
        DefineParam(mParaHelper->mUsage, C2_PARAMKEY_INPUT_STREAM_USAGE)
            .withConstValue(new C2StreamUsageTuning::input(
                0u, (uint64_t)C2MemoryUsage::CPU_READ))
            .build());

    addParameter(DefineParam(mAttrib, C2_PARAMKEY_COMPONENT_ATTRIBUTES)
                     .withConstValue(new C2ComponentAttributesSetting(
                         C2Component::ATTRIB_IS_TEMPORAL))
                     .build());

    addParameter(
        DefineParam(mParaHelper->mSize, C2_PARAMKEY_PICTURE_SIZE)
            .withDefault(new C2StreamPictureSizeInfo::input(0u, 320, 240))
            .withFields({
                C2F(mParaHelper->mSize, width).inRange(2, 2560, 2),
                C2F(mParaHelper->mSize, height).inRange(2, 2560, 2),
            })
            .withSetter(SizeSetter)
            .build());

    addParameter(
        DefineParam(mParaHelper->mGop, C2_PARAMKEY_GOP)
            .withDefault(C2StreamGopTuning::output::AllocShared(
                0 /* flexCount */, 0u /* stream */))
            .withFields({C2F(mParaHelper->mGop, m.values[0].type_).any(),
                         C2F(mParaHelper->mGop, m.values[0].count).any()})
            .withSetter(GopSetter)
            .build());

    addParameter(
        DefineParam(mActualInputDelay, C2_PARAMKEY_INPUT_DELAY)
            .withDefault(new C2PortActualDelayTuning::input(DEFAULT_B_FRAMES))
            .withFields(
                {C2F(mActualInputDelay, value).inRange(0, MAX_B_FRAMES)})
            .calculatedAs(InputDelaySetter, mParaHelper->mGop)
            .build());

    addParameter(
        DefineParam(mParaHelper->mFrameRate, C2_PARAMKEY_FRAME_RATE)
            .withDefault(new C2StreamFrameRateInfo::output(0u, 30.))
            .withFields({C2F(mParaHelper->mFrameRate, value).greaterThan(0.)})
            .withSetter(Setter<decltype(
                            *mParaHelper->mFrameRate)>::StrictValueWithNoDeps)
            .build());

    addParameter(
        DefineParam(mParaHelper->mBitrate, C2_PARAMKEY_BITRATE)
            .withDefault(new C2StreamBitrateInfo::output(0u, 64000))
            .withFields(
                {C2F(mParaHelper->mBitrate, value).inRange(4096, 12000000)})
            .withSetter(BitrateSetter)
            .build());

    addParameter(
        DefineParam(mParaHelper->mIntraRefresh, C2_PARAMKEY_INTRA_REFRESH)
            .withDefault(new C2StreamIntraRefreshTuning::output(
                0u, C2Config::INTRA_REFRESH_DISABLED, 0.))
            .withFields({C2F(mParaHelper->mIntraRefresh, mode)
                             .oneOf({C2Config::INTRA_REFRESH_DISABLED,
                                     C2Config::INTRA_REFRESH_ARBITRARY}),
                         C2F(mParaHelper->mIntraRefresh, period).any()})
            .withSetter(IntraRefreshSetter)
            .build());

    addParameter(
        DefineParam(mParaHelper->mProfileLevel, C2_PARAMKEY_PROFILE_LEVEL)
            .withDefault(new C2StreamProfileLevelInfo::output(
                0u, PROFILE_MJPEG_CONSTRAINED_BASELINE, LEVEL_MJPEG_4_1))
            .withFields({
                C2F(mParaHelper->mProfileLevel, profile)
                    .oneOf({
                        PROFILE_MJPEG_BASELINE,
                        PROFILE_MJPEG_CONSTRAINED_BASELINE,
                        PROFILE_MJPEG_MAIN,
                    }),
                C2F(mParaHelper->mProfileLevel, level)
                    .oneOf({
                        LEVEL_MJPEG_1,
                        LEVEL_MJPEG_1B,
                        LEVEL_MJPEG_1_1,
                        LEVEL_MJPEG_1_2,
                        LEVEL_MJPEG_1_3,
                        LEVEL_MJPEG_2,
                        LEVEL_MJPEG_2_1,
                        LEVEL_MJPEG_2_2,
                        LEVEL_MJPEG_3,
                        LEVEL_MJPEG_3_1,
                        LEVEL_MJPEG_3_2,
                        LEVEL_MJPEG_4,
                        LEVEL_MJPEG_4_1,
                        LEVEL_MJPEG_4_2,
                        LEVEL_MJPEG_5,
                    }),
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

  static C2R InputDelaySetter(bool /*mayBlock*/,
                              C2P<C2PortActualDelayTuning::input> &me,
                              const C2P<C2StreamGopTuning::output> &gop) {
    uint32_t maxBframes = 0;
    ParseGop(gop.v, nullptr, nullptr, &maxBframes);
    me.set().value = maxBframes;
    return C2R::Ok();
  }

  static C2R BitrateSetter(bool /*mayBlock*/,
                           C2P<C2StreamBitrateInfo::output> &me) {
    C2R res = C2R::Ok();
    if (me.v.value <= 4096) {
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
      me.set().profile = PROFILE_MJPEG_CONSTRAINED_BASELINE;
    }

    struct LevelLimits {
      C2Config::level_t level;
      float mbsPerSec;
      uint64_t mbs;
      uint32_t bitrate;
    };
    constexpr LevelLimits kLimits[] = {
        {LEVEL_MJPEG_1, 1485, 99, 64000},
        // Decoder does not properly handle level 1b.
        // { LEVEL_MJPEG_1B,    1485,   99,   128000 },
        {LEVEL_MJPEG_1_1, 3000, 396, 192000},
        {LEVEL_MJPEG_1_2, 6000, 396, 384000},
        {LEVEL_MJPEG_1_3, 11880, 396, 768000},
        {LEVEL_MJPEG_2, 11880, 396, 2000000},
        {LEVEL_MJPEG_2_1, 19800, 792, 4000000},
        {LEVEL_MJPEG_2_2, 20250, 1620, 4000000},
        {LEVEL_MJPEG_3, 40500, 1620, 10000000},
        {LEVEL_MJPEG_3_1, 108000, 3600, 14000000},
        {LEVEL_MJPEG_3_2, 216000, 5120, 20000000},
        {LEVEL_MJPEG_4, 245760, 8192, 20000000},
        {LEVEL_MJPEG_4_1, 245760, 8192, 50000000},
        {LEVEL_MJPEG_4_2, 522240, 8704, 50000000},
        {LEVEL_MJPEG_5, 589824, 22080, 135000000},
    };

    uint64_t mbs =
        uint64_t((size.v.width + 15) / 16) * ((size.v.height + 15) / 16);
    float mbsPerSec = static_cast<float>(mbs) * frameRate.v.value;

    // Check if the supplied level meets the MB / bitrate requirements. If
    // not, update the level with the lowest level meeting the requirements.

    bool found = false;
    // By default needsUpdate = false in case the supplied level does meet
    // the requirements. For Level 1b, we want to update the level anyway,
    // so we set it to true in that case.
    bool needsUpdate = (me.v.level == LEVEL_MJPEG_1B);
    for (const LevelLimits &limit : kLimits) {
      if (mbs <= limit.mbs && mbsPerSec <= limit.mbsPerSec &&
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
      me.set().level = LEVEL_MJPEG_5;
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

MjpegEncComponent::MjpegEncComponent(const char *name, c2_node_id_t id,
                                     const std::shared_ptr<IntfImpl> &intfImpl)
    : HwC2Component(
          std::make_shared<HwC2Interface<IntfImpl>>(name, id, intfImpl)),
      mIntf(intfImpl) {
  mCodec = std::make_unique<VencComponent>(name, mIntf->mParaHelper);
}

MjpegEncComponent::~MjpegEncComponent() {}

c2_status_t MjpegEncComponent::onInit() { return mCodec->onInit(); }

c2_status_t MjpegEncComponent::onStart() { return mCodec->onStart(); }

c2_status_t MjpegEncComponent::onStop() { return mCodec->onStop(); }

void MjpegEncComponent::onReset() { mCodec->onReset(); }

void MjpegEncComponent::onRelease() { mCodec->onRelease(); }

c2_status_t MjpegEncComponent::onFlush_sm() { return mCodec->onFlush_sm(); }

void MjpegEncComponent::process(const std::unique_ptr<C2Work> &work,
                                const std::shared_ptr<C2BlockPool> &pool) {
  mCodec->process(work, pool);
}

c2_status_t MjpegEncComponent::drain(uint32_t drainMode,
                                     const std::shared_ptr<C2BlockPool> &pool) {
  return mCodec->drain(drainMode, pool);
}

class MjpegEncComponentFactory : public C2ComponentFactory {
 public:
  MjpegEncComponentFactory()
      : mHelper(std::static_pointer_cast<C2ReflectorHelper>(
            GetCodec2HwComponentStore()->getParamReflector())) {}

  c2_status_t createComponent(
      c2_node_id_t id, std::shared_ptr<C2Component> *const component,
      std::function<void(C2Component *)> deleter) override {
    *component = std::shared_ptr<C2Component>(
        new MjpegEncComponent(
            COMPONENT_NAME, id,
            std::make_shared<MjpegEncComponent::IntfImpl>(mHelper)),
        deleter);
    return C2_OK;
  }

  c2_status_t createInterface(
      c2_node_id_t id, std::shared_ptr<C2ComponentInterface> *const interface,
      std::function<void(C2ComponentInterface *)> deleter) override {
    *interface = std::shared_ptr<C2ComponentInterface>(
        new HwC2Interface<MjpegEncComponent::IntfImpl>(
            COMPONENT_NAME, id,
            std::make_shared<MjpegEncComponent::IntfImpl>(mHelper)),
        deleter);
    return C2_OK;
  }

  ~MjpegEncComponentFactory() override = default;

 private:
  std::shared_ptr<C2ReflectorHelper> mHelper;
};

}  // namespace android

extern "C" ::C2ComponentFactory *CreateCodec2Factory() {
  return new ::android::MjpegEncComponentFactory();
}

extern "C" void DestroyCodec2Factory(::C2ComponentFactory *factory) {
  delete factory;
}
