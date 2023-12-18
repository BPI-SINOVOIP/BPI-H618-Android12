/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef ANDROID_MEDIA_ECO_SERVICE_STATS_PROVIDER_H_
#define ANDROID_MEDIA_ECO_SERVICE_STATS_PROVIDER_H_

#include <android/media/eco/BnECOServiceStatsProvider.h>
#include <android/media/eco/IECOSession.h>
#include <android/media/eco/IECOService.h>
#include <binder/BinderService.h>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include "ECOData.h"
#include "ECOServiceConstants.h"

namespace android {
namespace media {
namespace eco {

using ::android::binder::Status;

/**
 * ECOServiceStatsProvider interface class.
 */
class ECOServiceStatsProvider : public BinderService<IECOServiceStatsProvider>,
                                public BnECOServiceStatsProvider,
                                public virtual IBinder::DeathRecipient {
    friend class BinderService<IECOServiceStatsProvider>;

public:
    virtual ~ECOServiceStatsProvider() {}

    virtual Status getType(int32_t* _aidl_return);
    virtual Status getName(::android::String16* _aidl_return);
    virtual Status getECOSession(::android::sp<::android::IBinder>* _aidl_return);
    virtual Status isCameraRecording(bool* _aidl_return);

    // IBinder::DeathRecipient implementation
    virtual void binderDied(const wp<IBinder>& who);

    bool updateStats(const ECOData& data);
    bool addProvider();
    bool removeProvider();
    float getFramerate(int64_t currTimestamp);
    static android::sp<ECOServiceStatsProvider> create(
        int32_t width, int32_t height, bool isCameraRecording, const char* name);

private:
    ECOServiceStatsProvider(int32_t width, int32_t height, bool isCameraRecording,
                            android::sp<IECOSession>& session, const char* name);
    int32_t mWidth = 0;
    int32_t mHeight = 0;
    bool mIsCameraRecording = false;
    android::sp<IECOSession> mECOSession = nullptr;
    const char* mProviderName = nullptr;
    int64_t mLastFrameTimestamp = 0;
};

}  // namespace eco
}  // namespace media
}  // namespace android

#endif  // ANDROID_MEDIA_ECO_SERVICE_STATS_PROVIDER_H_
