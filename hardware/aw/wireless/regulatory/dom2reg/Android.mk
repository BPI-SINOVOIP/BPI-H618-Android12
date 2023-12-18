LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := dom2reg
LOCAL_SRC_FILES := main.c
LOCAL_SHARED_LIBRARIES := libnl libcutils liblog
LOCAL_PROPRIETARY_MODULE := true
LOCAL_CFLAGS := -DCONFIG_LIBNL20
LOCAL_C_INCLUDES := external/libnl/include
LOCAL_C_INCLUDES += system/core/libutils/include
LOCAL_INIT_RC := dom2reg.rc
include $(BUILD_EXECUTABLE)
