/**
 * Copyright (c) 2020, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "carwatchdogd"

#include "PackageInfoResolver.h"

#include <android-base/strings.h>
#include <android/automotive/watchdog/internal/ApplicationCategoryType.h>
#include <android/automotive/watchdog/internal/ComponentType.h>
#include <android/automotive/watchdog/internal/UidType.h>
#include <cutils/android_filesystem_config.h>

#include <inttypes.h>

#include <iterator>
#include <string_view>

namespace android {
namespace automotive {
namespace watchdog {

using ::android::IBinder;
using ::android::sp;
using ::android::automotive::watchdog::internal::ApplicationCategoryType;
using ::android::automotive::watchdog::internal::ComponentType;
using ::android::automotive::watchdog::internal::PackageInfo;
using ::android::automotive::watchdog::internal::UidType;
using ::android::base::Error;
using ::android::base::Result;
using ::android::base::StartsWith;
using ::android::binder::Status;

using GetpwuidFunction = std::function<struct passwd*(uid_t)>;
using PackageToAppCategoryMap =
        std::unordered_map<std::string,
                           android::automotive::watchdog::internal::ApplicationCategoryType>;

namespace {

constexpr const char* kSharedPackagePrefix = "shared:";

ComponentType getComponentTypeForNativeUid(uid_t uid, std::string_view packageName,
                                           const std::vector<std::string>& vendorPackagePrefixes) {
    for (const auto& prefix : vendorPackagePrefixes) {
        if (StartsWith(packageName, prefix)) {
            return ComponentType::VENDOR;
        }
    }
    if ((uid >= AID_OEM_RESERVED_START && uid <= AID_OEM_RESERVED_END) ||
        (uid >= AID_OEM_RESERVED_2_START && uid <= AID_OEM_RESERVED_2_END) ||
        (uid >= AID_ODM_RESERVED_START && uid <= AID_ODM_RESERVED_END)) {
        return ComponentType::VENDOR;
    }
    /**
     * There are no third party native services. Thus all non-vendor services are considered system
     * services.
     */
    return ComponentType::SYSTEM;
}

Result<PackageInfo> getPackageInfoForNativeUid(
        uid_t uid, const std::vector<std::string>& vendorPackagePrefixes,
        const GetpwuidFunction& getpwuidHandler) {
    PackageInfo packageInfo;
    passwd* usrpwd = getpwuidHandler(uid);
    if (!usrpwd) {
        return Error() << "Failed to fetch package name";
    }
    const char* packageName = usrpwd->pw_name;
    packageInfo.packageIdentifier.name = packageName;
    packageInfo.packageIdentifier.uid = uid;
    packageInfo.uidType = UidType::NATIVE;
    packageInfo.componentType =
            getComponentTypeForNativeUid(uid, packageName, vendorPackagePrefixes);
    packageInfo.appCategoryType = ApplicationCategoryType::OTHERS;
    packageInfo.sharedUidPackages = {};

    return packageInfo;
}

}  // namespace

sp<PackageInfoResolver> PackageInfoResolver::sInstance = nullptr;
GetpwuidFunction PackageInfoResolver::sGetpwuidHandler = &getpwuid;

sp<IPackageInfoResolver> PackageInfoResolver::getInstance() {
    if (sInstance == nullptr) {
        sInstance = new PackageInfoResolver();
    }
    return sInstance;
}

void PackageInfoResolver::terminate() {
    sInstance.clear();
}

Result<void> PackageInfoResolver::initWatchdogServiceHelper(
        const sp<IWatchdogServiceHelper>& watchdogServiceHelper) {
    std::unique_lock writeLock(mRWMutex);
    if (watchdogServiceHelper == nullptr) {
        return Error() << "Must provide a non-null watchdog service helper instance";
    }
    if (mWatchdogServiceHelper != nullptr) {
        return Error() << "Duplicate initialization";
    }
    mWatchdogServiceHelper = watchdogServiceHelper;
    return {};
}

void PackageInfoResolver::setPackageConfigurations(
        const std::unordered_set<std::string>& vendorPackagePrefixes,
        const PackageToAppCategoryMap& packagesToAppCategories) {
    std::unique_lock writeLock(mRWMutex);
    mVendorPackagePrefixes.clear();
    std::copy(vendorPackagePrefixes.begin(), vendorPackagePrefixes.end(),
              std::back_inserter(mVendorPackagePrefixes));
    mPackagesToAppCategories = packagesToAppCategories;
    // Clear the package info cache as the package configurations have changed.
    mUidToPackageInfoMapping.clear();
}

void PackageInfoResolver::updatePackageInfos(const std::vector<uid_t>& uids) {
    std::unique_lock writeLock(mRWMutex);
    std::vector<int32_t> missingUids;
    for (const uid_t uid : uids) {
        if (mUidToPackageInfoMapping.find(uid) != mUidToPackageInfoMapping.end()) {
            continue;
        }
        if (uid >= AID_APP_START) {
            missingUids.emplace_back(static_cast<int32_t>(uid));
            continue;
        }
        auto result = getPackageInfoForNativeUid(uid, mVendorPackagePrefixes,
                                                 PackageInfoResolver::sGetpwuidHandler);
        if (!result.ok()) {
            missingUids.emplace_back(static_cast<int32_t>(uid));
            continue;
        }
        mUidToPackageInfoMapping[uid] = *result;
        if (StartsWith(result->packageIdentifier.name, kSharedPackagePrefix)) {
            // When the UID is shared, poll car watchdog service to fetch the shared packages info.
            missingUids.emplace_back(static_cast<int32_t>(uid));
        }
    }

    /*
     * There is delay between creating package manager instance and initializing watchdog service
     * helper. Thus check the watchdog service helper instance before proceeding further.
     */
    if (missingUids.empty() || mWatchdogServiceHelper == nullptr) {
        return;
    }

    std::vector<PackageInfo> packageInfos;
    Status status =
            mWatchdogServiceHelper->getPackageInfosForUids(missingUids, mVendorPackagePrefixes,
                                                           &packageInfos);
    if (!status.isOk()) {
        ALOGE("Failed to fetch package infos from car watchdog service: %s",
              status.exceptionMessage().c_str());
        return;
    }
    for (auto& packageInfo : packageInfos) {
        const auto& id = packageInfo.packageIdentifier;
        if (id.name.empty()) {
            continue;
        }
        if (const auto it = mPackagesToAppCategories.find(id.name);
            packageInfo.uidType == UidType::APPLICATION && it != mPackagesToAppCategories.end()) {
            packageInfo.appCategoryType = it->second;
        }
        mUidToPackageInfoMapping[id.uid] = packageInfo;
    }
}

std::unordered_map<uid_t, std::string> PackageInfoResolver::getPackageNamesForUids(
        const std::vector<uid_t>& uids) {
    std::unordered_map<uid_t, std::string> uidToPackageNameMapping;
    if (uids.empty()) {
        return uidToPackageNameMapping;
    }
    updatePackageInfos(uids);
    {
        std::shared_lock readLock(mRWMutex);
        for (const auto& uid : uids) {
            if (mUidToPackageInfoMapping.find(uid) != mUidToPackageInfoMapping.end()) {
                uidToPackageNameMapping[uid] =
                        mUidToPackageInfoMapping.at(uid).packageIdentifier.name;
            }
        }
    }
    return uidToPackageNameMapping;
}

std::unordered_map<uid_t, PackageInfo> PackageInfoResolver::getPackageInfosForUids(
        const std::vector<uid_t>& uids) {
    std::unordered_map<uid_t, PackageInfo> uidToPackageInfoMapping;
    if (uids.empty()) {
        return uidToPackageInfoMapping;
    }
    updatePackageInfos(uids);
    {
        std::shared_lock readLock(mRWMutex);
        for (const auto& uid : uids) {
            if (mUidToPackageInfoMapping.find(uid) != mUidToPackageInfoMapping.end()) {
                uidToPackageInfoMapping[uid] = mUidToPackageInfoMapping.at(uid);
            }
        }
    }
    return uidToPackageInfoMapping;
}

}  // namespace watchdog
}  // namespace automotive
}  // namespace android
