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

# Base platform for car builds
# car packages should be added to car.mk instead of here

ifeq ($(DISABLE_CAR_PRODUCT_CONFIG_OVERLAY),)
PRODUCT_PACKAGE_OVERLAYS += packages/services/Car/car_product/overlay
endif

ifeq ($(DISABLE_CAR_PRODUCT_VISUAL_OVERLAY),)
PRODUCT_PACKAGE_OVERLAYS += packages/services/Car/car_product/overlay-visual
endif

PRODUCT_COPY_FILES += \
    packages/services/Car/car_product/build/component-overrides.xml:$(TARGET_COPY_OUT_VENDOR)/etc/sysconfig/component-overrides.xml \

PRODUCT_PACKAGES += \
    com.android.wifi \
    Home \
    BasicDreams \
    CaptivePortalLogin \
    CertInstaller \
    DocumentsUI \
    DownloadProviderUi \
    FusedLocation \
    InputDevices \
    KeyChain \
    Keyguard \
    LatinIME \
    Launcher2 \
    ManagedProvisioning \
    PacProcessor \
    PrintSpooler \
    ProxyHandler \
    Settings \
    SharedStorageBackup \
    VpnDialogs \
    MmsService \
    ExternalStorageProvider \
    atrace \
    libandroidfw \
    libaudioutils \
    libmdnssd \
    libnfc_ndef \
    libpowermanager \
    libvariablespeed \
    A2dpSinkService \
    PackageInstaller \
    carbugreportd \

# ENABLE_CAMERA_SERVICE must be set as true from the product's makefile if it wants to support
# Android Camera service.
ifneq ($(ENABLE_CAMERA_SERVICE), true)
PRODUCT_PROPERTY_OVERRIDES += config.disable_cameraservice=true
endif

# EVS service
include packages/services/Car/cpp/evs/manager/evsmanager.mk

ifeq ($(ENABLE_EVS_SAMPLE), true)
# ENABLE_EVS_SAMPLE should set be true or their vendor specific equivalents should be included in
# the device.mk with the corresponding selinux policies
PRODUCT_PRODUCT_PROPERTIES += persist.automotive.evs.mode=0
PRODUCT_PACKAGES += evs_app \
                    android.hardware.automotive.evs@1.1-sample \
                    android.frameworks.automotive.display@1.0-service
include packages/services/Car/cpp/evs/apps/sepolicy/evsapp.mk
include packages/services/Car/cpp/evs/sampleDriver/sepolicy/evsdriver.mk
endif
ifeq ($(ENABLE_CAREVSSERVICE_SAMPLE), true)
PRODUCT_PACKAGES += CarEvsCameraPreviewApp \
                    CarSystemUIEvsRRO
endif
ifeq ($(ENABLE_REAR_VIEW_CAMERA_SAMPLE), true)
PRODUCT_PACKAGES += SampleRearViewCamera
PRODUCT_PACKAGE_OVERLAYS += packages/services/Car/tests/SampleRearViewCamera/overlay
endif

# Device running Android is a car
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.type.automotive.xml:system/etc/permissions/android.hardware.type.automotive.xml

# Default permission grant exceptions
PRODUCT_COPY_FILES += \
    packages/services/Car/car_product/build/preinstalled-packages-product-car-base.xml:system/etc/sysconfig/preinstalled-packages-product-car-base.xml

$(call inherit-product, $(SRC_TARGET_DIR)/product/core_minimal.mk)

# Default dex optimization configurations
PRODUCT_PROPERTY_OVERRIDES += \
     pm.dexopt.disable_bg_dexopt=true

# Required init rc files for car
PRODUCT_COPY_FILES += \
    packages/services/Car/car_product/init/init.bootstat.rc:system/etc/init/init.bootstat.car.rc \
    packages/services/Car/car_product/init/init.car.rc:system/etc/init/init.car.rc

# Enable car watchdog
include packages/services/Car/cpp/watchdog/product/carwatchdog.mk

# Enable car power policy
include packages/services/Car/cpp/powerpolicy/product/carpowerpolicy.mk
