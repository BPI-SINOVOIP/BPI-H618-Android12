# Getting started with Protected Virtual Machines

## Prepare a device

First you will need a device that is capable of running virtual machines. On arm64, this means
a device which boots the kernel in EL2 and the kernel was built with KVM enabled.

Here are instructions for select devices:

 * [yukawa: Khadas VIM3L](yukawa.md) (arm64)
 * [goldfish: Android Emulator](goldfish.md) (x86_64)

## Running tests

Virtualization source code and relevant tests are located in
[packages/modules/Virtualization](https://android.googlesource.com/platform/packages/modules/Virtualization)
of the AOSP repository.

### Device-side tests

The tests spawn guest VMs and test different aspects of the architecture.

You can build and run them with:

```shell
atest VirtualizationTestCases
```

If you run into problems, inspect the logs produced by `atest`. Their location is printed at the
end. The `host_log_*.zip` file should contain the output of individual commands as well as VM logs.

## CrosVM

[CrosVM](https://android.googlesource.com/platform/external/crosvm/) is a Rust-based Virtual Machine
Monitor (VMM) originally built for ChromeOS and ported to Android.

It is not installed in regular Android builds (yet!), but it's installed in the VIM3L (yukawa)
build, as part of the `com.android.virt` APEX.

### Spawning your own VMs

You can spawn your own VMs by passing a JSON config file to the Virt Manager via the `vm` tool on a
rooted KVM-enabled device. If your device is attached over ADB, you can run:

```shell
$ cat > vm_config.json
{
  "kernel": "/data/local/tmp/kernel",
  "initrd": "/data/local/tmp/ramdisk",
  "params": "rdinit=/bin/init"
}
$ adb root
$ adb push <kernel> /data/local/tmp/kernel
$ adb push <ramdisk> /data/local/tmp/ramdisk
$ adb push vm_config.json /data/local/tmp/vm_config.json
$ adb shell "start virtmanager"
$ adb shell "/apex/com.android.virt/bin/vm run /data/local/tmp/vm_config.json"
```

The `vm` command also has other subcommands for debugging; run `/apex/com.android.virt/bin/vm help`
for details.

### Building and updating CrosVM and Virt Manager

You can update CrosVM and the Virt Manager service by updating the `com.android.virt` APEX. If your
device already has `com.android.virt` (e.g. VIM3L):

```shell
$ TARGET_BUILD_APPS="com.android.virt" m
$ adb install $ANDROID_PRODUCT_OUT/system/apex/com.android.virt.apex
$ adb reboot
```

If it doesn't have the APEX yet, you first need to place it manually to the
system partition.

```shell
$ adb root
$ adb disable-verity
$ adb reboot
$ adb wait-for-device root
$ adb remount
$ m com.android.virt
$ adb sync
$ adb reboot
```

Once the APEX is in `/system/apex`, you can use `adb install` to update it
further.
