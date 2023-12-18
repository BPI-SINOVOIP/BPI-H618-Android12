/*
 * Copyright 2020 The Android Open Source Project
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

#define LOG_TAG "BluetoothActivityAttributionJni"

#include <string.h>

#include <shared_mutex>

#include "base/logging.h"
#include "com_android_bluetooth.h"
#include "hardware/bt_activity_attribution.h"

using bluetooth::activity_attribution::ActivityAttributionCallbacks;
using bluetooth::activity_attribution::ActivityAttributionInterface;

namespace android {
static jmethodID method_onWakeup;
static jmethodID method_onActivityLogsReady;

static ActivityAttributionInterface* sActivityAttributionInterface = nullptr;
static std::shared_timed_mutex interface_mutex;

static jobject mCallbacksObj = nullptr;
static std::shared_timed_mutex callbacks_mutex;

class ActivityAttributionCallbacksImpl : public ActivityAttributionCallbacks {
 public:
  ~ActivityAttributionCallbacksImpl() = default;

  void OnWakeup(const Activity activity, const RawAddress& bd_addr) override {
    LOG(INFO) << __func__;

    std::shared_lock<std::shared_timed_mutex> lock(callbacks_mutex);
    CallbackEnv sCallbackEnv(__func__);
    if (!sCallbackEnv.valid() || mCallbacksObj == nullptr) return;

    ScopedLocalRef<jbyteArray> addr(
        sCallbackEnv.get(), sCallbackEnv->NewByteArray(sizeof(RawAddress)));
    if (!addr.get()) {
      LOG(ERROR)
          << "Failed to allocate jbyteArray for bd_addr of wakeup callback";
      return;
    }

    sCallbackEnv->SetByteArrayRegion(addr.get(), 0, sizeof(RawAddress),
                                     (jbyte*)&bd_addr);
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onWakeup, (jint)activity,
                                 addr.get());
  }

  void OnActivityLogsReady(
      const std::vector<BtaaAggregationEntry> logs) override {
    LOG(INFO) << __func__;

    std::shared_lock<std::shared_timed_mutex> lock(callbacks_mutex);
    CallbackEnv sCallbackEnv(__func__);
    if (!sCallbackEnv.valid() || mCallbacksObj == nullptr) return;

    jsize logs_size = logs.size() * sizeof(BtaaAggregationEntry);
    ScopedLocalRef<jbyteArray> addr(sCallbackEnv.get(),
                                    sCallbackEnv->NewByteArray(logs_size));
    if (!addr.get()) {
      LOG(ERROR) << "Failed to allocate jbyteArray for logs from activity "
                    "logging callback";
      return;
    }

    sCallbackEnv->SetByteArrayRegion(addr.get(), 0, logs_size,
                                     (jbyte*)logs.data());
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onActivityLogsReady,
                                 addr.get());
  }
};

static ActivityAttributionCallbacksImpl sActivityAttributionCallbacks;

static void classInitNative(JNIEnv* env, jclass clazz) {
  method_onWakeup = env->GetMethodID(clazz, "onWakeup", "(I[B)V");
  method_onActivityLogsReady =
      env->GetMethodID(clazz, "onActivityLogsReady", "([B)V");

  LOG(INFO) << __func__ << ": succeeds";
}

static void initNative(JNIEnv* env, jobject object) {
  std::unique_lock<std::shared_timed_mutex> interface_lock(interface_mutex);
  std::unique_lock<std::shared_timed_mutex> callbacks_lock(callbacks_mutex);
  const bt_interface_t* btInf = getBluetoothInterface();
  if (btInf == nullptr) {
    LOG(ERROR) << "Bluetooth module is not loaded";
    return;
  }

  if (sActivityAttributionInterface != nullptr) {
    LOG(INFO)
        << "Cleaning up ActivityAttribution Interface before initializing...";
    sActivityAttributionInterface->Cleanup();
    sActivityAttributionInterface = nullptr;
  }

  if (mCallbacksObj != nullptr) {
    LOG(INFO) << "Cleaning up ActivityAttribution callback object";
    env->DeleteGlobalRef(mCallbacksObj);
    mCallbacksObj = nullptr;
  }

  if ((mCallbacksObj = env->NewGlobalRef(object)) == nullptr) {
    LOG(ERROR)
        << "Failed to allocate Global Ref for ActivityAttribution Callbacks";
    return;
  }

  sActivityAttributionInterface =
      (ActivityAttributionInterface*)btInf->get_profile_interface(
          BT_ACTIVITY_ATTRIBUTION_ID);
  if (sActivityAttributionInterface == nullptr) {
    LOG(ERROR) << "Failed to get ActivityAttribution Interface";
    return;
  }

  sActivityAttributionInterface->RegisterCallbacks(
      &sActivityAttributionCallbacks);
}

static void cleanupNative(JNIEnv* env, jobject object) {
  std::unique_lock<std::shared_timed_mutex> interface_lock(interface_mutex);
  std::unique_lock<std::shared_timed_mutex> callbacks_lock(callbacks_mutex);

  const bt_interface_t* btInf = getBluetoothInterface();
  if (btInf == nullptr) {
    LOG(ERROR) << "Bluetooth module is not loaded";
    return;
  }

  if (sActivityAttributionInterface != nullptr) {
    sActivityAttributionInterface->Cleanup();
    sActivityAttributionInterface = nullptr;
  }

  if (mCallbacksObj != nullptr) {
    env->DeleteGlobalRef(mCallbacksObj);
    mCallbacksObj = nullptr;
  }
}

static JNINativeMethod sMethods[] = {
    {"classInitNative", "()V", (void*)classInitNative},
    {"initNative", "()V", (void*)initNative},
    {"cleanupNative", "()V", (void*)cleanupNative},
};

int register_com_android_bluetooth_btservice_activity_attribution(JNIEnv* env) {
  return jniRegisterNativeMethods(
      env,
      "com/android/bluetooth/btservice/activityattribution/"
      "ActivityAttributionNativeInterface",
      sMethods, NELEM(sMethods));
}

}  // namespace android
