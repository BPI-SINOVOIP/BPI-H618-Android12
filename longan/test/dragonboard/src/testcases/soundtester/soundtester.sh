#!/bin/sh

source send_cmd_pipe.sh
source script_parser.sh
source log.sh

while true; do
	LOG_TAG="audiocodec test start....."
	tinymix 4 18
	audiocodectester SPK
	audiocodectester HP
	audiocodectester LINEIN1
	audiocodectester LINEIN2
	LOG_TAG="audiocodec test end....."

	LOG_TAG="daudio test start....."
	i2stester TX &

	i2stester RX
	i2stester CHECK
	LOG_TAG="daudio test end....."

	LOG_TAG="spdif test start....."
	spdiftester TX &

	spdiftester RX
	spdiftester CHECK
	LOG_TAG="spdif test end....."
	sleep 6
done

