#!/bin/sh

process=$(ps -ef | grep core | grep -v grep)
[ -n "$process" ] && exit 1

/bin/start_adb.sh &
source script_parser.sh

mkdir -p /system /vendor /vendor/etc 2>/dev/null

if [ ! -d /system/vendor/ ]; then
	ln -s /vendor /system/vendor
	ln -s /lib/modules/`uname -r`/ /vendor/modules
fi

if [ ! -d /vendor/etc/hawkview ]; then
	ln -s /dragonboard/bin/hawkview /vendor/etc/hawkview
fi

if [ ! -d /vendor/etc/firmware ]; then
	ln -s /dragonboard/firmware /vendor/etc/firmware
fi

if [ ! -d /lib/firmware ]; then
	ln -s /dragonboard/firmware /lib/firmware
fi

ROOT_DEVICE="/dev/mmcblk0p7"
for parm in $(cat /proc/cmdline); do
	case $parm in
		root=*)
			ROOT_DEVICE=`echo $parm | awk -F\= '{print $2}'`
			;;
	esac
done

# install nand driver if we boot from sdmmc
nand_activated=`script_fetch "nand" "activated"`
echo "nand activated #$nand_activated"
if [ $nand_activated -eq 1 ]; then
	case $ROOT_DEVICE in
		/dev/mmc*)

		nand_module_path=`script_fetch "nand" "module_path"`
		if [ -n "$nand_module_path" ]; then
			insmod "$nand_module_path"
	   fi
			;;
	esac

fi

# insmod touchscreen driver
tp_module_path=`script_fetch "tp" "module_path"`
if [ -n "$tp_module_path" ]; then
	insmod "$tp_module_path"

	#traverse event num so that match tp event num and export it
	a=-1
	device_name=`script_fetch "tp" "device_name"`
	echo "script_fetch device name is $device_name"
	sleep 1
	if [ -n "$device_name" ]; then								# match device name on a83 a64 platform
		for event in $(cd /sys/class/input; ls event*); do
			name=`cat /sys/class/input/$event/device/name`
			let a++
			case $name in
				$device_name)
					echo "Found tp device_name is $name, event num is $a"
					event_num=$a
					export TSLIB_TSDEVICE=/dev/input/event$event_num	# if match,export it for tp drvier
					;;
				*)
					;;
			esac
		done
	else
		# match event num on a33 platform
		event_num=`script_fetch "tp" "event_num"`
		export TSLIB_TSDEVICE=/dev/input/event$event_num
	fi

	# waiting for tp dev node
	found_time=0
	 while true; do
		if [ ! -c /dev/input/event$event_num ]; then
			let found_time++
			if [ $found_time -eq 6 ];then
				echo "Time out,can not find tp event num /dev/input/event$event_num"
				break
			fi
			sleep 1
			continue
		else
			echo "Found tp event num is /dev/input/event$event_num"
			# calibrate touchscreen if need
			tp_type=`script_fetch "tp" "type"`
			if [ $tp_type -eq 0 ]; then
				while true; do
					ts_calibrate
					if [ $? -eq 0 ]; then
						break
					fi
				done
			fi
			break
		fi
	done
else
	echo "NO!!! touchscreen driver to be insmod"
fi


# insmod ir driver
ir_activated=`script_fetch "ir" "activated"`
[ -z "$ir_activated" ] && ir_activated=0
if [ $ir_activated -eq 1 ]; then
	ir_module_path=`script_fetch "ir" "module_path"`
	if [ -n "$ir_module_path" ]; then
		insmod "$ir_module_path"
	fi
fi

# start camera test firstly
while true; do
	camera_activated=`script_fetch "camera" "activated"`
	echo "camera activated #$camera_activated"
	if [ $camera_activated -eq 1 ]; then
		echo "camera activated"
		module_count=`script_fetch "camera" "module_count"`
		if [ $module_count -gt 0 ]; then
			for i in $(seq $module_count); do
				key_name="module"$i"_path"
				module_path=`script_fetch "camera" "$key_name"`
				if [ -n "$module_path" ]; then
					insmod "$module_path"
					if [ $? -ne 0 ]; then
						echo "insmod $module_path failed"
					fi
				fi
			done
		fi
	else
		echo "camera not activated"
		break
	fi

	echo "camera module insmod done"
	touch /tmp/camera_insmod_done
        break
done

if [ ! -d /data/misc/dmt/ ]; then
	mkdir -p /data/misc/dmt/
fi

# check soc whether vs driver need to be loaded
vs_driver_activated=`script_fetch "vs_driver" "activated"`
if [ $vs_driver_activated -eq 1 ]; then
	module_aw_path=`script_fetch "vs_driver" "module_aw_path"`
	if [ -z "$module_aw_path" ]; then
		echo "failed to get module_aw_path"
		break
	fi
	module_vs_path=`script_fetch "vs_driver" "module_vs_path"`
	if [ -z "$module_vs_path" ]; then
		echo "failed to get module_vs_path"
		break
	fi
	module_loadmips_path=`script_fetch "vs_driver" "module_loadmips_path"`
	if [ -z "$module_loadmips_path" ]; then
		echo "failed to get module_loadmips_path"
		break
	fi
	module_mips_path=`script_fetch "vs_driver" "module_mips_path"`
	if [ -z "$module_mips_path" ]; then
		echo "failed to get module_mips_path"
		break
	fi
	#insmod vs & aw driver
	insmod $module_vs_path/vs_io_helper.ko
	insmod $module_aw_path/sunxi_tvtop.ko
	insmod $module_aw_path/decd.ko
	insmod $module_aw_path/tvutils.ko
	echo 1 > /sys/class/tvtop/tvtop/tvdisp
	echo 1 > /sys/class/tvtop/tvtop/tvfe
	echo 1 > /sys/class/tvtop/tvtop/tvcap
	#load mips
	$module_loadmips_path/loadmips $module_mips_path
	insmod $module_vs_path/ge2d_dev.ko
fi

# run dragonboard core process
core &
