#
# Copyright (C) 2016 The Android Open-Source Project
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

include build/make/target/board/BoardConfigMainlineCommon.mk
include build/make/target/board/BoardConfigPixelCommon.mk

TARGET_BOARD_PLATFORM := sdm710
TARGET_BOARD_INFO_FILE := device/google/bonito/board-info.txt
USES_DEVICE_GOOGLE_B4S4 := true

TARGET_ARCH := arm64
TARGET_ARCH_VARIANT := armv8-a
TARGET_CPU_ABI := arm64-v8a
TARGET_CPU_ABI2 :=
TARGET_CPU_VARIANT := generic
TARGET_CPU_VARIANT_RUNTIME := cortex-a75

TARGET_2ND_ARCH := arm
TARGET_2ND_ARCH_VARIANT := armv8-a
TARGET_2ND_CPU_ABI := armeabi-v7a
TARGET_2ND_CPU_ABI2 := armeabi
TARGET_2ND_CPU_VARIANT := generic
TARGET_2ND_CPU_VARIANT_RUNTIME := cortex-a75

TARGET_BOARD_COMMON_PATH := device/google/bonito/sdm710

BUILD_BROKEN_DUP_RULES := true

BOARD_KERNEL_CMDLINE += console=ttyMSM0,115200n8 androidboot.console=ttyMSM0 printk.devkmsg=on
BOARD_KERNEL_CMDLINE += msm_rtb.filter=0x237
BOARD_KERNEL_CMDLINE += ehci-hcd.park=3
BOARD_KERNEL_CMDLINE += service_locator.enable=1
BOARD_KERNEL_CMDLINE += firmware_class.path=/vendor/firmware
BOARD_KERNEL_CMDLINE += cgroup.memory=nokmem
# STOPSHIP Bringup hack- no low power
BOARD_KERNEL_CMDLINE += lpm_levels.sleep_disabled=1
BOARD_KERNEL_CMDLINE += loop.max_part=7
BOARD_KERNEL_CMDLINE += androidboot.boot_devices=soc/7c4000.sdhci

BOARD_KERNEL_BASE        := 0x00000000
BOARD_KERNEL_PAGESIZE    := 4096
ifeq ($(filter-out bonito_kasan sargo_kasan, $(TARGET_PRODUCT)),)
BOARD_KERNEL_OFFSET      := 0x80000
BOARD_KERNEL_TAGS_OFFSET := 0x02500000
BOARD_RAMDISK_OFFSET     := 0x02700000
BOARD_MKBOOTIMG_ARGS     := --kernel_offset $(BOARD_KERNEL_OFFSET) --ramdisk_offset $(BOARD_RAMDISK_OFFSET) --tags_offset $(BOARD_KERNEL_TAGS_OFFSET)
else
BOARD_KERNEL_TAGS_OFFSET := 0x01E00000
BOARD_RAMDISK_OFFSET     := 0x02000000
endif

BOARD_INCLUDE_DTB_IN_BOOTIMG := true
BOARD_BOOT_HEADER_VERSION := 2
BOARD_MKBOOTIMG_ARGS += --header_version $(BOARD_BOOT_HEADER_VERSION)

# DTBO partition definitions
BOARD_PREBUILT_DTBOIMAGE := device/google/bonito-kernel/dtbo.img
BOARD_DTBOIMG_PARTITION_SIZE := 8388608

TARGET_NO_KERNEL := false
BOARD_USES_RECOVERY_AS_BOOT := true
BOARD_USES_METADATA_PARTITION := true

AB_OTA_UPDATER := true

AB_OTA_PARTITIONS += \
    boot \
    system \
    vbmeta \
    dtbo \
    product \
    system_ext

BOARD_PRODUCTIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_SYSTEM_EXTIMAGE_FILE_SYSTEM_TYPE := ext4

# Partitions (listed in the file) to be wiped under recovery.
TARGET_RECOVERY_WIPE := device/google/bonito/recovery.wipe
TARGET_RECOVERY_FSTAB := device/google/bonito/fstab.hardware
TARGET_RECOVERY_PIXEL_FORMAT := RGBX_8888
TARGET_RECOVERY_UI_LIB := \
    librecovery_ui_pixel \
    libfstab

# system.img
BOARD_SYSTEMIMAGE_JOURNAL_SIZE := 0

# persist.img
BOARD_PERSISTIMAGE_PARTITION_SIZE := 41943040
BOARD_PERSISTIMAGE_FILE_SYSTEM_TYPE := ext4

# boot.img
BOARD_BOOTIMAGE_PARTITION_SIZE := 0x04000000

BOARD_SUPER_PARTITION_GROUPS := google_dynamic_partitions
BOARD_GOOGLE_DYNAMIC_PARTITIONS_PARTITION_LIST := \
    system \
    vendor \
    product \
    system_ext

BOARD_SUPER_PARTITION_SIZE := 4072669184
BOARD_SUPER_PARTITION_METADATA_DEVICE := system
BOARD_SUPER_PARTITION_BLOCK_DEVICES := system vendor
BOARD_SUPER_PARTITION_SYSTEM_DEVICE_SIZE := 3267362816
BOARD_SUPER_PARTITION_VENDOR_DEVICE_SIZE := 805306368
# Assume 4MB metadata size.
# TODO(b/117997386): Use correct metadata size.
BOARD_GOOGLE_DYNAMIC_PARTITIONS_SIZE := 4068474880

BOARD_FLASH_BLOCK_SIZE := 131072

BOARD_ROOT_EXTRA_SYMLINKS := /mnt/vendor/persist:/persist
BOARD_ROOT_EXTRA_SYMLINKS += /vendor/dsp:/dsp

include device/google/bonito-sepolicy/bonito-sepolicy.mk

TARGET_FS_CONFIG_GEN := device/google/bonito/config.fs

QCOM_BOARD_PLATFORMS += sdm710
BOARD_HAVE_BLUETOOTH_QCOM := true
BOARD_USES_COMMON_BLUETOOTH_HAL := true

# Camera
TARGET_USES_AOSP := true
BOARD_QTI_CAMERA_32BIT_ONLY := false
CAMERA_DAEMON_NOT_PRESENT := true
TARGET_USES_ION := true
TARGET_USES_EASEL := true
BOARD_USES_EASEL := true

# GPS
TARGET_NO_RPC := true
BOARD_VENDOR_QCOM_GPS_LOC_API_HARDWARE := default
BOARD_VENDOR_QCOM_LOC_PDK_FEATURE_SET := true

# RenderScript
OVERRIDE_RS_DRIVER := libRSDriver_adreno.so

# Sensors
USE_SENSOR_MULTI_HAL := true
TARGET_SUPPORT_DIRECT_REPORT := true
# Enable sensor Version V_2
USE_SENSOR_HAL_VER := 2.0

# CHRE
CHRE_DAEMON_ENABLED := true
CHRE_DAEMON_LOAD_INTO_SENSORSPD := true

# wlan
BOARD_WLAN_DEVICE := qcwcn
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_HOSTAPD_DRIVER := NL80211
WIFI_DRIVER_DEFAULT := qca_cld3
WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_$(BOARD_WLAN_DEVICE)
BOARD_HOSTAPD_PRIVATE_LIB := lib_driver_cmd_$(BOARD_WLAN_DEVICE)
WIFI_HIDL_FEATURE_AWARE := true
WIFI_HIDL_FEATURE_DUAL_INTERFACE:= true
WIFI_FEATURE_WIFI_EXT_HAL := true
WIFI_FEATURE_IMU_DETECTION := false
WIFI_HIDL_UNIFIED_SUPPLICANT_SERVICE_RC_ENTRY := true

# Audio
BOARD_USES_ALSA_AUDIO := true
AUDIO_FEATURE_ENABLED_MULTI_VOICE_SESSIONS := true
AUDIO_FEATURE_ENABLED_SND_MONITOR := true
AUDIO_FEATURE_ENABLED_USB_TUNNEL := true
AUDIO_FEATURE_ENABLED_CIRRUS_SPKR_PROTECTION := true
BOARD_SUPPORTS_SOUND_TRIGGER := true
AUDIO_FEATURE_FLICKER_SENSOR_INPUT := true
SOUND_TRIGGER_FEATURE_LPMA_ENABLED := true
AUDIO_FEATURE_ENABLED_MAXX_AUDIO := true
BOARD_SUPPORTS_SOUND_TRIGGER_5514 := true
AUDIO_FEATURE_ENABLED_24BITS_CAMCORDER := true

# Graphics
TARGET_USES_GRALLOC1 := true
TARGET_USES_HWC2 := true
TARGET_USES_NV21_CAMERA_PREVIEW := true

VSYNC_EVENT_PHASE_OFFSET_NS := 2000000
SF_VSYNC_EVENT_PHASE_OFFSET_NS := 6000000

# Display
TARGET_HAS_WIDE_COLOR_DISPLAY := false
TARGET_USES_DISPLAY_RENDER_INTENTS := true
TARGET_USES_COLOR_METADATA := true
TARGET_USES_DRM_PP := true

# Vendor Interface Manifest
DEVICE_MANIFEST_FILE := device/google/bonito/manifest.xml
DEVICE_MATRIX_FILE := device/google/bonito/compatibility_matrix.xml
DEVICE_FRAMEWORK_COMPATIBILITY_MATRIX_FILE := device/google/bonito/device_framework_matrix.xml

# Userdebug only Vendor Interface Manifest
ifneq (,$(filter userdebug eng, $(TARGET_BUILD_VARIANT)))
DEVICE_FRAMEWORK_MANIFEST_FILE += device/google/bonito/framework_manifest_userdebug.xml
DEVICE_MATRIX_FILE += device/google/bonito/compatibility_matrix_userdebug.xml
endif

ODM_MANIFEST_SKUS := \
    G020A \
    G020B \
    G020C \
    G020D \
    G020E \
    G020F \
    G020G \
    G020H \

ODM_MANIFEST_G020A_FILES := device/google/bonito/nfc/manifest_se_SIM1.xml
ODM_MANIFEST_G020B_FILES := device/google/bonito/nfc/manifest_se_SIM1.xml
ODM_MANIFEST_G020C_FILES := device/google/bonito/nfc/manifest_se_SIM1.xml
ODM_MANIFEST_G020D_FILES := device/google/bonito/nfc/manifest_se_eSE1.xml
ODM_MANIFEST_G020E_FILES := device/google/bonito/nfc/manifest_se_SIM1.xml
ODM_MANIFEST_G020F_FILES := device/google/bonito/nfc/manifest_se_SIM1.xml
ODM_MANIFEST_G020G_FILES := device/google/bonito/nfc/manifest_se_SIM1.xml
ODM_MANIFEST_G020H_FILES := device/google/bonito/nfc/manifest_se_eSE1.xml

# Use mke2fs to create ext4 images
TARGET_USES_MKE2FS := true

# Kernel modules
ifeq (,$(filter-out sargo_kasan bonito_kasan, $(TARGET_PRODUCT)))
BOARD_VENDOR_KERNEL_MODULES += \
    $(wildcard device/google/bonito-kernel/kasan/*.ko)
else ifeq (,$(filter-out sargo_kernel_debug_memory bonito_kernel_debug_memory, $(TARGET_PRODUCT)))
BOARD_VENDOR_KERNEL_MODULES += \
    $(wildcard device/google/bonito-kernel/debug_memory/*.ko)
else ifeq (,$(filter-out sargo_kernel_debug_memory_accounting bonito_kernel_debug_memory_accounting, $(TARGET_PRODUCT)))
BOARD_VENDOR_KERNEL_MODULES += \
    $(wildcard device/google/bonito-kernel/debug_memory_acounting/*.ko)
BOARD_KERNEL_CMDLINE += page_owner=on
else ifeq (,$(filter-out sargo_kernel_debug_locking bonito_kernel_debug_locking, $(TARGET_PRODUCT)))
BOARD_VENDOR_KERNEL_MODULES += \
    $(wildcard device/google/bonito-kernel/debug_locking/*.ko)
else ifeq (,$(filter-out sargo_kernel_debug_hang bonito_kernel_debug_hang, $(TARGET_PRODUCT)))
BOARD_VENDOR_KERNEL_MODULES += \
    $(wildcard device/google/bonito-kernel/debug_hang/*.ko)
else ifeq (,$(filter-out sargo_kernel_debug_api bonito_kernel_debug_api, $(TARGET_PRODUCT)))
BOARD_VENDOR_KERNEL_MODULES += \
    $(wildcard device/google/bonito-kernel/debug_api/*.ko)
else
BOARD_VENDOR_KERNEL_MODULES += \
    $(wildcard device/google/bonito-kernel/*.ko)
endif

# DTB
ifeq (,$(filter-out sargo_kasan bonito_kasan, $(TARGET_PRODUCT)))
BOARD_PREBUILT_DTBIMAGE_DIR := device/google/bonito-kernel/kasan
else ifeq (,$(filter-out sargo_kernel_debug_memory bonito_kernel_debug_memory, $(TARGET_PRODUCT)))
BOARD_PREBUILT_DTBIMAGE_DIR := device/google/bonito-kernel/debug_memory
else ifeq (,$(filter-out sargo_kernel_debug_memory_accounting bonito_kernel_debug_memory_accounting, $(TARGET_PRODUCT)))
BOARD_PREBUILT_DTBIMAGE_DIR := device/google/bonito-kernel/debug_memory_accounting
else ifeq (,$(filter-out sargo_kernel_debug_locking bonito_kernel_debug_locking, $(TARGET_PRODUCT)))
BOARD_PREBUILT_DTBIMAGE_DIR := device/google/bonito-kernel/debug_locking
else ifeq (,$(filter-out sargo_kernel_debug_hang bonito_kernel_debug_hang, $(TARGET_PRODUCT)))
BOARD_PREBUILT_DTBIMAGE_DIR := device/google/bonito-kernel/debug_hang
else ifeq (,$(filter-out sargo_kernel_debug_api bonito_kernel_debug_api, $(TARGET_PRODUCT)))
BOARD_PREBUILT_DTBIMAGE_DIR := device/google/bonito-kernel/debug_api
else
BOARD_PREBUILT_DTBIMAGE_DIR := device/google/bonito-kernel
endif

# Testing related defines
BOARD_PERFSETUP_SCRIPT := platform_testing/scripts/perf-setup/b4s4-setup.sh

-include vendor/google_devices/bonito/proprietary/BoardConfigVendor.mk
