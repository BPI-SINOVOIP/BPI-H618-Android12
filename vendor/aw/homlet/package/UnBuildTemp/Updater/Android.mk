LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_STATIC_JAVA_LIBRARIES := libprotocol-buffer android-support-v4 libxutilslib org.apache.http.legacy
LOCAL_PACKAGE_NAME := Update
LOCAL_CERTIFICATE := platform

#LOCAL_AAPT_FLAGS += -c mdpi

include $(BUILD_PACKAGE)
##################################################
include $(CLEAR_VARS)

LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := libprotocol-buffer:libs/protocol-buffer-java-2.5.0.jar

include $(BUILD_MULTI_PREBUILT)

##################################################
include $(CLEAR_VARS)

LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := libxutilslib:libs/xutilslib.jar

include $(BUILD_MULTI_PREBUILT)

# Use the folloing include to make our test apk.
include $(call all-makefiles-under,$(LOCAL_PATH))



