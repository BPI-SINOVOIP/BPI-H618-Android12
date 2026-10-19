# BPI-CM7 Android 12 修改汇总

更新时间：2026-10-20

## 1. 汇总范围

本文记录 `BPI-H618-Android12` 相对 `origin/master` 的 CM7 适配改动，包括：

- 已提交但尚未推送的提交 `64c6424491`：Wi-Fi K016/K019 NVRAM 支持。
- 当前工作区尚未提交的 CM7 产品、Longan 板级配置、设备树及驱动修改。
- 从 BPI-M4Zero 复用且重命名为 CM7 的板级文件。

当前分支状态：`master` 相对 `origin/master` 超前 1 个提交，CM7 适配改动尚未提交。

## 2. CM7 Android 产品定义

### 2.1 修改的文件

| 文件 | 修改内容 |
| --- | --- |
| `device/softwinner/apollo/AndroidProducts.mk` | 注册 `bananapi_cm7.mk`，增加 `bananapi_cm7-eng`、`bananapi_cm7-userdebug` 和 `bananapi_cm7-user` 三个 lunch 目标。 |
| `README.md` | 增加 CM7 的 Longan `autoconfig`、Android `lunch` 构建示例。 |

### 2.2 新增的产品文件

| 文件/目录 | 说明 |
| --- | --- |
| `device/softwinner/apollo/bananapi_cm7.mk` | CM7 产品定义。设置产品名、设备名、板型和型号；复用 M4Zero 原有的 `DragonAtt` 与 `SoundRecorder` 产品包配置。 |
| `device/softwinner/apollo/bananapi-cm7/BoardConfig.mk` | Android 板级编译配置，从 M4Zero 版型复用。 |
| `device/softwinner/apollo/bananapi-cm7/camera/` | Camera 配置、媒体 profile、sensor list 和初始化 RC。 |
| `device/softwinner/apollo/bananapi-cm7/system/` | 分区表、TOC 配置及 vendor ramdisk 模块清单。 |
| `device/softwinner/apollo/bananapi-cm7/wireless/bluetooth/` | 蓝牙 vendor 配置，增加 CM7 命名的 `vnd_bananapi_cm7.txt`。 |

`bananapi_cm7.mk` 的主要产品标识如下：

```make
PRODUCT_NAME := bananapi_cm7
PRODUCT_DEVICE := bananapi-cm7
PRODUCT_BOARD := cm7
PRODUCT_MODEL := Bananapi BPI-CM7
```

## 3. Longan 板型配置

新增目录：

```text
longan/device/config/chips/h618/configs/cm7/
```

目录以 `m4zero` 为基础建立，包含 Android、DragonBoard、Longan、U-Boot、Linux 4.9/5.4 和分区配置。主要差异如下：

| 文件 | 修改内容 |
| --- | --- |
| `configs/cm7/android/BoardConfig.mk` | 内核配置切换为 `sun50iw9p1smp_bananapi_cm7_android_defconfig`。 |
| `configs/cm7/sys_config.fex` | 将打包机型由 `m4zero` 改为 `cm7`。 |
| `configs/cm7/uboot-board.dts` | 型号改为 `Bananapi BPI-CM7`，增加 `bananapi,bpi-cm7` compatible。 |
| `configs/cm7/board.dts` | 符号链接到 `linux-5.4/board.dts`。 |
| `configs/cm7/linux-5.4/board.dts` | CM7 的主要硬件适配，详见下一节。 |
| `configs/cm7/linux-4.9/board.dts` | 保留的 Linux 4.9 板级配置。 |
| `configs/cm7/android/sys_partition.fex` | Android 分区配置。 |
| `configs/cm7/dragonboard/*` | DragonBoard 工厂测试环境配置。 |
| `configs/cm7/linux/*` | Linux 构建与分区配置。 |
| `configs/cm7/longan/*` | Longan 构建配置。 |

新增内核 defconfig：

```text
longan/kernel/linux-5.4/arch/arm64/configs/
sun50iw9p1smp_bananapi_cm7_android_defconfig
```

该文件当前从 M4Zero Android defconfig 复用，提供独立的 CM7 配置入口，方便后续按板型调整。

## 4. CM7 Linux 5.4 设备树修改

文件：

```text
longan/device/config/chips/h618/configs/cm7/linux-5.4/board.dts
```

CM7 当前的关键硬件配置如下；其中多数条目是在 M4Zero 版型基础上调整，EPHY 配置则直接沿用其可用设置：

| 功能 | 改动 |
| --- | --- |
| 板型标识 | `model` 改为 `Bananapi BPI-CM7`，增加 `bananapi,bpi-cm7` compatible。 |
| PC7 冲突 | 禁用使用 PC7 的 `gpio-keys`，因为 CM7 Carrier 将 PC7 交给 `gpio_para`。 |
| 以太网 LED | 增加 PG16、低电平有效的 `eth_act` LED。 |
| SD 卡检测 | 使用 PF6 作为 `cd-gpios`，即原理图中的 `SDC_DET`。 |
| I²C EEPROM | 启用 TWI3，增加地址 `0x56` 和 `0x57` 的两个 24C02 EEPROM 节点。 |
| Wi-Fi 引脚 | `WL_REG_ON` 改为 PG14，`WL_HOST_WAKE` 改为 PG11。 |
| Wi-Fi 时序 | 增加 `wlan_power_on_delay_ms = <200>`，SDIO 枚举前等待模块退出复位。 |
| 蓝牙引脚 | `BT_WAKE` 改为 PG12，`BT_HOST_WAKE` 改为 PG13；复位脚使用 PG19。 |
| 显示模式 | `disp_mode` 从 0 改为 5。 |
| 板载音频 | 将 `pa_pin_max` 改为 0，取消 M4Zero 的外置功放 GPIO 配置，避免错误控制不存在的 PA。 |
| 以太网 PHY | 沿用 `gmac1` RMII 和 H618 内置 EPHY 配置；实机已识别 PHY ID `0x00441400`。 |

## 5. Wi-Fi K016/K019 支持

### 5.1 已提交的文件

提交：`64c6424491 wifi: add K016 and K019 NVRAM support`

| 文件 | 修改内容 |
| --- | --- |
| `hardware/aw/wireless/partner/ampak/firmware/k016/nvram_k016.txt` | 新增 K016 NVRAM。 |
| `hardware/aw/wireless/partner/ampak/firmware/k016/nvram_k019.txt` | 新增 K019 NVRAM。 |
| `hardware/aw/wireless/partner/ampak/firmware/device-wlan.mk` | 将 `nvram_k016.txt` 打包到 vendor firmware。工作区又追加了 K019 打包规则。 |
| `longan/kernel/linux-5.4/drivers/net/wireless/bcmdhd/dhd_config.c` | BCM43430 对应 K016、BCM4345/BCM43454 对应 K019；同时修正部分 K016 OTP 将模块错误报告为 `ap6255` 的情况。 |

### 5.2 当前尚未提交的补充修改

| 文件 | 修改内容 |
| --- | --- |
| `hardware/aw/wireless/partner/ampak/firmware/device-wlan.mk` | 增加 `nvram_k019.txt` 的复制规则。 |
| `hardware/aw/wireless/wlan/firmware/broadcom/device-bcm.mk` | 将 `k016/nvram_k019.txt` 显式复制为 vendor 下的 `nvram_k019.txt`。 |

### 5.3 SDIO 上电延时

| 文件 | 修改内容 |
| --- | --- |
| `longan/kernel/linux-5.4/drivers/misc/sunxi-rf/sunxi-rfkill.h` | 在 WLAN 平台数据中增加 `power_on_delay_ms`。 |
| `longan/kernel/linux-5.4/drivers/misc/sunxi-rf/sunxi-wlan.c` | 读取 DTS 的 `wlan_power_on_delay_ms`；拉高 `WL_REG_ON` 后按配置延时。 |

该修改用于避免 Wi-Fi 模组尚未退出复位，MMC/SDIO 就开始发送 CMD5/CMD52，导致首次枚举失败。

## 6. 以太网 PHY 驱动修正

文件：

```text
longan/kernel/linux-5.4/drivers/net/ethernet/allwinner/sunxi-gmac.c
```

修改内容：将 `mdiobus_scan()` 结果判断由只检查空指针：

```c
if (!phydev_tmp)
```

改为同时检查错误指针：

```c
if (IS_ERR_OR_NULL(phydev_tmp))
```

这样在某个 MDIO 地址没有 PHY 响应时不会继续解引用 `ERR_PTR()`，避免启动期间崩溃，并允许继续扫描实际存在的内置 EPHY。

## 7. DragonAtt 工厂及老化测试

CM7 从 M4Zero 版型复用了以下产品包配置：

```make
PRODUCT_PACKAGES += DragonAtt
PRODUCT_PACKAGES += SoundRecorder
```

`DragonAtt` 是预编译的综合板测和 Aging 工具：

```text
vendor/aw/public/prebuild/apk/DragonAtt/DragonAtt.apk
```

包名及常用入口：

```sh
# 主界面
am start -n com.softwinner.dragonatt/.ui.MainActivity

# Aging 配置
am start -n com.softwinner.dragonatt/.ui.config.AgingConfigActivity

# Aging 测试
am start -n com.softwinner.dragonatt/.ui.aging.AgingActivity
```

SD 卡或 U 盘上的 Aging 配置路径为：

```text
DragonATT/dragon_att_config_aging.xml
```

注意：`DragonAtt` 可以读取非模拟外部存储中的配置，但当前 APK 没有监听 `MEDIA_MOUNTED`，因此插入 SD 卡不会自动打开应用。

## 8. 构建方法

### 8.1 Longan/内核

```sh
cd longan
./build.sh autoconfig -i h618 -o android -n default -a arm64 -k linux-5.4 -b cm7
./build.sh
```

### 8.2 Android 和打包

```sh
source build/envsetup.sh
lunch bananapi_cm7-userdebug
make -j20
pack
```

已生成并用于测试的镜像：

```text
longan/out/2026-10-19-bananapi-cm7-android12.img
```

镜像大小约 1.3 GiB。

## 9. 当前验证结果

| 项目 | 状态 |
| --- | --- |
| Android 镜像编译和打包 | 已完成 |
| eMMC/SD 启动 | 已进入 Android |
| SD 卡检测 PF6 | 已按原理图配置 |
| 内置 EPHY | 已识别，PHY ID 为 `0x00441400` |
| 以太网链路 | 已建立 |
| Wi-Fi SDIO | 已枚举 |
| Wi-Fi K019 | 已加载对应 NVRAM，`wlan0` 可以扫描 |
| DragonAtt | 已包含在 CM7 产品包列表中 |

## 10. 当前 Git 文件状态

已修改但尚未提交：

```text
README.md
device/softwinner/apollo/AndroidProducts.mk
hardware/aw/wireless/partner/ampak/firmware/device-wlan.mk
hardware/aw/wireless/wlan/firmware/broadcom/device-bcm.mk
longan/kernel/linux-5.4/drivers/misc/sunxi-rf/sunxi-rfkill.h
longan/kernel/linux-5.4/drivers/misc/sunxi-rf/sunxi-wlan.c
longan/kernel/linux-5.4/drivers/net/ethernet/allwinner/sunxi-gmac.c
```

新增但尚未提交：

```text
device/softwinner/apollo/bananapi-cm7/
device/softwinner/apollo/bananapi_cm7.mk
longan/device/config/chips/h618/configs/cm7/
longan/kernel/linux-5.4/arch/arm64/configs/sun50iw9p1smp_bananapi_cm7_android_defconfig
```

本文档本身也是新增文件：

```text
ANDROID_CM7_CHANGELOG.md
```
