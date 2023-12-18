# PRODUCT_PACKAGES
PRODUCT_PACKAGES += android-ex-camera2

# hal
PRODUCT_PACKAGES += \
    camera.device@3.5-impl \
    android.hardware.camera.provider@2.4-impl \
    libcamera \
    camera.apollo


# properties
PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
    sys.camera.facedetection.enable = true


# camera hal: P Regular version must use camera hal v1
USE_CAMERA_HAL_3_4 := false
USE_CAMERA_HAL_1_0 := true

ifndef PRODUCT_HAS_UVC_CAMERA
    PRODUCT_HAS_UVC_CAMERA := false
endif
# copy files
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.camera.front.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.camera.front.xml \
    frameworks/native/data/etc/android.hardware.camera.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/id.hardware.camera.xml \

ifeq ($(PRODUCT_HAS_UVC_CAMERA),true)

PRODUCT_PACKAGES += \
    android.hardware.camera.provider@2.4-external-service

PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
    ro.camera.uvcfacing=front \
    camera2.portability.force_api=1

PRODUCT_PROPERTY_OVERRIDES += \
    vendor.camera.uvc.fourcc=0

PRODUCT_COPY_FILES += \
    device/softwinner/apollo/common/camera/external_camera_config.xml:$(TARGET_COPY_OUT_VENDOR)/etc/external_camera_config.xml \
    frameworks/native/data/etc/android.hardware.camera.external.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.camera.external.xml

DEVICE_MANIFEST_FILE += $(PRODUCT_PLATFORM_PATH)/common/camera/external_camera_manifest.xml
else
PRODUCT_PROPERTY_OVERRIDES += \
    ro.camera.enableLazyHal=true
PRODUCT_PACKAGES += \
    android.hardware.camera.provider@2.4-service-lazy
DEVICE_MANIFEST_FILE += $(PRODUCT_PLATFORM_PATH)/common/camera/camera_manifest.xml
endif
