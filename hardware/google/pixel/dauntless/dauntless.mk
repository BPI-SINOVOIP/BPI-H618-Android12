# Dauntless
PRODUCT_PACKAGES += \
    citadeld \
    citadel_updater \
    android.hardware.weaver@1.0-service.citadel \
    android.hardware.identity@1.0-service.citadel

# AIDL
PRODUCT_PACKAGES += \
    android.hardware.security.keymint-service.citadel

PRODUCT_COPY_FILES += \
    hardware/google/pixel/dauntless/android.hardware.strongbox_keystore.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.strongbox_keystore.xml

# init scripts (won't be in AOSP)
-include vendor/google_nos/init/dauntless/init.mk

ifneq ($(wildcard vendor/google_nos/provision),)
PRODUCT_PACKAGES_DEBUG += CitadelProvision
endif
