# Copyright 2007 The Android Open Source Project
#
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := preinstall
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_SRC_FILES := preinstall
LOCAL_INIT_RC := preinstall.rc
include $(BUILD_PREBUILT)
