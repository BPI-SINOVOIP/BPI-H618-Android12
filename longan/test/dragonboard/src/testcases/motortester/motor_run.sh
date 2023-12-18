#!/bin/sh

##############################################################################
# \version     1.0.0
# \date        2022-05-26
# \author      wangs <wangs@allwinnertech.com>
# \Descriptions:
#			   让振动马达每隔3秒振动一下
##############################################################################

source send_cmd_pipe.sh
source script_parser.sh


# 客户可视情况加载 vibrator.ko 驱动
# insmod /vendor/lib/modules/sunxi-vibrator.ko

# 添加可执行文件的操作权限
chown system system /sys/class/timed_output/vibrator/enable 
chmod 0660 /sys/class/timed_output/vibrator/enable

while true ; do
	motorpath="/sys/class/timed_output/vibrator/enable"
	if [ -e $motorpath ]; then
		echo 1 > $motorpath
		sleep 3
		echo 0 > $motorpath
	# 客户可视情况将下面else内容放开，用以提示找不到振动马达设备	
	#else
		#SEND_CMD_PIPE_FAIL_EX $3 "No Vibrator"
	fi
	sleep 3
done