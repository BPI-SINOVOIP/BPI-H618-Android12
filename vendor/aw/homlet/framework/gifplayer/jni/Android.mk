LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
# $(warning "gifplayer jni compiling ...")
LOCAL_MODULE := libgifplayer_jni

LOCAL_SHARED_LIBRARIES := libnativehelper

LOCAL_C_INCLUDES := \
    external/giflib \
    frameworks/base/core/jni \
    libnativehelper/include/nativehelper

LOCAL_SRC_FILES := \
    PthreadSleep.cpp \
    SyncTime.cpp \
    GifPlayer.cpp \
    GifJni.cpp

LOCAL_LDLIBS := -lm -llog -ljnigraphics -landroid

LOCAL_USE_EMBEDDED_NATIVE_LIBS := true

LOCAL_STATIC_LIBRARIES := libgif

LOCAL_PROGUARD_ENABLED:= disabled

include $(BUILD_SHARED_LIBRARY)
