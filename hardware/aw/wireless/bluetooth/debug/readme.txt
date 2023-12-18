Main function:
1. setup gadget virtual uart device: /dev/ttyGS0
2. route bluetooth uart data & virtual uart data: /dev/ttyGS0 <=> /dev/ttyX

Kernel defconfig:
+CONFIG_U_SERIAL_CONSOLE=y
-CONFIG_USB_DUMMY_HCD=y
-CONFIG_USB_CONFIGFS_SERIAL=y
+CONFIG_USB_G_SERIAL=m

# disable console output for kernel(if neccessery)
$ echo 0 > /proc/sysrq-trigger

# disable android bluetooth
$ svc bluetooth disable

# setup uart & bridge:
$ echo 0 > /proc/sysrq-trigger
$ uart-setup -i
$ uart-bridge &

# enable bt hardware
$ echo 1 > /sys/class/rfkill/rfkill1/state
$ echo 1 > /proc/bluetooth/sleep/lpm
$ echo 1 > /proc/bluetooth/sleep/btwrite
