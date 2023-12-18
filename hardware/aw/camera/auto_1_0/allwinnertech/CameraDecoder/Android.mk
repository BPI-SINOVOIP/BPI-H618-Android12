LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := libcamera_decorder
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
    CameraDecoder.cpp

LOCAL_C_INCLUDES += \
    frameworks/av/media/libcedarc/include

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libMemAdapter \
    libvencoder \
    libvdecoder \
    libvideoengine \
    libui \
    libdl \
    liblog

include $(BUILD_SHARED_LIBRARY)

