PRODUCT_PLATFORM_PATH := $(shell dirname $(lastword $(MAKEFILE_LIST)))

TARGET_BOARD_IC := h618
PRODUCT_BRAND := Bananapi
PRODUCT_NAME := bananapi_m4berry
PRODUCT_DEVICE := bananapi-m4berry
PRODUCT_BOARD := m4berry
PRODUCT_MODEL := Bananapi M4Berry
PRODUCT_MANUFACTURER := Sinovoip

PRODUCT_PREBUILT_PATH := longan/out/$(TARGET_BOARD_IC)/$(PRODUCT_BOARD)/android
PRODUCT_DEVICE_PATH := $(PRODUCT_PLATFORM_PATH)/$(PRODUCT_DEVICE)

PRODUCT_BUILD_VENDOR_BOOT_IMAGE := true

CONFIG_LOW_RAM_DEVICE := false
CONFIG_SUPPORT_GMS := false
CONFIG_OTA_FROM_10 := false
BOARD_HAS_SECURE_OS := true

PRODUCT_COPY_FILES += $(PRODUCT_PREBUILT_PATH)/bImage:kernel

#set speaker project(true: double speaker, false: single speaker)
#set default eq
PRODUCT_PROPERTY_OVERRIDES += \
    ro.vendor.spk_dul.used=false \
    ro.vendor.audio.eq=false

PRODUCT_PACKAGES += DragonAtt
PRODUCT_PACKAGES += SoundRecorder

PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
    persist.sys.timezone=Asia/Shanghai \
    persist.sys.country=US \
    persist.sys.language=en

ifeq ($(BOARD_BUILD_BOX),true)
PRODUCT_PROPERTY_OVERRIDES += \
    ro.sf.lcd_density=280
else
PRODUCT_PROPERTY_OVERRIDES += \
    ro.sf.lcd_density=213
endif

PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
    ro.minui.default_rotation=ROTATION_NONE \
    ro.recovery.ui.touch_high_threshold=60

PRODUCT_HAS_UVC_CAMERA := true

PRODUCT_AAPT_CONFIG := mdpi xlarge hdpi xhdpi large
PRODUCT_AAPT_PREF_CONFIG :=xhdpi


$(call inherit-product, $(PRODUCT_DEVICE_PATH)/*/config.mk)
$(call inherit-product, $(PRODUCT_PLATFORM_PATH)/common/*/config.mk)
$(call inherit-product, vendor/bananapi/apps/apps.mk)
