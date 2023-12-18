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

#ifndef CPP_WATCHDOG_SERVER_TESTS_OVERUSECONFIGURATIONTESTUTILS_H_
#define CPP_WATCHDOG_SERVER_TESTS_OVERUSECONFIGURATIONTESTUTILS_H_

#include <android/automotive/watchdog/PerStateBytes.h>
#include <android/automotive/watchdog/internal/ApplicationCategoryType.h>
#include <android/automotive/watchdog/internal/ComponentType.h>
#include <android/automotive/watchdog/internal/IoOveruseAlertThreshold.h>
#include <android/automotive/watchdog/internal/IoOveruseConfiguration.h>
#include <android/automotive/watchdog/internal/PackageMetadata.h>
#include <android/automotive/watchdog/internal/PerStateIoOveruseThreshold.h>
#include <android/automotive/watchdog/internal/ResourceOveruseConfiguration.h>
#include <gmock/gmock.h>

#include <string>
#include <vector>

namespace android {
namespace automotive {
namespace watchdog {

android::automotive::watchdog::internal::ResourceOveruseConfiguration
constructResourceOveruseConfig(
        const android::automotive::watchdog::internal::ComponentType type,
        const std::vector<std::string>&& safeToKill,
        const std::vector<std::string>&& vendorPrefixes,
        const std::vector<android::automotive::watchdog::internal::PackageMetadata> packageMetadata,
        const android::automotive::watchdog::internal::IoOveruseConfiguration&
                ioOveruseConfiguration);

android::automotive::watchdog::internal::IoOveruseConfiguration constructIoOveruseConfig(
        android::automotive::watchdog::internal::PerStateIoOveruseThreshold componentLevel,
        const std::vector<android::automotive::watchdog::internal::PerStateIoOveruseThreshold>&
                packageSpecific,
        const std::vector<android::automotive::watchdog::internal::PerStateIoOveruseThreshold>&
                categorySpecific,
        const std::vector<android::automotive::watchdog::internal::IoOveruseAlertThreshold>&
                systemWide);

PerStateBytes toPerStateBytes(const int64_t fgBytes, const int64_t bgBytes,
                              const int64_t garageModeBytes);

android::automotive::watchdog::internal::PerStateIoOveruseThreshold toPerStateIoOveruseThreshold(
        const std::string& name, const PerStateBytes& perStateBytes);

android::automotive::watchdog::internal::PerStateIoOveruseThreshold toPerStateIoOveruseThreshold(
        const std::string& name, const int64_t fgBytes, const int64_t bgBytes,
        const int64_t garageModeBytes);

android::automotive::watchdog::internal::PerStateIoOveruseThreshold toPerStateIoOveruseThreshold(
        const android::automotive::watchdog::internal::ComponentType type,
        const PerStateBytes& perStateBytes);

android::automotive::watchdog::internal::PerStateIoOveruseThreshold toPerStateIoOveruseThreshold(
        const android::automotive::watchdog::internal::ComponentType type, const int64_t fgBytes,
        const int64_t bgBytes, const int64_t garageModeBytes);

android::automotive::watchdog::internal::PackageMetadata toPackageMetadata(
        std::string packageName,
        android::automotive::watchdog::internal::ApplicationCategoryType type);

android::automotive::watchdog::internal::IoOveruseAlertThreshold toIoOveruseAlertThreshold(
        const int64_t durationInSeconds, const int64_t writtenBytesPerSecond);

testing::Matcher<const android::automotive::watchdog::internal::ResourceOveruseConfiguration>
ResourceOveruseConfigurationMatcher(
        const android::automotive::watchdog::internal::ResourceOveruseConfiguration& config);

}  // namespace watchdog
}  // namespace automotive
}  // namespace android

#endif  //  CPP_WATCHDOG_SERVER_TESTS_OVERUSECONFIGURATIONTESTUTILS_H_
