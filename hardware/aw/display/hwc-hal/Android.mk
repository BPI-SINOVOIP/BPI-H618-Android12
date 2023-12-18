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

ifneq ($(TARGET_BOARD_PLATFORM), epic)
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE               := hwcomposer.$(TARGET_BOARD_PLATFORM)
LOCAL_PROPRIETARY_MODULE   := true
LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_C_INCLUDES += \
		hardware/aw/display \
		hardware/aw/display/include  \
		hardware/libhardware/include \
		hardware/aw/display/interfaces/config/1.0/src

LOCAL_CFLAGS += -Wno-unused-parameter \
		-D_board_$(TARGET_BOARD_PLATFORM)_ \
		-DLOG_TAG=\"sunxihwc\" \
		-DHWC2_USE_CPP11 -DHWC2_INCLUDE_STRINGIFICATION

LOCAL_STATIC_LIBRARIES := libhwc3-static

LOCAL_SRC_FILES := \
		HWComposer.cpp

ifeq ($(TARGET_PLATFORM), auto)
	LOCAL_SRC_FILES += disp2/TabletDeviceFactory.cpp \
			vendorservice/default/VendorServiceAdapter.cpp
else ifeq ($(TARGET_PLATFORM), homlet)
	LOCAL_SRC_FILES += disp2/HomletDeviceFactory.cpp \
			   vendorservice/homlet/VendorServiceAdapter.cpp
else ifeq ($(PRODUCT_DEVICE), ceres-t3)
	LOCAL_SRC_FILES += disp2/GeneralDeviceFactory.cpp \
		   vendorservice/default/VendorServiceAdapter.cpp
else
	LOCAL_SRC_FILES += disp2/TabletDeviceFactory.cpp \
			   vendorservice/default/VendorServiceAdapter.cpp
endif

ifneq ($(WRITE_BACK_MODE), )
	LOCAL_CFLAGS += -DWRITE_BACK_MODE=$(WRITE_BACK_MODE)
	LOCAL_C_INCLUDES += hardware/aw/display/hwc-hal/writeback/disp2/include \
						hardware/aw/display/hwc-hal/writeback/render/include

ifneq ($(GPU_PUBLIC_INCLUDE),)
	LOCAL_C_INCLUDES += hardware/aw/gpu \
			    hardware/aw/gpu/include
	LOCAL_CFLAGS += -DGPU_PUBLIC_INCLUDE=\"$(GPU_PUBLIC_INCLUDE)\"
endif

LOCAL_SRC_FILES += writeback/WriteBackBufferPool.cpp \
				   writeback/WriteBackManager.cpp \
				   writeback/disp2/NormalWriteBackDisp2.cpp \
				   writeback/disp2/SelfWriteBackDisp2.cpp \
				   writeback/disp2/UtilsDisp2.cpp \
				   writeback/render/EGLImageBuffer.cpp \
				   writeback/render/EGLImageWrapper.cpp \
				   writeback/render/glengine.cpp \
				   writeback/render/ImageMapper.cpp \
				   writeback/render/KeystoneRender.cpp

LOCAL_SHARED_LIBRARIES += libGLESv2 libGLESv3 libui libEGL
endif

LOCAL_STATIC_LIBRARIES += libawhdr10p

LOCAL_SHARED_LIBRARIES += \
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
endif
