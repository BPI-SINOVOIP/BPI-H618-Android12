LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_JAVA_LIBRARIES := org.apache.http.legacy

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := playreadydemo

LOCAL_SDK_VERSION := current

LOCAL_CERTIFICATE := platform
include $(BUILD_PACKAGE)
