#
# Copyright 2020 The Android Open Source Project
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

####
#### This file should be included at the bottom of the aosp_PHONE_car.mk file
####

# Auto modules
PRODUCT_PACKAGES += \
            android.hardware.automotive.audiocontrol-service.example \
            android.hardware.automotive.can@1.0-service

PRODUCT_PACKAGES_DEBUG += \
            canhalctrl \
            canhaldump \
            canhalsend \
            android.hardware.automotive.occupant_awareness@1.0-service \
            android.hardware.automotive.occupant_awareness@1.0-service_mock

BOARD_SEPOLICY_DIRS += device/google_car/common/sepolicy

# Sepolicy for occupant awareness system
include packages/services/Car/car_product/occupant_awareness/OccupantAwareness.mk

# Sepolicy for compute pipe system
include packages/services/Car/cpp/computepipe/products/computepipe.mk

PRODUCT_COPY_FILES += \
    device/google_car/common/audio_effects.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_effects.xml \

PRODUCT_PROPERTY_OVERRIDES += \
        ro.boot.wificountrycode=00 \
        log.tag.CarTrustAgentUnlockEvent=I

# Phone car targets don't support ramdump
EXCLUDE_BUILD_RAMDUMP_UPLOADER_DEBUG_TOOL := true