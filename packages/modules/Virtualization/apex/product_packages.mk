#
# Copyright (C) 2021 Google Inc.
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

# TODO: Remove this once the APEX is included in base system.

# To include the APEX in your build, insert this in your device.mk:
#   $(call inherit-product, packages/modules/Virtualization/apex/product_packages.mk)

PRODUCT_PACKAGES += com.android.virt
PRODUCT_ARTIFACT_PATH_REQUIREMENT_ALLOWED_LIST += \
    system/apex/com.android.virt.apex \
    system/bin/crosvm \
    system/lib64/%.dylib.so \
    system/lib64/libfdt.so \
    system/lib64/libgfxstream_backend.so \
    system/lib64/libcuttlefish_allocd_utils.so \
    system/lib64/libcuttlefish_fs.so \
    system/lib64/libcuttlefish_utils.so

$(call inherit-product, external/crosvm/seccomp/crosvm_seccomp_policy_product_packages.mk)
