LOCAL_MODULE_PATH := $(shell dirname $(lastword $(MAKEFILE_LIST)))

# widevine config
BOARD_WIDEVINE_OEMCRYPTO_LEVEL := 3

BOARD_HAS_SECURE_OS ?= true
ifeq ($(BOARD_HAS_SECURE_OS), true)
SECURE_OS_OPTEE := yes
PRODUCT_PACKAGES += \
    libteec \
    tee_supplicant

# keymint version
BOARD_KEYMINT_VERSION ?= 1
KEYMINT_TA_NAME:=75ff9c58-92d0-4ff3-e7872176a1873cd4.ta
# keymaster version (0 or 2 or 4)
BOARD_KEYMASTER_VERSION ?= 4
SECURE_OS_SIGN_KEY_VER ?= 2

ifeq ($(SECURE_OS_SIGN_KEY_VER), 2)
TA_COPY_SUB_DIR = sign_key_v2/
else
TA_COPY_SUB_DIR = ./
endif

ifeq ($(BOARD_KEYMINT_VERSION),1)
# keymint ta
PRODUCT_COPY_FILES += \
    device/softwinner/common/optee_ta/$(TA_COPY_SUB_DIR)/$(KEYMINT_TA_NAME):$(TARGET_COPY_OUT_VENDOR)/lib/optee_armtz/$(KEYMINT_TA_NAME)

else ifeq ($(BOARD_KEYMASTER_VERSION), 0)
# keymaster ta
PRODUCT_COPY_FILES += \
    device/softwinner/common/optee_ta/$(TA_COPY_SUB_DIR)/d6bebe60-be3e-4046-b239891e0a594860.ta:$(TARGET_COPY_OUT_VENDOR)/lib/optee_armtz/d6bebe60-be3e-4046-b239891e0a594860.ta
else ifeq ($(BOARD_KEYMASTER_VERSION), 4)
PRODUCT_COPY_FILES += \
    device/softwinner/common/optee_ta/$(TA_COPY_SUB_DIR)/663d017b-102d-4fe0-c086523e1c754846.ta:$(TARGET_COPY_OUT_VENDOR)/lib/optee_armtz/663d017b-102d-4fe0-c086523e1c754846.ta

# keystore algorithm info
PRODUCT_PROPERTY_OVERRIDES += ro.hardware.keystore_desede=true
else
PRODUCT_COPY_FILES += \
    device/softwinner/common/optee_ta/$(TA_COPY_SUB_DIR)/f5f7b549-ba64-44fe-9b74f3fc357c7c61.ta:$(TARGET_COPY_OUT_VENDOR)/lib/optee_armtz/f5f7b549-ba64-44fe-9b74f3fc357c7c61.ta

# hardware keymaster hal
PRODUCT_PACKAGES += \
    keystore

endif

# gatekeeper ta
PRODUCT_COPY_FILES += \
    device/softwinner/common/optee_ta/$(TA_COPY_SUB_DIR)/2233b43b-cec6-449a-9509469f5023e425.ta:$(TARGET_COPY_OUT_VENDOR)/lib/optee_armtz/2233b43b-cec6-449a-9509469f5023e425.ta

ifeq ($(BOARD_WIDEVINE_OEMCRYPTO_LEVEL), 1)
PRODUCT_PACKAGES += \
    liboemcrypto
PRODUCT_COPY_FILES += \
    device/softwinner/common/optee_ta/$(TA_COPY_SUB_DIR)/a98befed-d679-ce4a-a3c827dcd51d21ed.ta:$(TARGET_COPY_OUT_VENDOR)/lib/optee_armtz/a98befed-d679-ce4a-a3c827dcd51d21ed.ta \
    device/softwinner/common/optee_ta/$(TA_COPY_SUB_DIR)/4d78d2ea-a631-70fb-aaa787c2b5773052.ta:$(TARGET_COPY_OUT_VENDOR)/lib/optee_armtz/4d78d2ea-a631-70fb-aaa787c2b5773052.ta \
    device/softwinner/common/optee_ta/$(TA_COPY_SUB_DIR)/e41f7029-c73c-344a-8c5bae90c7439a47.ta:$(TARGET_COPY_OUT_VENDOR)/lib/optee_armtz/e41f7029-c73c-344a-8c5bae90c7439a47.ta
endif # ifeq ($(BOARD_WIDEVINE_OEMCRYPTO_LEVEL), 1)

else # ifeq ($(BOARD_HAS_SECURE_OS), true)
SECURE_OS_OPTEE := no
# if has no secure os, widevine level must set to 3
BOARD_WIDEVINE_OEMCRYPTO_LEVEL := 3
endif # ifeq ($(BOARD_HAS_SECURE_OS), true)

PRODUCT_PACKAGES += \
    android.hardware.drm@1.0-impl \
    android.hardware.drm@1.4-service-lazy.widevine \
    android.hardware.drm@1.4-service-lazy.clearkey \

PRODUCT_PACKAGES += \
    libwvhidl \
    libvtswidevine \
    libwvdrmengine

# PRODUCT_COPY_FILES
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.software.verified_boot.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.verified_boot.xml \

$(call inherit-product, $(SRC_TARGET_DIR)/product/gsi_keys.mk)

# keymint HAL
PRODUCT_PACKAGES += android.hardware.security.keymint-service
# keymint support attest key, copy xml that announce we support this feature
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.keystore.app_attest_key.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.keystore.app_attest_key.xml



# new gatekeeper HAL
PRODUCT_PACKAGES += \
    android.hardware.gatekeeper@1.0-impl-aw \
    android.hardware.gatekeeper@1.0-service-aw \
    libgatekeeper \
    gatekeeper.apollo \
