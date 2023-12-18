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

#ifndef ST_HAL_LPMA_HANDLER_H_
#define ST_HAL_LPMA_HANDLER_H_

#include <condition_variable>
#include <cstdio>
#include <functional>
#include <mutex>
#include <thread>

#include "chre_host/log.h"

#include <android/hardware/soundtrigger/2.0/ISoundTriggerHw.h>
#include <hardware_legacy/power.h>

using android::hardware::hidl_death_recipient;
using android::hardware::Return;
using android::hardware::soundtrigger::V2_0::ISoundTriggerHw;
using android::hardware::soundtrigger::V2_0::SoundModelHandle;
using android::hardware::soundtrigger::V2_0::SoundModelType;
using android::hidl::base::V1_0::IBase;

namespace android {
namespace chre {

/**
 * Handles interactions with the SoundTrigger (ST) HAL, to issue configuration
 * requests for the always-on audio hardware to enable Low Power Microphone
 * Access (LPMA) in CHRE. Typically, this class is used when the CHRE audio
 * implementation for a given system requires notifying the ST HAL when audio
 * is requested/released within CHRE
 */
class StHalLpmaHandler {
 public:
  //! Class to handle when a connected ST HAL service dies
  class StHalDeathRecipient : public hidl_death_recipient {
   public:
    StHalDeathRecipient() = delete;
    explicit StHalDeathRecipient(std::function<void()> cb) : mCallback(cb) {}

    /**
     * Callback that is called when a connected service dies. It mainly
     * resets the LPMA Enabled flag, and unblocks the LPMA processing thread.
     * It is to be used in conjunction with linkToDeath(), which we do
     * in checkConnectionToStHalServiceLocked().
     */
    virtual void serviceDied(uint64_t /* cookie */,
                             const wp<IBase> & /* who */) override {
      mCallback();
    }

   private:
    std::function<void()> mCallback;
  };

  StHalLpmaHandler() = delete;

  explicit StHalLpmaHandler(bool allowed);

  ~StHalLpmaHandler() {
    if (mThread.has_value()) {
      // TODO: Change this to join after adding proper handler
      mThread->detach();
    }
  }

  /**
   * If LPMA is enabled, starts a worker thread to load/unload models.
   */
  void init();

  /**
   * Sets the target state for LPMA to be enabled. This triggers another thread
   * to perform the async operation of enabling or disabling the LPMA use case.
   *
   * @param enabled Whether LPMA is to be enabled or disabled.
   */
  void enable(bool enabled);

  /**
   * Loads the LPMA use case via the SoundTrigger HAL HIDL service.
   *
   * @return true if LPMA was enabled successfully, false otherwise.
   */
  bool load();

  /**
   * Unloads the LPMA use case via the SoundTrigger HAL HIDL service. This
   * function does not indicate success/failure as it is expected that even in
   * the event of a failure to unload, the use case will be unloaded. As long as
   * the sound trigger HAL received the request we can be assured that the use
   * case will be unloaded (even if it means reseting the codec or otherwise).
   */
  void unload();

  /**
   * Entry point for the thread that loads/unloads sound models from the
   * ST HAL
   */
  void StHalLpmaHandlerThreadEntry();

 private:
  const bool mIsLpmaAllowed;
  bool mCurrentLpmaEnabled;
  bool mTargetLpmaEnabled;
  bool mCondVarPredicate;
  SoundModelHandle mLpmaHandle = 0;

  int mRetryCount;
  useconds_t mRetryDelay;

  std::optional<std::thread> mThread;
  std::mutex mMutex;
  std::condition_variable mCondVar;

  sp<StHalDeathRecipient> mDeathRecipient;
  sp<ISoundTriggerHw> mStHalService;

  /**
   * Checks for a valid connection to the ST HAL service, reconnects if not
   * already connected. This method should only be invoked after acquiring the
   * class mutex has been locked.
   */
  void checkConnectionToStHalServiceLocked();

  /**
   * Invoked by the HAL service death callback. see
   * StHalDeathRecipient::ServiceDied()
   */
  void onStHalServiceDeath();

  /**
   * This function blocks on a condition variable and when notified, based
   * on its current state and as notified by enable(), performs a load or
   * unload. The function also resets the delay and retry counts if the current
   * and next states match
   *
   * @return true if the state update succeeded, and we don't need to retry with
   * a delay
   */
  bool waitOnStHalRequestAndProcess();

  /**
   * Delay retrying a load if a state update failed
   */
  void delay();
};

}  // namespace chre
}  // namespace android

#endif  // ST_HAL_LPMA_HANDLER_H_
