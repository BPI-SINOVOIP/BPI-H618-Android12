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

#pragma once

#include <atomic>
#include <string>

namespace android::hardware::automotive::vehicle::V2_0::impl {

/**
 *  Listen on a Unix socket for power state updates, and change the power
 *  state marker file accordingly.
 */
class PowerStateListener {
  public:
    // TODO(chenhaosjtuacm): use std::filesystem::path when available
    PowerStateListener(const std::string& socketPath, const std::string& powerStateMarkerFilePath);

    void Listen();

    void Stop();

  private:
    std::atomic<bool> mShuttingDownFlag{false};
    const std::string mSocketPath;
    const std::string mPowerStateMarkerFilePath;
};

}  // namespace android::hardware::automotive::vehicle::V2_0::impl
