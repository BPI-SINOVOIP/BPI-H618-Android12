LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests

LOCAL_SRC_FILES := $(call all-java-files-under, src) $(call all-renderscript-files-under, src)

LOCAL_PACKAGE_NAME := JavaNoDebugWaitAttach
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_PRIVATE_PLATFORM_APIS := true

include $(BUILD_PACKAGE)
