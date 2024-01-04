LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    com_softwinner_Gpio.cpp

LOCAL_SHARED_LIBRARIES := \
    libandroid_runtime \
    libnativehelper \
    libutils \
    libbinder \
    libui \
    libcutils \
    libgpioservice

LOCAL_STATIC_LIBRARIES :=

LOCAL_C_INCLUDES += \
    frameworks/base/core/jni \
    $(LOCAL_PATH)/../libgpio \
	libnativehelper/include/nativehelper

LOCAL_CFLAGS += -Wno-unused-parameter

LOCAL_MODULE_TAGS := optional

LOCAL_LDLIBS := -llog

LOCAL_MODULE:= libgpio_jni

LOCAL_PRELINK_MODULE:= false

include $(BUILD_SHARED_LIBRARY)

