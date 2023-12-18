#
# Copyright (C) 2020 The Android Open Source Project
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

ifeq ($(TARGET_USES_CUTTLEFISH_AUDIO),true)
# Cuttlefish Audio HAL with custom configuration
LOCAL_AUDIO_PRODUCT_COPY_FILES ?= \
    device/google/cuttlefish/shared/config/audio_policy.conf:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy.conf \
    frameworks/av/services/audiopolicy/config/audio_policy_configuration_generic.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_configuration.xml \
    device/google/trout/product_files/vendor/etc/primary_audio_policy_configuration.cf.xml:$(TARGET_COPY_OUT_VENDOR)/etc/primary_audio_policy_configuration.xml

LOCAL_AUDIO_PROPERTIES ?=
else
# Trout Audio HAL
LOCAL_AUDIO_PRODUCT_PACKAGE ?= \
    audio.primary.trout \
    audio.r_submix.default \
    android.hardware.audio@6.0-impl:32 \
    android.hardware.audio.effect@6.0-impl:32 \
    android.hardware.audio.service \
    android.hardware.soundtrigger@2.3-impl

LOCAL_AUDIO_DEVICE_PACKAGE_OVERLAYS ?= device/google/trout/hal/audio/6.0/overlay

LOCAL_AUDIO_PROPERTIES ?= \
    ro.hardware.audio.primary=trout \

LOCAL_AUDIO_PRODUCT_COPY_FILES ?= \
    device/google/trout/hal/audio/6.0/audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_configuration.xml \
    device/google/trout/hal/audio/6.0/car_audio_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/car_audio_configuration.xml \
    frameworks/native/data/etc/android.hardware.broadcastradio.xml:system/etc/permissions/android.hardware.broadcastradio.xml \
    frameworks/av/services/audiopolicy/config/a2dp_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/a2dp_audio_policy_configuration.xml \
    frameworks/av/services/audiopolicy/config/usb_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/usb_audio_policy_configuration.xml
endif

# Audio Control HAL
LOCAL_AUDIOCONTROL_HAL_PRODUCT_PACKAGE ?= android.hardware.automotive.audiocontrol-service.trout

# Dumpstate HAL
LOCAL_DUMPSTATE_PRODUCT_PACKAGE ?= android.hardware.dumpstate@1.1-service.trout
LOCAL_DUMPSTATE_PROPERTIES ?= \
    ro.vendor.dumpstate.server.cid=2 \
    ro.vendor.dumpstate.server.port=9310 \
    ro.vendor.helpersystem.log_loc=/data/host_logs \

# Vehicle HAL
LOCAL_VHAL_PRODUCT_PACKAGE ?= android.hardware.automotive.vehicle@2.0-virtualization-service

# EVS HAL
LOCAL_EVS_PRODUCT_COPY_FILES ?= \
    device/google/trout/product_files/etc/automotive/evs/config_override.json:${TARGET_COPY_OUT_SYSTEM}/etc/automotive/evs/config_override.json \
    device/google/trout/product_files/vendor/etc/automotive/evs/evs_configuration_override.xml:$(TARGET_COPY_OUT_VENDOR)/etc/automotive/evs/evs_configuration_override.xml \

ENABLE_EVS_SAMPLE := true

BOARD_SEPOLICY_DIRS += device/google/trout/sepolicy/vendor/google

# Disable Vulkan feature flag as it is not supported on trout
TARGET_VULKAN_SUPPORT := false

PRODUCT_PROPERTY_OVERRIDES += \
    ro.hardware.type=automotive \
    ${LOCAL_AUDIO_PROPERTIES} \
    ${LOCAL_AUDIOCONTROL_PROPERTIES} \
    ${LOCAL_DUMPSTATE_PROPERTIES}

PRODUCT_CHARACTERISTICS := nosdcard,automotive

TARGET_BOARD_INFO_FILE ?= device/google/trout/board-info.txt

# Keymaster HAL
LOCAL_KEYMASTER_PRODUCT_PACKAGE ?= android.hardware.keymaster@4.1-service

# Gatekeeper HAL
LOCAL_GATEKEEPER_PRODUCT_PACKAGE ?= android.hardware.gatekeeper@1.0-service.software

PRODUCT_PACKAGES += tinyplay

PRODUCT_COPY_FILES += \
    ${LOCAL_EVS_PRODUCT_COPY_FILES} \

PRODUCT_COPY_FILES += \
    ${LOCAL_EVS_PRODUCT_COPY_FILES} \

include packages/services/Car/cpp/computepipe/products/computepipe.mk

# Trout fstab (workaround b/182190949)
PRODUCT_COPY_FILES += \
    device/google/trout/product_files/fstab.trout:$(TARGET_COPY_OUT_RAMDISK)/fstab.trout \
    device/google/trout/product_files/fstab.trout:$(TARGET_COPY_OUT_VENDOR)/etc/fstab.trout \
    device/google/trout/product_files/fstab.trout:$(TARGET_COPY_OUT_RECOVERY)/root/first_stage_ramdisk/fstab.trout

# User HAL support
TARGET_SUPPORTS_USER_HAL ?= true

ifeq ($(TARGET_SUPPORTS_USER_HAL),true)
PRODUCT_SYSTEM_DEFAULT_PROPERTIES += android.car.user_hal_enabled=true
endif
