/*
 * Copyright 2021 The Android Open Source Project
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

#include "EvsDeathRecipient.h"

#include <android-base/logging.h>

using ::android::sp;
using ::android::wp;
using ::android::hardware::automotive::evs::V1_1::IEvsEnumerator;
using ::android::hidl::base::V1_0::IBase;

namespace android {
namespace automotive {
namespace evs {

EvsDeathRecipient::EvsDeathRecipient(const sp<IEvsEnumerator>& svc, EvsServiceCallback* callback) :
      mService(svc), mCallback(callback) {}

void EvsDeathRecipient::serviceDied(uint64_t /*cookie*/, const wp<IBase>& who) {
    LOG(ERROR) << "EVS service has died.";
    if (mCallback != nullptr) {
        mCallback->onServiceDied(who);
    }
}

}  // namespace evs
}  // namespace automotive
}  // namespace android
