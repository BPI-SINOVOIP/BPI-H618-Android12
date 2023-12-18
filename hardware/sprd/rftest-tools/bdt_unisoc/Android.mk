LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    bdt_unisoc.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../btsuite/main/include

LOCAL_MODULE:= bdt_unisoc-cli
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := bdt_unisoc
LOCAL_MODULE := bdt_unisoc
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_PREBUILT)
