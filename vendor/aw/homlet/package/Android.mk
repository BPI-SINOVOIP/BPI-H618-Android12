ifeq ($(TARGET_PLATFORM),homlet)
LOCAL_PATH := $(call my-dir)
include $(call all-makefiles-under,$(LOCAL_PATH))
endif
