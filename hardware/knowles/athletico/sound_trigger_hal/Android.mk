# Copyright (C) 2018 Knowles Electronics
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
ifneq (,$(findstring $(USE_SOUND_TRIGGER_HAL),iaxxx))

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := sound_trigger.primary.$(TARGET_BOARD_PLATFORM)
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := sound_trigger_hw_iaxxx.c cvq_util.c
LOCAL_VENDOR_MODULE := true
LOCAL_C_INCLUDES += external/tinyalsa/include \
			$(call include-path-for, audio-route)
LOCAL_HEADER_LIBRARIES := libhardware_headers
LOCAL_SHARED_LIBRARIES := liblog \
			libcutils \
			libtinyalsa \
			libaudioroute \
			libodsp \
			libhardware_legacy
LOCAL_MODULE_TAGS := optional
ifneq (,$(findstring $(PLATFORM_VERSION), P))
LOCAL_PROPRIETARY_MODULE := true
LOCAL_CFLAGS += -DANDROID_P
endif

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := adnc_strm.primary.default
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_VENDOR_MODULE := true
LOCAL_SRC_FILES := adnc_strm.c
LOCAL_SHARED_LIBRARIES := liblog \
			libcutils \
			libtinyalsa \
			libtunnel
LOCAL_MODULE_TAGS := optional
ifneq (,$(findstring $(PLATFORM_VERSION), P))
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := libtunnel
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_VENDOR_MODULE := true
LOCAL_SRC_FILES := tunnel.c
LOCAL_SHARED_LIBRARIES := liblog \
			libcutils

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := libodsp
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_VENDOR_MODULE := true
LOCAL_SRC_FILES := iaxxx_odsp_hw.c
LOCAL_SHARED_LIBRARIES := liblog \
			libcutils
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := tunneling_hal_test
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_VENDOR_MODULE := true
LOCAL_SRC_FILES := tests/tunnel_test.c \
			tests/conversion_routines.c
LOCAL_32_BIT_ONLY := true
LOCAL_SHARED_LIBRARIES := liblog \
			libcutils \
			libtunnel
LOCAL_CFLAGS += -DFILENAME_ASSIGN

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE := sensor_param_test
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_SRC_FILES := tests/sensor_param_test.c
LOCAL_32_BIT_ONLY := true
LOCAL_SHARED_LIBRARIES := liblog \
			libutils \
			libcutils \
			libtinyalsa
LOCAL_C_INCLUDES += external/tinyalsa/include \
		      $(LOCAL_PATH)/../hal

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := libosloutils
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_SRC_FILES := tests/oslo_sound_model_control.cpp \
			tests/oslo_iaxxx_sensor_control.c
LOCAL_C_INCLUDES += external/tinyalsa/include \
			$(call include-path-for, audio-route)
LOCAL_HEADER_LIBRARIES := libhardware_headers
LOCAL_SHARED_LIBRARIES := liblog \
			libutils \
			libcutils \
			libtinyalsa \
			libhidlbase \
			libodsp \
			android.hardware.soundtrigger@2.0
LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := oslo_data_injection_test
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_SRC_FILES := tests/oslo_data_injection_test.c
LOCAL_32_BIT_ONLY := true
LOCAL_HEADER_LIBRARIES := libhardware_headers
LOCAL_SHARED_LIBRARIES := liblog \
			libutils \
			libcutils \
			libosloutils
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := oslo_config_test
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_SRC_FILES := tests/oslo_config_test.c
LOCAL_32_BIT_ONLY := true
LOCAL_HEADER_LIBRARIES := libhardware_headers
LOCAL_SHARED_LIBRARIES := liblog \
			libutils \
			libcutils \
			libosloutils
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := oslo_get_stats
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_SRC_FILES := tests/oslo_get_stats.c
LOCAL_HEADER_LIBRARIES := libhardware_headers
LOCAL_SHARED_LIBRARIES := liblog \
			libutils \
			libcutils \
			libosloutils
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := spi_reliability_test
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_SRC_FILES := tests/spi_reliability_test.c
LOCAL_32_BIT_ONLY := true
LOCAL_SHARED_LIBRARIES := liblog \
			libcutils

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := oslo_package_test
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_SRC_FILES := ./tests/oslo_package_test.c
LOCAL_32_BIT_ONLY := true
LOCAL_SHARED_LIBRARIES := liblog \
			libcutils \
			libtinyalsa

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := tunneling_hal_test_sensor
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_SRC_FILES := tests/tunnel_test_sensor.c
LOCAL_32_BIT_ONLY := true
LOCAL_SHARED_LIBRARIES := liblog

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := odsp_api_test
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_VENDOR_MODULE := true
LOCAL_SRC_FILES := tests/odsp_api_test.c
LOCAL_C_INCLUDES += ./tests/
LOCAL_32_BIT_ONLY := true
LOCAL_SHARED_LIBRARIES := liblog \
			libcutils \
			libodsp

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE := dump_debug_info
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_SRC_FILES := tests/dump_debug_info.c
LOCAL_32_BIT_ONLY := true
LOCAL_SHARED_LIBRARIES := liblog

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE := crash_event_logger
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_SRC_FILES := tests/crash_event_logger.c tests/crash_analyzer.c
LOCAL_32_BIT_ONLY := true
LOCAL_SHARED_LIBRARIES := liblog libcutils
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE := false
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE := setparamblk_test
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_SRC_FILES := tests/setparamblk_test.c
LOCAL_32_BIT_ONLY := true
LOCAL_SHARED_LIBRARIES := libcutils \
			libodsp

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := crash_trigger_test
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_SRC_FILES := tests/crash_trigger_test.c
LOCAL_VENDOR_MODULE := true
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../hal
LOCAL_32_BIT_ONLY := true
LOCAL_SHARED_LIBRARIES := liblog libodsp

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := plugin_status_test
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_SRC_FILES := tests/plugin_status_test.c
LOCAL_VENDOR_MODULE := true
LOCAL_C_INCLUDES += $(LOCAL_PATH)/
LOCAL_32_BIT_ONLY := true
LOCAL_SHARED_LIBRARIES := libcutils libodsp

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := get_pwr_stats
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_VENDOR_MODULE := true
LOCAL_SRC_FILES := tests/get_pwr_stats.c
LOCAL_C_INCLUDES += $(LOCAL_PATH)/
LOCAL_32_BIT_ONLY := true
LOCAL_SHARED_LIBRARIES := libcutils liblog

include $(BUILD_EXECUTABLE)
endif
