/* * Copyright (C) 2020 The Android Open Source Project
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

#include "chre_cross_validator_wifi_manager.h"

#include <stdio.h>
#include <algorithm>
#include <cinttypes>
#include <cstring>

#include "chre/util/nanoapp/assert.h"
#include "chre/util/nanoapp/callbacks.h"
#include "chre/util/nanoapp/log.h"
#include "chre_cross_validation_wifi.nanopb.h"
#include "chre_test_common.nanopb.h"

#define LOG_TAG "ChreCrossValidatorWifi"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

namespace chre {

namespace cross_validator_wifi {

// Fake scan monitor cookie which is not used
constexpr uint32_t kScanMonitoringCookie = 0;

void Manager::handleEvent(uint32_t senderInstanceId, uint16_t eventType,
                          const void *eventData) {
  switch (eventType) {
    case CHRE_EVENT_MESSAGE_FROM_HOST:
      handleMessageFromHost(
          senderInstanceId,
          static_cast<const chreMessageFromHostData *>(eventData));
      break;
    case CHRE_EVENT_WIFI_ASYNC_RESULT:
      handleWifiAsyncResult(static_cast<const chreAsyncResult *>(eventData));
      break;
    case CHRE_EVENT_WIFI_SCAN_RESULT:
      handleWifiScanResult(static_cast<const chreWifiScanEvent *>(eventData));
      break;
    default:
      LOGE("Unknown message type %" PRIu16 "received when handling event",
           eventType);
  }
}

void Manager::handleMessageFromHost(uint32_t senderInstanceId,
                                    const chreMessageFromHostData *hostData) {
  if (senderInstanceId != CHRE_INSTANCE_ID) {
    LOGE("Incorrect sender instance id: %" PRIu32, senderInstanceId);
  } else {
    mCrossValidatorState.hostEndpoint = hostData->hostEndpoint;
    switch (hostData->messageType) {
      case chre_cross_validation_wifi_MessageType_STEP_START: {
        pb_istream_t stream = pb_istream_from_buffer(
            static_cast<const pb_byte_t *>(
                const_cast<const void *>(hostData->message)),
            hostData->messageSize);
        chre_cross_validation_wifi_StepStartCommand stepStartCommand;
        if (!pb_decode(&stream,
                       chre_cross_validation_wifi_StepStartCommand_fields,
                       &stepStartCommand)) {
          LOGE("Error decoding StepStartCommand");
        } else {
          handleStepStartMessage(stepStartCommand);
        }
        break;
      }
      case chre_cross_validation_wifi_MessageType_SCAN_RESULT:
        handleDataMessage(hostData);
        break;
      default:
        LOGE("Unknown message type %" PRIu32 " for host message",
             hostData->messageType);
    }
  }
}

void Manager::handleStepStartMessage(
    chre_cross_validation_wifi_StepStartCommand stepStartCommand) {
  switch (stepStartCommand.step) {
    case chre_cross_validation_wifi_Step_INIT:
      LOGE("Received StepStartCommand for INIT step");
      CHRE_ASSERT(false);
      break;
    case chre_cross_validation_wifi_Step_CAPABILITIES: {
      chre_cross_validation_wifi_WifiCapabilities wifiCapabilities =
          makeWifiCapabilitiesMessage(chreWifiGetCapabilities());
      encodeAndSendMessageToHost(
          static_cast<void *>(&wifiCapabilities),
          chre_cross_validation_wifi_WifiCapabilities_fields,
          chre_cross_validation_wifi_MessageType_WIFI_CAPABILITIES);
      break;
    }
    case chre_cross_validation_wifi_Step_SETUP: {
      if (!chreWifiConfigureScanMonitorAsync(true /* enable */,
                                             &kScanMonitoringCookie)) {
        LOGE("chreWifiConfigureScanMonitorAsync() failed");
        chre_test_common_TestResult testResult =
            makeTestResultProtoMessage(false, "setupWifiScanMonitoring failed");
        encodeAndSendMessageToHost(
            static_cast<void *>(&testResult),
            chre_test_common_TestResult_fields,
            chre_cross_validation_wifi_MessageType_STEP_RESULT);
      } else {
        LOGD("chreWifiConfigureScanMonitorAsync() succeeded");
      }
      break;
    }
    case chre_cross_validation_wifi_Step_VALIDATE:
      LOGE("start message received in VALIDATE phase");
      break;
  }
  mStep = stepStartCommand.step;
}

void Manager::handleDataMessage(const chreMessageFromHostData *hostData) {
  pb_istream_t stream =
      pb_istream_from_buffer(reinterpret_cast<const pb_byte_t *>(
                                 const_cast<const void *>(hostData->message)),
                             hostData->messageSize);
  WifiScanResult scanResult(&stream);
  uint8_t scanResultIndex = scanResult.getResultIndex();
  mApScanResultsSize = scanResult.getTotalNumResults();
  if (scanResultIndex > mApScanResultsSize) {
    LOGE("AP scan result index is greater than scan results size");
  } else {
    mApScanResults[scanResultIndex] = scanResult;
    if (scanResult.isLastMessage()) {
      mApDataCollectionDone = true;
      if (mChreDataCollectionDone) {
        compareAndSendResultToHost();
      }
    }
  }
}

void Manager::handleWifiScanResult(const chreWifiScanEvent *event) {
  for (uint8_t i = 0; i < event->resultCount; i++) {
    mChreScanResults[mChreScanResultsI++] = WifiScanResult(event->results[i]);
  }
  mNumResultsProcessed += event->resultCount;
  if (mNumResultsProcessed >= event->resultTotal) {
    mChreScanResultsSize = mChreScanResultsI;
    mChreDataCollectionDone = true;
    if (mApDataCollectionDone) {
      compareAndSendResultToHost();
    }
  }
}

void Manager::compareAndSendResultToHost() {
  chre_test_common_TestResult testResult;
  // TODO(b/185188753): Log info about all scan results so that it is easier
  // to figure out which AP or CHRE scan results are missing or corrupted.
  if (mApScanResultsSize != mChreScanResultsSize) {
    testResult = makeTestResultProtoMessage(
        false, "There is a different number of AP and CHRE scan results.");
    LOGE("AP and CHRE wifi scan result counts differ, AP = %" PRIu8
         ", CHRE = %" PRIu8,
         mApScanResultsSize, mChreScanResultsSize);
  } else {
    verifyScanResults(&testResult);
  }
  encodeAndSendMessageToHost(
      static_cast<const void *>(&testResult),
      chre_test_common_TestResult_fields,
      chre_cross_validation_wifi_MessageType_STEP_RESULT);
}

void Manager::verifyScanResults(chre_test_common_TestResult *testResultOut) {
  bool allResultsValid = true;
  for (uint8_t i = 0; i < mChreScanResultsSize; i++) {
    const WifiScanResult &chreScanResult = mChreScanResults[i];
    uint8_t apScanResultIndex;
    bool didFind = getMatchingScanResult(mApScanResults, mApScanResultsSize,
                                         chreScanResult, &apScanResultIndex);
    // TODO(b/185188753): Log info about each result iterated through here and
    // do not just break at the first error.
    if (didFind) {
      WifiScanResult &apScanResult = mApScanResults[apScanResultIndex];
      if (apScanResult.getSeen()) {
        *testResultOut = makeTestResultProtoMessage(
            false, "Saw a CHRE scan result with a duplicate BSSID.");
        allResultsValid = false;
        break;
      }
      if (!WifiScanResult::areEqual(chreScanResult, apScanResult)) {
        *testResultOut =
            makeTestResultProtoMessage(false,
                                       "Fields differ between an AP and "
                                       "CHRE scan result with same Bssid.");
        allResultsValid = false;
        break;
      }
      apScanResult.didSee();
    } else {
      // Error CHRE BSSID does not match any AP
      *testResultOut = makeTestResultProtoMessage(
          false,
          "Could not find an AP scan result with the same Bssid as a CHRE "
          "result");
      allResultsValid = false;
      break;
    }
  }
  if (allResultsValid) {
    *testResultOut = makeTestResultProtoMessage(true);
  }
}

bool Manager::getMatchingScanResult(WifiScanResult *results,
                                    uint8_t resultsSize,
                                    const WifiScanResult &queryResult,
                                    uint8_t *resultIndexOut) {
  for (uint8_t i = 0; i < resultsSize; i++) {
    if (WifiScanResult::bssidsAreEqual(results[i], queryResult)) {
      // Mark this scan result as already seen so that the next time it is used
      // as a match the test will fail because of duplicate scan results.
      *resultIndexOut = i;
      return true;
    }
  }
  return false;
}

bool Manager::encodeErrorMessage(pb_ostream_t *stream,
                                 const pb_field_t * /*field*/,
                                 void *const *arg) {
  const char *str = static_cast<const char *>(const_cast<const void *>(*arg));
  size_t len = strlen(str);
  return pb_encode_tag_for_field(
             stream, &chre_test_common_TestResult_fields
                         [chre_test_common_TestResult_errorMessage_tag - 1]) &&
         pb_encode_string(stream, reinterpret_cast<const pb_byte_t *>(str),
                          len);
}

chre_test_common_TestResult Manager::makeTestResultProtoMessage(
    bool success, const char *errMessage) {
  // TODO(b/154271547): Move this implementation into
  // common/shared/send_message.cc::sendTestResultToHost
  chre_test_common_TestResult testResult =
      chre_test_common_TestResult_init_default;
  testResult.has_code = true;
  testResult.code = success ? chre_test_common_TestResult_Code_PASSED
                            : chre_test_common_TestResult_Code_FAILED;
  if (!success && errMessage != nullptr) {
    testResult.errorMessage = {.funcs = {.encode = encodeErrorMessage},
                               .arg = const_cast<char *>(errMessage)};
  }
  return testResult;
}

chre_cross_validation_wifi_WifiCapabilities
Manager::makeWifiCapabilitiesMessage(uint32_t capabilitiesFromChre) {
  chre_cross_validation_wifi_WifiCapabilities capabilities;
  capabilities.has_wifiCapabilities = true;
  capabilities.wifiCapabilities = capabilitiesFromChre;
  return capabilities;
}

void Manager::encodeAndSendMessageToHost(const void *message,
                                         const pb_field_t *fields,
                                         uint32_t messageType) {
  size_t encodedSize;
  if (!pb_get_encoded_size(&encodedSize, fields, message)) {
    LOGE("Could not get encoded size of test result message");
  } else {
    pb_byte_t *buffer = static_cast<pb_byte_t *>(chreHeapAlloc(encodedSize));
    if (buffer == nullptr) {
      LOG_OOM();
    } else {
      pb_ostream_t ostream = pb_ostream_from_buffer(buffer, encodedSize);
      if (!pb_encode(&ostream, fields, message)) {
        LOGE("Could not encode data proto message");
      } else if (!chreSendMessageToHostEndpoint(
                     static_cast<void *>(buffer), encodedSize, messageType,
                     mCrossValidatorState.hostEndpoint,
                     heapFreeMessageCallback)) {
        LOGE("Could not send message to host");
      }
    }
  }
}

void Manager::handleWifiAsyncResult(const chreAsyncResult *result) {
  chre_test_common_TestResult testResult;
  bool sendMessage = false;
  if (result->requestType == CHRE_WIFI_REQUEST_TYPE_CONFIGURE_SCAN_MONITOR) {
    if (mStep != chre_cross_validation_wifi_Step_SETUP) {
      testResult = makeTestResultProtoMessage(
          false, "Received scan monitor result event when step is not SETUP");
      sendMessage = true;
    } else {
      if (result->success) {
        LOGD("Wifi scan monitoring setup successfully");
        testResult = makeTestResultProtoMessage(true);
        sendMessage = true;
      } else {
        LOGE("Wifi scan monitoring setup failed async w/ error code %" PRIu8
             ".",
             result->errorCode);
        testResult = makeTestResultProtoMessage(
            false, "Wifi scan monitoring setup failed async.");
        sendMessage = true;
      }
    }
  } else {
    testResult = makeTestResultProtoMessage(
        false, "Unknown chre async result type received");
    sendMessage = true;
  }
  if (sendMessage) {
    encodeAndSendMessageToHost(
        static_cast<void *>(&testResult), chre_test_common_TestResult_fields,
        chre_cross_validation_wifi_MessageType_STEP_RESULT);
  }
}

}  // namespace cross_validator_wifi

}  // namespace chre
