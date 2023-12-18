LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
#OPENCV_LIB_TYPE := STATIC
APP_ABI := all
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := liballwinnertech_read_private

LOCAL_PROGUARD_ENABLED := disabled
LOCAL_SRC_FILES := native.c \

LOCAL_C_INCLUDES += \
    vendor/aw/homlet/hardware/include

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libsst

include $(BUILD_SHARED_LIBRARY)
