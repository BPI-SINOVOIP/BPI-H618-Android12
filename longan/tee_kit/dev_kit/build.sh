#!/bin/bash

PLATFORM="sun8iw3p1"
ATF_EXIST=""
MODE=""
demos[0]=""

collect_demos()
{
	local count=0
	for demo in $( find demo -mindepth 1 -maxdepth 1 -type d |sort ); do
		demos[$count]=`basename $demo`
		let count=$count+1
	done
}

list_demos()
{
	local count=0
	for demo in ${demos[*]}; do
		local probe="$count. $demo"
		if [ -f demo/$demo/README ]; then
			probe=${probe}' '---`cat demo/$demo/README`
		fi
		echo $probe
		let count=$count+1
	done
}

build_one_demo()
{
	if [ $# -gt 0 ]; then
		make -C $1
		exit
	fi

	list_demos
	while true; do
		read -p "Please select a demo to build:"
		RES=`expr match $REPLY "[0-9][0-9]*$"`
		if [ "$RES" -le 0 ]; then
			echo "please use index number"
			continue
		fi
		if [ "$REPLY" -ge ${#demos[*]} ]; then
			echo "too big"
			continue
		fi
		if [ "$REPLY" -lt "0" ]; then
			echo "too small"
			continue
		fi
		index=$REPLY
		make -C demo/${demos[$index]}
		break
	done
}

show_help()
{
	printf "\nbuild.sh - Top level build scritps\n"
	echo "Valid Options:"
	echo "  -h  Show help message"
	echo "  -t install gcc tools chain"
	echo "  clean     clean the tmp files"
	echo "  config    set compile settings"
	echo "  demo      select one demo from list and build"
	echo "  $${demo_dir} build demo in $${demo_dir}"
	echo "  list      list avaliable demos"
	printf "\n\n"
}

prepare_toolchain()
{
        local ARCH="arm";
        local GCC="";
        local GCC_PREFIX="";
        local toolchain_archive_aarch64="../build/toolchain/gcc-linaro-5.3.1-2016.05-x86_64_aarch64-linux-gnu.tar.xz";
        local toolchain_archive_arm="../build/toolchain/gcc-linaro-5.3.1-2016.05-x86_64_arm-linux-gnueabi.tar.xz";
        local another_toolchain_archive_aarch64="../prebuilt/kernelbuilt/aarch64/gcc-linaro-5.3.1-2016.05-x86_64_aarch64-linux-gnu.tar.xz";
        local another_toolchain_archive_arm="../prebuilt/kernelbuilt/arm/gcc-linaro-5.3.1-2016.05-x86_64_arm-linux-gnueabi.tar.xz";
        local tooldir_aarch64="../out/gcc-linaro-5.3.1-2016.05-x86_64_aarch64-linux-gnu";
        local tooldir_arm="../out/gcc-linaro-5.3.1-2016.05-x86_64_arm-linux-gnueabi";
        local another_tooldir_aarch64="../out/gcc-linaro-5.3.1-2016.05-x86_64_aarch64-linux-gnu";
        local another_tooldir_arm="../out/gcc-linaro-5.3.1-2016.05-x86_64_arm-linux-gnueabi";

	if [ -e ../build/toolchain ]; then
	        if [ ! -d "${tooldir_aarch64}" ]; then
        		echo "Prepare toolchain `basename ${tooldir_aarch64}`..."
                	mkdir -p ${tooldir_aarch64} || exit 1
                	tar --strip-components=1 -xf ${another_toolchain_archive_aarch64} -C ${tooldir_aarch64} || exit 1
        	fi

        	if [ ! -d "${tooldir_arm}" ]; then
        		echo "Prepare toolchain `basename ${tooldir_arm}`..."
                	mkdir -p ${tooldir_arm} || exit 1
                	tar --strip-components=1 -xf ${toolchain_archive_arm} -C ${tooldir_arm} || exit 1
		fi
	else
	        if [ ! -d "${another_tooldir_aarch64}" ]; then
        		echo "Prepare toolchain `basename ${another_tooldir_aarch64}`..."
                	mkdir -p ${another_tooldir_aarch64} || exit 1
                	tar --strip-components=1 -xf ${another_toolchain_archive_aarch64} -C ${another_tooldir_aarch64} || exit 1
		fi

        	if [ ! -d "${another_tooldir_arm}" ]; then
        		echo "Prepare toolchain `basename ${another_tooldir_arm}`..."
                	mkdir -p ${another_tooldir_arm} || exit 1
                	tar --strip-components=1 -xf ${another_toolchain_archive_arm} -C ${another_tooldir_arm} || exit 1
		fi

	fi

}

show_success()
{
	echo -e "\033[40;32m #### make completed successfully  #### \033[0m"
}

show_error()
{
	echo -e "\033[40;31m #### make failed to build some targets  #### \033[0m"
}

clean_all()
{
	for demo in ${demos[*]}; do
		make clean -C demo/${demo}
		if [ $? -ne 0 ]
		then
			show_error
		else
			show_success
		fi
	done
}

build_all()
{
	for demo in ${demos[*]}; do
		make -C demo/${demo}
		if [ $? -ne 0 ]
		then
			show_error
			exit
		else
			show_success
		fi
	done
}
probe_config()
{
	if [ ! -f platform_config.mk ]; then
		echo -e "\033[40;31m #### config.mk is not exist!       #### \033[0m"
		echo -e "\033[40;31m #### use: ./build.sh config first  #### \033[0m"
		exit
	fi
}

build_select_chip()
{
	local count=0
	local chip=0

	if [ -f ../.buildconfig ]; then
		chip=`cat ../.buildconfig | grep -w "LICHEE_CHIP" | awk -F= '{printf $2}'`
		if [ x${chip} = x ]; then
			echo "please config longan first"
			exit;
		fi
	else
		echo "please config longan first"
		exit;
	fi

	if [ ! -d dev_kit/arm-plat-${chip} ]; then
		echo "dev_kit for `cat ../.buildconfig | grep -w "LICHEE_IC" | awk -F= '{printf $2}'` not provided"
		exit;
	fi
	local chip_dir=arm-plat-${chip}

	if grep -q 'CFG_WITH_ARM_TRUSTED_FW' dev_kit/${chip_dir}/"export-ta_arm32"/host_include/conf.mk ; then
		ATF_EXIST='y' ;
	else
		ATF_EXIST='n' ;
	fi

	SUNXI_TA_ENCRYPT='n' ;
	if grep -q 'CFG_SUNXI_TA_ENCRYPT_SUPPORT' dev_kit/${chip_dir}/"export-ta_arm32"/mk/conf.mk ; then
		while true; do
			read -p "encrypt TA(y/n):"
			if [ x"$REPLY" = x"y" ]; then
				SUNXI_TA_ENCRYPT='y' ;
				break
			fi
			if [ x"$REPLY" = x"n" ]; then
				break
			fi
		done
	fi

	if [ $SUNXI_TA_ENCRYPT = 'y' ]; then
		USING_DERIVE_KEY='n'
		while true; do
			read -p "using derive key(y/n):"
			if [ x"$REPLY" = x"y" ]; then
				USING_DERIVE_KEY='y' ;
				break
			fi
			if [ x"$REPLY" = x"n" ]; then
				break
			fi
		done
	fi

	touch platform_config.mk
	echo "# auto-generated t-coffer configuration file" > platform_config.mk
	echo -e "\nPLATFORM := ${chip}\n" >> platform_config.mk
	echo -e "ATF_EXIST := ${ATF_EXIST}\n" >> platform_config.mk
	echo -e "SUNXI_TA_ENCRYPT := ${SUNXI_TA_ENCRYPT}\n" >> platform_config.mk
	echo -e "USING_DERIVE_KEY := ${USING_DERIVE_KEY}\n" >> platform_config.mk
	echo -e "export  PLATFORM\n" >> platform_config.mk
	echo -e "export  ATF_EXIST\n" >> platform_config.mk
	echo -e "export  SUNXI_TA_ENCRYPT\n" >> platform_config.mk
	echo -e "export  USING_DERIVE_KEY\n" >> platform_config.mk

	prepare_toolchain
	# touch link.mk to make sure encrypt config take effect
	# after this config changing
	touch dev_kit/${chip_dir}/export-ta_arm32/mk/link.mk
}

collect_demos
while [ $# -gt 0 ]; do
	case "$1" in
	-t)
	prepare_toolchain
	exit;
	;;
	-h)
	show_help
	exit;
	;;
	config*)
		build_select_chip;
		exit;
		;;
	distclean)
		if [ ! -f platform_config.mk ]; then
			touch platform_config.mk
		fi
		clean_all
		rm platform_config.mk
		exit;
		;;
	clean)
		clean_all
		exit;
		;;
	list)
		list_demos
		exit;
		;;
	demo)
		build_one_demo
		exit;
		;;
	*)
		if [ -d "$1" ]; then
			build_one_demo $1
		fi
		exit;
		;;
	esac;
done

probe_config
build_all


