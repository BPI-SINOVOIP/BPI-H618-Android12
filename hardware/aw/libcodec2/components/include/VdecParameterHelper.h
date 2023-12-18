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

#ifndef ANDROID_COMPONENTS_INCLUDE_VDECPARAMETERHELPER_H
#define ANDROID_COMPONENTS_INCLUDE_VDECPARAMETERHELPER_H

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

namespace android {

class VdecParameterHelper {
 public:
  // TODO(kay): add lock!!
  std::shared_ptr<C2StreamColorAspectsInfo::output> getColorAspects_l() {
    return mColorAspects;
  }
  /***********************For decoder start***********************/
  std::shared_ptr<C2StreamPictureSizeInfo::output> mSize;
  std::shared_ptr<C2StreamProfileLevelInfo::input> mProfileLevel;
  std::shared_ptr<C2StreamMaxPictureSizeTuning::output> mMaxSize;
  std::shared_ptr<C2StreamMaxBufferSizeInfo::input> mMaxInputSize;
  std::shared_ptr<C2StreamColorInfo::output> mColorInfo;
  std::shared_ptr<C2StreamColorAspectsInfo::input> mCodedColorAspects;
  std::shared_ptr<C2StreamColorAspectsTuning::output> mDefaultColorAspects;
  std::shared_ptr<C2StreamPixelFormatInfo::output> mPixelFormat;
  std::shared_ptr<C2StreamColorAspectsInfo::output> mColorAspects;
  /***********************For decoder end***********************/
};

}  // namespace android

#endif  // ANDROID_COMPONENTS_INCLUDE_VDECPARAMETERHELPER_H
