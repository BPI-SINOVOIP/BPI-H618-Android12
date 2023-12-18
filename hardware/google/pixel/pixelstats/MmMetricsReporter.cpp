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

#define LOG_TAG "pixelstats: MmMetrics"

#include <aidl/android/frameworks/stats/IStats.h>
#include <android-base/file.h>
#include <android-base/parseint.h>
#include <android-base/properties.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <android/binder_manager.h>
#include <hardware/google/pixel/pixelstats/pixelatoms.pb.h>
#include <pixelstats/MmMetricsReporter.h>
#include <utils/Log.h>

#define SZ_4K 0x00001000
#define SZ_2M 0x00200000

namespace android {
namespace hardware {
namespace google {
namespace pixel {

using aidl::android::frameworks::stats::IStats;
using aidl::android::frameworks::stats::VendorAtom;
using aidl::android::frameworks::stats::VendorAtomValue;
using android::base::ReadFileToString;
using android::base::StartsWith;
using android::hardware::google::pixel::PixelAtoms::CmaStatus;
using android::hardware::google::pixel::PixelAtoms::CmaStatusExt;
using android::hardware::google::pixel::PixelAtoms::PixelMmMetricsPerDay;
using android::hardware::google::pixel::PixelAtoms::PixelMmMetricsPerHour;

const std::vector<MmMetricsReporter::MmMetricsInfo> MmMetricsReporter::kMmMetricsPerHourInfo = {
        {"nr_free_pages", PixelMmMetricsPerHour::kFreePagesFieldNumber, false},
        {"nr_anon_pages", PixelMmMetricsPerHour::kAnonPagesFieldNumber, false},
        {"nr_file_pages", PixelMmMetricsPerHour::kFilePagesFieldNumber, false},
        {"nr_slab_reclaimable", PixelMmMetricsPerHour::kSlabReclaimableFieldNumber, false},
        {"nr_zspages", PixelMmMetricsPerHour::kZspagesFieldNumber, false},
        {"nr_unevictable", PixelMmMetricsPerHour::kUnevictableFieldNumber, false},
};

const std::vector<MmMetricsReporter::MmMetricsInfo> MmMetricsReporter::kMmMetricsPerDayInfo = {
        {"workingset_refault", PixelMmMetricsPerDay::kWorkingsetRefaultFieldNumber, true},
        {"workingset_refault_file", PixelMmMetricsPerDay::kWorkingsetRefaultFieldNumber, true},
        {"pswpin", PixelMmMetricsPerDay::kPswpinFieldNumber, true},
        {"pswpout", PixelMmMetricsPerDay::kPswpoutFieldNumber, true},
        {"allocstall_dma", PixelMmMetricsPerDay::kAllocstallDmaFieldNumber, true},
        {"allocstall_dma32", PixelMmMetricsPerDay::kAllocstallDma32FieldNumber, true},
        {"allocstall_normal", PixelMmMetricsPerDay::kAllocstallNormalFieldNumber, true},
        {"allocstall_movable", PixelMmMetricsPerDay::kAllocstallMovableFieldNumber, true},
        {"pgalloc_dma", PixelMmMetricsPerDay::kPgallocDmaFieldNumber, true},
        {"pgalloc_dma32", PixelMmMetricsPerDay::kPgallocDma32FieldNumber, true},
        {"pgalloc_normal", PixelMmMetricsPerDay::kPgallocNormalFieldNumber, true},
        {"pgalloc_movable", PixelMmMetricsPerDay::kPgallocMovableFieldNumber, true},
        {"pgsteal_kswapd", PixelMmMetricsPerDay::kPgstealKswapdFieldNumber, true},
        {"pgsteal_direct", PixelMmMetricsPerDay::kPgstealDirectFieldNumber, true},
        {"pgscan_kswapd", PixelMmMetricsPerDay::kPgscanKswapdFieldNumber, true},
        {"pgscan_direct", PixelMmMetricsPerDay::kPgscanDirectFieldNumber, true},
        {"oom_kill", PixelMmMetricsPerDay::kOomKillFieldNumber, true},
        {"pgalloc_costly_order", PixelMmMetricsPerDay::kPgallocHighFieldNumber, true},
        {"pgcache_hit", PixelMmMetricsPerDay::kPgcacheHitFieldNumber, true},
        {"pgcache_miss", PixelMmMetricsPerDay::kPgcacheMissFieldNumber, true},
};

const std::vector<MmMetricsReporter::MmMetricsInfo> MmMetricsReporter::kCmaStatusInfo = {
        {"alloc_pages_attempts", CmaStatus::kCmaAllocPagesAttemptsFieldNumber, true},
        {"alloc_pages_failfast_attempts", CmaStatus::kCmaAllocPagesSoftAttemptsFieldNumber, true},
        {"fail_pages", CmaStatus::kCmaFailPagesFieldNumber, true},
        {"fail_failfast_pages", CmaStatus::kCmaFailSoftPagesFieldNumber, true},
        {"migrated_pages", CmaStatus::kMigratedPagesFieldNumber, true},
};

const std::vector<MmMetricsReporter::MmMetricsInfo> MmMetricsReporter::kCmaStatusExtInfo = {
        {"latency_low", CmaStatusExt::kCmaAllocLatencyLowFieldNumber, false},
        {"latency_mid", CmaStatusExt::kCmaAllocLatencyMidFieldNumber, false},
        {"latency_high", CmaStatusExt::kCmaAllocLatencyHighFieldNumber, false},
};

const std::map<std::string, MmMetricsReporter::CmaType> MmMetricsReporter::kCmaTypeInfo = {
        {"farawimg", MmMetricsReporter::FARAWIMG},  {"faimg", MmMetricsReporter::FAIMG},
        {"faceauth_tpu", MmMetricsReporter::FATPU}, {"faprev", MmMetricsReporter::FAPREV},
        {"vframe", MmMetricsReporter::VFRAME},      {"vstream", MmMetricsReporter::VSTREAM},
};

MmMetricsReporter::MmMetricsReporter()
    : kVmstatPath("/proc/vmstat"),
      kIonTotalPoolsPath("/sys/kernel/dma_heap/total_pools_kb"),
      kIonTotalPoolsPathForLegacy("/sys/kernel/ion/total_pools_kb"),
      kGpuTotalPages("/sys/kernel/pixel_stat/gpu/mem/total_page_count"),
      kPixelStatMm("/sys/kernel/pixel_stat/mm") {}

bool MmMetricsReporter::ReadFileToUint(const char *const path, uint64_t *val) {
    std::string file_contents;

    if (!ReadFileToString(path, &file_contents)) {
        ALOGI("Unable to read %s - %s", path, strerror(errno));
        return false;
    } else {
        file_contents = android::base::Trim(file_contents);
        if (!android::base::ParseUint(file_contents, val)) {
            ALOGI("Unable to convert %s to uint - %s", path, strerror(errno));
            return false;
        }
    }
    return true;
}

bool MmMetricsReporter::reportVendorAtom(const std::shared_ptr<IStats> &stats_client, int atom_id,
                                         const std::vector<VendorAtomValue> &values,
                                         const std::string &atom_name) {
    // Send vendor atom to IStats HAL
    VendorAtom event = {.reverseDomainName = PixelAtoms::ReverseDomainNames().pixel(),
                        .atomId = atom_id,
                        .values = std::move(values)};
    const ndk::ScopedAStatus ret = stats_client->reportVendorAtom(event);
    if (!ret.isOk()) {
        ALOGE("Unable to report %s to Stats service", atom_name.c_str());
        return false;
    }
    return true;
}

/**
 * Parse the output of /proc/vmstat or the sysfs having the same output format.
 * The map containing pairs of {field_string, data} will be returned.
 */
std::map<std::string, uint64_t> MmMetricsReporter::readVmStat(const char *path) {
    std::string file_contents;
    std::map<std::string, uint64_t> vmstat_data;

    if (path == nullptr) {
        ALOGI("vmstat path is not specified");
        return vmstat_data;
    }

    if (!ReadFileToString(path, &file_contents)) {
        ALOGE("Unable to read vmstat from %s, err: %s", path, strerror(errno));
        return vmstat_data;
    }

    std::istringstream data(file_contents);
    std::string line;
    while (std::getline(data, line)) {
        std::vector<std::string> words = android::base::Split(line, " ");
        if (words.size() != 2)
            continue;

        uint64_t i;
        if (!android::base::ParseUint(words[1], &i))
            continue;

        vmstat_data[words[0]] = i;
    }
    return vmstat_data;
}

uint64_t MmMetricsReporter::getIonTotalPools() {
    uint64_t res;

    if (!ReadFileToUint(kIonTotalPoolsPathForLegacy, &res) || (res == 0)) {
        if (!ReadFileToUint(kIonTotalPoolsPath, &res)) {
            return 0;
        }
    }

    return res;
}

/**
 * Collect GPU memory from kGpuTotalPages and return the total number of 4K page.
 */
uint64_t MmMetricsReporter::getGpuMemory() {
    uint64_t gpu_size = 0;

    if (!ReadFileToUint(kGpuTotalPages, &gpu_size)) {
        return 0;
    }
    return gpu_size;
}

/**
 * fillAtomValues() is used to copy Mm metrics to values
 * metrics_info: This is a vector of MmMetricsInfo {field_string, atom_key, update_diff}
 *               field_string is used to get the data from mm_metrics.
 *               atom_key is the position where the data should be put into values.
 *               update_diff will be true if this is an accumulated data.
 *               metrics_info may have multiple entries with the same atom_key,
 *               e.g. workingset_refault and workingset_refault_file.
 * mm_metrics: This map contains pairs of {field_string, cur_value} collected
 *             from /proc/vmstat or the sysfs for the pixel specific metrics.
 *             e.g. {"nr_free_pages", 200000}
 *             Some data in mm_metrics are accumulated, e.g. pswpin.
 *             We upload the difference instead of the accumulated value
 *             when update_diff of the field is true.
 * prev_mm_metrics: The pointer to the metrics we collected last time.
 * atom_values: The atom values that will be reported later.
 */
void MmMetricsReporter::fillAtomValues(const std::vector<MmMetricsInfo> &metrics_info,
                                       const std::map<std::string, uint64_t> &mm_metrics,
                                       std::map<std::string, uint64_t> *prev_mm_metrics,
                                       std::vector<VendorAtomValue> *atom_values) {
    VendorAtomValue tmp;
    tmp.set<VendorAtomValue::longValue>(0);
    // resize atom_values to add all fields defined in metrics_info
    int max_idx = 0;
    for (auto &entry : metrics_info) {
        if (max_idx < entry.atom_key)
            max_idx = entry.atom_key;
    }
    int size = max_idx - kVendorAtomOffset + 1;
    if (atom_values->size() < size)
        atom_values->resize(size, tmp);

    for (auto &entry : metrics_info) {
        int atom_idx = entry.atom_key - kVendorAtomOffset;

        auto data = mm_metrics.find(entry.name);
        if (data == mm_metrics.end())
            continue;

        uint64_t cur_value = data->second;
        uint64_t prev_value = 0;
        if (prev_mm_metrics->size() != 0) {
            auto prev_data = prev_mm_metrics->find(entry.name);
            if (prev_data != prev_mm_metrics->end())
                prev_value = prev_data->second;
        }

        if (entry.update_diff) {
            tmp.set<VendorAtomValue::longValue>(cur_value - prev_value);
        } else {
            tmp.set<VendorAtomValue::longValue>(cur_value);
        }
        (*atom_values)[atom_idx] = tmp;
    }
    (*prev_mm_metrics) = mm_metrics;
}

void MmMetricsReporter::logPixelMmMetricsPerHour(const std::shared_ptr<IStats> &stats_client) {
    // Currently, we collect these metrics and report this atom only for userdebug_or_eng
    // We only grant permissions to access sysfs for userdebug_or_eng.
    // Add a check to avoid unnecessary access.
    if (android::base::GetProperty("ro.build.type", "") == "user")
        return;

    std::map<std::string, uint64_t> vmstat = readVmStat(kVmstatPath);
    if (vmstat.size() == 0)
        return;

    uint64_t ion_total_pools = getIonTotalPools();
    uint64_t gpu_memory = getGpuMemory();

    std::vector<VendorAtomValue> values;
    bool is_first_atom = (prev_hour_vmstat_.size() == 0) ? true : false;
    fillAtomValues(kMmMetricsPerHourInfo, vmstat, &prev_hour_vmstat_, &values);

    // resize values to add the following fields
    VendorAtomValue tmp;
    tmp.set<VendorAtomValue::longValue>(0);
    int size = PixelMmMetricsPerHour::kGpuMemoryFieldNumber - kVendorAtomOffset + 1;
    if (values.size() < size) {
        values.resize(size, tmp);
    }
    tmp.set<VendorAtomValue::longValue>(ion_total_pools);
    values[PixelMmMetricsPerHour::kIonTotalPoolsFieldNumber - kVendorAtomOffset] = tmp;
    tmp.set<VendorAtomValue::longValue>(gpu_memory);
    values[PixelMmMetricsPerHour::kGpuMemoryFieldNumber - kVendorAtomOffset] = tmp;

    // Don't report the first atom to avoid big spike in accumulated values.
    if (!is_first_atom) {
        // Send vendor atom to IStats HAL
        reportVendorAtom(stats_client, PixelAtoms::Atom::kPixelMmMetricsPerHour, values,
                         "PixelMmMetricsPerHour");
    }
}

void MmMetricsReporter::logPixelMmMetricsPerDay(const std::shared_ptr<IStats> &stats_client) {
    // Currently, we collect these metrics and report this atom only for userdebug_or_eng
    // We only grant permissions to access sysfs for userdebug_or_eng.
    // Add a check to avoid unnecessary access.
    if (android::base::GetProperty("ro.build.type", "") == "user")
        return;

    std::map<std::string, uint64_t> vmstat = readVmStat(kVmstatPath);
    if (vmstat.size() == 0)
        return;

    std::vector<VendorAtomValue> values;
    bool is_first_atom = (prev_day_vmstat_.size() == 0) ? true : false;
    fillAtomValues(kMmMetricsPerDayInfo, vmstat, &prev_day_vmstat_, &values);

    std::map<std::string, uint64_t> pixel_vmstat =
            readVmStat(android::base::StringPrintf("%s/vmstat", kPixelStatMm).c_str());
    fillAtomValues(kMmMetricsPerDayInfo, pixel_vmstat, &prev_day_pixel_vmstat_, &values);
    fillProcessStime(PixelMmMetricsPerDay::kKswapdStimeClksFieldNumber, "kswapd0", &kswapd_pid_,
                     &prev_kswapd_stime_, &values);
    fillProcessStime(PixelMmMetricsPerDay::kKcompactdStimeClksFieldNumber, "kcompactd0",
                     &kcompactd_pid_, &prev_kcompactd_stime_, &values);

    // Don't report the first atom to avoid big spike in accumulated values.
    if (!is_first_atom) {
        // Send vendor atom to IStats HAL
        reportVendorAtom(stats_client, PixelAtoms::Atom::kPixelMmMetricsPerDay, values,
                         "PixelMmMetricsPerDay");
    }
}

/**
 * Check if /proc/<pid>/comm is equal to name.
 */
bool MmMetricsReporter::isValidPid(int pid, const char *name) {
    if (pid <= 0)
        return false;

    std::string file_contents;
    std::string path = android::base::StringPrintf("/proc/%d/comm", pid);
    if (!ReadFileToString(path, &file_contents)) {
        ALOGI("Unable to read %s, err: %s", path.c_str(), strerror(errno));
        return false;
    }

    file_contents = android::base::Trim(file_contents);
    return !file_contents.compare(name);
}

/**
 * Return pid if /proc/<pid>/comm is equal to name, or -1 if not found.
 */
int MmMetricsReporter::findPidByProcessName(const char *name) {
    std::unique_ptr<DIR, int (*)(DIR *)> dir(opendir("/proc"), closedir);
    if (!dir)
        return -1;

    int pid;
    while (struct dirent *dp = readdir(dir.get())) {
        if (dp->d_type != DT_DIR)
            continue;

        if (!android::base::ParseInt(dp->d_name, &pid))
            continue;

        // Avoid avc denial since pixelstats-vendor doesn't have the permission to access /proc/1
        if (pid == 1)
            continue;

        std::string file_contents;
        std::string path = android::base::StringPrintf("/proc/%s/comm", dp->d_name);
        if (!ReadFileToString(path, &file_contents))
            continue;

        file_contents = android::base::Trim(file_contents);
        if (file_contents.compare(name))
            continue;

        return pid;
    }
    return -1;
}

/**
 * Get stime of a process from /proc/<pid>/stat
 * stime is the 15th field.
 */
uint64_t MmMetricsReporter::getStimeByPid(int pid) {
    const int stime_idx = 15;
    uint64_t stime;
    std::string file_contents;
    std::string path = android::base::StringPrintf("/proc/%d/stat", pid);
    if (!ReadFileToString(path, &file_contents)) {
        ALOGI("Unable to read %s, err: %s", path.c_str(), strerror(errno));
        return false;
    }

    std::vector<std::string> data = android::base::Split(file_contents, " ");
    if (data.size() < stime_idx) {
        ALOGI("Unable to find stime from %s. size: %lu", path.c_str(), data.size());
        return false;
    }

    if (android::base::ParseUint(data[stime_idx - 1], &stime))
        return stime;
    else
        return 0;
}

/**
 * Find stime of the process and copy it into atom_values
 * atom_key: Currently, it can only be kKswapdTimeFieldNumber or kKcompactdTimeFieldNumber
 * name: process name
 * pid: The pid of the process. It would be the pid we found last time,
 *      or -1 if not found.
 * prev_stime: The stime of the process collected last time.
 * atom_values: The atom we will report later.
 */
void MmMetricsReporter::fillProcessStime(int atom_key, const char *name, int *pid,
                                         uint64_t *prev_stime,
                                         std::vector<VendorAtomValue> *atom_values) {
    // resize atom_values if there is no space for this stime field.
    int atom_idx = atom_key - kVendorAtomOffset;
    int size = atom_idx + 1;
    VendorAtomValue tmp;
    tmp.set<VendorAtomValue::longValue>(0);
    if (atom_values->size() < size)
        atom_values->resize(size, tmp);

    if (!isValidPid(*pid, name)) {
        (*pid) = findPidByProcessName(name);
        if ((*pid) <= 0) {
            ALOGI("Unable to find pid of %s, err: %s", name, strerror(errno));
            return;
        }
    }

    uint64_t stime = getStimeByPid(*pid);
    tmp.set<VendorAtomValue::longValue>(stime - *prev_stime);
    (*atom_values)[atom_idx] = tmp;
    (*prev_stime) = stime;
}

/**
 * Collect CMA metrics from kPixelStatMm/cma/<cma_type>/<metric>
 * cma_type: CMA heap name
 * metrics_info: This is a vector of MmMetricsInfo {metric, atom_key, update_diff}.
 *               Currently, we only collect CMA metrics defined in metrics_info
 */
std::map<std::string, uint64_t> MmMetricsReporter::readCmaStat(
        const std::string &cma_type,
        const std::vector<MmMetricsReporter::MmMetricsInfo> &metrics_info) {
    uint64_t file_contents;
    std::map<std::string, uint64_t> cma_stat;
    for (auto &entry : metrics_info) {
        std::string path = android::base::StringPrintf("%s/cma/%s/%s", kPixelStatMm,
                                                       cma_type.c_str(), entry.name.c_str());
        if (!ReadFileToUint(path.c_str(), &file_contents))
            continue;
        cma_stat[entry.name] = file_contents;
    }
    return cma_stat;
}

/**
 * This function is to collect CMA metrics and upload them.
 * The CMA metrics are collected by readCmaStat(), copied into atom values
 * by fillAtomValues(), and then uploaded by reportVendorAtom(). The collected
 * metrics will be stored in prev_cma_stat_ and prev_cma_stat_ext_ according
 * to its CmaType.
 *
 * stats_client: The Stats service
 * atom_id: The id of atom. It can be PixelAtoms::Atom::kCmaStatus or kCmaStatusExt
 * cma_type: The name of CMA heap. We only collect metrics from CMA heaps defined
 *           in kCmaTypeInfo.
 * type_idx: The id of the CMA heap. We add this id in atom values to identify
 *           the CMA status data.
 * metrics_info: This is a vector of MmMetricsInfo {metric, atom_key, update_diff}.
 *               We only collect metrics defined in metrics_info from CMA heap path.
 * all_prev_cma_stat: This is the CMA status collected last time.
 *                    It is a map containing pairs of {type_idx, cma_stat}, and cma_stat is
 *                    a map contains pairs of {metric, cur_value}.
 *                    e.g. {CmaType::FARAWIMG, {"alloc_pages_attempts", 100000}, {...}, ....}
 *                    is collected from kPixelStatMm/cma/farawimg/alloc_pages_attempts
 */
void MmMetricsReporter::reportCmaStatusAtom(
        const std::shared_ptr<IStats> &stats_client, int atom_id, const std::string &cma_type,
        CmaType type_idx, const std::vector<MmMetricsInfo> &metrics_info,
        std::map<CmaType, std::map<std::string, uint64_t>> *all_prev_cma_stat) {
    std::map<std::string, uint64_t> cma_stat = readCmaStat(cma_type, metrics_info);
    if (!cma_stat.empty()) {
        std::vector<VendorAtomValue> values;
        VendorAtomValue tmp;
        tmp.set<VendorAtomValue::intValue>(type_idx);
        values.push_back(tmp);

        std::map<std::string, uint64_t> prev_cma_stat;
        auto entry = all_prev_cma_stat->find(type_idx);
        if (entry != all_prev_cma_stat->end())
            prev_cma_stat = entry->second;

        bool is_first_atom = (prev_cma_stat.size() == 0) ? true : false;
        fillAtomValues(metrics_info, cma_stat, &prev_cma_stat, &values);
        (*all_prev_cma_stat)[type_idx] = prev_cma_stat;
        if (!is_first_atom)
            reportVendorAtom(stats_client, atom_id, values, "CmaStatus");
    }
}

/**
 * Find the CMA heap defined in kCmaTypeInfo, and then call reportCmaStatusAtom()
 * to collect the CMA metrics from kPixelStatMm/cma/<cma_type> and upload them.
 */
void MmMetricsReporter::logCmaStatus(const std::shared_ptr<IStats> &stats_client) {
    std::string cma_root = android::base::StringPrintf("%s/cma", kPixelStatMm);
    std::unique_ptr<DIR, int (*)(DIR *)> dir(opendir(cma_root.c_str()), closedir);
    if (!dir)
        return;

    while (struct dirent *dp = readdir(dir.get())) {
        if (dp->d_type != DT_DIR)
            continue;

        std::string cma_type(dp->d_name);
        auto type = kCmaTypeInfo.find(cma_type);
        if (type == kCmaTypeInfo.end())
            continue;

        reportCmaStatusAtom(stats_client, PixelAtoms::Atom::kCmaStatus, cma_type, type->second,
                            kCmaStatusInfo, &prev_cma_stat_);
        reportCmaStatusAtom(stats_client, PixelAtoms::Atom::kCmaStatusExt, cma_type, type->second,
                            kCmaStatusExtInfo, &prev_cma_stat_ext_);
    }
}

}  // namespace pixel
}  // namespace google
}  // namespace hardware
}  // namespace android
