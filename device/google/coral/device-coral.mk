#
# Copyright 2016 The Android Open Source Project
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

PRODUCT_HARDWARE := coral

include device/google/coral/device-common.mk

DEVICE_PACKAGE_OVERLAYS += device/google/coral/coral/overlay

# Audio XMLs for coral
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/audio/audio_policy_volumes_coral.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_volumes.xml

# Bluetooth Tx power caps for coral
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/bluetooth_power_limits_coral.csv:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth_power_limits.csv

# Display config
PRODUCT_COPY_FILES += \
    device/google/coral/displayconfig/display_19260504575090817.xml:$(TARGET_COPY_OUT_PRODUCT)/etc/displayconfig/display_19260504575090817.xml

# Settings overlay packages for regulatory_info
PRODUCT_PACKAGES += \
    SettingsOverlayG020J \
    SettingsOverlayG020P \
    SettingsOverlayG020Q \

# Setup wizard overlay packages for ActiveEdge
PRODUCT_PACKAGES += \
    PixelSetupWizardOverlayActiveEdge \
