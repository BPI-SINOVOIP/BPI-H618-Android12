LOCAL_MODULE_PATH := $(shell dirname $(lastword $(MAKEFILE_LIST)))

ifeq ($(BOARD_BUILD_BOX),true)
$(call inherit-product-if-exists, vendor/aw/common/homlet.mk)
else
$(call inherit-product-if-exists, vendor/aw/common/tablet.mk)
endif

# properties
# system properties
PRODUCT_PROPERTY_OVERRIDES += \
    ro.config.per_app_memcg=false \

#set gralloc debug
#fatal:0x1 error:0x2 warning:0x4 message:0x8 verbose:0x10 calltrace:0x20
PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
    vendor.product.pvr.debug_level=7

#zram write back
PRODUCT_PROPERTY_OVERRIDES += \
    persist.sys.zram_enabled = 1 \
    ro.zram.first_wb_delay_mins = 180 \
    ro.zram.mark_idle_delay_mins = 60 \
    ro.zram.periodic_wb_delay_hours = 24 \

# info
PRODUCT_CPU_TYPE := $(shell echo $(TARGET_BOARD_IC) | tr a-z A-Z)
PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
    ro.sys.cputype=QuadCore-$(PRODUCT_CPU_TYPE) \
    ro.product.firmware=$(PRODUCT_CPU_TYPE)-android12-v1.0rc4

# storage
PRODUCT_SYSTEM_DEFAULT_PROPERTIES += persist.fw.force_adoptable=true

# bootevent
PRODUCT_SYSTEM_DEFAULT_PROPERTIES += persist.sys.bootevent=true

# Tracing disabled by default,set 1 to enable atrace
PRODUCT_SYSTEM_DEFAULT_PROPERTIES += persist.debug.traced.enable=0

# display
PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
    persist.display.smart_backlight=1 \
    persist.display.enhance_mode=0 \

ifneq (,$(filter userdebug eng,$(TARGET_BUILD_VARIANT)))
PRODUCT_DEBUG := true
PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
    persist.sys.usb.config=adb \
    ro.adb.secure=0 \
    ro.sys.dis_app_animation=true \

else
PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
    ro.adb.secure=1 \

endif

# product properties
# sf debug
PRODUCT_PROPERTY_OVERRIDES += debug.sf.latch_unsignaled=1

# drm
PRODUCT_PROPERTY_OVERRIDES += drm.service.enabled=true

# dynamic partitions
PRODUCT_PROPERTY_OVERRIDES += ro.boot.dynamic_partitions_retrofit=true

PRODUCT_PROPERTY_OVERRIDES += \
    ro.rebootescrow.device=/dev/block/pmem0

PRODUCT_PACKAGES += \
    android.hardware.rebootescrow-service.default

# system
PRODUCT_PROPERTY_OVERRIDES += \
    ro.frp.pst=/dev/block/by-name/frp \
    ro.control_privapp_permissions=enforce \
    init.userspace_reboot.is_supported=1 \

PRODUCT_PROPERTY_OVERRIDES += \
    ro.soc.manufacturer=Allwinner \
    ro.soc.model=H618

# theme overlay
PRODUCT_PACKAGES += GoogleSearchGoOverlay
PRODUCT_PACKAGES += AwAccentColorOverlay
PRODUCT_PACKAGES += AwIconShapeOverlay
#PRODUCT_PACKAGES += AwGestureOverlay

PRODUCT_PACKAGES += \
    TimerSwitch

# PRODUCT_PACKAGES
PRODUCT_PACKAGES += \
    WallpaperPicker2 \

GLOBAL_REMOVED_PACKAGES += \
    TeleService \
    Telecom \
    TelephonyProvider

# audio utils
PRODUCT_PACKAGES += \
    tinymix \
    tinyplay \
    tinycap \
    tinypcminfo

ifneq ($(PRODUCT_HAS_UVC_CAMERA), true)
PRODUCT_PACKAGES += \
    AWCamera
endif

ifneq (,$(filter userdebug eng,$(TARGET_BUILD_VARIANT)))
# memory debug
PRODUCT_PACKAGES += dmabuf_dump
endif
PRODUCT_PACKAGES += PRODUCT_PACKAGES += vndservicemanager

PRODUCT_USE_DYNAMIC_PARTITIONS := true

PRODUCT_PACKAGES += \
    fastbootd \
    android.hardware.fastboot@1.0-impl

# call other makefile
# 32bit android,you should define TARGET_ARCH := arm
# 64bit android,you should define TARGET_ARCH := arm64
TARGET_ARCH ?= arm
ifeq ($(TARGET_ARCH),arm)
$(call inherit-product, $(LOCAL_MODULE_PATH)/apollo_32_bit.mk)
else ifeq ($(TARGET_ARCH),arm64)
$(call inherit-product, $(LOCAL_MODULE_PATH)/apollo_64_bit.mk)
endif

$(call inherit-product, device/softwinner/common/pad.mk)
$(call inherit-product-if-exists, vendor/aw/public/tool.mk)
$(call inherit-product-if-exists, device/softwinner/common/common.mk)

# Enabling type-precise GC results in larger optimized DEX files.  The
# additional storage requirements for ".odex" files can cause /system
# to overflow on some devices, so this is configured separately for
# each product.
PRODUCT_TAGS += dalvik.gc.type-precise

# set product shipping(first) api level
ifneq ($(CONFIG_OTA_FROM_11), true)
    PRODUCT_SHIPPING_API_LEVEL := 31
else
    PRODUCT_SHIPPING_API_LEVEL := 30
endif
PRODUCT_PACKAGES_SHIPPING_API_LEVEL_29 := disable_configstore

# enable property split
PRODUCT_COMPATIBLE_PROPERTY_OVERRIDE := true

# treble
BOARD_PROPERTY_OVERRIDES_SPLIT_ENABLED := true
PRODUCT_FULL_TREBLE_OVERRIDE := true

#this mean [ro.build.characteristics] values
ifeq ($(BOARD_BUILD_BOX),true)
    PRODUCT_CHARACTERISTICS := homlet
else
    PRODUCT_CHARACTERISTICS := tablet
endif

CONFIG_USE_ANDROID_MAINLINE := true

# Disable APEX_LIBS_ABSENCE_CHECK
# We got offending entrie if enable check: libicui18n and libicuuc.
# Both are dependencies of libcedarx. We must fix the dependencies and then enable check.
DISABLE_APEX_LIBS_ABSENCE_CHECK := true

# PRODUCT_COPY_FILES
# lmkd whitelist
PRODUCT_COPY_FILES += $(LOCAL_MODULE_PATH)/lmkd_whitelist:$(TARGET_COPY_OUT_SYSTEM)/etc/lmkd_whitelist
# thermal info config
PRODUCT_COPY_FILES += $(LOCAL_MODULE_PATH)/thermal_info_config.json:$(TARGET_COPY_OUT_VENDOR)/etc/thermal_info_config.json

$(call inherit-product-if-exists, vendor/aw/public/prebuild/lib/librild/radio_common.mk)

PRODUCT_COPY_FILES += \
    $(LOCAL_MODULE_PATH)/init.recovery.sun50iw9p1.rc:root/init.recovery.sun50iw9p1.rc \
    $(LOCAL_MODULE_PATH)/init.recovery.sun50iw9p1.rc:$(TARGET_COPY_OUT_VENDOR_RAMDISK)/init.recovery.sun50iw9p1.rc \
    $(LOCAL_MODULE_PATH)/ueventd.sun50iw9p1.rc:$(TARGET_COPY_OUT_VENDOR)/ueventd.rc \
    $(LOCAL_MODULE_PATH)/init.sun50iw9p1.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.sun50iw9p1.rc \
    $(LOCAL_MODULE_PATH)/init.sun50iw9p1.usb.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.sun50iw9p1.usb.rc \
    $(LOCAL_MODULE_PATH)/init.secondmodules.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/init.secondmodules.rc \
    $(LOCAL_MODULE_PATH)/init.scheduler.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/init.scheduler.rc \

PRODUCT_COPY_FILES += \
    device/softwinner/common/config/awbms_config:$(TARGET_COPY_OUT_VENDOR)/etc/awbms_config \
    frameworks/native/data/etc/android.software.controls.xml:$(TARGET_COPY_OUT_SYSTEM)/etc/permissions/android.software.controls.xml \
    $(LOCAL_MODULE_PATH)/preferred-apps/custom.xml:system/etc/preferred-apps/custom.xml \
    device/softwinner/common/config/android.hardware.location.network.xml:$(TARGET_COPY_OUT_SYSTEM)/etc/permissions/android.hardware.location.network.xml \
    frameworks/native/data/etc/android.hardware.ethernet.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.ethernet.xml \

ifeq ($(BOARD_BUILD_BOX),true)
PRODUCT_COPY_FILES += \
    device/softwinner/common/config/tv_core_hardware.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/tv_core_hardware.xml
else
PRODUCT_COPY_FILES += \
    device/softwinner/common/config/tablet_core_hardware.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/tablet_core_hardware.xml
endif

# usb and backup permissions file
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.software.midi.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.midi.xml \
    frameworks/native/data/etc/android.hardware.usb.host.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.usb.host.xml \
    frameworks/native/data/etc/android.hardware.usb.accessory.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.usb.accessory.xml \
    frameworks/native/data/etc/android.software.backup.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.backup.xml \
    frameworks/native/data/etc/android.hardware.reboot_escrow.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.reboot_escrow.xml
# low_ram device config
# all devices got ram size equal to or less than 1GB should be defined as low ram device.
# also we can get rid of the software limit, and fully use 2GB ram and config it as regular device.
CONFIG_LOW_RAM_DEVICE ?= true
ifeq ($(CONFIG_LOW_RAM_DEVICE),true)
    # Reduces GC frequency of foreground apps by 50% (not recommanded for 512M devices)
    PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
        dalvik.vm.foreground-heap-growth-multiplier=2.0 \
        dalvik.vm.madvise-random=true \

    #lmkd
    PRODUCT_PROPERTY_OVERRIDES += \
        ro.lmk.psi_complete_stall_ms = 500

    # apex
    PRODUCT_PROPERTY_OVERRIDES += ro.apex.updatable=false

    # limit dex2oat threads to improve thermals
    PRODUCT_PROPERTY_OVERRIDES += \
        dalvik.vm.boot-dex2oat-threads=3 \
        dalvik.vm.dex2oat-threads=2 \
        dalvik.vm.image-dex2oat-threads=3 \
        dalvik.vm.dex2oat-flags=--no-watch-dog \
        dalvik.vm.jit.codecachesize=0 \
        pm.dexopt.boot=verify \
        dalvik.vm.heapstartsize=5m \
        dalvik.vm.heapsize=256m \
        dalvik.vm.heaptargetutilization=0.75 \
        dalvik.vm.heapminfree=512k \
        dalvik.vm.heapmaxfree=8m \

    $(call inherit-product, device/softwinner/common/go_common.mk)
    $(call inherit-product, $(LOCAL_MODULE_PATH)/go_base.mk)
    # Mainline partner build config - low RAM
    OVERRIDE_TARGET_FLATTEN_APEX := true

    # Strip the local variable table and the local variable type table to reduce
    # the size of the system image. This has no bearing on stack traces, but will
    # leave less information available via JDWP.
    PRODUCT_MINIMIZE_JAVA_DEBUG_INFO := true

    # Do not generate libartd.
    PRODUCT_ART_TARGET_INCLUDE_DEBUG_BUILD := false

    # Enable DM file preopting to reduce first boot time
    PRODUCT_DEX_PREOPT_GENERATE_DM_FILES :=true
    PRODUCT_DEX_PREOPT_DEFAULT_COMPILER_FILTER := verify

    # launcher
    PRODUCT_PACKAGES += Launcher3QuickStepGo
else
    PRODUCT_PROPERTY_OVERRIDES += \
        dalvik.vm.heapstartsize=5m \
        dalvik.vm.heapgrowthlimit=256m \
        dalvik.vm.heapsize=512m \
        dalvik.vm.heaptargetutilization=0.75 \
        dalvik.vm.heapminfree=512k \
        dalvik.vm.heapmaxfree=8m

    $(call inherit-product, build/target/product/full_base.mk)

    # Mainline partner build config - updatable APEX
    MAINLINE_INCLUDE_WIFI_MODULE := false

    # launcher
    PRODUCT_PACKAGES += Launcher3QuickStep

endif
