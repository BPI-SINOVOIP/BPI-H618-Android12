LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := TVTestInput
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_MODULE_TAGS := optional
LOCAL_PROGUARD_ENABLED := disabled
# Overlay view related functionality requires system APIs.
LOCAL_SDK_VERSION := system_current
LOCAL_PROGUARD_ENABLED := disabled

LOCAL_STATIC_JAVA_LIBRARIES := \
    tv-test-common \
    tv-common

# Disable dexpreopt and <uses-library> check for test.
LOCAL_ENFORCE_USES_LIBRARIES := false
LOCAL_DEX_PREOPT := false

LOCAL_RESOURCE_DIR := $(LOCAL_PATH)/../common/res $(LOCAL_PATH)/res
LOCAL_AAPT_FLAGS := --auto-add-overlay \
    --extra-packages com.android.tv.testing

include $(BUILD_PACKAGE)

ifneq ($(filter TV,$(TARGET_BUILD_APPS)),)
  $(call dist-for-goals,apps_only,$(LOCAL_BUILT_MODULE):$(LOCAL_PACKAGE_NAME).apk)
endif
