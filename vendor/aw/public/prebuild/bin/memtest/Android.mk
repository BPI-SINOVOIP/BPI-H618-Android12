LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := memtester
LOCAL_MODULE := memtester
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := memtester
LOCAL_MODULE := memtester_vendor
LOCAL_MODULE_STEM := memtester
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_PREBUILT)
