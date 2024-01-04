PRODUCT_COPY_FILES += \
    $(PRODUCT_PREBUILT_PATH)/dist/init-input.ko:recovery/root/init-input.ko \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.touchscreen.multitouch.xml \

ifeq ($(USE_GSLFIRMWARE),true)
$(call inherit-product, vendor/aw/public/package/bin/gsltool/gsl_firmware.mk)
endif
PRODUCT_HOST_PACKAGES += gsltool

# Sensor hal 2.0
#PRODUCT_PACKAGES += android.hardware.sensors@2.0-service
