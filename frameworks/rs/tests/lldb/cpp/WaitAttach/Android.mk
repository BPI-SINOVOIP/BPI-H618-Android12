LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := CppDebugWaitAttach
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice

LOCAL_SRC_FILES := \
	WaitAttach.cpp \
	simple.rscript

include frameworks/rs/tests/lldb/cpp/common.mk
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_MODULE := CppNoDebugWaitAttach
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice

LOCAL_SRC_FILES := \
	WaitAttach.cpp \
	simple.rscript

include frameworks/rs/tests/lldb/cpp/common.mk
LOCAL_RENDERSCRIPT_FLAGS := $(filter-out -g,$(LOCAL_RENDERSCRIPT_FLAGS))
include $(BUILD_EXECUTABLE)
