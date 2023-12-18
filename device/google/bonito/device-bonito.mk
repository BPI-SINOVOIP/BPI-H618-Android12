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

PRODUCT_HARDWARE := bonito

include device/google/bonito/device-common.mk

DEVICE_PACKAGE_OVERLAYS += device/google/bonito/bonito/overlay

# SKU specific RROs
PRODUCT_PACKAGES += \
    SettingsOverlayG020A \
    SettingsOverlayG020B \
    SettingsOverlayG020C \
    SettingsOverlayG020D \

# Setup wizard overlay packages for ActiveEdge
PRODUCT_PACKAGES += \
    PixelSetupWizardOverlayActiveEdge \

PRODUCT_COPY_FILES += \
    device/google/bonito/nfc/libnfc-nxp.bonito.conf:$(TARGET_COPY_OUT_VENDOR)/etc/libnfc-nxp.conf \
    device/google/bonito/nfc/libnfc-nxp.bonito.uicc.conf:$(TARGET_COPY_OUT_VENDOR)/etc/libnfc-nxp-G020A.conf \
    device/google/bonito/nfc/libnfc-nxp.bonito.uicc.conf:$(TARGET_COPY_OUT_VENDOR)/etc/libnfc-nxp-G020B.conf \
    device/google/bonito/nfc/libnfc-nxp.bonito.uicc.conf:$(TARGET_COPY_OUT_VENDOR)/etc/libnfc-nxp-G020C.conf \
    device/google/bonito/nfc/libnfc-nxp.bonito.ese.conf:$(TARGET_COPY_OUT_VENDOR)/etc/libnfc-nxp-G020D.conf

# Vibrator HAL
PRODUCT_PRODUCT_PROPERTIES +=\
    ro.vendor.vibrator.hal.config.dynamic=1 \
    ro.vendor.vibrator.hal.click.duration=8 \
    ro.vendor.vibrator.hal.tick.duration=5 \
    ro.vendor.vibrator.hal.heavyclick.duration=12 \
    ro.vendor.vibrator.hal.short.voltage=120 \
    ro.vendor.vibrator.hal.long.voltage=90 \
    ro.vendor.vibrator.hal.long.frequency.shift=10 \
    ro.vendor.vibrator.hal.double_click.duration=182

# DRV2624 Haptics Waveform
PRODUCT_COPY_FILES += \
    device/google/bonito/vibrator/drv2624/drv2624_B4.bin:$(TARGET_COPY_OUT_VENDOR)/firmware/drv2624.bin

# camera front flashColor
PRODUCT_PROPERTY_OVERRIDES += \
    persist.camera.front.flashColor=0xffebcc

# Add white point compensated coefficient
PRODUCT_PROPERTY_OVERRIDES += \
    vendor.display.adaptive_white_coefficient=0.0051,-0.3462,18.964,0.0047,-0.1155,-10.312,0.005,-0.2931,12.301
