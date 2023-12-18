#
# Copyright 2018 The Android Open Source Project
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

#
# All components inherited here go to system image
#
ifeq (,$(filter %_64,$(TARGET_PRODUCT)))
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit.mk)
else
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit_only.mk)
endif
$(call inherit-product, $(SRC_TARGET_DIR)/product/generic_system.mk)

# Enable mainline checking
PRODUCT_ENFORCE_ARTIFACT_PATH_REQUIREMENTS := strict

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
# All components inherited here go to vendor image
#
# TODO(b/136525499): move *_vendor.mk into the vendor makefile later
$(call inherit-product, $(SRC_TARGET_DIR)/product/handheld_vendor.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/telephony_vendor.mk)

$(call inherit-product, device/google/redfin/device-redfin.mk)
$(call inherit-product-if-exists, vendor/google_devices/redfin/proprietary/device-vendor.mk)
$(call inherit-product-if-exists, vendor/google_devices/redfin/prebuilts/device-vendor-redfin.mk)

# Exclude features that are not available on AOSP devices.
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/aosp_excluded_hardware.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/aosp_excluded_hardware.xml

# Keep the VNDK APEX in /system partition for REL branches and Vendor Freeze targets
# as these are expected to have stable API/ABI surfaces.
ifneq (REL,$(PLATFORM_VERSION_CODENAME))
ifneq ($(PRODUCT_VENDOR_FREEZE_SYSTEM_BUILD),true)
  PRODUCT_PACKAGES += com.android.vndk.current.on_vendor
endif
endif

# Don't build super.img.
PRODUCT_BUILD_SUPER_PARTITION := false

# b/113232673 STOPSHIP deal with Qualcomm stuff later
# PRODUCT_RESTRICT_VENDOR_FILES := all

# b/189477034: Bypass build time check on uses_libs until vendor fixes all their apps
PRODUCT_BROKEN_VERIFY_USES_LIBRARIES := true

PRODUCT_MANUFACTURER := Google
PRODUCT_BRAND := Android
PRODUCT_NAME := aosp_redfin
PRODUCT_DEVICE := redfin
PRODUCT_MODEL := AOSP on redfin
