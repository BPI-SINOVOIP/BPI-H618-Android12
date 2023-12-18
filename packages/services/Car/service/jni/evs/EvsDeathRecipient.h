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
#ifndef ANDROID_CARSERVICE_EVS_DEATH_RECIPIENT_H
#define ANDROID_CARSERVICE_EVS_DEATH_RECIPIENT_H

#include "EvsServiceCallback.h"

#include <android/hardware/automotive/evs/1.1/IEvsEnumerator.h>
#include <android/hidl/base/1.0/IBase.h>
#include <binder/Binder.h>
#include <hidl/HidlSupport.h>

namespace android {
namespace automotive {
namespace evs {

class EvsDeathRecipient : public android::hardware::hidl_death_recipient {
public:
    explicit EvsDeathRecipient(
            const android::sp<android::hardware::automotive::evs::V1_1::IEvsEnumerator>& svc,
            EvsServiceCallback* callback);
    void serviceDied(uint64_t cookie,
                     const android::wp<android::hidl::base::V1_0::IBase>& who) override;

private:
    android::sp<android::hardware::automotive::evs::V1_1::IEvsEnumerator> mService;
    EvsServiceCallback* mCallback;
};

}  // namespace evs
}  // namespace automotive
}  // namespace android

#endif  // ANDROID_CARSERVICE_EVS_DEATH_RECIPIENT_H
