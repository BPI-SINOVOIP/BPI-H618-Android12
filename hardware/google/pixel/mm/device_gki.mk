PRODUCT_COPY_FILES += \
      hardware/google/pixel/mm/pixel-mm-gki.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/pixel-mm-gki.rc

ifneq (,$(filter userdebug eng, $(TARGET_BUILD_VARIANT)))
PRODUCT_PACKAGES += \
    mm_logd
endif

# ZRAM writeback
PRODUCT_PROPERTY_OVERRIDES += \
    ro.zram.mark_idle_delay_mins=60 \
    ro.zram.first_wb_delay_mins=1440 \
    ro.zram.periodic_wb_delay_hours=24

# LMK tuning
PRODUCT_PROPERTY_OVERRIDES += \
    ro.lmk.filecache_min_kb=153600

BOARD_SEPOLICY_DIRS += hardware/google/pixel-sepolicy/mm/gki
