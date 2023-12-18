# Copyright 2020 The Android Open Source Project
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
    vendor/qcom/barbet/proprietary \

# AOSP packages required by the blobs
PRODUCT_PACKAGES := \
    ims \
    qcrilmsgtunnel \
    QtiTelephonyService

PRODUCT_PACKAGES += \
    libhoaeffects_csim \
    libimscamera_jni \
    libimsmedia_jni \
    lib-imsvideocodec \
    liblistensoundmodel2.qti \
    libmmosal \
    libqct_resampler \
    libvr_amb_engine \
    libvr_object_engine \

#  blob(s) necessary for barbet hardware
PRODUCT_COPY_FILES := \
     vendor/qcom/barbet/proprietary/com.qualcomm.qcrilmsgtunnel.xml:system_ext/etc/permissions/com.qualcomm.qcrilmsgtunnel.xml \
     vendor/qcom/barbet/proprietary/com.qti.media.secureprocessor.xml:system_ext/etc/permissions/com.qti.media.secureprocessor.xml \
     vendor/qcom/barbet/proprietary/com.qualcomm.qcrilmsgtunnel.xml:system_ext/etc/permissions/com.qualcomm.qcrilmsgtunnel.xml \
     vendor/qcom/barbet/proprietary/com.qualcomm.qti.imscmservice-V2.0-java.xml:system_ext/etc/permissions/com.qualcomm.qti.imscmservice-V2.0-java.xml \
     vendor/qcom/barbet/proprietary/com.qualcomm.qti.imscmservice-V2.1-java.xml:system_ext/etc/permissions/com.qualcomm.qti.imscmservice-V2.1-java.xml \
     vendor/qcom/barbet/proprietary/com.qualcomm.qti.imscmservice-V2.2-java.xml:system_ext/etc/permissions/com.qualcomm.qti.imscmservice-V2.2-java.xml \
     vendor/qcom/barbet/proprietary/org_codeaurora_ims.xml:system_ext/etc/permissions/org_codeaurora_ims.xml \
     vendor/qcom/barbet/proprietary/qcrilhook.xml:system_ext/etc/permissions/qcrilhook.xml \
     vendor/qcom/barbet/proprietary/com.qualcomm.qti.imscmservice-V2.0-java.jar:system_ext/framework/com.qualcomm.qti.imscmservice-V2.0-java.jar \
     vendor/qcom/barbet/proprietary/com.qualcomm.qti.imscmservice-V2.1-java.jar:system_ext/framework/com.qualcomm.qti.imscmservice-V2.1-java.jar \
     vendor/qcom/barbet/proprietary/com.qualcomm.qti.imscmservice-V2.2-java.jar:system_ext/framework/com.qualcomm.qti.imscmservice-V2.2-java.jar \
     vendor/qcom/barbet/proprietary/com.qualcomm.qti.uceservice-V2.0-java.jar:system_ext/framework/com.qualcomm.qti.uceservice-V2.0-java.jar \
     vendor/qcom/barbet/proprietary/com.qualcomm.qti.uceservice-V2.1-java.jar:system_ext/framework/com.qualcomm.qti.uceservice-V2.1-java.jar \
     vendor/qcom/barbet/proprietary/com.qualcomm.qti.uceservice-V2.2-java.jar:system_ext/framework/com.qualcomm.qti.uceservice-V2.2-java.jar \
     vendor/qcom/barbet/proprietary/com.quicinc.cne.api-V1.0-java.jar:system_ext/framework/com.quicinc.cne.api-V1.0-java.jar \
     vendor/qcom/barbet/proprietary/com.quicinc.cne.api-V1.1-java.jar:system_ext/framework/com.quicinc.cne.api-V1.1-java.jar \
     vendor/qcom/barbet/proprietary/com.quicinc.cne.constants-V1.0-java.jar:system_ext/framework/com.quicinc.cne.constants-V1.0-java.jar \
     vendor/qcom/barbet/proprietary/com.quicinc.cne.constants-V2.0-java.jar:system_ext/framework/com.quicinc.cne.constants-V2.0-java.jar \
     vendor/qcom/barbet/proprietary/com.quicinc.cne.constants-V2.1-java.jar:system_ext/framework/com.quicinc.cne.constants-V2.1-java.jar \
     vendor/qcom/barbet/proprietary/qcrilhook.jar:system_ext/framework/qcrilhook.jar \
     vendor/qcom/barbet/proprietary/qti-telephony-hidl-wrapper.jar:system_ext/framework/qti-telephony-hidl-wrapper.jar \
     vendor/qcom/barbet/proprietary/qti-telephony-utils.jar:system_ext/framework/qti-telephony-utils.jar \
     vendor/qcom/barbet/proprietary/vendor.qti.hardware.data.connection-V1.0-java.jar:system_ext/framework/vendor.qti.hardware.data.connection-V1.0-java.jar \
     vendor/qcom/barbet/proprietary/vendor.qti.hardware.data.dynamicdds-V1.0-java.jar:system_ext/framework/vendor.qti.hardware.data.dynamicdds-V1.0-java.jar \
     vendor/qcom/barbet/proprietary/vendor.qti.hardware.data.iwlan-V1.0-java.jar:system_ext/framework/vendor.qti.hardware.data.iwlan-V1.0-java.jar \
     vendor/qcom/barbet/proprietary/vendor.qti.hardware.data.latency-V1.0-java.jar:system_ext/framework/vendor.qti.hardware.data.latency-V1.0-java.jar \
     vendor/qcom/barbet/proprietary/vendor.qti.hardware.fingerprint-V1.0-java.jar:system_ext/framework/vendor.qti.hardware.fingerprint-V1.0-java.jar \
     vendor/qcom/barbet/proprietary/vendor.qti.ims.callinfo-V1.0-java.jar:system_ext/framework/vendor.qti.ims.callinfo-V1.0-java.jar \
     vendor/qcom/barbet/proprietary/vendor.qti.ims.rcsconfig-V1.0-java.jar:system_ext/framework/vendor.qti.ims.rcsconfig-V1.0-java.jar \
     vendor/qcom/barbet/proprietary/vendor.qti.latency-V2.0-java.jar:system_ext/framework/vendor.qti.latency-V2.0-java.jar \
     vendor/qcom/barbet/proprietary/vendor.qti.voiceprint-V1.0-java.jar:system_ext/framework/vendor.qti.voiceprint-V1.0-java.jar \
     vendor/qcom/barbet/proprietary/vendor.qti.data.factory-V1.0-java.jar:system_ext/framework/vendor.qti.data.factory-V1.0-java.jar \
     vendor/qcom/barbet/proprietary/vendor.qti.data.factory-V2.0-java.jar:system_ext/framework/vendor.qti.data.factory-V2.0-java.jar \
     vendor/qcom/barbet/proprietary/vendor.qti.data.factory-V2.1-java.jar:system_ext/framework/vendor.qti.data.factory-V2.1-java.jar \
     vendor/qcom/barbet/proprietary/vendor.qti.hardware.data.cne.internal.api-V1.0-java.jar:system_ext/framework/vendor.qti.hardware.data.cne.internal.api-V1.0-java.jar \
     vendor/qcom/barbet/proprietary/vendor.qti.hardware.data.cne.internal.constants-V1.0-java.jar:system_ext/framework/vendor.qti.hardware.data.cne.internal.constants-V1.0-java.jar \
     vendor/qcom/barbet/proprietary/vendor.qti.hardware.data.cne.internal.server-V1.0-java.jar:system_ext/framework/vendor.qti.hardware.data.cne.internal.server-V1.0-java.jar \
     vendor/qcom/barbet/proprietary/vendor.qti.hardware.data.qmi-V1.0-java.jar:system_ext/framework/vendor.qti.hardware.data.qmi-V1.0-java.jar \
     vendor/qcom/barbet/proprietary/vendor.qti.hardware.slmadapter-V1.0-java.jar:system_ext/framework/vendor.qti.hardware.slmadapter-V1.0-java.jar \
     vendor/qcom/barbet/proprietary/vendor.qti.ims.factory-V1.0-java.jar:system_ext/framework/vendor.qti.ims.factory-V1.0-java.jar \

