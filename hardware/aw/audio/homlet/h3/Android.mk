# Copyright (C) 2011 The Android Open Source Project
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

include $(CLEAR_VARS)

#LOCAL_32_BIT_ONLY := true
#LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

LOCAL_MODULE := audio.primary.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := audio_hw.c
LOCAL_CFLAGS += -DHOMLET_PLATFORM
ifeq ($(KARAOK_PRODUCT), true)
	LOCAL_CFLAGS += -DKARAOK_AUDIO_DEVICE
endif

LOCAL_C_INCLUDES += \
    hardware/libhardware/include \
    system/media/audio_utils/include \
    system/media/audio_effects/include \
    system/media/audio_route/include \

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libcutils \
	libaudioutils \
	libdl \
	libaudioroute \
	libtinyalsa_audio

LOCAL_MODULE_TAGS := optional

LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under, $(LOCAL_PATH))

