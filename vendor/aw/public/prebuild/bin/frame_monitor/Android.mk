
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := frame_monitor
LOCAL_MODULE  := frame_monitor

LOCAL_MODULE_PATH := $(PRODUCT_OUT)/system/bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES

include $(BUILD_PREBUILT)
