PRODUCT_PACKAGES += \
	dispconfig \
	hwcdebug \
	parsedispconfig

ifeq ($(PRODUCT_SPECIAL_DISPLAYCONFIG),)
VENDOR_DISPLAY_DIR := hardware/aw/display
PRODUCT_COPY_FILES += $(call find-copy-subdir-files,*,$(VENDOR_DISPLAY_DIR)/configs/dispconfigs/,$(TARGET_COPY_OUT_VENDOR)/etc/dispconfigs)
endif
