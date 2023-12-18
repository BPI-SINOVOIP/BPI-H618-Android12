#
# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Caution: This file is used by NDK to generate all platform library files.
#          Please don't change this file to Android.bp.
LOCAL_PATH := $(call my-dir)

# Default cflags
MY_CFLAGS :=  -Wall -Werror -Wno-unused-parameter -D__ANDROID_ARCH__=\"$(TARGET_ARCH_ABI)\"

# If the TARGET_ARCH_ABI is 32bit, it adds __LIVE_ONLY_32BIT__ in MY_CFLAGS.
ABIS_FOR_32BIT_ONLY := armeabi-v7a armeabi x86 mips
ifneq ($(filter $(TARGET_ARCH_ABI),$(ABIS_FOR_32BIT_ONLY)),)
MY_CFLAGS += -D__LIVE_ONLY_32BIT__=1
endif

include $(CLEAR_VARS)

LOCAL_MODULE := libsplitappjni
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_SRC_FILES := com_android_cts_splitapp_Native.cpp

LOCAL_LDLIBS += -llog

LOCAL_CFLAGS := $(MY_CFLAGS)

# tag this module as a cts test artifact
LOCAL_COMPATIBILITY_SUITE := cts general-tests

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := libsplitappjni_revision
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_SRC_FILES := com_android_cts_splitapp_Native.cpp

LOCAL_LDLIBS += -llog

LOCAL_CFLAGS := $(MY_CFLAGS) -D__REVISION_HAVE_SUB__=1

# tag this module as a cts test artifact
LOCAL_COMPATIBILITY_SUITE := cts general-tests

include $(BUILD_SHARED_LIBRARY)
