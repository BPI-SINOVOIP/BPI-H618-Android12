LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests

LOCAL_SRC_FILES := $(call all-java-files-under, src) \
                   $(call all-renderscript-files-under, src)

LOCAL_PACKAGE_NAME := JNIKernelVariables
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_SDK_VERSION := current

LOCAL_JNI_SHARED_LIBRARIES := libjnikernelvariables

LOCAL_RENDERSCRIPT_FLAGS := -g -O0 -target-api 0

include $(BUILD_PACKAGE)
include $(LOCAL_PATH)/jnikernelvariables/Android.mk
