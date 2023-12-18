# Copyright (C) 2013 The Android Open Source Project
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
LOCAL_MODULE := aw_eq.ceres
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES_32 := lib32/libaw_eq.so
LOCAL_SRC_FILES_64 := lib64/libaw_eq.so
LOCAL_MULTILIB:= both
LOCAL_SHARED_LIBRARIES := libc++ libc libdl liblog libm
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
