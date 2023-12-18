#!/bin/bash
#
# This script generates the apex manifest version number (which is also used for
# the outer aab/jar object's version available to PackageManager).
#
# That version is limited to INT_MAX
# Strategy:
#   if(local eng build)
#      version = 2147480000
#   else
#      version = numeric part of build number
#
# 2147480000 is chosen as being a value that can install over any expected build
# server build number that is still a little smaller than INT_MAX to leave room
# for maneuvering

default_eng_build_number=2147480000

build_number=$(cat $OUT_DIR/soong/build_number.txt)
if [[ "$build_number" == "eng."* ]]; then
  numeric_build_number=$default_eng_build_number
else
  numeric_build_number=$(cat $OUT_DIR/soong/build_number.txt | tr -d -c 0-9)
  if [[ -z "$numeric_build_number" ]]; then
    numeric_build_number=$default_eng_build_number
  fi
  if ((numeric_build_number < 1)); then
    numeric_build_number=1
  fi
  if ((numeric_build_number >= default_eng_build_number)); then
    numeric_build_number=$((default_eng_build_number-1))
  fi
fi

cat $1 | sed -E "s/\{BUILD_NUMBER\}/$numeric_build_number/g" | sed -E "s/\{BUILD_ID\}/$build_number/g" > $2

