LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    com_softwinner_utils_Config.cpp

LOCAL_SHARED_LIBRARIES := \
    libandroid_runtime \
    libnativehelper \
    libutils \
    libui \
    libcutils \
    libswconfig

LOCAL_STATIC_LIBRARIES :=

LOCAL_C_INCLUDES += \
    frameworks/base/core/jni \
    $(JNI_H_INCLUDE)

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= libconfig_jni

LOCAL_PRELINK_MODULE:= false

include $(BUILD_SHARED_LIBRARY)

