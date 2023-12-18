/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef HARDWARE_GOOGLE_PIXEL_PIXELSTATS_UEVENTLISTENER_H
#define HARDWARE_GOOGLE_PIXEL_PIXELSTATS_UEVENTLISTENER_H

#include <aidl/android/frameworks/stats/IStats.h>
#include <android-base/chrono_utils.h>
#include <pixelstats/BatteryCapacityReporter.h>
#include <pixelstats/PcaChargeStats.h>
#include <pixelstats/WirelessChargeStats.h>
#include <pixelstats/WlcReporter.h>

namespace android {
namespace hardware {
namespace google {
namespace pixel {

using aidl::android::frameworks::stats::IStats;

/**
 * A class to listen for uevents and report reliability events to
 * the PixelStats HAL.
 * Runs in a background thread if created with ListenForeverInNewThread().
 * Alternatively, process one message at a time with ProcessUevent().
 */
class UeventListener {
  public:
    /* Both WirelessChargerPtmcUevent and WirelessChargerPtmcPath is use to get
     * wireless charger ptmx id, for most case we don't need asisign both of
     * them.
     **/
    struct UeventPaths {
        const char *const AudioUevent;
        const char *const SsocDetailsPath;
        const char *const OverheatPath;
        const char *const ChargeMetricsPath;
        const char *const TypeCPartnerVidPath;
        const char *const TypeCPartnerPidPath;
        const char *const WirelessChargerPtmcUevent;
        const char *const WirelessChargerPtmcPath;
    };
    constexpr static const char *const ssoc_details_path =
            "/sys/class/power_supply/battery/ssoc_details";
    constexpr static const char *const overheat_path_default =
            "/sys/devices/platform/soc/soc:google,overheat_mitigation";
    constexpr static const char *const charge_metrics_path_default =
            "/sys/class/power_supply/battery/charge_stats";
    constexpr static const char *const typec_partner_vid_path_default =
            "/sys/class/typec/port0-partner/identity/id_header";
    constexpr static const char *const typec_partner_pid_path_default =
            "/sys/class/typec/port0-partner/identity/product";

    UeventListener(const std::string audio_uevent, const std::string ssoc_details_path = "",
                   const std::string overheat_path = overheat_path_default,
                   const std::string charge_metrics_path = charge_metrics_path_default,
                   const std::string typec_partner_vid_path = typec_partner_vid_path_default,
                   const std::string typec_partner_pid_path = typec_partner_pid_path_default);
    UeventListener(const struct UeventPaths &paths);

    bool ProcessUevent();  // Process a single Uevent.
    void ListenForever();  // Process Uevents forever

  private:
    bool ReadFileToInt(const std::string &path, int *val);
    bool ReadFileToInt(const char *path, int *val);
    void ReportMicStatusUevents(const std::shared_ptr<IStats> &stats_client, const char *devpath,
                                const char *mic_status);
    void ReportMicBrokenOrDegraded(const std::shared_ptr<IStats> &stats_client, const int mic,
                                   const bool isBroken);
    void ReportUsbPortOverheatEvent(const std::shared_ptr<IStats> &stats_client,
                                    const char *driver);
    void ReportChargeStats(const std::shared_ptr<IStats> &stats_client, const std::string line,
                           const std::string wline_at, const std::string wline_ac,
                           const std::string pca_line);
    void ReportVoltageTierStats(const std::shared_ptr<IStats> &stats_client, const char *line,
                                const bool has_wireless, const std::string wfile_contents);
    void ReportChargeMetricsEvent(const std::shared_ptr<IStats> &stats_client, const char *driver);
    void ReportWlc(const std::shared_ptr<IStats> &stats_client, const bool pow_wireless,
                   const bool online, const char *ptmc);
    void ReportBatteryCapacityFGEvent(const std::shared_ptr<IStats> &stats_client,
                                      const char *subsystem);
    void ReportTypeCPartnerId(const std::shared_ptr<IStats> &stats_client);

    const std::string kAudioUevent;
    const std::string kBatterySSOCPath;
    const std::string kUsbPortOverheatPath;
    const std::string kChargeMetricsPath;
    const std::string kTypeCPartnerVidPath;
    const std::string kTypeCPartnerPidPath;
    const std::string kWirelessChargerPtmcUevent;
    const std::string kWirelessChargerPtmcPath;

    BatteryCapacityReporter battery_capacity_reporter_;

    // Proto messages are 1-indexed and VendorAtom field numbers start at 2, so
    // store everything in the values array at the index of the field number
    // -2.
    const int kVendorAtomOffset = 2;

    int uevent_fd_;
    int log_fd_;

    PcaChargeStats pca_charge_stats_;
    WirelessChargeStats wireless_charge_stats_;

    WlcReporter wlc_reporter_ = WlcReporter(kWirelessChargerPtmcPath.c_str());
};

}  // namespace pixel
}  // namespace google
}  // namespace hardware
}  // namespace android

#endif  // HARDWARE_GOOGLE_PIXEL_PIXELSTATS_UEVENTLISTENER_H
