LOCAL_PATH := $(call my-dir)
#第三方的编译模块


#include $(CLEAR_VARS)
#LOCAL_MODULE := libjni_panoramiclib
#LOCAL_SRC_FILES := native-lib.cpp CroppImg.cpp ImgStitcher.cpp
#LOCAL_C_INCLUDES += $(LOCAL_PATH)/include
#LOCAL_SHARED_LIBRARIES := libopencv_java3
#LOCAL_NDK_STL_VARIANT := c++_shared
#LOCAL_PRODUCT_MODULE := true
#LOCAL_CPPFLAGS += -frtti -fexceptions -std=c++14 -lz
#LOCAL_CFLAGS += -Wall -Wextra -Werror -Wno-error=constant-conversion -Wno-unused-parameter -Wno-unused-variable -Wno-sign-compare -Wno-format-extra-args -Wno-sometimes-uninitialized
#LOCAL_PREBUILT_JNI_LIBS := $(LOCAL_PATH)/lib/armeabi-v7a/libopencv_java3.so
#LOCAL_PRELINK_MODULE:= false
#LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog -ljnigraphics
#include $(BUILD_SHARED_LIBRARY)
#include $(LOCAL_PATH)/lib/Android.mk
#include $(CLEAR_VARS)
#LOCAL_MODULE_SUFFIX := .so
#LOCAL_MODULE    := libyuv
#LOCAL_SRC_FILES := yuvutils/YUVLIB/lib/armeabi-v7a/libyuv.so
#LOCAL_MODULE_CLASS := SHARED_LIBRARIES
#include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libyuvutil
LOCAL_SRC_FILES := yuvutils/yuv_util.cpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/yuvutils/YUVLIB/include \
                    libnativehelper/include_jni
LOCAL_SHARED_LIBRARIES := libyuv
#LOCAL_PRODUCT_MODULE := true
#LOCAL_PREBUILT_JNI_LIBS := yuvutils/YUVLIB/lib/armeabi-v7a/libyuv.so
LOCAL_CPPFLAGS += -frtti -fexceptions
LOCAL_CFLAGS += -Wall -Wextra -Werror -Wno-error=constant-conversion -Wno-unused-parameter -Wno-unused-variable -Wno-sign-compare -Wno-format-extra-args
LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog
include $(BUILD_SHARED_LIBRARY)
#include $(LOCAL_PATH)/lib/Android.mk
