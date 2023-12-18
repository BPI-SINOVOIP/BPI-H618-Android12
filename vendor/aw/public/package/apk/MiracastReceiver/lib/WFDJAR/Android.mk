LOCAL_PATH:= $(call my-dir)

##################################################
include $(CLEAR_VARS)

LIB_ROOT=$(LOCAL_PATH)/../..
include $(LIB_ROOT)/config.mk

LOCAL_MULTILIB := 32

ifneq ($(CONFIG_OS_VERSION), $(OPTION_OS_VERSION_ANDROID_10_0))
###LOCAL_MODULE_TAGS := eng
endif

ifneq ($(CONFIG_OS_VERSION), $(OPTION_OS_VERSION_ANDROID_11_0))
####LOCAL_MODULE_TAGS := eng
endif

ifneq ($(CONFIG_OS_VERSION), $(OPTION_OS_VERSION_ANDROID_12_0))
####LOCAL_MODULE_TAGS := eng
endif

ifneq ($(CONFIG_OS_VERSION), $(OPTION_OS_VERSION_ANDROID_13_0))
####LOCAL_MODULE_TAGS := eng
endif

#WITH_DEXPREOPT := false
LOCAL_DEX_PREOPT := false

#LOCAL_SRC_FILES := $(call all-subdir-java-files)
LOCAL_SRC_FILES := \
    WFDManager.java \
    WFDCallback.java \
    ViewCallback.java

ifeq ($(CONFIG_OS_VERSION), $(OPTION_OS_VERSION_ANDROID_4_4))
    LOCAL_SRC_FILES += WFDSinkView/Kitkat/WFDSinkView.java
else ifeq ($(CONFIG_OS_VERSION), $(OPTION_OS_VERSION_ANDROID_5_0))
    LOCAL_SRC_FILES += WFDSinkView/Lollipop/WFDSinkView.java
else
    LOCAL_SRC_FILES += WFDSinkView/Marshmallow/WFDSinkView.java
endif

LOCAL_MODULE := com.softwinner.wfdsink

LOCAL_PROGUARD_ENABLED := full
LOCAL_PROGUARD_FLAG_FILES := proguard.flags

# disable only when release jar
#ifeq ($(CONFIG_OS_VERSION), $(OPTION_OS_VERSION_ANDROID_4_4))
#else ifeq ($(CONFIG_OS_VERSION), $(OPTION_OS_VERSION_ANDROID_5_0))
#else
#    LOCAL_JACK_ENABLED := disabled
#endif

include $(BUILD_STATIC_JAVA_LIBRARY)

##################################################
include $(CLEAR_VARS)

LOCAL_MULTILIB := 32

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := com.softwinner.wfdsink.xml

LOCAL_MODULE_CLASS := ETC

LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/permissions

LOCAL_SRC_FILES := $(LOCAL_MODULE)

include $(BUILD_PREBUILT)

##################################################
