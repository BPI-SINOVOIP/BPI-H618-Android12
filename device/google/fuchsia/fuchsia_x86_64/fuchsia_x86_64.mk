#
# Copyright (C) 2019 The Android Open Source Project
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


PRODUCT_NAME := fuchsia_x86_64
PRODUCT_DEVICE := fuchsia_x86_64

include device/google/fuchsia/fuchsia.mk

# TODO(b/122047335): Uncomment this.
#PRODUCT_COPY_FILES += \
    prebuilts/clang/host/linux-x86/clang-fuchsia/lib/x86_64-fuchsia/lib/libunwind.so.1:system/lib64/libunwind.so.1 \
    prebuilts/fuchsia_sdk/arch/x64/sysroot/dist/lib/ld.so.1:system/lib64/ld.so.1
