/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <linux/if_ether.h>
#include <net/if.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include <string>

#include "BpfSyscallWrappers.h"

namespace android {
namespace bpf {

constexpr const int OVERFLOW_COUNTERSET = 2;

constexpr const uint64_t NONEXISTENT_COOKIE = 0;

uint64_t getSocketCookie(int sockFd);
int synchronizeKernelRCU();
int setrlimitForTest();

#define KVER(a, b, c) (((a) << 24) + ((b) << 16) + (c))

unsigned kernelVersion();

static inline bool isAtLeastKernelVersion(unsigned major, unsigned minor, unsigned sub) {
    return kernelVersion() >= KVER(major, minor, sub);
}

#define SKIP_IF_BPF_SUPPORTED                                                    \
    do {                                                                         \
        if (android::bpf::isAtLeastKernelVersion(4, 9, 0)) {                     \
            GTEST_LOG_(INFO) << "This test is skipped since bpf is supported\n"; \
            return;                                                              \
        }                                                                        \
    } while (0)

#define SKIP_IF_BPF_NOT_SUPPORTED                                                    \
    do {                                                                             \
        if (!android::bpf::isAtLeastKernelVersion(4, 9, 0)) {                        \
            GTEST_LOG_(INFO) << "This test is skipped since bpf is not supported\n"; \
            return;                                                                  \
        }                                                                            \
    } while (0)

#define SKIP_IF_EXTENDED_BPF_NOT_SUPPORTED                                        \
    do {                                                                          \
        if (!android::bpf::isAtLeastKernelVersion(4, 14, 0)) {                    \
            GTEST_LOG_(INFO) << "This test is skipped since extended bpf feature" \
                             << "not supported\n";                                \
            return;                                                               \
        }                                                                         \
    } while (0)

#define SKIP_IF_XDP_NOT_SUPPORTED                                \
    do {                                                         \
        if (!android::bpf::isAtLeastKernelVersion(5, 9, 0)) {    \
            GTEST_LOG_(INFO) << "This test is skipped since xdp" \
                             << "not supported\n";               \
            return;                                              \
        }                                                        \
    } while (0)

}  // namespace bpf
}  // namespace android
