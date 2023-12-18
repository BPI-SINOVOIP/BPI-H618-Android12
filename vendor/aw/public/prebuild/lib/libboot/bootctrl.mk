LOCAL_PATH := vendor/aw/public/prebuild/lib/libboot
# build source code
PRODUCT_PACKAGES += \
	bootctrl.default
#PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/bootctrl.default.so:$(TARGET_COPY_OUT_VENDOR)/lib/hw/bootctrl.default.so
