LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
#OPENCV_LIB_TYPE := STATIC
APP_ABI := all
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := liballwinnertech_read_private_dragonbox

LOCAL_SHARED_LIBRARIES := libnativehelper

LOCAL_PROGUARD_ENABLED := disabled
LOCAL_SRC_FILES := native.c \

LOCAL_C_INCLUDES := \
    vendor/aw/homlet/hardware/include \
    hardware/aw/include \
    frameworks/base/core/jni \
    libnativehelper/include/nativehelper \
    libnativehelper/include_jni

LOCAL_USE_EMBEDDED_NATIVE_LIBS := true

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libsst

LOCAL_CFLAGS := -Wno-unused-parameter

include $(BUILD_SHARED_LIBRARY)
