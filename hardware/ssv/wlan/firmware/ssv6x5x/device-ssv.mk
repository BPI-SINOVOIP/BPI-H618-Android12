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

-include hardware/ssv/wlan/wpa_supplicant_conf/config-ssv.mk

########################

PRODUCT_COPY_FILES += \
    hardware/ssv/wlan/firmware/ssv6x5x/ssv6x5x-wifi.cfg:$(TARGET_COPY_OUT_VENDOR)/etc/firmware/ssv6x5x/ssv6x5x-wifi.cfg \
    hardware/ssv/wlan/firmware/ssv6x5x/ssv6x5x-sw.bin:$(TARGET_COPY_OUT_VENDOR)/etc/firmware/ssv6x5x/ssv6x5x-sw.bin \
    hardware/ssv/wlan/firmware/ssv6x5x/flash.bin:$(TARGET_COPY_OUT_VENDOR)/etc/firmware/ssv6x5x/flash.bin
########################
