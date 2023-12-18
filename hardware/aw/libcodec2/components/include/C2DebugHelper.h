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

#ifndef ANDROID_COMPONENTS_INCLUDE_C2DEBUGHELPER_H
#define ANDROID_COMPONENTS_INCLUDE_C2DEBUGHELPER_H

#include <C2Debug.h>
#include <C2PlatformSupport.h>
#include <Codec2BufferUtils.h>
#include <media/stagefright/foundation/AUtils.h>
#include <util/C2InterfaceHelper.h>
#include <utils/Vector.h>

#include <map>

namespace android {

/** Get time */
#define GETTIME(a, b) gettimeofday(a, b);

/** Compute difference between start and end */
#define TIME_DIFF(start, end, diff)                    \
  diff = (((end).tv_sec - (start).tv_sec) * 1000000) + \
         ((end).tv_usec - (start).tv_usec);

enum FLAG_WORK_OPERATION {
  FLAG_SUBMIT_WORK = 0,
  FLAG_FILL_WORK = 1,
};

class C2DebugHelper {
 public:
  bool mFirstFillWork;
  bool mFirstSubmtiStream;
  struct timeval mFirstSubmtiStreamSysPts;
  struct timeval mFirstFillWorkSysPts;
  struct timeval mLastSubmtiStreamSysPts;
  struct timeval mLastFillWorkSysPts;
  struct timeval mCurrentFillWorkSysPts;
  uint64_t mTotalFillWorkTime;
  uint64_t mFillWorkCount;
  uint64_t mAverageFillWorkTime;
  int64_t mFillWorkTimeDiff;
};

}  // namespace android

#endif  // ANDROID_COMPONENTS_INCLUDE_C2DEBUGHELPER_H
