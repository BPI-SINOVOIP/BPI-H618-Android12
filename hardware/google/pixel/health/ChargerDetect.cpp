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

#define LOG_TAG "dChargerDetect"

#include <android-base/file.h>
#include <android-base/parseint.h>
#include <android-base/strings.h>
#include <cutils/klog.h>
#include <dirent.h>
#include <pixelhealth/ChargerDetect.h>
#include <string>

constexpr char kPowerSupplySysfsPath[]{"/sys/class/power_supply/"};
constexpr char kUsbOnlinePath[]{"/sys/class/power_supply/usb/online"};
constexpr char kUsbPowerSupplySysfsPath[]{"/sys/class/power_supply/usb/usb_type"};
constexpr char kTcpmPsyFilter[]{"tcpm"};
using android::BatteryMonitor;

namespace hardware {
namespace google {
namespace pixel {
namespace health {

int ChargerDetect::readFromFile(const std::string& path, std::string* buf) {
    if (android::base::ReadFileToString(path.c_str(), buf)) {
        *buf = android::base::Trim(*buf);
    }
    return buf->length();
}

int ChargerDetect::getIntField(const std::string& path) {
    std::string buf;
    int value = 0;

    if (readFromFile(path, &buf) > 0)
        android::base::ParseInt(buf, &value);

    return value;
}

/*
 * Traverses through /sys/class/power_supply/ to identify TCPM(Type-C/PD) power supply.
 * TCPM power supply's name follows the format "tcpm-source-psy-6-0025" with i2c/i3c bus id
 * and client id(SID) baked in.
 */
void ChargerDetect::populateTcpmPsyName(std::string* tcpmPsyName) {
    std::unique_ptr<DIR, decltype(&closedir)> dir(opendir(kPowerSupplySysfsPath), closedir);
    if (dir == NULL) {
            KLOG_ERROR(LOG_TAG, "Could not open %s\n", kPowerSupplySysfsPath);
    } else {
        struct dirent* entry;

        while ((entry = readdir(dir.get()))) {
            const char* name = entry->d_name;

	    KLOG_INFO(LOG_TAG, "Psy name:%s", name);
            if (strstr(name, kTcpmPsyFilter)) {
                *tcpmPsyName = name;
	    }
        }
    }
}

/*
 * The contents of /sys/class/power_supply/<Power supply name>/usb_type follows the format:
 * Unknown [SDP] CDP DCP
 * with the current selected value encloses within square braces.
 * This functions extracts the current selected value and returns it back to the caller.
 */
int ChargerDetect::getPsyUsbType(const std::string& path, std::string* type) {
    size_t start;
    std::string usbType;
    int ret;

    ret = readFromFile(path, &usbType);
    if (ret <= 0) {
        KLOG_ERROR(LOG_TAG, "Error reading %s: %d\n", path.c_str(), ret);
        return -EINVAL;
    }

    start = usbType.find('[');
    if (start == std::string::npos) {
        KLOG_ERROR(LOG_TAG, "'[' not found in %s: %s\n", path.c_str(), usbType.c_str());
        return -EINVAL;
    }

    *type = usbType.substr(start + 1, usbType.find(']') - start - 1);
    return 0;
}

/*
 * Reads the usb power_supply's usb_type and the tcpm power_supply's usb_type to infer
 * HealthInfo(hardware/interfaces/health/1.0/types.hal) online property.
 */
void ChargerDetect::onlineUpdate(struct android::BatteryProperties *props) {
    std::string tcpmOnlinePath, usbPsyType;
    static std::string tcpmPsyName;
    int ret;

    props->chargerAcOnline = false;
    props->chargerUsbOnline = false;

    if (tcpmPsyName.empty()) {
        populateTcpmPsyName(&tcpmPsyName);
        KLOG_INFO(LOG_TAG, "TcpmPsyName:%s\n", tcpmPsyName.c_str());
    }

    if (!getIntField(kUsbOnlinePath)) {
        return;
    }

    ret = getPsyUsbType(kUsbPowerSupplySysfsPath, &usbPsyType);
    if (!ret) {
        if (usbPsyType == "CDP" || usbPsyType == "DCP") {
            props->chargerAcOnline = true;
            return;
        } else if (usbPsyType == "SDP") {
            props->chargerUsbOnline = true;
            return;
	}
    }

    /* Safe to assume AC charger here if BC1.2 non compliant */
    props->chargerAcOnline = true;

    if (tcpmPsyName.empty()) {
        return;
    }

    ret = getPsyUsbType(std::string(kPowerSupplySysfsPath) + tcpmPsyName + "/usb_type" ,
			&usbPsyType);
    if (ret < 0) {
        return;
    }

    KLOG_INFO(LOG_TAG, "TcpmPsy Usbtype:%s\n", usbPsyType.c_str());

    return;
}

}  // namespace health
}  // namespace pixel
}  // namespace google
}  // namespace hardware
