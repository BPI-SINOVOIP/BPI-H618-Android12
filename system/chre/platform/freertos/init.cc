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

#include "chre/core/init.h"

#include "chpp/platform/chpp_init.h"
#include "chre/core/event_loop_manager.h"
#include "chre/core/static_nanoapps.h"
#include "chre/platform/shared/dram_vote_client.h"
#include "chre/target_platform/init.h"

#ifdef CHRE_USE_BUFFERED_LOGGING
#include "chre/platform/shared/log_buffer_manager.h"
#include "chre/target_platform/macros.h"
#endif

#include "task.h"

namespace chre {
namespace freertos {
namespace {

constexpr configSTACK_DEPTH_TYPE kChreTaskStackDepthWords = 0x800;

constexpr UBaseType_t kChreTaskPriority = tskIDLE_PRIORITY + 1;

TaskHandle_t gChreTaskHandle;

#ifdef CHRE_USE_BUFFERED_LOGGING

TaskHandle_t gChreFlushTaskHandle;

#ifdef CHRE_HIGH_POWER_TEXT_ATTRIBUTE
CHRE_HIGH_POWER_TEXT_ATTRIBUTE
#endif
uint8_t gSecondaryLogBufferData[CHRE_LOG_BUFFER_DATA_SIZE];

uint8_t gPrimaryLogBufferData[CHRE_LOG_BUFFER_DATA_SIZE];

#endif

// This function is intended to be the task action function for FreeRTOS.
// It Initializes CHRE, runs the event loop, and only exits if it receives
// a message to shutdown. Note that depending on the hardware platform this
// runs on, CHRE might create additional threads, which are cleaned up when
// CHRE exits.
void chreThreadEntry(void *context) {
  DramVoteClientSingleton::init();

  chre::init();
  chre::EventLoopManagerSingleton::get()->lateInit();
  chre::loadStaticNanoapps();

  chre::EventLoopManagerSingleton::get()->getEventLoop().run();

  // we only get here if the CHRE EventLoop exited
  chre::deinit();

  DramVoteClientSingleton::deinit();

  vTaskDelete(nullptr);
  gChreTaskHandle = nullptr;
}

#ifdef CHRE_USE_BUFFERED_LOGGING
void chreFlushLogsToHostThreadEntry(void *context) {
  // Never exits
  chre::LogBufferManagerSingleton::get()->startSendLogsToHostLoop();
}
#endif

}  // namespace

#ifdef CHRE_USE_BUFFERED_LOGGING
const char *getChreFlushTaskName();
#endif

BaseType_t init() {
  BaseType_t rc = pdPASS;

#ifdef CHRE_USE_BUFFERED_LOGGING
  chre::LogBufferManagerSingleton::init(gPrimaryLogBufferData,
                                        gSecondaryLogBufferData,
                                        sizeof(gPrimaryLogBufferData));

  rc = xTaskCreate(chreFlushLogsToHostThreadEntry, getChreFlushTaskName(),
                   kChreTaskStackDepthWords, nullptr /* args */,
                   kChreTaskPriority, &gChreFlushTaskHandle);
#endif

  if (rc == pdPASS) {
    rc = xTaskCreate(chreThreadEntry, getChreTaskName(),
                     kChreTaskStackDepthWords, nullptr /* args */,
                     kChreTaskPriority, &gChreTaskHandle);
  }

  CHRE_ASSERT(rc == pdPASS);

  chpp::init();

  return rc;
}

void deinit() {
  // On a deinit call, we just stop the CHRE event loop. This causes the 'run'
  // method in the task function exit, and move on to handle task cleanup
  if (gChreTaskHandle != nullptr) {
    chre::EventLoopManagerSingleton::get()->getEventLoop().stop();
  }

  chpp::deinit();
}

const char *getChreTaskName() {
  static constexpr char kChreTaskName[] = "CHRE";
  return kChreTaskName;
}

#ifdef CHRE_USE_BUFFERED_LOGGING
const char *getChreFlushTaskName() {
  static constexpr char kChreFlushTaskName[] = "CHRELogs";
  return kChreFlushTaskName;
}
#endif

}  // namespace freertos

BaseType_t getChreTaskPriority() {
  return freertos::kChreTaskPriority;
}

}  // namespace chre
