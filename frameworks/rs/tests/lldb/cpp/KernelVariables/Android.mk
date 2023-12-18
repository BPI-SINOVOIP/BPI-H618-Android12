LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := CppKernelVariables
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice

LOCAL_SRC_FILES := \
	KernelVariables.cpp \
	simple.rscript

include frameworks/rs/tests/lldb/cpp/common.mk
include $(BUILD_EXECUTABLE)
