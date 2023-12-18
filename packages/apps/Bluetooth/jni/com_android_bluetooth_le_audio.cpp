/*   Copyright 2019 HIMSA II K/S - www.himsa.com
 * Represented by EHIMA - www.ehima.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "BluetoothLeAudioServiceJni"

#define LOG_NDEBUG 0

#include <hardware/bluetooth.h>

#include <array>
#include <optional>
#include <shared_mutex>

#include "com_android_bluetooth.h"
#include "hardware/bt_le_audio.h"

using bluetooth::le_audio::ConnectionState;
using bluetooth::le_audio::GroupStatus;
using bluetooth::le_audio::LeAudioClientCallbacks;
using bluetooth::le_audio::LeAudioClientInterface;

namespace android {
static jmethodID method_onConnectionStateChanged;
static jmethodID method_onGroupStatus;
static jmethodID method_onAudioConf;
static jmethodID method_onSetMemberAvailable;

static LeAudioClientInterface* sLeAudioClientInterface = nullptr;
static std::shared_timed_mutex interface_mutex;

static jobject mCallbacksObj = nullptr;
static std::shared_timed_mutex callbacks_mutex;

class LeAudioClientCallbacksImpl : public LeAudioClientCallbacks {
 public:
  ~LeAudioClientCallbacksImpl() = default;

  void OnConnectionState(ConnectionState state,
                         const RawAddress& bd_addr) override {
    LOG(INFO) << __func__ << ", state:" << int(state);

    std::shared_lock<std::shared_timed_mutex> lock(callbacks_mutex);
    CallbackEnv sCallbackEnv(__func__);
    if (!sCallbackEnv.valid() || mCallbacksObj == nullptr) return;

    ScopedLocalRef<jbyteArray> addr(
        sCallbackEnv.get(), sCallbackEnv->NewByteArray(sizeof(RawAddress)));
    if (!addr.get()) {
      LOG(ERROR) << "Failed to new jbyteArray bd addr for connection state";
      return;
    }

    sCallbackEnv->SetByteArrayRegion(addr.get(), 0, sizeof(RawAddress),
                                     (jbyte*)&bd_addr);
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onConnectionStateChanged,
                                 (jint)state, addr.get());
  }

  void OnGroupStatus(uint8_t group_id, GroupStatus group_status,
                     uint8_t group_flags) override {
    LOG(INFO) << __func__;

    std::shared_lock<std::shared_timed_mutex> lock(callbacks_mutex);
    CallbackEnv sCallbackEnv(__func__);
    if (!sCallbackEnv.valid() || mCallbacksObj == nullptr) return;

    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onGroupStatus,
                                 (jint)group_id, (jint)group_status,
                                 (jint)group_flags);
  }

  void OnAudioConf(const RawAddress& bd_addr, uint8_t direction,
                   uint8_t group_id, uint32_t sink_audio_location,
                   uint32_t source_audio_location) override {
    LOG(INFO) << __func__;

    std::shared_lock<std::shared_timed_mutex> lock(callbacks_mutex);
    CallbackEnv sCallbackEnv(__func__);
    if (!sCallbackEnv.valid() || mCallbacksObj == nullptr) return;

    ScopedLocalRef<jbyteArray> addr(
        sCallbackEnv.get(), sCallbackEnv->NewByteArray(sizeof(RawAddress)));
    if (!addr.get()) {
      LOG(ERROR) << "Failed to new jbyteArray bd addr for group status";
      return;
    }

    sCallbackEnv->SetByteArrayRegion(addr.get(), 0, sizeof(RawAddress),
                                     (jbyte*)&bd_addr);
    sCallbackEnv->CallVoidMethod(
        mCallbacksObj, method_onAudioConf, (jint)direction, (jint)group_id,
        (jint)sink_audio_location, (jint)source_audio_location, addr.get());
  }

  void OnSetMemberAvailable(const RawAddress& bd_addr,
                            uint8_t group_id) override {
    LOG(INFO) << __func__ << ", group id:" << int(group_id);

    std::shared_lock<std::shared_timed_mutex> lock(callbacks_mutex);
    CallbackEnv sCallbackEnv(__func__);
    if (!sCallbackEnv.valid() || mCallbacksObj == nullptr) return;

    ScopedLocalRef<jbyteArray> addr(
        sCallbackEnv.get(), sCallbackEnv->NewByteArray(sizeof(RawAddress)));
    if (!addr.get()) {
      LOG(ERROR) << "Failed to new jbyteArray bd addr for connection state";
      return;
    }

    sCallbackEnv->SetByteArrayRegion(addr.get(), 0, sizeof(RawAddress),
                                     (jbyte*)&bd_addr);
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onSetMemberAvailable,
                                 addr.get(), (jint)group_id);
  }
};

static LeAudioClientCallbacksImpl sLeAudioClientCallbacks;

static void classInitNative(JNIEnv* env, jclass clazz) {
  method_onGroupStatus = env->GetMethodID(clazz, "onGroupStatus", "(III)V");
  method_onAudioConf = env->GetMethodID(clazz, "onAudioConf", "(IIII[B)V");
  method_onConnectionStateChanged =
      env->GetMethodID(clazz, "onConnectionStateChanged", "(I[B)V");
  method_onSetMemberAvailable =
      env->GetMethodID(clazz, "onSetMemberAvailable", "([BI)V");
}

static void initNative(JNIEnv* env, jobject object) {
  std::unique_lock<std::shared_timed_mutex> interface_lock(interface_mutex);
  std::unique_lock<std::shared_timed_mutex> callbacks_lock(callbacks_mutex);

  const bt_interface_t* btInf = getBluetoothInterface();
  if (btInf == nullptr) {
    LOG(ERROR) << "Bluetooth module is not loaded";
    return;
  }

  if (mCallbacksObj != nullptr) {
    LOG(INFO) << "Cleaning up LeAudio callback object";
    env->DeleteGlobalRef(mCallbacksObj);
    mCallbacksObj = nullptr;
  }

  if ((mCallbacksObj = env->NewGlobalRef(object)) == nullptr) {
    LOG(ERROR) << "Failed to allocate Global Ref for LeAudio Callbacks";
    return;
  }

  sLeAudioClientInterface =
      (LeAudioClientInterface*)btInf->get_profile_interface(
          BT_PROFILE_LE_AUDIO_ID);
  if (sLeAudioClientInterface == nullptr) {
    LOG(ERROR) << "Failed to get Bluetooth LeAudio Interface";
    return;
  }

  sLeAudioClientInterface->Initialize(&sLeAudioClientCallbacks);
}

static void cleanupNative(JNIEnv* env, jobject object) {
  std::unique_lock<std::shared_timed_mutex> interface_lock(interface_mutex);
  std::unique_lock<std::shared_timed_mutex> callbacks_lock(callbacks_mutex);

  const bt_interface_t* btInf = getBluetoothInterface();
  if (btInf == nullptr) {
    LOG(ERROR) << "Bluetooth module is not loaded";
    return;
  }

  if (sLeAudioClientInterface != nullptr) {
    sLeAudioClientInterface->Cleanup();
    sLeAudioClientInterface = nullptr;
  }

  if (mCallbacksObj != nullptr) {
    env->DeleteGlobalRef(mCallbacksObj);
    mCallbacksObj = nullptr;
  }
}

static jboolean connectLeAudioNative(JNIEnv* env, jobject object,
                                     jbyteArray address) {
  LOG(INFO) << __func__;
  std::shared_lock<std::shared_timed_mutex> lock(interface_mutex);
  if (!sLeAudioClientInterface) return JNI_FALSE;

  jbyte* addr = env->GetByteArrayElements(address, nullptr);
  if (!addr) {
    jniThrowIOException(env, EINVAL);
    return JNI_FALSE;
  }

  RawAddress* tmpraw = (RawAddress*)addr;
  sLeAudioClientInterface->Connect(*tmpraw);
  env->ReleaseByteArrayElements(address, addr, 0);
  return JNI_TRUE;
}

static jboolean disconnectLeAudioNative(JNIEnv* env, jobject object,
                                        jbyteArray address) {
  LOG(INFO) << __func__;
  std::shared_lock<std::shared_timed_mutex> lock(interface_mutex);
  if (!sLeAudioClientInterface) return JNI_FALSE;

  jbyte* addr = env->GetByteArrayElements(address, nullptr);
  if (!addr) {
    jniThrowIOException(env, EINVAL);
    return JNI_FALSE;
  }

  RawAddress* tmpraw = (RawAddress*)addr;
  sLeAudioClientInterface->Disconnect(*tmpraw);
  env->ReleaseByteArrayElements(address, addr, 0);
  return JNI_TRUE;
}

static void groupStreamNative(JNIEnv* env, jobject object, jint group_id,
                              jint content_type) {
  LOG(INFO) << __func__;

  if (!sLeAudioClientInterface) {
    LOG(ERROR) << __func__ << ": Failed to get the Bluetooth LeAudio Interface";
    return;
  }

  sLeAudioClientInterface->GroupStream(group_id, content_type);
}

static void groupSuspendNative(JNIEnv* env, jobject object, jint group_id) {
  LOG(INFO) << __func__;

  if (!sLeAudioClientInterface) {
    LOG(ERROR) << __func__ << ": Failed to get the Bluetooth LeAudio Interface";
    return;
  }

  sLeAudioClientInterface->GroupSuspend(group_id);
}

static void groupStopNative(JNIEnv* env, jobject object, jint group_id) {
  LOG(INFO) << __func__;

  if (!sLeAudioClientInterface) {
    LOG(ERROR) << __func__ << ": Failed to get the Bluetooth LeAudio Interface";
    return;
  }

  sLeAudioClientInterface->GroupStop(group_id);
}

static JNINativeMethod sMethods[] = {
    {"classInitNative", "()V", (void*)classInitNative},
    {"initNative", "()V", (void*)initNative},
    {"cleanupNative", "()V", (void*)cleanupNative},
    {"connectLeAudioNative", "([B)Z", (void*)connectLeAudioNative},
    {"disconnectLeAudioNative", "([B)Z", (void*)disconnectLeAudioNative},
    {"groupStreamNative", "(II)V", (void*)groupStreamNative},
    {"groupSuspendNative", "(I)V", (void*)groupSuspendNative},
    {"groupStopNative", "(I)V", (void*)groupStopNative},
};

int register_com_android_bluetooth_le_audio(JNIEnv* env) {
  return jniRegisterNativeMethods(
      env, "com/android/bluetooth/le_audio/LeAudioNativeInterface", sMethods,
      NELEM(sMethods));
}
}  // namespace android
