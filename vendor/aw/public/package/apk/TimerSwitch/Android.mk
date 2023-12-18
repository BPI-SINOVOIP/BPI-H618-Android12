LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_STATIC_ANDROID_LIBRARIES := \
    androidx.appcompat_appcompat \
    androidx.recyclerview_recyclerview \
    androidx.recyclerview_recyclerview-selection \

LOCAL_PROGUARD_FLAG_FILES := proguard-project.txt

LOCAL_PACKAGE_NAME := TimerSwitch
LOCAL_MODULE_TAGS := optional
LOCAL_PROGUARD_ENABLED := disabled
LOCAL_CERTIFICATE := platform

LOCAL_PRIVATE_PLATFORM_APIS := true

include $(BUILD_PACKAGE)
