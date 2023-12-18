#!/system/bin/sh

if [[ -f /data/misc/adb/adb_keys ]]; then
    echo "qemu-adb-setup: /data/misc/adb/adb_keys exists. Skipping qemu adb setup" > /dev/kmsg
else
    echo "qemu-adb-setup: Copying over adb_keys" > /dev/kmsg
    setprop vendor.qemu.adb.copykey 1
fi
