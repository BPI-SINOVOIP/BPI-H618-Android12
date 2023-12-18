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

#include "EvsServiceContext.h"

#include <android-base/logging.h>
#include <android/hardware_buffer.h>
#include <android_runtime/android_hardware_HardwareBuffer.h>
#include <nativehelper/JNIHelp.h>
#include <vndk/hardware_buffer.h>

using android::hardware::automotive::evs::V1_0::EvsResult;
using namespace android::hardware::automotive::evs::V1_1;

namespace {

jmethodID getMethodIDOrDie(JNIEnv* env, jclass clazz, const char* name, const char* signature) {
    jmethodID res = env->GetMethodID(clazz, name, signature);
    LOG_ALWAYS_FATAL_IF(res == nullptr, "Unable to find method %s with signature %s", name,
                        signature);

    return res;
}

}  // namespace

namespace android {
namespace automotive {
namespace evs {

// "default" is reserved for the latest version of EVS manager.
const char* EvsServiceContext::kServiceName = "default";

EvsServiceContext::EvsServiceContext(JavaVM* vm, jclass clazz) :
      mVm(vm), mCallbackThread(vm), mCarEvsServiceObj(nullptr) {
    JNIEnv* env = nullptr;
    vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_4);
    if (env != nullptr) {
        // Registers post-native handlers
        mDeathHandlerMethodId = getMethodIDOrDie(env, clazz, "postNativeDeathHandler", "()V");
        mEventHandlerMethodId = getMethodIDOrDie(env, clazz, "postNativeEventHandler", "(I)V");
        mFrameHandlerMethodId = getMethodIDOrDie(env, clazz, "postNativeFrameHandler",
                                                 "(ILandroid/hardware/HardwareBuffer;)V");
    } else {
        jniThrowException(env, "java/lang/IllegalArgumentException",
                          "Failed to get JNIEnv from a given VM instance.");
    }
}

EvsServiceContext::~EvsServiceContext() {
    {
        std::lock_guard<std::mutex> lock(mLock);
        if (mService != nullptr) {
            mService->unlinkToDeath(mDeathRecipient);
        }
        mService = nullptr;
        mCamera = nullptr;
        mStreamHandler = nullptr;
    }

    // Stops the callback thread
    mCallbackThread.stop();

    // Deletes a global reference to the CarEvsService object
    JNIEnv* env = nullptr;
    if (mVm != nullptr) {
        mVm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_4);
        if (env != nullptr) {
            env->DeleteGlobalRef(mCarEvsServiceObj);
        }
    }
}

bool EvsServiceContext::initialize(JNIEnv* env, jobject thiz) {
    sp<IEvsEnumerator> service = IEvsEnumerator::tryGetService(EvsServiceContext::kServiceName);
    if (!service) {
        // TODO(b/177923058): it may be desired to retry a few times if the
        // connection fails.
        LOG(ERROR) << "Failed to connect to EVS service.";
        return false;
    }

    sp<EvsDeathRecipient> deathRecipient = new EvsDeathRecipient(service, this);
    auto ret = service->linkToDeath(deathRecipient, /*cookie=*/0);
    if (!ret.isOk() || ret == false) {
        LOG(ERROR) << "Failed to register a death recipient; the service may die.";
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(mLock);
        mService = service;
        mDeathRecipient = deathRecipient;
        if (!mCarEvsServiceObj) {
            mCarEvsServiceObj = env->NewGlobalRef(thiz);
        }
    }

    return true;
}

bool EvsServiceContext::openCamera(const char* id) {
    if (!isAvailable()) {
        LOG(ERROR) << "Has not connected to EVS service yet.";
        return false;
    }

    if (isCameraOpened()) {
        if (!strcmp(id, mCameraIdInUse)) {
            LOG(DEBUG) << "Camera " << id << " is has opened already.";
            return true;
        } else {
            // Close a current camera device.
            mService->closeCamera(mCamera);
        }
    }

    sp<IEvsCamera> camera = IEvsCamera::castFrom(mService->openCamera(id));
    if (!camera) {
        LOG(ERROR) << "Failed to open a camera " << id;
        return false;
    }

    sp<StreamHandler> streamHandler =
            new StreamHandler(camera, this, EvsServiceContext::kMaxNumFramesInFlight);
    if (!streamHandler) {
        LOG(ERROR) << "Failed to initialize a stream streamHandler.";
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(mLock);
        mCamera = camera;
        mStreamHandler = streamHandler;
        mCameraIdInUse = id;
    }

    return true;
}

void EvsServiceContext::closeCamera() {
    if (!isCameraOpened()) {
        LOG(DEBUG) << "Camera has not opened yet.";
        return;
    }

    mService->closeCamera(mCamera);
}

bool EvsServiceContext::startVideoStream() {
    if (!isCameraOpened()) {
        LOG(ERROR) << "Camera has not opened yet.";
        return JNI_FALSE;
    }

    return mStreamHandler->startStream();
}

void EvsServiceContext::stopVideoStream() {
    if (!isCameraOpened()) {
        LOG(DEBUG) << "Camera has not opened; a request to stop a video steram is ignored.";
        return;
    }

    if (!mStreamHandler->asyncStopStream()) {
        LOG(WARNING) << "Failed to stop a video stream.  EVS service may die.";
    }
}

void EvsServiceContext::acquireCameraAndDisplay() {
    // Acquires the display ownership.  Because EVS awards this to the single
    // client, no other clients can use EvsDisplay as long as CarEvsManager
    // alives.
    mDisplay = mService->openDisplay_1_1(EvsServiceContext::kExclusiveMainDisplayId);
    if (!mDisplay) {
        LOG(WARNING) << "Failed to acquire the display ownership.  "
                     << "CarEvsManager may not be able to render "
                     << "the contents on the screen.";
        return;
    }

    // Attempts to become a primary owner
    auto ret = mCamera->forceMaster(mDisplay);
    if (!ret.isOk() || ret != EvsResult::OK) {
        LOG(WARNING) << "Failed to own a camera device.";
    }
}

void EvsServiceContext::doneWithFrame(int bufferId) {
    BufferDesc bufferToReturn;
    {
        std::lock_guard<std::mutex> lock(mLock);
        auto it = mBufferRecords.find(bufferId);
        if (it == mBufferRecords.end()) {
            LOG(WARNING) << "Unknown buffer is requested to return.";
            return;
        }

        bufferToReturn = it->second;
        mBufferRecords.erase(it);
    }
    mStreamHandler->doneWithFrame(bufferToReturn);

    // If this is the first frame since current video stream started, we'd claim
    // the exclusive ownership of the camera and the display and keep for the rest
    // of the lifespan.
    std::call_once(mDisplayAcquired, &EvsServiceContext::acquireCameraAndDisplay, this);
}

/*
 * Forwards EVS stream events to the client.  This method will run in the
 * context of EvsCallbackThread.
 */
void EvsServiceContext::onNewEvent(EvsEventDesc event) {
    mCallbackThread.enqueue([event, this](JNIEnv* env) {
        // Gives an event callback
        env->CallVoidMethod(mCarEvsServiceObj, mEventHandlerMethodId,
                            static_cast<jint>(event.aType));
    });
}

/*
 * Forwards EVS frames to the client.  This method will run in the context of
 * EvsCallbackThread.
 */
void EvsServiceContext::onNewFrame(BufferDesc bufferDesc) {
    mCallbackThread.enqueue([bufferDesc, this](JNIEnv* env) {
        const auto createFromAHardwareBuffer =
                android::android_hardware_HardwareBuffer_createFromAHardwareBuffer;

        // Clones a AHardwareBuffer
        AHardwareBuffer_Desc const* desc =
                reinterpret_cast<AHardwareBuffer_Desc const*>(&bufferDesc.buffer.description);

        AHardwareBuffer* rawBuffer;
        auto status =
                AHardwareBuffer_createFromHandle(desc, bufferDesc.buffer.nativeHandle,
                                                 AHARDWAREBUFFER_CREATE_FROM_HANDLE_METHOD_CLONE,
                                                 &rawBuffer);
        if (status != android::NO_ERROR) {
            LOG(ERROR) << "Failed to create a raw hardware buffer from a native handle.";
            mStreamHandler->doneWithFrame(bufferDesc);
        } else {
            {
                std::lock_guard<std::mutex> lock(mLock);
                mBufferRecords.try_emplace(bufferDesc.bufferId, bufferDesc);
            }

            // Calls back
            jobject hwBuffer = createFromAHardwareBuffer(env, rawBuffer);
            if (!hwBuffer) {
                LOG(WARNING) << "Failed to create HardwareBuffer from AHardwareBuffer.";
                mStreamHandler->doneWithFrame(bufferDesc);
            } else {
                env->CallVoidMethod(mCarEvsServiceObj, mFrameHandlerMethodId, bufferDesc.bufferId,
                                    hwBuffer);
            }
            env->DeleteLocalRef(hwBuffer);
            AHardwareBuffer_release(rawBuffer);
        }
    });
}

/*
 * Handles an unexpected death of EVS service.  This method will run in the
 * context of EvsCallbackThread.
 */
void EvsServiceContext::onServiceDied(const wp<hidl::base::V1_0::IBase>& who) {
    mCallbackThread.enqueue([who, this](JNIEnv* env) {
        if (!isEqual(who)) {
            // We're not interested in this death notification.
            return;
        }

        // Drops invalidated service handles.  We will re-initialize them when
        // we try to reconnect.  The buffer record would be cleared safely
        // because all buffer references get invalidated upon the death of the
        // native EVS service.
        {
            std::lock_guard<std::mutex> lock(mLock);
            mCamera = nullptr;
            mService = nullptr;
            mStreamHandler = nullptr;
            mBufferRecords.clear();
        }

        LOG(ERROR) << "The native EVS service has died.";
        // EVS service has died but CarEvsManager instance still alives.
        // Only a service handle needs to be destroyed; this will be
        // re-created when CarEvsManager successfully connects to EVS service
        // when it comes back.
        env->CallVoidMethod(mCarEvsServiceObj, mDeathHandlerMethodId);
    });
}

}  // namespace evs
}  // namespace automotive
}  // namespace android
