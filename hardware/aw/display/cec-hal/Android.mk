
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES := liblog libutils libcutils libhardware  libsysutils
LOCAL_SRC_FILES := HdmiCecHal.cpp hdmi2.x/hdmi2x-cec-device.cpp

LOCAL_C_INCLUDES += $(TARGET_HARDWARE_INCLUDE)
LOCAL_C_INCLUDES += \
    system/core/include/ \
    hardware/aw/include

LOCAL_CFLAGS:= -DLOG_TAG=\"hdmi-cec\" -Wno-unused-parameter

LOCAL_MODULE               := hdmi_cec.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE   := true
LOCAL_MODULE_TAGS          := optional

include $(BUILD_SHARED_LIBRARY)
