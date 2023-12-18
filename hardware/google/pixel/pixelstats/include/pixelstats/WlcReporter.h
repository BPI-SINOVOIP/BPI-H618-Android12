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

#ifndef HARDWARE_GOOGLE_PIXEL_PIXELSTATS_WLCREPORTER_H
#define HARDWARE_GOOGLE_PIXEL_PIXELSTATS_WLCREPORTER_H

#include <aidl/android/frameworks/stats/IStats.h>
#include <utils/RefBase.h>

#include <cstdint>

namespace android {
namespace hardware {
namespace google {
namespace pixel {

using aidl::android::frameworks::stats::IStats;

/**
 * A class to upload wireless metrics
 */
class WlcReporter : public RefBase {
  public:
    void checkAndReport(const std::shared_ptr<IStats> &stats_client, const bool online,
                        const char *ptmc_uevent);

    WlcReporter(const char *ptmc_path);

  private:
    struct WlcStatus {
        bool is_charging;
        bool check_charger_vendor_id;
        bool check_charger_vendor_id_scheduled;
        int check_vendor_id_attempts;
        WlcStatus();
    };
    WlcStatus wlc_status_;
    struct ReportRecord {
        char const *name;
        int64_t last_reported_time_in_sec_today;
        int64_t last_reported_time_in_sec;
        int count_today;
        ReportRecord(char const *name_);
    };
    ReportRecord rec_wlc_vendor_{"wlc_vendor"};
    ReportRecord rec_orientation_{"orientation"};

    void checkVendorId(const std::shared_ptr<IStats> &stats_client, const char *ptmc_uevent);
    void scheduleReportVendorId(const std::shared_ptr<IStats> &stats_client);
    void reportOrientation(const std::shared_ptr<IStats> &stats_client);
    void reportVendor(const std::shared_ptr<IStats> &stats_client, const int ptmcId);
    bool reportVendorMayRetry(const std::shared_ptr<IStats> &stats_client, const char *ptmc_uevent);
    // Translate device orientation value from sensor Hal to atom enum value
    int translateDeviceOrientationToAtomValue(int orientation);
    void reportInBackground(const std::shared_ptr<IStats> &stats_client, const char *ptmc_path);
    /*
     * Wait timer for make delay before read ptmc path, return false on error
     * timerfd: fd create by timerfd_create, need create/close by caller
     **/
    bool ptmcWaitTimer(int timerfd);
    /* For some case (ex if wireless charger was connect to a low power PTMC AC
     * adapter), the wireless charger keep restaring (it might casuse will
     * check and update data in a not reasonable rare).
     * return: true, it has not hit upload rare limit
     *         false, it has hit rate litmit, we should drop current
     *                upload atom
     **/
    bool checkRateLimit(int64_t minSecond, int maxCount, ReportRecord *rec);

    // Proto messages are 1-indexed and VendorAtom field numbers start at 2, so
    // store everything in the values array at the index of the field number
    // -2.
    const int kVendorAtomOffset = 2;
    const int kMaxVendorIdAttempts = 5;
    const char *kWirelessChargerPtmcPath;

    int readPtmcId(const char *ptmc_str);
};

}  // namespace pixel
}  // namespace google
}  // namespace hardware
}  // namespace android

#endif  // HARDWARE_GOOGLE_PIXEL_PIXELSTATS_WLCREPORTER_H
