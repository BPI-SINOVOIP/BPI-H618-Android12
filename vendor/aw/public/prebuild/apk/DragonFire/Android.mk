###############################################################################
# DragonFire
LOCAL_PATH := $(call my-dir)

##########################
include $(CLEAR_VARS)
LOCAL_MODULE := DragonComposite
LOCAL_MODULE_TAGS := optional
LOCAL_CERTIFICATE := platform
LOCAL_DEX_PREOPT := false
LOCAL_MODULE_CLASS := APPS
LOCAL_MODULE_SUFFIX := $(COMMON_ANDROID_PACKAGE_SUFFIX)
LOCAL_SRC_FILES := DragonComposite.apk
#LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_PREBUILT)

########################################
include $(CLEAR_VARS)
LOCAL_MODULE := VRDragonFire
LOCAL_MODULE_TAGS := optional
LOCAL_CERTIFICATE := platform
LOCAL_DEX_PREOPT := false
LOCAL_MODULE_CLASS := APPS
LOCAL_MODULE_SUFFIX := $(COMMON_ANDROID_PACKAGE_SUFFIX)
LOCAL_OVERRIDES_PACKAGES := DragonFire
LOCAL_SRC_FILES := VRDragonFire.apk
#LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_PREBUILT)

########################################
include $(CLEAR_VARS)
LOCAL_MODULE := SphereVideo
LOCAL_MODULE_TAGS := optional
LOCAL_CERTIFICATE := platform
LOCAL_DEX_PREOPT := false
LOCAL_MODULE_CLASS := APPS
LOCAL_MODULE_SUFFIX := $(COMMON_ANDROID_PACKAGE_SUFFIX)
LOCAL_SRC_FILES := SphereVideo.apk
#LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_PREBUILT)
