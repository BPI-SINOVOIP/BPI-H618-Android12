#!/bin/bash

localpath=$(cd $(dirname $0) && pwd)
support_list=($(find $localpath -maxdepth 1 -type d -name "android*" | xargs -I {} basename {}))

function detect_support_version()
{
	local fw=$(find $localpath -maxdepth 1 -name "*.img")
	local version v s
	if [ -n "$fw" ]; then
		echo "Start detect firmware version..."
		if [ "$(echo $fw | wc -w )" -gt 1 ]; then
			echo "Multi firmware file find, please check!"
			return 1
		else
			version=$(grep -aob -m 1 "ro.system.build.version.release=.*" $fw | awk -F= '{print $2}')
			if [ -z "$version" ]; then
				echo "Cannot detect firmware version!"
				return 1
			fi
			version="android${version}"
			for s in ${support_list[@]}; do
				if [ "$version" == "$s" ]; then
					echo "Support version[$version] for image: $fw"
					echo $s > $localpath/match_version
					mv $fw $localpath/$version
					return 0
				fi
			done
		fi
	else
		return 0
	fi
	echo "Cannot find support pack hook version[$version]."
	return 1
}

function run_match_hook()
{
	local version success

	detect_support_version
	[ $? -ne 0 ] && return $?

	if [ ! -f $localpath/match_version ]; then
		echo "Maybe no firmware found & no detect version stored, please check!"
		return 1
	fi

	version="$(cat $localpath/match_version | head -n 1)"
	success="false"
	for s in ${support_list[@]}; do
		if [ "$version" == "$s" ]; then
			echo "Use pack hook version: $version"
			success="true"
			break
		fi
	done

	[ "$success" == "false" ] && return 1

	[ ! -d $localpath/$version/override ] && \
	[ -d $localpath/override ] && \
	(cd $localpath && ln -sf override $version/override)

	$localpath/$version/pack_hook.sh $@
}

run_match_hook $@
