# 简介

DragonRunin自动化老化测试工具，无需通过检查样机日志，直接通过LED板卡直接显示测试状态结果，测试结果高效、稳定、可靠，通过工具可以快速排查出物料、以及系统的稳定性问题，为研发测试提供高效的自动化工具。

# 集成测试方法

修改：android/device/softwinner/common/common.mk，添加

```makefile
# 引用测试包
include vendor/aw/public/package/apk/DragonRunin/runin.mk
```

* 产线为了加快生产速度，建议同时在方案mk中删除gms包引用，减少固件体积同时加快启动的时间。

# 修改测试配置

* 通过app：点击Runin-Config打开配置界面，根据界面选择配置，点击保存即可，配置文件保存路径：/sdcard/DragonFire/runin/config.xml
* 手动修改或将a步骤中配置文件放到默认位置：android/vendor/aw/public/package/apk/DragonRunin/RuninConfig/assets/default_config.xml

```xml
<?xml version="1.0" encoding="utf-8" ?>
<!-- test为测试开关，ture则可以手动跳过当前项测试；cycle为循环测试次数，整体测试循环20次，以下配置共耗时约2.5hx20=50h -->
<Root test="false" cycle="20">
    <Foreground>
        <!-- 视频老化测试，时长1800000ms=30min，src为播放视频路径，默认使用apk自带视频不用配置-->
        <VideoTest duration="1800000">
            <src>/mnt/sdcard/runin/video</src>
        </VideoTest>
        <!-- GPU 3D老化测试，时长1800000ms=30min-->
        <ThreeDimensionalTest duration="1800000" />
        <!-- 内存测试，每次约12s，重复150次，约30min -->
        <MemtesterTest repeat="150" />
        <!-- 休眠唤醒测试，亮屏时间6s，灭屏时间24s，重复60次，约30min -->
        <SleepwakeTest open="6000" close="24000" repeat="60" />
        <!-- 重启老化测试，重复25次，约30min -->
        <RebootTest repeat="25">
            <reason>runintest</reason>
        </RebootTest>
    </Foreground>
    <Background>
    </Background>
</Root>
```

# 测试命令
## 重新测试
```
am broadcast -a android.intent.action.BOOT_COMPLETED -n com.softwinner.runin/com.softwinner.autorun.BootReceiver --ez restart true
```

# led灯状态说明

| 机器状态         | led灯状态                |
| ---------------- | ------------------------ |
| 开机启动过程     | 随机                     |
| 关机状态         | 随机                     |
| 休眠状态         | 随机                     |
| 开始老化测试     | 红、绿灯同时0.5s间隔闪烁 |
| 测试结果通过     | 绿灯0.5s间隔闪烁         |
| 测试结果失败     | 红灯0.5s间隔闪烁         |
| 测试手动停止退出 | 灭灯                     |

# 日志分析

日志文件存放路径为：/sdcard/DragonFire/runin/logcat，其中包括：

* runninlog.csv

用于显示结果，如：

```csv
VideoTest,ThreeDimensionalTest,TwoDimensionalTest,MemtesterTest,SleepwakeTest,RebootTest
Pass,Pass,NA,Fail,Pass 60 Loop,Pass 30 Loop
```

其中：Pass表示对应项测试通过，NA表示测试项未测试，Fail表示测试项测试失败。

* runinlog.txt

记录了每个测试项开始和结束以及一些测试用例参数，用于判断当前/中断时，runnin工具测试到哪一项以及具体时间。可以用来初步判断死机或系统重启等情况触发时运行的用例。例如：

```log
2020-05-14 08:25:51 video start duration:3000ms
2020-05-14 08:25:57 video stop
2020-05-14 08:25:57 3d start duration:3000ms
2020-05-14 08:26:02 3d stop
2020-05-14 08:26:02 memtester start 2
2020-05-14 08:26:28 memtester stop
2020-05-14 08:26:28 sleepwake start open:3000ms close:8000ms
2020-05-14 08:26:28 sleepwake off :1x
2020-05-14 08:26:36 sleepwake on :1x
2020-05-14 08:26:39 sleepwake off :2x
2020-05-14 08:26:47 sleepwake on :2x
2020-05-14 08:26:50 sleepwake off :3x
2020-05-14 08:26:58 sleepwake on :3x
2020-05-14 08:26:58 sleepwake stop
2020-05-14 08:26:58 reboot start repeat:1
2020-05-14 08:27:54 reboot pass: 1x
2020-05-14 08:27:54 reboot stop
2020-05-14 08:27:54 cycle complete:1
```

* 详细log文件，以timestamp命名

应用进程的logcat打印，用于调试测试失败的问题
