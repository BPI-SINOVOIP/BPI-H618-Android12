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

#include "LearnedCapacityBackupRestore.h"

namespace device {
namespace google {
namespace bonito {
namespace health {

static constexpr char kChgFullDesignFile[] = "sys/class/power_supply/bms/charge_full_design";
static constexpr char kChgFullFile[] = "sys/class/power_supply/bms/charge_full";
static constexpr char kSysCFPersistFile[] = "/mnt/vendor/persist/battery/qcom_charge_full";
static constexpr int kBuffSize = 256;

LearnedCapacityBackupRestore::LearnedCapacityBackupRestore() {}

void LearnedCapacityBackupRestore::Restore() {
    ReadPersistData();
    ReadNominalCapacity();
    ReadCapacity();
    UpdateAndSave();
}

void LearnedCapacityBackupRestore::Backup() {
    ReadCapacity();
    UpdateAndSave();
}

void LearnedCapacityBackupRestore::ReadPersistData() {
    std::string buffer;

    if (!android::base::ReadFileToString(std::string(kSysCFPersistFile), &buffer)) {
        LOG(ERROR) << "Cannot read the storage file";
        return;
    }

    if (sscanf(buffer.c_str(), "%d", &sw_cap_) < 1)
        LOG(ERROR) << "data format is wrong in the storage file: " << buffer;
    else
        LOG(INFO) << "Storage data: " << buffer;
}

void LearnedCapacityBackupRestore::SaveToStorage() {
    char strData[kBuffSize];

    snprintf(strData, kBuffSize, "%d", sw_cap_);

    LOG(INFO) << "Save to Storage: " << strData;

    if (!android::base::WriteStringToFile(strData, std::string(kSysCFPersistFile)))
        LOG(ERROR) << "Write file error: " << strerror(errno);
}

void LearnedCapacityBackupRestore::ReadNominalCapacity() {
    std::string buffer;

    if (!android::base::ReadFileToString(std::string(kChgFullDesignFile), &buffer)) {
        LOG(ERROR) << "Read nominal capacity error: " << strerror(errno);
        return;
    }

    buffer = android::base::Trim(buffer);

    if (sscanf(buffer.c_str(), "%d", &nom_cap_) < 1)
        LOG(ERROR) << "Failed to parse nominal capacity: " << buffer;
    else
        LOG(INFO) << "nominal capacity: " << buffer;
}

void LearnedCapacityBackupRestore::ReadCapacity() {
    std::string buffer;

    if (!android::base::ReadFileToString(std::string(kChgFullFile), &buffer)) {
        LOG(ERROR) << "Read capacity error: " << strerror(errno);
        return;
    }

    buffer = android::base::Trim(buffer);

    if (sscanf(buffer.c_str(), "%d", &hw_cap_) < 1)
        LOG(ERROR) << "Failed to parse capacity: " << buffer;
    else
        LOG(INFO) << "capacity: " << buffer;
}

void LearnedCapacityBackupRestore::SaveToSRAM() {
    char strData[kBuffSize];

    snprintf(strData, kBuffSize, "%d", hw_cap_);

    LOG(INFO) << "Save to SRAM: " << strData;

    if (!android::base::WriteStringToFile(strData, std::string(kChgFullFile)))
        LOG(ERROR) << "Write data error: " << strerror(errno);
}

void LearnedCapacityBackupRestore::UpdateAndSave() {
    bool backup = false;
    bool restore = false;
    if (hw_cap_) {
        if (((hw_cap_ != sw_cap_) && (hw_cap_ != nom_cap_)) ||
            (sw_cap_ == 0)) {
            sw_cap_ = hw_cap_;
            backup = true;
        } else if ((sw_cap_ > 0) && (hw_cap_ == nom_cap_)) {
            hw_cap_ = sw_cap_;
            restore = true;
        }
    }
    if (restore)
        SaveToSRAM();
    if (backup)
        SaveToStorage();
}

}  // namespace health
}  // namespace bonito
}  // namespace google
}  // namespace device
