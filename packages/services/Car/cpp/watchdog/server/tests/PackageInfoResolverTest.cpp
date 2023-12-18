/*
 * Copyright 2020 The Android Open Source Project
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

#include "MockWatchdogServiceHelper.h"
#include "PackageInfoResolver.h"

#include <android-base/stringprintf.h>
#include <android/automotive/watchdog/internal/ApplicationCategoryType.h>
#include <android/automotive/watchdog/internal/ComponentType.h>
#include <android/automotive/watchdog/internal/UidType.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace android {
namespace automotive {
namespace watchdog {

using ::android::automotive::watchdog::internal::ApplicationCategoryType;
using ::android::automotive::watchdog::internal::ComponentType;
using ::android::automotive::watchdog::internal::PackageInfo;
using ::android::automotive::watchdog::internal::UidType;
using ::android::base::StringAppendF;
using ::testing::_;
using ::testing::DoAll;
using ::testing::NotNull;
using ::testing::Pair;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::UnorderedElementsAre;
using ::testing::UnorderedElementsAreArray;

namespace {

using PackageToAppCategoryMap =
        std::unordered_map<std::string,
                           android::automotive::watchdog::internal::ApplicationCategoryType>;

PackageInfo constructPackageInfo(const char* packageName, int32_t uid, UidType uidType,
                                 ComponentType componentType,
                                 ApplicationCategoryType appCategoryType,
                                 std::vector<std::string> sharedUidPackages = {}) {
    PackageInfo packageInfo;
    packageInfo.packageIdentifier.name = packageName;
    packageInfo.packageIdentifier.uid = uid;
    packageInfo.uidType = uidType;
    packageInfo.componentType = componentType;
    packageInfo.appCategoryType = appCategoryType;
    packageInfo.sharedUidPackages = sharedUidPackages;
    return packageInfo;
}

std::string toString(const std::unordered_map<uid_t, PackageInfo>& mappings) {
    std::string buffer = "{";
    for (const auto& [uid, info] : mappings) {
        if (buffer.size() > 1) {
            StringAppendF(&buffer, ", ");
        }
        StringAppendF(&buffer, "{%d: %s}", uid, info.toString().c_str());
    }
    StringAppendF(&buffer, "}");
    return buffer;
}

}  // namespace

namespace internal {

class PackageInfoResolverPeer {
public:
    PackageInfoResolverPeer() {
        PackageInfoResolver::getInstance();
        mPackageInfoResolver = PackageInfoResolver::sInstance;
        mockWatchdogServiceHelper = new MockWatchdogServiceHelper();
        mPackageInfoResolver->initWatchdogServiceHelper(mockWatchdogServiceHelper);
    }

    ~PackageInfoResolverPeer() {
        PackageInfoResolver::sInstance.clear();
        PackageInfoResolver::sGetpwuidHandler = &getpwuid;
        clearMappingCache();
    }

    void injectCacheMapping(const std::unordered_map<uid_t, PackageInfo>& mapping) {
        mPackageInfoResolver->mUidToPackageInfoMapping = mapping;
    }

    void setPackageConfigurations(const std::unordered_set<std::string>& vendorPackagePrefixes,
                                  const PackageToAppCategoryMap& packagesToAppCategories) {
        mPackageInfoResolver->setPackageConfigurations(vendorPackagePrefixes,
                                                       packagesToAppCategories);
    }

    void stubGetpwuid(const std::unordered_map<uid_t, std::string>& nativeUidToPackageNameMapping) {
        updateNativeUidToPackageNameMapping(nativeUidToPackageNameMapping);
        PackageInfoResolver::sGetpwuidHandler = [&](uid_t uid) -> struct passwd* {
            const auto& it = mNativeUidToPackageNameMapping.find(uid);
            if (it == mNativeUidToPackageNameMapping.end()) {
                return nullptr;
            }
            return &it->second;
        };
    }

    sp<MockWatchdogServiceHelper> mockWatchdogServiceHelper;

private:
    void updateNativeUidToPackageNameMapping(
            const std::unordered_map<uid_t, std::string>& mapping) {
        clearMappingCache();
        for (const auto& it : mapping) {
            char* packageName = new char[it.second.size() + 1];
            if (packageName == nullptr) {
                continue;
            }
            memset(packageName, 0, sizeof(packageName));
            snprintf(packageName, it.second.size() + 1, "%s", it.second.c_str());

            struct passwd pwd {
                .pw_name = packageName, .pw_uid = it.first
            };
            mNativeUidToPackageNameMapping.insert(std::make_pair(it.first, pwd));
        }
    }

    void clearMappingCache() {
        for (const auto it : mNativeUidToPackageNameMapping) {
            // Delete the previously allocated char array before clearing the mapping.
            delete it.second.pw_name;
        }
        mNativeUidToPackageNameMapping.clear();
    }

    sp<PackageInfoResolver> mPackageInfoResolver;
    std::unordered_map<uid_t, struct passwd> mNativeUidToPackageNameMapping;
};

}  // namespace internal

TEST(PackageInfoResolverTest, TestGetPackageInfosForUidsViaGetpwuid) {
    internal::PackageInfoResolverPeer peer;
    auto packageInfoResolver = PackageInfoResolver::getInstance();
    PackageToAppCategoryMap packagesToAppCategories = {
            // These mappings should be ignored for native packages.
            {"system.package.B", ApplicationCategoryType::MAPS},
            {"vendor.package.A", ApplicationCategoryType::MEDIA},
            {"vendor.pkg.maps", ApplicationCategoryType::MAPS},
    };
    peer.setPackageConfigurations({"vendor.pkg"}, packagesToAppCategories);

    std::unordered_map<uid_t, PackageInfo> expectedMappings{
            {7700,
             constructPackageInfo("system.package.B", 7700, UidType::NATIVE, ComponentType::SYSTEM,
                                  ApplicationCategoryType::OTHERS)},
            {5100,
             constructPackageInfo("vendor.package.A", 5100, UidType::NATIVE, ComponentType::VENDOR,
                                  ApplicationCategoryType::OTHERS)},
            {6700,
             constructPackageInfo("vendor.package.B", 6700, UidType::NATIVE, ComponentType::VENDOR,
                                  ApplicationCategoryType::OTHERS)},
            {9997,
             constructPackageInfo("vendor.pkg.C", 9997, UidType::NATIVE, ComponentType::VENDOR,
                                  ApplicationCategoryType::OTHERS)},
    };

    peer.stubGetpwuid({{7700, "system.package.B"},
                       {5100, "vendor.package.A"},
                       {6700, "vendor.package.B"},
                       {9997, "vendor.pkg.C"}});
    EXPECT_CALL(*peer.mockWatchdogServiceHelper, getPackageInfosForUids(_, _, _)).Times(0);

    auto actualMappings = packageInfoResolver->getPackageInfosForUids({7700, 5100, 6700, 9997});

    EXPECT_THAT(actualMappings, UnorderedElementsAreArray(expectedMappings))
            << "Expected: " << toString(expectedMappings)
            << "\nActual: " << toString(actualMappings);
}

TEST(PackageInfoResolverTest, TestGetPackageInfosForUidsViaWatchdogService) {
    internal::PackageInfoResolverPeer peer;
    auto packageInfoResolver = PackageInfoResolver::getInstance();
    PackageToAppCategoryMap packagesToAppCategories = {
            // system.package.B is native package so this should be ignored.
            {"system.package.B", ApplicationCategoryType::MAPS},
            {"vendor.package.A", ApplicationCategoryType::MEDIA},
    };
    peer.setPackageConfigurations({"vendor.pkg"}, packagesToAppCategories);
    /*
     * Shared UID should be resolved with car watchdog service as well to get the shared packages
     * list.
     */
    peer.stubGetpwuid({{6100, "shared:system.package.A"}});

    std::unordered_map<uid_t, PackageInfo> expectedMappings{
            {6100,
             constructPackageInfo("shared:system.package.A", 6100, UidType::NATIVE,
                                  ComponentType::SYSTEM, ApplicationCategoryType::OTHERS,
                                  {"system.pkg.1", "system.pkg.2"})},
            {7700,
             constructPackageInfo("system.package.B", 7700, UidType::NATIVE, ComponentType::SYSTEM,
                                  ApplicationCategoryType::OTHERS)},
            {15100,
             constructPackageInfo("vendor.package.A", 15100, UidType::APPLICATION,
                                  ComponentType::VENDOR, ApplicationCategoryType::MEDIA)},
            {16700,
             constructPackageInfo("vendor.pkg", 16700, UidType::NATIVE, ComponentType::VENDOR,
                                  ApplicationCategoryType::OTHERS)},
    };

    std::vector<int32_t> expectedUids = {6100, 7700, 15100, 16700};
    std::vector<std::string> expectedPrefixes = {"vendor.pkg"};
    std::vector<PackageInfo> injectPackageInfos = {expectedMappings.at(6100),
                                                   expectedMappings.at(7700),
                                                   expectedMappings.at(15100),
                                                   expectedMappings.at(16700)};
    EXPECT_CALL(*peer.mockWatchdogServiceHelper,
                getPackageInfosForUids(expectedUids, expectedPrefixes, _))
            .WillOnce(DoAll(SetArgPointee<2>(injectPackageInfos), Return(binder::Status::ok())));

    auto actualMappings = packageInfoResolver->getPackageInfosForUids({6100, 7700, 15100, 16700});

    EXPECT_THAT(actualMappings, UnorderedElementsAreArray(expectedMappings))
            << "Expected: " << toString(expectedMappings)
            << "\nActual: " << toString(actualMappings);
}

TEST(PackageInfoResolverTest, TestResolvesApplicationUidFromLocalCache) {
    internal::PackageInfoResolverPeer peer;
    auto packageInfoResolver = PackageInfoResolver::getInstance();
    std::unordered_map<uid_t, PackageInfo> expectedMappings{
            {1003456,
             constructPackageInfo("vendor.package", 1003456, UidType::NATIVE, ComponentType::SYSTEM,
                                  ApplicationCategoryType::OTHERS)}};
    peer.injectCacheMapping(expectedMappings);

    peer.stubGetpwuid({});
    EXPECT_CALL(*peer.mockWatchdogServiceHelper, getPackageInfosForUids(_, _, _)).Times(0);

    auto actualMappings = packageInfoResolver->getPackageInfosForUids({1003456});

    EXPECT_THAT(actualMappings, UnorderedElementsAreArray(expectedMappings))
            << "Expected: " << toString(expectedMappings)
            << "\nActual: " << toString(actualMappings);
}

}  // namespace watchdog
}  // namespace automotive
}  // namespace android
