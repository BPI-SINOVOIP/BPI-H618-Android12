#!/bin/bash

# longan envsetup.sh help
function build_help()
{
    echo "==========================================================================================="
    echo "longan envsetup help:"
    local info=""
    local i fun msg
    local sbfile="$LICHEE_TOP_DIR/build/envsetup.sh"
    local function_pattern="^function \([a-zA-Z][a-zA-Z0-9_-]*\).*"
    for i in $(sed -n "/${function_pattern}/=" ${sbfile}); do
        fun=$(sed -n "${i}s/${function_pattern}/\1/p" ${sbfile})
        msg=$(sed -n "$((i-1))s/^\s*#\s*\(.*\)$/\1/p" ${sbfile})
        info+="`printf "  %-20s -- %s" "$fun" "$msg"`\n"
    done
    printf "$info" | sort
    echo "==========================================================================================="
}

# show build useful command
function build_usage()
{
    printf "Usage: build [args]
    build               - default build all
    build bootloader    - only build bootloader
    build buildroot     - only build buildroot
    build kernel        - only build kernel
    build recovery      - only build kernel for recovery
    build clean         - clean all
    build distclean     - distclean all
"
    return 0
}

# show pack usage
function pack_usage()
{
    printf "Usage: pack [args]
    pack                - pack firmware
    pack -d             - pack firmware with debug info output to card0
    pack -s             - pack firmware with secureboot
    pack -sd            - pack firmware with secureboot and debug info output to card0
"
    return 0
}

function _load_config()
{
	local cfgkey=$1
	local cfgfile=$2
	local defval=$3
	local val=""

	[ -f "$cfgfile" ] && val="$(sed -n "/^\s*export\s\+$cfgkey\s*=/h;\${x;p}" $cfgfile | sed -e 's/^[^=]\+=//g' -e 's/^\s\+//g' -e 's/\s\+$//g')"
	eval echo "${val:-"$defval"}"
}

# cd <LICHEE_TOP_DIR>
function croot()
{
	cd $LICHEE_TOP_DIR
}

# same as croot
function cl()
{
	croot
}

# cd current kernel dir
function ckernel()
{
	local dkey="LICHEE_KERN_DIR"
	local dval=$(_load_config $dkey $LICHEE_TOP_DIR/.buildconfig)
	[ -z "$dval" ] && echo "ERROR: $dkey not set in .buildconfig" && return 1
	cd $dval
}

# same as ck
function ck()
{
	ckernel
}

# save kernel .config to xxx_defconfig
function saveconfig()
{
	$LICHEE_TOP_DIR/build.sh saveconfig $@
}

# run kernel menuconfig
function menuconfig()
{
	$LICHEE_TOP_DIR/build.sh menuconfig $@
}

# load kernel xxx_defconfig to .config
function loadconfig()
{
	$LICHEE_TOP_DIR/build.sh loadconfig $@
}

# cd current dts dir
function cdts()
{
	local dkey1="LICHEE_KERN_DIR"
	local dkey2="LICHEE_ARCH"
	local dkey3="LICHEE_CHIP_CONFIG_DIR"

	local dval1=$(_load_config $dkey1 $LICHEE_TOP_DIR/.buildconfig)
	local dval2=$(_load_config $dkey2 $LICHEE_TOP_DIR/.buildconfig)
	local dval3=$(_load_config $dkey3 $LICHEE_TOP_DIR/.buildconfig)

	[ -z "$dval1" ] && echo "ERROR: $dkey1 not set in .buildconfig" && return 1
	[ -z "$dval2" ] && echo "ERROR: $dkey2 not set in .buildconfig" && return 1

	local dval=$dval1/arch/$dval2/boot/dts
	[ "$dval2" == "arm64" ] && dval=$dval/sunxi
	[ "$dval1" == "$LICHEE_TOP_DIR/kernel/linux-5.10" ] && dval=$dval3/configs/default/linux-5.10
	cd $dval
}

# cd current product dir
function cdevice()
{
	local dkey="LICHEE_PRODUCT_CONFIG_DIR"
	local dval=$(_load_config $dkey $LICHEE_TOP_DIR/.buildconfig)
	[ -z "$dval" ] && echo "ERROR: $dkey not set in .buildconfig" && return 1
	cd $dval
}

# cd current board config dir
function cconfigs()
{
	local dkey="LICHEE_BOARD_CONFIG_DIR"
	local dval=$(_load_config $dkey $LICHEE_TOP_DIR/.buildconfig)
	[ -z "$dval" ] && echo "ERROR: $dkey not set in .buildconfig" && return 1
	cd $dval
}

# cd current board bootloader bin dir
function cbin()
{
	local dkey="LICHEE_BRANDY_OUT_DIR"
	local dval=$(_load_config $dkey $LICHEE_TOP_DIR/.buildconfig)
	[ -z "$dval" ] && echo "ERROR: $dkey not set in .buildconfig" && return 1
	cd $dval
}

# same as cconfigs
function cbd()
{
	cconfigs
}

# cd current product out dir
function cout()
{
	local dkey="LICHEE_PLAT_OUT"
	local dval=$(_load_config $dkey $LICHEE_TOP_DIR/.buildconfig)
	[ -z "$dval" ] && echo "ERROR: $dkey not set in .buildconfig" && return 1
	cd $dval
}

# cd brandy dir
function cbrandy()
{
	local dkey="LICHEE_BRANDY_DIR"
	local dval=$(_load_config $dkey $LICHEE_TOP_DIR/.buildconfig)
	[ -z "$dval" ] && echo "ERROR: $dkey not set in .buildconfig" && return 1
	cd $dval
}

# cd uboot dir
function cboot()
{
	local dkey="LICHEE_BRANDY_DIR"
	local dval=$(_load_config $dkey $LICHEE_TOP_DIR/.buildconfig)
	[ -z "$dval" ] && echo "ERROR: $dkey not set in .buildconfig" && return 1
	cd $dval/u-boot-2018
}

# cd spl dir
function cboot0()
{
	local dkey="LICHEE_BRANDY_DIR"
	local dval=$(_load_config $dkey $LICHEE_TOP_DIR/.buildconfig)
	[ -z "$dval" ] && echo "ERROR: $dkey not set in .buildconfig" && return 1
	cd $dval/spl
}

# cd buildroot dir
function cbr()
{
	local dkey="LICHEE_BR_DIR"
	local dval=$(_load_config $dkey $LICHEE_TOP_DIR/.buildconfig)
	[ -z "$dval" ] && echo "ERROR: $dkey not set in .buildconfig" && return 1
	cd $dval
}

# cd bsp dir
function cbsp()
{
	cd $LICHEE_BSP_DIR
}

# print current .buildconfig
function printconfig()
{
	cat $LICHEE_TOP_DIR/.buildconfig
}

# run ./build.sh pack command
function pack()
{
	local mode=$@
	if [ $# -gt 1 ]; then
		echo "too much args"
		return 1
	fi

	if [ "x$mode" == "x" ]; then
		./build.sh pack
		[ $? -ne 0 ] && return 1
	elif [ "x$mode" == "x-d" ]; then
		./build.sh pack_debug
		[ $? -ne 0 ] && return 1
	elif [ "x$mode" == "x-s" ]; then
		./build.sh pack_secure
		[ $? -ne 0 ] && return 1
	elif [ "x$mode" == "x-sd" ]; then
		./build.sh pack_debug_secure
		[ $? -ne 0 ] && return 1
	else
		echo "unkwon commad."
		pack_usage
	fi
}

# run ./build.sh xxx command
function build()
{
	local mode=$@
	if [ $# -gt 1 ]; then
		echo "too much args"
		return 1
	fi

	if [ "x$mode" == "x" ]; then
		./build.sh
		[ $? -ne 0 ] && return 1
	elif [ "x$mode" == "xbrandy" ]; then
		./build.sh brandy
		[ $? -ne 0 ] && return 1
	elif [ "x$mode" == "xkernel" ]; then
		./build.sh kernel
		[ $? -ne 0 ] && return 1
	elif [ "x$mode" == "xrecovery" ]; then
		./build.sh recovery
		[ $? -ne 0 ] && return 1
	elif [ "x$mode" == "xbuildroot" ]; then
		./build.sh buildroot
		[ $? -ne 0 ] && return 1
	elif [ "x$mode" == "xdistclean" ]; then
		./build.sh distclean
		[ $? -ne 0 ] && return 1
	elif [ "x$mode" == "xclean" ]; then
		./build.sh clean
		[ $? -ne 0 ] && return 1
	elif [ "x$mode" == "xhelp" ]; then
		build_help
		[ $? -ne 0 ] && return 1
	else
		echo "unknown command."
		build_usage
	fi
}

function print_red(){
	echo -e '\033[0;31;1m'
	echo $1
	echo -e '\033[0m'
}

function swupdate_pack_swu() {

	#T=$(gettop)
	#\cd $T
	cd ${LICHEE_TOP_DIR}
	local BIN_DIR="${LICHEE_PLAT_OUT}"
	local SWU_DIR="${BIN_DIR}/swupdate"
	rm -rf $SWU_DIR
	mkdir -p $SWU_DIR
	local SWUPDATE_CONFIG_DIR="${LICHEE_BOARD_CONFIG_DIR}/${LICHEE_LINUX_DEV}"
	local SWUPDATE_COMMON_CONFIG_DIR="${LICHEE_BOARD_CONFIG_DIR}/${LICHEE_LINUX_DEV}"
	local SWUPDATE_GENERIC_CONFIG_DIR="${LICHEE_BOARD_CONFIG_DIR}/${LICHEE_LINUX_DEV}"
	#mkdir -p $SWUPDATE_CONFIG_DIR
	#local TARGET_COMMON="$(awk -F "-" '{print $1}' <<< ${TARGET_BOARD})-common"
	local CFG="sw-subimgs$1.cfg"
	#mkdir -p "$SWU_DIR"
	local storage_type_nor=0
	local f="${LICHEE_BOARD_CONFIG_DIR}/sys_config.fex"
	local B="$( awk -F"=" '/^storage_type/{print $2}' $f | sed 's/^[ \t]*//g' )"
	case $B in
		-1)
			print_red "###storage type error###"
			print_red "###cannot choose boot0, please config storage_type in sys_config ###"
			;;
		*0 | *5)
			local boot0_img=boot0_nand.fex
			;;
		*1 | *2 | *4)
			local boot0_img=boot0_sdcard.fex
			;;
		3)
			local boot0_img=boot0_spinor.fex
			storage_type_nor=1
			;;
		*)
			print_red "###storage type error###"
			print_red "###cannot choose boot0, please config storage_type in sys_config ###"
			;;
	esac
	#[ -n "$boot0_img" ] && {
	#	rm -rf $BIN_DIR/boot0.img
	#	dd if=$BIN_DIR/image/$boot0_img of=$BIN_DIR/boot0.img
	#}
	#local U="$(get_uboot)"
	#if [[ "$U" =~ "2011" ]]; then
	#	local uboot_img=u-boot.fex
	#else
	#	if [ x"$storage_type_nor" = x"1" ]; then
	#		local uboot_img=boot_package_nor.fex
	#	else
	#		local uboot_img=boot_package.fex
	#	fi
	#fi
	#rm -rf $BIN_DIR/uboot.img
	#dd if=$BIN_DIR/image/$uboot_img of=$BIN_DIR/uboot.img

	if [ -e $SWUPDATE_CONFIG_DIR/$CFG ]; then
		local SWUPDATE_SUBIMGS="$SWUPDATE_CONFIG_DIR/$CFG"
	elif [ -e $SWUPDATE_COMMON_CONFIG_DIR/$CFG ]; then
		local SWUPDATE_SUBIMGS="$SWUPDATE_COMMON_CONFIG_DIR/$CFG"
	else
		local SWUPDATE_SUBIMGS="$SWUPDATE_GENERIC_CONFIG_DIR/$CFG"
	fi

	unset swota_file_list
	unset swota_copy_file_list

	echo "####$SWUPDATE_SUBIMGS####"
	. $SWUPDATE_SUBIMGS
	echo ${swota_file_list[@]} | sed 's/ /\n/g'
	echo ${swota_copy_file_list[@]} | sed 's/ /\n/g'

	[ ! -f "$SWUPDATE_SUBIMGS" ] && print_red "$SWUPDATE_SUBIMGS not exist!!" &&  return 1

	echo "-------------------- config --------------------"
	echo "subimgs config by: $SWUPDATE_SUBIMGS"
	echo "out dir: $SWU_DIR"

	echo "-------------------- do copy --------------------"
	cp "$SWUPDATE_SUBIMGS" "$SWU_DIR"
	rm -f "$SWU_DIR/sw-subimgs-fix.cfg"

	# files pack into swu
	for line in ${swota_file_list[@]} ; do
		ori_file=$(echo $line | awk -F: '{print $1}')
		base_name=$(basename "$line")
		fix_name=${base_name#*:}
		[ ! -f "$ori_file" ] && print_red "$ori_file not exist!!" && return 1
		cp $ori_file $SWU_DIR/$fix_name
		echo $fix_name >> "$SWU_DIR/sw-subimgs-fix.cfg"
	done

	# files only copy but not pack into swu
	for line in ${swota_copy_file_list[@]} ; do
		ori_file=$(echo $line | awk -F: '{print $1}')
		base_name=$(basename "$line")
		fix_name=${base_name#*:}
		[ ! -f "$ori_file" ] && print_red "$ori_file not exist!!" && return 1
		cp $ori_file $SWU_DIR/$fix_name
		echo $fix_name >> "$SWU_DIR/sw-subimgs-copy.cfg"
	done
	\cd - > /dev/null
	\cd "$SWU_DIR"

	echo "-------------------- do sha256 --------------------"
	cp sw-description sw-description.bk
	[ -f $SWU_DIR/sw-subimgs-fix.cfg ] && {
		while IFS= read -r line
		do
			item="$line"
			if grep -q -E "sha256 = \"@$item\"" sw-description ; then
				echo "sha256 $item"
				item_hash=$(sha256sum "$item" | awk '{print $1}')
				item_size=$(du -b "$item" | awk '{print $1}')
				sed -i "s/\(.*\)\(sha256 = \"@$item\"\)/\1sha256 = \"$item_hash\"/g" sw-description
				sed -i "s/\(.*\)\(size = \"@$item\"\)/\1size = \"$item_size\"/g" sw-description
			fi
		done < "$SWU_DIR/sw-subimgs-fix.cfg"
	}

	[ -f $SWU_DIR/sw-subimgs-copy.cfg ] && {
		while IFS= read -r line
		do
			item="$line"
			if grep -q -E "sha256 = \"@$item\"" sw-description ; then
				echo "sha256 $item"
				item_hash=$(sha256sum "$item" | awk '{print $1}')
				item_size=$(du -b "$item" | awk '{print $1}')
				sed -i "s/\(.*\)\(sha256 = \"@$item\"\)/\1sha256 = \"$item_hash\"/g" sw-description
				sed -i "s/\(.*\)\(size = \"@$item\"\)/\1size = \"$item_size\"/g" sw-description
			fi
		done < "$SWU_DIR/sw-subimgs-copy.cfg"
	}

	diff sw-description.bk sw-description

	#echo "-------------------- do sign --------------------"

	#local swupdate_need_sign=""
	#grep "CONFIG_SWUPDATE_CONFIG_SIGNED_IMAGES=y" "$T/target/allwinner/${TARGET_BOARD}/defconfig" && {
	#	swupdate_need_sign=1
	#	echo "need do sign"
	#}

	#local swupdate_sign_method=""
	#local password_para=""
	##for rsa
	#local priv_key_file="$SWUPDATE_CONFIG_DIR/swupdate_priv.pem"
	#local password_file="$SWUPDATE_CONFIG_DIR/swupdate_priv.password"
	##for cms
	#local cert_cert_file="$SWUPDATE_CONFIG_DIR/swupdate_cert.cert.pem"
	#local cert_key_file="$SWUPDATE_CONFIG_DIR/swupdate_cert.key.pem"
    #
	#[ x$swupdate_need_sign = x"1" ] && {
	#	echo "add sw-description.sig to sw-subimgs-fix.cfg"
	#	sed '1 asw-description.sig' -i sw-subimgs-fix.cfg
	#	grep "CONFIG_SWUPDATE_CONFIG_SIGALG_RAWRSA=y" "$T/target/allwinner/${TARGET_BOARD}/defconfig" && swupdate_sign_method="RSA"
	#	grep "CONFIG_SWUPDATE_CONFIG_SIGALG_CMS=y" "$T/target/allwinner/${TARGET_BOARD}/defconfig" && swupdate_sign_method="CMS"
	#	[ -e "$password_file" ] && {
	#		echo "password file exist"
	#		password_para="-passin file:$password_file"
	#	}
    #
	#	if [ x"$swupdate_sign_method" = x"RSA" ]; then
	#		echo "generate sw-description.sig with rsa"
	#		openssl dgst -sha256 -sign "$priv_key_file" $password_para "$SWU_DIR/sw-description" > "$SWU_DIR/sw-description.sig"
	#	elif [ x"$swupdate_sign_method" = x"CMS" ]; then
	#		echo "generate sw-description.sig with cms"
	#		openssl cms -sign -in  "$SWU_DIR/sw-description" -out "$SWU_DIR/sw-description.sig" -signer "$cert_cert_file" \
	#			-inkey "$cert_key_file" -outform DER -nosmimecap -binary
	#	fi
	#}
    #
	echo "-------------------- do md5sum --------------------"
	local md5_file="cpio_item_md5"
	rm -f $md5_file
	while IFS= read -r line
	do
		md5sum "$line" >> $md5_file
	done < "$SWU_DIR/sw-subimgs-fix.cfg"
	echo "$md5_file" >> sw-subimgs-fix.cfg
	cat $md5_file

	local out_name="${LICHEE_IC}_${LICHEE_BOARD}$1.swu"
	echo "-------------------- do cpio --------------------"
	while IFS= read -r line
	do
		echo "$line"
	done < "$SWU_DIR/sw-subimgs-fix.cfg" | cpio -ov -H crc >  "$SWU_DIR/$out_name"

	echo "-------------------- out file in --------------------"
	echo ""
	print_red "$SWU_DIR/$out_name"
	du -sh "$SWU_DIR/$out_name"
	echo ""
	cd ${LICHEE_TOP_DIR}

}

function ota_ab(){
	echo "copy env.cfg"
	cp ${LICHEE_BOARD_CONFIG_DIR}/${LICHEE_LINUX_DEV}/env_ab.cfg  ${LICHEE_BOARD_CONFIG_DIR}/${LICHEE_LINUX_DEV}/env.cfg -rf
	echo "copy sys_partition.fex"
	cp ${LICHEE_BOARD_CONFIG_DIR}/${LICHEE_LINUX_DEV}/sys_partition_ab.fex  ${LICHEE_BOARD_CONFIG_DIR}/${LICHEE_LINUX_DEV}/sys_partition.fex
}

function ota_recovery(){
	echo "copy env.cfg"
	cp -f ${LICHEE_BOARD_CONFIG_DIR}/${LICHEE_LINUX_DEV}/env-recovery.cfg  ${LICHEE_BOARD_CONFIG_DIR}/${LICHEE_LINUX_DEV}/env.cfg
	echo "copy sys_partition.fex"
	cp -f ${LICHEE_BOARD_CONFIG_DIR}/${LICHEE_LINUX_DEV}/sys_partition-recovery.fex  ${LICHEE_BOARD_CONFIG_DIR}/${LICHEE_LINUX_DEV}/sys_partition.fex
}

function swupdate_make_delta()
{
	[ $# -lt 2 ] && print_red "usage:swupdate_make_delta base_swu new_swu [output_dir]" && return 1

	local BIN_DIR="${LICHEE_PLAT_OUT}"
	local SWU_DIR="${BIN_DIR}/swupdate"
	local base_swu=$(readlink -f "$1")
	local new_swu=$(readlink -f "$2")
	local delta_dir="${LICHEE_OUT_DIR}/swupdate_delta"
	[ -n "$3" ] && delta_dir="$3/swupdate_delta"

	echo "base_swu  : $base_swu"
	echo "new_swu   : $new_swu"
	echo "delta_dir : $delta_dir"

	rm -rf "$delta_dir"
	mkdir -p "$delta_dir/base"
	mkdir -p "$delta_dir/base_sig"
	mkdir -p "$delta_dir/new"
	mkdir -p "$delta_dir/delta"

	local md5_file="cpio_item_md5"
	local check=""

	cd "$delta_dir/base"
	cpio -idmv < "$base_swu"
	check=$(md5sum --quiet -c $md5_file)
	[ x"$check" = x"" ] || {
		print_red "check md5 fail"
		md5sum -c $md5_file
		cd -
		return 1
	}
	cd -

	cd "$delta_dir/new"
	cpio -idmv < "$new_swu"
	check=$(md5sum --quiet -c $md5_file)
	[ x"$check" = x"" ] || {
		print_red "check md5 fail"
		md5sum -c $md5_file
		cd -
		return 1
	}
	cd -

	#uncompress *.gz before make delta
	for file in "$delta_dir"/base/*; do
		local filename=$(basename "$file")
		local basefile=$delta_dir/base/$filename
		local newfile=$delta_dir/new/$filename
		if [ x${filename##*.} = x"gz" ]; then
			echo "unzip $basefile"
			gzip -d $basefile
			echo "unzip $newfile"
			gzip -d $newfile
		fi
		if [ x${filename##*.} = x"zst" ]; then
			echo "unzstd $basefile"
			zstd -d $basefile
			echo "unzstd $newfile"
			zstd -d $newfile
		fi
	done

	for file in "$delta_dir"/base/*; do
		local filename=$(basename "$file")
		local basefile=$delta_dir/base/$filename
		local newfile=$delta_dir/new/$filename
		local sigfile=$delta_dir/base_sig/$filename.rdiff.sig
		local deltafile=$delta_dir/delta/$filename.rdiff.delta

		if [ -f "$basefile" ]; then
			${LICHEE_BOARD_CONFIG_DIR}/${LICHEE_LINUX_DEV}/rdiff signature "$basefile" "$sigfile"
			[ -e "$newfile" ] && {
				echo "prepare $deltafile"
				${LICHEE_BOARD_CONFIG_DIR}/${LICHEE_LINUX_DEV}/rdiff delta "$sigfile" "$newfile" "$deltafile"
			}
		fi
	done
	ll -h "$delta_dir/delta/"
}

export LICHEE_TOP_DIR=$(cd $(dirname ${BASH_SOURCE[0]})/.. && pwd)

if [ ! -f $LICHEE_TOP_DIR/.buildconfig ]; then
	./build.sh config
fi

. $LICHEE_TOP_DIR/.buildconfig
build_help
