LOCAL_MODULE_PATH := $(shell dirname $(lastword $(MAKEFILE_LIST)))

# copy files
PRODUCT_COPY_FILES += \
    $(LOCAL_MODULE_PATH)/camera.cfg:$(TARGET_COPY_OUT_VENDOR)/etc/camera.cfg \
    $(LOCAL_MODULE_PATH)/hawkview/sensor_list_cfg.ini:vendor/etc/hawkview/sensor_list_cfg.ini \
    $(LOCAL_MODULE_PATH)/init.camera.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/init.camera.rc \
	$(LOCAL_MODULE_PATH)/media_profiles.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_profiles_V1_0.xml \
    frameworks/native/data/etc/android.hardware.camera.flash-autofocus.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.camera.flash-autofocus.xml \
