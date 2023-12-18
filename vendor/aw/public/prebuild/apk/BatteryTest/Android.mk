###############################################################################
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := BatteryTest
LOCAL_MODULE_TAGS := optional
LOCAL_CERTIFICATE := platform
LOCAL_DEX_PREOPT := false
LOCAL_MODULE_CLASS := APPS
LOCAL_MODULE_SUFFIX := $(COMMON_ANDROID_PACKAGE_SUFFIX)
LOCAL_SRC_FILES := BatteryTest.apk
LOCAL_INIT_RC := batterytest.rc
#LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_PREBUILT)
