# Copyright 2018 The Android Open Source Project
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

PRODUCT_SOONG_NAMESPACES += \
    vendor/qcom/sargo/proprietary

# AOSP packages required by the blobs
PRODUCT_PACKAGES := \
    ims \
    QtiTelephonyService

#  blob(s) necessary for sargo hardware
PRODUCT_PACKAGES += \
    libadsprpc_system \
    libcdsprpc_system \
    libDiagService \
    libimscamera_jni \
    libimsmedia_jni \
    lib-imsvideocodec \
    lib-imsvtextutils \
    lib-imsvt \
    lib-imsvtutils \
    libmdsprpc_system \
    librcc \
    libsdm-disp-apis \
    libsdsprpc_system \
    libsecureui_svcsock_system \

PRODUCT_COPY_FILES := \
    vendor/qcom/sargo/proprietary/andsfCne.xml:system/etc/cne/andsfCne.xml:qcom \
    vendor/qcom/sargo/proprietary/qcrilhook.xml:system/etc/permissions/qcrilhook.xml:qcom \
    vendor/qcom/sargo/proprietary/telephonyservice.xml:system/etc/permissions/telephonyservice.xml:qcom \
    vendor/qcom/sargo/proprietary/uimremoteclient.xml:system/etc/permissions/uimremoteclient.xml:qcom \
    vendor/qcom/sargo/proprietary/uimremoteserver.xml:system/etc/permissions/uimremoteserver.xml:qcom \
    vendor/qcom/sargo/proprietary/UimService.xml:system/etc/permissions/UimService.xml:qcom \
    vendor/qcom/sargo/proprietary/qcrilhook.jar:system/framework/qcrilhook.jar:qcom \
    vendor/qcom/sargo/proprietary/QtiTelephonyServicelibrary.jar:system/framework/QtiTelephonyServicelibrary.jar:qcom \
    vendor/qcom/sargo/proprietary/uimremoteclientlibrary.jar:system/framework/uimremoteclientlibrary.jar:qcom \
    vendor/qcom/sargo/proprietary/uimremoteserverlibrary.jar:system/framework/uimremoteserverlibrary.jar:qcom \

