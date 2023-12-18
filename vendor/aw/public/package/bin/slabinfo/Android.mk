LOCAL_PATH:=$(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE:=slabinfo
LOCAL_MODULE_TAGS:=optional
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES:=\
	slabinfo.c
LOCAL_SHARED_LIBRARIES := libcutils

include $(BUILD_EXECUTABLE)
