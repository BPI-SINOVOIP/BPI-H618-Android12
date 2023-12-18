LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_USE_AAPT2 := true

LOCAL_MODULE_TAGS := optional
LOCAL_PACKAGE_NAME := AWCamera
LOCAL_PRIVATE_PLATFORM_APIS := true
LOCAL_MULTILIB=32
LOCAL_STATIC_ANDROID_LIBRARIES := \
    android-support-v4 \
    android-support-v7-appcompat \
    android-support-v7-cardview \
    android-support-v7-recyclerview \
    android-support-v13 \
    android-support-design \

LOCAL_STATIC_JAVA_LIBRARIES := \
    eventbus-3.1.1 \
    eventbus-3.1.1-sources \
    fastjson-1.2.35 \
    okhttp-3.2.0

LOCAL_STATIC_JAVA_AAR_LIBRARIES := \
    aliyun-log-android-sdk-2.0.0 \
    core-0.0.5 \
    facedetection-0.0.5 \
    handgesturedetection-0.0.6 \
    mnn-0.0.4 \
    portraitsegmentation-0.0.6

LOCAL_AAPT_FLAGS := --auto-add-overlay \
                    --extra-packages com.alibaba.android.mnnkit.facedetection \
                    --extra-packages com.alibaba.android.mnnkit.handgesturedetection \
                    --extra-packages com.taobao.android.mnn \
                    --extra-packages com.aliyun.sls.android.sdk \
                    --extra-packages com.alibaba.android.mnnkit.core \
                    --extra-packages com.alibaba.android.mnnkit.portraitsegmentation \

LOCAL_JNI_SHARED_LIBRARIES := libyuvutil
LOCAL_SRC_FILES := $(call all-java-files-under, app/src/main)
LOCAL_MANIFEST_FILE := app/src/main/AndroidManifest.xml
LOCAL_RESOURCE_DIR := $(LOCAL_PATH)/app/src/main/res

LOCAL_ASSET_DIR := $(LOCAL_PATH)/app/src/main/assets

LOCAL_RESOURCE_DIR += prebuilts/sdk/current/support/v7/appcompat/res
LOCAL_CERTIFICATE := platform
LOCAL_OVERRIDES_PACKAGES := Camera2

LOCAL_PREBUILT_JNI_LIBS := \
    app/src/main/jniLibs/armeabi-v7a/libc++_shared.so \
    app/src/main/jniLibs/armeabi-v7a/libMNN.so \
    app/src/main/jniLibs/armeabi-v7a/libMNN_CL.so \
    app/src/main/jniLibs/armeabi-v7a/libmnnfacedetection.so \
    app/src/main/jniLibs/armeabi-v7a/libmnnhandgesturedetection.so \
    app/src/main/jniLibs/armeabi-v7a/libmnnkitcore.so \
    app/src/main/jniLibs/armeabi-v7a/libmnnportraitsegmentation.so \

LOCAL_DEX_PREOPT := false
LOCAL_PROGUARD_ENABLED := disabled
include $(BUILD_PACKAGE)

include $(CLEAR_VARS)
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES +=eventbus-3.1.1:app/libs/eventbus-3.1.1.jar
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES +=eventbus-3.1.1-sources:app/libs/eventbus-3.1.1-sources.jar
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES +=aliyun-log-android-sdk-2.0.0:app/libs/aliyun-log-android-sdk-2.0.0.aar
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES +=core-0.0.5:app/libs/core-0.0.5.aar
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES +=facedetection-0.0.5:app/libs/facedetection-0.0.5.aar
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES +=fastjson-1.2.35:app/libs/fastjson-1.2.35.jar
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES +=handgesturedetection-0.0.6:app/libs/handgesturedetection-0.0.6.aar
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES +=mnn-0.0.4:app/libs/mnn-0.0.4.aar
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES +=portraitsegmentation-0.0.6:app/libs/portraitsegmentation-0.0.6.aar
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES +=okhttp-3.2.0:app/libs/okhttp-3.2.0.jar
LOCAL_PROGUARD_ENABLED := disabled
LOCAL_DEX_PREOPT := false
include $(BUILD_MULTI_PREBUILT)

include $(LOCAL_PATH)/app/src/main/jni/Android.mk
