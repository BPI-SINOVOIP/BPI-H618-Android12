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

#include "chre/pal/gnss.h"

#include "chre/util/memory.h"
#include "chre/util/unique_ptr.h"

#include <chrono>
#include <cinttypes>
#include <future>
#include <thread>

/**
 * A simulated implementation of the GNSS PAL for the linux platform.
 */
namespace {
const struct chrePalSystemApi *gSystemApi = nullptr;
const struct chrePalGnssCallbacks *gCallbacks = nullptr;

//! Thread to deliver asynchronous location data after a CHRE request.
std::thread gLocationEventsThread;
std::promise<void> gStopLocationEventsThread;

//! Thead to use when delivering a location status update.
std::thread gLocationStatusThread;

//! Thread to deliver asynchronous measurement data after a CHRE request.
std::thread gMeasurementEventsThread;
std::promise<void> gStopMeasurementEventsThread;

//! Thead to use when delivering a measurement status update.
std::thread gMeasurementStatusThread;

void sendLocationEvents(uint32_t minIntervalMs) {
  gCallbacks->locationStatusChangeCallback(true, CHRE_ERROR_NONE);

  std::future<void> signal = gStopLocationEventsThread.get_future();
  while (signal.wait_for(std::chrono::milliseconds(minIntervalMs)) ==
         std::future_status::timeout) {
    auto event = chre::MakeUniqueZeroFill<struct chreGnssLocationEvent>();
    event->timestamp = gSystemApi->getCurrentTime();
    gCallbacks->locationEventCallback(event.release());
  }
}

void sendMeasurementEvents(uint32_t minIntervalMs) {
  gCallbacks->measurementStatusChangeCallback(true, CHRE_ERROR_NONE);

  std::future<void> signal = gStopMeasurementEventsThread.get_future();
  while (signal.wait_for(std::chrono::milliseconds(minIntervalMs)) ==
         std::future_status::timeout) {
    auto event = chre::MakeUniqueZeroFill<struct chreGnssDataEvent>();
    auto measurement = chre::MakeUniqueZeroFill<struct chreGnssMeasurement>();
    measurement->c_n0_dbhz = 63.0f;

    event->measurements = measurement.release();
    event->measurement_count = 1;
    event->clock.time_ns = static_cast<int64_t>(gSystemApi->getCurrentTime());
    gCallbacks->measurementEventCallback(event.release());
  }
}

void stopLocation() {
  gCallbacks->locationStatusChangeCallback(false, CHRE_ERROR_NONE);
}

void stopMeasurement() {
  gCallbacks->measurementStatusChangeCallback(false, CHRE_ERROR_NONE);
}

void stopLocationThreads() {
  if (gLocationEventsThread.joinable()) {
    gStopLocationEventsThread.set_value();
    gLocationEventsThread.join();
  }
  if (gLocationStatusThread.joinable()) {
    gLocationStatusThread.join();
  }
}

void stopMeasurementThreads() {
  if (gMeasurementEventsThread.joinable()) {
    gStopMeasurementEventsThread.set_value();
    gMeasurementEventsThread.join();
  }
  if (gMeasurementStatusThread.joinable()) {
    gMeasurementStatusThread.join();
  }
}

uint32_t chrePalGnssGetCapabilities() {
  return CHRE_GNSS_CAPABILITIES_LOCATION | CHRE_GNSS_CAPABILITIES_MEASUREMENTS;
}

bool chrePalControlLocationSession(bool enable, uint32_t minIntervalMs,
                                   uint32_t /* minTimeToNextFixMs */) {
  stopLocationThreads();

  if (enable) {
    gStopLocationEventsThread = std::promise<void>();
    gLocationEventsThread = std::thread(sendLocationEvents, minIntervalMs);
  } else {
    gLocationStatusThread = std::thread(stopLocation);
  }

  return true;
}

void chrePalGnssReleaseLocationEvent(struct chreGnssLocationEvent *event) {
  chre::memoryFree(event);
}

bool chrePalControlMeasurementSession(bool enable, uint32_t minIntervalMs) {
  stopMeasurementThreads();

  if (enable) {
    gStopMeasurementEventsThread = std::promise<void>();
    gMeasurementEventsThread =
        std::thread(sendMeasurementEvents, minIntervalMs);
  } else {
    gMeasurementStatusThread = std::thread(stopMeasurement);
  }

  return true;
}

void chrePalGnssReleaseMeasurementDataEvent(struct chreGnssDataEvent *event) {
  chre::memoryFree(
      const_cast<struct chreGnssMeasurement *>(event->measurements));
  chre::memoryFree(event);
}

void chrePalGnssApiClose() {
  stopLocationThreads();
  stopMeasurementThreads();
}

bool chrePalGnssApiOpen(const struct chrePalSystemApi *systemApi,
                        const struct chrePalGnssCallbacks *callbacks) {
  chrePalGnssApiClose();

  bool success = false;
  if (systemApi != nullptr && callbacks != nullptr) {
    gSystemApi = systemApi;
    gCallbacks = callbacks;
    success = true;
  }

  return success;
}

}  // anonymous namespace

const struct chrePalGnssApi *chrePalGnssGetApi(uint32_t requestedApiVersion) {
  static const struct chrePalGnssApi kApi = {
      .moduleVersion = CHRE_PAL_GNSS_API_CURRENT_VERSION,
      .open = chrePalGnssApiOpen,
      .close = chrePalGnssApiClose,
      .getCapabilities = chrePalGnssGetCapabilities,
      .controlLocationSession = chrePalControlLocationSession,
      .releaseLocationEvent = chrePalGnssReleaseLocationEvent,
      .controlMeasurementSession = chrePalControlMeasurementSession,
      .releaseMeasurementDataEvent = chrePalGnssReleaseMeasurementDataEvent,
  };

  if (!CHRE_PAL_VERSIONS_ARE_COMPATIBLE(kApi.moduleVersion,
                                        requestedApiVersion)) {
    return nullptr;
  } else {
    return &kApi;
  }
}
