LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MANIFEST_FILE := app/AndroidManifest.xml

LOCAL_MODULE_TAGS := optional

LOCAL_STATIC_ANDROID_LIBRARIES := \
    androidx.legacy_legacy-support-v13 \
    androidx.legacy_legacy-support-v4 \
    androidx.core_core

LOCAL_STATIC_JAVA_LIBRARIES := android-ex-camera2-portability
LOCAL_STATIC_JAVA_LIBRARIES += xmp_toolkit
LOCAL_STATIC_JAVA_LIBRARIES += glide
LOCAL_STATIC_JAVA_LIBRARIES += guava
LOCAL_STATIC_JAVA_LIBRARIES += jsr305

LOCAL_USES_LIBRARIES := org.apache.http.legacy

LOCAL_SRC_FILES := $(call all-java-files-under, app/src)
LOCAL_SRC_FILES += $(call all-java-files-under, app/src_pd)
LOCAL_SRC_FILES += $(call all-java-files-under, app/src_pd_gcam)

LOCAL_RESOURCE_DIR += \
	$(LOCAL_PATH)/app/res \
	$(LOCAL_PATH)/app/res_p

include $(LOCAL_PATH)/version.mk

LOCAL_AAPT_FLAGS := \
        --auto-add-overlay \
        --version-name "$(version_name_package)" \
        --version-code $(version_code_package) \

LOCAL_USE_AAPT2 := true

LOCAL_PACKAGE_NAME := Camera2
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice

LOCAL_SDK_VERSION := current
LOCAL_CERTIFICATE := platform

LOCAL_PRODUCT_MODULE := true

LOCAL_PROGUARD_FLAG_FILES := proguard.flags

# Guava uses deprecated org.apache.http.legacy classes.
LOCAL_JAVA_LIBRARIES += org.apache.http.legacy

LOCAL_JNI_SHARED_LIBRARIES := libjni_tinyplanet libjni_jpegutil

include $(BUILD_PACKAGE)

include $(call all-makefiles-under, $(LOCAL_PATH))
