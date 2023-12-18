# Khadas VIM3L (yukawa)

The [Khadas VIM3L](https://www.khadas.com/vim3l) is an extremely hackable development board with an
Amlogic Armv8.2 SoC and complete upstream support in U-boot, Linux and even
[AOSP](https://android.googlesource.com/device/amlogic/yukawa/+/refs/heads/master).
That makes it a compelling target for testing virtualization.

The [prebuilt kernel](https://android.googlesource.com/device/amlogic/yukawa-kernel/+/refs/heads/master)
in AOSP is currently not GKI, but it is close and kept up to date.

Note that the `yukawa` target has SELinux policy set to `permissive`.

Resources:
  * [AOSP instructions](https://android.googlesource.com/device/amlogic/yukawa/+/refs/heads/master/sei610/README)
    for flashing a bootloader with `fastboot` support
  * [Manufaturer's wiki](https://docs.khadas.com/vim3/index.html) for things like setting up UART
    and entering recovery mode
  * [go/vim3l](https://goto.google.com/vim3l) is a more detailed document but only accessible to
    Google employees

Build Android for the board:
``` shell
$ . build/envsetup.sh
$ lunch yukawa-userdebug
$ export TARGET_VIM3L=true
$ export TARGET_KERNEL_USE=5.10
$ m
```

Flash your device and reboot.
