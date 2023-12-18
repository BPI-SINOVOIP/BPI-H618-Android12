LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE:= rstest-cpp-globalguard
LOCAL_LICENSE_KINDS:= SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS:= notice

LOCAL_SDK_VERSION := 21
LOCAL_NDK_STL_VARIANT := c++_static

LOCAL_SRC_FILES:= \
	multiply.rscript \
	compute.cpp

LOCAL_STATIC_LIBRARIES := \
	libRScpp_static

LOCAL_LDFLAGS += -llog

include frameworks/rs/tests/cpp_api/common.mk
include $(BUILD_EXECUTABLE)
