#!/bin/sh

source send_cmd_pipe.sh
source script_parser.sh
source log.sh

LOG_TAG="nandtester"

flashdev=""
mountpoint=""
testfile="nandtest"
nand_capacity=0

test_part=""
system_type=""
test_size=0

## get nand property ##
test_part=`script_fetch "nand" "test_part"`
system_type=`script_fetch "nand" "system_type"`
test_size=`script_fetch "nand" "test_size"`

check_dev_exist()
{
	ret=0

	if [ -b $1 ];then
		ret=1
	else
		ret=0
	fi

	return $ret
}
## get flashdev ###
get_flashdev()
{
	if [ "$system_type" == "Android" ];then
		flashdev="/dev/block/nand0"

	elif [ "$system_type" == "Linux" -o "$system_type == Tina" ];then
		flashdev="/dev/nand0"

	elif [ -b "/dev/nand0" ];then
		flashdev="/dev/nand0"

	fi
}

## get test part path(test_part_path) from test_part(e.g. rootfs to get /dev/nand0p4)
#@return 0: success
#@return 1: fail
get_flash_part_path_by_name()
{
	ret=1
	if [ "$system_type" == "Android" -a -b "/dev/block/nand0" ];then
		if [ "$test_part" == "null" ];then
			test_part_path="/dev/block/nand0"
			LOGD "warning: test parition is $test_part, use $test_part_path to test"

		else
			test_part_path="/dev/block/by-name/$test_part"
		fi

		ret=0
	elif [ "$system_type" == "Linux" -o "$system_type" == "Tina" -a -b "/dev/nand0" ];then

		if [ "$test_part" == "null" ];then
			test_part_path="/dev/nand0"
			LOGD "warning: test parition is $test_part, use $test_part_path to test"
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

		ret=0
	elif [ -b "/dev/nand0" ];then

		if [ "$test_part" == "null" ];then
			test_part_path="/dev/nand0"
			LOGD "warning: test parition is $test_part, use $test_part_path to test"
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

		ret=0
	else
		ret=1
	fi

	return $ret
}

## get BOOT_TYPE ##
get_boot_type()
{
	#get boot start flash type:0: nand, 1:card0, 2:emmc/tsd
	BOOT_TYPE=-1
	ROOTFS_TYPE=""
	for parm in $(cat /proc/cmdline) ; do
		case $parm in
			boot_type=*)
				BOOT_TYPE=`echo $parm | awk -F\= '{print $2}'`
				;;
			rootfstype=*)
				ROOTFS_TYPE=`echo $parm | awk -F\= '{print $2}'`
				;;
		esac
	done
}

## get nand capacity ##
get_nand_capacity()
{
	###get the  nand capacity########
	nand_capacity=$(cat /sys/block/nand0/size|grep -o '\<[0-9]\{1,\}.[0-9]\{1,\}\>'|awk '{sum+=$0}END{printf "%.2f\n",sum/2097152}')
	have_realnum=$(echo $nand_capacity | grep -E [1-9])
	if [ -z "$have_realnum" ];then
		nand_capacity=0
	fi
	echo "nand cacpacity : $nand_capacity"
}

#main begin
## get BOOT_TYPE value
get_boot_type

## get flashdev value
get_flashdev

### nand as dragonborad test boot, don't to do simple test, and think it's ok
### and to display capacity
if [ $ROOTFS_TYPE = "ubifs,rw" ];then
	LOGD "ubi nand boot"

	SIZE=0
	nand_capacity=0
	for parm in $(ls /sys/block) ; do
		SIZE=$(cat /sys/block/$parm/size)
		if [ $(cat /sys/block/$parm/device/type) = "nand" ];then
			nand_capacity=`expr ${nand_capacity} + ${SIZE}`
		fi
	done

	nand_capacity=`expr ${nand_capacity} / 2 / 1024`
	if [ "$nand_capacity" != 0 ];then
		SEND_CMD_PIPE_OK_EX $3 "$nand_capacity"M""
		LOGD "[ubi-nand] get ubi nand size:$nand_capacity"
		LOGD "[ubi-nand] test ok!!"
		exit 0
	else
		LOGE "[ubi-nand] get ubi nand capacity fail"
		SEND_CMD_PIPE_FAIL $3
		exit 1
	fi

elif [ ${BOOT_TYPE} -eq 0 ];then
	LOGD "nand boot"

	check_dev_exist $flashdev
	if [ $? -ne 1 ];then
		SEND_CMD_PIPE_FAIL_EX $3 "11"
		LOGD "$flashdev no exist, please check nand driver is no insmod ?"
		exit 1
	fi

	###get the  nand capacity(nand_capacity value)########
	get_nand_capacity
	if [ "$nand_capacity" != 0 ];then
		SEND_CMD_PIPE_OK_EX $3 "$nand_capacity"G""
		LOGD "[nand] get nand size:$nand_capacity"
		LOGD "[nand] test ok!!"
		exit 0
	else
		LOGE "[nand] get nand capacity fail"
		SEND_CMD_PIPE_FAIL $3
		exit 1
	fi


elif [ ${BOOT_TYPE} -eq 1 -o  ${BOOT_TYPE} -eq 2 ];then #card/emmc boot
	LOGD "card/emmc boot,boot_type = $BOOT_TYPE"
	###judge nand is exist ? #########
	check_dev_exist $flashdev
	if [ $? -ne 1 ];then
		SEND_CMD_PIPE_FAIL_EX $3 "11"
		LOGD "$flashdev no exist, please check nand driver is no insmod ?"
		exit 1
	fi

	###get the  nand capacity(nand_capacity value)########
	get_nand_capacity
	if [ "$nand_capacity" !=  0 ];then
		SEND_CMD_PIPE_OK_EX $3 "$nand_capacity"G""
	else
		LOGE "[nand] get nand capacity fail"
		SEND_CMD_PIPE_FAIL $3
		exit 1
	fi

	get_flash_part_path_by_name
	if [ $? -ne 1 ];then
		LOGD "[nand] to get test part path from name@$test_part fail, to test $flashdev"
		dd if=$flashdev of=/txt bs=1 count=20240
		md5sum /txt > /txt.md5sum
		dd if=/txt of=$flashdev bs=1 count=20240
		echo 3 > /proc/sys/vm/drop_caches
		dd if=$flashdev of=/txt2 bs=1 count=20240
		md5sum /txt2 > /txt2.md5sum

		txtmd5=$(cat /txt.md5sum | awk '{print $1}')
		txt2md5=$(cat /txt2.md5sum | awk '{print $1}')
		if [ "$txtmd5" == "$txt2md5" ];then
			LOGD "[nand] get nand size:${nand_capacity}G"
			LOGD "nand simple test ok"
			SEND_CMD_PIPE_OK_EX $3
			exit 0
		fi
			LOGE "nand rw test failed"
			SEND_CMD_PIPE_FAIL_EX $3 "nand rw test failed"
			exit 1
	fi


	####mount test part ######################
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

		## mount success to write and read in filesystem way
		## mount fail to write and read in ioctl way
		if [ $? -eq 0 ];then
			nandrw "$mountpoint/$testfile" "8"

			if [ $? -ne 0 ]; then
				LOGE "nand rw test failed"
				SEND_CMD_PIPE_FAIL_EX $3 "nand rw test failed"
				exit 1
			else
				LOGD "[nand] get nand size:$nand_capacity"
				LOGD "[nand] simple test ok"
				SEND_CMD_PIPE_OK_EX $3
				exit 0
			fi
		else
			nandrw "$flashdev"
			if [ $? -ne 0 ]; then
				LOGE "nand rw test failed"
				SEND_CMD_PIPE_FAIL_EX $3 "nand rw test failed"
				exit 1
			else
				LOGD "[nand] get nand size:$nand_capacity"
				LOGD "nand simple test ok"
				SEND_CMD_PIPE_OK_EX $3
				exit 0
			fi

		fi

	fi

else
	LOGE "no define boot_type"
	SEND_CMD[M _<_PIPE_FAIL $3
	exit 1
fi

