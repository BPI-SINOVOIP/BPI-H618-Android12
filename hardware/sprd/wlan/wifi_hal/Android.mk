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
	$(LOCAL_PATH) \
	$(call include-path-for, libhardware_legacy)/hardware_legacy \
	external/libnl/include \
	external/wpa_supplicant_8/src/drivers

LOCAL_SRC_FILES := \
	wifi_hal.cpp \
	common.cpp \
	cpp_bindings.cpp \
	llstats.cpp \
	gscan.cpp \
	gscan_event_handler.cpp \
	rtt.cpp \
	ifaceeventhandler.cpp \
	tdls.cpp \
	nan.cpp \
	nan_ind.cpp \
	nan_req.cpp \
	nan_rsp.cpp \
	wificonfig.cpp \
	wifilogger.cpp \
	wifilogger_diag.cpp \
	ring_buffer.cpp \
	rb_wrapper.cpp \
	rssi_monitor.cpp \
	roam.cpp \
	radio_mode.cpp

LOCAL_MODULE := libwifi-hal-sprd
LOCAL_VENDOR_MODULE := true
LOCAL_CLANG := true
LOCAL_SHARED_LIBRARIES += \
	libnetutils \
	liblog \
	libdl \
	libnl \
	libwpa_client

LOCAL_HEADER_LIBRARIES := \
	libcutils_headers \
	libutils_headers

# gscan.cpp: address of array 'cached_results[i].results' will always evaluate to 'true'
LOCAL_CLANG_CFLAGS := \
	-Wno-pointer-bool-conversion

LOCAL_CFLAGS := \
	-Wno-format \
	-Wno-unused-parameter \
	-Wno-unused-variable \
	-Wno-unreachable-code-loop-increment \
	-DSPRD_PRODUCT_WIFI_DEVICE

LOCAL_REQUIRED_MODULES := liblowi_wifihal

include $(BUILD_STATIC_LIBRARY)

# Make the HAL library
# ============================================================

include $(CLEAR_VARS)

LOCAL_CFLAGS := -Wno-unused-parameter

LOCAL_C_INCLUDES += \
	$(call include-path-for, libhardware_legacy)/hardware_legacy \
	external/libnl/include \
	external/wpa_supplicant_8/src/drivers

LOCAL_STATIC_LIBRARIES += \
	libwifi-hal-sprd

LOCAL_SHARED_LIBRARIES += \
	libcutils \
	liblog \
	libutils \
	libhardware \
	libhardware_legacy \
	libnetutils \
	libnl \
	libdl

LOCAL_HEADER_LIBRARIES := \
	libcutils_headers \
	libutils_headers

LOCAL_SRC_FILES := \
	test/wifi_hal_cli.cpp

LOCAL_MODULE := wifi-hal-cli
LOCAL_VENDOR_MODULE := true

include $(BUILD_EXECUTABLE)

include $(LOCAL_PATH)/lowi/Android.mk
