/*
 * Copyright (C) 2021 The Android Open Source Project
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

#ifndef HARDWARE_GOOGLE_PIXEL_USB_USBOVERHEATEVENT_H
#define HARDWARE_GOOGLE_PIXEL_USB_USBOVERHEATEVENT_H

#include <android-base/file.h>
#include <android-base/properties.h>
#include <android-base/unique_fd.h>
#include <android/hardware/thermal/2.0/IThermal.h>
#include <android/hardware/thermal/2.0/IThermalChangedCallback.h>
#include <android/hardware/thermal/2.0/types.h>
#include <android/hidl/manager/1.0/IServiceManager.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <unistd.h>
#include <utils/Log.h>

#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace android {
namespace hardware {
namespace google {
namespace pixel {
namespace usb {

using ::android::base::unique_fd;
using ::android::base::WriteStringToFile;

using ::android::hardware::thermal::V1_0::ThermalStatus;
using ::android::hardware::thermal::V1_0::ThermalStatusCode;
using ::android::hardware::thermal::V2_0::IThermal;
using ::android::hardware::thermal::V2_0::IThermalChangedCallback;
using ::android::hardware::thermal::V2_0::Temperature;
using ::android::hardware::thermal::V2_0::TemperatureType;
using ::android::hardware::thermal::V2_0::ThrottlingSeverity;

using ::android::hidl::manager::V1_0::IServiceManager;
using ::android::hidl::manager::V1_0::IServiceNotification;

using ::std::lock_guard;
using ::std::max;
using ::std::move;
using ::std::mutex;
using ::std::string;
using ::std::thread;
using ::std::unique_ptr;
using ::std::vector;

class ZoneInfo {
  public:
    // Type of the thermal sensor
    TemperatureType type_;
    // Name of the thermal sensor
    string name_;
    // Throttling severity when monitor_ is started for polling temperature
    ThrottlingSeverity severity_;
    ZoneInfo(const TemperatureType &type, const string &name, const ThrottlingSeverity &severity);
};

class UsbOverheatEvent : public IServiceNotification, public IThermalChangedCallback {
  private:
    // To wake up thread to record max temperature
    unique_fd timer_fd_;
    // Pools on timer_fd_
    unique_fd epoll_fd_;
    // To wake up the thread when waiting on TimerFd
    unique_fd event_fd_;
    // Thermal zone for monitoring Throttling event
    ZoneInfo monitored_zone_;
    // Info of thermal zones that are queried during polling.
    // ATM Suez UsbPortOverheatEvent can only report one of the values though.
    // Therefore the first one in the list will be used for
    // getCurrentTemperature and max_overheat_temp_.
    vector<ZoneInfo> queried_zones_;
    //  Sampling interval for monitoring the temperature
    int monitor_interval_sec_;
    // protects the CV.
    std::mutex lock_;
    // Thread waits here when mRecordMaxTemp is false
    std::condition_variable cv_;
    // Thread object that executes the ep monitoring logic
    unique_ptr<thread> monitor_;
    // Maximum overheat temperature recorded
    float max_overheat_temp_;
    // Reference to thermal service
    ::android::sp<IThermal> thermal_service_;
    // Thread that polls temperature to record max temp
    static void *monitorThread(void *param);
    // Register service notification listener
    bool registerListener();
    // Thermal ServiceNotification listener
    Return<void> onRegistration(const hidl_string & /*fully_qualified_name*/,
                                const hidl_string & /*instance_name*/,
                                bool /*pre_existing*/) override;
    // Thermal service callback
    Return<void> notifyThrottling(const Temperature &temperature) override;

  public:
    UsbOverheatEvent(const ZoneInfo &monitored_zone, const std::vector<ZoneInfo> &queried_zones,
                     const int &monitor_interval_sec);
    // Start monitoring thermal zone for maximum temperature
    bool startRecording();
    // Stop monitoring thermal zone
    bool stopRecording();
    // Enquire current USB temperature
    bool getCurrentTemperature(const string &name, float *temp);
    // Query Max overheat temperature
    float getMaxOverheatTemperature();
};

}  // namespace usb
}  // namespace pixel
}  // namespace google
}  // namespace hardware
}  // namespace android
#endif
