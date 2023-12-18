
LOCAL_PATH := $(call my-dir)

### charger_res_images ###
ifneq ($(strip $(LOCAL_CHARGER_NO_UI)),true)
include $(CLEAR_VARS)
LOCAL_MODULE := aw_charger_res_animation
LOCAL_MODULE_STEM := animation.txt
LOCAL_SRC_FILES := animation.txt
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_PRODUCT_ETC)/res/values/charger
include $(BUILD_PREBUILT)

define _add-charger-image
include $$(CLEAR_VARS)
LOCAL_MODULE := aw_charger_res_images_$(notdir $(1))
LOCAL_MODULE_STEM := $(notdir $(1))
_img_modules += $$(LOCAL_MODULE)
LOCAL_SRC_FILES := $1
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $$(TARGET_OUT_PRODUCT_ETC)/res/images/charger
include $$(BUILD_PREBUILT)
endef

_img_modules :=
_images :=
$(foreach _img, $(call find-subdir-subdir-files, "images", "*.png"), \
  $(eval $(call _add-charger-image,$(_img))))

include $(CLEAR_VARS)
LOCAL_MODULE := aw_charger_res_images
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(_img_modules) aw_charger_res_animation
include $(BUILD_PHONY_PACKAGE)

_add-charger-image :=
_img_modules :=
endif # LOCAL_CHARGER_NO_UI
