LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := libjninodebugwaitattach
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice

LOCAL_SRC_FILES := jninodebugwaitattach.cpp simple.rscript

include frameworks/rs/tests/lldb/jni/common.mk
include $(BUILD_SHARED_LIBRARY)
