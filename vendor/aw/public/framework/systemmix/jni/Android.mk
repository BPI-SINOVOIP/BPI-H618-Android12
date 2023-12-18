LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    com_softwinner_SystemMix.cpp

LOCAL_SHARED_LIBRARIES := \
    libandroid_runtime \
    libnativehelper \
    libutils \
    libbinder \
    libui \
    libcutils \
    libsystemmixservice

LOCAL_STATIC_LIBRARIES :=

LOCAL_C_INCLUDES += \
    frameworks/base/core/jni \
    $(LOCAL_PATH)/../libsystemmix \
    $(JNI_H_INCLUDE) \
    system/core/include/cutils \
	libnativehelper/include/nativehelper

LOCAL_CFLAGS += -Wno-unused-parameter

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= libsystemmix_jni

LOCAL_LDLIBS := -llog

LOCAL_PRELINK_MODULE:= false

include $(BUILD_SHARED_LIBRARY)

