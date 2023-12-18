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

#include <gtest/gtest.h>
#include <string>

#include "chre/platform/atomic.h"
#include "chre/platform/condition_variable.h"
#include "chre/platform/mutex.h"
#include "chre/platform/shared/log_buffer.h"

namespace chre {

// TODO(b/146164384): Test that the onLogsReady callback is called
//                    asynchronously

class TestLogBufferCallback : public LogBufferCallbackInterface {
 public:
  void onLogsReady() {
    // Do nothing
  }
};

static constexpr size_t kDefaultBufferSize = 1024;
static constexpr size_t kBytesBeforeLogData = 5;

// Helpers
void copyStringWithOffset(char *destination, const char *source,
                          size_t sourceOffset) {
  size_t strlength = strlen(source + sourceOffset);
  // +1 to copy nullbyte on the end
  memcpy(destination, source + sourceOffset, strlength + 1);
}

TEST(LogBuffer, HandleOneLogAndCopy) {
  char buffer[kDefaultBufferSize];
  constexpr size_t kOutBufferSize = 20;
  char outBuffer[kOutBufferSize];
  const char *testLogStr = "test";
  char testedBuffer[kOutBufferSize];
  TestLogBufferCallback callback;

  LogBuffer logBuffer(&callback, buffer, kDefaultBufferSize);
  logBuffer.handleLog(LogBufferLogLevel::INFO, 0, testLogStr);
  size_t numLogsDropped;
  size_t bytesCopied =
      logBuffer.copyLogs(outBuffer, kOutBufferSize, &numLogsDropped);

  EXPECT_EQ(bytesCopied, strlen(testLogStr) +
                             kBytesBeforeLogData /*loglevel, timestamp*/ + 1);
  copyStringWithOffset(testedBuffer, outBuffer, strlen(testLogStr) + 1);
  EXPECT_TRUE(strcmp(testedBuffer, testLogStr) == 0);
}

TEST(LogBuffer, HandleTwoLogsAndCopy) {
  char buffer[kDefaultBufferSize];
  constexpr size_t kOutBufferSize = 30;
  char outBuffer[kOutBufferSize];
  const char *testLogStr = "test";
  const char *testLogStr2 = "test2";
  char testedBuffer[kOutBufferSize];
  TestLogBufferCallback callback;

  LogBuffer logBuffer(&callback, buffer, kDefaultBufferSize);
  logBuffer.handleLog(LogBufferLogLevel::INFO, 0, testLogStr);
  logBuffer.handleLog(LogBufferLogLevel::INFO, 0, testLogStr2);
  size_t numLogsDropped;
  size_t bytesCopied =
      logBuffer.copyLogs(outBuffer, kOutBufferSize, &numLogsDropped);

  EXPECT_EQ(bytesCopied, strlen(testLogStr) + strlen(testLogStr2) +
                             2 * kBytesBeforeLogData /*loglevel, timestamp*/ +
                             2);
  copyStringWithOffset(testedBuffer, outBuffer, kBytesBeforeLogData);
  EXPECT_TRUE(strcmp(testedBuffer, testLogStr) == 0);
  copyStringWithOffset(testedBuffer, outBuffer,
                       2 * kBytesBeforeLogData + strlen(testLogStr) + 1);
  EXPECT_TRUE(strcmp(testedBuffer, testLogStr2) == 0);
}

TEST(LogBuffer, FailOnMoreCopyThanHandle) {
  char buffer[kDefaultBufferSize];
  constexpr size_t kOutBufferSize = 20;
  char outBuffer[kOutBufferSize];
  const char *testLogStr = "test";
  TestLogBufferCallback callback;

  LogBuffer logBuffer(&callback, buffer, kDefaultBufferSize);
  logBuffer.handleLog(LogBufferLogLevel::INFO, 0, testLogStr);
  size_t numLogsDropped;
  logBuffer.copyLogs(outBuffer, kOutBufferSize, &numLogsDropped);
  size_t bytesCopied =
      logBuffer.copyLogs(outBuffer, kOutBufferSize, &numLogsDropped);

  EXPECT_EQ(bytesCopied, 0);
}

TEST(LogBuffer, FailOnHandleLargerLogThanBufferSize) {
  char buffer[kDefaultBufferSize];
  constexpr size_t kOutBufferSize = 20;
  char outBuffer[kOutBufferSize];
  // Note the size of this log is too big to fit in the buffer that we are
  // using for the LogBuffer object
  std::string testLogStrStr(1025, 'a');
  TestLogBufferCallback callback;

  LogBuffer logBuffer(&callback, buffer, kDefaultBufferSize);
  logBuffer.handleLog(LogBufferLogLevel::INFO, 0, testLogStrStr.c_str());
  size_t numLogsDropped;
  size_t bytesCopied =
      logBuffer.copyLogs(outBuffer, kOutBufferSize, &numLogsDropped);

  // Should not be able to read this log out because there should be no log in
  // the first place
  EXPECT_EQ(bytesCopied, 0);
}

TEST(LogBuffer, LogOverwritten) {
  char buffer[kDefaultBufferSize];
  constexpr size_t kOutBufferSize = 200;
  char outBuffer[kOutBufferSize];
  char testedBuffer[kOutBufferSize];
  TestLogBufferCallback callback;
  LogBuffer logBuffer(&callback, buffer, kDefaultBufferSize);

  // This for loop adds 1060 bytes of data through the buffer which is > than
  // 1024
  for (size_t i = 0; i < 10; i++) {
    std::string testLogStrStr(100, 'a' + i);
    const char *testLogStr = testLogStrStr.c_str();
    logBuffer.handleLog(LogBufferLogLevel::INFO, 0, testLogStr);
  }
  size_t numLogsDropped;
  size_t bytesCopied =
      logBuffer.copyLogs(outBuffer, kOutBufferSize, &numLogsDropped);
  memcpy(testedBuffer, outBuffer + kBytesBeforeLogData, 101);

  // Should have read out the second from front test log string which is 'a' + 1
  // = 'b'
  EXPECT_TRUE(strcmp(testedBuffer, std::string(100, 'b').c_str()) == 0);
  EXPECT_EQ(bytesCopied, kBytesBeforeLogData + 100 + 1);
  // Should have dropped the first log
  EXPECT_EQ(numLogsDropped, 1);
}

TEST(LogBuffer, CopyIntoEmptyBuffer) {
  char buffer[kDefaultBufferSize];
  constexpr size_t kOutBufferSize = 0;
  char outBuffer[kOutBufferSize];
  TestLogBufferCallback callback;
  LogBuffer logBuffer(&callback, buffer, kDefaultBufferSize);

  logBuffer.handleLog(LogBufferLogLevel::INFO, 0, "test");
  size_t numLogsDropped;
  size_t bytesCopied =
      logBuffer.copyLogs(outBuffer, kOutBufferSize, &numLogsDropped);

  EXPECT_EQ(bytesCopied, 0);
}

TEST(LogBuffer, NoCopyInfoBufferAfterHandleEmptyLog) {
  char buffer[kDefaultBufferSize];
  constexpr size_t kOutBufferSize = 200;
  char outBuffer[kOutBufferSize];
  TestLogBufferCallback callback;
  LogBuffer logBuffer(&callback, buffer, kDefaultBufferSize);

  logBuffer.handleLog(LogBufferLogLevel::INFO, 0, "");
  size_t numLogsDropped;
  size_t bytesCopied =
      logBuffer.copyLogs(outBuffer, kOutBufferSize, &numLogsDropped);

  EXPECT_EQ(bytesCopied, 0);
}

TEST(LogBuffer, HandleLogOfNullBytes) {
  char buffer[kDefaultBufferSize];
  constexpr size_t kOutBufferSize = 200;
  char outBuffer[kOutBufferSize];
  TestLogBufferCallback callback;
  LogBuffer logBuffer(&callback, buffer, kDefaultBufferSize);

  logBuffer.handleLog(LogBufferLogLevel::INFO, 0, "\0\0\0");
  size_t numLogsDropped;
  size_t bytesCopied =
      logBuffer.copyLogs(outBuffer, kOutBufferSize, &numLogsDropped);

  EXPECT_EQ(bytesCopied, 0);
}

TEST(LogBuffer, TruncateLongLog) {
  char buffer[kDefaultBufferSize];
  constexpr size_t kOutBufferSize = 500;
  char outBuffer[kOutBufferSize];
  TestLogBufferCallback callback;
  LogBuffer logBuffer(&callback, buffer, kDefaultBufferSize);
  std::string testStr(256, 'a');

  logBuffer.handleLog(LogBufferLogLevel::INFO, 0, testStr.c_str());
  size_t numLogsDropped;
  size_t bytesCopied =
      logBuffer.copyLogs(outBuffer, kOutBufferSize, &numLogsDropped);

  // Should truncate the logs down to the kLogMaxSize value of 255 by the time
  // it is copied out.
  EXPECT_EQ(bytesCopied, 255);
}

TEST(LogBuffer, WouldCauseOverflowTest) {
  char buffer[kDefaultBufferSize];
  TestLogBufferCallback callback;
  LogBuffer logBuffer(&callback, buffer, kDefaultBufferSize);

  // With an empty buffer inerting one character should not overflow
  // ASSERT because if this fails the next ASSERT statement is undefined most
  // likely.
  ASSERT_FALSE(logBuffer.logWouldCauseOverflow(1));

  // This for loop adds 1000 bytes of data. There is 24 bytes of space left in
  // the buffer after this loop.
  for (size_t i = 0; i < 10; i++) {
    std::string testLogStrStr(94, 'a');
    const char *testLogStr = testLogStrStr.c_str();
    logBuffer.handleLog(LogBufferLogLevel::INFO, 0, testLogStr);
  }

  std::string testLogStrStr(11, 'a');
  const char *testLogStr = testLogStrStr.c_str();
  // After this log entry there is room enough for a log of character size 1.
  logBuffer.handleLog(LogBufferLogLevel::INFO, 0, testLogStr);

  // There should be just enough space for this log
  ASSERT_FALSE(logBuffer.logWouldCauseOverflow(1));

  // Inserting any more than a one char log should cause overflow
  ASSERT_TRUE(logBuffer.logWouldCauseOverflow(2));
}

TEST(LogBuffer, TransferTest) {
  char buffer[kDefaultBufferSize];
  const size_t kOutBufferSize = 10;
  char outBuffer[kOutBufferSize];
  size_t numLogsDropped;
  TestLogBufferCallback callback;
  LogBuffer logBufferFrom(&callback, buffer, kDefaultBufferSize);
  LogBuffer logBufferTo(&callback, buffer, kDefaultBufferSize);

  const char *str1 = "str1";
  const char *str2 = "str2";
  const char *str3 = "str3";

  logBufferFrom.handleLog(LogBufferLogLevel::INFO, 0, str1);
  logBufferFrom.handleLog(LogBufferLogLevel::INFO, 0, str2);
  logBufferFrom.handleLog(LogBufferLogLevel::INFO, 0, str3);

  logBufferFrom.transferTo(logBufferTo);

  // The logs should have the text of each of the logs pushed onto the From
  // buffer in FIFO ordering.
  logBufferTo.copyLogs(outBuffer, kOutBufferSize, &numLogsDropped);
  ASSERT_TRUE(strcmp(outBuffer + kBytesBeforeLogData, str1) == 0);
  logBufferTo.copyLogs(outBuffer, kOutBufferSize, &numLogsDropped);
  ASSERT_TRUE(strcmp(outBuffer + kBytesBeforeLogData, str2) == 0);
  logBufferTo.copyLogs(outBuffer, kOutBufferSize, &numLogsDropped);
  ASSERT_TRUE(strcmp(outBuffer + kBytesBeforeLogData, str3) == 0);

  size_t bytesCopied =
      logBufferTo.copyLogs(outBuffer, kOutBufferSize, &numLogsDropped);
  // There should have been no logs left in the To buffer for that last copyLogs
  ASSERT_EQ(bytesCopied, 0);
}

// TODO(srok): Add multithreaded tests

}  // namespace chre
