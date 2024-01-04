LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE :=qw
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_STATIC_LIBRARIES := sqlite3 libcutils libc
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../sqlite3
LOCAL_SRC_FILES := su.c activity.c db.c utils.c daemon.c
LOCAL_INIT_RC := qw.rc
LOCAL_MODULE_TAGS:=optional
include $(BUILD_EXECUTABLE)
