LOCAL_PATH:= $(call my-dir)

# TinyPlanet
include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cc
LOCAL_LDFLAGS   := -llog -ljnigraphics
LOCAL_SDK_VERSION := 17
LOCAL_MODULE    := libjni_tinyplanet
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/NOTICE
LOCAL_SRC_FILES := tinyplanet.cc
LOCAL_PRODUCT_MODULE := true

LOCAL_CFLAGS += -ffast-math -O3 -funroll-loops
LOCAL_CFLAGS += -Wall -Wextra -Werror
LOCAL_ARM_MODE := arm

include $(BUILD_SHARED_LIBRARY)

# JpegUtil
include $(CLEAR_VARS)

LOCAL_NDK_STL_VARIANT := c++_static
LOCAL_LDFLAGS   := -llog -ldl -ljnigraphics
LOCAL_SDK_VERSION := 17
LOCAL_MODULE    := libjni_jpegutil
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/NOTICE
LOCAL_PRODUCT_MODULE := true
LOCAL_SRC_FILES := jpegutil.cpp jpegutilnative.cpp

LOCAL_STATIC_LIBRARIES := libjpeg_static_ndk

LOCAL_CFLAGS += -ffast-math -O3 -funroll-loops
LOCAL_CFLAGS += -Wall -Wextra -Werror
LOCAL_ARM_MODE := arm

include $(BUILD_SHARED_LIBRARY)
