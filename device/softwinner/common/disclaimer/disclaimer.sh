#!/bin/bash
# scripts/disclaimer.sh
#
# (c) Copyright 2018
# Allwinner Technology Co., Ltd. <www.allwinnertech.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
function start_disclaimer_option()
{
	local sdk_build_top=$PWD
	local disclaimer_dir="${sdk_build_top}/device/softwinner/common/disclaimer"
	local disclaimer_accept="${sdk_build_top}/.disclaimer_accpet"
	local disclaimer_notice="${disclaimer_dir}/Allwinnertech_Disclaimer_Notice.txt"
	local disclaimer_md="${disclaimer_dir}/Allwinnertech_Disclaimer(Cn_En)_20181122.md"

	if [ -f ${disclaimer_accept} ]; then
		return 0
	fi

	if ([ ! -d ${disclaimer_dir} ]) || ([ ! -f ${disclaimer_notice} ]); then
		echo "SDK miss disclaimer notice files in {disclaimer_dir}!!!"
		return 1
	fi

	# show the disclaimer information
	echo -e "\nBefore using this files, please make sure that you note the following important information."
	echo "**********************************************************************"
	cat ${disclaimer_notice}
	echo "**********************************************************************"
	echo -e "You can read ${disclaimer_md} for detailed information. \n"

	# countdown waits for reading end
	local countdown=8
	echo "You read time left ${countdown} seconds...."
	while [ $countdown -gt 0 ]; do
		echo -ne "$countdown"
		sleep 1
		let countdown-=1
		echo -ne "\r 	\r"
	done

	# deal with customer selection option
	while true; do
		read -r -p "I have already read, understood and accepted the above terms? [Y/N]" select
		case $select in
			[yY][eE][sS]|[yY])
				echo "You select Yes, Build continue...."
				touch ${disclaimer_accept}
				export > ${disclaimer_accept}
				break;
				;;
			[nN][oO]|[nN])
				echo "if you choose NO, you would not be authorized to use the SDK!"
				return 1
				;;
			*)
				echo "Invalid input..."
		esac
	done

	return 0
}

start_disclaimer_option