# Android Emulator (goldfish)

The built-in local emulator is the quickest way how to get started with KVM and Android.

## x86_64

KVM on x86_64 does not provide the same guest protection as arm64 but you will be able to spawn
virtual machines and use the same APIs to communicate with the guest. The main reason for choosing
the x86_64 emulator over its arm64 counterpart is performance. With native virtualization it is
easily 10x faster than arm64 emulation.

For optimal performance make sure to
[enable nested virtualization](https://www.linux-kvm.org/page/Nested_Guests) on your machine.
Don't forget to add your user account into the `kvm` group, then re-login for it to take effect.
``` shell
$ sudo gpasswd -a $USER kvm
```

Build Android for the emulator:
``` shell
$ . build/envsetup.sh
$ lunch sdk_phone_x86_64-eng
$ m -j$(nproc)
```

Once you have an Android image, invoke `emulator`. The script will automatically find the image you
just built and run it in QEMU.
``` shell
$ emulator -no-window -show-kernel -writable-system -qemu -cpu host
```
Explanation of the arguments:
  * `-no-window`: run headless
  * `-show-kernel`: print kernel UART logs to the console (useful for debugging),
  * `-writable-system`: support remounting `system/` as writable, needed for `adb sync`,
  * `-qemu -cpu host`: needed to enable nested virtualization, instructs QEMU to allow Android
    access CPU features of the host kernel

If you get an error saying “x86_64 emulation currently requires hardware acceleration!”, your
user account is not in the `kvm` group (see above).

You should now see the virtual device when you run:
``` shell
$ adb devices
List of devices attached
emulator-5554   device
```
