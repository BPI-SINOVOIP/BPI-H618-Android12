#!/bin/sh
##############################################################################
# \version     1.0.0
# \date        2014.04.11
# \author      MINI_NGUI<liaoyongming@allwinnertech.com>
# \Descriptions:
#			add test LED for homlet
##############################################################################
source send_cmd_pipe.sh
source script_parser.sh
source log.sh

LOG_TAG="ledtester"

module_path=`script_fetch "gpio_led" "module_path"`
led_count=`script_fetch "gpio_led" "led_count"`

if [ -z "$module_path" ]; then
	LOGE "no ${module_path} to install"
	SEND_CMD_PIPE_FAIL $3
	exit 1
else
	LOGD "begin install ${module_path}"
	insmod "$module_path"
	if [ $? -ne 0 ]; then
		LOGE "inStall ${module_path} failed"
	fi
fi

node_path=`script_fetch "gpio_led" "node_path"`
if [ -z "$node_path" ]; then
	node_path="/sys/class/gpio_sw"
fi

node_name=`script_fetch "gpio_led" "node_name"`
if [ -z "$node_name" ]; then
	node_name="light"
fi


if [ $led_count -gt 0 ] ; then
	while true ; do
		for i in $(seq $led_count); do
			temp_name="led"$i"_name"
			led_name=`script_fetch "gpio_led" "$temp_name"`
			full_node_path=$node_path"/"$led_name
			if [ -n led_name ] ; then
				if [ ! -d "$full_node_path" ]; then
					LOGE "has no ${led_name} node,mabey cant intall ${module_path}"
					SEND_CMD_PIPE_FAIL $3
					exit 1
				else
					echo 1 > $full_node_path"/"$node_name
					if [ $? -ne 0 ]; then
						LOGE "set ${led_name} light to 1 err"
						SEND_CMD_PIPE_FAIL $3
						exit 1
					fi
					usleep 200000
					echo 0 > $full_node_path"/"$node_name
					if [ $? -ne 0 ]; then
						LOGE "set ${led_name} light to 0 err"
						SEND_CMD_PIPE_FAIL $3
						exit 1
					fi
				fi
			fi
		done
		SEND_CMD_PIPE_OK_EX $3
		usleep 200000
	done
fi
LOGE "no led to test"
SEND_CMD_PIPE_FAIL $3



