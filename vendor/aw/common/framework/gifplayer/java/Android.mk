LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libgifplayer
LOCAL_SRC_FILES := GifPlayer.java
LOCAL_PROGUARD_ENABLED := disabled

include $(BUILD_STATIC_JAVA_LIBRARY)

