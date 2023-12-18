LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(call all-subdir-java-files)
LOCAL_PACKAGE_NAME := Test_AD
LOCAL_CERTIFICATE := platform
LOCAL_STATIC_JAVA_LIBRARIES := \
	    libadmanager
LOCAL_PRIVATE_PLATFORM_APIS := true
include $(BUILD_PACKAGE)
