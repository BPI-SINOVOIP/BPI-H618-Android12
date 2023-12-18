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

#include <thread>

namespace android::automotive::agl::utils {
class SystemdWatchdog {
  public:
    SystemdWatchdog();
    virtual ~SystemdWatchdog() = default;

  protected:
    virtual bool IsHealthy() = 0;

  private:
    SystemdWatchdog(const SystemdWatchdog&) = delete;
    SystemdWatchdog& operator=(const SystemdWatchdog&) = delete;

    void WatchdogThread();
    std::thread mThread;
};
}  // namespace android::automotive::agl::utils
