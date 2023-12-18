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

#ifndef HARDWARE_GOOGLE_PIXEL_PIXELSTATS_MMMETRICSREPORTER_H
#define HARDWARE_GOOGLE_PIXEL_PIXELSTATS_MMMETRICSREPORTER_H

#include <map>
#include <string>

#include <aidl/android/frameworks/stats/IStats.h>
#include <hardware/google/pixel/pixelstats/pixelatoms.pb.h>

namespace android {
namespace hardware {
namespace google {
namespace pixel {

using aidl::android::frameworks::stats::IStats;
using aidl::android::frameworks::stats::VendorAtomValue;

/**
 * A class to upload Pixel MM health metrics
 */
class MmMetricsReporter {
  public:
    MmMetricsReporter();
    void logPixelMmMetricsPerHour(const std::shared_ptr<IStats> &stats_client);
    void logPixelMmMetricsPerDay(const std::shared_ptr<IStats> &stats_client);
    void logCmaStatus(const std::shared_ptr<IStats> &stats_client);

  private:
    struct MmMetricsInfo {
        std::string name;
        int atom_key;
        bool update_diff;
    };

    enum CmaType {
        FARAWIMG = 0,
        FAIMG = 1,
        FATPU = 2,
        FAPREV = 3,
        VFRAME = 4,
        VSTREAM = 5,
    };

    static const std::vector<MmMetricsInfo> kMmMetricsPerHourInfo;
    static const std::vector<MmMetricsInfo> kMmMetricsPerDayInfo;
    static const std::vector<MmMetricsInfo> kCmaStatusInfo;
    static const std::vector<MmMetricsInfo> kCmaStatusExtInfo;
    static const std::map<std::string, CmaType> kCmaTypeInfo;

    bool ReadFileToUint(const char *const path, uint64_t *val);
    bool reportVendorAtom(const std::shared_ptr<IStats> &stats_client, int atom_id,
                          const std::vector<VendorAtomValue> &values, const std::string &atom_name);
    std::map<std::string, uint64_t> readVmStat(const char *path);
    uint64_t getIonTotalPools();
    uint64_t getGpuMemory();
    void fillAtomValues(const std::vector<MmMetricsInfo> &metrics_info,
                        const std::map<std::string, uint64_t> &mm_metrics,
                        std::map<std::string, uint64_t> *prev_mm_metrics,
                        std::vector<VendorAtomValue> *atom_values);
    bool isValidPid(int pid, const char *name);
    int findPidByProcessName(const char *name);
    uint64_t getStimeByPid(int pid);
    void fillProcessStime(int atom_key, const char *name, int *pid, uint64_t *prev_stime,
                          std::vector<VendorAtomValue> *atom_values);
    std::map<std::string, uint64_t> readCmaStat(const std::string &cma_type,
                                                const std::vector<MmMetricsInfo> &metrics_info);
    void reportCmaStatusAtom(const std::shared_ptr<IStats> &stats_client, int atom_id,
                             const std::string &cma_type, CmaType type_idx,
                             const std::vector<MmMetricsInfo> &metrics_info,
                             std::map<CmaType, std::map<std::string, uint64_t>> *all_prev_cma_stat);

    const char *const kVmstatPath;
    const char *const kIonTotalPoolsPath;
    const char *const kIonTotalPoolsPathForLegacy;
    const char *const kGpuTotalPages;
    const char *const kPixelStatMm;
    // Proto messages are 1-indexed and VendorAtom field numbers start at 2, so
    // store everything in the values array at the index of the field number
    // -2.
    const int kVendorAtomOffset = 2;

    std::map<std::string, uint64_t> prev_hour_vmstat_;
    std::map<std::string, uint64_t> prev_day_vmstat_;
    std::map<std::string, uint64_t> prev_day_pixel_vmstat_;
    std::map<CmaType, std::map<std::string, uint64_t>> prev_cma_stat_;
    std::map<CmaType, std::map<std::string, uint64_t>> prev_cma_stat_ext_;
    int kswapd_pid_ = -1;
    int kcompactd_pid_ = -1;
    uint64_t prev_kswapd_stime_ = 0;
    uint64_t prev_kcompactd_stime_ = 0;
};

}  // namespace pixel
}  // namespace google
}  // namespace hardware
}  // namespace android

#endif  // HARDWARE_GOOGLE_PIXEL_PIXELSTATS_BATTERYCAPACITYREPORTER_H
