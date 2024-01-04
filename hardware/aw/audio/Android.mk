MY_LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_PLATFORM),tablet)
    include $(call all-named-subdir-makefiles,tablet)
else ifeq ($(TARGET_PLATFORM),homlet)
    include $(call all-named-subdir-makefiles,homlet)
else ifeq ($(TARGET_PLATFORM),auto)
    include $(call all-named-subdir-makefiles,auto)
else
    include $(call all-named-subdir-makefiles,hal)
endif

include $(MY_LOCAL_PATH)/equalizer/Android.mk
ifneq ($(TARGET_BOARD_PLATFORM),apollo)
    include $(MY_LOCAL_PATH)/soundtrigger/Android.mk
endif
