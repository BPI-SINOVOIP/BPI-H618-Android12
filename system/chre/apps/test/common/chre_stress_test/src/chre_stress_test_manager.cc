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

#include "chre_stress_test_manager.h"

#include <pb_decode.h>

#include "chre/util/macros.h"
#include "chre/util/nanoapp/callbacks.h"
#include "chre/util/nanoapp/log.h"
#include "chre_stress_test.nanopb.h"
#include "send_message.h"

#define LOG_TAG "[ChreStressTest]"

namespace chre {

namespace stress_test {

namespace {

constexpr chre::Nanoseconds kWifiScanInterval = chre::Seconds(5);

bool isRequestTypeForLocation(uint8_t requestType) {
  return (requestType == CHRE_GNSS_REQUEST_TYPE_LOCATION_SESSION_START) ||
         (requestType == CHRE_GNSS_REQUEST_TYPE_LOCATION_SESSION_STOP);
}

bool isRequestTypeForMeasurement(uint8_t requestType) {
  return (requestType == CHRE_GNSS_REQUEST_TYPE_MEASUREMENT_SESSION_START) ||
         (requestType == CHRE_GNSS_REQUEST_TYPE_MEASUREMENT_SESSION_STOP);
}

}  // anonymous namespace

void Manager::handleEvent(uint32_t senderInstanceId, uint16_t eventType,
                          const void *eventData) {
  if (eventType == CHRE_EVENT_MESSAGE_FROM_HOST) {
    handleMessageFromHost(
        senderInstanceId,
        static_cast<const chreMessageFromHostData *>(eventData));
  } else if (senderInstanceId == CHRE_INSTANCE_ID) {
    handleDataFromChre(eventType, eventData);
  } else {
    LOGW("Got unknown event type from senderInstanceId %" PRIu32
         " and with eventType %" PRIu16,
         senderInstanceId, eventType);
  }
}

void Manager::handleMessageFromHost(uint32_t senderInstanceId,
                                    const chreMessageFromHostData *hostData) {
  bool success = false;
  uint32_t messageType = hostData->messageType;
  if (senderInstanceId != CHRE_INSTANCE_ID) {
    LOGE("Incorrect sender instance id: %" PRIu32, senderInstanceId);
  } else if (messageType == chre_stress_test_MessageType_TEST_HOST_RESTARTED) {
    // Do nothing and only update the host endpoint
    mHostEndpoint = hostData->hostEndpoint;
    success = true;
  } else if (messageType != chre_stress_test_MessageType_TEST_COMMAND) {
    LOGE("Invalid message type %" PRIu32, messageType);
  } else if (mHostEndpoint.has_value() &&
             hostData->hostEndpoint != mHostEndpoint.value()) {
    LOGE("Invalid host endpoint %" PRIu16 " expected %" PRIu16,
         hostData->hostEndpoint, mHostEndpoint.value());
  } else {
    pb_istream_t istream = pb_istream_from_buffer(
        static_cast<const pb_byte_t *>(hostData->message),
        hostData->messageSize);
    chre_stress_test_TestCommand testCommand =
        chre_stress_test_TestCommand_init_default;

    if (!pb_decode(&istream, chre_stress_test_TestCommand_fields,
                   &testCommand)) {
      LOGE("Failed to decode start command error %s", PB_GET_ERROR(&istream));
    } else {
      LOGI("Got message from host: feature %d start %d", testCommand.feature,
           testCommand.start);

      success = true;
      switch (testCommand.feature) {
        case chre_stress_test_TestCommand_Feature_WIFI_ON_DEMAND_SCAN: {
          handleWifiStartCommand(testCommand.start);
          break;
        }
        case chre_stress_test_TestCommand_Feature_GNSS_LOCATION: {
          handleGnssLocationStartCommand(testCommand.start);
          break;
        }
        case chre_stress_test_TestCommand_Feature_GNSS_MEASUREMENT: {
          handleGnssMeasurementStartCommand(testCommand.start);
          break;
        }
        case chre_stress_test_TestCommand_Feature_WWAN: {
          handleWwanStartCommand(testCommand.start);
          break;
        }
        case chre_stress_test_TestCommand_Feature_WIFI_SCAN_MONITOR: {
          handleWifiScanMonitoringCommand(testCommand.start);
          break;
        }
        default: {
          LOGE("Unknown feature %d", testCommand.feature);
          success = false;
          break;
        }
      }
    }

    mHostEndpoint = hostData->hostEndpoint;
  }

  if (!success) {
    test_shared::sendTestResultWithMsgToHost(
        hostData->hostEndpoint,
        chre_stress_test_MessageType_TEST_RESULT /* messageType */, success,
        nullptr /* errMessage */);
  }
}

void Manager::handleDataFromChre(uint16_t eventType, const void *eventData) {
  switch (eventType) {
    case CHRE_EVENT_TIMER:
      handleTimerEvent(static_cast<const uint32_t *>(eventData));
      break;

    case CHRE_EVENT_WIFI_ASYNC_RESULT:
      handleWifiAsyncResult(static_cast<const chreAsyncResult *>(eventData));
      break;

    case CHRE_EVENT_WIFI_SCAN_RESULT:
      handleWifiScanEvent(static_cast<const chreWifiScanEvent *>(eventData));
      break;

    case CHRE_EVENT_GNSS_ASYNC_RESULT:
      handleGnssAsyncResult(static_cast<const chreAsyncResult *>(eventData));
      break;

    case CHRE_EVENT_GNSS_LOCATION:
      handleGnssLocationEvent(
          static_cast<const chreGnssLocationEvent *>(eventData));
      break;

    case CHRE_EVENT_GNSS_DATA:
      handleGnssDataEvent(static_cast<const chreGnssDataEvent *>(eventData));
      break;

    case CHRE_EVENT_WWAN_CELL_INFO_RESULT:
      handleCellInfoResult(
          static_cast<const chreWwanCellInfoResult *>(eventData));
      break;

    default:
      LOGW("Unknown event type %" PRIu16, eventType);
      break;
  }
}

void Manager::handleTimerEvent(const uint32_t *handle) {
  if (*handle == mWifiScanTimerHandle) {
    handleDelayedWifiTimer();
  } else if (*handle == mWifiScanAsyncTimerHandle) {
    sendFailure("WiFi scan request timed out");
  } else if (*handle == mGnssLocationTimerHandle) {
    makeGnssLocationRequest();
  } else if (*handle == mGnssMeasurementTimerHandle) {
    makeGnssMeasurementRequest();
  } else if (*handle == mGnssLocationAsyncTimerHandle &&
             mGnssLocationAsyncRequest.has_value()) {
    sendFailure("GNSS location async result timed out");
  } else if (*handle == mGnssMeasurementAsyncTimerHandle &&
             mGnssMeasurementAsyncRequest.has_value()) {
    sendFailure("GNSS measurement async result timed out");
  } else if (*handle == mWwanTimerHandle) {
    makeWwanCellInfoRequest();
  } else if (*handle == mWifiScanMonitorAsyncTimerHandle) {
    sendFailure("WiFi scan monitor request timed out");
  } else {
    sendFailure("Unknown timer handle");
  }
}

void Manager::handleDelayedWifiTimer() {
  // NOTE: We set the maxScanAgeMs to something smaller than the WiFi
  // scan periodicity to ensure new scans are generated.
  static const struct chreWifiScanParams params = {
      /*.scanType=*/CHRE_WIFI_SCAN_TYPE_NO_PREFERENCE,
      /*.maxScanAgeMs=*/2000,  // 2 seconds
      /*.frequencyListLen=*/0,
      /*.frequencyList=*/NULL,
      /*.ssidListLen=*/0,
      /*.ssidList=*/NULL,
      /*.radioChainPref=*/CHRE_WIFI_RADIO_CHAIN_PREF_DEFAULT,
      /*.channelSet=*/CHRE_WIFI_CHANNEL_SET_NON_DFS};

  bool success = chreWifiRequestScanAsync(&params, &kOnDemandWifiScanCookie);
  LOGI("Requested on demand wifi success ? %d", success);
  if (!success) {
    sendFailure("Failed to make WiFi scan request");
  } else {
    mWifiScanAsyncRequest = AsyncRequest(&kOnDemandWifiScanCookie);
    setTimer(CHRE_WIFI_SCAN_RESULT_TIMEOUT_NS, true /* oneShot */,
             &mWifiScanAsyncTimerHandle);
  }
}

void Manager::handleWifiAsyncResult(const chreAsyncResult *result) {
  if (result->requestType == CHRE_WIFI_REQUEST_TYPE_REQUEST_SCAN) {
    if (result->success) {
      LOGI("On-demand scan success");
    } else {
      LOGW("On-demand scan failed: code %" PRIu8, result->errorCode);
    }

    if (!mWifiScanAsyncRequest.has_value()) {
      sendFailure("Received WiFi async result with no pending request");
    } else if (result->cookie != mWifiScanAsyncRequest->cookie) {
      sendFailure("On-demand scan cookie mismatch");
    }

    cancelTimer(&mWifiScanAsyncTimerHandle);
    mWifiScanAsyncRequest.reset();
    requestDelayedWifiScan();
  } else if (result->requestType ==
             CHRE_WIFI_REQUEST_TYPE_CONFIGURE_SCAN_MONITOR) {
    if (!result->success) {
      LOGE("Scan monitor async failure: code %" PRIu8, result->errorCode);
      sendFailure("Scan monitor async failed");
    }

    cancelTimer(&mWifiScanMonitorAsyncTimerHandle);
    mWifiScanMonitorEnabled = (result->cookie != nullptr);
  } else {
    sendFailure("Unknown WiFi async result type");
  }
}

void Manager::handleGnssAsyncResult(const chreAsyncResult *result) {
  if (isRequestTypeForLocation(result->requestType)) {
    validateGnssAsyncResult(result, mGnssLocationAsyncRequest,
                            &mGnssLocationAsyncTimerHandle);
  } else if (isRequestTypeForMeasurement(result->requestType)) {
    validateGnssAsyncResult(result, mGnssMeasurementAsyncRequest,
                            &mGnssMeasurementAsyncTimerHandle);
  } else {
    sendFailure("Unknown GNSS async result type");
  }
}

void Manager::validateGnssAsyncResult(const chreAsyncResult *result,
                                      Optional<AsyncRequest> &request,
                                      uint32_t *asyncTimerHandle) {
  if (!request.has_value()) {
    sendFailure("Received GNSS async result with no pending request");
  } else if (!result->success) {
    sendFailure("Async GNSS failure");
  } else if (result->cookie != request->cookie) {
    sendFailure("GNSS async cookie mismatch");
  }

  cancelTimer(asyncTimerHandle);
  request.reset();
}

void Manager::checkTimestamp(uint64_t timestamp, uint64_t pastTimestamp) {
  if (timestamp < pastTimestamp) {
    sendFailure("Timestamp was too old");
  } else if (timestamp == pastTimestamp) {
    sendFailure("Timestamp was duplicate");
  }
}

void Manager::handleGnssLocationEvent(const chreGnssLocationEvent *event) {
  LOGI("Received GNSS location event at %" PRIu64 " ms", event->timestamp);

  checkTimestamp(event->timestamp, mPrevGnssLocationEventTimestampMs);
  mPrevGnssLocationEventTimestampMs = event->timestamp;
}

void Manager::handleGnssDataEvent(const chreGnssDataEvent *event) {
  static uint32_t sPrevDiscontCount = 0;
  LOGI("Received GNSS measurement event at %" PRIu64 " ns count %" PRIu32
       " flags 0x%" PRIx16,
       event->clock.time_ns, event->clock.hw_clock_discontinuity_count,
       event->clock.flags);

  if (sPrevDiscontCount == event->clock.hw_clock_discontinuity_count) {
    checkTimestamp(event->clock.time_ns, mPrevGnssMeasurementEventTimestampNs);
  }

  sPrevDiscontCount = event->clock.hw_clock_discontinuity_count;
  mPrevGnssMeasurementEventTimestampNs = event->clock.time_ns;
}

void Manager::handleWifiScanEvent(const chreWifiScanEvent *event) {
  LOGI("Received Wifi scan event of type %" PRIu8 " with %" PRIu8
       " results at %" PRIu64 " ns",
       event->scanType, event->resultCount, event->referenceTime);

  if (event->eventIndex == 0) {
    checkTimestamp(event->referenceTime, mPrevWifiScanEventTimestampNs);
    mPrevWifiScanEventTimestampNs = event->referenceTime;
  }

  if (mWifiScanMonitorEnabled) {
    chreSendMessageToHostEndpoint(
        nullptr, 0,
        chre_stress_test_MessageType_TEST_WIFI_SCAN_MONITOR_TRIGGERED,
        mHostEndpoint.value(), nullptr /* freeCallback */);
  }
}

void Manager::handleCellInfoResult(const chreWwanCellInfoResult *event) {
  LOGI("Received %" PRIu8 " cell info results", event->cellInfoCount);

  mWwanCellInfoAsyncRequest.reset();
  if (event->errorCode != CHRE_ERROR_NONE) {
    LOGE("Cell info request failed with error code %" PRIu8, event->errorCode);
    sendFailure("Cell info request failed");
  } else if (event->cellInfoCount > 0) {
    uint64_t maxTimestamp = 0;
    for (uint8_t i = 0; i < event->cellInfoCount; i++) {
      maxTimestamp = MAX(maxTimestamp, event->cells[i].timeStamp);
      checkTimestamp(event->cells[i].timeStamp,
                     mPrevWwanCellInfoEventTimestampNs);
    }

    mPrevWwanCellInfoEventTimestampNs = maxTimestamp;
  }
}

void Manager::handleWifiStartCommand(bool start) {
  mWifiTestStarted = start;
  if (start) {
    requestDelayedWifiScan();
  } else {
    cancelTimer(&mWifiScanTimerHandle);
  }
}

void Manager::handleGnssLocationStartCommand(bool start) {
  constexpr uint64_t kTimerDelayNs = Seconds(60).toRawNanoseconds();

  if (chreGnssGetCapabilities() & CHRE_GNSS_CAPABILITIES_LOCATION) {
    mGnssLocationTestStarted = start;
    makeGnssLocationRequest();

    if (start) {
      setTimer(kTimerDelayNs, false /* oneShot */, &mGnssLocationTimerHandle);
    } else {
      cancelTimer(&mGnssLocationTimerHandle);
    }
  } else {
    sendFailure("Platform has no location capability");
  }
}

void Manager::handleGnssMeasurementStartCommand(bool start) {
  constexpr uint64_t kTimerDelayNs = Seconds(60).toRawNanoseconds();

  if (chreGnssGetCapabilities() & CHRE_GNSS_CAPABILITIES_MEASUREMENTS) {
    mGnssMeasurementTestStarted = start;
    makeGnssMeasurementRequest();

    if (start) {
      setTimer(kTimerDelayNs, false /* oneShot */,
               &mGnssMeasurementTimerHandle);
    } else {
      cancelTimer(&mGnssMeasurementTimerHandle);
    }
  } else {
    sendFailure("Platform has no GNSS measurement capability");
  }
}

void Manager::handleWwanStartCommand(bool start) {
  constexpr uint64_t kTimerDelayNs = CHRE_ASYNC_RESULT_TIMEOUT_NS;

  if (chreWwanGetCapabilities() & CHRE_WWAN_GET_CELL_INFO) {
    mWwanTestStarted = start;
    makeWwanCellInfoRequest();

    if (start) {
      setTimer(kTimerDelayNs, false /* oneShot */, &mWwanTimerHandle);
    } else {
      cancelTimer(&mWwanTimerHandle);
    }
  } else {
    sendFailure("Platform has no WWAN cell info capability");
  }
}

void Manager::handleWifiScanMonitoringCommand(bool start) {
  if (chreWifiGetCapabilities() & CHRE_WIFI_CAPABILITIES_SCAN_MONITORING) {
    const uint32_t kWifiScanMonitorEnabledCookie = 0x1234;
    bool success = chreWifiConfigureScanMonitorAsync(
        start, start ? &kWifiScanMonitorEnabledCookie : nullptr);
    LOGI("Scan monitor enable %d request success ? %d", start, success);

    if (!success) {
      sendFailure("Scan monitor request failed");
    } else {
      setTimer(CHRE_ASYNC_RESULT_TIMEOUT_NS, true /* oneShot */,
               &mWifiScanMonitorAsyncTimerHandle);
    }
  } else {
    sendFailure("Platform has no WiFi scan monitoring capability");
  }
}

void Manager::setTimer(uint64_t delayNs, bool oneShot, uint32_t *timerHandle) {
  *timerHandle = chreTimerSet(delayNs, timerHandle, oneShot);
  if (*timerHandle == CHRE_TIMER_INVALID) {
    sendFailure("Failed to set timer");
  }
}

void Manager::cancelTimer(uint32_t *timerHandle) {
  if (*timerHandle != CHRE_TIMER_INVALID) {
    if (!chreTimerCancel(*timerHandle)) {
      // We don't treat this as a test failure, because the CHRE API does not
      // guarantee this method succeeds (e.g. if the timer is one-shot and just
      // fired).
      LOGW("Failed to cancel timer");
    }
    *timerHandle = CHRE_TIMER_INVALID;
  }
}

void Manager::makeGnssLocationRequest() {
  // The list of location intervals to iterate; wraps around.
  static const uint32_t kMinIntervalMsList[] = {1000, 0};
  static size_t sIntervalIndex = 0;

  uint32_t minIntervalMs = 0;
  if (mGnssLocationTestStarted) {
    minIntervalMs = kMinIntervalMsList[sIntervalIndex];
    sIntervalIndex = (sIntervalIndex + 1) % ARRAY_SIZE(kMinIntervalMsList);
  } else {
    sIntervalIndex = 0;
  }

  bool success = false;
  if (minIntervalMs > 0) {
    success = chreGnssLocationSessionStartAsync(
        minIntervalMs, 0 /* minTimeToNextFixMs */, &kGnssLocationCookie);
  } else {
    success = chreGnssLocationSessionStopAsync(&kGnssLocationCookie);
  }

  LOGI("Configure GNSS location interval %" PRIu32 " ms success ? %d",
       minIntervalMs, success);

  if (!success) {
    sendFailure("Failed to make location request");
  } else {
    mGnssLocationAsyncRequest = AsyncRequest(&kGnssLocationCookie);
    setTimer(CHRE_GNSS_ASYNC_RESULT_TIMEOUT_NS, true /* oneShot */,
             &mGnssLocationAsyncTimerHandle);
  }
}

void Manager::makeGnssMeasurementRequest() {
  // The list of measurement intervals to iterate; wraps around.
  static const uint32_t kMinIntervalMsList[] = {1000, 0};
  static size_t sIntervalIndex = 0;

  uint32_t minIntervalMs = 0;
  if (mGnssMeasurementTestStarted) {
    minIntervalMs = kMinIntervalMsList[sIntervalIndex];
    sIntervalIndex = (sIntervalIndex + 1) % ARRAY_SIZE(kMinIntervalMsList);
  } else {
    sIntervalIndex = 0;
  }

  bool success = false;
  if (minIntervalMs > 0) {
    success = chreGnssMeasurementSessionStartAsync(minIntervalMs,
                                                   &kGnssMeasurementCookie);
  } else {
    success = chreGnssMeasurementSessionStopAsync(&kGnssMeasurementCookie);
    // Reset the previous timestamp, since the GNSS internal clock may reset.
    mPrevGnssMeasurementEventTimestampNs = 0;
  }

  LOGI("Configure GNSS measurement interval %" PRIu32 " ms success ? %d",
       minIntervalMs, success);

  if (!success) {
    sendFailure("Failed to make measurement request");
  } else {
    mGnssMeasurementAsyncRequest = AsyncRequest(&kGnssMeasurementCookie);
    setTimer(CHRE_GNSS_ASYNC_RESULT_TIMEOUT_NS, true /* oneShot */,
             &mGnssMeasurementAsyncTimerHandle);
  }
}

void Manager::requestDelayedWifiScan() {
  if (mWifiTestStarted) {
    if (chreWifiGetCapabilities() & CHRE_WIFI_CAPABILITIES_ON_DEMAND_SCAN) {
      setTimer(kWifiScanInterval.toRawNanoseconds(), true /* oneShot */,
               &mWifiScanTimerHandle);
    } else {
      sendFailure("Platform has no on-demand scan capability");
    }
  }
}

void Manager::makeWwanCellInfoRequest() {
  if (mWwanTestStarted) {
    if (mWwanCellInfoAsyncRequest.has_value()) {
      if (chreGetTime() > mWwanCellInfoAsyncRequest->requestTimeNs +
                              CHRE_ASYNC_RESULT_TIMEOUT_NS) {
        sendFailure("Prev cell info request did not complete in time");
      }
    } else {
      bool success = chreWwanGetCellInfoAsync(&kWwanCellInfoCookie);

      LOGI("Cell info request success ? %d", success);

      if (!success) {
        sendFailure("Failed to make cell info request");
      } else {
        mWwanCellInfoAsyncRequest = AsyncRequest(&kWwanCellInfoCookie);
      }
    }
  }
}

void Manager::sendFailure(const char *errorMessage) {
  test_shared::sendTestResultWithMsgToHost(
      mHostEndpoint.value(),
      chre_stress_test_MessageType_TEST_RESULT /* messageType */,
      false /* success */, errorMessage);
}

}  // namespace stress_test

}  // namespace chre
