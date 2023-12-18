#!/bin/sh
##############################################################################
# \version     1.0.0
# \date        2019.10.8
# \author
# \Descriptions:
#			test tv
##############################################################################
source send_cmd_pipe.sh
source script_parser.sh
source log.sh

LOG_TAG="tvtester"

for i in /sys/class/extcon/extcon*
do
	if [ -d $i ]
	then
		if cat $i/name | grep "cvbs_out"; then
			hpd_state_path=$i/state
			echo "hpd_state_path:$hpd_state_path"
		fi
	fi
done

while true; do
	if [ -e $hpd_state_path ]; then
		echo "extcon cvbs_out exist"
		hpd_state=$hpd_state_path
	elif [ -e "/sys/class/extcon/extcon1/state" ]; then
		hpd_state=/sys/class/extcon/extcon1/state
	else
		hpd_state=/sys/class/extcon/cvbs/state
	fi
	if cat $hpd_state | grep "1"; then
		echo "CVBS plug in"
		SEND_CMD_PIPE_OK_EX $3
		break
	fi
	sleep 2
done

if [ ! -d /sys/kernel/debug/dispdbg ]; then
	mount -t debugfs nodev /sys/kernel/debug/
fi

cd /sys/kernel/debug/dispdbg/
echo switch > command
echo disp1 > name
echo 2 11 > param
echo 1 > start

echo 1 > /sys/class/disp/disp/attr/disp
echo 8 > /sys/class/disp/disp/attr/colorbar
