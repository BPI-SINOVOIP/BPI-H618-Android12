#!/bin/bash
#
# scripts/mkcommon.sh
# (c) Copyright 2013
# Allwinner Technology Co., Ltd. <www.allwinnertech.com>
# James Deng <csjamesdeng@allwinnertech.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

export LC_ALL=C
BR_SCRIPTS_DIR=$(cd $(dirname $0) && pwd)
BUILD_CONFIG=$BR_SCRIPTS_DIR/../.buildconfig
SKIP_BR=0

# source shflags
source ${BR_SCRIPTS_DIR}/shflags

if [ -f ${BUILD_CONFIG} ]; then
	. ${BUILD_CONFIG}
	LICHEE_TOP_DIR_NEW=$(cd $BR_SCRIPTS_DIR/.. && pwd)
	if [ -n "$LICHEE_TOP_DIR" -a "$LICHEE_TOP_DIR_NEW" != "$LICHEE_TOP_DIR" ]; then
		sed -i "s|$LICHEE_TOP_DIR|$LICHEE_TOP_DIR_NEW|g" ${BUILD_CONFIG}
		. ${BUILD_CONFIG}
	fi
fi

# define option, format:
#   'long option' 'default value' 'help message' 'short option'
# WARN: Don't modify default value, because we will check it later
DEFINE_string  'ic'       ''          'ic to build, e.g. V316'             'i'
DEFINE_string  'kernel'   ''          'Kernel to build, e.g. 3.3'          'k'
DEFINE_string  'board'    ''          'Board to build, e.g. evb'           'b'
DEFINE_string  'flash'    'default'   'flash to build, e.g. nor'           'n'
DEFINE_string  'os'       ''          'os to build, e.g. android bsp'      'o'
DEFINE_string  'arch'     ''          'arch to build, e.g. arm arm64'      'a'
DEFINE_string  'module'   'all'       'Module to build, e.g. buildroot, kernel, uboot, clean' 'm'
DEFINE_string  'sata_mod' 'all'       'sata module to build when build sata, e.g. all, spi, pinctrl, ...' 's'

DEFINE_boolean 'config'   false       'Config compile platfom'             'c'
DEFINE_boolean 'force'    false       'Force to build, clean some old files. ex, rootfs/' 'f'

FLAGS_HELP="Top level build script for lichee

Examples:
  1. Build lichee, it maybe config platform options, if
     you first use it. And it will pack firmware use default
     argument.
      $ ./build.sh

  2. Configurate platform option
      $ ./build.sh config

  3. Autoconfigurete
      $ ./build.sh autoconfig -i V316 -o bsp -n nor -b perf1

  4. Pack linux, dragonboard image
      $ ./build.sh pack[_<option>] e.g. debug, dump, prvt, verity

  5. Build lichee using command argument
      $ ./build.sh -i <ic> e.g. V316

  6. Build module using command argument
      $ ./build.sh -m <module>

  7. Build special kernel
      $ ./build.sh -k <kernel directly>

  8. Build forcely to clean rootfs dir
      $ ./build.sh -f

  9. compress or decompress the ramfs
      decompress: $ ./build.sh ramfs e|x src_absolute_path dst_absolute_address
      compress:   $ ./build.sh ramfs  c  src_absolute_path dst_absolute_address
"
# Include base command!
source ${BR_SCRIPTS_DIR}/mkcmd.sh

# Invoke 'bsp.sh' directly
if echo "$1" | grep -w "bsp" >/dev/null; then
    opt=${@/bsp/}  # Remove 'bsp' from '$@'
    bsp_action $opt
    exit $?
fi

# parse the command-line
FLAGS "$@" || exit $?
eval set -- "${FLAGS_ARGV}"

ic=${FLAGS_ic}
platform=${FLAGS_os}
kernel=${FLAGS_kernel}
board=${FLAGS_board}
flash=${FLAGS_flash}
run_arch=${FLAGS_arch}
module=${FLAGS_module}
config=${FLAGS_config}
sata_mod=${FLAGS_sata_mod}
force=""

################ Preset an empty command #################
function nfunc(){
	echo "Begin Action"
}
ACTION=":;"

################ Parse other arguments ###################
while [ $# -gt 0 ]; do
	case "$1" in
	config*)
		opt=${1##*_};
		if [ "${opt}" == "all" ]; then
			export CONFIG_ALL=${FLAGS_TRUE};
		else
			export CONFIG_ALL=${FLAGS_FALSE};
		fi
		FLAGS_config=${FLAGS_TRUE};
		break;
		;;
	autoconfig)
		ACTION="mk_autoconfig;"
		FLAGS_config=${FLAGS_TRUE};
		break;
		;;
	loadconfig|menuconfig|saveconfig|mergeconfig)
		ACTION="kernel_config $@;"
		module=""
		;;
	pack*)
		optlist=$(echo ${1#pack} | sed 's/_/ /g')
		mode=""
		for opt in $optlist; do
			case "$opt" in
				debug)
					mode="$mode -d card0"
					;;
				dump)
					mode="$mode -m dump"
					;;
				prvt)
					mode="$mode -f prvt"
					;;
				secure)
					mode="$mode -v secure"
					;;
				prev)
					mode="$mode -s prev_refurbish"
					;;
				crash)
					mode="$mode -m crashdump"
					;;
				vsp)
					mode="$mode -t vsp"
					;;
				raw)
					mode="$mode -w programmer"
					;;
				verity)
					mode="$mode --verity"
					;;
				signfel)
					mode="$mode --signfel"
					;;
				*)
					mk_error "Invalid pack option $opt"
					exit 1
					;;
			esac
		done

		######### Don't build other module, if pack firmware ########
		ACTION="mkpack ${mode};";
		module="";
		break;
		;;
	buildroot)
		ACTION="mkbr;";
		module=buildroot;
		break;
		;;
	clean|distclean)
		ACTION="mk${1};";
		module="";
		break;
		;;
	bootloader|brandy|arisc|rootfs)
		ACTION="mk${1};";
		module="${1}";
		break;
		;;
	kernel|modules)
		ACTION="mkkernel $1;";
		module="";
		break;
		;;
	recovery)
		ACTION="mkrecovery;";
		module="recovery";
		break;
		;;
	dts)
		ACTION="mkdts;";
		module="";
		break;
		;;
	ramfs)
		# $2:action $3:src $4:dst
		ACTION="mk_ramfs $2 $3 $4;";
		module="";
		break
		;;
	*) ;;
	esac;
	shift;
done

# if not .buildconfig or FLAGS_config equal FLAGS_TRUE, then mksetup.sh & exit
if [ ! -f ${BUILD_CONFIG}  -o  ${FLAGS_config} -eq ${FLAGS_TRUE} ]; then

	#
	# If we do config, it must be clean the old env var.
	#
	clean_old_env_var

	if [ "x$1" == "xautoconfig" ]; then
		tmp=${flash}
		source ${BR_SCRIPTS_DIR}/mkcmd.sh
		mk_autoconfig "${ic}" "${platform}" "${board}" "${tmp}" "${kernel}" "${run_arch}" "${sata_mod}"
		exit 0;
	fi
	. ${BR_SCRIPTS_DIR}/mksetup.sh

	if [ ${FLAGS_config} -eq ${FLAGS_TRUE} ]; then
		exit 0;
	fi
fi

if [ ${FLAGS_force} -eq ${FLAGS_TRUE} ]; then
	force="f";
fi

if [ "x${LICHEE_LINUX_DEV}" = "xlongan" -o "x${LICHEE_LINUX_DEV}" = "xtinyos" ] ; then
	SKIP_BR=0;
else
	SKIP_BR=1;
fi

if [ -n "${platform}" -o -n "${ic}" \
	-o -n "${kernel}" -o -n "${board}" ]; then \

	if [ "${platform}" = "${ic}" ] ; then \
		platform="linux";
	fi

	if ! init_ic ${ic} || \
		! init_platforms ${platform} ; then \
		mk_error "Invalid platform '${FLAGS_platform}'";
		exit 1;
	fi

	if ! init_kern_ver ${kernel} ; then \
		mk_error "Invalid kernel '${FLAGS_kernel}'";
		exit 1;
	fi

	if [ ${FLAGS_board} ] && \
		! init_boards ${LICHEE_CHIP} ${board} ; then \
		mk_error "Invalid board '${FLAGS_board}'";
		exit 1;
	fi
fi

# mkdir output dir
check_output_dir

############### Append ',' end character #################
module="${module},"
while [ -n "${module}" ]; do
	act=${module%%,*};
	case ${act} in
		all*)
			ACTION="mklichee;";
			module="";
			break;
			;;
		uboot)
			ACTION="${ACTION}mkboot;";
			;;
	esac
	module=${module#*,};
done


#
# Execute the action list.
#
echo "ACTION List: ${ACTION}========"
action_exit_status=$?
while [ -n "${ACTION}" ]; do
	act=${ACTION%%;*};
	echo "Execute command: ${act} ${force}"
	${act} ${force}
	action_exit_status=$?
	ACTION=${ACTION#*;};
done

exit ${action_exit_status}
