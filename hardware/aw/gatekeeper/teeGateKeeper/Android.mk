#
# Copyright (C) 2015 The Android Open-Source Project
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

# WARNING: Everything listed here will be built on ALL platforms,
# including x86, the emulator, and the SDK.  Modules must be uniquely
# named (liblights.panda), and must build everywhere, or limit themselves
# to only building on ARM if they include assembly. Individual makefiles
# are responsible for having their own logic, for fine-grained control.

LOCAL_PATH := $(call my-dir)

ifeq ($(BOARD_HAS_SECURE_OS), true)

include $(CLEAR_VARS)
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE := gatekeeper.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_CFLAGS := -Wall -Wextra -Werror -Wunused
LOCAL_SRC_FILES := \
    module_sunxi.cpp \
    trusty_sunxi_gatekeeper.cpp

LOCAL_C_INCLUDES := \
    $(TOP)/system/keymaster/include \
    $(TOP)/hardware/aw/optee_client/public

LOCAL_HEADER_LIBRARIES := libhardware_headers libutils_headers
LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcutils \
    libteec

include $(BUILD_SHARED_LIBRARY)
endif
