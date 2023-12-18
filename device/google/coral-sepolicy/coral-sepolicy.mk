PRODUCT_PUBLIC_SEPOLICY_DIRS += device/google/coral-sepolicy/public
PRODUCT_PRIVATE_SEPOLICY_DIRS += device/google/coral-sepolicy/private

# vendors
BOARD_SEPOLICY_DIRS += device/google/coral-sepolicy/vendor/google
BOARD_SEPOLICY_DIRS += device/google/coral-sepolicy/vendor/qcom/common
BOARD_SEPOLICY_DIRS += device/google/coral-sepolicy/vendor/qcom/sm8150
BOARD_SEPOLICY_DIRS += device/google/coral-sepolicy/vendor/knowles/common
BOARD_SEPOLICY_DIRS += device/google/coral-sepolicy/tracking_denials
BOARD_SEPOLICY_DIRS += device/google/coral-sepolicy/vendor/st
BOARD_SEPOLICY_DIRS += device/google/coral-sepolicy/vendor/verizon

# Pixel-wide
BOARD_SEPOLICY_DIRS += hardware/google/pixel-sepolicy/powerstats

# system_ext
SYSTEM_EXT_PUBLIC_SEPOLICY_DIRS += device/google/coral-sepolicy/system_ext/public
SYSTEM_EXT_PRIVATE_SEPOLICY_DIRS += device/google/coral-sepolicy/system_ext/private
