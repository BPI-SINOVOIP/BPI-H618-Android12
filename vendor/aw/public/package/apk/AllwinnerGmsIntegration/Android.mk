###############################################################################
# GmsSampleIntegration
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_PACKAGE_NAME := AllwinnerGmsIntegration
LOCAL_MODULE_TAGS := optional
LOCAL_CERTIFICATE := platform
LOCAL_OVERRIDES_PACKAGES := BullheadLayout
LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_SDK_VERSION := current
include $(BUILD_PACKAGE)
