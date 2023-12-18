LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	main_gpioservice.cpp 

LOCAL_SHARED_LIBRARIES := \
	libgpioservice \
	libutils \
	liblog \
	libbinder
LOCAL_INIT_RC := gpioservice.rc
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../libgpio

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= gpioservice

include $(BUILD_EXECUTABLE)
