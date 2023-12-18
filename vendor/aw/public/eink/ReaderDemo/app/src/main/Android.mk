LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

$(info local_path $(LOCAL_PATH))

# android(x) support libs
LOCAL_STATIC_ANDROID_LIBRARIES := \
	android-support-v7-appcompat

LOCAL_STATIC_JAVA_LIBRARIES := \
	libeink

LOCAL_SRC_FILES := $(call all-java-files-under, java)

LOCAL_RESOURCE_DIR := $(LOCAL_PATH)/res

# don't let target sdk as 29, will limit by app sandbox, we need write ExternalStorage
#LOCAL_SDK_VERSION := current
LOCAL_PRIVATE_PLATFORM_APIS := true

LOCAL_AAPT_FLAGS := --auto-add-overlay

#LOCAL_JNI_SHARED_LIBRARIES := libjni

LOCAL_PACKAGE_NAME := ReaderDemo

LOCAL_MODULE_TAGS := optional
LOCAL_PROGUARD_ENABLED := disabled
LOCAL_CERTIFICATE := platform

#Specifies both LOCAL_SDK_VERSION (system_current) and LOCAL_PRIVATE_PLATFORM_APIS (true) but should specify only one
#LOCAL_PRODUCT_MODULE means  LOCAL_SDK_VERSION is system_current
#LOCAL_PRODUCT_MODULE := true
LOCAL_SYSTEM_EXT_MODULE := true

include $(BUILD_PACKAGE)
