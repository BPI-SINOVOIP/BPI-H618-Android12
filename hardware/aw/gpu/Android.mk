#
# Copyright (C) 2017-2019 Allwinner Technology Co., Ltd. All rights reserved.
#
LOCAL_PATH := $(call my-dir)
LOCAL_SAVE := $(LOCAL_PATH)

$(call define-copy-target,$(PRODUCT_GPU_FILES))

LOCAL_PATH := $(LOCAL_SAVE)

include $(CLEAR_VARS)
LOCAL_MODULE := gpu-package
LOCAL_MODULE_OWNER := google
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(PRODUCT_GPU_PACKAGES)
LOCAL_REQUIRED_MODULES += $(call get-copy-target,$(PRODUCT_GPU_FILES))
LOCAL_REQUIRED_MODULES += gpu-properity
include $(BUILD_PHONY_PACKAGE)

include $(CLEAR_VARS)
LOCAL_MODULE := gpu-properity
LOCAL_MODULE_CLASS:= ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/etc/init
LOCAL_MODULE_STEM := init.gpu.property.rc

GEN := $(local-generated-sources-dir)/init.gpu.property.rc
$(GEN): $(LOCAL_PATH)/prop_gen.sh
	@echo "Generator: $@"
	@$< "$(PRODUCT_GPU_PROPERTIES)" $@

LOCAL_PREBUILT_MODULE_FILE := $(GEN)
include $(BUILD_PREBUILT)

include $(wildcard $(addsuffix /Android.mk, $(addprefix $(LOCAL_PATH)/,$(GPU_ARCH))))
