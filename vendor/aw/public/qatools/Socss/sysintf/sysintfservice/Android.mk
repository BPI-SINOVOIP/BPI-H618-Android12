LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	main_sysintfservice.cpp 

LOCAL_SHARED_LIBRARIES := \
	libsysintfservice \
	libutils \
	libbinder \
	liblog

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../libsysintf

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= sysintfservice
LOCAL_INIT_RC := sysintf.rc
include $(BUILD_EXECUTABLE)
