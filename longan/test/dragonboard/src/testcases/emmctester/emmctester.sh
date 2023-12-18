#!/bin/sh

source send_cmd_pipe.sh
source script_parser.sh
source log.sh

LOG_TAG="emmctester"
flashdev=""
udisk_part=""
blkname=""
mountpoint=""
testfile="emmctest"
partcnt=0

test_part_path=""

## get emmc property ##
test_part=`script_fetch "emmc" "test_part"`
system_type=`script_fetch "emmc" "system_type"`
test_size=`script_fetch "emmc" "test_size"`

if [ -z "$test_size" -o $test_size -le 0 -o $test_size -gt $total_size ]; then
	test_size=64
fi





sssssssss
ROOT_DEVICE=/dev/nandd
for parm in $(cat /proc/cmdline); do
	case $parm in
		root=*)
			ROOT_DEVICE=`echo $parm | awk -F\= '{print $2}'`
			;;
	esac
done

BOOT_TYPE=-1
for parm in $(cat /proc/cmdline); do
	case $parm in
		boot_type=*)
			BOOT_TYPE=`echo $parm | awk -F\= '{print $2}'`
			;;
	esac
done

case $ROOT_DEVICE in
	/dev/nand*)
		LOGD "nand boot"
		mount /dev/nanda /boot
		blkname="mmcblk0"
		;;
	/dev/mmc*)
		case $BOOT_TYPE in
			2)
				LOGD "emmc boot,boot_type = $BOOT_TYPE"
				mount /dev/mmcblk0p2 /boot
				SEND_CMD_PIPE_OK_EX $3
				blkname="mmcblk0"
				;;
			*)
				LOGD "card boot,boot_type = $BOOT_TYPE"
				blkname="mmcblk1"
				;;
		esac
		;;
	*)
		LOGD "default boot type"
		mount /dev/nanda /boot
		;;
esac

### emmc as dragonborad test boot, don't to do simple test, and think it's ok
if [ $BOOT_TYPE -eq 2 ];then
	if [ -b "/dev/$blkname" ];then
		flashdev="/dev/$blkname"
	elif [ -b "/dev/block/$blkname" ];then
		flashdev="/dev/block/$blkname"
	else
		SEND_CMD_PIPE_FAIL_EX $3 "11"
		LOGD "$blkname no register, please check emmc driver is no insmod"
		exit 1
	fi

	###get the  emmc capacity########
	capacity=`cat /sys/block/$blkname/size|grep -o '\<[0-9]\{1,\}.[0-9]\{1,\}\>'|awk '{sum+=$0}END{printf "%.2f\n",sum/2097152}'`
	have_realnum=`echo $capacity | grep -E [1-9]`
	if [ -n "$have_realnum" ];then
		LOGD "[emmc] get emmc size:$capacity"
		LOGD "[emmc] test ok!!"
		SEND_CMD_PIPE_OK_EX $3 "$capacity"G""
		exit 0
	else
		LOGE "[emmc] get emmc capacity fail"
		SEND_CMD_PIPE_FAIL $3
		exit 1
	fi
fi

echo ========= system type: ${system_type} =======

######get flashdev and test part path| card boot, emmc register as mmcblk1########
if [ "$system_type" == "Android" -a -b "/dev/block/mmcblk1" ];then
	flashdev="/dev/block/mmcblk1"
	if [ "$test_part" == "null" ];then
		test_part_path=$flashdev
		LOGD "[emmc] warning: test parition is $test_part, use $test_part_path to test"
	else
		echo ************* 1 *************
		test_part_path="/dev/block/by-name/$test_part"
	fi

elif [ "$system_type" == "Linux" -o "$system_type" == "Tina" -a -b "/dev/mmcblk1" ];then
	flashdev="/dev/mmcblk1"


		echo ************* 2 *************

	if [ "$test_part" == "null" ];then
		echo ************* 3 *************
		test_part_path=$flashdev
		LOGD "[emmc] warning: test parition is $test_part, use $test_part_path to test"
	elif [ -z "$test_part" ];then
		LOGD "[emmc] to get test part path from name@$test_part fail, to test $flashdev"
		dd if=$flashdev of=/etxt bs=1 count=20240
		md5sum /etxt > /etxt.md5sum
		dd if=/etxt of=$flashdev bs=1 count=20240
		echo 3 > /proc/sys/vm/drop_caches
		dd if=$flashdev of=/etxt2 bs=1 count=20240
		md5sum /etxt2 > /etxt2.md5sum

		echo ************* 4 *************
		txtmd5=$(cat /etxt.md5sum | awk '{print $1}')
		txt2md5=$(cat /etxt2.md5sum | awk '{print $1}')
		if [ "$txtmd5" == "$txt2md5" ];then
			###get the  emmc capacity########
			capacity=`cat /sys/block/$blkname/size|grep -o '\<[0-9]\{1,\}.[0-9]\{1,\}\>'|awk '{sum+=$0}END{printf "%.2f\n",sum/2097152}'`
			have_realnum=`echo $capacity | grep -E [1-9]`
			if [ -n "$have_realnum" ];then
				LOGD "[emmc] get emmc size:${capacity}G"
				LOGD "[emmc] emmc simple test ok"
				SEND_CMD_PIPE_OK_EX $3 "$capacity"G""

				exit 0
			else
				LOGE "[emmc] get emmc capacity fail"
				SEND_CMD_PIPE_FAIL $3
				exit 1
			fi
		fi
		LOGE "emmc rw test failed"
		SEND_CMD_PIPE_FAIL_EX $3 "emmc rw test failed"
		exit 1

	else

		# parse the real part
		for parm in $(cat /proc/cmdline); do
			case $parm in
				partitions=*)
					by_name_part=$(echo $parm | awk -F '=' '{print $2}')
					;;
			esac
		done

		by_name_part=$(echo $by_name_part | sed "s/:/ /g")

		for parm in $by_name_part;do
			case $parm in
				$test_part@*)
					test_part_path="${flashdev}/$(echo $parm | awk -F '@' '{print $2}')"
					;;
			esac
		done
	fi

elif [ -b "/dev/mmcblk1" ];then
	flashdev="/dev/mmcblk1"

		echo ************* 5 *************

	if [ "$test_part" == "null" ];then
		test_part_path=$flashdev
		LOGD "[emmc] warning: test parition is $test_part, use $test_part_path to test"
	elif [ -z "$test_part" ];then
		LOGD "[emmc] to get test part path from name@$test_part fail, to test $flashdev"
		dd if=$flashdev of=/etxt bs=1 count=20240
		md5sum /etxt > /etxt.md5sum
		dd if=/etxt of=$flashdev bs=1 count=20240
		echo 3 > /proc/sys/vm/drop_caches
		dd if=$flashdev of=/etxt2 bs=1 count=20240
		md5sum /etxt2 > /etxt2.md5sum

		txtmd5=$(cat /etxt.md5sum | awk '{print $1}')
		txt2md5=$(cat /etxt2.md5sum | awk '{print $1}')
		if [ "$txtmd5" == "$txt2md5" ];then
			###get the  emmc capacity########
			capacity=`cat /sys/block/$blkname/size|grep -o '\<[0-9]\{1,\}.[0-9]\{1,\}\>'|awk '{sum+=$0}END{printf "%.2f\n",sum/2097152}'`
			have_realnum=`echo $capacity | grep -E [1-9]`
			if [ -n "$have_realnum" ];then
				LOGD "[emmc] get emmc size:${capacity}G"
				LOGD "[emmc] emmc simple test ok"
				SEND_CMD_PIPE_OK_EX $3 "$capacity"G""
				exit 0
			else
				LOGE "[emmc] get emmc capacity fail"
				SEND_CMD_PIPE_FAIL $3
				exit 1
			fi
		fi
		LOGE "emmc rw test failed"
		SEND_CMD_PIPE_FAIL_EX $3 "emmc rw test failed"
		exit 1

	else

		# parse the real part
		for parm in $(cat /proc/cmdline); do
			case $parm in
				partitions=*)
					by_name_part=$(echo $parm | awk -F '=' '{print $2}')
					;;
			esac
		done

		by_name_part=$(echo $by_name_part | sed "s/:/ /g")

		for parm in $by_name_part;do
			case $parm in
				$test_part@*)
					test_part_path="${flashdev}/$(echo $parm | awk -F '@' '{print $2}')"
					;;
			esac
		done
	fi

else
	#/dev/mmcblk1 is not exist,maybe emmc driver hasn't been insmod
	SEND_CMD_PIPE_FAIL_EX $3 "11"
	LOGD "[emmc] check test_config property system_type fail or flashdev hasn't been insmod"

	exit 1
fi

#################### mount test part##################################
mountpoint="/data/dragonborad/$test_part"

mkdir -p $mountpoint

mount $test_part_path $mountpoint
if [ $? -ne 0 ];then

	mkfs.ext4 $test_part_path
	if [ $? -ne 0 ];then
		SEND_CMD_PIPE_FAIL_EX $3 "22"
		LOGD "[emmc] format to ext4 fail"
		exit 1
	fi
	mount -t ext4 $test_part_path $mountpoint

	if [ $? -eq 0 ];then

		LOGD "[emmc] test_part=$test_part in $test_part_path"
		LOGD "[emmc] test_size=$test_size"
		LOGD "[emmc] simple test read and write"

		###get the  emmc capacity########
		capacity=`cat /sys/block/$blkname/size|grep -o '\<[0-9]\{1,\}.[0-9]\{1,\}\>'|awk '{sum+=$0}END{printf "%.2f\n",sum/2097152}'`
		have_realnum=`echo $capacity | grep -E [1-9]`
		if [ -n "$have_realnum" ];then
			LOGD "[emmc] get emmc size:$capacity"
			####simple test read and write#########
			emmcrw "$mountpoint/$testfile" "$test_size"
			if [ $? -ne 0 ]; then
				SEND_CMD_PIPE_FAIL_EX $3 "33"
				LOGE "[emmc] test fail!!"
			else
				SEND_CMD_PIPE_OK_EX $3 "$capacity"G""
				LOGD "[emmc] test ok!!"
			fi
		else
			LOGE "[emmc] get emmc capacity fail"
			SEND_CMD_PIPE_FAIL $3
			exit 1
		fi

		umount $mountpoint
	else
		LOGD "[emmc] mount $test_part_path in $mountpoint fail"
		SEND_CMD_PIPE_FAIL_EX $3 "33"
		LOGE "[emmc] test fail!!"
		exit 1
	fi

fi


