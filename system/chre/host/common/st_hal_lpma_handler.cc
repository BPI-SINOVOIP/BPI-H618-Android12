/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <cinttypes>

#include "chre_host/st_hal_lpma_handler.h"

namespace android {
namespace chre {

namespace {

constexpr char kChreWakeLockName[] = "chre_daemon";

void acquireWakeLock() {
  int rc;
  if ((rc = acquire_wake_lock(PARTIAL_WAKE_LOCK, kChreWakeLockName)) != 0) {
    LOGE("Failed to acquire wakelock (err %d)", rc);
  }
}

void releaseWakeLock() {
  int rc;
  static bool wakeLockInitialRelease = true;

  // It's expected to get an error when we first try to release the
  // wakelock
  // as it won't exist unless it was leaked previously - don't output a
  // false warning for this case
  if (((rc = release_wake_lock(kChreWakeLockName)) != 0) &&
      !wakeLockInitialRelease) {
    LOGE("Failed to release wakelock (err %d)", rc);
  }

  wakeLockInitialRelease = false;
}

}  // anonymous namespace

StHalLpmaHandler::StHalLpmaHandler(bool allowed) : mIsLpmaAllowed(allowed) {
  auto cb = [&]() { onStHalServiceDeath(); };
  mDeathRecipient = new StHalDeathRecipient(cb);
}

void StHalLpmaHandler::init() {
  if (mIsLpmaAllowed) {
    mThread = std::thread(&StHalLpmaHandler::StHalLpmaHandlerThreadEntry, this);
  }
}

void StHalLpmaHandler::enable(bool enabled) {
  if (mIsLpmaAllowed) {
    std::lock_guard<std::mutex> lock(mMutex);
    mTargetLpmaEnabled = enabled;
    mCondVarPredicate = true;
    mCondVar.notify_one();
  } else {
    LOGE("Trying to modify LPMA state when LPMA is disabled");
  }
}

bool StHalLpmaHandler::load() {
  constexpr uint8_t kUuidNode[] = {0x2E, 0x95, 0xA2, 0x31, 0x3A, 0xEE};

  LOGV("Loading LPMA");

  ISoundTriggerHw::SoundModel soundModel;
  soundModel.type = SoundModelType::GENERIC;
  soundModel.vendorUuid.timeLow = 0x57CADDB1;
  soundModel.vendorUuid.timeMid = 0xACDB;
  soundModel.vendorUuid.versionAndTimeHigh = 0x4DCE;
  soundModel.vendorUuid.variantAndClockSeqHigh = 0x8CB0;

  memcpy(&soundModel.vendorUuid.node[0], kUuidNode, sizeof(kUuidNode));
  soundModel.data.resize(1);  // Insert an empty byte to bypass HAL NULL checks.

  bool loaded = false;
  checkConnectionToStHalServiceLocked();
  int32_t loadResult;
  Return<void> hidlResult = mStHalService->loadSoundModel(
      soundModel, nullptr /* callback */, 0 /* cookie */,
      [&](int32_t retval, SoundModelHandle handle) {
        loadResult = retval;
        mLpmaHandle = handle;
      });

  if (hidlResult.isOk()) {
    if (loadResult == 0) {
      LOGD("Loaded LPMA");
      loaded = true;
    } else {
      LOGE("Failed to load LPMA with %" PRId32, loadResult);
    }
  } else {
    LOGE("Failed to load LPMA due to hidl error %s",
         hidlResult.description().c_str());
  }

  return loaded;
}

void StHalLpmaHandler::unload() {
  checkConnectionToStHalServiceLocked();
  Return<int32_t> hidlResult = mStHalService->unloadSoundModel(mLpmaHandle);
  mLpmaHandle = 0;

  if (hidlResult.isOk()) {
    if (hidlResult != 0) {
      LOGE("Failed to unload LPMA with %" PRId32, int32_t(hidlResult));
    }
  } else {
    LOGE("Failed to unload LPMA due to hidl error %s",
         hidlResult.description().c_str());
  }
}

void StHalLpmaHandler::checkConnectionToStHalServiceLocked() {
  if (mStHalService == nullptr) {
    mStHalService = ISoundTriggerHw::getService();
    if (mStHalService != nullptr) {
      LOGI("Connected to ST HAL service");
      mStHalService->linkToDeath(mDeathRecipient, 0 /* flags */);
    }
  }
}

bool StHalLpmaHandler::waitOnStHalRequestAndProcess() {
  bool noDelayNeeded = true;
  std::unique_lock<std::mutex> lock(mMutex);

  if (mCurrentLpmaEnabled == mTargetLpmaEnabled) {
    mRetryDelay = 0;
    mRetryCount = 0;
    releaseWakeLock();  // Allow the system to suspend while waiting.
    mCondVar.wait(lock, [this] { return mCondVarPredicate; });
    mCondVarPredicate = false;
    acquireWakeLock();  // Ensure the system stays up while retrying.
  } else if (mTargetLpmaEnabled && load()) {
    mCurrentLpmaEnabled = mTargetLpmaEnabled;
  } else if (!mTargetLpmaEnabled) {
    // Regardless of whether the use case fails to unload, set the
    // currentLpmaEnabled to the targetLpmaEnabled. This will allow the next
    // enable request to proceed. After a failure to unload occurs, the
    // supplied handle is invalid and should not be unloaded again.
    unload();
    mCurrentLpmaEnabled = mTargetLpmaEnabled;
  } else {
    noDelayNeeded = false;
  }

  return noDelayNeeded;
}

void StHalLpmaHandler::delay() {
  constexpr useconds_t kInitialRetryDelayUs = 500000;
  constexpr int kRetryGrowthFactor = 2;
  constexpr int kRetryGrowthLimit = 5;     // Terminates at 8s retry interval.
  constexpr int kRetryWakeLockLimit = 10;  // Retry with a wakelock 10 times.

  if (mRetryDelay == 0) {
    mRetryDelay = kInitialRetryDelayUs;
  } else if (mRetryCount < kRetryGrowthLimit) {
    mRetryDelay *= kRetryGrowthFactor;
  }
  usleep(mRetryDelay);
  mRetryCount++;
  if (mRetryCount > kRetryWakeLockLimit) {
    releaseWakeLock();
  }
}

void StHalLpmaHandler::StHalLpmaHandlerThreadEntry() {
  LOGD("Starting LPMA thread");

  while (true) {
    if (!waitOnStHalRequestAndProcess()) {
      // processing an LPMA state update failed, retry after a little while
      delay();
    }
  }
}

void StHalLpmaHandler::onStHalServiceDeath() {
  LOGE("ST HAL Service Died");
  std::lock_guard<std::mutex> lock(mMutex);
  mStHalService = nullptr;
  if (mTargetLpmaEnabled) {
    // ST HAL has died, so assume that the sound model is no longer active,
    // and trigger a reload of the sound model.
    mCurrentLpmaEnabled = false;
    mCondVarPredicate = true;
    mCondVar.notify_one();
  }
}

}  // namespace chre
}  // namespace android
