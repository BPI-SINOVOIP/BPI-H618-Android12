# 
# Copyright (C) 2018 The Android Open Source Project
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

LOCAL_PATH := $(call my-dir)
ifeq ($(TARGET_BOARD_PLATFORM),epic)

include $(CLEAR_VARS)

LOCAL_MODULE               := hwcomposer.$(TARGET_BOARD_PLATFORM)
LOCAL_PROPRIETARY_MODULE   := true
LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_C_INCLUDES += \
		hardware/aw/display \
		hardware/aw/display/include  \
		hardware/libhardware/include \
		hardware/aw/display/interfaces/config/1.0/src

LOCAL_CFLAGS += -Wno-unused-parameter
LOCAL_CFLAGS += -DLOG_TAG=\"sunxihwc\"
LOCAL_CFLAGS += -DHWC2_USE_CPP11 -DHWC2_INCLUDE_STRINGIFICATION

LOCAL_STATIC_LIBRARIES := libhwc3-eink-static

LOCAL_SRC_FILES := \
		HWComposer.cpp \
		vendorservice/default/VendorServiceAdapter.cpp

ifeq ($(TARGET_PLATFORM), auto)
LOCAL_SRC_FILES += disp2/GeneralDeviceFactory.cpp
else
LOCAL_SRC_FILES += disp2/TabletDeviceFactory.cpp
endif

LOCAL_SHARED_LIBRARIES := \
		libhwc_config \
		libdisplayconfig \
		vendor.display.config@1.0 \
		vendor.display.config@1.0-impl

LOCAL_SHARED_LIBRARIES += \
		libbase     \
		libcutils   \
		libutils    \
		libui       \
		libion      \
		libsysutils \
		libsync     \
		libprocessgroup \
		liblog

include $(BUILD_SHARED_LIBRARY)

endif # end of sun50iw10p1
