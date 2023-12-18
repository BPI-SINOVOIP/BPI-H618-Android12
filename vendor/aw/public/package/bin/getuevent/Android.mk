LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := getuevent
LOCAL_SRC_FILES := uevent.c
LOCAL_SHARED_LIBRARIES := libcutils liblog
LOCAL_PROPRIETARY_MODULE := true
LOCAL_C_INCLUDES += system/core/libutils/include
include $(BUILD_EXECUTABLE)
