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

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <pixelhealth/StatsHelper.h>

#define LOG_TAG "pixelhealth-vendor"

#include <utils/Log.h>

namespace hardware {
namespace google {
namespace pixel {
namespace health {

using aidl::android::frameworks::stats::VendorAtom;
using aidl::android::frameworks::stats::VendorAtomValue;
namespace PixelAtoms = android::hardware::google::pixel::PixelAtoms;

std::shared_ptr<IStats> getStatsService() {
    const std::string instance = std::string() + IStats::descriptor + "/default";
    static bool isStatsDeclared = false;
    if (!isStatsDeclared) {
        // It is good to cache the result - it would not be changed
        isStatsDeclared = AServiceManager_isDeclared(instance.c_str());
        if (!isStatsDeclared) {
            LOG(ERROR) << "Stats service is not registered.";
            return nullptr;
        }
    }
    /* TODO stayfan@: b/187221893 Review implementing separate thread to log atoms
     * to prevent data loss at device boot stage, while IStats might not be ready
     */
    return IStats::fromBinder(ndk::SpAIBinder(AServiceManager_getService(instance.c_str())));
}

void reportBatteryHealthSnapshot(const std::shared_ptr<IStats> &stats_client,
                                 const VendorBatteryHealthSnapshot &batteryHealthSnapshot) {
    // Load values array
    std::vector<VendorAtomValue> values(7);
    VendorAtomValue tmp;
    tmp.set<VendorAtomValue::intValue>(batteryHealthSnapshot.type());
    values[0] = tmp;
    tmp.set<VendorAtomValue::intValue>(batteryHealthSnapshot.temperature_deci_celsius());
    values[1] = tmp;
    tmp.set<VendorAtomValue::intValue>(batteryHealthSnapshot.voltage_micro_volt());
    values[2] = tmp;
    tmp.set<VendorAtomValue::intValue>(batteryHealthSnapshot.current_micro_amps());
    values[3] = tmp;
    tmp.set<VendorAtomValue::intValue>(batteryHealthSnapshot.open_circuit_micro_volt());
    values[4] = tmp;
    tmp.set<VendorAtomValue::intValue>(batteryHealthSnapshot.resistance_micro_ohm());
    values[5] = tmp;
    tmp.set<VendorAtomValue::intValue>(batteryHealthSnapshot.level_percent());
    values[6] = tmp;

    // Send vendor atom to IStats HAL
    VendorAtom event = {.reverseDomainName = PixelAtoms::ReverseDomainNames().pixel(),
                        .atomId = PixelAtoms::Atom::kVendorBatteryHealthSnapshot,
                        .values = std::move(values)};
    const ndk::ScopedAStatus ret = stats_client->reportVendorAtom(event);
    if (!ret.isOk())
        LOG(ERROR) << "Unable to report VendorBatteryHealthSnapshot to IStats service";
}

void reportBatteryCausedShutdown(const std::shared_ptr<IStats> &stats_client,
                                 const VendorBatteryCausedShutdown &batteryCausedShutdown) {
    // Load values array
    std::vector<VendorAtomValue> values(1);
    VendorAtomValue tmp;
    tmp.set<VendorAtomValue::intValue>(batteryCausedShutdown.last_recorded_micro_volt());
    values[0] = tmp;

    // Send vendor atom to IStats HAL
    VendorAtom event = {.reverseDomainName = PixelAtoms::ReverseDomainNames().pixel(),
                        .atomId = PixelAtoms::Atom::kVendorBatteryCausedShutdown,
                        .values = std::move(values)};
    const ndk::ScopedAStatus ret = stats_client->reportVendorAtom(event);
    if (!ret.isOk())
        LOG(ERROR) << "Unable to report VendorBatteryHealthSnapshot to IStats service";
}

}  // namespace health
}  // namespace pixel
}  // namespace google
}  // namespace hardware
