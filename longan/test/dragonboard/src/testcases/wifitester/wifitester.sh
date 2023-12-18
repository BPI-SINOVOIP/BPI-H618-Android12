#!/bin/sh
##############################################################################
# \version	 1.0.0
# \date		2012年05月31日
# \author	  James Deng <csjamesdeng@allwinnertech.com>
# \Descriptions:
#			create the inital version

# \version	 1.1.0
# \date		2012年09月26日
# \author	  Martin <zhengjiewen@allwinnertech.com>
# \Descriptions:
#			add some new features:
#			1.wifi hotpoint ssid and single strongth san
#			2.sort the hotpoint by single strongth quickly
##############################################################################
source send_cmd_pipe.sh
source script_parser.sh
source log.sh

LOG_TAG="wifitester"

WIFI_PIPE=/tmp/wifi_pipe
wlan_try=0

module_count=0

for f in $(cd /vendor/etc/firmware && find -name "wifi_*.ini"); do
	ln -sf /vendor/etc/firmware/$(basename $f) /vendor/etc/$(basename $f)
done

modpath="/vendor/modules"

insmod $modpath/sunxi_rfkill.ko 2>/dev/null

wifi_vnd=`script_fetch "wifi" "wifi_vnd"`
wifi_signal_low_threshold=`script_fetch "wifi" "signal_low_threshold"`

[ -z "$wifi_signal_low_threshold" ] && wifi_signal_low_threshold=-60

if [ "$wifi_vnd" == "common" ]; then
	vendor="$(wl_info -v | awk -F= '/vendor=/{print $NF}' | sed $'s,\x1b\\[[0-9;]*[a-zA-Z],,g')"
	driver="$(wl_info -d | awk -F= '/driver=/{print $NF}' | sed $'s,\x1b\\[[0-9;]*[a-zA-Z],,g')"
	drvarg="$(wl_info -g | awk -F= '/drvarg=/{print $NF}' | sed $'s,\x1b\\[[0-9;]*[a-zA-Z],,g')"
	case "$vendor" in
		sprd)
			insmod $modpath/uwe5622_bsp_sdio.ko
			;;
		aic)
			insmod $modpath/aic8800_bsp.ko
			;;
	esac

	if [ -n "$driver" ]; then
		insmod $modpath/${driver}.ko $drvarg
	fi
else
	while true; do
		module_path=$(script_fetch "wifi" "module"${idx}"_path")
		module_args=$(script_fetch "wifi" "module"${idx}"_args")
		if [ -n "$module_path" ]; then
			module_count=$((module_count+1))
			insmod $module_path $module_args 2>/dev/null
			[ $? -ne 17 ] && \
			[ $? -ne 0 ] && SEND_CMD_PIPE_FAIL $3 && exit 1
		fi
		if [ -z "$idx" ]; then
			idx=0
		else
			idx=$((idx+1))
			[ $idx -ge 20 ] && break
		fi
	done
	[ "$module_count" -eq 0 ] && SEND_CMD_PIPE_FAIL $3 && exit 1
fi

flag="######"

sleep 3

success=0
if ifconfig -a | grep wlan0; then
	for i in `seq 3`; do
		ifconfig wlan0 up > /dev/null
		[ $? -eq 0 ] && success=1 && break
		LOGW "ifconfig wlan0 up failed, try again 1s later"
		sleep 1
	done
fi

if [ $success -ne 1 ]; then
	LOGE "ifconfig wlan0 up failed, no more try"
	SEND_CMD_PIPE_FAIL $3
	exit 1
fi

ap_ssid=$(script_fetch "wifi" "test_ap_ssid")
ap_pass=$(script_fetch "wifi" "test_ap_pass")

state=0
macaddr=$(ifconfig wlan0 | awk '/HWaddr/{print $NF}')

while true ; do
	wifi="$(iw wlan0 scan | busybox sed -n "/\(SSID: \|signal: \)/p"  | awk '{print $2}' | sed 's|^$|unknow|g' | xargs -n 2 | awk '{print $2":"$1}' | sort -n -r -k 2 -t : | uniq)"
	for item in $wifi; do
		name=${item/:*/}
		level=${item/*:/}
		level=${level/.*/}
		if [ ${#name} -gt 11 ]; then
			name="${name:0:11}..."
		fi
		echo "$name:$level" >> $WIFI_PIPE
	done

	echo $flag >> $WIFI_PIPE

	highlevel=$(echo "$wifi" | head -n 1 | awk -F: '{print $2}' | sed 's|\..*$||g')
	if [ $highlevel -lt $wifi_signal_low_threshold ]; then
		LOGE "Signal to low, ant error?($highlevel vs. $wifi_signal_low_threshold)"
		SEND_CMD_PIPE_FAIL $3
		SEND_CMD_PIPE_FAIL_EX $3 "[$macaddr] Signal to low, ant error?"
		exit 1
	fi

	if [ $state -eq 0 ]; then
		if [ -n "$ap_ssid" ]; then
			LOGD "Test AP SSID: $ap_ssid"
			LOGD "Test AP PASS: $ap_pass"
			pidlist=$(ps -ef | grep wpa_supplicant | grep -v grep | awk '{print $1}')
			for pid in $pidlist; do
				kill -9 $pid
			done
			if [ -n "$ap_pass" ]; then
				wpa_passphrase "$ap_ssid" "$ap_pass" > /etc/wpa_supplicant.conf
				LOGD "Connecting to ${ap_ssid} with password ${ap_pass}..."
			else
				echo -e "network={\n\tssid=\"$ap_ssid\"\n\tkey_mgmt=NONE\n\t}" > /etc/wpa_supplicant.conf
				LOGD "Connecting to ${ap_ssid} with no password..."
			fi
			wpa_supplicant -iwlan0 -Dnl80211 -c /etc/wpa_supplicant.conf -B
			udhcpc -i wlan0
			hwaddr=$(ifconfig wlan0 | grep -o HWaddr.* | awk '{print $2}')
			ipaddr=$(ifconfig wlan0 | grep -o "inet addr:\s*[0-9.]*" | grep -o "[0-9.]\{7,\}")
			mask=$(ifconfig wlan0 | grep -o "Mask:\s*[0-9.]*" | grep -o "[0-9.]\{7,\}")
			dns=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}')
			gateway=$(ip route | grep "default" | grep -o "[0-9.]\{7,\}")
			speed=$(iw dev wlan0 link | grep "bitrate" | awk '{print $3" "$4}')
			LOGD "Connected to $ap_ssid"
			LOGD "MAC     [ $hwaddr ]"
			LOGD "IPADDR  [ $ipaddr ]"
			LOGD "GATEWAY [ $gateway ]"
			LOGD "MASK    [ $mask ]"
			LOGD "DNS     [ $dns ]"
			LOGD "SPEED   [ $speed ]"
			SEND_CMD_PIPE_OK_EX $3 "[$macaddr] Connected to $ap_ssid"
		else
			LOGD "No Test AP SSID configured"
			SEND_CMD_PIPE_OK_EX $3 "[$macaddr] No Test AP SSID configured"
		fi
		state=1
	fi

	sleep 5
done

LOGE "wlan0 not found, no more try"
SEND_CMD_PIPE_FAIL $3
exit 1
