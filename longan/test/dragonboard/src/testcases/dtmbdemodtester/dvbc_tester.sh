#!/bin/sh

source send_cmd_pipe.sh
source script_parser.sh
source log.sh

LOG_TAG="dtv-dvbc-test"
test_freq=`script_fetch "dtv_dvbc" "test_freq"`

LOGD "Start dtv-dvbc test"
while true; do
	atv_is_run=`ps|grep TestTuner |grep -v grep |wc -l`

	if [ $atv_is_run -eq 1 ]; then
		LOGD "atv test is running, do not run dvbc"
		sleep 5
	else
		ut_demod pgs DVBC "$test_freq" >/dev/null 2>&1
		if [ $? -eq 0 ]; then
			LOGD "DVBC demod ok"
			SEND_CMD_PIPE_OK_EX $3 freq="$test_freq"MHz""
			break
		fi
		sleep 7
	fi
done