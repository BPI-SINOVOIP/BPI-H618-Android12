#!/bin/sh

source send_cmd_pipe.sh
source script_parser.sh
source log.sh

LOG_TAG="dtv-dtmb-test"
test_freq=`script_fetch "dtv_dtmb" "test_freq"`

LOGD "Start dtv_dtmb test"
while true; do
	atv_is_run=`ps|grep TestTuner |grep -v grep |wc -l`

	if [ $atv_is_run -eq 1 ]; then
		LOGD "atv test is running, do not run dtmb"
		sleep 5
	else
		ut_demod pgs DTMB "$test_freq" >/dev/null 2>&1
		if [ $? -eq 0 ]; then
			LOGD "DTMB demod ok"
			SEND_CMD_PIPE_OK_EX $3 freq="$test_freq"MHz""
			break
		fi
		sleep 7
	fi
done