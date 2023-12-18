#!/usr/bin/env bash

srcpath=$2
target=$3

source=$srcpath/$(basename $target)

echo source: $source
echo target: $target

mkdir -p $(dirname $target)
echo "/* Auto-generated */"    > $target
echo "/* source: $source */"  >> $target
echo "/* target: $target */"  >> $target
echo "/* date  : $(date) */"  >> $target
[ -f $source ] && cat $source >> $target

sed -i "s|/system/bin/|/vendor/bin/|g" $target
