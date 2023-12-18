#!/bin/sh

##############################################################################
# \version     1.0.0
# \date        2014年08月28日
# \author      hesimin <hesimin@allwinnertech.com>
# \Descriptions:
#			create the inital version
##############################################################################

source send_cmd_pipe.sh
source script_parser.sh

powerpath="/sys/class/power_supply"
if ls -l $powerpath| grep "battery" >/dev/null;then
	batterypath=`find $powerpath -name "*battery*"`
else
	SEND_CMD_PIPE_FAIL $3 "NOT PMU"
	exit 1
fi

present="/present"
status="/status"
checkpath=$batterypath$present
if cat $checkpath | grep 0; then
	SEND_CMD_PIPE_FAIL $3 "NOT PMU"
	exit 1
fi

checkpath=$batterypath$status
pmu_status=`cat $checkpath`
echo Please connect the charging device!
while [ ${pmu_status} != "Charging" ] ; do
	pmu_status=`cat $checkpath`
	sleep 1
done
SEND_CMD_PIPE_OK_EX $3 "battery is charging :${pmu_status}"

