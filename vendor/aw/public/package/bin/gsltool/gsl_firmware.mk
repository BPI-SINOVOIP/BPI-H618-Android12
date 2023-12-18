FIRMWARE_PATH := vendor/aw/public/package/bin/gsltool/firmware
RECOVERY_FIRMWARE_PATH := /root/vendor/etc/firmware
ifeq ($(GSLFIRMWARELIST),)
PRODUCT_COPY_FILES += \
    $(call find-copy-subdir-files,"*.bin",$(FIRMWARE_PATH),$(TARGET_COPY_OUT_VENDOR)/etc/firmware/gsl_firmware)

PRODUCT_COPY_FILES += \
    $(call find-copy-subdir-files,"*.bin",$(FIRMWARE_PATH),$(RECOVERY_FIRMWARE_PATH)/gsl_firmware)
else
PRODUCT_COPY_FILES += \
    $(foreach f,$(GSLFIRMWARELIST),$(FIRMWARE_PATH)/$(f).bin:$(TARGET_COPY_OUT_VENDOR)/etc/firmware/gsl_firmware/$(f).bin)

PRODUCT_COPY_FILES += \
    $(foreach f,$(GSLFIRMWARELIST),$(FIRMWARE_PATH)/$(f).bin:$(RECOVERY_FIRMWARE_PATH)/gsl_firmware/$(f).bin)

endif
