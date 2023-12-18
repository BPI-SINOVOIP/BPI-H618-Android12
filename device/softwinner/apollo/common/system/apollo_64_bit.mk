# Primary Arch
TARGET_ARCH := arm64
TARGET_ARCH_VARIANT := armv8-a
TARGET_CPU_VARIANT := generic
TARGET_CPU_ABI := arm64-v8a
TARGET_CPU_ABI2 :=

# Secondary Arch
TARGET_2ND_ARCH := arm
TARGET_2ND_ARCH_VARIANT := armv8-a
TARGET_2ND_CPU_ABI := armeabi-v7a
TARGET_2ND_CPU_ABI2 := armeabi
TARGET_2ND_CPU_VARIANT := generic

# 64-bit app support
TARGET_SUPPORTS_64_BIT_APPS := true

PRODUCT_PROPERTY_OVERRIDES += \
    ro.zygote=zygote64_32

# Copy different zygote settings for vendor.img to select by setting property
# ro.zygote=zygote64_32 or ro.zygote=zygote32_64:
#   1. 64-bit primary, 32-bit secondary OR
#   2. 32-bit primary, 64-bit secondary
#   3. 64-bit only is currently forbidden (b/64280459#comment6)
PRODUCT_COPY_FILES += \
    system/core/rootdir/init.zygote64_32.rc:/system/etc/init/hw/init.zygote64_32.rc
