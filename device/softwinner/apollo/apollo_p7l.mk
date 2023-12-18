PRODUCT_PLATFORM_PATH := $(shell dirname $(lastword $(MAKEFILE_LIST)))

TARGET_BOARD_IC := h618
PRODUCT_BRAND := Allwinner
PRODUCT_NAME := apollo_p7l
PRODUCT_DEVICE := apollo-p7l
PRODUCT_BOARD := p7l
PRODUCT_MODEL := QUAD-CORE H618 p7l
PRODUCT_MANUFACTURER := Allwinner

PRODUCT_PREBUILT_PATH := longan/out/$(TARGET_BOARD_IC)/$(PRODUCT_BOARD)/android
PRODUCT_DEVICE_PATH := $(PRODUCT_PLATFORM_PATH)/$(PRODUCT_DEVICE)

PRODUCT_BUILD_VENDOR_BOOT_IMAGE := true

CONFIG_LOW_RAM_DEVICE := true
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

PRODUCT_PROPERTY_OVERRIDES += \
    ro.sf.lcd_density=240

PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
    ro.minui.default_rotation=ROTATION_NONE \
    ro.recovery.ui.touch_high_threshold=60

PRODUCT_HAS_UVC_CAMERA := true

PRODUCT_AAPT_CONFIG := mdpi xlarge hdpi xhdpi large
PRODUCT_AAPT_PREF_CONFIG :=xhdpi
PRODUCT_BROKEN_VERIFY_USES_LIBRARIES := true

GLOBAL_REMOVED_PACKAGES += \
    MmsService \
    Email \
    BasicDreams \
    vr

$(call inherit-product, $(PRODUCT_DEVICE_PATH)/*/config.mk)
$(call inherit-product, $(PRODUCT_PLATFORM_PATH)/common/*/config.mk)
