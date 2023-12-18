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
#ifndef ANDROID_CARSERVICE_EVSSERVICECALLBACK_H
#define ANDROID_CARSERVICE_EVSSERVICECALLBACK_H

#include <android/hardware/automotive/evs/1.1/types.h>
#include <hidl/HidlTransportSupport.h>

namespace android {
namespace automotive {
namespace evs {

/*
 * This abstract class defines callback methods to listen to the native Extended
 * View System service.
 */
class EvsServiceCallback {
public:
    virtual ~EvsServiceCallback(){};

    // Called upon the arrival of the new stream event.
    virtual void onNewEvent(hardware::automotive::evs::V1_1::EvsEventDesc) = 0;

    // Called upon the arrival of the new frame.
    virtual void onNewFrame(hardware::automotive::evs::V1_1::BufferDesc) = 0;

    // Called upon the death of the native Extended View System service.
    virtual void onServiceDied(const wp<hidl::base::V1_0::IBase>&) = 0;
};

}  // namespace evs
}  // namespace automotive
}  // namespace android

#endif  // ANDROID_CARSERVICE_EVSSERVICECALLBACK_H
