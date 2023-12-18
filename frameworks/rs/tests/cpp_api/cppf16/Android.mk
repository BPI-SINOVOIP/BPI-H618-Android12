LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE:= rstest-cppf16
LOCAL_LICENSE_KINDS:= SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS:= notice

LOCAL_NDK_STL_VARIANT := c++_static

LOCAL_SRC_FILES:= \
	compute.cpp

LOCAL_STATIC_LIBRARIES := \
	libRScpp_static

LOCAL_LDFLAGS += -llog -ldl

include frameworks/rs/tests/cpp_api/common.mk
include $(BUILD_EXECUTABLE)
