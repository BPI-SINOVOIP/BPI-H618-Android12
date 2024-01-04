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

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_STATIC_ANDROID_LIBRARIES := \
    androidx.legacy_legacy-support-v4
LOCAL_STATIC_JAVA_LIBRARIES := \
    libsystemmix \
    libgifplayer \

LOCAL_PROGUARD_FLAG_FILES := proguard-project.txt

LOCAL_PACKAGE_NAME := TvdFileManager
LOCAL_CERTIFICATE := platform
LOCAL_PRIVILEGED_MODULE := true
LOCAL_PRIVATE_PLATFORM_APIS := true
LOCAL_PROGUARD_ENABLED := disabled

LOCAL_OVERRIDES_PACKAGES := DocumentsUI

include $(BUILD_PACKAGE)
