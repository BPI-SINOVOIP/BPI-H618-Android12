LIBISP_FLAGS = -Wall -Wno-unused-parameter -Wno-macro-redefined -Wno-unused-parameter -Wno-extra-tokens \
        -Wno-null-arithmetic -Wno-format -Wno-reorder -Wno-unused-variable -Wno-writable-strings \
        -Wno-logical-op-parentheses -Wno-sign-compare -Wno-unused-parameter -Wno-unused-value \
        -Wno-unused-function -Wno-parentheses -Wno-extern-c-compat -Wno-null-conversion -Wno-error \
        -Wno-sometimes-uninitialized -Wno-gnu-designator -Wno-unused-label -Wno-pointer-arith -Wno-empty-body -fPIC

#########################################

ISP_PATH:= $(call my-dir)
include $(all-subdir-makefiles)
include $(CLEAR_VARS)

LOCAL_PATH := $(ISP_PATH)
LOCAL_CFLAGS += $(LIBISP_FLAGS) -lm -lc

LOCAL_MODULE := libisp

LOCAL_PROPRIETARY_MODULE := true

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS += -Wno-extern-c-compat -Wno-unused-variable -Wno-macro-redefined -Wno-unused-variable -Wno-unused-parameter -Wno-unknown-warning-option -Wno-sign-compare -Wno-empty-body -Wno-pointer-arith -Wno-incompatible-pointer-types-discards-qualifiers

LOCAL_SRC_FILES := \
    isp.c \
    isp_events/events.c \
    isp_tuning/isp_tuning.c \
    isp_manage/isp_helper.c \
    isp_manage/isp_manage.c


LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/include/V4l2Camera \
    $(LOCAL_PATH)/include/device \
    $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/isp_dev \
    $(LOCAL_PATH)/isp_tuning \
    system/core/include/utils \
    system/core/include/log \
    system/core/include \
    $(LOCAL_PATH)/

LOCAL_STATIC_LIBRARIES += \
    libiniparser \
    libisp_dev \
	libisp_math

LOCAL_SHARED_LIBRARIES += \
    libisp_ini libcutils liblog

LOCAL_LDFLAGS_32 += \
	$(LOCAL_PATH)/out/libisp_algo.a

LOCAL_LDFLAGS_64 += \
	$(LOCAL_PATH)/out_64/libisp_algo.a

include $(BUILD_SHARED_LIBRARY)

