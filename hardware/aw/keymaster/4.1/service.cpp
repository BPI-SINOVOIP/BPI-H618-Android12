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

#define LOG_TAG "android.hardware.keymaster@4.1-service-aw"

#include <android/hardware/keymaster/4.1/IKeymasterDevice.h>
#include <hidl/HidlTransportSupport.h>
#include "ca/AW_Keymaster41Device.h"
#include <AndroidKeymaster41Device.h>
#include <cutils/properties.h>

using android::hardware::keymaster::V4_0::SecurityLevel;

int main() {
    ::android::hardware::configureRpcThreadpool(1, true /* willJoinThreadpool */);

    int8_t keymasterKlogEnable = 0;
    keymasterKlogEnable = property_get_bool("ro.boot.keymaster_klog_enable", 0);

    aw::hardware::keymaster::AWKeymasterLogger logger("AW_keymaster_4.1");
    logger.enable_kmsg(1);
    logger.log("start keymaster server");

    char secure_os_exist[PROPERTY_VALUE_MAX] = {};
    if (property_get("ro.boot.secure_os_exist", secure_os_exist, "1") &&
        secure_os_exist[0] == '0') {
        logger.log(ANDROID_LOG_WARN, "no optee, use software keymaster");
        auto keymaster = ::keymaster::V4_1::CreateKeymasterDevice(SecurityLevel::SOFTWARE);
        auto status = keymaster->registerAsService();
        if (status != android::OK) {
            logger.log(ANDROID_LOG_ERROR, "register keymaster 4.1 service failed");
        }
    } else {
        auto awKeymaster =
            new aw::hardware::keymaster::V4_1::implementation::AndroidKeymaster41Device_AW();
        awKeymaster->setLogger(&logger);
        int TaSetUp = -1;
        do {
            TaSetUp = awKeymaster->SetUpTa();
            if (TaSetUp != 0) {
                logger.log(ANDROID_LOG_ERROR, "TA set up failed! Wait 1s and retry");
                sleep(1);
            }
        } while (TaSetUp != 0);
        auto status = awKeymaster->registerAsService();
        if (status != android::OK) {
            logger.log(ANDROID_LOG_ERROR, "register keymaster 4.1 service failed");
        }
    }

    logger.enable_kmsg(keymasterKlogEnable);
    android::hardware::joinRpcThreadpool();
    return -1;  // Should never get here.
}
