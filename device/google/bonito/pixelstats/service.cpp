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

#define LOG_TAG "pixelstats"

#include <android-base/logging.h>
#include <utils/StrongPointer.h>

#include <pixelstats/SysfsCollector.h>
#include <pixelstats/DropDetect.h>
#include <pixelstats/UeventListener.h>

using android::sp;
using android::hardware::google::pixel::SysfsCollector;
using android::hardware::google::pixel::DropDetect;
using android::hardware::google::pixel::UeventListener;

const struct SysfsCollector::SysfsPaths sysfs_paths = {
    .SlowioReadCntPath = "/sys/devices/platform/soc/7c4000.sdhci/mmc_host/mmc0/slowio_read_cnt",
    .SlowioWriteCntPath = "/sys/devices/platform/soc/7c4000.sdhci/mmc_host/mmc0/slowio_write_cnt",
    .SlowioUnmapCntPath =
        "/sys/devices/platform/soc/7c4000.sdhci/mmc_host/mmc0/slowio_discard_cnt",
    .SlowioSyncCntPath = "/sys/devices/platform/soc/7c4000.sdhci/mmc_host/mmc0/slowio_flush_cnt",
    .CycleCountBinsPath = "/sys/class/power_supply/bms/device/cycle_counts_bins",
    .ImpedancePath = "/sys/class/misc/msm_cirrus_playback/resistance_left_right",
    .CodecPath =
        "/sys/devices/platform/soc/c440000.qcom,spmi/spmi-0/spmi0-03/"
        "c440000.qcom,spmi:qcom,pm660l@3:analog-codec@f000/codec_state",
    .Codec1Path =
        "/sys/devices/platform/soc/a88000.i2c/i2c-0/0-0057/codec_state",
    .F2fsStatsPath = "/sys/fs/f2fs/",
};

int main() {
    LOG(INFO) << "starting PixelStats";

    sp<DropDetect> dropDetector = DropDetect::start();
    if (!dropDetector) {
        LOG(ERROR) << "Unable to launch drop detection";
        return 1;
    }

    UeventListener ueventListener("");
    std::thread listenThread(&UeventListener::ListenForever, &ueventListener);
    listenThread.detach();

    SysfsCollector collector(sysfs_paths);
    collector.collect();  // This blocks forever.

    return 0;
}
