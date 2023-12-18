# Automotive Telemetry Service

A structured log collection service for CarTelemetryService. See ARCHITECTURE.md to learn internals.

## Useful Commands

**Dumping the service information**

`adb shell dumpsys android.automotive.telemetry.internal.ICarTelemetryInternal/default`

**Enabling VERBOSE logs**

```
adb shell setprop log.tag.android.automotive.telemetryd@1.0 V
adb shell setprop log.tag.cartelemetryd_impl_test V
```

**Starting emulator with cold boot**

`emulator -verbose -show-kernel -selinux permissive -writable-system -no-snapshot -wipe-data`

**Running the tests**

`atest cartelemetryd_impl_test:CarTelemetryInternalImplTest#TestSetListenerReturnsOk`

`atest cartelemetryd_impl_test`

## Enabling cartelemetryd

`cartelemertryd` service is by default not included in the final build. To include it on specific
targets, add the following lines in necessary `.mk` files:

```
# Enable Automotive Telemetry Services (cartelemetryd) only on specific devices.
ifneq ($(filter $(TARGET_PRODUCT), TARGET1 TARGET2),)
include packages/services/Car/cpp/telemetry/cartelemetryd/products/telemetry.mk
endif
```

To find where the service was included, run this from `$ANDROID_BUILD_TOP`:

```
grep -rH --color --exclude-dir='**/.*' --exclude-dir='out' --include='*.mk' \
    'cartelemetryd/products/telemetry.mk' device/ vendor/
```
