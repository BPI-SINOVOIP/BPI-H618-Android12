LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	main_securefileserver.cpp 

LOCAL_SHARED_LIBRARIES := \
	libsecurefileservice \
	libutils \
	liblog \
	libbinder

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../libsecurefileservice

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= securefileserver

include $(BUILD_EXECUTABLE)
