#
# Copyright (C) 2021 The Android Open Source Project
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

PRODUCT_ENFORCE_ARTIFACT_PATH_REQUIREMENTS := relaxed
PRODUCT_PACKAGES += android.frameworks.automotive.display@1.0-service

PRODUCT_ARTIFACT_PATH_REQUIREMENT_ALLOWED_LIST += \
    root/init.bootstat.rc \
    root/init.car.rc \
    system/app/CarFrameworkPackageStubs/CarFrameworkPackageStubs.apk \
    system/app/CarFrameworkPackageStubs/oat/arm64/CarFrameworkPackageStubs.odex \
    system/app/CarFrameworkPackageStubs/oat/arm64/CarFrameworkPackageStubs.vdex \
    system/app/CarHTMLViewer/CarHTMLViewer.apk \
    system/app/CarLatinIME/CarLatinIME.apk \
    system/app/CarMapsPlaceholder/CarMapsPlaceholder.apk \
    system/app/CarPermissionControllerRRO/CarPermissionControllerRRO.apk \
    system/app/RotaryIME/RotaryIME.apk \
    system/app/RotaryIME/oat/arm64/RotaryIME.odex \
    system/app/RotaryIME/oat/arm64/RotaryIME.vdex \
    system/app/CarRotaryImeRRO/CarRotaryImeRRO.apk \
    system/app/RotaryPlayground/RotaryPlayground.apk \
    system/app/SampleCustomInputService/SampleCustomInputService.apk \
    system/app/SampleRearViewCamera/SampleRearViewCamera.apk \
    system/app/SampleRearViewCamera/oat/arm64/SampleRearViewCamera.odex \
    system/app/SampleRearViewCamera/oat/arm64/SampleRearViewCamera.vdex \
    system/app/SystemUpdater/SystemUpdater.apk \
    system/bin/android.automotive.evs.manager@1.1 \
    system/bin/carbugreportd \
    system/bin/carpowerpolicyd \
    system/bin/carwatchdogd \
    system/bin/com.android.car.procfsinspector \
    system/etc/apns-conf.xml \
    system/etc/init/android.automotive.evs.manager@1.1.rc \
    system/etc/init/carbugreportd.rc \
    system/etc/init/carpowerpolicyd.rc \
    system/etc/init/carwatchdogd.rc \
    system/etc/init/com.android.car.procfsinspector.rc \
    system/etc/init/init.bootstat.car.rc \
    system/etc/init/init.car.rc \
    system/etc/old-apns-conf.xml \
    system/etc/permissions/android.car.cluster.xml \
    system/etc/permissions/android.car.usb.handler.xml \
    system/etc/permissions/android.hardware.broadcastradio.xml \
    system/etc/permissions/android.hardware.type.automotive.xml \
    system/etc/permissions/com.android.car.activityresolver.xml \
    system/etc/permissions/com.android.car.bugreport.xml \
    system/etc/permissions/com.android.car.carlauncher.xml \
    system/etc/permissions/com.android.car.cluster.home.xml \
    system/etc/permissions/com.android.car.dialer.xml \
    system/etc/permissions/com.android.car.hvac.xml \
    system/etc/permissions/com.android.car.media.xml \
    system/etc/permissions/com.android.car.messenger.xml \
    system/etc/permissions/com.android.car.radio.xml \
    system/etc/permissions/com.android.car.rotary.xml \
    system/etc/permissions/com.android.car.settings.xml \
    system/etc/permissions/com.android.car.shell.xml \
    system/etc/permissions/com.android.car.xml \
    system/etc/permissions/com.google.android.car.defaultstoragemonitoringcompanionapp.xml \
    system/etc/permissions/com.google.android.car.garagemode.testapp.xml \
    system/etc/permissions/com.google.android.car.kitchensink.xml \
    system/etc/permissions/com.google.android.car.networking.preferenceupdater.xml \
    system/etc/sysconfig/preinstalled-packages-product-car-base.xml \
    system/etc/vintf/manifest/carpowerpolicyd.xml \
    system/etc/vintf/manifest/carwatchdogd.xml \
    system/etc/vintf/manifest/manifest_android.automotive.evs.manager@1.1.xml \
    system/framework/android.car.jar \
    system/framework/car-frameworks-service.jar \
    system/framework/oat/arm64/car-frameworks-service.odex \
    system/framework/oat/arm64/car-frameworks-service.vdex \
    system/lib/libcar-framework-service-jni.so \
    system/lib/libsuspend.so \
    system/lib64/android.automotive.watchdog-V3-cpp.so \
    system/lib64/android.automotive.watchdog.internal-cpp.so \
    system/lib64/android.frameworks.automotive.display@1.0.so \
    system/lib64/android.frameworks.automotive.powerpolicy-V1-cpp.so \
    system/lib64/android.frameworks.automotive.powerpolicy.internal-cpp.so \
    system/lib64/android.hardware.automotive.evs@1.0.so \
    system/lib64/android.hardware.automotive.evs@1.1.so \
    system/lib64/android.hardware.automotive.vehicle@2.0.so \
    system/lib64/libcar-framework-service-jni.so \
    system/lib64/libscriptexecutor.so \
    system/lib64/libscriptexecutorjni.so \
    system/lib64/libsuspend.so \
    system/lib64/libwatchdog_binder_mediator.so \
    system/lib64/libwatchdog_package_info_resolver.so \
    system/lib64/libwatchdog_perf_service.so \
    system/lib64/libwatchdog_process_service.so \
    system/media/bootanimation.zip \
    system/priv-app/BugReportApp/BugReportApp.apk \
    system/priv-app/CarActivityResolver/CarActivityResolver.apk \
    system/priv-app/CarActivityResolver/oat/arm64/CarActivityResolver.odex \
    system/priv-app/CarActivityResolver/oat/arm64/CarActivityResolver.vdex \
    system/priv-app/CarDialerApp/CarDialerApp.apk \
    system/priv-app/CarHvacApp/CarHvacApp.apk \
    system/priv-app/CarLauncher/CarLauncher.apk \
    system/priv-app/CarMediaApp/CarMediaApp.apk \
    system/priv-app/CarMessengerApp/CarMessengerApp.apk \
    system/priv-app/CarRadioApp/CarRadioApp.apk \
    system/priv-app/CarRotaryController/CarRotaryController.apk \
    system/priv-app/CarService/CarService.apk \
    system/priv-app/CarService/lib/arm64/libscriptexecutorjni.so \
    system/priv-app/CarService/lib/x86_64/libscriptexecutorjni.so \
    system/priv-app/CarService/oat/arm64/CarService.odex \
    system/priv-app/CarService/oat/arm64/CarService.vdex \
    system/priv-app/CarSettings/CarSettings.apk \
    system/priv-app/CarShell/CarShell.apk \
    system/priv-app/CarShell/oat/arm64/CarShell.odex \
    system/priv-app/CarShell/oat/arm64/CarShell.vdex \
    system/priv-app/CarUsbHandler/CarUsbHandler.apk \
    system/priv-app/CarUsbHandler/oat/arm64/CarUsbHandler.odex \
    system/priv-app/CarUsbHandler/oat/arm64/CarUsbHandler.vdex \
    system/priv-app/ClusterHomeSample/ClusterHomeSample.apk \
    system/priv-app/ClusterHomeSample/oat/arm64/ClusterHomeSample.odex \
    system/priv-app/ClusterHomeSample/oat/arm64/ClusterHomeSample.vdex \
    system/priv-app/DefaultStorageMonitoringCompanionApp/DefaultStorageMonitoringCompanionApp.apk \
    system/priv-app/DirectRenderingCluster/DirectRenderingCluster.apk \
    system/priv-app/DirectRenderingCluster/oat/arm64/DirectRenderingCluster.odex \
    system/priv-app/DirectRenderingCluster/oat/arm64/DirectRenderingCluster.vdex \
    system/priv-app/EmbeddedKitchenSinkApp/EmbeddedKitchenSinkApp.apk \
    system/priv-app/ExperimentalCarService/ExperimentalCarService.apk \
    system/priv-app/ExperimentalCarService/oat/arm64/ExperimentalCarService.odex \
    system/priv-app/ExperimentalCarService/oat/arm64/ExperimentalCarService.vdex \
    system/priv-app/GarageModeTestApp/GarageModeTestApp.apk \
    system/priv-app/LocalMediaPlayer/LocalMediaPlayer.apk \
    system/priv-app/NetworkPreferenceApp/NetworkPreferenceApp.apk \
    system/lib64/libcarservicejni.so \
    system/priv-app/CarService/lib/arm64/libcarservicejni.so \
    system/priv-app/CarService/lib/x86_64/libcarservicejni.so \
    system/bin/android.automotive.telemetryd@1.0 \
    system/etc/init/android.automotive.telemetryd@1.0.rc \
    system/etc/vintf/manifest/android.automotive.telemetryd@1.0.xml \
    system/lib64/android.automotive.telemetryd@1.0-impl.so \
    system/lib64/android.frameworks.automotive.telemetry-V1-cpp.so \
    system/lib64/android.automotive.telemetry.internal-ndk_platform.so \
    system/lib64/android.frameworks.automotive.telemetry-V1-ndk_platform.so \
    system/etc/automotive/watchdog/system_resource_overuse_configuration.xml \
    system/etc/automotive/watchdog/third_party_resource_overuse_configuration.xml \
    system/bin/android.frameworks.automotive.display@1.0-service \
    system/etc/init/android.frameworks.automotive.display@1.0-service.rc \
    system/etc/vintf/manifest/manifest_android.frameworks.automotive.display@1.0.xml \

PRODUCT_ARTIFACT_PATH_REQUIREMENT_ALLOWED_LIST += %.odex %.vdex %.art

PRODUCT_PACKAGE_OVERLAYS := device/generic/car/common/overlay
EMULATOR_VENDOR_NO_SENSORS := true
PRODUCT_USE_DYNAMIC_PARTITION_SIZE := true
$(call inherit-product, device/generic/car/emulator/aosp_car_emulator.mk)
EMULATOR_VENDOR_NO_SOUND := true
