CONFIG_SUPPORT_GMS ?= true

ifeq ($(CONFIG_SUPPORT_GMS),true)
PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
    ro.com.google.clientidbase=android-allwinner
ifeq ($(CONFIG_LOW_RAM_DEVICE),true)
    $(call inherit-product-if-exists, vendor/partner_modules/build/mainline_modules_low_ram.mk)

    ifeq ($(CONFIG_LOW_RAM_2GB_DEVICE),true)
        # include gms package for 2gb go
        $(call inherit-product-if-exists, vendor/partner_gms/products/gms_go_2gb-mandatory.mk)
    else
        # include gms package for go
        $(call inherit-product-if-exists, vendor/partner_gms/products/gms_go-mandatory.mk)
    endif # ifeq ($(CONFIG_LOW_RAM_2GB_DEVICE),true))
else
    $(call inherit-product-if-exists, vendor/partner_modules/build/mainline_modules.mk)

    # include gms package
    $(call inherit-product-if-exists, vendor/partner_gms/products/gms-mandatory.mk)
endif # ifeq ($(CONFIG_LOW_RAM_DEVICE),true)
PRODUCT_PACKAGES += GooglePackageInstallerOverlay
endif # ifeq ($(CONFIG_SUPPORT_GMS),true)
