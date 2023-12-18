LOCAL_PATH := $(call my-dir)

# Install DisplayOutputManager Service
include $(CLEAR_VARS)
LOCAL_MODULE := DisplayDaemonService
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_SRC_FILES := DisplayDaemonService
# LOCAL_REQUIRED_MODULES := DisplayDaemon.jar
LOCAL_INIT_RC := DisplayDaemonService.rc
#LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_PREBUILT)

