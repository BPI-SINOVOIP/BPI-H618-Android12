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

# This makefile shows how to build a shared library and an activity that
# bundles the shared library and calls it using JNI.

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

# Build activity
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := Socss

LOCAL_PROGUARD_ENABLED := disabled

#LOCAL_SDK_VERSION := current

LOCAL_DEX_PREOPT := false

LOCAL_STATIC_JAVA_LIBRARIES := android-support-v4 android-support-v7-appcompat

LOCAL_STATIC_JAVA_AAR_LIBRARIES := cameraview

LOCAL_AAPT_FLAGS := --auto-add-overlay
LOCAL_AAPT_FLAGS += --extra-packages com.google.android.cameraview

LOCAL_JNI_SHARED_LIBRARIES := \
    libsysintf_jni


LOCAL_PRIVATE_PLATFORM_APIS := true
include $(BUILD_PACKAGE)

include $(CLEAR_VARS)
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := \
    cameraview:libs/cameraview.aar
include $(BUILD_MULTI_PREBUILT)

# ============================================================

# Also build all of the sub-targets under this one: the shared library.
include $(call all-makefiles-under,$(LOCAL_PATH))

