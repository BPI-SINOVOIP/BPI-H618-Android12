LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_USE_AAPT2 := true

LOCAL_MODULE_TAGS := optional

LOCAL_PACKAGE_NAME := Update
LOCAL_STATIC_ANDROID_LIBRARIES := \
    bbcCoreSdk \

LOCAL_SRC_FILES := $(call all-java-files-under, app/src/main)
LOCAL_MANIFEST_FILE := app/src/main/AndroidManifest.xml
LOCAL_RESOURCE_DIR := $(LOCAL_PATH)/app/src/main/res

LOCAL_PRIVATE_PLATFORM_APIS := true
#LOCAL_SDK_VERSION := current
LOCAL_CERTIFICATE := platform
LOCAL_DEX_PREOPT := false

include $(BUILD_PACKAGE)

include $(CLEAR_VARS)
LOCAL_USE_AAPT2 := true
LOCAL_MODULE := bbcCoreSdk
LOCAL_SRC_FILES := $(call all-java-files-under, bbcCoreSdk/src/main)
LOCAL_MANIFEST_FILE := bbcCoreSdk/src/main/AndroidManifest.xml
LOCAL_RESOURCE_DIR := $(LOCAL_PATH)/bbcCoreSdk/src/main/res
LOCAL_PROGUARD_ENABLED := disabled
LOCAL_STATIC_ANDROID_LIBRARIES := \
    android-support-v7-appcompat \
    androidx.annotation_annotation

LOCAL_STATIC_JAVA_LIBRARIES := \
    bbc_aliyun-oos-sdk-android \
    bbc_gson \
    bbc_okhttp \
    bbc_okio \
    bbc_xutils
include $(BUILD_STATIC_JAVA_LIBRARY)

include $(CLEAR_VARS)
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES +=bbc_aliyun-oos-sdk-android:bbcCoreSdk/libs/aliyun-oss-sdk-android-2.2.0.jar
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES +=bbc_gson:bbcCoreSdk/libs/gson-2.8.0.jar
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES +=bbc_okhttp:bbcCoreSdk/libs/okhttp-3.2.0.jar
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES +=bbc_okio:bbcCoreSdk/libs/okio-1.6.0.jar
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES +=bbc_xutils:bbcCoreSdk/libs/xutils-3.3.40.jar
include $(BUILD_MULTI_PREBUILT)

