LOCAL_MODULE_PATH := $(shell dirname $(lastword $(MAKEFILE_LIST)))

USE_XML_AUDIO_POLICY_CONF := 1

# properties
# audio default output standby_ms
# PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
#    ro.audio.flinger_standbytime_ms=50

# PRODUCT_PACKAGES
# Audio
PRODUCT_PACKAGES += \
    audio.primary.apollo \
    android.hardware.audio@2.0-service \
    android.hardware.audio@7.0-impl \
    android.hardware.audio.effect@7.0-impl \

# Audio

# PRODUCT_COPY_FILES
$(call inherit-product, $(LOCAL_MODULE_PATH)/sounds/AudioPackage.mk)
AUDIO_CONFIG_PATH := $(LOCAL_MODULE_PATH)/audio
PRODUCT_COPY_FILES += \
    frameworks/av/services/audiopolicy/config/usb_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/usb_audio_policy_configuration.xml \
    hardware/libhardware_legacy/audio/audio_policy.conf:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy.conf \
    $(AUDIO_CONFIG_PATH)/audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_configuration.xml \
    $(AUDIO_CONFIG_PATH)/audio_policy_volumes_drc.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_volumes_drc.xml \
    $(AUDIO_CONFIG_PATH)/audio_platform_info.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_platform_info.xml \
    $(AUDIO_CONFIG_PATH)/audio_effects.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_effects.xml \
    $(AUDIO_CONFIG_PATH)/audio_mixer_paths.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_mixer_paths.xml \
    hardware/aw/audio/equalizer/awequal.conf:$(TARGET_COPY_OUT_VENDOR)/etc/awequal.conf \

# setting default audio output/input
PRODUCT_PROPERTY_OVERRIDES += \
    vendor.audio.output.active=AUDIO_CODEC,AUDIO_HDMI \
    vendor.audio.input.active=AUDIO_AC107

# codec
CODEC_CONFIG_PATH := $(LOCAL_MODULE_PATH)/codec
PRODUCT_COPY_FILES += \
    $(CODEC_CONFIG_PATH)/media_codecs.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs.xml \
    $(CODEC_CONFIG_PATH)/media_codecs_performance.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs_performance.xml

MEDIA_CTS_TEST_ENABLE := true
ifneq ($(MEDIA_CTS_TEST_ENABLE), true)
PRODUCT_COPY_FILES += \
    $(CODEC_CONFIG_PATH)/media_codecs.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs.xml \
    $(CODEC_CONFIG_PATH)/media_codecs_allwinner_video.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs_allwinner_video.xml \
    $(CODEC_CONFIG_PATH)/media_codecs_google_audio.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs_google_audio.xml \
    $(CODEC_CONFIG_PATH)/media_codecs_google_video.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs_google_video.xml \
    $(CODEC_CONFIG_PATH)/mediacodec-arm.policy:$(TARGET_COPY_OUT_VENDOR)/etc/seccomp_policy/mediacodec.policy \
    $(CODEC_CONFIG_PATH)/media_codecs_c2.xml:$(TARGET_COPY_OUT_SYSTEM)/etc/media_codecs.xml \

PRODUCT_PACKAGES += \
    android.hardware.media.aw.c2@1.0-service
endif

BOOTANIMATION_CONFIG_PATH := $(LOCAL_MODULE_PATH)/bootanimation
PRODUCT_COPY_FILES += \
    $(BOOTANIMATION_CONFIG_PATH)/bootanimation.zip:system/media/bootanimation.zip

