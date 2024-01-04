# BoardConfig.mk
#
ifeq ($(BOARD_BUILD_BOX),true)
TARGET_PLATFORM := homlet
else
TARGET_PLATFORM := tablet
endif
TARGET_BOARD_PLATFORM := apollo
$(call soong_config_add,vendor,board,$(TARGET_BOARD_PLATFORM))
$(call soong_config_add,vendor,platform,$(TARGET_PLATFORM))
$(call soong_config_add,widevine,cryptolevel,$(BOARD_WIDEVINE_OEMCRYPTO_LEVEL))


# Enable dex-preoptimization to speed up first boot sequence
WITH_DEXPREOPT := true
DONT_DEXPREOPT_PREBUILTS := false
TARGET_USE_NEON_OPTIMIZATION := true

TARGET_CPU_SMP := true

#Reserve0
BOARD_ROOT_EXTRA_FOLDERS += Reserve0

TARGET_BOARD_KERN_VER := 5.4
TARGET_BOARD_CHIP := sun50iw9p1
TARGET_BOOTLOADER_BOARD_NAME := exdroid
TARGET_BOOTLOADER_NAME := exdroid
TARGET_OTA_RESTORE_BOOT_STORAGE_DATA := true

BOARD_KERNEL_BASE    := 0x40000000
BOARD_KERNEL_ARCH    := $(call get_kernel_arch,arm64)
BOARD_KERNEL_OFFSET  := $(call get_boot_offset,$(BOARD_KERNEL_ARCH),KERNEL)
BOARD_DTB_OFFSET     := $(call get_boot_offset,$(BOARD_KERNEL_ARCH),DTB)
BOARD_RAMDISK_OFFSET := $(call get_boot_offset,$(BOARD_KERNEL_ARCH),RAMDISK)
BOARD_MKBOOTIMG_ARGS := --board $(BOARD_KERNEL_ARCH) --kernel_offset $(BOARD_KERNEL_OFFSET) --dtb_offset $(BOARD_DTB_OFFSET) --ramdisk_offset $(BOARD_RAMDISK_OFFSET)
BOARD_CHARGER_ENABLE_SUSPEND := true
BOARD_INCLUDE_RECOVERY_DTBO := true

BOARD_AVB_ENABLE := true
BOARD_AVB_ALGORITHM := SHA256_RSA2048
BOARD_AVB_KEY_PATH := vendor/security/toc_keys/SCPFirmwareContentCertPK.pem

# Using sha256 for dm-verity partitions.
# product, vendor_dlkm
BOARD_AVB_PRODUCT_ADD_HASHTREE_FOOTER_ARGS += --hash_algorithm sha256
BOARD_AVB_VENDOR_DLKM_ADD_HASHTREE_FOOTER_ARGS += --hash_algorithm sha256

BOARD_AVB_VBMETA_SYSTEM := system
BOARD_AVB_VBMETA_SYSTEM_KEY_PATH := vendor/security/toc_keys/SCPFirmwareContentCertPK.pem
BOARD_AVB_VBMETA_SYSTEM_ALGORITHM := SHA256_RSA2048
BOARD_AVB_VBMETA_SYSTEM_ROLLBACK_INDEX := $(PLATFORM_SECURITY_PATCH_TIMESTAMP)
BOARD_AVB_VBMETA_SYSTEM_ROLLBACK_INDEX_LOCATION := 1
BOARD_AVB_SYSTEM_ADD_HASHTREE_FOOTER_ARGS += --hash_algorithm sha256

BOARD_AVB_VBMETA_VENDOR := vendor
BOARD_AVB_VBMETA_VENDOR_KEY_PATH := vendor/security/toc_keys/SCPFirmwareContentCertPK.pem
BOARD_AVB_VBMETA_VENDOR_ALGORITHM := SHA256_RSA2048
BOARD_AVB_VBMETA_VENDOR_ROLLBACK_INDEX := $(PLATFORM_SECURITY_PATCH_TIMESTAMP)
BOARD_AVB_VBMETA_VENDOR_ROLLBACK_INDEX_LOCATION := 2
BOARD_AVB_VENDOR_ADD_HASHTREE_FOOTER_ARGS += --hash_algorithm sha256

BOARD_AVB_RECOVERY_KEY_PATH := vendor/security/toc_keys/SCPFirmwareContentCertPK.pem
BOARD_AVB_RECOVERY_ALGORITHM := SHA256_RSA2048
BOARD_AVB_RECOVERY_ROLLBACK_INDEX := $(PLATFORM_SECURITY_PATCH_TIMESTAMP)
BOARD_AVB_RECOVERY_ROLLBACK_INDEX_LOCATION := 3

BOARD_ADD_PACK_CONFIG += $(TARGET_DEVICE_DIR)/system/sys_partition.fex
BOARD_ADD_PACK_CONFIG += $(PRODUCT_PLATFORM_PATH)/common/system/env.cfg
BOARD_ADD_PACK_CONFIG += $(TARGET_DEVICE_DIR)/system/dragon_toc.cfg
# LZ4 ramdisk
BOARD_RAMDISK_USE_LZ4 := true

BOARD_BOOT_HEADER_VERSION := 3
BOARD_USES_RECOVERY_AS_BOOT := true
TARGET_NO_RECOVERY := true
TARGET_RECOVERY_UPDATER_LIBS :=

BOARD_BOOTIMAGE_PARTITION_SIZE := $(call get_partition_size,boot,$(PARTITION_CFG_FILE))
BOARD_FLASH_BLOCK_SIZE := 4096

BOARD_DTBO_SUPPORT ?= true
ifeq ($(BOARD_DTBO_SUPPORT),true)
# prebuilt dtbo
BOARD_PREBUILT_DTBOIMAGE := $(PRODUCT_PREBUILT_PATH)/dtbo.img
BOARD_DTBOIMG_PARTITION_SIZE := $(call get_partition_size,dtbo,$(PARTITION_CFG_FILE))
# include dtb in boot image
BOARD_INCLUDE_DTB_IN_BOOTIMG := true
BOARD_PREBUILT_DTBIMAGE_DIR := $(PRODUCT_PREBUILT_PATH)
endif

# loop device max partition number set
# # This fixes following CTS test case if adaptable storage is enabled
# # CTS-CtsAppSecurityHostTestCases-android.appsecurity.cts.AdoptableHostTest#testEjected
BOARD_KERNEL_CMDLINE += loop.max_part=4

# boot
BOARD_BOOT_HEADER_VERSION ?= 3
BOARD_MKBOOTIMG_ARGS += --header_version $(BOARD_BOOT_HEADER_VERSION)
ifeq ($(PRODUCT_BUILD_VENDOR_BOOT_IMAGE),true)
    BOARD_VENDOR_BOOTIMAGE_PARTITION_SIZE := $(call get_partition_size,vendor_boot,$(PARTITION_CFG_FILE))
    # BOARD_VENDOR_RAMDISK_KERNEL_MODULES will copied to vendor-ramdisk/lib/modules
    BOARD_VENDOR_RAMDISK_KERNEL_MODULES := \
        $(patsubst %.ko,$(PRODUCT_PREBUILT_PATH)/dist/%.ko,$(shell cat $(TARGET_DEVICE_DIR)/system/vendor_ramdisk.modules | sed 's/^\s\+//g;s/\s\+$$//g;/^#/d;/^$$/d'))
endif

# BOARD_VENDOR_KERNEL_MODULES will copied to vendor/lib/modules
BOARD_VENDOR_KERNEL_MODULES := \
    $(filter-out $(BOARD_VENDOR_RAMDISK_KERNEL_MODULES),$(wildcard $(PRODUCT_PREBUILT_PATH)/dist/*.ko))

BOARD_BUILD_SUPER_IMAGE_BY_DEFAULT := true
BOARD_SUPER_IMAGE_IN_UPDATE_PACKAGE := true
BOARD_SUPER_PARTITION_SIZE := $(call get_partition_size,super,$(PARTITION_CFG_FILE))
BOARD_SUPER_PARTITION_GROUPS := sb
BOARD_SB_SIZE := $(shell expr $(BOARD_SUPER_PARTITION_SIZE) - 8388608)
BOARD_SB_PARTITION_LIST := system vendor product vendor_dlkm
# add dynamic prop
BOARD_KERNEL_CMDLINE += androidboot.dynamic_partitions=true
BOARD_KERNEL_CMDLINE += androidboot.dynamic_partitions_retrofit=true
# enable init full log,default is disable
# BOARD_KERNEL_CMDLINE += printk.devkmsg=on

BOARD_SYSTEMIMAGE_FILE_SYSTEM_TYPE ?= ext4

BOARD_USES_VENDORIMAGE ?= true
BOARD_VENDORIMAGE_FILE_SYSTEM_TYPE ?= ext4
TARGET_COPY_OUT_VENDOR ?= vendor

BOARD_USES_METADATA_PARTITION ?= true

# Build a separate product.img partition
BOARD_USES_PRODUCTIMAGE ?= true
BOARD_PRODUCTIMAGE_FILE_SYSTEM_TYPE ?= ext4
TARGET_COPY_OUT_PRODUCT := product

# Build a separate vendor_dlkm.img partiton
BOARD_USES_VENDOR_DLKMIMAGE ?= true
BOARD_VENDOR_DLKMIMAGE_FILE_SYSTEM_TYPE ?= ext4
TARGET_COPY_OUT_VENDOR_DLKM := vendor_dlkm

TARGET_USERIMAGES_USE_F2FS ?= true

BOARD_VNDK_VERSION := current

#BOARD_SEPOLICY_VERS := 26.0
BOARD_SEPOLICY_DIRS += $(PRODUCT_PLATFORM_PATH)/common/sepolicy/vendor
SYSTEM_EXT_PUBLIC_SEPOLICY_DIRS += $(PRODUCT_PLATFORM_PATH)/common/sepolicy/public
SYSTEM_EXT_PRIVATE_SEPOLICY_DIRS += $(PRODUCT_PLATFORM_PATH)/common/sepolicy/private

#time for health alarm
BOARD_PERIODIC_CHORES_INTERVAL_FAST := 86400
BOARD_PERIODIC_CHORES_INTERVAL_SLOW := 86400

# Product partition TREBLE required configuration start from R
# PRODUCT_PRODUCT_VNDK_VERSION := current
# PRODUCT_ENFORCE_PRODUCT_PARTITON_INTERFACE := true

# Enable SVELTE malloc
MALLOC_SVELTE := true

DEVICE_MANIFEST_FILE += $(PRODUCT_PLATFORM_PATH)/common/system/manifest.xml
DEVICE_MATRIX_FILE := $(PRODUCT_PLATFORM_PATH)/common/system/compatibility_matrix.xml

DEVICE_PRODUCT_COMPATIBILITY_MATRIX_FILE := $(PRODUCT_PLATFORM_PATH)/common/system/compatibility_matrix_product.xml

# When PRODUCT_SHIPPING_API_LEVEL >= 27, TARGET_USES_MKE2FS must be true
TARGET_USES_MKE2FS := true

USE_OPENGL_RENDERER := true
NUM_FRAMEBUFFER_SURFACE_BUFFERS := 2
TARGET_USES_HWC2 := true
TARGET_GPU_TYPE := mali-g31
USE_IOMMU := true

# recovery touch high threshold
TARGET_RECOVERY_UI_TOUCH_HIGH_THRESHOLD := 200

TARGET_USES_64_BIT_BINDER := true
TARGET_SUPPORTS_32_BIT_APPS := true
TARGET_USES_G2D := true
WRITE_BACK_MODE := 0
$(call soong_config_add,disp,writebackMode,$(WRITE_BACK_MODE))

include hardware/aw/gpu/product_config.mk
ifeq ($(BOARD_BUILD_BOX),true)
include vendor/aw/common/HomletBoardConfig.mk
else
include vendor/aw/common/TabletBoardConfig.mk
endif
# include hardware/aw/display/pq/pq_config.mk
