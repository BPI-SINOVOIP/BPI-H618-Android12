ifeq ($(BOARD_HAVE_BLUETOOTH_XRADIO),true)
LOCAL_PATH := $(call my-dir)
include $(call all-subdir-makefiles)
endif
