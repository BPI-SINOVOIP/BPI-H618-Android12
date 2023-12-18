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

#ifndef CHRE_PLATFORM_FREERTOS_PLATFORM_DEBUG_DUMP_MANAGER_BASE_H_
#define CHRE_PLATFORM_FREERTOS_PLATFORM_DEBUG_DUMP_MANAGER_BASE_H_

#include <cstdbool>
#include <cstddef>
#include <cstdint>

namespace chre {

/**
 * FreeRTOS-specific debug dump functionality.
 */
class PlatformDebugDumpManagerBase {
 public:
  /**
   * To be called when receiving a debug dump request from host.
   *
   * @param hostClientId The host client ID that requested the debug dump.
   */
  void onDebugDumpRequested(uint16_t hostClientId);

  /**
   * @see PlatformDebugDumpManager::sendDebugDump
   */
  void sendDebugDumpResult(const char *debugStr, size_t debugStrSize,
                           bool complete);

 protected:
  //! Host client ID that triggered the debug dump process.
  uint16_t mHostClientId = 0;

  //! Number of times sendDebugDumpToHost called with debugStrSize > 0.
  uint32_t mDataCount = 0;

  //! Whenther the last debug dump session has been marked complete.
  bool mComplete = true;

  static constexpr size_t kDebugDumpStrMaxSize = CHRE_MESSAGE_TO_HOST_MAX_SIZE;
};

}  // namespace chre

#endif  // CHRE_PLATFORM_FREERTOS_PLATFORM_DEBUG_DUMP_MANAGER_BASE_H_
