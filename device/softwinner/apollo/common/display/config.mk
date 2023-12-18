LOCAL_MODULE_PATH := $(shell dirname $(lastword $(MAKEFILE_LIST)))

PRODUCT_COPY_FILES += \
    $(LOCAL_MODULE_PATH)/init.display.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/init.display.rc

PRODUCT_PACKAGES += \
    gpu-package \

# Gralloc
PRODUCT_PACKAGES += \
	android.hardware.graphics.allocator@2.0-impl \
	android.hardware.graphics.allocator@2.0-service \
	android.hardware.graphics.mapper@2.0-impl-2.1 \

# HW Composer
PRODUCT_PACKAGES += \
    android.hardware.graphics.composer@2.2-impl \
    android.hardware.graphics.composer@2.2-service \
    hwcomposer.apollo \
    gralloc.apollo \
    libde201 \
    pqd

# ION
PRODUCT_PACKAGES += \
    libion

# Light Hal
PRODUCT_PACKAGES += \
    android.hardware.lights-service

#display - hdmi-hdcp2.2
PRODUCT_COPY_FILES += \
    device/softwinner/apollo/common/display/esm.fex:$(TARGET_COPY_OUT_VENDOR)/etc/hdcp/esm.fex \
    device/softwinner/apollo/common/display/hdcptool.sh:$(TARGET_COPY_OUT_VENDOR)/bin/hdcptool.sh


# display - hdmi-cec
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.hdmi.cec.xml:system/etc/permissions/android.hardware.hdmi.cec.xml
PRODUCT_PROPERTY_OVERRIDES += ro.hdmi.device_type=4
PRODUCT_SYSTEM_DEFAULT_PROPERTIES += persist.sys.hdmi.keep_awake=false

PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
    ro.hdmi.set_menu_language=1 \

PRODUCT_PACKAGES += \
    android.hardware.tv.cec@1.0-service \
    android.hardware.tv.cec@1.0-impl \
    hdmi_cec.apollo
