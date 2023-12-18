LOCAL_PATH:= $(call my-dir)

##################################################
include $(CLEAR_VARS)

LIB_ROOT=$(LOCAL_PATH)/..
include $(LIB_ROOT)/config.mk

LOCAL_MULTILIB := 32

#LOCAL_MODULE_TAGS := tests
#LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_TAGS := samples

LOCAL_STATIC_JAVA_LIBRARIES := com.softwinner.wfdsink android-support-v4

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := MiracastReceiver
LOCAL_CERTIFICATE := platform
#LOCAL_SDK_VERSION := current
ifeq ($(CONFIG_OS_VERSION), $(OPTION_OS_VERSION_ANDROID_9_0))
LOCAL_PRIVATE_PLATFORM_APIS := true
endif

ifeq ($(CONFIG_OS_VERSION), $(OPTION_OS_VERSION_ANDROID_10_0))
LOCAL_PRIVATE_PLATFORM_APIS := true
MIRACAST_SRC_FILES := $(call all-java-files-under, platform/Android10)
endif

ifeq ($(CONFIG_OS_VERSION), $(OPTION_OS_VERSION_ANDROID_11_0))
LOCAL_PRIVATE_PLATFORM_APIS := true
MIRACAST_SRC_FILES := $(call all-java-files-under, platform/Android11)
endif

ifeq ($(CONFIG_OS_VERSION), $(OPTION_OS_VERSION_ANDROID_12_0))
LOCAL_PRIVATE_PLATFORM_APIS := true
MIRACAST_SRC_FILES := $(call all-java-files-under, platform/Android12)
endif

ifeq ($(CONFIG_OS_VERSION), $(OPTION_OS_VERSION_ANDROID_13_0))
LOCAL_PRIVATE_PLATFORM_APIS := true
MIRACAST_SRC_FILES := $(call all-java-files-under, platform/Android13)
endif

ifneq ($(MIRACAST_SRC_FILES),)
    LOCAL_SRC_FILES += $(MIRACAST_SRC_FILES)
else
    # universal implement for un-configured platform
    $(warning DragonAging: No platform configured, use universal implement!)
    LOCAL_SRC_FILES += $(call all-java-files-under, platform/Universal)
endif

#LOCAL_JNI_SHARED_LIBRARIES := libjni_WFDManager libwfdmanager libwfdplayer libwfdrtsp

LOCAL_REQUIRED_MODULES := libjni_WFDManager \
                          libwfdmanager \
                          libwfdplayer \
                          libwfdrtsp \
                          libcdv_playback \
                          libcdv_output

LOCAL_PROGUARD_ENABLED := full optimization obfuscation
LOCAL_PROGUARD_FLAG_FILES := proguard.flags

include $(BUILD_PACKAGE)

##################################################
include $(LIB_ROOT)/lib/Android.mk
