LOCAL_PATH:= $(call my-dir)
ifeq ($(strip $(LOCAL_PACKAGE_OVERRIDES)),)
# Use the following include to make our test apk.
include $(call all-makefiles-under, $(LOCAL_PATH))
endif
