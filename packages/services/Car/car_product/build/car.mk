#
# Copyright (C) 2016 The Android Open-Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Common make file for all car builds

PRODUCT_PUBLIC_SEPOLICY_DIRS += packages/services/Car/car_product/sepolicy/public
PRODUCT_PRIVATE_SEPOLICY_DIRS += packages/services/Car/car_product/sepolicy/private

PRODUCT_PACKAGES += \
    Bluetooth \
    CarActivityResolver \
    CarDeveloperOptions \
    CarSettingsIntelligence \
    OneTimeInitializer \
    CarProvision \
    StatementService \
    SystemUpdater


PRODUCT_PACKAGES += \
    clatd \
    pppd \
    screenrecord

# This is for testing
ifneq (,$(filter userdebug eng, $(TARGET_BUILD_VARIANT)))
PRODUCT_PACKAGES += \
    DefaultStorageMonitoringCompanionApp \
    EmbeddedKitchenSinkApp \
    GarageModeTestApp \
    ExperimentalCarService \
    BugReportApp \
    NetworkPreferenceApp \
    SampleCustomInputService \

# SEPolicy for test apps / services
BOARD_SEPOLICY_DIRS += packages/services/Car/car_product/sepolicy/test
endif

# ClusterOsDouble is the testing app to test Cluster2 framework and it can handle Cluster VHAL
# and do some Cluster OS role.
ifeq ($(ENABLE_CLUSTER_OS_DOUBLE), true)
PRODUCT_PACKAGES += ClusterHomeSample ClusterOsDouble
else
# DirectRenderingCluster is the sample app for the old Cluster framework.
PRODUCT_PACKAGES += DirectRenderingCluster
endif  # ENABLE_CLUSTER_OS_DOUBLE

PRODUCT_COPY_FILES += \
    frameworks/av/media/libeffects/data/audio_effects.conf:system/etc/audio_effects.conf

PRODUCT_PROPERTY_OVERRIDES += \
    persist.bluetooth.enablenewavrcp=false \
    ro.carrier=unknown

PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
    config.disable_systemtextclassifier=true

###
### Suggested values for multi-user properties - can be overridden
###

# Enable headless system user mode
PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
    ro.fw.mu.headless_system_user?=true

# Enable user pre-creation
PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
    android.car.number_pre_created_users?=1 \
    android.car.number_pre_created_guests?=1

### end of multi-user properties ###

# Overlay for Google network and fused location providers
$(call inherit-product, device/sample/products/location_overlay.mk)
$(call inherit-product-if-exists, frameworks/webview/chromium/chromium.mk)
$(call inherit-product, packages/services/Car/car_product/build/car_base.mk)

# Overrides
PRODUCT_BRAND := generic
PRODUCT_DEVICE := generic
PRODUCT_NAME := generic_car_no_telephony

PRODUCT_IS_AUTOMOTIVE := true

PRODUCT_PROPERTY_OVERRIDES := \
    ro.config.ringtone=Girtab.ogg \
    ro.config.notification_sound=Tethys.ogg \
    ro.config.alarm_alert=Oxygen.ogg \
    $(PRODUCT_PROPERTY_OVERRIDES) \

PRODUCT_PROPERTY_OVERRIDES += \
    keyguard.no_require_sim=true

# Automotive specific packages
PRODUCT_PACKAGES += \
    CarFrameworkPackageStubs \
    CarService \
    CarShell \
    CarDialerApp \
    CarRadioApp \
    OverviewApp \
    CarLauncher \
    CarSystemUI \
    LocalMediaPlayer \
    CarMediaApp \
    CarMessengerApp \
    CarHTMLViewer \
    CarHvacApp \
    CarMapsPlaceholder \
    CarLatinIME \
    CarSettings \
    CarUsbHandler \
    android.car \
    car-frameworks-service \
    com.android.car.procfsinspector \
    libcar-framework-service-jni \

# RROs
PRODUCT_PACKAGES += \
    CarPermissionControllerRRO \

# System Server components
# Order is important: if X depends on Y, then Y should precede X on the list.
PRODUCT_SYSTEM_SERVER_JARS += car-frameworks-service
# TODO: make the order optimal by appending 'car-frameworks-service' at the end
# after its dependency 'services'. Currently the order is violated because this
# makefile is included before AOSP makefile.
PRODUCT_BROKEN_SUBOPTIMAL_ORDER_OF_SYSTEM_SERVER_JARS := true

# Boot animation
PRODUCT_COPY_FILES += \
    packages/services/Car/car_product/bootanimations/bootanimation-832.zip:system/media/bootanimation.zip

PRODUCT_LOCALES := \
    en_US \
    af_ZA \
    am_ET \
    ar_EG ar_XB \
    as_IN \
    az_AZ \
    be_BY \
    bg_BG \
    bn_BD \
    bs_BA \
    ca_ES \
    cs_CZ \
    da_DK \
    de_DE \
    el_GR \
    en_AU en_CA en_GB en_IN en_XA \
    es_ES es_US \
    et_EE \
    eu_ES \
    fa_IR \
    fi_FI \
    fil_PH \
    fr_CA fr_FR \
    gl_ES \
    gu_IN \
    hi_IN \
    hr_HR \
    hu_HU \
    hy_AM \
    id_ID \
    is_IS \
    it_IT \
    iw_IL \
    ja_JP \
    ka_GE \
    kk_KZ \
    km_KH km_MH \
    kn_IN \
    ko_KR \
    ky_KG \
    lo_LA \
    lv_LV \
    lt_LT \
    mk_MK \
    ml_IN \
    mn_MN \
    mr_IN \
    ms_MY \
    my_MM \
    ne_NP \
    nl_NL \
    no_NO \
    or_IN \
    pa_IN \
    pl_PL \
    pt_BR pt_PT \
    ro_RO \
    ru_RU \
    si_LK \
    sk_SK \
    sl_SI \
    sq_AL \
    sr_RS \
    sv_SE \
    sw_TZ \
    ta_IN \
    te_IN \
    th_TH \
    tr_TR \
    uk_UA \
    ur_PK \
    uz_UZ \
    vi_VN \
    zh_CN zh_HK zh_TW \
    zu_ZA

PRODUCT_BOOT_JARS += \
    android.car

PRODUCT_HIDDENAPI_STUBS := \
    android.car-stubs-dex

PRODUCT_HIDDENAPI_STUBS_SYSTEM := \
    android.car-system-stubs-dex

PRODUCT_HIDDENAPI_STUBS_TEST := \
    android.car-test-stubs-dex

# Disable Prime Shader Cache in SurfaceFlinger to make it available faster
PRODUCT_PROPERTY_OVERRIDES += \
    service.sf.prime_shader_cache=0
