#!/bin/bash

set -e

localpath=$(cd $(dirname $0) && pwd)
lichee_top_path=${LICHEE_TOP_DIR:-"$(cd $localpath/../../../.. && pwd)"}
prebuiltpath=$localpath/prebuilt
overridepath=$localpath/override
configlist="$lichee_top_path/.buildconfig $localpath/android.conf $prebuiltpath/build.prop"

function do_prepare()
{
	source $(cd $localpath/.. && pwd)/common.sh

	prepare_tools

	local fw=$localpath/$1
	[ ! -f $localpath/$1 ] && fw=$(find $localpath -maxdepth 1 -name "*.img")
	if [ -n "$fw" ]; then
		if [ "$(echo $fw | wc -w )" -gt 1 ]; then
			echo "multi firmware file find, please check!"
			return 1
		else
			unpack_img $fw $prebuiltpath/unpack
			rm -rf $fw
		fi
	fi

	rm -rf $prebuiltpath/*.img

	local list=$(load_config BOARD_ESSITIAL_IMAGE "$configlist")
	for f in $list; do
		[ -f $overridepath/${f}.img ] && \
		cp $overridepath/${f}.img $prebuiltpath || \
		cp $prebuiltpath/unpack/${f}.img $prebuiltpath
	done

	e2cp $prebuiltpath/system.img:/system/build.prop $prebuiltpath
	cp $(load_config LICHEE_PLAT_OUT "$configlist")/sunxi.dtb $prebuiltpath/dtb.img

	return 0
}

function make_boot()
{
	# gki image
	[ -f $overridepath/boot.img ] && return 0

	unpack_boot $prebuiltpath/boot.img $prebuiltpath/vb
	local CMDLINE=$(grep "command line args: " $prebuiltpath/vb/unpack_boot.info | sed 's|^.*command line args:\s\+||g')

	local RAMDISK=$prebuiltpath/vb/ramdisk
	local MKBOOTIMG=mkbootimg
	local AVBTOOL=avbtool
	local KERNEL=$(load_config LICHEE_PLAT_OUT "$configlist")/bImage

	local OS_PATCH_LEVEL=$(load_config ro.build.version.security_patch "$configlist")
	local OS_VERSION=$(load_config ro.build.version.release "$configlist")

	local BOARD_BOOTIMAGE_PARTITION_SIZE=$(get_img_size $prebuiltpath/unpack/boot.img)
	local INTERNAL_BOOTIMAGE_ARGS="--kernel ${KERNEL} --ramdisk ${RAMDISK}"
	local INTERNAL_MKBOOTIMG_VERSION_ARGS="--os_version ${OS_VERSION} --os_patch_level ${OS_PATCH_LEVEL}"
	local BOARD_MKBOOTIMG_ARGS="--header_version 0x3"
	local INTERNAL_AVB_BOOT_SIGNING_ARGS="--prop com.android.build.boot.os_version:${OS_VERSION}"
	local BOARD_AVB_BOOT_ADD_HASH_FOOTER_ARGS=""

	rm -rf $prebuiltpath/boot.img
	${MKBOOTIMG} ${INTERNAL_BOOTIMAGE_ARGS} \
		--cmdline "${CMDLINE}" \
		${INTERNAL_MKBOOTIMG_VERSION_ARGS} \
		${BOARD_MKBOOTIMG_ARGS} \
		--output $prebuiltpath/boot.img

	${AVBTOOL} add_hash_footer \
		--image $prebuiltpath/boot.img \
		--partition_size ${BOARD_BOOTIMAGE_PARTITION_SIZE} \
		--partition_name boot ${INTERNAL_AVB_BOOT_SIGNING_ARGS} \
		${BOARD_AVB_BOOT_ADD_HASH_FOOTER_ARGS}
	rm -rf $prebuiltpath/vb
}

function make_vendor_boot()
{
	local kmodpath=$(load_config LICHEE_PLAT_OUT "$configlist")/lib/modules/*/
	[ -f $overridepath/vendor_boot.img ] && return 0

	unpack_boot $prebuiltpath/vendor_boot.img $prebuiltpath/vb
	local VENDOR_CMDLINE=$(grep "command line args: " $prebuiltpath/vb/unpack_boot.info | sed 's|^.*command line args:\s\+||g')

	local RAMDISK=$prebuiltpath/vb/vendor_ramdisk
	local staging=$prebuiltpath/vb/staging
	local filetype=$(file $RAMDISK | awk '{print $2}')
	local buildvariant=$(load_config ro.build.type "$configlist")

	make_ramdisk e $RAMDISK $staging $filetype

	for f in $(find $staging/lib/modules -name "*.ko"); do
		if [ -f $kmodpath/$(basename $f) ]; then
			cp $kmodpath/$(basename $f) $staging/lib/modules
		fi
	done

	# debugable for gsi
	if  [ -f $overridepath/system.img -a -f $overridepath/plat_sepolicy.cil -a "$buildvariant" == "user" ]; then
		cp $overridepath/plat_sepolicy.cil $staging/userdebug_plat_sepolicy.cil
		touch $staging/force_debuggable
		echo "ro.adb.secure=0"         > $staging/adb_debug.prop
		echo "ro.debuggable=1"        >> $staging/adb_debug.prop
		echo "ro.force.debuggable=1"  >> $staging/adb_debug.prop
		fakeroot chmod 644 $staging/userdebug_plat_sepolicy.cil
		fakeroot chmod 644 $staging/force_debuggable
		fakeroot chmod 644 $staging/adb_debug.prop
	fi

	make_ramdisk c $staging $RAMDISK $filetype

	local MKBOOTIMG=mkbootimg
	local AVBTOOL=avbtool
	local KERNEL=$(load_config LICHEE_PLAT_OUT "$configlist")/bImage

	local KERNEL_OFFSET=0x8000
	local RAMDISK_OFFSET=0x3100000
	local DTB_OFFSET=0x3000000
	local OS_PATCH_LEVEL=$(load_config ro.build.version.security_patch "$configlist")
	local OS_VERSION=$(load_config ro.build.version.release "$configlist")

	local arch=$(load_config LICHEE_ARCH $configlist)
	[ "x$arch" == "xarm64" ] && KERNEL_OFFSET=0x80000

	local BOARD_VENDOR_BOOTIMAGE_PARTITION_SIZE=$(get_img_size $prebuiltpath/unpack/vendor_boot.img)
	local INTERNAL_MKBOOTIMG_VERSION_ARGS="--os_version ${OS_VERSION} --os_patch_level ${OS_PATCH_LEVEL}"
	local BOARD_MKBOOTIMG_ARGS="--kernel_offset ${KERNEL_OFFSET} --ramdisk_offset ${RAMDISK_OFFSET} --dtb_offset ${DTB_OFFSET} --header_version 0x3"
	local INTERNAL_AVB_VENDOR_BOOT_SIGNING_ARGS="--prop com.android.build.boot.os_version:${OS_VERSION}"
	local BOARD_AVB_VENDOR_BOOT_ADD_HASH_FOOTER_ARGS=""
	local INTERNAL_VENDOR_BOOTIMAGE_ARGS="--dtb $prebuiltpath/dtb.img --base 0x40000000"
	local INTERNAL_VENDOR_RAMDISK_TARGET=$RAMDISK
	BOARD_MKBOOTIMG_ARGS+=" --board $arch"

	rm -rf $prebuiltpath/vendor_boot.img
	${MKBOOTIMG} \
		${INTERNAL_VENDOR_BOOTIMAGE_ARGS} \
		--vendor_cmdline "${VENDOR_CMDLINE}" \
		${INTERNAL_MKBOOTIMG_VERSION_ARGS} \
		${BOARD_MKBOOTIMG_ARGS} \
		--vendor_ramdisk ${INTERNAL_VENDOR_RAMDISK_TARGET} \
		--vendor_boot $prebuiltpath/vendor_boot.img

	${AVBTOOL} add_hash_footer \
		--image $prebuiltpath/vendor_boot.img \
		--partition_size ${BOARD_VENDOR_BOOTIMAGE_PARTITION_SIZE} \
		--partition_name vendor_boot ${INTERNAL_AVB_VENDOR_BOOT_SIGNING_ARGS} \
		${BOARD_AVB_VENDOR_BOOT_ADD_HASH_FOOTER_ARGS}
	rm -rf $prebuiltpath/vb
}

function do_pack()
{
	local platform_version=$(load_config ro.build.version.release "$configlist")
	local packpath=$lichee_top_path/build
	local addition=$(load_config BOARD_ADD_PACK_CONFIG "$configlist")
	for f in $addition; do
		if [ -f $prebuiltpath/unpack/$f ]; then
			addtion_files+=" $prebuiltpath/unpack/$f:$lichee_top_path/out/pack_out/$(basename $f)"
		elif [ -f $overridepath/$f ]; then
			addtion_files+=" $overridepath/$f:$lichee_top_path/out/pack_out/$(basename $f)"
		else
			return 1
		fi
	done

	(
	cd $packpath
	ANDROID_IMAGE_OUT=$prebuiltpath \
	./pack -a "$addtion_files" --platform_version ${platform_version} $@
	)

	return $?
}

function pack_hook()
{
	local functab=(
			do_prepare
			modify_system
			modify_vendor
			modify_vendor_dlkm
			modify_product
			make_super
			make_boot
			make_vendor_boot
			make_dtbo
			make_vbmeta_system
			make_vbmeta_vendor
			make_vbmeta
			do_pack)

	for func in ${functab[@]}; do
		$func $@
		if [ $? -eq 0 ]; then
			printf "\033[48;34m$(date "+%Y-%m-%d %H:%M:%S") - ${func}: run success\033[0m\n"
		else
			printf "\033[48;31m$(date "+%Y-%m-%d %H:%M:%S") - ${func}: run failed\033[0m\n"
			exit 1
		fi
	done

	return 0
}

pack_hook $@
