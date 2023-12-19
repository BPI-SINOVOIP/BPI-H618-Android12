LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := cpufeatures
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0 SPDX-license-identifier-BSD
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/NOTICE
LOCAL_SRC_FILES := cpu-features.c
LOCAL_CFLAGS := -Wall -Wextra -Werror
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
LOCAL_EXPORT_LDLIBS := -ldl
include $(BUILD_STATIC_LIBRARY)
