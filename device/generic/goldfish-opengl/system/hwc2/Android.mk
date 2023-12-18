#
# Copyright 2015 The Android Open-Source Project
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

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_VENDOR_MODULE := true
emulator_hwcomposer_shared_libraries := \
    android.hardware.graphics.mapper@2.0 \
    android.hardware.graphics.mapper@4.0 \
    libbase \
    libEGL \
    libcutils \
    libcuttlefish_device_config \
    libcuttlefish_device_config_proto \
    libcuttlefish_utils \
    libcuttlefish_fs \
    libdrm \
    libgralloctypes \
    libhardware \
    libhidlbase \
    libjpeg \
    liblog \
    libsync \
    libui \
    libutils \
    libutils \

emulator_hwcomposer_cflags += \
    -DLOG_TAG=\"hwc2\" \
    -DPLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION) \
    -DANDROID_BASE_UNIQUE_FD_DISABLE_IMPLICIT_CONVERSION

emulator_hwcomposer_c_includes += \
    device/generic/goldfish-opengl/host/include/libOpenglRender \
    device/generic/goldfish-opengl/android-emu \
    device/generic/goldfish-opengl/shared/OpenglCodecCommon \
    device/generic/goldfish-opengl/system/OpenglSystemCommon \
    device/generic/goldfish-opengl/system/include \
    device/generic/goldfish-opengl/system/renderControl_enc \
    external/libdrm \
    system/core/libsync \
    system/core/libsync/include \

emulator_hwcomposer_relative_path := hw

emulator_hwcomposer2_src_files := \
    Device.cpp \
    Display.cpp \
    Drm.cpp \
    DrmPresenter.cpp \
    Gralloc.cpp \
    GuestComposer.cpp \
    HostComposer.cpp \
    Layer.cpp \

include $(CLEAR_VARS)

LOCAL_VENDOR_MODULE := true
LOCAL_STATIC_LIBRARIES := libyuv_static
LOCAL_SHARED_LIBRARIES := $(emulator_hwcomposer_shared_libraries)
LOCAL_SHARED_LIBRARIES += libOpenglSystemCommon lib_renderControl_enc
LOCAL_SHARED_LIBRARIES += libui
LOCAL_SRC_FILES := $(emulator_hwcomposer2_src_files)
LOCAL_C_INCLUDES := $(emulator_hwcomposer_c_includes)
LOCAL_C_INCLUDES += external/drm_hwcomposer
LOCAL_C_INCLUDES += external/minigbm/cros_gralloc
LOCAL_MODULE_RELATIVE_PATH := $(emulator_hwcomposer_relative_path)

LOCAL_MODULE := hwcomposer.ranchu
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/../../LICENSE
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_VENDOR_MODULE := true
LOCAL_SRC_FILES := drmTest.cpp
LOCAL_SHARED_LIBRARIES := $(emulator_hwcomposer_shared_libraries)
LOCAL_SHARED_LIBRARIES += libOpenglSystemCommon lib_renderControl_enc
LOCAL_SHARED_LIBRARIES += libui
LOCAL_SHARED_LIBRARIES += libdrm
LOCAL_C_INCLUDES := $(emulator_hwcomposer_c_includes)
LOCAL_C_INCLUDES += external/libdrm
LOCAL_C_INCLUDES += external/drm_hwcomposer
LOCAL_C_INCLUDES += external/minigbm/cros_gralloc
LOCAL_MODULE := emulatorDrmTest
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/../../LICENSE
include $(BUILD_EXECUTABLE)
