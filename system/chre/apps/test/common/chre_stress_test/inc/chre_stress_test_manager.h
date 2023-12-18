/*
 * Copyright (C) 2021 The Android Open Source Project
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

#ifndef CHRE_STRESS_TEST_MANAGER_H_
#define CHRE_STRESS_TEST_MANAGER_H_

#include "chre_stress_test.nanopb.h"

#include <chre.h>
#include <cinttypes>

#include "chre/util/optional.h"
#include "chre/util/singleton.h"
#include "chre/util/time.h"

namespace chre {

namespace stress_test {

/**
 * A class to manage a CHRE stress test session.
 */
class Manager {
 public:
  /**
   * Handles an event from CHRE. Semantics are the same as nanoappHandleEvent.
   */
  void handleEvent(uint32_t senderInstanceId, uint16_t eventType,
                   const void *eventData);

 private:
  struct AsyncRequest {
    AsyncRequest(const void *cookie_) {
      cookie = cookie_;
    }

    uint64_t requestTimeNs = chreGetTime();
    const void *cookie;
  };

  /**
   * Handles a message from the host.
   *
   * @param senderInstanceId The sender instance ID of this message.
   * @param hostData The data from the host.
   */
  void handleMessageFromHost(uint32_t senderInstanceId,
                             const chreMessageFromHostData *hostData);
  /**
   * Processes data from CHRE.
   *
   * @param eventType The event type as defined by CHRE.
   * @param eventData A pointer to the data.
   */
  void handleDataFromChre(uint16_t eventType, const void *eventData);

  /**
   * @param handle A pointer to the timer handle.
   */
  void handleTimerEvent(const uint32_t *handle);

  /**
   * Validates a timestamp of an event where the timestamp is expected
   * to be monotonically increasing.
   *
   * @param timestamp The timestamp.
   * @param pastTimestamp The previous timestamp.
   */
  void checkTimestamp(uint64_t timestamp, uint64_t pastTimestamp);

  /**
   * Handles a start command from the host.
   *
   * @param start true to start the test, stop otherwise.
   */
  void handleWifiStartCommand(bool start);
  void handleGnssLocationStartCommand(bool start);
  void handleGnssMeasurementStartCommand(bool start);
  void handleWwanStartCommand(bool start);
  void handleWifiScanMonitoringCommand(bool start);

  /**
   * @param result The WiFi async result from CHRE.
   */
  void handleWifiAsyncResult(const chreAsyncResult *result);

  /**
   * @param result The WiFi scan event from CHRE.
   */
  void handleWifiScanEvent(const chreWifiScanEvent *event);

  /**
   * Sets up a WiFi scan request after some time.
   */
  void requestDelayedWifiScan();
  void handleDelayedWifiTimer();

  /**
   * Sends the failure to the host.
   *
   * @param errorMessage The error message string.
   */
  void sendFailure(const char *errorMessage);

  /**
   * Sets/cancels a timer and asserts success.
   *
   * @param delayNs The delay of the timer in nanoseconds.
   * @param oneShot true if the timer request is one-shot.
   * @param timerHandle A non-null pointer to where the timer handle is stored.
   */
  void setTimer(uint64_t delayNs, bool oneShot, uint32_t *timerHandle);
  void cancelTimer(uint32_t *timerHandle);

  /**
   * Makes the next GNSS request.
   */
  void makeGnssLocationRequest();
  void makeGnssMeasurementRequest();

  /**
   * @param result The GNSS async result from CHRE.
   */
  void handleGnssAsyncResult(const chreAsyncResult *result);

  /**
   * @param result The result to validate.
   * @param request The async request associated with this result.
   * @param asyncTimerHandle The async timer handle for this request.
   */
  void validateGnssAsyncResult(const chreAsyncResult *result,
                               Optional<AsyncRequest> &request,
                               uint32_t *asyncTimerHandle);

  /**
   * @param event The GNSS event from CHRE.
   */
  void handleGnssLocationEvent(const chreGnssLocationEvent *event);
  void handleGnssDataEvent(const chreGnssDataEvent *event);

  /**
   * Makes the next cell info request.
   */
  void makeWwanCellInfoRequest();

  /**
   * @param event The cell info event from CHRE.
   */
  void handleCellInfoResult(const chreWwanCellInfoResult *event);

  //! The host endpoint of the current test host.
  Optional<uint16_t> mHostEndpoint;

  //! The timer handle for performing requests.
  uint32_t mWifiScanTimerHandle = CHRE_TIMER_INVALID;
  uint32_t mWifiScanAsyncTimerHandle = CHRE_TIMER_INVALID;
  uint32_t mGnssLocationTimerHandle = CHRE_TIMER_INVALID;
  uint32_t mGnssLocationAsyncTimerHandle = CHRE_TIMER_INVALID;
  uint32_t mGnssMeasurementTimerHandle = CHRE_TIMER_INVALID;
  uint32_t mGnssMeasurementAsyncTimerHandle = CHRE_TIMER_INVALID;
  uint32_t mWwanTimerHandle = CHRE_TIMER_INVALID;
  uint32_t mWifiScanMonitorAsyncTimerHandle = CHRE_TIMER_INVALID;

  //! true if the test has been started for the feature.
  bool mWifiTestStarted = false;
  bool mGnssLocationTestStarted = false;
  bool mGnssMeasurementTestStarted = false;
  bool mWwanTestStarted = false;

  //! true if scan monitor is enabled for the nanoapp.
  bool mWifiScanMonitorEnabled = false;

  //! The cookie to use for requests.
  const uint32_t kOnDemandWifiScanCookie = 0xface;
  const uint32_t kGnssLocationCookie = 0xbeef;
  const uint32_t kGnssMeasurementCookie = 0xbead;
  const uint32_t kWwanCellInfoCookie = 0x1337;

  //! The pending requests.
  Optional<AsyncRequest> mWifiScanAsyncRequest;
  Optional<AsyncRequest> mGnssLocationAsyncRequest;
  Optional<AsyncRequest> mGnssMeasurementAsyncRequest;
  Optional<AsyncRequest> mWwanCellInfoAsyncRequest;

  //! The previous timestamp of events.
  uint64_t mPrevGnssLocationEventTimestampMs = 0;
  uint64_t mPrevGnssMeasurementEventTimestampNs = 0;
  uint64_t mPrevWifiScanEventTimestampNs = 0;
  uint64_t mPrevWwanCellInfoEventTimestampNs = 0;
};

// The stress test manager singleton.
typedef chre::Singleton<Manager> ManagerSingleton;

}  // namespace stress_test

}  // namespace chre

#endif  // CHRE_STRESS_TEST_MANAGER_H_
