LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libsystemmix
LOCAL_SRC_FILES := SystemMix.java
LOCAL_PROGUARD_ENABLED := disabled
#LOCAL_JACK_ENABLED := disabled

include $(BUILD_STATIC_JAVA_LIBRARY)
