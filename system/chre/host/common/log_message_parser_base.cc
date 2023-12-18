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

#include <endian.h>
#include <cstdio>

#include "chre/util/time.h"
#include "chre_host/log_message_parser_base.h"

using chre::kOneMillisecondInNanoseconds;
using chre::kOneSecondInMilliseconds;

namespace android {
namespace chre {

namespace {
#if defined(LOG_NDEBUG) || LOG_NDEBUG != 0
constexpr bool kVerboseLoggingEnabled = true;
#else
constexpr bool kVerboseLoggingEnabled = false;
#endif
}  // anonymous namespace

ChreLogMessageParserBase::ChreLogMessageParserBase()
    : mVerboseLoggingEnabled(kVerboseLoggingEnabled) {}

void ChreLogMessageParserBase::dump(const uint8_t *buffer, size_t size) {
  if (mVerboseLoggingEnabled) {
    char line[32];
    char lineChars[32];
    int offset = 0;
    int offsetChars = 0;

    size_t orig_size = size;
    if (size > 128) {
      size = 128;
      LOGV("Dumping first 128 bytes of buffer of size %zu", orig_size);
    } else {
      LOGV("Dumping buffer of size %zu bytes", size);
    }
    for (size_t i = 1; i <= size; ++i) {
      offset += snprintf(&line[offset], sizeof(line) - offset, "%02x ",
                         buffer[i - 1]);
      offsetChars +=
          snprintf(&lineChars[offsetChars], sizeof(lineChars) - offsetChars,
                   "%c", (isprint(buffer[i - 1])) ? buffer[i - 1] : '.');
      if ((i % 8) == 0) {
        LOGV("  %s\t%s", line, lineChars);
        offset = 0;
        offsetChars = 0;
      } else if ((i % 4) == 0) {
        offset += snprintf(&line[offset], sizeof(line) - offset, " ");
      }
    }

    if (offset > 0) {
      char tabs[8];
      char *pos = tabs;
      while (offset < 28) {
        *pos++ = '\t';
        offset += 8;
      }
      *pos = '\0';
      LOGV("  %s%s%s", line, tabs, lineChars);
    }
  }
}

android_LogPriority ChreLogMessageParserBase::chreLogLevelToAndroidLogPriority(
    uint8_t level) {
  switch (level) {
    case LogLevel::ERROR:
      return ANDROID_LOG_ERROR;
    case LogLevel::WARNING:
      return ANDROID_LOG_WARN;
    case LogLevel::INFO:
      return ANDROID_LOG_INFO;
    case LogLevel::DEBUG:
      return ANDROID_LOG_DEBUG;
    default:
      return ANDROID_LOG_SILENT;
  }
}

void ChreLogMessageParserBase::log(const uint8_t *logBuffer,
                                   size_t logBufferSize) {
  size_t bufferIndex = 0;
  while (bufferIndex < logBufferSize) {
    const LogMessage *message =
        reinterpret_cast<const LogMessage *>(&logBuffer[bufferIndex]);
    uint64_t timeNs = le64toh(message->timestampNanos);
    emitLogMessage(message->logLevel, timeNs / kOneMillisecondInNanoseconds,
                   message->logMessage);
    bufferIndex += sizeof(LogMessage) +
                   strnlen(message->logMessage, logBufferSize - bufferIndex) +
                   1;
  }
}

void ChreLogMessageParserBase::logV2(const uint8_t *logBuffer,
                                     size_t logBufferSize,
                                     uint32_t numLogsDropped) {
  size_t bufferIndex = 0;
  if (numLogsDropped < mNumLogsDropped) {
    LOGE(
        "The numLogsDropped value received from CHRE is less than the last "
        "value received. Received: %" PRIu32 " Last value: %" PRIu32,
        numLogsDropped, mNumLogsDropped);
  }
  // Log the number of logs dropped once before logging remaining logs from CHRE
  uint32_t diffLogsDropped = numLogsDropped - mNumLogsDropped;
  mNumLogsDropped = numLogsDropped;
  if (diffLogsDropped > 0) {
    LOGI("# logs dropped: %" PRIu32, diffLogsDropped);
  }
  while (bufferIndex < logBufferSize) {
    const LogMessageV2 *message =
        reinterpret_cast<const LogMessageV2 *>(&logBuffer[bufferIndex]);
    emitLogMessage(message->logLevel, le32toh(message->timestampMillis),
                   message->logMessage);
    bufferIndex += sizeof(LogMessageV2) +
                   strnlen(message->logMessage, logBufferSize - bufferIndex) +
                   1;
  }
}

void ChreLogMessageParserBase::emitLogMessage(uint8_t level,
                                              uint32_t timestampMillis,
                                              const char *logMessage) {
  constexpr const char kLogTag[] = "CHRE";
  uint32_t timeSec = timestampMillis / kOneSecondInMilliseconds;
  uint32_t timeMsRemainder = timestampMillis % kOneSecondInMilliseconds;
  android_LogPriority priority = chreLogLevelToAndroidLogPriority(level);
  LOG_PRI(priority, kLogTag, kHubLogFormatStr, timeSec, timeMsRemainder,
          logMessage);
}

}  // namespace chre
}  // namespace android
