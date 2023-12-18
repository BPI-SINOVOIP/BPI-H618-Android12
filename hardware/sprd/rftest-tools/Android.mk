LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := unisoc-rftest-tools
LOCAL_MODULE_OWNER := unisoc
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := iwnpi bdt_unisoc libbt-sprd_suite hciattach unisoc-rftest-apk bdt_unisoc-cli
include $(BUILD_PHONY_PACKAGE)

include $(call all-makefiles-under,$(LOCAL_PATH))
