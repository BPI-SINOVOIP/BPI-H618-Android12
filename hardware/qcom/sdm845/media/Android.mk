QCOM_MEDIA_ROOT := $(call my-dir)

#Compile these for all targets under QCOM_BOARD_PLATFORMS list.
ifeq ($(call is-board-platform-in-list, $(QCOM_BOARD_PLATFORMS)),true)
ifneq ($(BUILD_WITHOUT_VENDOR),true)
include $(QCOM_MEDIA_ROOT)/libstagefrighthw/Android.mk
include $(QCOM_MEDIA_ROOT)/mm-core/Android.mk
endif
endif

ifeq ($(call is-board-platform-in-list, $(MSM_VIDC_TARGET_LIST)),true)
ifneq ($(BUILD_WITHOUT_VENDOR),true)
include $(QCOM_MEDIA_ROOT)/libplatformconfig/Android.mk
include $(QCOM_MEDIA_ROOT)/mm-video-v4l2/Android.mk
include $(QCOM_MEDIA_ROOT)/libc2dcolorconvert/Android.mk
endif
endif
