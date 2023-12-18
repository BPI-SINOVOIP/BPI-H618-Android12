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
#define LOG_TAG "Mpeg4DecComponent"
#include "Mpeg4DecComponent.h"

#include <C2Debug.h>
#include <C2PlatformSupport.h>
#include <Codec2Mapper.h>
#include <media/stagefright/foundation/MediaDefs.h>
#include <time.h>

#include <memory>

#include "C2HwSupport.h"
#include "C2Log.h"
#include "HwC2Interface.h"
#include GPU_PUBLIC_INCLUDE

#define GETTIME(a, b) gettimeofday(a, b);
#define TIME_DIFF(start, end, diff)                    \
  diff = (((end).tv_sec - (start).tv_sec) * 1000000) + \
         ((end).tv_usec - (start).tv_usec);
namespace android {

namespace {

constexpr char COMPONENT_NAME[] = "c2.allwinner.mpeg4.decoder";
constexpr uint32_t kDefaultOutputDelay = 9;
constexpr uint32_t kMaxOutputDelay = 20;
}  // namespace

class Mpeg4DecComponent::IntfImpl : public HwC2Interface<void>::BaseParams {
 public:
  explicit IntfImpl(const std::shared_ptr<C2ReflectorHelper> &helper)
      : HwC2Interface<void>::BaseParams(
            helper, COMPONENT_NAME, C2Component::KIND_DECODER,
            C2Component::DOMAIN_VIDEO, MEDIA_MIMETYPE_VIDEO_MPEG4) {
    noPrivateBuffers();
    noInputReferences();
    noOutputReferences();
    noInputLatency();
    noTimeStretch();
    mParaHelper = std::make_shared<VdecParameterHelper>();

    addParameter(
        DefineParam(mActualOutputDelay, C2_PARAMKEY_OUTPUT_DELAY)
            .withDefault(
                new C2PortActualDelayTuning::output(kDefaultOutputDelay))
            .withFields(
                {C2F(mActualOutputDelay, value).inRange(0, kMaxOutputDelay)})
            .withSetter(
                Setter<decltype(*mActualOutputDelay)>::StrictValueWithNoDeps)
            .build());

    addParameter(DefineParam(mAttrib, C2_PARAMKEY_COMPONENT_ATTRIBUTES)
                     .withConstValue(new C2ComponentAttributesSetting(
                         C2Component::ATTRIB_IS_TEMPORAL))
                     .build());

    // coded and output picture size is the same for this codec
    addParameter(
        DefineParam(mParaHelper->mSize, C2_PARAMKEY_PICTURE_SIZE)
            .withDefault(new C2StreamPictureSizeInfo::output(0u, 320, 240))
            .withFields({
                C2F(mParaHelper->mSize, width).inRange(128, 1920, 2),
                C2F(mParaHelper->mSize, height).inRange(96, 1088, 2),
            })
            .withSetter(SizeSetter)
            .build());

    addParameter(
        DefineParam(mParaHelper->mProfileLevel, C2_PARAMKEY_PROFILE_LEVEL)
            .withDefault(new C2StreamProfileLevelInfo::input(
                0u, C2Config::PROFILE_MP4V_SIMPLE, C2Config::LEVEL_MP4V_3))
            .withFields(
                {C2F(mParaHelper->mProfileLevel, profile)
                     .equalTo(C2Config::PROFILE_MP4V_SIMPLE),
                 C2F(mParaHelper->mProfileLevel, level)
                     .oneOf({C2Config::LEVEL_MP4V_0, C2Config::LEVEL_MP4V_0B,
                             C2Config::LEVEL_MP4V_1, C2Config::LEVEL_MP4V_2,
                             C2Config::LEVEL_MP4V_3, C2Config::LEVEL_MP4V_3B,
                             C2Config::LEVEL_MP4V_4, C2Config::LEVEL_MP4V_4A,
                             C2Config::LEVEL_MP4V_5, C2Config::LEVEL_MP4V_6})})
            .withSetter(ProfileLevelSetter, mParaHelper->mSize)
            .build());

    addParameter(
        DefineParam(mParaHelper->mMaxSize, C2_PARAMKEY_MAX_PICTURE_SIZE)
            .withDefault(
                new C2StreamMaxPictureSizeTuning::output(0u, 1920, 1088))
            .withFields({
                C2F(mParaHelper->mSize, width).inRange(176, 1920, 16),
                C2F(mParaHelper->mSize, height).inRange(144, 1088, 16),
            })
            .withSetter(MaxPictureSizeSetter, mParaHelper->mSize)
            .build());

    addParameter(DefineParam(mParaHelper->mMaxInputSize,
                             C2_PARAMKEY_INPUT_MAX_BUFFER_SIZE)
                     .withDefault(new C2StreamMaxBufferSizeInfo::input(
                         0u, 1920 * 1088 * 3 / 2))
                     .withFields({
                         C2F(mParaHelper->mMaxInputSize, value).any(),
                     })
                     .calculatedAs(MaxInputSizeSetter, mParaHelper->mMaxSize)
                     .build());

    C2ChromaOffsetStruct locations[1] = {C2ChromaOffsetStruct::ITU_YUV_420_0()};
    std::shared_ptr<C2StreamColorInfo::output> defaultColorInfo =
        C2StreamColorInfo::output::AllocShared(1u, 0u, 8u /* bitDepth */,
                                               C2Color::YUV_420);
    memcpy(defaultColorInfo->m.locations, locations, sizeof(locations));

    defaultColorInfo = C2StreamColorInfo::output::AllocShared(
        {C2ChromaOffsetStruct::ITU_YUV_420_0()}, 0u, 8u /* bitDepth */,
        C2Color::YUV_420);
    helper->addStructDescriptors<C2ChromaOffsetStruct>();

    addParameter(
        DefineParam(mParaHelper->mColorInfo, C2_PARAMKEY_CODED_COLOR_INFO)
            .withConstValue(defaultColorInfo)
            .build());

    addParameter(
        DefineParam(mParaHelper->mPixelFormat, C2_PARAMKEY_PIXEL_FORMAT)
            .withConstValue(new C2StreamPixelFormatInfo::output(
                0u, HAL_PIXEL_FORMAT_YCBCR_420_888))
            .build());
  }

  static C2R SizeSetter(bool /*mayBlock*/,
                        const C2P<C2StreamPictureSizeInfo::output> &oldMe,
                        C2P<C2StreamPictureSizeInfo::output> &me) {
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

  static C2R MaxPictureSizeSetter(
      bool /*mayBlock*/, C2P<C2StreamMaxPictureSizeTuning::output> &me,
      const C2P<C2StreamPictureSizeInfo::output> &size) {
    // TODO: get max width/height from the size's field helpers vs. hardcoding
    me.set().width = c2_min(c2_max(me.v.width, size.v.width), 1920u);
    me.set().height = c2_min(c2_max(me.v.height, size.v.height), 1088u);
    return C2R::Ok();
  }

  static C2R MaxInputSizeSetter(
      bool /*mayBlock*/, C2P<C2StreamMaxBufferSizeInfo::input> &me,
      const C2P<C2StreamMaxPictureSizeTuning::output> &maxSize) {
    // assume compression ratio of 2
    me.set().value =
        (((maxSize.v.width + 15) / 16) * ((maxSize.v.height + 15) / 16) * 384);
    return C2R::Ok();
  }

  static C2R ProfileLevelSetter(
      bool /*mayBlock*/, C2P<C2StreamProfileLevelInfo::input> & /*me*/,
      const C2P<C2StreamPictureSizeInfo::output> & /*size*/) {
    return C2R::Ok();
  }
  std::shared_ptr<VdecParameterHelper> mParaHelper;
};

void Mpeg4DecComponent::handleWorkCb2(std::unique_ptr<C2Work> work) {
  handleWorkCb(std::move(work));
}

Mpeg4DecComponent::Mpeg4DecComponent(const char *name, c2_node_id_t id,
                                     const std::shared_ptr<IntfImpl> &intfImpl)
    : HwC2Component(
          std::make_shared<HwC2Interface<IntfImpl>>(name, id, intfImpl)),
      mIntf(intfImpl) {
  mCodec = std::make_unique<VdecComponent>(name, mIntf->mParaHelper);
  mCodec->setWorkHandlerCb(std::bind(&Mpeg4DecComponent::handleWorkCb2, this,
                                     std::placeholders::_1));
  mCodec->setIntf(mIntf);
}

Mpeg4DecComponent::~Mpeg4DecComponent() {}

c2_status_t Mpeg4DecComponent::onInit() { return mCodec->onInit(); }

c2_status_t Mpeg4DecComponent::onStart() { return mCodec->onStart(); }

c2_status_t Mpeg4DecComponent::onStop() { return mCodec->onStop(); }

void Mpeg4DecComponent::onReset() { mCodec->onReset(); }

void Mpeg4DecComponent::onRelease() { mCodec->onRelease(); }

c2_status_t Mpeg4DecComponent::onFlush_sm() { return mCodec->onFlush_sm(); }

void Mpeg4DecComponent::process(const std::unique_ptr<C2Work> &work,
                                const std::shared_ptr<C2BlockPool> &pool) {
  mCodec->process(work, pool);
}

c2_status_t Mpeg4DecComponent::drain(uint32_t drainMode,
                                     const std::shared_ptr<C2BlockPool> &pool) {
  return mCodec->drain(drainMode, pool);
}

class Mpeg4DecComponentFactory : public C2ComponentFactory {
 public:
  Mpeg4DecComponentFactory()
      : mHelper(std::static_pointer_cast<C2ReflectorHelper>(
            GetCodec2HwComponentStore()->getParamReflector())) {}

  c2_status_t createComponent(
      c2_node_id_t id, std::shared_ptr<C2Component> *const component,
      std::function<void(C2Component *)> deleter) override {
    *component = std::shared_ptr<C2Component>(
        new Mpeg4DecComponent(
            COMPONENT_NAME, id,
            std::make_shared<Mpeg4DecComponent::IntfImpl>(mHelper)),
        deleter);
    return C2_OK;
  }

  c2_status_t createInterface(
      c2_node_id_t id, std::shared_ptr<C2ComponentInterface> *const interface,
      std::function<void(C2ComponentInterface *)> deleter) override {
    *interface = std::shared_ptr<C2ComponentInterface>(
        new HwC2Interface<Mpeg4DecComponent::IntfImpl>(
            COMPONENT_NAME, id,
            std::make_shared<Mpeg4DecComponent::IntfImpl>(mHelper)),
        deleter);
    return C2_OK;
  }

  ~Mpeg4DecComponentFactory() override = default;

 private:
  std::shared_ptr<C2ReflectorHelper> mHelper;
};

}  // namespace android

extern "C" ::C2ComponentFactory *CreateCodec2Factory() {
  return new ::android::Mpeg4DecComponentFactory();
}

extern "C" void DestroyCodec2Factory(::C2ComponentFactory *factory) {
  delete factory;
}
