LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := cpu_monitor
LOCAL_MODULE := cpu_monitor 
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_SHARED_LIBRARIES := libc++ libc libcutils libdl liblog libm libnetutils
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_PREBUILT)
