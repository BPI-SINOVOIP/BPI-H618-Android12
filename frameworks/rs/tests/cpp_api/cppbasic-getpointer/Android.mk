LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE:= rstest-compute-getpointer
LOCAL_LICENSE_KINDS:= SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS:= notice

LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

LOCAL_SRC_FILES:= \
	mono.rscript \
	compute.cpp

LOCAL_SHARED_LIBRARIES := \
	libRScpp

include frameworks/rs/tests/cpp_api/common.mk
include $(BUILD_EXECUTABLE)
