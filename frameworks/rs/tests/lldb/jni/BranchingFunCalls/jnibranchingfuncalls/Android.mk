LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := libjnibranchingfuncalls
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice

LOCAL_SRC_FILES := jnibranchingfuncalls.cpp scalars.rscript

LOCAL_RENDERSCRIPT_FLAGS := -g

include frameworks/rs/tests/lldb/jni/common.mk
include $(BUILD_SHARED_LIBRARY)
