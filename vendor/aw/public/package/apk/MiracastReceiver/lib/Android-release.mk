LOCAL_PATH:= $(call my-dir)

##################################################
include $(CLEAR_VARS)

LIB_ROOT=$(LOCAL_PATH)/..
include $(LIB_ROOT)/config.mk

LOCAL_MULTILIB := 32

LOCAL_MODULE_TAGS := eng

#WITH_DEXPREOPT := false
LOCAL_DEX_PREOPT := false

ifeq ($(CONFIG_OS_VERSION), $(OPTION_OS_VERSION_ANDROID_4_4))
    LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := com.softwinner.wfdsink:WFDJAR/Kitkat/com.softwinner.wfdsink.jar
else ifeq ($(CONFIG_OS_VERSION), $(OPTION_OS_VERSION_ANDROID_5_0))
    LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := com.softwinner.wfdsink:WFDJAR/Lollipop/com.softwinner.wfdsink.jar
else
    LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := com.softwinner.wfdsink:WFDJAR/Marshmallow/com.softwinner.wfdsink.jar
endif

LOCAL_PREBUILT_LIBS := \
    libjni_WFDManager:libjni_WFDManager.so \
    libwfdmanager:libwfdmanager.so \
    libwfdrtsp:libwfdrtsp.so

ifeq ($(CONFIG_OS_VERSION), $(OPTION_OS_VERSION_ANDROID_4_4))
    LOCAL_PREBUILT_LIBS += libwfdplayer:WFDPlayer/Kitkat/libwfdplayer.so
else
    LOCAL_PREBUILT_LIBS += libwfdplayer:WFDPlayer/Lollipop/libwfdplayer.so
endif

include $(BUILD_MULTI_PREBUILT)

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