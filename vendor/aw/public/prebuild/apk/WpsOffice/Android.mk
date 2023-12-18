###############################################################################
# ESFileExplorer
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := WpsOffice
LOCAL_MODULE_TAGS := optional
LOCAL_CERTIFICATE := PRESIGNED
LOCAL_DEX_PREOPT := false
#LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/preinstall
LOCAL_MODULE_CLASS := APPS
LOCAL_PRODUCT_MODULE := true
LOCAL_MODULE_SUFFIX := $(COMMON_ANDROID_PACKAGE_SUFFIX)
LOCAL_SRC_FILES := moffice_12.9.2_2052_cn00563_multidex_1bb510d25a.apk
#LOCAL_PROPRIETARY_MODULE := true
LOCAL_ENFORCE_USES_LIBRARIES := false
include $(BUILD_PREBUILT)
