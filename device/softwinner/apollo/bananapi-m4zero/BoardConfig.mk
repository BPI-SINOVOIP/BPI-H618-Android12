include device/softwinner/common/BoardConfigCommon.mk

include $(PRODUCT_PLATFORM_PATH)/BoardConfig.mk

# must include common BoardConfig.mk in the end
BOARD_KERNEL_CMDLINE += androidboot.selinux=permissive
BOARD_KERNEL_CMDLINE += androidboot.dtbo_idx=0,1,2
BOARD_KERNEL_CMDLINE += firmware_class.path=/vendor/etc/firmware


# wifi and bt configuration
# 1. Wifi Configuration
BOARD_WIFI_VENDOR := common
BOARD_USR_WIFI    :=
WIFI_DRIVER_MODULE_PATH :=
WIFI_DRIVER_MODULE_NAME :=
WIFI_DRIVER_MODULE_ARG  :=

# 2. Bluetooth Configuration
BOARD_BLUETOOTH_VENDOR    := common
BOARD_HAVE_BLUETOOTH_NAME :=
#BOARD_BLUETOOTH_CONFIG_DIR := $(TARGET_DEVICE_DIR)/wireless/bluetooth
BOARD_BLUETOOTH_CONFIG_DIR :=  $(PRODUCT_PLATFORM_PATH)/common/wireless/bluetooth
BOARD_BLUETOOTH_TTY := /dev/ttyAS1
# Must include after wifi/bt configuration
include device/softwinner/common/config/wireless/wireless_config.mk
