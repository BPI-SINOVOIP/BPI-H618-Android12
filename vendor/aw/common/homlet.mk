$(call inherit-product-if-exists, vendor/aw/common/hardware/input/multi_ir/multiir.mk)
#app
PRODUCT_PACKAGES += \
    TvSettings \
    privapp-vendor-permissions \
    TvLauncher \
    TvdVideo \
    TvdFileManager \
    WebScreensaver \
    Test_AD \
    ImageParser \
    DragonBox \
    DragonAgingTV \
    MiracastReceiver \
    AwTvProvision \
    SettingsAssist \
    provision-permissions \
    DragonSN  \
    GalleryTV

PRODUCT_PACKAGES += \
    LiveTv \
    TvProvider \
    tv_input.default

#service
PRODUCT_PACKAGES += \
    libsystemmix_jni \
    systemmixservice \
    libsystemmixservice \
    isomountservice \
    libisomountmanager_jni \
    libisomountmanagerservice \
    libsystemmix \
    nfsprobe \
    libjni_swos \
    libadmanager_jni \
    libjni_WFDManager.so \
    libwfdrtsp.so \
    libwfdplayer.so \
    libwfdmanager.so \
    libgifplayer \
    libgifplayer_jni

# utils, add multi_ir to recovery
PRODUCT_PACKAGES += \
   qw \

PRODUCT_PACKAGES += \
   appsdisable

#add debug method for eng img
ifeq ($(TARGET_BUILD_VARIANT),eng)
PRODUCT_PROPERTY_OVERRIDES += \
    vold.debug=true
endif



# for gpio
PRODUCT_PACKAGES += \
    gpioservice \
    libgpio_jni \
    libgpioservice \

# for homlet display hal
PRODUCT_PACKAGES += \
    libhwcprivateservice \
    libedid \
    libdisplayd \
    softwinner.homlet.displayd@1.0 \
    softwinner.homlet.displayd@1.0-service

ifeq (true, &(call math_gt_or_eq, 29, $(PLATFORM_SDK_VERSION)))
PRODUCT_PACKAGES += \
    displayd-test \
    softwinner.display \
    DisplayDaemon \
    DisplayDaemonService
else
PRODUCT_PACKAGES += \
    displayoutput-test \
    hdmitest

PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
    ro.vendor.display.useExtend=true
endif

PRODUCT_PACKAGES += \
    dispdebug \
    dispconfig \
    hdcptool

# for audio
PRODUCT_PACKAGES += \
    softwinner.audio

# secure storage support
PRODUCT_PACKAGES += \
    libsst \
    sst_test_v2

# format Reserve0 for save display config
PRODUCT_PACKAGES += \
   format_device

#logcatd -n 2 -r 200000
PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
    persist.logd.RotateSizeKBytes=200000 \
    persist.logd.logpersistd.size=2
