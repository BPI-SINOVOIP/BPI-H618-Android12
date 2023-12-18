LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

$(info local_path $(LOCAL_PATH))

# android(x) support libs
#LOCAL_STATIC_ANDROID_LIBRARIES := \
	android-support-v7-recyclerview \
	android-support-v7-appcompat

LOCAL_SRC_FILES := $(call all-java-files-under, java)

LOCAL_RESOURCE_DIR := $(LOCAL_PATH)/res

# don't let target sdk as 29, will limit by app sandbox, we need write ExternalStorage
LOCAL_PRIVATE_PLATFORM_APIS := true

#LOCAL_JNI_SHARED_LIBRARIES := libjni

LOCAL_PACKAGE_NAME := EinkLaunch
LOCAL_OVERRIDES_PACKAGES := Home Launcher2 Launcher3 Launcher3QuickStep Launcher3QuickStepGo

LOCAL_MODULE_TAGS := optional
LOCAL_PROGUARD_ENABLED := disabled
LOCAL_CERTIFICATE := platform

#LOCAL_PRODUCT_MODULE := true

include $(BUILD_PACKAGE)
