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
#define LOG_TAG "android.hardware.media.aw.c2@1.0-service"

#include <binder/ProcessState.h>
#include <codec2/hidl/1.0/ComponentStore.h>
#include <hidl/HidlTransportSupport.h>
#include <minijail.h>

#include <C2Component.h>

#include <C2HwSupport.h>
#include <C2Store.h>
#include <vdecoder.h>

// This is created by module "codec2.vendor.base.policy". This can be modified.
static constexpr char kBaseSeccompPolicyPath[] =
    "/vendor/etc/seccomp_policy/codec2.vendor.base.policy";

// Additional device-specific seccomp permissions can be added in this file.
static constexpr char kExtSeccompPolicyPath[] =
    "/vendor/etc/seccomp_policy/codec2.vendor.ext.policy";

int main(int /* argc */, char** /* argv */) {
  ALOGD("android.hardware.media.aw.c2@1.0-service starting...");

  signal(SIGPIPE, SIG_IGN);
  android::SetUpMinijail(kBaseSeccompPolicyPath, kExtSeccompPolicyPath);

  // vndbinder is needed by BufferQueue.
  android::ProcessState::initWithDriver("/dev/vndbinder");
  android::ProcessState::self()->startThreadPool();

  // Extra threads may be needed to handle a stacked IPC sequence that
  // contains alternating binder and hwbinder calls. (See b/35283480.)
  android::hardware::configureRpcThreadpool(8, true /* callerWillJoin */);

  // Create IComponentStore service.
  {
    // using namespace ::android::hardware::media::c2::V1_0;
    using android::hardware::media::c2::V1_0::IComponentStore;
    using android::hardware::media::c2::V1_0::utils::ComponentStore;
    android::sp<IComponentStore> store;

    // Vendor's TODO: Replace this with
    ALOGD("Instantiating Codec2's vendor IComponentStore service......");

    store = new ComponentStore(android::GetCodec2HwComponentStore());
    if (store == nullptr) {
      ALOGE("Cannot create Codec2's IComponentStore service.");
    } else {
      if (store->registerAsService("default") != android::OK) {
        ALOGE(
            "Cannot register Codec2's "
            "IComponentStore service.");
      } else {
        ALOGI("Codec2's IComponentStore service created.");
      }
    }
    AddVDPlugin();
  }

  android::hardware::joinRpcThreadpool();
  return 0;
}
