::: tip
请按照版本号降序排列（最新的在最前）
:::

# 2023-3-10

# 变化

1.新增工具：linuxptp、ltrace、strace

# 2023-1-17

## 变化

1.新的rootfs基于buildroot-202205+gcc10.3编译

2.新增工具：perf（静态编译）、file、taskset、ldd、fio、rz、i2c-tools v4.3、ip、iproute、cansend、candump、cangen、canbusload、ethtool、tc、tcpdump、udhcpc、iperf3、lspci、lsscsi、devmem

3.保留重要的历史修改：

* support get console info from /proc/cmdline

```
diff --git a/etc/inittab b/etc/inittab
index e909af6..a305e69 100644
--- a/etc/inittab
+++ b/etc/inittab
@@ -30,8 +30,7 @@ null::sysinit:/bin/ln -sf /proc/self/fd/2 /dev/stderr
 ::sysinit:/etc/init.d/rcS

 # Put a getty on the serial port
-#ttyS0::respawn:/sbin/getty -L  ttyS0 115200 vt100 # GENERIC_SERIAL
-::respawn:-/bin/sh
+::respawn:/sbin/getty -L `cat /proc/cmdline | awk -F ",115200" '{print $1}' | awk -F "console=" '{print $2}'` 115200 vt100 -n -l /bin/ash # GENERIC_SERIAL

 # Stuff to do for the 3-finger salute
 #::ctrlaltdel:/sbin/reboot
```

* 支持某些特殊工具和脚本

add rwcheck、iozone、spec-rand.sh、 spec-seq.sh、 spec-tiny-seq.sh、runin.sh

## 影响

1.rootfs中的编译工具链已升级至10.3，增加新工具时同样需要使用10.3工具链，防止动态链接出错
