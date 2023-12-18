#!/bin/sh
###############################################################################
# \version     1.0.0
# \date        Sep 26, 2012
# \author      Martin <zhengjiewen@allwinnertech.com>
# \Descriptions:
#			create the inital version
###############################################################################
# \version     1.0.1
# \date        May 17, 2022
# \author      kanghoupeng <kanghoupeng@allwinnertech.com>
# \Descriptions:
#			support detect by USB0 bus num.
###############################################################################

source send_cmd_pipe.sh
source log.sh

LOG_TAG="otgtester"

ehci_busnum=000
ohci_busnum=000
while true; do
	ehci0_controller_path=`find /sys/devices/platform/ -name "*.ehci0-controller" | grep -E "[0-9a-z]/[0-9a-z]*.ehci0-controller"`
	ohci0_controller_path=`find /sys/devices/platform/ -name "*.ohci0-controller" | grep -E "[0-9a-z]/[0-9a-z]*.ohci0-controller"`
	ehci_busnum_path=`find $ehci0_controller_path/ -name "busnum" | grep -E "usb[0-9]/busnum"`
	ohci_busnum_path=`find $ohci0_controller_path/ -name "busnum" | grep -E "usb[0-9]/busnum"`
	if [ -n "$ehci_busnum_path" ]; then
		ehci_busnum=`cat $ehci_busnum_path`
		ehci_busnum=`printf "%03d" ${ehci_busnum}`
	fi
	if [ -n "$ohci_busnum_path" ]; then
		ohci_busnum=`cat $ohci_busnum_path`
		ohci_busnum=`printf "%03d" ${ohci_busnum}`
	fi
	pid_vid=`lsusb | awk '(NF&&$2~/('$ehci_busnum'|'$ohci_busnum')/&&$6!~/^1d6b/) {print $6}'`
	pid=`echo $pid_vid | awk -F: '{print $1}'`
	vid=`echo $pid_vid | awk -F: '{print $2}'`

	if [ -n "$pid" ] && [ -n "$vid" ]; then
		LOGD "pid=$pid;vid=$vid"
		SEND_CMD_PIPE_OK_EX $3
	fi
	sleep 1
done

