PRODUCT_PUBLIC_SEPOLICY_DIRS += device/google/bonito-sepolicy/public
PRODUCT_PRIVATE_SEPOLICY_DIRS += device/google/bonito-sepolicy/private

# vendors
BOARD_VENDOR_SEPOLICY_DIRS += device/google/bonito-sepolicy/vendor/qcom/common
BOARD_VENDOR_SEPOLICY_DIRS += device/google/bonito-sepolicy/vendor/qcom/sdm710
BOARD_VENDOR_SEPOLICY_DIRS += device/google/bonito-sepolicy/vendor/google
BOARD_VENDOR_SEPOLICY_DIRS += device/google/bonito-sepolicy/vendor/verizon
BOARD_VENDOR_SEPOLICY_DIRS += device/google/bonito-sepolicy/tracking_denials

# Pixel-wide policy
BOARD_VENDOR_SEPOLICY_DIRS += hardware/google/pixel-sepolicy/ramdump/common
BOARD_VENDOR_SEPOLICY_DIRS += hardware/google/pixel-sepolicy/powerstats
