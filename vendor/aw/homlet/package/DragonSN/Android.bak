 LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_STATIC_JAVA_LIBRARIES := jtds
LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PROGUARD_ENABLED := disabled

LOCAL_CERTIFICATE := platform
LOCAL_PACKAGE_NAME := DragonSN
LOCAL_SHARED_LIBRARIES := liblog liballwinnertech_read_private
LOCAL_REQUIRED_MODULES :=liballwinnertech_read_private
include $(BUILD_PACKAGE)

include $(CLEAR_VARS)
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := jtds:jtds-1.3.1.jar

include $(BUILD_MULTI_PREBUILT)

include $(call all-makefiles-under,$(LOCAL_PATH))
