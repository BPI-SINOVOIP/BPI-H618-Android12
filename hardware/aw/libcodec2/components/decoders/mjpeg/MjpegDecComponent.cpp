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
#define LOG_TAG "MjpegDecComponent"
#include "MjpegDecComponent.h"

#include <C2Debug.h>
#include <C2PlatformSupport.h>
#include <Codec2Mapper.h>
#include <media/stagefright/foundation/MediaDefs.h>
#include <time.h>

#include <memory>
#include <utility>

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

constexpr char COMPONENT_NAME[] = "c2.allwinner.mjpeg.decoder";
constexpr uint32_t kDefaultOutputDelay = 9;
constexpr uint32_t kMaxOutputDelay = 20;
}  // namespace

class MjpegDecComponent::IntfImpl : public HwC2Interface<void>::BaseParams {
 public:
  explicit IntfImpl(const std::shared_ptr<C2ReflectorHelper> &helper)
      : HwC2Interface<void>::BaseParams(
            helper, COMPONENT_NAME, C2Component::KIND_DECODER,
            C2Component::DOMAIN_VIDEO, MEDIA_MIMETYPE_VIDEO_MJPEG) {
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
    me.set().width = c2_min(c2_max(me.v.width, size.v.width), 4080u);
    me.set().height = c2_min(c2_max(me.v.height, size.v.height), 4080u);
    return C2R::Ok();
  }

  static C2R MaxInputSizeSetter(
      bool /*mayBlock*/, C2P<C2StreamMaxBufferSizeInfo::input> &me,
      const C2P<C2StreamMaxPictureSizeTuning::output> &maxSize) {
    // assume compression ratio of 2
    me.set().value =
        (((maxSize.v.width + 15) / 16) * ((maxSize.v.height + 15) / 16) * 192);
    return C2R::Ok();
  }

  static C2R ProfileLevelSetter(
      bool /*mayBlock*/, C2P<C2StreamProfileLevelInfo::input> & /*me*/,
      const C2P<C2StreamPictureSizeInfo::output> & /*size*/) {
    return C2R::Ok();
  }

  static C2R DefaultColorAspectsSetter(
      bool /*mayBlock*/, C2P<C2StreamColorAspectsTuning::output> &me) {
    if (me.v.range > C2Color::RANGE_OTHER) {
      me.set().range = C2Color::RANGE_OTHER;
    }
    if (me.v.primaries > C2Color::PRIMARIES_OTHER) {
      me.set().primaries = C2Color::PRIMARIES_OTHER;
    }
    if (me.v.transfer > C2Color::TRANSFER_OTHER) {
      me.set().transfer = C2Color::TRANSFER_OTHER;
    }
    if (me.v.matrix > C2Color::MATRIX_OTHER) {
      me.set().matrix = C2Color::MATRIX_OTHER;
    }
    return C2R::Ok();
  }

  static C2R CodedColorAspectsSetter(bool /*mayBlock*/,
                                     C2P<C2StreamColorAspectsInfo::input> &me) {
    if (me.v.range > C2Color::RANGE_OTHER) {
      me.set().range = C2Color::RANGE_OTHER;
    }
    if (me.v.primaries > C2Color::PRIMARIES_OTHER) {
      me.set().primaries = C2Color::PRIMARIES_OTHER;
    }
    if (me.v.transfer > C2Color::TRANSFER_OTHER) {
      me.set().transfer = C2Color::TRANSFER_OTHER;
    }
    if (me.v.matrix > C2Color::MATRIX_OTHER) {
      me.set().matrix = C2Color::MATRIX_OTHER;
    }
    return C2R::Ok();
  }

  static C2R ColorAspectsSetter(
      bool /*mayBlock*/, C2P<C2StreamColorAspectsInfo::output> &me,
      const C2P<C2StreamColorAspectsTuning::output> &def,
      const C2P<C2StreamColorAspectsInfo::input> &coded) {
    me.set().range =
        coded.v.range == RANGE_UNSPECIFIED ? def.v.range : coded.v.range;
    me.set().primaries = coded.v.primaries == PRIMARIES_UNSPECIFIED
                             ? def.v.primaries
                             : coded.v.primaries;
    me.set().transfer = coded.v.transfer == TRANSFER_UNSPECIFIED
                            ? def.v.transfer
                            : coded.v.transfer;
    me.set().matrix =
        coded.v.matrix == MATRIX_UNSPECIFIED ? def.v.matrix : coded.v.matrix;
    return C2R::Ok();
  }

  std::shared_ptr<VdecParameterHelper> mParaHelper;
};

void MjpegDecComponent::handleWorkCb2(std::unique_ptr<C2Work> work) {
  handleWorkCb(std::move(work));
}

MjpegDecComponent::MjpegDecComponent(const char *name, c2_node_id_t id,
                                     const std::shared_ptr<IntfImpl> &intfImpl)
    : HwC2Component(
          std::make_shared<HwC2Interface<IntfImpl>>(name, id, intfImpl)),
      mIntf(intfImpl) {
  mCodec = std::make_unique<VdecComponent>(name, mIntf->mParaHelper);
  mCodec->setNeedsCodecConfig(false);
  mCodec->setWorkHandlerCb(std::bind(&MjpegDecComponent::handleWorkCb2, this,
                                     std::placeholders::_1));
  mCodec->setIntf(mIntf);
}

MjpegDecComponent::~MjpegDecComponent() {}

c2_status_t MjpegDecComponent::onInit() { return mCodec->onInit(); }

c2_status_t MjpegDecComponent::onStart() { return mCodec->onStart(); }

c2_status_t MjpegDecComponent::onStop() { return mCodec->onStop(); }

void MjpegDecComponent::onReset() { mCodec->onReset(); }

void MjpegDecComponent::onRelease() { mCodec->onRelease(); }

c2_status_t MjpegDecComponent::onFlush_sm() { return mCodec->onFlush_sm(); }

void MjpegDecComponent::process(const std::unique_ptr<C2Work> &work,
                                const std::shared_ptr<C2BlockPool> &pool) {
  mCodec->process(work, pool);
}

c2_status_t MjpegDecComponent::drain(uint32_t drainMode,
                                     const std::shared_ptr<C2BlockPool> &pool) {
  return mCodec->drain(drainMode, pool);
}

class MjpegDecComponentFactory : public C2ComponentFactory {
 public:
  MjpegDecComponentFactory()
      : mHelper(std::static_pointer_cast<C2ReflectorHelper>(
            GetCodec2HwComponentStore()->getParamReflector())) {}

  c2_status_t createComponent(
      c2_node_id_t id, std::shared_ptr<C2Component> *const component,
      std::function<void(C2Component *)> deleter) override {
    *component = std::shared_ptr<C2Component>(
        new MjpegDecComponent(
            COMPONENT_NAME, id,
            std::make_shared<MjpegDecComponent::IntfImpl>(mHelper)),
        deleter);
    return C2_OK;
  }

  c2_status_t createInterface(
      c2_node_id_t id, std::shared_ptr<C2ComponentInterface> *const interface,
      std::function<void(C2ComponentInterface *)> deleter) override {
    *interface = std::shared_ptr<C2ComponentInterface>(
        new HwC2Interface<MjpegDecComponent::IntfImpl>(
            COMPONENT_NAME, id,
            std::make_shared<MjpegDecComponent::IntfImpl>(mHelper)),
        deleter);
    return C2_OK;
  }

  ~MjpegDecComponentFactory() override = default;

 private:
  std::shared_ptr<C2ReflectorHelper> mHelper;
};

}  // namespace android

extern "C" ::C2ComponentFactory *CreateCodec2Factory() {
  return new ::android::MjpegDecComponentFactory();
}

extern "C" void DestroyCodec2Factory(::C2ComponentFactory *factory) {
  delete factory;
}
