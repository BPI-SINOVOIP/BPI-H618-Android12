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

-include hardware/sprd/wlan/wpa_supplicant_conf/config-sprd.mk

########################

PRODUCT_COPY_FILES += \
    hardware/sprd/wlan/firmware/uwe5622/wcnmodem.bin:$(TARGET_COPY_OUT_VENDOR)/etc/firmware/wcnmodem.bin \
    $(call find-copy-subdir-files,"wifi_*.ini",hardware/sprd/wlan/firmware/uwe5622,$(TARGET_COPY_OUT_VENDOR)/etc)
########################
