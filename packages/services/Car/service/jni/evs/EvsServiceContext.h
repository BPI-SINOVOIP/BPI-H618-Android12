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
#ifndef ANDROID_CARSERVICE_EVS_SERVICE_WRAPPER_H
#define ANDROID_CARSERVICE_EVS_SERVICE_WRAPPER_H

#include "EvsCallbackThread.h"
#include "EvsDeathRecipient.h"
#include "EvsServiceCallback.h"
#include "StreamHandler.h"

#include <android/hardware/automotive/evs/1.1/IEvsDisplay.h>
#include <android/hardware/automotive/evs/1.1/IEvsEnumerator.h>
#include <android/hardware/automotive/evs/1.1/types.h>
#include <hidl/HidlTransportSupport.h>
#include <utils/Mutex.h>
#include <utils/StrongPointer.h>

#include <jni.h>

#include <mutex>

namespace android {
namespace automotive {
namespace evs {

/*
 * This class wraps around HIDL transactions to the Extended View System service
 * and the video stream managements.
 */
class EvsServiceContext : public EvsServiceCallback {
public:
    EvsServiceContext(JavaVM* vm, jclass clazz);
    virtual ~EvsServiceContext();

    /*
     * Initializes the service context and connects to the native Extended View
     * System service.
     *
     * @param env A pointer to the JNI environment
     * @param env A reference to CarEvsService object
     * @return false if it fails to connect to the native Extended View System
     *         service or to register a death recipient.
     *         true otherwise.
     */
    bool initialize(JNIEnv* env, jobject thiz) ACQUIRE(mLock);

    /*
     * Requests to open a target camera device.
     *
     * @param id a string camera device identifier
     * @return bool false if it has not connected to EVS service, fails to open
     *              a camera device, or fails to initialize a stream handler;
     *              true otherwise.
     */
    bool openCamera(const char* id) ACQUIRE(mLock);

    /*
     * Requests to close an active camera device.
     */
    void closeCamera();

    /*
     * Requests to start a video stream from a successfully opened camera device.
     */
    bool startVideoStream();

    /*
     * Requests to stop an active video stream.
     */
    void stopVideoStream();

    /*
     * Notifies that the client finishes with this buffer.
     *
     * @param frame a consumed frame buffer
     */
    void doneWithFrame(int bufferId);

    /*
     * Tells whether or not we're connected to the Extended View System service
     */
    bool isAvailable() ACQUIRE(mLock) {
        std::lock_guard<std::mutex> lock(mLock);
        return mService != nullptr;
    }

    /*
     * Tells whether or not a target camera device is opened
     */
    bool isCameraOpened() ACQUIRE(mLock) {
        std::lock_guard<std::mutex> lock(mLock);
        return mCamera != nullptr;
    }

    /*
     * Compares the binder interface
     */
    bool isEqual(const wp<hidl::base::V1_0::IBase>& who) ACQUIRE(mLock) {
        std::lock_guard<std::mutex> lock(mLock);
        return hardware::interfacesEqual(mService, who.promote());
    }

    /*
     * Implements EvsServiceCallback methods
     */
    void onNewEvent(hardware::automotive::evs::V1_1::EvsEventDesc) override;
    void onNewFrame(hardware::automotive::evs::V1_1::BufferDesc) override;
    void onServiceDied(const wp<hidl::base::V1_0::IBase>&) override;

private:
    // Acquires the camera and the display exclusive ownerships.
    void acquireCameraAndDisplay() ACQUIRE(mLock);

    // A mutex to protect shared resources
    mutable std::mutex mLock;

    // Extended View System Enumerator service handle
    sp<hardware::automotive::evs::V1_1::IEvsEnumerator> mService GUARDED_BY(mLock);

    // A camera device opened for the rearview service
    sp<hardware::automotive::evs::V1_1::IEvsCamera> mCamera GUARDED_BY(mLock);

    // A handler of a video stream from the rearview camera device
    sp<StreamHandler> mStreamHandler GUARDED_BY(mLock);

    // Extended View System display handle.  This would not be used but held by
    // us to prevent other EVS clients from using EvsDisplay.
    sp<hardware::automotive::evs::V1_1::IEvsDisplay> mDisplay;

    // A flag to acquire a display handle only once
    std::once_flag mDisplayAcquired;

    // A death recipient of Extended View System service
    sp<EvsDeathRecipient> mDeathRecipient GUARDED_BY(mLock);

    // Java VM
    JavaVM* mVm;

    // Background thread to handle callbacks from the native Extended View
    // System service
    EvsCallbackThread mCallbackThread;

    // Reference to CarEvsService object
    jobject mCarEvsServiceObj;

    // CarEvsService object's method to handle the accidental death of the
    // native Extended View System service
    jmethodID mDeathHandlerMethodId;

    // CarEvsService object's method to handle a new frame buffer
    jmethodID mFrameHandlerMethodId;

    // CarEvsService object's method to handle a new stream event
    jmethodID mEventHandlerMethodId;

    // Bookkeeps descriptors of received frame buffers.
    std::map<int, hardware::automotive::evs::V1_1::BufferDesc> mBufferRecords GUARDED_BY(mLock);

    // A name of the camera device currently in use.
    const char* mCameraIdInUse;

    // Service name for EVS enumerator
    static const char* kServiceName;

    // Maximum number of frames CarEvsService can hold.  This number has been
    // chosen heuristically.
    static constexpr int kMaxNumFramesInFlight = 6;

    // EVS service reserves a display ID 255 to allow the clients to open the main
    // display exclusively.
    static constexpr uint8_t kExclusiveMainDisplayId = 0xFF;
};

}  // namespace evs
}  // namespace automotive
}  // namespace android

#endif  // ANDROID_CARSERVICE_EVS_SERVICE_WRAPPER_H
