LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

CEDARX_ROOT=$(LOCAL_PATH)/../../../
include $(CEDARX_ROOT)/config.mk

LOCAL_SRC_FILES = \
                $(notdir $(wildcard $(LOCAL_PATH)/*.c))

LOCAL_C_INCLUDES:= \
    ./\
    $(CEDARX_ROOT)/ \
    $(CEDARX_ROOT)/libcore \
    $(CEDARX_ROOT)/libcore/base/include \
    $(CEDARX_ROOT)/libcore/muxer/include \
    $(CEDARX_ROOT)/libcore/muxer/base \
    $(CEDARX_ROOT)/libcore/stream/include \
    $(CEDARX_ROOT)/external/include/aencoder \
    $(TOP)/frameworks/av/media/libcedarc/include \

LOCAL_CFLAGS += $(CDX_CFLAGS)

LOCAL_SHARED_LIBRARIES += libz
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false

LOCAL_MODULE := libcdx_aac_muxer

ifeq ($(TARGET_ARCH),arm)
    LOCAL_CFLAGS += -Wno-psabi
endif

include $(BUILD_STATIC_LIBRARY)


