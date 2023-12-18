/**
 * Copyright (c) 2020, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "carwatchdogd"

#include "UidIoStats.h"

#include <android-base/file.h>
#include <android-base/parseint.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <log/log.h>

#include <inttypes.h>

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace android {
namespace automotive {
namespace watchdog {

using ::android::base::Error;
using ::android::base::ParseInt;
using ::android::base::ParseUint;
using ::android::base::ReadFileToString;
using ::android::base::Result;
using ::android::base::Split;
using ::android::base::StringPrintf;

namespace {

bool parseUidIoStats(const std::string& data, UidIoUsage* usage) {
    std::vector<std::string> fields = Split(data, " ");
    if (fields.size() < 11 || !ParseUint(fields[0], &usage->uid) ||
        !ParseInt(fields[3], &usage->ios.metrics[READ_BYTES][FOREGROUND]) ||
        !ParseInt(fields[4], &usage->ios.metrics[WRITE_BYTES][FOREGROUND]) ||
        !ParseInt(fields[7], &usage->ios.metrics[READ_BYTES][BACKGROUND]) ||
        !ParseInt(fields[8], &usage->ios.metrics[WRITE_BYTES][BACKGROUND]) ||
        !ParseInt(fields[9], &usage->ios.metrics[FSYNC_COUNT][FOREGROUND]) ||
        !ParseInt(fields[10], &usage->ios.metrics[FSYNC_COUNT][BACKGROUND])) {
        ALOGW("Invalid uid I/O stats: \"%s\"", data.c_str());
        return false;
    }
    return true;
}

int64_t maybeDiff(int64_t lhs, int64_t rhs) {
    return lhs > rhs ? lhs - rhs : 0;
}

}  // namespace

IoUsage& IoUsage::operator-=(const IoUsage& rhs) {
    metrics[READ_BYTES][FOREGROUND] =
            maybeDiff(metrics[READ_BYTES][FOREGROUND], rhs.metrics[READ_BYTES][FOREGROUND]);
    metrics[READ_BYTES][BACKGROUND] =
            maybeDiff(metrics[READ_BYTES][BACKGROUND], rhs.metrics[READ_BYTES][BACKGROUND]);
    metrics[WRITE_BYTES][FOREGROUND] =
            maybeDiff(metrics[WRITE_BYTES][FOREGROUND], rhs.metrics[WRITE_BYTES][FOREGROUND]);
    metrics[WRITE_BYTES][BACKGROUND] =
            maybeDiff(metrics[WRITE_BYTES][BACKGROUND], rhs.metrics[WRITE_BYTES][BACKGROUND]);
    metrics[FSYNC_COUNT][FOREGROUND] =
            maybeDiff(metrics[FSYNC_COUNT][FOREGROUND], rhs.metrics[FSYNC_COUNT][FOREGROUND]);
    metrics[FSYNC_COUNT][BACKGROUND] =
            maybeDiff(metrics[FSYNC_COUNT][BACKGROUND], rhs.metrics[FSYNC_COUNT][BACKGROUND]);
    return *this;
}

bool IoUsage::isZero() const {
    for (int i = 0; i < METRIC_TYPES; i++) {
        for (int j = 0; j < UID_STATES; j++) {
            if (metrics[i][j]) {
                return false;
            }
        }
    }
    return true;
}

std::string IoUsage::toString() const {
    return StringPrintf("FgRdBytes:%" PRIi64 " BgRdBytes:%" PRIi64 " FgWrBytes:%" PRIi64
                        " BgWrBytes:%" PRIi64 " FgFsync:%" PRIi64 " BgFsync:%" PRIi64,
                        metrics[READ_BYTES][FOREGROUND], metrics[READ_BYTES][BACKGROUND],
                        metrics[WRITE_BYTES][FOREGROUND], metrics[WRITE_BYTES][BACKGROUND],
                        metrics[FSYNC_COUNT][FOREGROUND], metrics[FSYNC_COUNT][BACKGROUND]);
}

Result<void> UidIoStats::collect() {
    if (!kEnabled) {
        return Error() << "Can not access " << kPath;
    }

    Mutex::Autolock lock(mMutex);
    const auto& uidIoUsages = getUidIoUsagesLocked();
    if (!uidIoUsages.ok() || uidIoUsages->empty()) {
        return Error() << "Failed to get UID IO stats: " << uidIoUsages.error();
    }

    mDeltaUidIoUsages.clear();
    for (const auto& it : *uidIoUsages) {
        UidIoUsage curUsage = it.second;
        if (curUsage.ios.isZero()) {
            continue;
        }
        if (mLatestUidIoUsages.find(it.first) != mLatestUidIoUsages.end()) {
            if (curUsage -= mLatestUidIoUsages[it.first]; curUsage.ios.isZero()) {
                continue;
            }
        }
        mDeltaUidIoUsages[it.first] = curUsage;
    }
    mLatestUidIoUsages = *uidIoUsages;
    return {};
}

Result<std::unordered_map<uid_t, UidIoUsage>> UidIoStats::getUidIoUsagesLocked() const {
    std::string buffer;
    if (!ReadFileToString(kPath, &buffer)) {
        return Error() << "ReadFileToString failed for " << kPath;
    }

    std::vector<std::string> ioStats = Split(std::move(buffer), "\n");
    std::unordered_map<uid_t, UidIoUsage> uidIoUsages;
    UidIoUsage usage;
    for (size_t i = 0; i < ioStats.size(); i++) {
        if (ioStats[i].empty() || !ioStats[i].compare(0, 4, "task")) {
            // Skip per-task stats as CONFIG_UID_SYS_STATS_DEBUG is not set in the kernel and
            // the collected data is aggregated only per-UID.
            continue;
        }
        if (!parseUidIoStats(std::move(ioStats[i]), &usage)) {
            return Error() << "Failed to parse the contents of " << kPath;
        }
        uidIoUsages[usage.uid] = usage;
    }
    return uidIoUsages;
}

}  // namespace watchdog
}  // namespace automotive
}  // namespace android
