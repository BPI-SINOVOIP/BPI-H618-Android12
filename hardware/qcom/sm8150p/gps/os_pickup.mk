ifeq ($(PRODUCT_PLATFORM),sm8150p)
ifneq ($(BUILD_WITHOUT_VENDOR), true)
LOCAL_PATH := $(call my-dir)
subdir_makefiles=$(call first-makefiles-under,$(LOCAL_PATH))

SKIP_BUILD_DIRS +=    \
  hardware/qcom/sm8150p/display \
  hardware/qcom/sm8150p/media

SKIP_MAKEFILES := $(call all-named-subdir-makefiles, $(SKIP_BUILD_DIRS))
subdir_makefiles := $(filter-out $(SKIP_MAKEFILES), $(subdir_makefiles))

subdir_makefiles += hardware/qcom/sm8150/display/Android.mk \
                    hardware/qcom/sm8150/media/Android.mk

$(foreach mk,$(subdir_makefiles),$(info including $(mk) ...)$(eval include $(mk)))
endif
endif
