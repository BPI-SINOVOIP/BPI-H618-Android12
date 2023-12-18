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

#ifndef CHRE_LOG_MESSAGE_PARSER_BASE_H_
#define CHRE_LOG_MESSAGE_PARSER_BASE_H_

#include <cinttypes>
#include <cstdint>

#include "chre_host/log.h"

#include <android-base/logging.h>

namespace android {
namespace chre {

// TODO: Since nanoapp logging won't use tokenized logging, refactor this
// to have the base class handle normal log messages, and a subclass
// that could extend to handle log messages of other types (eg: tokenized
// logging), and can then invoke the parent class methods if they receive
// a log of the 'normal' type.
class ChreLogMessageParserBase {
 public:
  ChreLogMessageParserBase();

  virtual ~ChreLogMessageParserBase() {}

  virtual bool init() {
    return true;
  };

  virtual bool deinit() {
    return true;
  };

  //! Logs from a log buffer containing one or more log messages (version 1)
  virtual void log(const uint8_t *logBuffer, size_t logBufferSize);

  //! Logs from a log buffer containing one or more log messages (version 2)
  virtual void logV2(const uint8_t *logBuffer, size_t logBufferSize,
                     uint32_t numLogsDropped);

  /**
   * With verbose logging enabled (via enableVerbose()), dump a
   * binary log buffer to a human-readable string
   *
   * @param logBuffer buffer to be output as a string
   * @param logBufferSize size of the buffer being output
   */
  virtual void dump(const uint8_t *logBuffer, size_t logBufferSize);

  void enableVerbose(bool enable) {
    mVerboseLoggingEnabled = enable;
  }

 protected:
  static constexpr char kHubLogFormatStr[] = "@ %3" PRIu32 ".%03" PRIu32 ": %s";

  bool mVerboseLoggingEnabled;

  static android_LogPriority chreLogLevelToAndroidLogPriority(uint8_t level);

  void emitLogMessage(uint8_t level, uint32_t timestampMillis,
                      const char *logMessage);

 private:
  enum LogLevel : uint8_t {
    ERROR = 1,
    WARNING = 2,
    INFO = 3,
    DEBUG = 4,
    VERBOSE = 5,
  };

  //! See host_messages.fbs for the definition of this struct.
  struct LogMessage {
    enum LogLevel logLevel;
    uint64_t timestampNanos;
    char logMessage[];
  } __attribute__((packed));

  //! See host_messages.fbs for the definition of this struct.
  struct LogMessageV2 {
    enum LogLevel logLevel;
    uint32_t timestampMillis;
    char logMessage[];
  } __attribute__((packed));

  //! The number of logs dropped since CHRE start
  uint32_t mNumLogsDropped = 0;
};

}  // namespace chre
}  // namespace android

#endif  // CHRE_LOG_MESSAGE_PARSER_BASE_H_
