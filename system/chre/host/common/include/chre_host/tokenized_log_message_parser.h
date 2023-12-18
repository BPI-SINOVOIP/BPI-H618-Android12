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

#ifndef CHRE_TOKENIZED_LOG_MESSAGE_PARSER_H_
#define CHRE_TOKENIZED_LOG_MESSAGE_PARSER_H_

#include <memory>
#include "chre_host/daemon_base.h"
#include "chre_host/log_message_parser_base.h"

#include "pw_tokenizer/detokenize.h"

using pw::tokenizer::DetokenizedString;
using pw::tokenizer::Detokenizer;

namespace android {
namespace chre {

class ChreTokenizedLogMessageParser : public ChreLogMessageParserBase {
 public:
  virtual bool init() override final {
    mDetokenizer = logDetokenizerInit();
    return mDetokenizer != nullptr;
  }

  virtual void log(const uint8_t *logBuffer,
                   size_t logBufferSize) override final {
    parseAndEmitTokenizedLogMessages(logBuffer, logBufferSize,
                                     mDetokenizer.get());
  }

  void logV2(const uint8_t *logBuffer, size_t logBufferSize) override {
    // TODO(b/172654554): Modify tokenized logging to only support v2 logs.
  }

 private:
  std::unique_ptr<Detokenizer> mDetokenizer;

  /**
   * Initialize the Log Detokenizer
   *
   * The log detokenizer reads a binary database file that contains key value
   * pairs of hash-keys <--> Decoded log messages, and creates an instance
   * of the Detokenizer.
   *
   * @return an instance of the Detokenizer
   */
  std::unique_ptr<Detokenizer> logDetokenizerInit() {
    constexpr const char kLogDatabaseFilePath[] =
        "/vendor/etc/chre/libchre_log_database.bin";
    std::vector<uint8_t> tokenData;
    if (ChreDaemonBase::readFileContents(kLogDatabaseFilePath, &tokenData)) {
      pw::tokenizer::TokenDatabase database =
          pw::tokenizer::TokenDatabase::Create(tokenData);
      if (database.ok()) {
        return std::make_unique<Detokenizer>(database);
      } else {
        LOGE("CHRE Token database creation not OK");
      }
    } else {
      LOGE("Failed to read CHRE Token database file");
    }
    return std::unique_ptr<Detokenizer>(nullptr);
  }

  // Log messages are routed through ashLog if tokenized logging
  // is disabled, so only parse tokenized log messages here.
  void parseAndEmitTokenizedLogMessages(const uint8_t *message,
                                        unsigned int messageLen,
                                        const Detokenizer *detokenizer) {
    if (detokenizer != nullptr) {
      // TODO: Pull out common code from the tokenized/standard log
      // parser functions when we implement batching for tokenized
      // logs (b/148873804)
      constexpr size_t kLogMessageHeaderSize =
          1 /*logLevel*/ + sizeof(uint64_t) /*timestamp*/;
      uint8_t level = *message;
      ++message;

      uint64_t timestampNanos;
      memcpy(&timestampNanos, message, sizeof(uint64_t));
      timestampNanos = le64toh(timestampNanos);
      message += sizeof(uint64_t);

      DetokenizedString detokenizedLog =
          detokenizer->Detokenize(message, messageLen - kLogMessageHeaderSize);
      std::string decodedLog = detokenizedLog.BestStringWithErrors();
      uint32_t timestampMillis =
          timestampNanos / chre::kOneMillisecondInNanoseconds;
      emitLogMessage(level, timestampNanos, decodedLog.c_str());
    } else {
      // log an error and risk log spam? fail silently? log once?
    }
  }
};

}  // namespace chre
}  // namespace android

#endif  // CHRE_TOKENIZED_LOG_MESSAGE_PARSER_H_