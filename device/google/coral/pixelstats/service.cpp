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

#include <pixelstats/DropDetect.h>
#include <pixelstats/SysfsCollector.h>
#include <pixelstats/UeventListener.h>

using android::sp;
using android::hardware::google::pixel::DropDetect;
using android::hardware::google::pixel::SysfsCollector;
using android::hardware::google::pixel::UeventListener;

#define MAXIM_DIR(filename) "/sys/class/power_supply/maxfg/" #filename
#define UFSHC_PATH(filename) "/dev/sys/block/bootdevice/" #filename
#define UFSHC_HEALTH_PATH(filename) "/dev/sys/block/bootdevice/health/" #filename
const struct SysfsCollector::SysfsPaths sysfs_paths = {
    .SlowioReadCntPath = UFSHC_PATH(slowio_read_cnt),
    .SlowioWriteCntPath = UFSHC_PATH(slowio_write_cnt),
    .SlowioUnmapCntPath = UFSHC_PATH(slowio_unmap_cnt),
    .SlowioSyncCntPath = UFSHC_PATH(slowio_sync_cnt),
    .CycleCountBinsPath = "/sys/class/power_supply/battery/cycle_counts",
    .ImpedancePath = "/sys/class/misc/msm_cirrus_playback/resistance_left_right",
    .CodecPath =     "/sys/class/iaxxx-dev/iaxxx_misc/codec_state",
    .SpeechDspPath = "/sys/class/iaxxx-dev/iaxxx_misc/wdsp_stat",
    .BatteryCapacityCC = MAXIM_DIR(delta_cc_sum),
    .BatteryCapacityVFSOC = MAXIM_DIR(delta_vfsoc_sum),
    .UFSLifetimeA = UFSHC_HEALTH_PATH(lifetimeA),
    .UFSLifetimeB = UFSHC_HEALTH_PATH(lifetimeB),
    .UFSLifetimeC = UFSHC_HEALTH_PATH(lifetimeC),
    .UFSHostResetPath = UFSHC_PATH(err_stats/err_host_reset),
    .F2fsStatsPath = "/sys/fs/f2fs/",
};

const char *const kAudioUevent = "/kernel/q6audio/q6voice_uevent";
const char *const kSSOCDetailsPath = "/sys/class/power_supply/battery/ssoc_details";

int main() {
    LOG(INFO) << "starting PixelStats";

    sp<DropDetect> dropDetector = DropDetect::start();
    if (!dropDetector) {
        LOG(ERROR) << "Unable to launch drop detection";
        return 1;
    }

    UeventListener ueventListener(kAudioUevent, kSSOCDetailsPath);
    std::thread listenThread(&UeventListener::ListenForever, &ueventListener);
    listenThread.detach();

    SysfsCollector collector(sysfs_paths);
    collector.collect();  // This blocks forever.

    return 0;
}
