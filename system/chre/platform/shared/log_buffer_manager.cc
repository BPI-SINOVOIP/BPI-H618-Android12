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

#include "chre/platform/shared/log_buffer_manager.h"

#include "chre/core/event_loop_manager.h"
#include "chre/util/lock_guard.h"

void chrePlatformLogToBuffer(chreLogLevel chreLogLevel, const char *format,
                             ...) {
  va_list args;
  va_start(args, format);
  if (chre::LogBufferManagerSingleton::isInitialized()) {
    chre::LogBufferManagerSingleton::get()->logVa(chreLogLevel, format, args);
  }
  va_end(args);
}

namespace chre {

void LogBufferManager::onLogsReady() {
  LockGuard<Mutex> lockGuard(mFlushLogsMutex);
  if (!mLogFlushToHostPending) {
    if (EventLoopManagerSingleton::isInitialized() &&
        EventLoopManagerSingleton::get()
            ->getEventLoop()
            .getPowerControlManager()
            .hostIsAwake()) {
      mLogFlushToHostPending = true;
      mSendLogsToHostCondition.notify_one();
    }
  } else {
    mLogsBecameReadyWhileFlushPending = true;
  }
}

void LogBufferManager::flushLogs() {
  onLogsReady();
}

void LogBufferManager::onLogsSentToHost(bool success) {
  LockGuard<Mutex> lockGuard(mFlushLogsMutex);
  onLogsSentToHostLocked(success);
}

void LogBufferManager::startSendLogsToHostLoop() {
  LockGuard<Mutex> lockGuard(mFlushLogsMutex);
  // TODO(b/181871430): Allow this loop to exit for certain platforms
  while (true) {
    while (!mLogFlushToHostPending) {
      mSendLogsToHostCondition.wait(mFlushLogsMutex);
    }
    bool logWasSent = false;
    if (EventLoopManagerSingleton::get()
            ->getEventLoop()
            .getPowerControlManager()
            .hostIsAwake()) {
      auto &hostCommsMgr =
          EventLoopManagerSingleton::get()->getHostCommsManager();
      preSecondaryBufferUse();
      if (mSecondaryLogBuffer.getBufferSize() == 0) {
        // TODO (b/184178045): Transfer logs into the secondary buffer from
        // primary if there is room.
        mPrimaryLogBuffer.transferTo(mSecondaryLogBuffer);
      }
      // If the primary buffer was not flushed to the secondary buffer then set
      // the flag that will cause sendLogsToHost to be run again after
      // onLogsSentToHost has been called and the secondary buffer has been
      // cleared out.
      if (mPrimaryLogBuffer.getBufferSize() > 0) {
        mLogsBecameReadyWhileFlushPending = true;
      }
      if (mSecondaryLogBuffer.getBufferSize() > 0) {
        mNumLogsDroppedTotal += mSecondaryLogBuffer.getNumLogsDropped();
        mFlushLogsMutex.unlock();
        hostCommsMgr.sendLogMessageV2(mSecondaryLogBuffer.getBufferData(),
                                      mSecondaryLogBuffer.getBufferSize(),
                                      mNumLogsDroppedTotal);
        logWasSent = true;
        mFlushLogsMutex.lock();
      }
    }
    if (!logWasSent) {
      onLogsSentToHostLocked(false);
    }
  }
}

void LogBufferManager::log(chreLogLevel logLevel, const char *formatStr, ...) {
  va_list args;
  va_start(args, formatStr);
  logVa(logLevel, formatStr, args);
  va_end(args);
}

void LogBufferManager::logVa(chreLogLevel logLevel, const char *formatStr,
                             va_list args) {
  LogBufferLogLevel logBufLogLevel = chreToLogBufferLogLevel(logLevel);
  uint64_t timeNs = SystemTime::getMonotonicTime().toRawNanoseconds();
  uint32_t timeMs =
      static_cast<uint32_t>(timeNs / kOneMillisecondInNanoseconds);
  // Copy the va_list before getting size from vsnprintf so that the next
  // argument that will be accessed in buffer.handleLogVa is the starting one.
  va_list getSizeArgs;
  va_copy(getSizeArgs, args);
  size_t logSize = vsnprintf(nullptr, 0, formatStr, getSizeArgs);
  va_end(getSizeArgs);
  if (mPrimaryLogBuffer.logWouldCauseOverflow(logSize)) {
    LockGuard<Mutex> lockGuard(mFlushLogsMutex);
    if (!mLogFlushToHostPending) {
      preSecondaryBufferUse();
      mPrimaryLogBuffer.transferTo(mSecondaryLogBuffer);
    }
  }
  mPrimaryLogBuffer.handleLogVa(logBufLogLevel, timeMs, formatStr, args);
}

LogBufferLogLevel LogBufferManager::chreToLogBufferLogLevel(
    chreLogLevel chreLogLevel) {
  switch (chreLogLevel) {
    case CHRE_LOG_ERROR:
      return LogBufferLogLevel::ERROR;
    case CHRE_LOG_WARN:
      return LogBufferLogLevel::WARN;
    case CHRE_LOG_INFO:
      return LogBufferLogLevel::INFO;
    default:  // CHRE_LOG_DEBUG
      return LogBufferLogLevel::DEBUG;
  }
}

void LogBufferManager::onLogsSentToHostLocked(bool success) {
  if (success) {
    mSecondaryLogBuffer.reset();
  }
  // If there is a failure to send a log through do not try to send another
  // one to avoid an infinite loop occurring
  mLogFlushToHostPending = mLogsBecameReadyWhileFlushPending && success;
  mLogsBecameReadyWhileFlushPending = false;
  if (mLogFlushToHostPending) {
    mSendLogsToHostCondition.notify_one();
  }
}

//! Explicitly instantiate the EventLoopManagerSingleton to reduce codesize.
template class Singleton<LogBufferManager>;

}  // namespace chre
