#!/bin/sh

##############################################################################
# \version     1.0.0
# \date        2014年08月28日
# \author      xinouyang <xinouyang@allwinnertech.com>
# \Descriptions:
#			create the inital version
##############################################################################

source send_cmd_pipe.sh
source script_parser.sh
source log.sh

LOG_TAG="pwkeytester"

col_num=$(cat /proc/bus/input/devices | grep -n "axp" \
		| sed '/i2c/d' | awk -F ":" '{print $1}')

hang_num=`expr $col_num + 4`
event=$(cat /proc/bus/input/devices | sed -n "${hang_num}p" | awk '{print $NF}')

echo "pwkeytester $event"
if [ $event ]
then
	pwkeytester "/dev/input/$event"

	ret=$?
	if [ $ret -eq 0 ];then
		SEND_CMD_PIPE_OK_EX $3 "powerkey pass"
	fi

	if [ $ret -ne 0 ];then
		SEND_CMD_PIPE_FAIL $3 "NOT PMU"
	fi
else
	SEND_CMD_PIPE_FAIL $3 "NO EVENT"
fi

