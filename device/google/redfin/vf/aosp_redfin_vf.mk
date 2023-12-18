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

# Makefile for the system-only partial build of aosp_redfin.
# Used for creating a merged Vendor Freeze device.

PRODUCT_VENDOR_FREEZE_SYSTEM_BUILD := true

# Include VNDK v30, needed by the Android 11 vendor half.
PRODUCT_EXTRA_VNDK_VERSIONS = 30

# Include a product compatibility matrix file that includes older HALs provided
# by the Android 11 vendor half. This is needed because check_vintf fails for
# any "unused" HALs in the device manifest that aren't found in the framework
# compatibility matrix.
PRODUCT_PACKAGES += redfin_product_compatibility_matrix.R.5.xml

# Disable building certain non-system partitions in this build.
PRODUCT_BUILD_BOOT_IMAGE := false
PRODUCT_BUILD_RAMDISK_IMAGE := false
PRODUCT_BUILD_RECOVERY_IMAGE := false
PRODUCT_BUILD_VBMETA_IMAGE := false
PRODUCT_BUILD_VENDOR_IMAGE := false
PRODUCT_BUILD_VENDOR_BOOT_IMAGE := false

$(call inherit-product, device/google/redfin/aosp_redfin.mk)

PRODUCT_NAME := aosp_redfin_vf
