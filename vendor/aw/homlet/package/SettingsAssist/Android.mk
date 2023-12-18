LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := SettingsAssist
LOCAL_CERTIFICATE := platform
LOCAL_PRIVATE_PLATFORM_APIS := true
LOCAL_STATIC_ANDROID_LIBRARIES := \
    androidx.legacy_legacy-support-v4
LOCAL_STATIC_JAVA_LIBRARIES := \
	librecoverysystemex
LOCAL_PRIVILEGED_MODULE := true
include $(BUILD_PACKAGE)

# Use the folloing include to make our test apk.
include $(call all-makefiles-under,$(LOCAL_PATH))
