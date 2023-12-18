LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := tv-test-common-robo
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice

LOCAL_SRC_FILES := \
    $(call all-java-files-under, src/com/android/tv/testing/robo) \
    $(call all-java-files-under, src/com/android/tv/testing/shadows)

LOCAL_RESOURCE_DIR := $(LOCAL_PATH)/res

LOCAL_STATIC_JAVA_LIBRARIES := \
    robolectric_android-all-stub \
    Robolectric_all-target \
    mockito-robolectric-prebuilt \
    tv-test-common \

# Disable dexpreopt and <uses-library> check for test.
LOCAL_ENFORCE_USES_LIBRARIES := false
LOCAL_DEX_PREOPT := false

LOCAL_INSTRUMENTATION_FOR := LiveTv

LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_JAVA_LIBRARY)
