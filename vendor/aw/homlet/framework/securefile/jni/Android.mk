LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    com_softwinner_SecureFile.cpp

LOCAL_SHARED_LIBRARIES := \
    libandroid_runtime \
    libnativehelper \
    libutils \
    libbinder \
    libui \
    libcutils \
    libsecurefileservice

LOCAL_STATIC_LIBRARIES :=

LOCAL_C_INCLUDES += \
    frameworks/base/core/jni \
    $(LOCAL_PATH)/../libsecurefileservice \
    $(JNI_H_INCLUDE)

LOCAL_CFLAGS +=

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= libsecurefile_jni

LOCAL_PRELINK_MODULE:= false

include $(BUILD_SHARED_LIBRARY)
