LOCAL_MODULE_PATH := $(shell dirname $(lastword $(MAKEFILE_LIST)))

# PPPoE
PRODUCT_COPY_FILES += \
    $(LOCAL_MODULE_PATH)/android.software.pppoe.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.pppoe.xml \
    $(LOCAL_MODULE_PATH)/pppoe.rc:$(TARGET_COPY_OUT_SYSTEM)/etc/init/pppoe.rc

PRODUCT_COPY_FILES += \
    vendor/aw/homlet/external/pppoe/pppd/script/ip-up-pppoe:system/etc/ppp/ip-up-pppoe \
    vendor/aw/homlet/external/pppoe/pppd/script/ip-down-pppoe:system/etc/ppp/ip-down-pppoe \
    vendor/aw/homlet/external/pppoe/pppd/script/pppoe-options:system/etc/ppp/peers/pppoe-options \
    vendor/aw/homlet/external/pppoe/pppd/script/pppoe-connect:system/bin/pppoe-connect \
    vendor/aw/homlet/external/pppoe/pppd/script/pppoe-disconnect:system/bin/pppoe-disconnect

PRODUCT_PACKAGES += \
    pppoe \
    libpppoe-jni \
    pppoe-service

PRODUCT_SYSTEM_SERVER_JARS_EXTRA += \
    pppoe-service
