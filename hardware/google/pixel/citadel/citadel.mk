# Citadel
PRODUCT_PACKAGES += \
    citadeld \
    citadel_updater \
    android.hardware.authsecret@1.0-service.citadel \
    android.hardware.oemlock@1.0-service.citadel \
    android.hardware.weaver@1.0-service.citadel \
    android.hardware.keymaster@4.1-service.citadel \
    android.hardware.identity@1.0-service.citadel \
    android.hardware.fastboot@1.1-impl.pixel \
    wait_for_strongbox

# Citadel debug stuff
PRODUCT_PACKAGES_DEBUG += \
    test_citadel

# Resume on Reboot support
PRODUCT_PACKAGES += \
    android.hardware.rebootescrow-service.citadel

# init scripts (won't be in AOSP)
-include vendor/google_nos/init/citadel/init.mk

ifneq ($(wildcard vendor/google_nos/provision),)
PRODUCT_PACKAGES_DEBUG += CitadelProvision

# Set CITADEL_LAZY_PSK_SYNC to true on projects with faceauth, otherwise false.
#
#      EVT devices left the factory without being provisioned,
#      and thus the shared authtoken key is yet to be established.
#      Since faceauth HAT enforcement fails without the preshared
#      authtoken, auto-sync it in the field for userdebug/eng.
#      Please refer to b/135295587 for more detail.
#
CITADEL_LAZY_PSK_SYNC := false
endif

# Sepolicy
BOARD_SEPOLICY_DIRS += hardware/google/pixel-sepolicy/citadel
