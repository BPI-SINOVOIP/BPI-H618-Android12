LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

CEDARX_ROOT=$(LOCAL_PATH)/../../../
include $(CEDARX_ROOT)/config.mk

LOCAL_SRC_FILES = \
                $(notdir $(wildcard $(LOCAL_PATH)/*.c))

LOCAL_C_INCLUDES:= \
    $(CEDARX_ROOT)/ \
    $(CEDARX_ROOT)/libcore \
    $(CEDARX_ROOT)/libcore/base/include \
    $(CEDARX_ROOT)/libcore/muxer/include \
    $(CEDARX_ROOT)/libcore/stream/include \
    $(CEDARX_ROOT)/external/include/aencoder \
    $(TOP)/frameworks/av/media/libcedarc/include \

LOCAL_CFLAGS += $(CDX_CFLAGS)

LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false

LOCAL_MODULE:= libcdx_muxer

LOCAL_SHARED_LIBRARIES = libcdx_stream libcdx_base

LOCAL_SHARED_LIBRARIES += libicuuc libutils liblog libcutils libz libdl libssl libcrypto libcdx_common

LOCAL_STATIC_LIBRARIES = \
	libcdx_mp4_muxer \
	libcdx_aac_muxer \
	libcdx_ts_muxer

ifeq ($(TARGET_ARCH),arm)
    LOCAL_CFLAGS += -Wno-psabi
endif

include $(BUILD_SHARED_LIBRARY)

