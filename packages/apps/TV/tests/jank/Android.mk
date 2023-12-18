LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests


# Include all test java files.
LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := TVJankTests
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice

LOCAL_STATIC_JAVA_LIBRARIES := \
    androidx.test.runner \
    tv-test-common \
    ub-janktesthelper \
    ub-uiautomator \

LOCAL_JAVA_LIBRARIES := android.test.base.stubs

LOCAL_INSTRUMENTATION_FOR := LiveTv

LOCAL_SDK_VERSION := system_current
LOCAL_PROGUARD_ENABLED := disabled

include $(BUILD_PACKAGE)
