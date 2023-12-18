#!/bin/sh

source send_cmd_pipe.sh
source script_parser.sh
source log.sh

LOG_TAG="hallsensortester"

module_count=`script_fetch "hallsensor" "module_count"`
if [ $module_count -gt 0 ]; then
	for i in $(seq $module_count); do
		key_name="module"$i"_path"
		module_path=`script_fetch "hallsensor" "$key_name"`
		if [ -n "$module_path" ]; then
			LOGD "insmod $module_path"
			insmod "$module_path"
			if [ $? -ne 0 ]; then
				LOGE "insmod $module_path failed!"
			fi
		fi
	done
fi

sleep 3
device_count=`script_fetch "hallsensor" "device_count"`
if [ $device_count -gt 0 ]; then
	for i in $(seq $device_count); do
		key_name="device"$i"_name"
		device_name=`script_fetch "hallsensor" "$key_name"`
		for event in $(cd /sys/class/input; ls event*); do
			name=`cat /sys/class/input/$event/device/name`
			case $name in $device_name)
				hallsensortester $* "$device_name" &
				exit 0
			;;
			 *)
			;;
			esac
		done
	done
fi

SEND_CMD_PIPE_FAIL $3
