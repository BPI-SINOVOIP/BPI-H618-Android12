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

#define LOG_TAG "pixelstats-wlc"

#include <android-base/file.h>
#include <android-base/strings.h>
#include <hardware/google/pixel/pixelstats/pixelatoms.pb.h>
#include <log/log.h>
#include <pixelstats/OrientationCollector.h>
#include <pixelstats/WlcReporter.h>
#include <sys/timerfd.h>
#include <time.h>
#include <utils/Timers.h>

#include <thread>

/* I set a higher rare limit ti orientation, beacuae user might try to adjust
 * orientation when start charge
 **/
#define GOOGLE_PTMC_ID (0x72)
#define ID_UNKNOWN (0)
#define WLC_VENDOR_REPORT_RATE_LIMIT_MIN_SEC (60 * 60)
#define WLC_VENDOR_REPORT_RATE_LIMIT_MAX_COUNT_PER_DAY (10)
#define ORIENTATION_REPORT_RATE_LIMIT_MIN_SEC (0)
#define ORIENTATION_REPORT_RATE_LIMIT_MAX_COUNT_PER_DAY (10)
#define DAY_SECOND (86400)

namespace android {
namespace hardware {
namespace google {
namespace pixel {

using aidl::android::frameworks::stats::IStats;
using aidl::android::frameworks::stats::VendorAtom;
using aidl::android::frameworks::stats::VendorAtomValue;
using android::base::ReadFileToString;

WlcReporter::WlcStatus::WlcStatus()
    : is_charging(false),
      check_charger_vendor_id(false),
      check_charger_vendor_id_scheduled(false),
      check_vendor_id_attempts(0) {}

WlcReporter::ReportRecord::ReportRecord(char const *name_)
    : name(name_),
      last_reported_time_in_sec_today(0),
      last_reported_time_in_sec(0),
      count_today(0) {}

WlcReporter::WlcReporter(const char *ptmc_path) : kWirelessChargerPtmcPath(ptmc_path) {}
void WlcReporter::checkAndReport(const std::shared_ptr<IStats> &stats_client, bool online,
                                 const char *ptmc_uevent) {
    bool wireless_charging = online;
    bool started_wireless_charging = wireless_charging && !wlc_status_.is_charging;
    wlc_status_.is_charging = wireless_charging;
    ALOGV("reportVendorId is_charging: %s, started_wireless_charging: %s",
          (wireless_charging) ? "true" : "false", (started_wireless_charging) ? "true" : "false");

    if (started_wireless_charging) {
        reportOrientation(stats_client);
        wlc_status_.check_vendor_id_attempts = 0;
        if (checkRateLimit(WLC_VENDOR_REPORT_RATE_LIMIT_MIN_SEC,
                           WLC_VENDOR_REPORT_RATE_LIMIT_MAX_COUNT_PER_DAY, &rec_wlc_vendor_)) {
            wlc_status_.check_charger_vendor_id = true;
            if (kWirelessChargerPtmcPath != nullptr && strlen(kWirelessChargerPtmcPath) != 0) {
                scheduleReportVendorId(stats_client);
            } else {
                ALOGV("ptmc_path not set");
            }
        }
    }
    if (!wireless_charging) {
        wlc_status_.check_charger_vendor_id = false;
    }
    if (wireless_charging) {
        checkVendorId(stats_client, ptmc_uevent);
    }
}

void WlcReporter::checkVendorId(const std::shared_ptr<IStats> &stats_client,
                                const char *ptmc_uevent) {
    if (!ptmc_uevent || !wlc_status_.check_charger_vendor_id) {
        return;
    }
    if (reportVendorMayRetry(stats_client, ptmc_uevent)) {
        wlc_status_.check_charger_vendor_id = false;
    }
}

bool WlcReporter::reportVendorMayRetry(const std::shared_ptr<IStats> &stats_client,
                                       const char *ptmc_uevent) {
    int ptmcId = readPtmcId(ptmc_uevent);
    if (ptmcId == ID_UNKNOWN) {
        if (++(wlc_status_.check_vendor_id_attempts) < kMaxVendorIdAttempts) {
            return false;
        } /* else if ptmc not ready after retry assume ptmc not supported by charger */
    }
    ALOGV("reportVendorId from Uevent");
    reportVendor(stats_client, ptmcId);
    return true;
}
void WlcReporter::reportVendor(const std::shared_ptr<IStats> &stats_client, const int ptmcId) {
    int vendorCharger = (ptmcId == GOOGLE_PTMC_ID)
                                ? PixelAtoms::WirelessChargingStats::VENDOR_GOOGLE
                                : PixelAtoms::WirelessChargingStats::VENDOR_UNKNOWN;
    ALOGV("ptmcId: 0x%x; vendorCharger: %d", ptmcId, vendorCharger);
    VendorAtomValue tmp;
    tmp.set<VendorAtomValue::intValue>(vendorCharger);

    std::vector<VendorAtomValue> values(1);
    values[PixelAtoms::WirelessChargingStats::kChargerVendorFieldNumber - kVendorAtomOffset] = tmp;

    // Send vendor atom to IStats HAL
    VendorAtom event = {.reverseDomainName = PixelAtoms::ReverseDomainNames().pixel(),
                        .atomId = PixelAtoms::Atom::kWirelessChargingStats,
                        .values = std::move(values)};
    const ndk::ScopedAStatus retStat = stats_client->reportVendorAtom(event);
    if (!retStat.isOk()) {
        ALOGE("Unable to report WLC_STATS to Stats service");
    }
    return;
}

int WlcReporter::readPtmcId(const char *ptmc_str) {
    int id;
    if (sscanf(ptmc_str, "%x", &id) != 1) {
        return ID_UNKNOWN;
    }
    return id;
}

void WlcReporter::scheduleReportVendorId(const std::shared_ptr<IStats> &stats_client) {
    if (wlc_status_.check_charger_vendor_id_scheduled) {
        return;
    }

    wlc_status_.check_charger_vendor_id_scheduled = true;
    std::thread taskThread(&WlcReporter::reportInBackground, this, stats_client,
                           kWirelessChargerPtmcPath);
    taskThread.detach();
}

bool WlcReporter::ptmcWaitTimer(int timerfd) {
    const int kDelaytimeBeforeReadPtmcId = 60;
    struct itimerspec period;
    period.it_interval.tv_sec = 0;
    period.it_interval.tv_nsec = 0;
    period.it_value.tv_sec = kDelaytimeBeforeReadPtmcId;
    period.it_value.tv_nsec = 0;

    if (timerfd_settime(timerfd, 0, &period, nullptr)) {
        ALOGE("Unable to set timer - %s", strerror(errno));
        return false;
    }
    int readval;
    do {
        char buf[8];
        errno = 0;
        readval = read(timerfd, buf, sizeof(buf));
    } while (readval < 0 && errno == EINTR);
    if (readval < 0) {
        ALOGE("Timerfd error - %s", strerror(errno));
        return false;
    }
    return true;
}

/*
 * PTMC path use to sore wireless charger vendor id,
 * and it take some time to get ready after wireless chagre start,
 * to prevnt busy wait, I use timer and background thread to check PTMC ID
 **/
void WlcReporter::reportInBackground(const std::shared_ptr<IStats> &stats_client,
                                     const char *ptmc_path) {
    int timerfd = timerfd_create(CLOCK_BOOTTIME, 0);
    if (timerfd < 0) {
        ALOGE("Unable to create timerfd - %s", strerror(errno));
        return;
    }
    if (ptmcWaitTimer(timerfd)) {
        if (!wlc_status_.is_charging) {
            ALOGV("Not charging, skip report vender id");
        } else if (!wlc_status_.check_charger_vendor_id) {
            ALOGV("id reported by uevnt, skip report vender id");
        } else {
            std::string file_contents;
            if (!ReadFileToString(ptmc_path, &file_contents)) {
                ALOGE("ReadFileToString %s fail", ptmc_path);
            } else {
                int ptmcId = readPtmcId(file_contents.c_str());
                ALOGV("reportVendorId from file");
                reportVendor(stats_client, ptmcId);
            }
        }
    }
    wlc_status_.check_charger_vendor_id_scheduled = false;
    close(timerfd);
}

/*  Reference to frameworks/native/libs/ui/include/ui/DisplayInfo.h
 *  translate orientation value from sensor to enum define in
 *  pixelatoms.proto
 */
int WlcReporter::translateDeviceOrientationToAtomValue(int orientation) {
    switch (orientation) {
        case 0:
            return PixelAtoms::DeviceOrientation::ORIENTATION_0;
        case 1:
            return PixelAtoms::DeviceOrientation::ORIENTATION_90;
        case 2:
            return PixelAtoms::DeviceOrientation::ORIENTATION_180;
        case 3:
            return PixelAtoms::DeviceOrientation::ORIENTATION_270;
        default:
            return PixelAtoms::DeviceOrientation::ORIENTATION_UNKNOWN;
    }
}

void WlcReporter::reportOrientation(const std::shared_ptr<IStats> &stats_client) {
    ALOGV("reportOrientation");
    if (!checkRateLimit(ORIENTATION_REPORT_RATE_LIMIT_MIN_SEC,
                        ORIENTATION_REPORT_RATE_LIMIT_MAX_COUNT_PER_DAY, &rec_orientation_)) {
        return;
    }
    sp<OrientationCollector> orientationCollector =
            OrientationCollector::createOrientationCollector();
    if (orientationCollector != nullptr) {
        int orientationFromSensor = -1;
        orientationCollector->pollOrientation(&orientationFromSensor);
        VendorAtomValue tmp;
        tmp.set<VendorAtomValue::intValue>(
                translateDeviceOrientationToAtomValue(orientationFromSensor));

        std::vector<VendorAtomValue> values(1);
        values[PixelAtoms::DeviceOrientation::kOrientationFieldNumber - kVendorAtomOffset] = tmp;

        VendorAtom event = {.reverseDomainName = PixelAtoms::ReverseDomainNames().pixel(),
                            .atomId = PixelAtoms::Atom::kDeviceOrientation,
                            .values = std::move(values)};
        const ndk::ScopedAStatus retStat = stats_client->reportVendorAtom(event);
        if (!retStat.isOk()) {
            ALOGE("Unable to report Orientation to Stats service");
        }
        orientationCollector->disableOrientationSensor();
    }
}

bool WlcReporter::checkRateLimit(int64_t minSecond, int maxCount, ReportRecord *rec) {
    if (rec == nullptr) {
        ALOGE("ReportRecord should not be NULL");
        return false;
    }
    int64_t now = nanoseconds_to_seconds(systemTime(SYSTEM_TIME_BOOTTIME));
    if (rec->last_reported_time_in_sec > 0 && now - rec->last_reported_time_in_sec < minSecond) {
        ALOGV("%s: Rate limit, min period: %ld", rec->name, minSecond);
        return false;
    }
    if (rec->last_reported_time_in_sec_today == 0) {
        rec->last_reported_time_in_sec_today = now;
        ALOGV("%s: reset day time (init)", rec->name);
    } else if (now - rec->last_reported_time_in_sec_today > DAY_SECOND) {
        rec->last_reported_time_in_sec_today = now;
        rec->count_today = 0;
        ALOGV("%s: reset day time", rec->name);
    } else if (rec->count_today >= maxCount) {
        ALOGV("%s: Rate limit, max count: %d", rec->name, maxCount);
        return false;
    }
    (rec->count_today)++;
    ALOGV("%s: checkRateLimit count: %d", rec->name, rec->count_today);
    rec->last_reported_time_in_sec = now;
    return true;
}
}  // namespace pixel
}  // namespace google
}  // namespace hardware
}  // namespace android
