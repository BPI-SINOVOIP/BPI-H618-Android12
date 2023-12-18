LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    com_softwinner_multiir.cpp

LOCAL_SHARED_LIBRARIES := \
    libandroid_runtime \
    libnativehelper \
    libutils \
    libbinder \
    libui \
    libcutils \
    libmultiirservice

LOCAL_STATIC_LIBRARIES :=

LOCAL_C_INCLUDES += \
    frameworks/base/core/jni \
    $(LOCAL_PATH)/../libmultiir \
	libnativehelper/include/nativehelper

LOCAL_CFLAGS += -Wno-unused-parameter

LOCAL_MODULE_TAGS := optional

LOCAL_LDLIBS := -llog

LOCAL_MODULE:= libmultiir_jni

LOCAL_PRELINK_MODULE:= false

include $(BUILD_SHARED_LIBRARY)

