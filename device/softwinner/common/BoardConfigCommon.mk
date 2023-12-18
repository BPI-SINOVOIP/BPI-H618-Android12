-include device/softwinner/common/config/vendorcommand.mk
# image related
TARGET_NO_BOOTLOADER := true
TARGET_NO_RECOVERY := false
TARGET_NO_KERNEL := false

# recovery related
TARGET_RECOVERY_UPDATER_LIBS := librecovery_updater_common
TARGET_RECOVERY_UI_LIB := librecovery_ui_common

TARGET_RELEASETOOLS_EXTENSIONS := device/softwinner/common

BUILD_BROKEN_VENDOR_PROPERTY_NAMESPACE := true
# sepolicy
BOARD_SEPOLICY_DIRS += device/softwinner/common/sepolicy/vendor
SYSTEM_EXT_PUBLIC_SEPOLICY_DIRS := device/softwinner/common/sepolicy/public
SYSTEM_EXT_PRIVATE_SEPOLICY_DIRS := device/softwinner/common/sepolicy/private
