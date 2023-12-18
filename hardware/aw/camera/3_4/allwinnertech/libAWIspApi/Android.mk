LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    AWIspApi.cpp

LOCAL_SHARED_LIBRARIES := \
    libisp libisp_ini libcutils liblog

ifeq (isp_new, $(ISP_VERSION))
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/libisp_new/include/V4l2Camera \
    $(LOCAL_PATH)/libisp_new/include/device \
    $(LOCAL_PATH)/libisp_new/include \
    $(LOCAL_PATH)/libisp_new/isp_dev \
    $(LOCAL_PATH)/libisp_new/isp_tuning \
    $(LOCAL_PATH)/libisp_new \
    system/core/include \
    $(LOCAL_PATH)/

else
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/libisp/include/V4l2Camera \
    $(LOCAL_PATH)/libisp/include/device \
    $(LOCAL_PATH)/libisp/include \
    $(LOCAL_PATH)/libisp/isp_dev \
    $(LOCAL_PATH)/libisp/isp_tuning \
    $(LOCAL_PATH)/libisp \
    system/core/include \
    $(LOCAL_PATH)/
endif

LOCAL_CFLAGS += -Wno-multichar -Wno-unused-parameter -Wno-incompatible-pointer-types-discards-qualifiers

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= libAWIspApi

LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
