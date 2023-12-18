/*
 * Copyright 2021 The Android Open Source Project
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

#ifndef ANDROID_HWC_FENCEDBUFFER_H
#define ANDROID_HWC_FENCEDBUFFER_H

#include <android-base/unique_fd.h>
#include <cutils/native_handle.h>

namespace android {

class FencedBuffer {
 public:
  FencedBuffer() : mBuffer(nullptr) {}

  void setBuffer(buffer_handle_t buffer) { mBuffer = buffer; }
  void setFence(int fenceFd) {
    mFence = std::make_shared<base::unique_fd>(fenceFd);
  }

  buffer_handle_t getBuffer() const { return mBuffer; }
  int getFence() const { return mFence ? dup(mFence->get()) : -1; }

 private:
  buffer_handle_t mBuffer;
  std::shared_ptr<android::base::unique_fd> mFence;
};

}  // namespace android

#endif