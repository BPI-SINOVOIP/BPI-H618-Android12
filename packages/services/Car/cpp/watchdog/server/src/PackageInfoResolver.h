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

#ifndef CPP_WATCHDOG_SERVER_SRC_PACKAGEINFORESOLVER_H_
#define CPP_WATCHDOG_SERVER_SRC_PACKAGEINFORESOLVER_H_

#include "WatchdogServiceHelper.h"

#include <android-base/result.h>
#include <android/automotive/watchdog/internal/ApplicationCategoryType.h>
#include <android/automotive/watchdog/internal/PackageInfo.h>
#include <binder/IBinder.h>
#include <gtest/gtest_prod.h>
#include <utils/Mutex.h>
#include <utils/RefBase.h>
#include <utils/StrongPointer.h>

#include <pwd.h>

#include <functional>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>

namespace android {
namespace automotive {
namespace watchdog {

class ServiceManager;
class IoOveruseMonitor;
class IoOveruseConfigs;

// Forward declaration for testing use only.
namespace internal {

class PackageInfoResolverPeer;

}  // namespace internal

class IPackageInfoResolver : public android::RefBase {
public:
    virtual std::unordered_map<uid_t, std::string> getPackageNamesForUids(
            const std::vector<uid_t>& uids) = 0;
    virtual std::unordered_map<uid_t, android::automotive::watchdog::internal::PackageInfo>
    getPackageInfosForUids(const std::vector<uid_t>& uids) = 0;

protected:
    virtual android::base::Result<void> initWatchdogServiceHelper(
            const android::sp<IWatchdogServiceHelper>& watchdogServiceHelper) = 0;
    virtual void setPackageConfigurations(
            const std::unordered_set<std::string>& vendorPackagePrefixes,
            const std::unordered_map<
                    std::string, android::automotive::watchdog::internal::ApplicationCategoryType>&
                    packagesToAppCategories) = 0;

private:
    friend class ServiceManager;
    friend class IoOveruseMonitor;
    friend class IoOveruseConfigs;
};

/*
 * PackageInfoResolver maintains a cache of the UID to PackageInfo mapping in the CarWatchdog
 * daemon. PackageInfoResolver is a singleton and must be accessed only via the public static
 * methods.
 *
 * TODO(b/158131194): Extend IUidObserver in WatchdogBinderMediator and use the onUidGone API to
 *  keep the local mapping cache up-to-date.
 */
class PackageInfoResolver : public IPackageInfoResolver {
public:
    /*
     * Initializes the PackageInfoResolver's singleton instance only on the first call. Main thread
     * should make the first call as this method doesn't offer multi-threading protection.
     */
    static sp<IPackageInfoResolver> getInstance();

    /*
     * Resolves the given |uids| and returns a mapping of uids to package names. If the mapping
     * doesn't exist in the local cache, queries the car watchdog service for application uids and
     * getpwuid for native uids. Logs any error observed during this process.
     */
    std::unordered_map<uid_t, std::string> getPackageNamesForUids(const std::vector<uid_t>& uids);

    /*
     * Similar to getPackageNamesForUids, resolves the given |uids| and returns a mapping of uids to
     * package infos.
     */
    std::unordered_map<uid_t, android::automotive::watchdog::internal::PackageInfo>
    getPackageInfosForUids(const std::vector<uid_t>& uids);

    ~PackageInfoResolver() {
        std::unique_lock writeLock(mRWMutex);
        mWatchdogServiceHelper.clear();
        mUidToPackageInfoMapping.clear();
    }

protected:
    static void terminate();

    android::base::Result<void> initWatchdogServiceHelper(
            const android::sp<IWatchdogServiceHelper>& watchdogServiceHelper);

    virtual void setPackageConfigurations(
            const std::unordered_set<std::string>& vendorPackagePrefixes,
            const std::unordered_map<
                    std::string, android::automotive::watchdog::internal::ApplicationCategoryType>&
                    packagesToAppCategories);

private:
    // PackageInfoResolver instance can only be obtained via |getInstance|.
    PackageInfoResolver() :
          mWatchdogServiceHelper(nullptr),
          mUidToPackageInfoMapping({}),
          mVendorPackagePrefixes({}) {}

    void updatePackageInfos(const std::vector<uid_t>& uids);

    // Singleton instance.
    static android::sp<PackageInfoResolver> sInstance;

    mutable std::shared_mutex mRWMutex;

    /*
     * ServiceManager::startServices initializes PackageInfoResolver. However, between the
     * |getInstance| and |initWatchdogServiceHelper| calls it initializes few other services, which
     * may call |getPackageNamesForUids| or |getPackageInfosForUids| simultaneously on a separate
     * thread. In order to avoid a race condition between |initWatchdogServiceHelper| and
     * |getPackage*ForUids| calls, mWatchdogServiceHelper is guarded by a read-write lock.
     */
    android::sp<IWatchdogServiceHelper> mWatchdogServiceHelper GUARDED_BY(mRWMutex);
    std::unordered_map<uid_t, android::automotive::watchdog::internal::PackageInfo>
            mUidToPackageInfoMapping GUARDED_BY(mRWMutex);
    std::vector<std::string> mVendorPackagePrefixes GUARDED_BY(mRWMutex);
    std::unordered_map<std::string,
                       android::automotive::watchdog::internal::ApplicationCategoryType>
            mPackagesToAppCategories GUARDED_BY(mRWMutex);

    friend class ServiceManager;
    friend class IoOveruseMonitor;
    friend class IoOveruseConfigs;

    // For unit tests.
    static std::function<struct passwd*(uid_t)> sGetpwuidHandler;

    friend class internal::PackageInfoResolverPeer;
    FRIEND_TEST(PackageInfoResolverTest, TestResolvesNativeUid);
    FRIEND_TEST(PackageInfoResolverTest, TestResolvesApplicationUidFromWatchdogServiceHelper);
    FRIEND_TEST(PackageInfoResolverTest, TestResolvesApplicationUidFromLocalCache);
};

}  // namespace watchdog
}  // namespace automotive
}  // namespace android

#endif  //  CPP_WATCHDOG_SERVER_SRC_PACKAGEINFORESOLVER_H_
