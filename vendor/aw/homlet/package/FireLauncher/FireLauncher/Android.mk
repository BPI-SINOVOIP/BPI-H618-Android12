LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := umeng-analytics-v5.2.4:libs/umeng-analytics-v5.2.4.jar
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES += contentlib:libs/contentlib.jar
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES += umeng-update-v2.4.2:libs/umeng-update-v2.4.2.jar
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES += volley_launcher:libs/volley.jar 
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES += fasterxml:libs/fasterxml.jar umeng-fb-v4.3.2:libs/umeng-fb-v4.3.2.jar
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES += universal-image-loader-1.9.2:libs/universal-image-loader-1.9.2.jar
include $(BUILD_MULTI_PREBUILT) 

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_STATIC_JAVA_LIBRARIES := android-support-v4 contentlib  umeng-analytics-v5.2.4  umeng-update-v2.4.2  \
							   volley_launcher  fasterxml   umeng-fb-v4.3.2       universal-image-loader-1.9.2 \
							   android-support-v7-appcompat android-support-v13 android-common
#LOCAL_STATIC_JAVA_LIBRARIES := libsysapi

appcompat_dir := ../../../../../prebuilts/sdk/current/support/v7/appcompat/res
view_page_dir := ../library/Android-ViewPagerIndicator/library
content_lib_dir := ../library/contentlib

res_dir := res $(view_page_dir)/res $(content_lib_dir)/res $(appcompat_dir)

LOCAL_SRC_FILES := $(call all-java-files-under, src) 
LOCAL_SRC_FILES += $(call all-java-files-under, $(view_page_dir)/src)
LOCAL_SRC_FILES += $(call all-java-files-under, $(content_lib_dir)/src)

LOCAL_RESOURCE_DIR := $(addprefix $(LOCAL_PATH)/, $(res_dir))

LOCAL_ASSET_DIR := $(LOCAL_PATH)/assets

LOCAL_AAPT_FLAGS := --auto-add-overlay
LOCAL_AAPT_FLAGS += --extra-packages android.support.v7.appcompat:com.viewpagerindicator:com.softwinner.firelauncher:com.android.launcher2:com.softwinner.animationview:com.softwinner.fireplayer

LOCAL_PACKAGE_NAME := FireLauncher
LOCAL_OVERRIDES_PACKAGES := Launcher2
# We mark this out until Mtp and MediaMetadataRetriever is unhidden.
LOCAL_SDK_VERSION := current
#LOCAL_CERTIFICATE := platform

LOCAL_PROGUARD_FLAG_FILES := proguard.cfg
LOCAL_PROGUARD_ENABLED := disabled

include $(BUILD_PACKAGE)
