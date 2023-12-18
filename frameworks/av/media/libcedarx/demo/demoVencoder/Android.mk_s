LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_LDLIBS   := -lm -llog

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES:= EncoderTest.c

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/../../ \
    $(LOCAL_PATH)/../../libcore/base/include/ \
    $(LOCAL_PATH)/../../libcore/playback/include \
    $(LOCAL_PATH)/../../libcore/playback/ \
    $(LOCAL_PATH)/../../external/include/adecoder \
    $(LOCAL_PATH)/../../external/include/ \
    $(TOP)/frameworks/av/media/libcedarc/vencoder \
    $(TOP)/frameworks/av/media/libcedarc/include \

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    liblog \
    libvencoder \
    libcdx_base \
    libcdx_playback \
    libVE \
    libMemAdapter \

LOCAL_MODULE:= demoVenc

#include $(BUILD_SHARED_LIBRARY)
include $(BUILD_EXECUTABLE)
