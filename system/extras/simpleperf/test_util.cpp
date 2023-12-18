//
// Copyright (C) 2020 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//

#include <stdio.h>

#include <android-base/logging.h>
#include <android-base/properties.h>

#include "event_attr.h"
#include "event_fd.h"
#include "event_type.h"
#include "test_util.h"

bool IsInNativeAbi() {
  static int in_native_abi = -1;
  if (in_native_abi == -1) {
    FILE* fp = popen("uname -m", "re");
    char buf[40];
    memset(buf, '\0', sizeof(buf));
    CHECK_EQ(fgets(buf, sizeof(buf), fp), buf);
    pclose(fp);
    std::string s = buf;
    in_native_abi = 1;
    if (GetBuildArch() == ARCH_X86_32 || GetBuildArch() == ARCH_X86_64) {
      if (s.find("86") == std::string::npos) {
        in_native_abi = 0;
      }
    } else if (GetBuildArch() == ARCH_ARM || GetBuildArch() == ARCH_ARM64) {
      if (s.find("arm") == std::string::npos && s.find("aarch64") == std::string::npos) {
        in_native_abi = 0;
      }
    }
  }
  return in_native_abi == 1;
}

#if defined(__arm__)
// Check if we can get a non-zero instruction event count by monitoring current thread.
static bool HasNonZeroInstructionEventCount() {
  const simpleperf::EventType* type = simpleperf::FindEventTypeByName("instructions", false);
  if (type == nullptr) {
    return false;
  }
  perf_event_attr attr = CreateDefaultPerfEventAttr(*type);
  std::unique_ptr<EventFd> event_fd =
      EventFd::OpenEventFile(attr, gettid(), -1, nullptr, type->name, false);
  if (!event_fd) {
    return false;
  }
  // do some cpu work.
  for (volatile int i = 0; i < 100000; ++i) {
  }
  PerfCounter counter;
  if (event_fd->ReadCounter(&counter)) {
    return counter.value != 0;
  }
  return false;
}
#endif  // defined(__arm__)

bool HasHardwareCounter() {
#if defined(__linux__)
  static int has_hw_counter = -1;
  if (has_hw_counter == -1) {
    has_hw_counter = 1;
#if defined(__x86__) || defined(__x86_64__)
    // On x86 and x86_64, it's likely to run on an emulator or vm without hardware perf counters.
    // It's hard to enumerate them all. So check the support at runtime.
    const simpleperf::EventType* type = simpleperf::FindEventTypeByName("cpu-cycles", false);
    CHECK(type != nullptr);
    perf_event_attr attr = CreateDefaultPerfEventAttr(*type);
    has_hw_counter = IsEventAttrSupported(attr, "cpu-cycles") ? 1 : 0;
#elif defined(__arm__)
    // For arm32 devices, external non-invasive debug signal controls PMU counters. Once it is
    // disabled for security reason, we always get zero values for PMU counters. And we want to
    // skip hardware counter tests once we detect it.
    has_hw_counter &= HasNonZeroInstructionEventCount() ? 1 : 0;
#endif  // defined(__arm__)
  }
  return has_hw_counter == 1;
#else   // defined(__linux__)
  return false;
#endif  // defined(__linux__)
}

bool HasPmuCounter() {
  static int has_pmu_counter = -1;
  if (has_pmu_counter == -1) {
    has_pmu_counter = 0;
    auto callback = [&](const simpleperf::EventType& event_type) {
      if (event_type.IsPmuEvent()) {
        has_pmu_counter = 1;
        return false;
      }
      return true;
    };
    simpleperf::EventTypeManager::Instance().ForEachType(callback);
  }
  return has_pmu_counter == 1;
}

bool HasTracepointEvents() {
  static int has_tracepoint_events = -1;
  if (has_tracepoint_events == -1) {
    has_tracepoint_events = (GetTraceFsDir() != nullptr) ? 1 : 0;
  }
  return has_tracepoint_events == 1;
}
