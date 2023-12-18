LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:=               \
    ISysIntfService.cpp      \
    SysIntfService.cpp


LOCAL_SHARED_LIBRARIES :=     		\
	libcutils             			\
	libutils              			\
	libbinder             			\
	libbase                         \
	libandroid_runtime

LOCAL_C_INCLUDES += \
	libcore/include \
	system/core/include/cutils \

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= libsysintfservice

LOCAL_LDLIBS := -llog

LOCAL_PRELINK_MODULE:= false

include $(BUILD_SHARED_LIBRARY)
