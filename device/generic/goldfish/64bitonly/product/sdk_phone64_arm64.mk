#
# Copyright (C) 2009 The Android Open Source Project
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
QEMU_USE_SYSTEM_EXT_PARTITIONS := true
PRODUCT_USE_DYNAMIC_PARTITIONS := true

# This is a build configuration for a full-featured build of the
# Open-Source part of the tree. It's geared toward a US-centric
# build quite specifically for the emulator, and might not be
# entirely appropriate to inherit from for on-device configurations.

PRODUCT_ENFORCE_ARTIFACT_PATH_REQUIREMENTS := relaxed

#
# All components inherited here go to system image
#
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/generic_system.mk)

#
# All components inherited here go to system_ext image
#
$(call inherit-product, $(SRC_TARGET_DIR)/product/handheld_system_ext.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/telephony_system_ext.mk)

#
# All components inherited here go to product image
#
$(call inherit-product, $(SRC_TARGET_DIR)/product/aosp_product.mk)

#
# All components inherited here go to vendor or vendor_boot image
#
$(call inherit-product, device/generic/goldfish/arm64-vendor.mk)
$(call inherit-product, device/generic/goldfish/64bitonly/product/emulator64_vendor.mk)
$(call inherit-product, device/generic/goldfish/emulator64_arm64/device.mk)

# Define the host tools and libs that are parts of the SDK.
$(call inherit-product-if-exists, sdk/build/product_sdk.mk)
$(call inherit-product-if-exists, development/build/product_sdk.mk)

# Overrides
PRODUCT_BRAND := Android
PRODUCT_NAME := sdk_phone64_arm64
PRODUCT_DEVICE := emulator64_arm64
PRODUCT_MODEL := Android SDK built for arm64
