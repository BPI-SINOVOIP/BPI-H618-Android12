PRODUCT_COPY_FILES += \
      hardware/google/pixel/common/init.pixel.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/init.pixel.rc

BOARD_SEPOLICY_DIRS += hardware/google/pixel-sepolicy/common

# Write flags to the vendor space in /misc partition.
PRODUCT_PACKAGES += \
    misc_writer

# Enable atrace hal and tools for pixel devices
PRODUCT_PACKAGES += \
    android.hardware.atrace@1.0-service.pixel \
    dmabuf_dump


# fastbootd
PRODUCT_PACKAGES += \
    fastbootd

# Common ramdump file type.
BOARD_VENDOR_SEPOLICY_DIRS += hardware/google/pixel-sepolicy/ramdump/common
