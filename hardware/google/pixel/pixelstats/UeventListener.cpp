/*
 * Copyright (C) 2017 The Android Open Source Project
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

/* If you are watching for a new uevent, uncomment the following define.
 * After flashing your test build, run:
 *    adb root && adb shell
 *    stop vendor.pixelstats_vendor
 *    touch /data/local/tmp/uevents
 *    /vendor/bin/pixelstats-vendor &
 *
 *    then trigger any events.
 *    If you leave adb connected, you can watch them with
 *    tail -f /data/local/tmp/uevents
 *
 *    Once you are done,
 *
 *    adb pull /data/local/tmp/uevents
 *    adb rm /data/local/tmp/uevents
 *    adb reboot
 *
 *    provide this log in the bug as support for your feature.
 */
// #define LOG_UEVENTS_TO_FILE_ONLY_FOR_DEVEL "/data/local/tmp/uevents"

#define LOG_TAG "pixelstats-uevent"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/parseint.h>
#include <android-base/strings.h>
#include <android/binder_manager.h>
#include <cutils/uevent.h>
#include <fcntl.h>
#include <hardware/google/pixel/pixelstats/pixelatoms.pb.h>
#include <log/log.h>
#include <pixelstats/StatsHelper.h>
#include <pixelstats/UeventListener.h>
#include <pixelstats/WlcReporter.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utils/StrongPointer.h>

#include <string>
#include <thread>

namespace android {
namespace hardware {
namespace google {
namespace pixel {

using aidl::android::frameworks::stats::VendorAtom;
using aidl::android::frameworks::stats::VendorAtomValue;
using android::sp;
using android::base::ReadFileToString;
using android::base::WriteStringToFile;
using android::hardware::google::pixel::WlcReporter;
using android::hardware::google::pixel::PixelAtoms::ChargeStats;
using android::hardware::google::pixel::PixelAtoms::PdVidPid;
using android::hardware::google::pixel::PixelAtoms::VendorHardwareFailed;
using android::hardware::google::pixel::PixelAtoms::VendorUsbPortOverheat;
using android::hardware::google::pixel::PixelAtoms::VoltageTierStats;

constexpr int32_t UEVENT_MSG_LEN = 2048;  // it's 2048 in all other users.
constexpr int32_t PRODUCT_TYPE_OFFSET = 23;
constexpr int32_t PRODUCT_TYPE_MASK = 7;
constexpr int32_t PRODUCT_TYPE_CHARGER = 3;
constexpr int32_t VID_MASK = 0xffff;
constexpr int32_t VID_GOOGLE = 0x18d1;
constexpr int32_t PID_OFFSET = 2;
constexpr int32_t PID_LENGTH = 4;

bool UeventListener::ReadFileToInt(const std::string &path, int *val) {
    return ReadFileToInt(path.c_str(), val);
}

bool UeventListener::ReadFileToInt(const char *const path, int *val) {
    std::string file_contents;

    if (!ReadFileToString(path, &file_contents)) {
        ALOGE("Unable to read %s - %s", path, strerror(errno));
        return false;
    } else if (sscanf(file_contents.c_str(), "%d", val) != 1) {
        ALOGE("Unable to convert %s to int - %s", path, strerror(errno));
        return false;
    }
    return true;
}

void UeventListener::ReportMicBrokenOrDegraded(const std::shared_ptr<IStats> &stats_client,
                                               const int mic, const bool isbroken) {
    VendorHardwareFailed failure;
    failure.set_hardware_type(VendorHardwareFailed::HARDWARE_FAILED_MICROPHONE);
    failure.set_hardware_location(mic);
    failure.set_failure_code(isbroken ? VendorHardwareFailed::COMPLETE
                                      : VendorHardwareFailed::DEGRADE);
    reportHardwareFailed(stats_client, failure);
}

void UeventListener::ReportMicStatusUevents(const std::shared_ptr<IStats> &stats_client,
                                            const char *devpath, const char *mic_status) {
    if (!devpath || !mic_status)
        return;
    if (!strcmp(devpath, ("DEVPATH=" + kAudioUevent).c_str())) {
        std::vector<std::string> value = android::base::Split(mic_status, "=");
        bool isbroken;

        if (value.size() == 2) {
            if (!value[0].compare("MIC_BREAK_STATUS"))
                isbroken = true;
            else if (!value[0].compare("MIC_DEGRADE_STATUS"))
                isbroken = false;
            else
                return;

            if (!value[1].compare("true")) {
                ReportMicBrokenOrDegraded(stats_client, 0, isbroken);
            } else {
                int mic_status = atoi(value[1].c_str());

                if (mic_status > 0 && mic_status <= 7) {
                    for (int mic_bit = 0; mic_bit < 3; mic_bit++)
                        if (mic_status & (0x1 << mic_bit))
                            ReportMicBrokenOrDegraded(stats_client, mic_bit, isbroken);
                } else if (mic_status == 0) {
                    // mic is ok
                    return;
                } else {
                    // should not enter here
                    ALOGE("invalid mic status");
                    return;
                }
            }
        }
    }
}

void UeventListener::ReportUsbPortOverheatEvent(const std::shared_ptr<IStats> &stats_client,
                                                const char *driver) {
    if (!driver || strcmp(driver, "DRIVER=google,overheat_mitigation")) {
        return;
    }

    int32_t plug_temperature_deci_c = 0;
    int32_t max_temperature_deci_c = 0;
    int32_t time_to_overheat_secs = 0;
    int32_t time_to_hysteresis_secs = 0;
    int32_t time_to_inactive_secs = 0;

    // TODO(achant b/182941868): test return value and skip reporting in case of an error
    ReadFileToInt((kUsbPortOverheatPath + "/plug_temp"), &plug_temperature_deci_c);
    ReadFileToInt((kUsbPortOverheatPath + "/max_temp"), &max_temperature_deci_c);
    ReadFileToInt((kUsbPortOverheatPath + "/trip_time"), &time_to_overheat_secs);
    ReadFileToInt((kUsbPortOverheatPath + "/hysteresis_time"), &time_to_hysteresis_secs);
    ReadFileToInt((kUsbPortOverheatPath + "/cleared_time"), &time_to_inactive_secs);

    VendorUsbPortOverheat overheat_info;
    overheat_info.set_plug_temperature_deci_c(plug_temperature_deci_c);
    overheat_info.set_max_temperature_deci_c(max_temperature_deci_c);
    overheat_info.set_time_to_overheat_secs(time_to_overheat_secs);
    overheat_info.set_time_to_hysteresis_secs(time_to_hysteresis_secs);
    overheat_info.set_time_to_inactive_secs(time_to_inactive_secs);

    reportUsbPortOverheat(stats_client, overheat_info);
}

void UeventListener::ReportChargeStats(const std::shared_ptr<IStats> &stats_client,
                                       const std::string line, const std::string wline_at,
                                       const std::string wline_ac, const std::string pca_line) {
    int charge_stats_fields[] = {ChargeStats::kAdapterTypeFieldNumber,
                                 ChargeStats::kAdapterVoltageFieldNumber,
                                 ChargeStats::kAdapterAmperageFieldNumber,
                                 ChargeStats::kSsocInFieldNumber,
                                 ChargeStats::kVoltageInFieldNumber,
                                 ChargeStats::kSsocOutFieldNumber,
                                 ChargeStats::kVoltageOutFieldNumber,
                                 ChargeStats::kAdapterCapabilities0FieldNumber,
                                 ChargeStats::kAdapterCapabilities1FieldNumber,
                                 ChargeStats::kAdapterCapabilities2FieldNumber,
                                 ChargeStats::kAdapterCapabilities3FieldNumber,
                                 ChargeStats::kAdapterCapabilities4FieldNumber,
                                 ChargeStats::kReceiverState0FieldNumber,
                                 ChargeStats::kReceiverState1FieldNumber};
    const int32_t chg_fields_size = std::size(charge_stats_fields);
    static_assert(chg_fields_size == 14, "Unexpected charge stats fields size");
    const int32_t wlc_fields_size = 7;
    std::vector<VendorAtomValue> values(chg_fields_size);
    VendorAtomValue val;
    int32_t i = 0, tmp[chg_fields_size] = {0}, fields_size = (chg_fields_size - wlc_fields_size);
    int32_t pca_ac[2] = {0}, pca_rs[5] = {0};

    ALOGD("ChargeStats: processing %s", line.c_str());
    if (sscanf(line.c_str(), "%d,%d,%d, %d,%d,%d,%d", &tmp[0], &tmp[1], &tmp[2], &tmp[3], &tmp[4],
               &tmp[5], &tmp[6]) != 7) {
        ALOGE("Couldn't process %s", line.c_str());
        return;
    }

    if (!wline_at.empty()) {
        int32_t ssoc_tmp = 0;
        ALOGD("ChargeStats(wlc): processing %s", wline_at.c_str());
        if (sscanf(wline_at.c_str(), "A:%d", &ssoc_tmp) != 1) {
            ALOGE("Couldn't process %s", wline_at.c_str());
        } else {
            tmp[0] = wireless_charge_stats_.TranslateSysModeToAtomValue(ssoc_tmp);
            ALOGD("ChargeStats(wlc): processing %s", wline_ac.c_str());
            if (sscanf(wline_ac.c_str(), "D:%x,%x,%x,%x,%x, %x,%x", &tmp[7], &tmp[8], &tmp[9],
                       &tmp[10], &tmp[11], &tmp[12], &tmp[13]) != 7)
                ALOGE("Couldn't process %s", wline_ac.c_str());
            else
                fields_size = chg_fields_size; /* include wlc stats */
        }
    }

    if (!pca_line.empty()) {
        ALOGD("ChargeStats(pca): processing %s", pca_line.c_str());
        if (sscanf(pca_line.c_str(), "D:%x,%x %x,%x,%x,%x,%x", &pca_ac[0], &pca_ac[1], &pca_rs[0],
                   &pca_rs[1], &pca_rs[2], &pca_rs[3], &pca_rs[4]) != 7) {
            ALOGE("Couldn't process %s", pca_line.c_str());
        } else {
            fields_size = chg_fields_size; /* include pca stats */
            tmp[9] = pca_rs[2];
            tmp[10] = pca_rs[3];
            tmp[11] = pca_rs[4];
            tmp[13] = pca_rs[1];
            if (wline_at.empty()) {
                tmp[7] = pca_ac[0];
                tmp[8] = pca_ac[1];
                tmp[12] = pca_rs[0];
            }
        }
    }

    for (i = 0; i < fields_size; i++) {
        val.set<VendorAtomValue::intValue>(tmp[i]);
        values[charge_stats_fields[i] - kVendorAtomOffset] = val;
    }

    VendorAtom event = {.reverseDomainName = PixelAtoms::ReverseDomainNames().pixel(),
                        .atomId = PixelAtoms::Atom::kChargeStats,
                        .values = std::move(values)};
    const ndk::ScopedAStatus ret = stats_client->reportVendorAtom(event);
    if (!ret.isOk())
        ALOGE("Unable to report ChargeStats to Stats service");
}

void UeventListener::ReportVoltageTierStats(const std::shared_ptr<IStats> &stats_client,
                                            const char *line, const bool has_wireless,
                                            const std::string wfile_contents) {
    int voltage_tier_stats_fields[] = {
            VoltageTierStats::kVoltageTierFieldNumber,
            VoltageTierStats::kSocInFieldNumber, /* retrieved via ssoc_tmp */
            VoltageTierStats::kCcInFieldNumber,
            VoltageTierStats::kTempInFieldNumber,
            VoltageTierStats::kTimeFastSecsFieldNumber,
            VoltageTierStats::kTimeTaperSecsFieldNumber,
            VoltageTierStats::kTimeOtherSecsFieldNumber,
            VoltageTierStats::kTempMinFieldNumber,
            VoltageTierStats::kTempAvgFieldNumber,
            VoltageTierStats::kTempMaxFieldNumber,
            VoltageTierStats::kIbattMinFieldNumber,
            VoltageTierStats::kIbattAvgFieldNumber,
            VoltageTierStats::kIbattMaxFieldNumber,
            VoltageTierStats::kIclMinFieldNumber,
            VoltageTierStats::kIclAvgFieldNumber,
            VoltageTierStats::kIclMaxFieldNumber,
            VoltageTierStats::kMinAdapterPowerOutFieldNumber,
            VoltageTierStats::kTimeAvgAdapterPowerOutFieldNumber,
            VoltageTierStats::kMaxAdapterPowerOutFieldNumber,
            VoltageTierStats::kChargingOperatingPointFieldNumber};

    const int32_t vtier_fields_size = std::size(voltage_tier_stats_fields);
    static_assert(vtier_fields_size == 20, "Unexpected voltage tier stats fields size");
    const int32_t wlc_fields_size = 4;
    std::vector<VendorAtomValue> values(vtier_fields_size);
    VendorAtomValue val;
    float ssoc_tmp;
    int32_t i = 0, tmp[vtier_fields_size - 1] = {0}, /* ssoc_tmp is not saved in this array */
            fields_size = (vtier_fields_size - wlc_fields_size);

    if (sscanf(line, "%d, %f,%d,%d, %d,%d,%d, %d,%d,%d, %d,%d,%d, %d,%d,%d", &tmp[0], &ssoc_tmp,
               &tmp[1], &tmp[2], &tmp[3], &tmp[4], &tmp[5], &tmp[6], &tmp[7], &tmp[8], &tmp[9],
               &tmp[10], &tmp[11], &tmp[12], &tmp[13], &tmp[14]) != 16) {
        /* If format isn't as expected, then ignore line on purpose */
        return;
    }

    if (has_wireless) {
        wireless_charge_stats_.CalculateWirelessChargeStats(static_cast<int>(ssoc_tmp),
                                                            wfile_contents);
        tmp[15] = wireless_charge_stats_.pout_min_;
        tmp[16] = wireless_charge_stats_.pout_avg_;
        tmp[17] = wireless_charge_stats_.pout_max_;
        tmp[18] = wireless_charge_stats_.of_freq_;
        fields_size = vtier_fields_size; /* include wlc stats */
    }

    ALOGD("VoltageTierStats: processed %s", line);
    val.set<VendorAtomValue::intValue>(tmp[0]);
    values[voltage_tier_stats_fields[0] - kVendorAtomOffset] = val;
    val.set<VendorAtomValue::floatValue>(ssoc_tmp);
    values[voltage_tier_stats_fields[1] - kVendorAtomOffset] = val;
    for (i = 2; i < fields_size; i++) {
        val.set<VendorAtomValue::intValue>(tmp[i - 1]);
        values[voltage_tier_stats_fields[i] - kVendorAtomOffset] = val;
    }

    VendorAtom event = {.reverseDomainName = PixelAtoms::ReverseDomainNames().pixel(),
                        .atomId = PixelAtoms::Atom::kVoltageTierStats,
                        .values = std::move(values)};
    const ndk::ScopedAStatus ret = stats_client->reportVendorAtom(event);
    if (!ret.isOk())
        ALOGE("Unable to report VoltageTierStats to Stats service");
}

void UeventListener::ReportChargeMetricsEvent(const std::shared_ptr<IStats> &stats_client,
                                              const char *driver) {
    if (!driver || strcmp(driver, "DRIVER=google,battery")) {
        return;
    }

    std::string file_contents, line, wfile_contents, wline_at, wline_ac, pca_file_contents,
            pca_line;
    std::istringstream ss;
    bool has_wireless = wireless_charge_stats_.CheckWirelessContentsAndAck(&wfile_contents);
    bool has_pca = pca_charge_stats_.CheckPcaContentsAndAck(&pca_file_contents);

    if (!ReadFileToString(kChargeMetricsPath.c_str(), &file_contents)) {
        ALOGE("Unable to read %s - %s", kChargeMetricsPath.c_str(), strerror(errno));
        return;
    }

    ss.str(file_contents);

    if (!std::getline(ss, line)) {
        ALOGE("Unable to read first line");
        return;
    }

    if (!WriteStringToFile(std::to_string(0), kChargeMetricsPath.c_str())) {
        ALOGE("Couldn't clear %s - %s", kChargeMetricsPath.c_str(), strerror(errno));
    }

    if (has_pca) {
        std::istringstream pca_ss;

        pca_ss.str(pca_file_contents);
        std::getline(pca_ss, pca_line);
    }

    if (has_wireless) {
        std::istringstream wss;

        /* there are two lines in the head, A: ...(Adapter Type) and D: ...(Adapter Capabilities) */
        wss.str(wfile_contents);
        std::getline(wss, wline_at);
        std::getline(wss, wline_ac);

        /* reset initial tier soc */
        wireless_charge_stats_.tier_soc_ = 0;
    }

    ReportChargeStats(stats_client, line, wline_at, wline_ac, pca_line);

    while (std::getline(ss, line)) {
        ReportVoltageTierStats(stats_client, line.c_str(), has_wireless, wfile_contents);
    }
}

/* ReportWlc
 * Report wireless relate  metrics when wireless charging start
 */
void UeventListener::ReportWlc(const std::shared_ptr<IStats> &stats_client, const bool pow_wireless,
                               const bool online, const char *ptmc) {
    if (!pow_wireless) {
        return;
    }

    wlc_reporter_.checkAndReport(stats_client, online, ptmc);
}
/**
 * Report raw battery capacity, system battery capacity and associated
 * battery capacity curves. This data is collected to verify the filter
 * applied on the battery capacity. This will allow debugging of issues
 * ranging from incorrect fuel gauge hardware calculations to issues
 * with the software reported battery capacity.
 *
 * The data is retrieved by parsing the battery power supply's ssoc_details.
 *
 * This atom logs data in 5 potential events:
 *      1. When a device is connected
 *      2. When a device is disconnected
 *      3. When a device has reached a full charge (from the UI's perspective)
 *      4. When there is a >= 2 percent skip in the UI reported SOC
 *      5. When there is a difference of >= 4 percent between the raw hardware
 *          battery capacity and the system reported battery capacity.
 */
void UeventListener::ReportBatteryCapacityFGEvent(const std::shared_ptr<IStats> &stats_client,
                                                  const char *subsystem) {
    if (!subsystem || strcmp(subsystem, "SUBSYSTEM=power_supply")) {
        return;
    }

    // Indicates an implicit disable of the battery capacity reporting
    if (kBatterySSOCPath.empty()) {
        return;
    }

    battery_capacity_reporter_.checkAndReport(stats_client, kBatterySSOCPath);
}

void UeventListener::ReportTypeCPartnerId(const std::shared_ptr<IStats> &stats_client) {
    std::string file_contents_vid, file_contents_pid;
    uint32_t pid, vid;

    if (!ReadFileToString(kTypeCPartnerVidPath.c_str(), &file_contents_vid)) {
        ALOGE("Unable to read %s - %s", kTypeCPartnerVidPath.c_str(), strerror(errno));
        return;
    }

    if (sscanf(file_contents_vid.c_str(), "%x", &vid) != 1) {
        ALOGE("Unable to parse vid %s from file %s to int.", file_contents_vid.c_str(),
              kTypeCPartnerVidPath.c_str());
        return;
    }

    if (!ReadFileToString(kTypeCPartnerPidPath.c_str(), &file_contents_pid)) {
        ALOGE("Unable to read %s - %s", kTypeCPartnerPidPath.c_str(), strerror(errno));
        return;
    }

    if (sscanf(file_contents_pid.substr(PID_OFFSET, PID_LENGTH).c_str(), "%x", &pid) != 1) {
        ALOGE("Unable to parse pid %s from file %s to int.",
              file_contents_pid.substr(PID_OFFSET, PID_LENGTH).c_str(),
              kTypeCPartnerPidPath.c_str());
        return;
    }

    // Upload data only for chargers
    if (((vid >> PRODUCT_TYPE_OFFSET) & PRODUCT_TYPE_MASK) != PRODUCT_TYPE_CHARGER) {
        return;
    }

    // Upload data only for Google VID
    if ((VID_MASK & vid) != VID_GOOGLE) {
        return;
    }

    std::vector<VendorAtomValue> values(2);
    VendorAtomValue tmp;

    tmp.set<VendorAtomValue::intValue>(vid & VID_MASK);
    values[PdVidPid::kVidFieldNumber - kVendorAtomOffset] = tmp;
    tmp.set<VendorAtomValue::intValue>(pid);
    values[PdVidPid::kPidFieldNumber - kVendorAtomOffset] = tmp;

    // Send vendor atom to IStats HAL
    VendorAtom event = {.reverseDomainName = PixelAtoms::ReverseDomainNames().pixel(),
                        .atomId = PixelAtoms::Atom::kPdVidPid,
                        .values = std::move(values)};
    const ndk::ScopedAStatus ret = stats_client->reportVendorAtom(event);
    if (!ret.isOk()) {
        ALOGE("Unable to report PD VID/PID to Stats service");
    }
}

bool UeventListener::ProcessUevent() {
    char msg[UEVENT_MSG_LEN + 2];
    char *cp;
    const char *driver, *product, *subsystem;
    const char *mic_break_status, *mic_degrade_status;
    const char *devpath;
    bool collect_partner_id = false;
    bool pow_online;
    bool pow_wireless;
    const char *pow_ptmc;
    int n;

    if (uevent_fd_ < 0) {
        uevent_fd_ = uevent_open_socket(64 * 1024, true);
        if (uevent_fd_ < 0) {
            ALOGE("uevent_init: uevent_open_socket failed\n");
            return false;
        }
    }

#ifdef LOG_UEVENTS_TO_FILE_ONLY_FOR_DEVEL
    if (log_fd_ < 0) {
        /* Intentionally no O_CREAT so no logging will happen
         * unless the user intentionally 'touch's the file.
         */
        log_fd_ = open(LOG_UEVENTS_TO_FILE_ONLY_FOR_DEVEL, O_WRONLY);
    }
#endif

    n = uevent_kernel_multicast_recv(uevent_fd_, msg, UEVENT_MSG_LEN);
    if (n <= 0 || n >= UEVENT_MSG_LEN)
        return false;

    // Ensure double-null termination of msg.
    msg[n] = '\0';
    msg[n + 1] = '\0';

    driver = product = subsystem = NULL;
    mic_break_status = mic_degrade_status = devpath = pow_ptmc = NULL;
    pow_online = pow_wireless = false;

    /**
     * msg is a sequence of null-terminated strings.
     * Iterate through and record positions of string/value pairs of interest.
     * Double null indicates end of the message. (enforced above).
     */
    cp = msg;
    while (*cp) {
        if (log_fd_ > 0) {
            write(log_fd_, cp, strlen(cp));
            write(log_fd_, "\n", 1);
        }
        if (!strncmp(cp, "DRIVER=", strlen("DRIVER="))) {
            driver = cp;
        } else if (!strncmp(cp, "PRODUCT=", strlen("PRODUCT="))) {
            product = cp;
        } else if (!strncmp(cp, "MIC_BREAK_STATUS=", strlen("MIC_BREAK_STATUS="))) {
            mic_break_status = cp;
        } else if (!strncmp(cp, "MIC_DEGRADE_STATUS=", strlen("MIC_DEGRADE_STATUS="))) {
            mic_degrade_status = cp;
        } else if (!strncmp(cp, "DEVPATH=", strlen("DEVPATH="))) {
            devpath = cp;
        } else if (!strncmp(cp, "SUBSYSTEM=", strlen("SUBSYSTEM="))) {
            subsystem = cp;
        } else if (!strncmp(cp, "DEVTYPE=typec_partner", strlen("DEVTYPE=typec_partner"))) {
            collect_partner_id = true;
        } else if (!strncmp(cp, "POWER_SUPPLY_NAME=wireless",
                            strlen("POWER_SUPPLY_NAME=wireless"))) {
            pow_wireless = true;
        } else if (!strncmp(cp, "POWER_SUPPLY_ONLINE=1", strlen("POWER_SUPPLY_ONLINE=1"))) {
            pow_online = true;
        } else if (!kWirelessChargerPtmcUevent.empty() &&
                   !strncmp(cp, kWirelessChargerPtmcUevent.c_str(),
                            strlen(kWirelessChargerPtmcUevent.c_str()))) {
            pow_ptmc = cp + strlen(kWirelessChargerPtmcUevent.c_str());
        }
        /* advance to after the next \0 */
        while (*cp++) {
        }
    }

    std::shared_ptr<IStats> stats_client = getStatsService();
    if (!stats_client) {
        ALOGE("Unable to get Stats service instance.");
    } else {
        /* Process the strings recorded. */
        ReportMicStatusUevents(stats_client, devpath, mic_break_status);
        ReportMicStatusUevents(stats_client, devpath, mic_degrade_status);
        ReportUsbPortOverheatEvent(stats_client, driver);
        ReportChargeMetricsEvent(stats_client, driver);
        ReportWlc(stats_client, pow_wireless, pow_online, pow_ptmc);
        ReportBatteryCapacityFGEvent(stats_client, subsystem);
        if (collect_partner_id) {
            ReportTypeCPartnerId(stats_client);
        }
    }

    if (log_fd_ > 0) {
        write(log_fd_, "\n", 1);
    }
    return true;
}

UeventListener::UeventListener(const std::string audio_uevent, const std::string ssoc_details_path,
                               const std::string overheat_path,
                               const std::string charge_metrics_path,
                               const std::string typec_partner_vid_path,
                               const std::string typec_partner_pid_path)
    : kAudioUevent(audio_uevent),
      kBatterySSOCPath(ssoc_details_path),
      kUsbPortOverheatPath(overheat_path),
      kChargeMetricsPath(charge_metrics_path),
      kTypeCPartnerVidPath(typec_partner_vid_path),
      kTypeCPartnerPidPath(typec_partner_pid_path),
      kWirelessChargerPtmcUevent(""),
      kWirelessChargerPtmcPath(""),
      uevent_fd_(-1),
      log_fd_(-1) {}

UeventListener::UeventListener(const struct UeventPaths &uevents_paths)
    : kAudioUevent((uevents_paths.AudioUevent == nullptr) ? "" : uevents_paths.AudioUevent),
      kBatterySSOCPath((uevents_paths.SsocDetailsPath == nullptr) ? ssoc_details_path
                                                                  : uevents_paths.SsocDetailsPath),
      kUsbPortOverheatPath((uevents_paths.OverheatPath == nullptr) ? overheat_path_default
                                                                   : uevents_paths.OverheatPath),
      kChargeMetricsPath((uevents_paths.ChargeMetricsPath == nullptr)
                                 ? charge_metrics_path_default
                                 : uevents_paths.ChargeMetricsPath),
      kTypeCPartnerVidPath((uevents_paths.TypeCPartnerVidPath == nullptr)
                                   ? typec_partner_vid_path_default
                                   : uevents_paths.TypeCPartnerVidPath),
      kTypeCPartnerPidPath((uevents_paths.TypeCPartnerPidPath == nullptr)
                                   ? typec_partner_pid_path_default
                                   : uevents_paths.TypeCPartnerPidPath),
      kWirelessChargerPtmcUevent((uevents_paths.WirelessChargerPtmcUevent == nullptr)
                                         ? ""
                                         : uevents_paths.WirelessChargerPtmcUevent),
      kWirelessChargerPtmcPath((uevents_paths.WirelessChargerPtmcPath == nullptr)
                                       ? ""
                                       : uevents_paths.WirelessChargerPtmcPath),
      uevent_fd_(-1),
      log_fd_(-1) {}

/* Thread function to continuously monitor uevents.
 * Exit after kMaxConsecutiveErrors to prevent spinning. */
void UeventListener::ListenForever() {
    constexpr int kMaxConsecutiveErrors = 10;
    int consecutive_errors = 0;

    while (1) {
        if (ProcessUevent()) {
            consecutive_errors = 0;
        } else {
            if (++consecutive_errors >= kMaxConsecutiveErrors) {
                ALOGE("Too many ProcessUevent errors; exiting UeventListener.");
                return;
            }
        }
    }
}

}  // namespace pixel
}  // namespace google
}  // namespace hardware
}  // namespace android
