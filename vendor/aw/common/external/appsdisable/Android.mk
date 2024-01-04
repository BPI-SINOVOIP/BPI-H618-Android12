# Copyright 2007 The Android Open Source Project
#
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := appsdisable
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_SRC_FILES := appsdisable
LOCAL_INIT_RC := appsdisable.rc
include $(BUILD_PREBUILT)
