#!/bin/sh

source send_cmd_pipe.sh
source script_parser.sh
source log.sh

module_aw_path=`script_fetch "vs_driver" "module_aw_path"`
if [ -z "$module_aw_path" ]; then
	echo "failed to get module_aw_path"
	break
fi

module_vs_path=`script_fetch "vs_driver" "module_vs_path"`
if [ -z "$module_vs_path" ]; then
	LOGE "failed to get module_vs_path"
	break
fi

if [ -c /dev/dtmbip ];then
	LOGD "TVFE driver is insmod now"
else
	LOGE "TVFE driver need to insmod"
	insmod $module_aw_path/sunxi_dtmbip.ko
	insmod $module_vs_path/hidtvreg_dev.ko
	insmod $module_vs_path/demux_dev.ko
	insmod $module_vs_path/cpu_comm_dev.ko
fi

LOG_TAG="atvtest"
test_freq=`script_fetch "atv" "test_freq"`

LOGD "Start atv tuner test"
while true; do
	dtv_is_run=`ps|grep ut_demod |grep -v grep |wc -l`

	if [ $dtv_is_run -eq 1 ]; then
		LOGD "dtv test is running, do not run atv"
		sleep 3
	else
		TestTuner DVBC ATV "$test_freq" >/dev/null 2>&1
		if [ $? -eq 0 ]; then
			LOGD "atv ok"
			SEND_CMD_PIPE_OK_EX $3 freq="$test_freq"KHz""
			break
		fi
		sleep 5
	fi
done
