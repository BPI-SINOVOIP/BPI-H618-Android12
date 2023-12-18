# Copyright 2007 The Android Open Source Project
#
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := aw_thermal
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_SRC_FILES := aw_thermal.$(TARGET_BOARD_PLATFORM)
LOCAL_INIT_RC := aw_thermal.rc
include $(BUILD_PREBUILT)
