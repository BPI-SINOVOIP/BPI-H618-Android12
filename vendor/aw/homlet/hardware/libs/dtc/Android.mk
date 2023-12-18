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

#LOCAL_SHARED_LIBRARIES:= libdtc libscript

LOCAL_SRC_FILES := \
	checks.c \
	data.c \
	dtc.c \
	flattree.c \
	fstree.c \
	livetree.c \
	srcpos.c \
	treesource.c \
	updatetree.c \
	util.c \
	script_parser/iniparser.c \
	script_parser/dictionary.c \
	script_parser/script.c \
	script_parser/script_to_dts.c


LOCAL_MODULE := libdtc
LOCAL_CFLAGS += -Wall
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
	checks.c \
	data.c \
	libdtc.c \
	flattree.c \
	fstree.c \
	livetree.c \
	srcpos.c \
	treesource.c \
	updatetree.c \
	util.c \
	script_parser/iniparser.c \
	script_parser/dictionary.c \
	script_parser/script.c \
	script_parser/script_to_dts.c


LOCAL_MODULE := libdtc_t
LOCAL_CFLAGS += -Wall
include $(BUILD_STATIC_LIBRARY)




