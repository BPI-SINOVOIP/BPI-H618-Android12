## 背景

提升DRAM高低温问题暴露的概率

缩减DRAM高低温环境搭建的人力投入

尽量排除与DRAM无关的因素的影响



## 功能实现

支持测试项：memtest*4，休眠唤醒，reboot测试。

测试项规则：整体支持循环次数，测试项参数可修改，测试项顺序可调可增减。



## 配置文件

配置文件，格式如下：

```
loop=2
memtester_test 8M 2
sleepawake_test 6 10 5
reboot_test 2
```

其中：

- loop： 表示循环次数
- memtester_test： memtester测试，后面参数为memtester所带参数，启动4线程同时运行测试。
- sleepawake_test：休眠唤醒测试，后面参数依次为：唤醒时间、休眠时间及循环次数。
- reboot_test：循环重启测试，参数为循环次数。

默认路径为：/system/etc/runin_config，可以将配置文件复制到/sdcard/runin_config中，会覆盖系统的默认测试配置。



## 测试

输入命令

```shell
setprop debug.runin.restart 1
```

即可开始测试。



## 调试及打印

相关输出信息在/data/local/tmp/.runin目录，包含

- case_index：当前测试项，循环结束则删除该文件。
- loop_index：当前循环次数。
- reboot_time：当前循环重启次数，测完重启则删除该文件。
- log：日志输出文件。

其中日志输出，如下：

```shell
06-23 19:39:05 running loop 0
06-23 19:39:05 execute __runin_memtester_test 8M 2
06-23 19:39:06 memtester 8M 2 x 4
06-23 19:39:50 memtester test done
06-23 19:39:50 execute __runin_sleepawake_test 6 10 5
06-23 19:39:50 sleepawake test 6 10 5
06-23 19:39:56 going to sleep 0
06-23 19:40:07 wakeup 0
06-23 19:40:15 going to sleep 1
06-23 19:40:27 wakeup 1
06-23 19:40:35 going to sleep 2
06-23 19:40:46 wakeup 2
06-23 19:40:54 going to sleep 3
06-23 19:41:05 wakeup 3
06-23 19:41:13 going to sleep 4
06-23 19:41:24 wakeup 4
06-23 19:41:26 sleepawake test done
06-23 19:41:26 execute __runin_reboot_test 2
06-23 19:41:26 reboot test 2
06-23 19:41:26 reboot 0
06-23 19:42:30 execute __runin_reboot_test 2
06-23 19:42:30 reboot 1
06-23 19:43:30 execute __runin_reboot_test 2
06-23 19:43:31 reboot test done
06-23 19:43:31 running loop 1
06-23 19:43:32 execute __runin_memtester_test 8M 2
06-23 19:43:32 memtester 8M 2 x 4
06-23 19:44:24 memtester test done
06-23 19:44:24 execute __runin_sleepawake_test 6 10 5
06-23 19:44:24 sleepawake test 6 10 5
06-23 19:44:31 going to sleep 0
06-23 19:44:42 wakeup 0
06-23 19:44:51 going to sleep 1
06-23 19:45:02 wakeup 1
06-23 19:45:11 going to sleep 2
06-23 19:45:22 wakeup 2
06-23 19:45:31 going to sleep 3
06-23 19:45:42 wakeup 3
06-23 19:45:51 going to sleep 4
06-23 19:46:02 wakeup 4
06-23 19:46:05 sleepawake test done
06-23 19:46:05 execute __runin_reboot_test 2
06-23 19:46:05 reboot test 2
06-23 19:46:05 reboot 0
06-23 19:47:00 execute __runin_reboot_test 2
06-23 19:47:01 reboot 1
06-23 19:47:55 execute __runin_reboot_test 2
06-23 19:47:56 reboot test done
06-23 19:47:56 runin test done
```


