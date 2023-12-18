/*
 * Copyright 2020 The Android Open Source Project
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

#include "ProcPidStat.h"

#include "ProcPidDir.h"

#include <android-base/file.h>
#include <android-base/stringprintf.h>
#include <gmock/gmock.h>
#include <inttypes.h>

#include <algorithm>
#include <string>

namespace android {
namespace automotive {
namespace watchdog {

using ::android::automotive::watchdog::testing::populateProcPidDir;
using ::android::base::StringAppendF;
using ::android::base::StringPrintf;

namespace {

std::string toString(const PidStat& stat) {
    return StringPrintf("PID: %" PRIu32 ", PPID: %" PRIu32 ", Comm: %s, State: %s, "
                        "Major page faults: %" PRIu64 ", Num threads: %" PRIu32
                        ", Start time: %" PRIu64,
                        stat.pid, stat.ppid, stat.comm.c_str(), stat.state.c_str(),
                        stat.majorFaults, stat.numThreads, stat.startTime);
}

std::string toString(const ProcessStats& stats) {
    std::string buffer;
    StringAppendF(&buffer,
                  "Tgid: %" PRIi64 ", UID: %" PRIi64 ", VmPeak: %" PRIu64 ", VmSize: %" PRIu64
                  ", VmHWM: %" PRIu64 ", VmRSS: %" PRIu64 ", %s\n",
                  stats.tgid, stats.uid, stats.vmPeakKb, stats.vmSizeKb, stats.vmHwmKb,
                  stats.vmRssKb, toString(stats.process).c_str());
    StringAppendF(&buffer, "\tThread stats:\n");
    for (const auto& it : stats.threads) {
        StringAppendF(&buffer, "\t\t%s\n", toString(it.second).c_str());
    }
    StringAppendF(&buffer, "\n");
    return buffer;
}

std::string toString(const std::vector<ProcessStats>& stats) {
    std::string buffer;
    StringAppendF(&buffer, "Number of processes: %d\n", static_cast<int>(stats.size()));
    for (const auto& it : stats) {
        StringAppendF(&buffer, "%s", toString(it).c_str());
    }
    return buffer;
}

bool isEqual(const PidStat& lhs, const PidStat& rhs) {
    return lhs.pid == rhs.pid && lhs.comm == rhs.comm && lhs.state == rhs.state &&
            lhs.ppid == rhs.ppid && lhs.majorFaults == rhs.majorFaults &&
            lhs.numThreads == rhs.numThreads && lhs.startTime == rhs.startTime;
}

bool isEqual(std::vector<ProcessStats>* lhs, std::vector<ProcessStats>* rhs) {
    if (lhs->size() != rhs->size()) {
        return false;
    }
    std::sort(lhs->begin(), lhs->end(), [&](const ProcessStats& l, const ProcessStats& r) -> bool {
        return l.process.pid < r.process.pid;
    });
    std::sort(rhs->begin(), rhs->end(), [&](const ProcessStats& l, const ProcessStats& r) -> bool {
        return l.process.pid < r.process.pid;
    });
    return std::equal(lhs->begin(), lhs->end(), rhs->begin(),
                      [&](const ProcessStats& l, const ProcessStats& r) -> bool {
                          if (l.tgid != r.tgid || l.uid != r.uid || l.vmPeakKb != r.vmPeakKb ||
                              l.vmSizeKb != r.vmSizeKb || l.vmHwmKb != r.vmHwmKb ||
                              l.vmRssKb != r.vmRssKb || !isEqual(l.process, r.process) ||
                              l.threads.size() != r.threads.size()) {
                              return false;
                          }
                          for (const auto& lIt : l.threads) {
                              const auto& rIt = r.threads.find(lIt.first);
                              if (rIt == r.threads.end()) {
                                  return false;
                              }
                              if (!isEqual(lIt.second, rIt->second)) {
                                  return false;
                              }
                          }
                          return true;
                      });
}

std::string pidStatusStr(pid_t pid, uid_t uid) {
    return StringPrintf("Pid:\t%" PRIu32 "\nTgid:\t%" PRIu32 "\nUid:\t%" PRIu32 "\n", pid, pid,
                        uid);
}

std::string pidStatusStr(pid_t pid, uid_t uid, uint64_t vmPeakKb, uint64_t vmSizeKb,
                         uint64_t vmHwmKb, uint64_t vmRssKb) {
    return StringPrintf("%sVmPeak:\t%" PRIu64 "\nVmSize:\t%" PRIu64 "\nVmHWM:\t%" PRIu64
                        "\nVmRSS:\t%" PRIu64 "\n",
                        pidStatusStr(pid, uid).c_str(), vmPeakKb, vmSizeKb, vmHwmKb, vmRssKb);
}

}  // namespace

TEST(ProcPidStatTest, TestValidStatFiles) {
    std::unordered_map<pid_t, std::vector<pid_t>> pidToTids = {
            {1, {1, 453}},
            {1000, {1000, 1100}},
    };

    std::unordered_map<pid_t, std::string> perProcessStat = {
            {1, "1 (init) S 0 0 0 0 0 0 0 0 220 0 0 0 0 0 0 0 2 0 0\n"},
            {1000, "1000 (system_server) R 1 0 0 0 0 0 0 0 600 0 0 0 0 0 0 0 2 0 1000\n"},
    };

    std::unordered_map<pid_t, std::string> perProcessStatus = {
            {1, pidStatusStr(1, 0, 123, 456, 789, 345)},
            {1000, pidStatusStr(1000, 10001234, 234, 567, 890, 123)},
    };

    std::unordered_map<pid_t, std::string> perThreadStat = {
            {1, "1 (init) S 0 0 0 0 0 0 0 0 200 0 0 0 0 0 0 0 2 0 0\n"},
            {453, "453 (init) S 0 0 0 0 0 0 0 0 20 0 0 0 0 0 0 0 2 0 275\n"},
            {1000, "1000 (system_server) R 1 0 0 0 0 0 0 0 250 0 0 0 0 0 0 0 2 0 1000\n"},
            {1100, "1100 (system_server) S 1 0 0 0 0 0 0 0 350 0 0 0 0 0 0 0 2 0 1200\n"},
    };

    std::vector<ProcessStats> expected = {
            {.tgid = 1,
             .uid = 0,
             .vmPeakKb = 123,
             .vmSizeKb = 456,
             .vmHwmKb = 789,
             .vmRssKb = 345,
             .process = {1, "init", "S", 0, 220, 2, 0},
             .threads = {{1, {1, "init", "S", 0, 200, 2, 0}},
                         {453, {453, "init", "S", 0, 20, 2, 275}}}},
            {.tgid = 1000,
             .uid = 10001234,
             .vmPeakKb = 234,
             .vmSizeKb = 567,
             .vmHwmKb = 890,
             .vmRssKb = 123,
             .process = {1000, "system_server", "R", 1, 600, 2, 1000},
             .threads = {{1000, {1000, "system_server", "R", 1, 250, 2, 1000}},
                         {1100, {1100, "system_server", "S", 1, 350, 2, 1200}}}},
    };

    TemporaryDir firstSnapshot;
    ASSERT_RESULT_OK(populateProcPidDir(firstSnapshot.path, pidToTids, perProcessStat,
                                        perProcessStatus, perThreadStat));

    ProcPidStat procPidStat(firstSnapshot.path);
    ASSERT_TRUE(procPidStat.enabled())
            << "Files under the path `" << firstSnapshot.path << "` are inaccessible";
    ASSERT_RESULT_OK(procPidStat.collect());

    auto actual = std::vector<ProcessStats>(procPidStat.deltaStats());
    EXPECT_TRUE(isEqual(&expected, &actual)) << "First snapshot doesn't match.\nExpected:\n"
                                             << toString(expected) << "\nActual:\n"
                                             << toString(actual);
    pidToTids = {
            {1, {1, 453}}, {1000, {1000, 1400}},  // TID 1100 terminated and 1400 instantiated.
    };

    perProcessStat = {
            {1, "1 (init) S 0 0 0 0 0 0 0 0 920 0 0 0 0 0 0 0 2 0 0\n"},
            {1000, "1000 (system_server) R 1 0 0 0 0 0 0 0 1550 0 0 0 0 0 0 0 2 0 1000\n"},
    };

    perThreadStat = {
            {1, "1 (init) S 0 0 0 0 0 0 0 0 600 0 0 0 0 0 0 0 2 0 0\n"},
            {453, "453 (init) S 0 0 0 0 0 0 0 0 320 0 0 0 0 0 0 0 2 0 275\n"},
            {1000, "1000 (system_server) R 1 0 0 0 0 0 0 0 600 0 0 0 0 0 0 0 2 0 1000\n"},
            // TID 1100 hits +400 major page faults before terminating. This is counted against
            // PID 1000's perProcessStat.
            {1400, "1400 (system_server) S 1 0 0 0 0 0 0 0 200 0 0 0 0 0 0 0 2 0 8977476\n"},
    };

    expected = {
            {.tgid = 1,
             .uid = 0,
             .vmPeakKb = 123,
             .vmSizeKb = 456,
             .vmHwmKb = 789,
             .vmRssKb = 345,
             .process = {1, "init", "S", 0, 700, 2, 0},
             .threads = {{1, {1, "init", "S", 0, 400, 2, 0}},
                         {453, {453, "init", "S", 0, 300, 2, 275}}}},
            {.tgid = 1000,
             .uid = 10001234,
             .vmPeakKb = 234,
             .vmSizeKb = 567,
             .vmHwmKb = 890,
             .vmRssKb = 123,
             .process = {1000, "system_server", "R", 1, 950, 2, 1000},
             .threads = {{1000, {1000, "system_server", "R", 1, 350, 2, 1000}},
                         {1400, {1400, "system_server", "S", 1, 200, 2, 8977476}}}},
    };

    TemporaryDir secondSnapshot;
    ASSERT_RESULT_OK(populateProcPidDir(secondSnapshot.path, pidToTids, perProcessStat,
                                        perProcessStatus, perThreadStat));

    procPidStat.mPath = secondSnapshot.path;
    ASSERT_TRUE(procPidStat.enabled())
            << "Files under the path `" << secondSnapshot.path << "` are inaccessible";
    ASSERT_RESULT_OK(procPidStat.collect());

    actual = std::vector<ProcessStats>(procPidStat.deltaStats());
    EXPECT_TRUE(isEqual(&expected, &actual)) << "Second snapshot doesn't match.\nExpected:\n"
                                             << toString(expected) << "\nActual:\n"
                                             << toString(actual);
}

TEST(ProcPidStatTest, TestHandlesProcessTerminationBetweenScanningAndParsing) {
    std::unordered_map<pid_t, std::vector<pid_t>> pidToTids = {
            {1, {1}},
            {100, {100}},          // Process terminates after scanning PID directory.
            {1000, {1000}},        // Process terminates after reading stat file.
            {2000, {2000}},        // Process terminates after scanning task directory.
            {3000, {3000, 3300}},  // TID 3300 terminates after scanning task directory.
    };

    std::unordered_map<pid_t, std::string> perProcessStat = {
            {1, "1 (init) S 0 0 0 0 0 0 0 0 220 0 0 0 0 0 0 0 1 0 0\n"},
            // Process 100 terminated.
            {1000, "1000 (system_server) R 1 0 0 0 0 0 0 0 600 0 0 0 0 0 0 0 1 0 1000\n"},
            {2000, "2000 (logd) R 1 0 0 0 0 0 0 0 1200 0 0 0 0 0 0 0 1 0 4567\n"},
            {3000, "3000 (disk I/O) R 1 0 0 0 0 0 0 0 10300 0 0 0 0 0 0 0 2 0 67890\n"},
    };

    std::unordered_map<pid_t, std::string> perProcessStatus = {
            {1, "Pid:\t1\nTgid:\t1\nUid:\t0\t0\t0\t0\n"},
            // Process 1000 terminated.
            {2000, pidStatusStr(2000, 10001234)},
            {3000, pidStatusStr(3000, 10001234)},
    };

    std::unordered_map<pid_t, std::string> perThreadStat = {
            {1, "1 (init) S 0 0 0 0 0 0 0 0 200 0 0 0 0 0 0 0 1 0 0\n"},
            // Process 2000 terminated.
            {3000, "3000 (disk I/O) R 1 0 0 0 0 0 0 0 2400 0 0 0 0 0 0 0 2 0 67890\n"},
            // TID 3300 terminated.
    };

    std::vector<ProcessStats> expected = {
            {.tgid = 1,
             .uid = 0,
             .process = {1, "init", "S", 0, 220, 1, 0},
             .threads = {{1, {1, "init", "S", 0, 200, 1, 0}}}},
            {.tgid = -1,
             .uid = -1,
             .process = {1000, "system_server", "R", 1, 600, 1, 1000},
             // Stats common between process and main-thread are copied when
             // main-thread stats are not available.
             .threads = {{1000, {1000, "system_server", "R", 1, 0, 1, 1000}}}},
            {.tgid = 2000,
             .uid = 10001234,
             .process = {2000, "logd", "R", 1, 1200, 1, 4567},
             .threads = {{2000, {2000, "logd", "R", 1, 0, 1, 4567}}}},
            {.tgid = 3000,
             .uid = 10001234,
             .process = {3000, "disk I/O", "R", 1, 10300, 2, 67890},
             .threads = {{3000, {3000, "disk I/O", "R", 1, 2400, 2, 67890}}}},
    };

    TemporaryDir procDir;
    ASSERT_RESULT_OK(populateProcPidDir(procDir.path, pidToTids, perProcessStat, perProcessStatus,
                                        perThreadStat));

    ProcPidStat procPidStat(procDir.path);
    ASSERT_TRUE(procPidStat.enabled())
            << "Files under the path `" << procDir.path << "` are inaccessible";
    ASSERT_RESULT_OK(procPidStat.collect());

    auto actual = std::vector<ProcessStats>(procPidStat.deltaStats());
    EXPECT_TRUE(isEqual(&expected, &actual)) << "Proc pid contents doesn't match.\nExpected:\n"
                                             << toString(expected) << "\nActual:\n"
                                             << toString(actual);
}

TEST(ProcPidStatTest, TestHandlesPidTidReuse) {
    std::unordered_map<pid_t, std::vector<pid_t>> pidToTids = {
            {1, {1, 367, 453, 589}},
            {1000, {1000}},
            {2345, {2345}},
    };

    std::unordered_map<pid_t, std::string> perProcessStat = {
            {1, "1 (init) S 0 0 0 0 0 0 0 0 1200 0 0 0 0 0 0 0 4 0 0\n"},
            {1000, "1000 (system_server) R 1 0 0 0 0 0 0 0 250 0 0 0 0 0 0 0 1 0 1000\n"},
            {2345, "2345 (logd) R 1 0 0 0 0 0 0 0 54354 0 0 0 0 0 0 0 1 0 456\n"},
    };

    std::unordered_map<pid_t, std::string> perProcessStatus = {
            {1, pidStatusStr(1, 0)},
            {1000, pidStatusStr(1000, 10001234)},
            {2345, pidStatusStr(2345, 10001234)},
    };

    std::unordered_map<pid_t, std::string> perThreadStat = {
            {1, "1 (init) S 0 0 0 0 0 0 0 0 200 0 0 0 0 0 0 0 4 0 0\n"},
            {367, "367 (init) S 0 0 0 0 0 0 0 0 400 0 0 0 0 0 0 0 4 0 100\n"},
            {453, "453 (init) S 0 0 0 0 0 0 0 0 100 0 0 0 0 0 0 0 4 0 275\n"},
            {589, "589 (init) S 0 0 0 0 0 0 0 0 500 0 0 0 0 0 0 0 4 0 600\n"},
            {1000, "1000 (system_server) R 1 0 0 0 0 0 0 0 250 0 0 0 0 0 0 0 1 0 1000\n"},
            {2345, "2345 (logd) R 1 0 0 0 0 0 0 0 54354 0 0 0 0 0 0 0 1 0 456\n"},
    };

    std::vector<ProcessStats> expected = {
            {.tgid = 1,
             .uid = 0,
             .process = {1, "init", "S", 0, 1200, 4, 0},
             .threads = {{1, {1, "init", "S", 0, 200, 4, 0}},
                         {367, {367, "init", "S", 0, 400, 4, 100}},
                         {453, {453, "init", "S", 0, 100, 4, 275}},
                         {589, {589, "init", "S", 0, 500, 4, 600}}}},
            {.tgid = 1000,
             .uid = 10001234,
             .process = {1000, "system_server", "R", 1, 250, 1, 1000},
             .threads = {{1000, {1000, "system_server", "R", 1, 250, 1, 1000}}}},
            {.tgid = 2345,
             .uid = 10001234,
             .process = {2345, "logd", "R", 1, 54354, 1, 456},
             .threads = {{2345, {2345, "logd", "R", 1, 54354, 1, 456}}}},
    };

    TemporaryDir firstSnapshot;
    ASSERT_RESULT_OK(populateProcPidDir(firstSnapshot.path, pidToTids, perProcessStat,
                                        perProcessStatus, perThreadStat));

    ProcPidStat procPidStat(firstSnapshot.path);
    ASSERT_TRUE(procPidStat.enabled())
            << "Files under the path `" << firstSnapshot.path << "` are inaccessible";
    ASSERT_RESULT_OK(procPidStat.collect());

    auto actual = std::vector<ProcessStats>(procPidStat.deltaStats());
    EXPECT_TRUE(isEqual(&expected, &actual)) << "First snapshot doesn't match.\nExpected:\n"
                                             << toString(expected) << "\nActual:\n"
                                             << toString(actual);

    pidToTids = {
            {1, {1, 589}},       // TID 589 reused by the same process.
            {367, {367, 2000}},  // TID 367 reused as a PID. PID 2000 reused as a TID.
            // PID 1000 reused as a new PID. TID 453 reused by a different PID.
            {1000, {1000, 453}},
    };

    perProcessStat = {
            {1, "1 (init) S 0 0 0 0 0 0 0 0 1800 0 0 0 0 0 0 0 2 0 0\n"},
            {367, "367 (system_server) R 1 0 0 0 0 0 0 0 100 0 0 0 0 0 0 0 2 0 3450\n"},
            {1000, "1000 (logd) R 1 0 0 0 0 0 0 0 2000 0 0 0 0 0 0 0 2 0 4650\n"},
    };

    perProcessStatus = {
            {1, pidStatusStr(1, 0)},
            {367, pidStatusStr(367, 10001234)},
            {1000, pidStatusStr(1000, 10001234)},
    };

    perThreadStat = {
            {1, "1 (init) S 0 0 0 0 0 0 0 0 500 0 0 0 0 0 0 0 2 0 0\n"},
            {589, "589 (init) S 0 0 0 0 0 0 0 0 300 0 0 0 0 0 0 0 2 0 2345\n"},
            {367, "367 (system_server) R 1 0 0 0 0 0 0 0 50 0 0 0 0 0 0 0 2 0 3450\n"},
            {2000, "2000 (system_server) R 1 0 0 0 0 0 0 0 50 0 0 0 0 0 0 0 2 0 3670\n"},
            {1000, "1000 (logd) R 1 0 0 0 0 0 0 0 200 0 0 0 0 0 0 0 2 0 4650\n"},
            {453, "453 (logd) D 1 0 0 0 0 0 0 0 1800 0 0 0 0 0 0 0 2 0 4770\n"},
    };

    expected = {
            {.tgid = 1,
             .uid = 0,
             .process = {1, "init", "S", 0, 600, 2, 0},
             .threads = {{1, {1, "init", "S", 0, 300, 2, 0}},
                         {589, {589, "init", "S", 0, 300, 2, 2345}}}},
            {.tgid = 367,
             .uid = 10001234,
             .process = {367, "system_server", "R", 1, 100, 2, 3450},
             .threads = {{367, {367, "system_server", "R", 1, 50, 2, 3450}},
                         {2000, {2000, "system_server", "R", 1, 50, 2, 3670}}}},
            {.tgid = 1000,
             .uid = 10001234,
             .process = {1000, "logd", "R", 1, 2000, 2, 4650},
             .threads = {{1000, {1000, "logd", "R", 1, 200, 2, 4650}},
                         {453, {453, "logd", "D", 1, 1800, 2, 4770}}}},
    };

    TemporaryDir secondSnapshot;
    ASSERT_RESULT_OK(populateProcPidDir(secondSnapshot.path, pidToTids, perProcessStat,
                                        perProcessStatus, perThreadStat));

    procPidStat.mPath = secondSnapshot.path;
    ASSERT_TRUE(procPidStat.enabled())
            << "Files under the path `" << secondSnapshot.path << "` are inaccessible";
    ASSERT_RESULT_OK(procPidStat.collect());

    actual = std::vector<ProcessStats>(procPidStat.deltaStats());
    EXPECT_TRUE(isEqual(&expected, &actual)) << "Second snapshot doesn't match.\nExpected:\n"
                                             << toString(expected) << "\nActual:\n"
                                             << toString(actual);
}

TEST(ProcPidStatTest, TestErrorOnCorruptedProcessStatFile) {
    std::unordered_map<pid_t, std::vector<pid_t>> pidToTids = {
            {1, {1}},
    };

    std::unordered_map<pid_t, std::string> perProcessStat = {
            {1, "1 (init) S 0 0 0 0 0 0 0 0 200 0 0 0 CORRUPTED DATA\n"},
    };

    std::unordered_map<pid_t, std::string> perProcessStatus = {
            {1, pidStatusStr(1, 0)},
    };

    std::unordered_map<pid_t, std::string> perThreadStat = {
            {1, "1 (init) S 0 0 0 0 0 0 0 0 200 0 0 0 0 0 0 0 1 0 0\n"},
    };

    TemporaryDir procDir;
    ASSERT_RESULT_OK(populateProcPidDir(procDir.path, pidToTids, perProcessStat, perProcessStatus,
                                        perThreadStat));

    ProcPidStat procPidStat(procDir.path);
    ASSERT_TRUE(procPidStat.enabled())
            << "Files under the path `" << procDir.path << "` are inaccessible";
    ASSERT_FALSE(procPidStat.collect().ok()) << "No error returned for invalid process stat file";
}

TEST(ProcPidStatTest, TestErrorOnCorruptedProcessStatusFile) {
    std::unordered_map<pid_t, std::vector<pid_t>> pidToTids = {
            {1, {1}},
    };

    std::unordered_map<pid_t, std::string> perProcessStat = {
            {1, "1 (init) S 0 0 0 0 0 0 0 0 200 0 0 0 0 0 0 0 1 0 0\n"},
    };

    std::unordered_map<pid_t, std::string> perProcessStatus = {
            {1, "Pid:\t1\nTgid:\t1\nCORRUPTED DATA\n"},
    };

    std::unordered_map<pid_t, std::string> perThreadStat = {
            {1, "1 (init) S 0 0 0 0 0 0 0 0 200 0 0 0 0 0 0 0 1 0 0\n"},
    };

    TemporaryDir procDir;
    ASSERT_RESULT_OK(populateProcPidDir(procDir.path, pidToTids, perProcessStat, perProcessStatus,
                                        perThreadStat));

    ProcPidStat procPidStat(procDir.path);
    ASSERT_TRUE(procPidStat.enabled())
            << "Files under the path `" << procDir.path << "` are inaccessible";
    ASSERT_FALSE(procPidStat.collect().ok()) << "No error returned for invalid process status file";
}

TEST(ProcPidStatTest, TestErrorOnCorruptedThreadStatFile) {
    std::unordered_map<pid_t, std::vector<pid_t>> pidToTids = {
            {1, {1}},
    };

    std::unordered_map<pid_t, std::string> perProcessStat = {
            {1, "1 (init) S 0 0 0 0 0 0 0 0 200 0 0 0 0 0 0 0 1 0 0\n"},
    };

    std::unordered_map<pid_t, std::string> perProcessStatus = {
            {1, pidStatusStr(1, 0)},
    };

    std::unordered_map<pid_t, std::string> perThreadStat = {
            {1, "1 (init) S 0 0 0 0 0 0 0 0 200 0 0 0 CORRUPTED DATA\n"},
    };

    TemporaryDir procDir;
    ASSERT_RESULT_OK(populateProcPidDir(procDir.path, pidToTids, perProcessStat, perProcessStatus,
                                        perThreadStat));

    ProcPidStat procPidStat(procDir.path);
    ASSERT_TRUE(procPidStat.enabled())
            << "Files under the path `" << procDir.path << "` are inaccessible";
    ASSERT_FALSE(procPidStat.collect().ok()) << "No error returned for invalid thread stat file";
}

TEST(ProcPidStatTest, TestHandlesSpaceInCommName) {
    std::unordered_map<pid_t, std::vector<pid_t>> pidToTids = {
            {1, {1}},
    };

    std::unordered_map<pid_t, std::string> perProcessStat = {
            {1, "1 (random process name with space) S 0 0 0 0 0 0 0 0 200 0 0 0 0 0 0 0 1 0 0\n"},
    };

    std::unordered_map<pid_t, std::string> perProcessStatus = {
            {1, pidStatusStr(1, 0)},
    };

    std::unordered_map<pid_t, std::string> perThreadStat = {
            {1, "1 (random process name with space) S 0 0 0 0 0 0 0 0 200 0 0 0 0 0 0 0 1 0 0\n"},
    };

    std::vector<ProcessStats> expected = {
            {.tgid = 1,
             .uid = 0,
             .process = {1, "random process name with space", "S", 0, 200, 1, 0},
             .threads = {{1, {1, "random process name with space", "S", 0, 200, 1, 0}}}},
    };

    TemporaryDir procDir;
    ASSERT_RESULT_OK(populateProcPidDir(procDir.path, pidToTids, perProcessStat, perProcessStatus,
                                        perThreadStat));

    ProcPidStat procPidStat(procDir.path);
    ASSERT_TRUE(procPidStat.enabled())
            << "Files under the path `" << procDir.path << "` are inaccessible";
    ASSERT_RESULT_OK(procPidStat.collect());

    auto actual = std::vector<ProcessStats>(procPidStat.deltaStats());
    EXPECT_TRUE(isEqual(&expected, &actual)) << "Proc pid contents doesn't match.\nExpected:\n"
                                             << toString(expected) << "\nActual:\n"
                                             << toString(actual);
}

TEST(ProcPidStatTest, TestProcPidStatContentsFromDevice) {
    ProcPidStat procPidStat;
    ASSERT_TRUE(procPidStat.enabled()) << "/proc/[pid]/.* files are inaccessible";
    ASSERT_RESULT_OK(procPidStat.collect());

    const auto& processStats = procPidStat.deltaStats();
    // The below check should pass because there should be at least one process.
    EXPECT_GT(processStats.size(), 0);
}

}  // namespace watchdog
}  // namespace automotive
}  // namespace android
