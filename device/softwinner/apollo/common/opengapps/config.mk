BOARD_BUILD_OPENGAPPS := false

ifeq ($(BOARD_BUILD_OPENGAPPS), true)
GAPPS_VARIANT := pico
GAPPS_PRODUCT_PACKAGES += Chrome
GAPPS_EXCLUDED_PACKAGES := SetupWizard

# opengapps, github.com/opengapps/aosp_build
$(call inherit-product-if-exists, vendor/opengapps/build/opengapps-packages.mk)

PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
	ro.build.fingerprint=google/redfin/redfin:11/RQ3A.211001.001/7641976:user/release-keys
	
PRODUCT_BROKEN_VERIFY_USES_LIBRARIES := true
DONT_DEXPREOPT_PREBUILTS := true
endif
