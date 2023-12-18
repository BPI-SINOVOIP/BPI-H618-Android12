ifeq ($(call is-vendor-board-platform,QCOM),true)
ifneq ($(BUILD_WITHOUT_VENDOR),true)
include $(call all-named-subdir-makefiles,libbt-vendor)
endif
endif # is-vendor-board-platform
