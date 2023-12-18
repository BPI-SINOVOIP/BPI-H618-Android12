/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef CHRE_PLATFORM_ASSERT_H_
#define CHRE_PLATFORM_ASSERT_H_

#include "chre/platform/log.h"

/**
 * @file
 * Defines the CHRE_ASSERT and CHRE_ASSERT_LOG macros for CHRE platforms.
 * Platforms must supply an implementation for assertCondition or use the shared
 * implementation.
 */

#if defined(CHRE_ASSERTIONS_ENABLED)

#define CHRE_ASSERT(condition)                 \
  do {                                         \
    if (!(condition)) {                        \
      chre::doAssert(CHRE_FILENAME, __LINE__); \
    }                                          \
  } while (0)

#elif defined(CHRE_ASSERTIONS_DISABLED)

#define CHRE_ASSERT(condition) ((void)(condition))

#else
#error "CHRE_ASSERTIONS_ENABLED or CHRE_ASSERTIONS_DISABLED must be defined"
#endif  // CHRE_ASSERTIONS_ENABLED

#ifdef __cplusplus
#define CHRE_ASSERT_NOT_NULL(ptr) CHRE_ASSERT((ptr) != nullptr)
#else
#define CHRE_ASSERT_NOT_NULL(ptr) CHRE_ASSERT((ptr) != NULL)
#endif

/**
 * Combination macro that always logs an error message if the condition
 * evaluates to false.
 *
 * Note that the supplied condition may be evaluated more than once.
 *
 * @param condition Boolean expression which evaluates to false in the failure
 *        case
 * @param fmt Format string to pass to LOGE
 * @param ... Arguments to pass to LOGE
 */
#define CHRE_ASSERT_LOG(condition, fmt, ...) \
  do {                                       \
    if (!(condition)) {                      \
      LOGE("Assert: " fmt, ##__VA_ARGS__);   \
      CHRE_ASSERT(condition);                \
    }                                        \
  } while (0)

/**
 * Defines "if not test" macros that allow code to not assert when running
 * on-device unit tests if the assertion isn't useful during testing.
 */
#ifdef CHRE_ON_DEVICE_TESTS_ENABLED
#define CHRE_ASSERT_LOG_IF_NOT_TEST(condition, fmt, ...)
#define CHRE_ASSERT_IF_NOT_TEST(condition) ((void)(condition))
#else
#define CHRE_ASSERT_LOG_IF_NOT_TEST(condition, fmt, ...) \
  CHRE_ASSERT_LOG(condition, fmt, ##__VA_ARGS__)
#define CHRE_ASSERT_IF_NOT_TEST(condition) CHRE_ASSERT(condition)
#endif

namespace chre {

/**
 * Performs assertion while logging the filename and line provided.
 *
 * @param filename The filename containing the assertion being fired.
 * @param line The line that contains the assertion being fired.
 */
void doAssert(const char *filename, size_t line);

}  // namespace chre

#endif  // CHRE_PLATFORM_ASSERT_H_
