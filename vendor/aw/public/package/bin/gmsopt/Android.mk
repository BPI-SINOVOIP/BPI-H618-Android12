# Copyright 2007 The Android Open Source Project
#
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := gmsopt
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_SRC_FILES := gmsopt
LOCAL_INIT_RC := gmsopt.rc
include $(BUILD_PREBUILT)
