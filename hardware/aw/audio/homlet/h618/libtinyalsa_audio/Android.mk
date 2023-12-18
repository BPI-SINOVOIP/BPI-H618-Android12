LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := libtinyalsa_audio
LOCAL_SRC_FILES := mixer.c pcm.c
LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_SHARED_LIBRARIES:= libcutils libutils liblog
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS += -Werror

include $(BUILD_SHARED_LIBRARY)

