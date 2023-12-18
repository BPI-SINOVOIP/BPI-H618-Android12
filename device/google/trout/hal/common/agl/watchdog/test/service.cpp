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

#include "watchdog.h"

#include <chrono>
#include <thread>

class MyWatchdog : public android::automotive::agl::utils::SystemdWatchdog {
  protected:
    bool IsHealthy() override {
        ++mCounter;
        const bool ok = (mCounter <= 3);
        fprintf(stderr, "watchdog health: %d %s\n", mCounter, (ok ? "true" : "false"));
        return ok;
    }

  private:
    int mCounter = 0;
};

int main() {
    MyWatchdog wd;

    std::thread t1([]() {
        while (true) std::this_thread::sleep_for(std::chrono::hours(1));
    });

    t1.join();
    return 0;
}
