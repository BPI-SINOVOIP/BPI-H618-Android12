#####---A100---
ifeq (ceres,$(TARGET_BOARD_PLATFORM))
$(warning Use ceres awTuningApp!)
LIBISP_FLAGS = -Wall -Wno-unused-parameter -Wno-macro-redefined -Wno-unused-parameter -Wno-extra-tokens \
        -Wno-null-arithmetic -Wno-format -Wno-reorder -Wno-unused-variable -Wno-writable-strings \
        -Wno-logical-op-parentheses -Wno-sign-compare -Wno-unused-parameter -Wno-unused-value -Wno-incompatible-pointer-types \
        -Wno-unused-function -Wno-parentheses -Wno-extern-c-compat -Wno-null-conversion -Wno-error -Wno-date-time \
        -Wno-sometimes-uninitialized -Wno-gnu-designator -Wno-unused-label -Wno-pointer-arith -Wno-empty-body -fPIC

#########################################

ISP_PATH:= $(call my-dir)
include $(all-subdir-makefiles)
include $(CLEAR_VARS)

LOCAL_PATH := $(ISP_PATH)
LOCAL_CFLAGS += $(LIBISP_FLAGS) -lm -lc

LOCAL_MODULE := awTuningApp

#LOCAL_PROPRIETARY_MODULE := true
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS += -Wno-extern-c-compat -Wno-unused-variable -Wno-macro-redefined -Wno-unused-variable -Wno-unused-parameter -Wno-unknown-warning-option -Wno-sign-compare -Wno-empty-body -Wno-pointer-arith -Wno-incompatible-pointer-types-discards-qualifiers

LOCAL_SRC_FILES := \
    tuning_app/awTuningApp.c \
    tuning_app/log_handle.c \
    tuning_app/socket_protocol.c \
    tuning_app/thread_pool.c \
    tuning_app/server/capture_image.c \
    tuning_app/server/isp_handle.c \
    tuning_app/server/mini_shell.c \
    tuning_app/server/server.c \
    tuning_app/server/server_api.c \
    tuning_app/server/server_core.c \
    tuning_app/server/register_opt.c \
    tuning_app/server/raw_flow_opt.c \
    ../libisp_new/isp_cfg/isp_ini_parse.c \
    ../libisp_new/isp.c \
    ../libisp_new/isp_events/events.c \
    ../libisp_new/isp_tuning/isp_tuning.c \
    ../libisp_new/isp_manage/isp_helper.c \
    ../libisp_new/isp_manage/isp_manage.c

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../libisp_new/include/V4l2Camera \
    $(LOCAL_PATH)/../libisp_new/include/device \
    $(LOCAL_PATH)/../libisp_new/include \
    $(LOCAL_PATH)/../libisp_new/isp_dev \
    $(LOCAL_PATH)/../libisp_new/isp_tuning \
    $(LOCAL_PATH)/../libisp_new/isp_cfg/SENSOR_H \
    $(LOCAL_PATH)/tuning_app \
    $(LOCAL_PATH)/tuning_app/server \
    system/core/include/utils \
    system/core/include/log \
    system/core/include \
    $(LOCAL_PATH)/

LOCAL_STATIC_LIBRARIES += \
    libiniparser \
    libisp_dev \
    libisp_math

LOCAL_LDFLAGS_32 += \
    $(LOCAL_PATH)/../libisp_new/out/libisp_algo.a

LOCAL_LDFLAGS_64 += \
    $(LOCAL_PATH)/../libisp_new/out_64/libisp_algo.a

LOCAL_SHARED_LIBRARIES += \
    libcutils libutils liblog

include $(BUILD_EXECUTABLE)

#########################################

#####---T509---
else ifeq (pluto,$(TARGET_BOARD_PLATFORM))
$(warning Use pluto awTuningApp!)
LIBISP_FLAGS = -Wall -Wno-unused-parameter -Wno-macro-redefined -Wno-unused-parameter -Wno-extra-tokens \
        -Wno-null-arithmetic -Wno-format -Wno-reorder -Wno-unused-variable -Wno-writable-strings \
        -Wno-logical-op-parentheses -Wno-sign-compare -Wno-unused-parameter -Wno-unused-value -Wno-incompatible-pointer-types \
        -Wno-unused-function -Wno-parentheses -Wno-extern-c-compat -Wno-null-conversion -Wno-error -Wno-date-time \
        -Wno-sometimes-uninitialized -Wno-gnu-designator -Wno-unused-label -Wno-pointer-arith -Wno-empty-body -fPIC

#########################################

ISP_PATH:= $(call my-dir)
include $(all-subdir-makefiles)
include $(CLEAR_VARS)

LOCAL_PATH := $(ISP_PATH)
LOCAL_CFLAGS += $(LIBISP_FLAGS) -lm -lc

LOCAL_MODULE := awTuningApp

#LOCAL_PROPRIETARY_MODULE := true
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS += -Wno-extern-c-compat -Wno-unused-variable -Wno-macro-redefined -Wno-unused-variable -Wno-unused-parameter -Wno-unknown-warning-option -Wno-sign-compare -Wno-empty-body -Wno-pointer-arith -Wno-incompatible-pointer-types-discards-qualifiers

LOCAL_SRC_FILES := \
    tuning_app/awTuningApp.c \
    tuning_app/log_handle.c \
    tuning_app/socket_protocol.c \
    tuning_app/thread_pool.c \
    tuning_app/server/capture_image.c \
    tuning_app/server/isp_handle.c \
    tuning_app/server/mini_shell.c \
    tuning_app/server/server.c \
    tuning_app/server/server_api.c \
    tuning_app/server/server_core.c \
    tuning_app/server/register_opt.c \
    tuning_app/server/raw_flow_opt.c \
    ../libisp_new/isp_cfg/isp_ini_parse.c \
    ../libisp_new/isp.c \
    ../libisp_new/isp_events/events.c \
    ../libisp_new/isp_tuning/isp_tuning.c \
    ../libisp_new/isp_manage/isp_helper.c \
    ../libisp_new/isp_manage/isp_manage.c

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../libisp_new/include/V4l2Camera \
    $(LOCAL_PATH)/../libisp_new/include/device \
    $(LOCAL_PATH)/../libisp_new/include \
    $(LOCAL_PATH)/../libisp_new/isp_dev \
    $(LOCAL_PATH)/../libisp_new/isp_tuning \
    $(LOCAL_PATH)/../libisp_new/isp_cfg/SENSOR_H \
    $(LOCAL_PATH)/tuning_app \
    $(LOCAL_PATH)/tuning_app/server \
    system/core/include/utils \
    system/core/include/log \
    system/core/include \
    $(LOCAL_PATH)/

LOCAL_STATIC_LIBRARIES += \
    libiniparser \
    libisp_dev \
    libisp_math

LOCAL_LDFLAGS_32 += \
    $(LOCAL_PATH)/../libisp_new/out/libisp_algo.a

LOCAL_LDFLAGS_64 += \
    $(LOCAL_PATH)/../libisp_new/out_64/libisp_algo.a

LOCAL_SHARED_LIBRARIES += \
    libcutils libutils liblog

include $(BUILD_EXECUTABLE)

#########################################
endif
