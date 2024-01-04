LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:=               \
    IGpioService.cpp      \
    GpioService.cpp


LOCAL_SHARED_LIBRARIES :=     		\
	libcutils             			\
	libutils              			\
	libbinder             			\
	libandroid_runtime

LOCAL_C_INCLUDES += \
	libcore/include

LOCAL_MODULE_TAGS := optional

LOCAL_LDLIBS := -llog

LOCAL_MODULE:= libgpioservice

LOCAL_PRELINK_MODULE:= false

include $(BUILD_SHARED_LIBRARY)
