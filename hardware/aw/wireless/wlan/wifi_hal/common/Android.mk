# Copyright (C) 2011 The Android Open Source Project
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

LOCAL_PATH := $(call my-dir)

# Make the HAL library
# ============================================================
include $(CLEAR_VARS)

LOCAL_C_INCLUDES += \
    hardware/aw/wireless/hwinfo \
    external/libnl/include \
    $(call include-path-for, libhardware_legacy)/hardware_legacy \
    external/wpa_supplicant_8/src/drivers

LOCAL_WHOLE_STATIC_LIBRARIES := libhwinfo
LOCAL_HEADER_LIBRARIES := libutils_headers liblog_headers
LOCAL_SRC_FILES := wifi_hal.cpp

LOCAL_MODULE := libwifi-hal-autodetect
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libwifi-hal-package
LOCAL_MODULE_OWNER := google
LOCAL_MODULE_TAGS := optional

LOCAL_VENDOR_LIST := hardware/broadcom/wlan/bcmdhd/wifi_hal:libwifi-hal-bcm
LOCAL_VENDOR_LIST += hardware/realtek/wlan/wifi_hal:libwifi-hal-rtk
LOCAL_VENDOR_LIST += hardware/xradio/wlan/wifi_hal:libwifi-hal-xradio
LOCAL_VENDOR_LIST += hardware/ssv/wlan/wifi_hal:libwifi-hal-ssv
LOCAL_VENDOR_LIST += hardware/sprd/wlan/wifi_hal:libwifi-hal-sprd
LOCAL_VENDOR_LIST += hardware/aic/wlan/wifi_hal:libwifi-hal-aic
LOCAL_VENDOR_LIST += hardware/altobeam/wlan/wifi_hal:libwifi-hal-atbm

LOCAL_REQUIRED_MODULES :=

$(foreach e,$(LOCAL_VENDOR_LIST),\
    $(eval LOCAL_VENDOR_PATH   := $(shell echo $(e) | awk -F: '{print $$1}')) \
    $(eval LOCAL_VENDOR_MODULE := $(shell echo $(e) | awk -F: '{print $$2}')) \
    $(if $(wildcard $(LOCAL_VENDOR_PATH)),\
        $(eval LOCAL_REQUIRED_MODULES += $(LOCAL_VENDOR_MODULE)) \
    ) \
)

include $(BUILD_PHONY_PACKAGE)
