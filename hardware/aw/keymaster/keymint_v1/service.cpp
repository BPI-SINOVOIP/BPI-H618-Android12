/*
**
** Copyright 2016, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#define LOG_TAG "android.hardware.security.keymint-service-aw"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <AndroidKeyMintDevice.h>
#include <AndroidRemotelyProvisionedComponentDevice.h>
#include <AndroidSecureClock.h>
#include <AndroidSharedSecret.h>
#include <cutils/properties.h>
#include <android/log.h>
#include <log/log.h>
#include "ca/include/AW_KeyMintDevice.h"
#include "ca/include/AW_KeyMintOperation.h"
#include "ca/include/AW_RemotelyProvisionedComponentDevice.h"
#include "ca/include/AW_SecureClock.h"
#include "ca/include/AW_SharedSecret.h"

using aidl::android::hardware::security::keymint::AndroidKeyMintDevice;
using aidl::android::hardware::security::keymint::AndroidRemotelyProvisionedComponentDevice;
using aidl::android::hardware::security::keymint::SecurityLevel;
using aidl::android::hardware::security::secureclock::AndroidSecureClock;
using aidl::android::hardware::security::sharedsecret::AndroidSharedSecret;

using aidl::android::hardware::security::keymint::AW_KeyMintDevice;
using aidl::android::hardware::security::keymint::AW_RemotelyProvisionedComponentDevice;
using aidl::android::hardware::security::secureclock::AW_SecureClock;
using aidl::android::hardware::security::sharedsecret::AW_SharedSecret;

using aw::hardware::keymaster::AWTACommunicator;
using aw::hardware::keymaster::AWKeymasterLogger;

template <typename T, class... Args>
std::shared_ptr<T> addService(AWKeymasterLogger *logger, Args&&... args) {
    std::shared_ptr<T> ser = ndk::SharedRefBase::make<T>(std::forward<Args>(args)...);
    auto instanceName = std::string(T::descriptor) + "/default";
	logger->log("%s", std::string("adding keymint service instance: " + instanceName).c_str());
    binder_status_t status =
            AServiceManager_addService(ser->asBinder().get(), instanceName.c_str());
	if(status != STATUS_OK)
		logger->log("%s failed",std::string("adding keymint service instance: " + instanceName).c_str());
    CHECK(status == STATUS_OK);
    return ser;
}

int main() {
    int8_t keymasterKlogEnable = 0;
    keymasterKlogEnable = property_get_bool("ro.boot.keymaster_klog_enable", 0);

    AWKeymasterLogger logger("AW_keymint_1");
    logger.enable_kmsg(1);
    logger.log("starting aw keymint service");

    // Zero threads seems like a useless pool, but below we'll join this thread to it, increasing
    // the pool size to 1.
    ABinderProcess_setThreadPoolMaxThreadCount(0);

    char secure_os_exist[PROPERTY_VALUE_MAX] = {};
    if (property_get("ro.boot.secure_os_exist", secure_os_exist, "1") &&
        secure_os_exist[0] == '0') {
        logger.log(ANDROID_LOG_WARN, "no optee, use software keymint");
        // Add Keymint Service
        std::shared_ptr<AndroidKeyMintDevice> keyMint =
                addService<AndroidKeyMintDevice>(&logger,SecurityLevel::SOFTWARE);
        // Add Secure Clock Service
        addService<AndroidSecureClock>(&logger,keyMint);
        // Add Shared Secret Service
        addService<AndroidSharedSecret>(&logger,keyMint);
        // Add Remotely Provisioned Component Service
        addService<AndroidRemotelyProvisionedComponentDevice>(&logger,keyMint);
    } else {
		AWTACommunicator *ta_forward = new AWTACommunicator(&logger,::keymaster::MessageVersion(::keymaster::KmVersion::KEYMINT_1));
        int TaSetUp = -1;
        do {
            TaSetUp = AW_KeyMintDevice::SetUpTa(ta_forward);
            if (TaSetUp != 0) {
                logger.log(ANDROID_LOG_ERROR, "TA set up failed! Wait 1s and retry");
                sleep(1);
            }
        } while (TaSetUp != 0);
        // Add Keymint Service
        addService<AW_KeyMintDevice>(&logger,ta_forward,&logger);
        // Add Secure Clock Service
        addService<AW_SecureClock>(&logger,ta_forward,&logger);
        // Add Shared Secret Service
        addService<AW_SharedSecret>(&logger,ta_forward,&logger);
        // Add Remotely Provisioned Component Service
        addService<AW_RemotelyProvisionedComponentDevice>(&logger,ta_forward,&logger);
    }

    logger.log("start aw keymint service ok");
    logger.enable_kmsg(keymasterKlogEnable);
    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
