ifeq ($(TARGET_ARCH),arm)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= ethpkt.c

LOCAL_MODULE_TAGS:= optional

LOCAL_MODULE:= ethpkt
LOCAL_SHARED_LIBRARIES := libcutils libnetutils

include $(BUILD_EXECUTABLE)

endif
