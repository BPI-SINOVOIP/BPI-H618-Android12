LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	main_isomountservice.cpp 

LOCAL_SHARED_LIBRARIES := \
	libisomountmanagerservice \
	libutils \
	liblog \
	libbinder
LOCAL_INIT_RC := isomountservice.rc
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../libisomount

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= isomountservice

include $(BUILD_EXECUTABLE)
