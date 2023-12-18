LOCAL_PATH := $(call my-dir)

#######################
include $(CLEAR_VARS)

product = $(TARGET_BOARD_PLATFORM)

ifeq ($(product),)
    $(error No TARGET_BOARD_PLATFORM value found!)
endif

LOCAL_SRC_FILES := $(product)_cedarx.conf

ifeq ($(CONF_CMCC), yes)
    $(warning You have selected $(product) cmcc cedarx.conf)
    LOCAL_SRC_FILES := $(product)_cmcc_cedarx.conf
endif

FILE_EXIST := $(shell test -f $(TOP)/frameworks/av/media/libcedarx/conf/$(LOCAL_SRC_FILES) && echo yes)
ifneq ($(FILE_EXIST), yes)
    LOCAL_SRC_FILES := default_cedarx.conf
    $(warning You have selected default_cedarx.conf. Please provide specific platform conf.)
endif

LOCAL_MODULE := cedarx.conf
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/etc

LOCAL_MODULE_CLASS := ETC
include $(BUILD_PREBUILT)
