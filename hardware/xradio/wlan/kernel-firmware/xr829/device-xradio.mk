#
# Copyright (C) 2008 The Android Open Source Project
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

########################
PRODUCT_COPY_FILES += \
    hardware/xradio/wlan/kernel-firmware/xr829/boot_xr829.bin:$(TARGET_COPY_OUT_VENDOR)/etc/firmware/boot_xr829.bin \
    hardware/xradio/wlan/kernel-firmware/xr829/fw_xr829.bin:$(TARGET_COPY_OUT_VENDOR)/etc/firmware/fw_xr829.bin \
    hardware/xradio/wlan/kernel-firmware/xr829/sdd_xr829.bin:$(TARGET_COPY_OUT_VENDOR)/etc/firmware/sdd_xr829.bin \
    hardware/xradio/wlan/kernel-firmware/xr829/etf_xr829.bin:$(TARGET_COPY_OUT_VENDOR)/etc/firmware/etf_xr829.bin
########################
