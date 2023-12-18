# Copyright (C) 2012 The Android Open Source Project
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
include $(CLEAR_VARS)

src_files :=toc0/SBrom_Certif_parser.c toc0/SBrom_Certif_verify.c toc0/verify_key_item.c

src_files +=  toc1/openssl_ext.c toc1/obj_dat.c toc1/a_int.c

LOCAL_C_INCLUDES += \
	external/boringssl/include \
	vendor/aw/homlet/hardware/include \
	vendor/aw/homlet/hardware/include/libboot

LOCAL_SRC_FILES :=$(src_files)

LOCAL_SRC_FILES += \
	sunxi_boot_api.c \
	libboot_utils.c \
	libboot_para.c \
	libboot_mmc.c \
	libboot_recovery.c \
	display_utils.c \
	libboot_info.c \
	toc0_verify.c \
	toc0_sw.c \
	toc1_verify.c \
	toc1_sw.c \
	alg_for_ota.c \
	boot_akernel.c \
	sunxi_boot_verify.c \
	image-android.c \
	key_ladder_verify.c

LOCAL_STATIC_LIBRARIES := libutils libcutils liblog
LOCAL_MODULE := libboot
LOCAL_CFLAGS += -Wall -std=c99
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)


LOCAL_C_INCLUDES += \
	vendor/aw/homlet/hardware/include \
	vendor/aw/homlet/hardware/include/libboot

LOCAL_SRC_FILES := \
	ubootmmc.c

LOCAL_STATIC_LIBRARIES := \
	libboot \
	libdtc_t \
	libc \
	libcrypto_static

LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_MODULE := libboot_t
LOCAL_CFLAGS += -Wall
include $(BUILD_EXECUTABLE)

# Create 'ubootparam' for debug
include $(CLEAR_VARS)
LOCAL_C_INCLUDES += \
	vendor/aw/homlet/hardware/include \
	vendor/aw/homlet/hardware/include/libboot

LOCAL_SRC_FILES  := ubootparam.c
LOCAL_STATIC_LIBRARIES := libboot libc libcrypto_static
LOCAL_STATIC_LIBRARIES += libutils libcutils liblog
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_MODULE := ubootparam
LOCAL_CFLAGS += -Wall -std=c99
include $(BUILD_EXECUTABLE)
