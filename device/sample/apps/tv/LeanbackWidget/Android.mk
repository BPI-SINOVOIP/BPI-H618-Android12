LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(PRODUCT_OUT)/system/app

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := LeanbackWidget
LOCAL_LICENSE_KINDS := legacy_notice
LOCAL_LICENSE_CONDITIONS := notice

LOCAL_SDK_VERSION := current

include $(BUILD_PACKAGE)
