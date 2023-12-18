#!/bin/bash

# Copyright (C) 2020 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -e

bootstrap=
all=
in=
out=

function usage() {
  echo "usage: $0 [--bootstrap|--all] --in in --out out" && exit 1
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --bootstrap)
      bootstrap=yes
      shift
      ;;
    --all)
      all=yes
      shift
      ;;
    --in)
      in=$2
      shift
      shift
      ;;
    --out)
      out=$2
      shift
      shift
      ;;
    *)
      usage
  esac
done

if [ -z $in ] || [ -z $out ]; then
  usage
fi

if [ ! -z $bootstrap ] && [ ! -z $all ]; then
  usage
fi

activate_level=0
if [ ! -z $bootstrap ]; then
  activate_level=1
elif [ ! -z $all ]; then
  activate_level=2
fi

function get_level() {
  case $1 in
    com.android.art|com.android.runtime|com.android.i18n|com.android.tzdata|com.android.vndk.vR)
      echo 1 ;;
    *)
      echo 2 ;;
  esac
}

function abs() {
  if [[ $1 = /* ]]; then
    echo $1
  else
    echo $(realpath $(pwd)/$1)
  fi
}

ROOT_IN=$(abs $in)
ROOT_OUT=$(abs $out)

# to use relative paths
cd $(dirname $0)

rm -iRf $ROOT_OUT
mkdir -p $ROOT_OUT
mkdir -p $ROOT_OUT/apex
cp -R $ROOT_IN/* $ROOT_OUT

if test -f $ROOT_OUT/system/etc/linker.config.json; then
  conv_linker_config proto -s $ROOT_OUT/system/etc/linker.config.json -o $ROOT_OUT/system/etc/linker.config.pb
fi

apexInfo=$ROOT_OUT/apex/apex-info-list.xml
echo "<?xml version=\"1.0\" encoding=\"utf-8\"?>" > $apexInfo
echo "<apex-info-list>" > $apexInfo

for partition in system product system_ext vendor; do
  if [ -d $ROOT_OUT/$partition/apex ]; then
    for src in $ROOT_OUT/$partition/apex/*/; do
      name=$(basename $src)
      dst=$ROOT_OUT/apex/$name
      preinstalled_path=/$(realpath --relative-to=$ROOT_OUT $src)
      module_path=/$(realpath --relative-to=$ROOT_OUT $dst)
      if [ $(get_level $name) -le $activate_level ]; then
        cp -r $src $dst
        conv_apex_manifest proto $dst/apex_manifest.json -o $dst/apex_manifest.pb
        if test -f $dst/etc/linker.config.json; then
          conv_linker_config proto -s $dst/etc/linker.config.json -o $dst/etc/linker.config.pb
        fi
        mkdir $dst/lib
        echo " <apex-info moduleName=\"$name\" modulePath=\"$module_path\" preinstalledModulePath=\"$preinstalled_path\" isFactory=\"true\" isActive=\"true\" />" >> $apexInfo
      else
        echo " <apex-info moduleName=\"$name\" preinstalledModulePath=\"$preinstalled_path\" isFactory=\"true\" isActive=\"false\" />" >> $apexInfo
      fi
    done
  fi
done

echo "</apex-info-list>" >> $apexInfo
