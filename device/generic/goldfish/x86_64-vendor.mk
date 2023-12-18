include device/generic/goldfish/x86_64-kernel.mk

PRODUCT_PROPERTY_OVERRIDES += \
       vendor.rild.libpath=/vendor/lib64/libgoldfish-ril.so

# This is a build configuration for a full-featured build of the
# Open-Source part of the tree. It's geared toward a US-centric
# build quite specifically for the emulator, and might not be
# entirely appropriate to inherit from for on-device configurations.
PRODUCT_COPY_FILES += \
    device/generic/goldfish/data/etc/config.ini.xl:config.ini \
    device/generic/goldfish/data/etc/advancedFeatures.ini:advancedFeatures.ini \
    device/generic/goldfish/data/etc/encryptionkey.img:encryptionkey.img \
    device/generic/goldfish/task_profiles.json:$(TARGET_COPY_OUT_VENDOR)/etc/task_profiles.json \
    $(EMULATOR_KERNEL_FILE):kernel-ranchu

PRODUCT_SDK_ADDON_COPY_FILES += \
    device/generic/goldfish/data/etc/advancedFeatures.ini:images/x86_64/advancedFeatures.ini \
    device/generic/goldfish/data/etc/encryptionkey.img:images/x86_64/encryptionkey.img \
    $(EMULATOR_KERNEL_FILE):images/x86_64/kernel-ranchu

PRODUCT_COPY_FILES += \
    device/generic/goldfish/data/etc/configs/gpu.config:data/misc/gceconfigs/gpu.config

# a hack to reduce presubmit time where CTS tries to install media files one by one
# taking more than 30 minutes; following is just the list of files needed by current
# tests on presubmit
PRODUCT_COPY_FILES += \
    device/generic/goldfish/data/media/test/swirl_136x144_mpeg4.mp4:data/media/0/test/CtsMediaTestCases-1.4/swirl_136x144_mpeg4.mp4 \
    device/generic/goldfish/data/media/test/swirl_132x130_mpeg4.mp4:data/media/0/test/CtsMediaTestCases-1.4/swirl_132x130_mpeg4.mp4 \
    device/generic/goldfish/data/media/test/swirl_130x132_mpeg4.mp4:data/media/0/test/CtsMediaTestCases-1.4/swirl_130x132_mpeg4.mp4 \
    device/generic/goldfish/data/media/test/swirl_144x136_mpeg4.mp4:data/media/0/test/CtsMediaTestCases-1.4/swirl_144x136_mpeg4.mp4 \
    device/generic/goldfish/data/media/test/swirl_128x128_mpeg4.mp4:data/media/0/test/CtsMediaTestCases-1.4/swirl_128x128_mpeg4.mp4


PRODUCT_PACKAGES += \
    emulatorip

PRODUCT_PROPERTY_OVERRIDES += \
    ro.crypto.dm_default_key.options_format.version=2

PRODUCT_SHIPPING_API_LEVEL := 31
PRODUCT_OTA_ENFORCE_VINTF_KERNEL_REQUIREMENTS := false
TARGET_USES_MKE2FS := true


ifeq ($(QEMU_DISABLE_AVB),true)
  ifeq ($(QEMU_USE_SYSTEM_EXT_PARTITIONS),true)
    PRODUCT_COPY_FILES += \
      device/generic/goldfish/data/etc/dummy.vbmeta.img:$(PRODUCT_OUT)/vbmeta.img \
      device/generic/goldfish/fstab.ranchu.initrd.noavb.ex:$(TARGET_COPY_OUT_VENDOR_RAMDISK)/first_stage_ramdisk/fstab.ranchu \
      device/generic/goldfish/fstab.ranchu.initrd.noavb.ex:$(TARGET_COPY_OUT_VENDOR_RAMDISK)/fstab.ranchu \
      device/generic/goldfish/fstab.ranchu.noavb.ex:$(TARGET_COPY_OUT_VENDOR)/etc/fstab.ranchu
  else
    PRODUCT_COPY_FILES += \
      device/generic/goldfish/data/etc/dummy.vbmeta.img:$(PRODUCT_OUT)/vbmeta.img \
      device/generic/goldfish/fstab.ranchu.initrd.noavb:$(TARGET_COPY_OUT_VENDOR_RAMDISK)/first_stage_ramdisk/fstab.ranchu \
      device/generic/goldfish/fstab.ranchu.initrd.noavb:$(TARGET_COPY_OUT_VENDOR_RAMDISK)/fstab.ranchu \
      device/generic/goldfish/fstab.ranchu.noavb:$(TARGET_COPY_OUT_VENDOR)/etc/fstab.ranchu
  endif
endif

ifeq ($(QEMU_USE_SYSTEM_EXT_PARTITIONS),true)
PRODUCT_COPY_FILES += \
    device/generic/goldfish/fstab.ranchu.initrd.ex:$(TARGET_COPY_OUT_VENDOR_RAMDISK)/first_stage_ramdisk/fstab.ranchu \
    device/generic/goldfish/fstab.ranchu.initrd.ex:$(TARGET_COPY_OUT_VENDOR_RAMDISK)/fstab.ranchu \
    device/generic/goldfish/fstab.ranchu.ex:$(TARGET_COPY_OUT_VENDOR)/etc/fstab.ranchu
else
PRODUCT_COPY_FILES += \
    device/generic/goldfish/fstab.ranchu.initrd:$(TARGET_COPY_OUT_VENDOR_RAMDISK)/first_stage_ramdisk/fstab.ranchu \
    device/generic/goldfish/fstab.ranchu.initrd:$(TARGET_COPY_OUT_VENDOR_RAMDISK)/fstab.ranchu \
    device/generic/goldfish/fstab.ranchu:$(TARGET_COPY_OUT_VENDOR)/etc/fstab.ranchu
endif
