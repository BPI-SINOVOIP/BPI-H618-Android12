#
# Copyright (C) 2018 The Android Open-Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

TARGET_BOOTLOADER_BOARD_NAME := barbet
TARGET_SCREEN_DENSITY := 420
TARGET_RECOVERY_UI_MARGIN_HEIGHT := 165
USES_DEVICE_GOOGLE_BARBET := true

include device/google/redbull/BoardConfig-common.mk

# Allow LZ4 compression
BOARD_RAMDISK_USE_LZ4 := true

# Testing related defines
#BOARD_PERFSETUP_SCRIPT := platform_testing/scripts/perf-setup/b9-setup.sh

AUDIO_FEATURE_ENABLED_CS35L41 := false
AUDIO_FEATURE_ENABLED_CS35L41_CALIBRATION_TOOL := false
AUDIO_FEATURE_CONFIG_ENABLE_TAS_SPK_PROT := true
AUDIO_FEATURE_CONFIG_ENABLE_TAS_STEREO := true
AUDIO_FEATURE_CONFIG_ENABLE_TAS_SET_RE_IN_HAL := true

-include vendor/google_devices/$(TARGET_BOOTLOADER_BOARD_NAME)/proprietary/BoardConfigVendor.mk

-include device/google/barbet/soong/pixel_soong_config.mk
