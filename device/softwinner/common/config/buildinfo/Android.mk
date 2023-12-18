LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := buildinfo
LOCAL_MODULE_CLASS:= ETC
LOCAL_MODULE_PATH := $(PRODUCT_OUT)
LOCAL_IS_HOST_MODULE := true

GEN := $(local-generated-sources-dir)/$(LOCAL_MODULE)

$(GEN):
	@echo BOARD_ADD_PACK_CONFIG=$(BOARD_ADD_PACK_CONFIG)  > $@
	@echo PLATFORM_VERSION=$(PLATFORM_VERSION)           >> $@
	@echo PRODUCT_BOARD=$(PRODUCT_BOARD)                 >> $@
	@echo PRODUCT_MODEL=$(PRODUCT_MODEL)                 >> $@
	@echo TARGET_BOARD_CHIP=$(TARGET_BOARD_CHIP)         >> $@
	@echo TARGET_BOARD_IC=$(TARGET_BOARD_IC)             >> $@
	@echo TARGET_BOARD_KERN_VER=$(TARGET_BOARD_KERN_VER) >> $@
	@echo TARGET_DEVICE=$(TARGET_DEVICE)                 >> $@
	@echo TARGET_PRODUCT=$(TARGET_PRODUCT)               >> $@

LOCAL_PREBUILT_MODULE_FILE := $(GEN)

include $(BUILD_PREBUILT)
