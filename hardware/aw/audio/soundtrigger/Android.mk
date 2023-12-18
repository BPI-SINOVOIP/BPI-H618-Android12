# Copyright (C) 2015 The Android Open Source Project
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

LOCAL_PATH := $(call my-dir)

# The default sound_trigger HAL module, which is a stub, that is loaded if no other
# device specific modules are present. The exact load order can be seen in
# libhardware/hardware.c
#
# The format of the name is sound_trigger.<type>.<hardware/etc>.so where the only
# required type is 'primary'.
include $(CLEAR_VARS)

LOCAL_MODULE := sound_trigger.primary.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := sound_trigger_hw.c
LOCAL_C_INCLUDES += \
    external/tinyalsa/include \
    hardware/libhardware/include \
    system/core/libsystem/include \
    system/media/audio/include
LOCAL_CFLAGS += -Wno-error=unused-variable -Wno-error=unused-function -Wno-error=unused-label -Wno-error=unused-value -Wno-error=unused-parameter -Wno-error=incompatible-pointer-types
LOCAL_SHARED_LIBRARIES := liblog libcutils libtinyalsa
LOCAL_MODULE_TAGS := optional
# LOCAL_32_BIT_ONLY := true

include $(BUILD_SHARED_LIBRARY)
