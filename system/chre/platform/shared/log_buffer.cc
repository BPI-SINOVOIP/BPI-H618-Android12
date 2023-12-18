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

#include "chre/platform/shared/log_buffer.h"
#include "chre/platform/assert.h"
#include "chre/util/lock_guard.h"

#include <cstdarg>
#include <cstdio>

namespace chre {

LogBuffer::LogBuffer(LogBufferCallbackInterface *callback, void *buffer,
                     size_t bufferSize)
    : mBufferData(static_cast<uint8_t *>(buffer)),
      mBufferMaxSize(bufferSize),
      mCallback(callback) {
  CHRE_ASSERT(bufferSize >= kBufferMinSize);
}

void LogBuffer::handleLog(LogBufferLogLevel logLevel, uint32_t timestampMs,
                          const char *logFormat, ...) {
  va_list args;
  va_start(args, logFormat);
  handleLogVa(logLevel, timestampMs, logFormat, args);
  va_end(args);
}

void LogBuffer::handleLogVa(LogBufferLogLevel logLevel, uint32_t timestampMs,
                            const char *logFormat, va_list args) {
  constexpr size_t maxLogLen = kLogMaxSize - kLogDataOffset;
  char tempBuffer[maxLogLen];
  int logLenSigned = vsnprintf(tempBuffer, maxLogLen, logFormat, args);
  if (logLenSigned > 0) {
    size_t logLen = static_cast<size_t>(logLenSigned);
    if (logLen >= maxLogLen) {
      // Leave space for nullptr to be copied on end
      logLen = maxLogLen - 1;
    }
    size_t totalLogSize = kLogDataOffset + logLen;
    {
      LockGuard<Mutex> lockGuard(mLock);
      // Invalidate memory allocated for log at head while the buffer is greater
      // than max size
      while (mBufferDataSize + totalLogSize > mBufferMaxSize) {
        mNumLogsDropped++;
        size_t logSize;
        mBufferDataHeadIndex = getNextLogIndex(mBufferDataHeadIndex, &logSize);
        mBufferDataSize -= logSize;
      }
      // The final log level as parsed by the daemon requires that the log level
      // be incremented.
      uint8_t logLevelAdjusted = static_cast<uint8_t>(logLevel) + 1;
      copyToBuffer(sizeof(logLevelAdjusted), &logLevelAdjusted);
      copyToBuffer(sizeof(timestampMs), &timestampMs);
      copyToBuffer(logLen, tempBuffer);
      copyToBuffer(1, reinterpret_cast<const void *>("\0"));
    }
    if (mCallback != nullptr) {
      switch (mNotificationSetting) {
        case LogBufferNotificationSetting::ALWAYS: {
          mCallback->onLogsReady();
          break;
        }
        case LogBufferNotificationSetting::NEVER: {
          break;
        }
        case LogBufferNotificationSetting::THRESHOLD: {
          if (mBufferDataSize > mNotificationThresholdBytes) {
            mCallback->onLogsReady();
          }
          break;
        }
      }
    }
  }
}

size_t LogBuffer::copyLogs(void *destination, size_t size,
                           size_t *numLogsDropped) {
  LockGuard<Mutex> lock(mLock);
  return copyLogsLocked(destination, size, numLogsDropped);
}

bool LogBuffer::logWouldCauseOverflow(size_t logSize) {
  LockGuard<Mutex> lock(mLock);
  return (mBufferDataSize + logSize + kLogDataOffset + 1 /* nullptr */ >
          mBufferMaxSize);
}

void LogBuffer::transferTo(LogBuffer &buffer) {
  LockGuard<Mutex> lockGuardOther(buffer.mLock);
  size_t numLogsDropped;
  size_t bytesCopied;
  {
    LockGuard<Mutex> lockGuardThis(mLock);
    // The buffer being transferred to should be as big or bigger.
    CHRE_ASSERT(buffer.mBufferMaxSize >= mBufferMaxSize);

    buffer.resetLocked();

    bytesCopied = copyLogsLocked(buffer.mBufferData, buffer.mBufferMaxSize,
                                 &numLogsDropped);

    resetLocked();
  }
  buffer.mBufferDataTailIndex = bytesCopied % buffer.mBufferMaxSize;
  buffer.mBufferDataSize = bytesCopied;
  buffer.mNumLogsDropped = numLogsDropped;
}

void LogBuffer::updateNotificationSetting(LogBufferNotificationSetting setting,
                                          size_t thresholdBytes) {
  LockGuard<Mutex> lock(mLock);

  mNotificationSetting = setting;
  mNotificationThresholdBytes = thresholdBytes;
}

void LogBuffer::reset() {
  LockGuard<Mutex> lock(mLock);
  resetLocked();
}

const uint8_t *LogBuffer::getBufferData() {
  return mBufferData;
}

size_t LogBuffer::getBufferSize() {
  LockGuard<Mutex> lockGuard(mLock);
  return mBufferDataSize;
}

size_t LogBuffer::getNumLogsDropped() {
  LockGuard<Mutex> lockGuard(mLock);
  return mNumLogsDropped;
}

size_t LogBuffer::incrementAndModByBufferMaxSize(size_t originalVal,
                                                 size_t incrementBy) const {
  return (originalVal + incrementBy) % mBufferMaxSize;
}

void LogBuffer::copyToBuffer(size_t size, const void *source) {
  const uint8_t *sourceBytes = static_cast<const uint8_t *>(source);
  if (mBufferDataTailIndex + size > mBufferMaxSize) {
    size_t firstSize = mBufferMaxSize - mBufferDataTailIndex;
    size_t secondSize = size - firstSize;
    memcpy(&mBufferData[mBufferDataTailIndex], sourceBytes, firstSize);
    memcpy(mBufferData, &sourceBytes[firstSize], secondSize);
  } else {
    memcpy(&mBufferData[mBufferDataTailIndex], sourceBytes, size);
  }
  mBufferDataSize += size;
  mBufferDataTailIndex =
      incrementAndModByBufferMaxSize(mBufferDataTailIndex, size);
}

void LogBuffer::copyFromBuffer(size_t size, void *destination) {
  uint8_t *destinationBytes = static_cast<uint8_t *>(destination);
  if (mBufferDataHeadIndex + size > mBufferMaxSize) {
    size_t firstSize = mBufferMaxSize - mBufferDataHeadIndex;
    size_t secondSize = size - firstSize;
    memcpy(destinationBytes, &mBufferData[mBufferDataHeadIndex], firstSize);
    memcpy(&destinationBytes[firstSize], mBufferData, secondSize);
  } else {
    memcpy(destinationBytes, &mBufferData[mBufferDataHeadIndex], size);
  }
  mBufferDataSize -= size;
  mBufferDataHeadIndex =
      incrementAndModByBufferMaxSize(mBufferDataHeadIndex, size);
}

size_t LogBuffer::copyLogsLocked(void *destination, size_t size,
                                 size_t *numLogsDropped) {
  size_t copySize = 0;

  if (size != 0 && destination != nullptr && mBufferDataSize != 0) {
    if (size >= mBufferDataSize) {
      copySize = mBufferDataSize;
    } else {
      size_t logSize;
      // There is guaranteed to be a null terminator within the max log length
      // number of bytes so logStartIndex will not be maxBytes + 1
      size_t logStartIndex = getNextLogIndex(mBufferDataHeadIndex, &logSize);
      while (copySize + logSize <= size &&
             copySize + logSize <= mBufferDataSize) {
        copySize += logSize;
        logStartIndex = getNextLogIndex(logStartIndex, &logSize);
      }
    }
    copyFromBuffer(copySize, destination);
  }

  *numLogsDropped = mNumLogsDropped;

  return copySize;
}

void LogBuffer::resetLocked() {
  mBufferDataHeadIndex = 0;
  mBufferDataTailIndex = 0;
  mBufferDataSize = 0;
  mNumLogsDropped = 0;
}

size_t LogBuffer::getNextLogIndex(size_t startingIndex, size_t *logSize) {
  size_t logDataStartIndex =
      incrementAndModByBufferMaxSize(startingIndex, kLogDataOffset);

  size_t logDataSize = getLogDataLength(logDataStartIndex);
  *logSize = kLogDataOffset + logDataSize;
  return incrementAndModByBufferMaxSize(startingIndex, *logSize);
}

size_t LogBuffer::getLogDataLength(size_t startingIndex) {
  size_t currentIndex = startingIndex;
  constexpr size_t maxBytes = kLogMaxSize - kLogDataOffset;
  size_t numBytes = maxBytes + 1;

  for (size_t i = 0; i < maxBytes; i++) {
    if (mBufferData[currentIndex] == '\0') {
      // +1 to include the null terminator
      numBytes = i + 1;
      break;
    }
    currentIndex = incrementAndModByBufferMaxSize(currentIndex, 1);
  }
  return numBytes;
}

}  // namespace chre
