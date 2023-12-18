# Copyright 2007 The Android Open Source Project
#
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := fix_traceing
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_SRC_FILES := fix_traceing
LOCAL_INIT_RC := fix_traceing.rc
include $(BUILD_PREBUILT)
