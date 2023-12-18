LOCAL_PATH := vendor/aw/public/prebuild/lib/librild

RIL_LIB_VERSION  ?= 12.0
RIL_LIB_BASENAME ?= libsoftwinner-ril-$(RIL_LIB_VERSION)

# 3G Data Card Packages
PRODUCT_PACKAGES += \
	CarrierConfig \
	android.hardware.radio@1.6 \
	android.hardware.radio.config@1.3 \
	pppd_vendor \
	rild \
	chat \
	$(RIL_LIB_BASENAME) \
	radio_monitor

DISABLE_RILD_OEM_HOOK := true

# 3G Data Card Configuration Flie
PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/config/data_call/ip-down:$(TARGET_COPY_OUT_VENDOR)/bin/etc/ppp/ip-down \
	$(LOCAL_PATH)/config/data_call/ip-up:$(TARGET_COPY_OUT_VENDOR)/bin/etc/ppp/ip-up \
	$(LOCAL_PATH)/config/data_call/3g_dongle.cfg:$(TARGET_COPY_OUT_VENDOR)/etc/3g_dongle.cfg \
	$(LOCAL_PATH)/config/data_call/apns-conf_sdk.xml:$(TARGET_COPY_OUT_PRODUCT)/etc/apns-conf.xml \
	$(LOCAL_PATH)/config/initrc/env.librild.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/env.librild.rc

# Radio Monitor Configuration Flie
PRODUCT_PACKAGES += usb_modeswitch
PRODUCT_COPY_FILES += \
	$(call find-copy-subdir-files,*,$(LOCAL_PATH)/config/radio_monitor/usb_modeswitch.d,$(TARGET_COPY_OUT_VENDOR)/etc/usb_modeswitch.d)
	#$(LOCAL_PATH)/config/radio_monitor/usb_modeswitch:$(TARGET_COPY_OUT_VENDOR)/bin/usb_modeswitch \

# Radio parameter
PRODUCT_PROPERTY_OVERRIDES += \
	vendor.rild.libargs=-d/dev/ttyUSB2 \
	vendor.rild.libpath=$(RIL_LIB_BASENAME).so \
	ro.radio.noril=true \
	ro.radio.noawril=false \
	ro.vendor.sw.embeded.telephony=false
