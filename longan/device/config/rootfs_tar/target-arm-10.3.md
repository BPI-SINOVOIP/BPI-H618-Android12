::: tip
请按照版本号降序排列（最新的在最前）
:::

# 2023-3-23

## 变化

1.新的rootfs基于buildroot-202205+gcc10.3编译

2.新增工具：perf（静态编译）、file、taskset、ldd、fio、rz、i2c-tools v4.3、ip、iproute、cansend、candump、cangen、canbusload、ethtool、tc、tcpdump、udhcpc、iperf3、lspci、lsscsi、devmem、linuxptp、ltrace、strace

* 支持某些特殊工具和脚本

add rwcheck、iozone、spec-rand.sh、 spec-seq.sh、 spec-tiny-seq.sh

## 影响

1.rootfs中的编译工具链已升级至10.3，增加新工具时同样需要使用10.3工具链，防止动态链接出错
