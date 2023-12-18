#!/bin/bash

property="$1"
target="$2"

mkdir -p $(dirname $target)

echo "# Auto-generated"  > $target
echo "# date: $(date)"  >> $target
[ -z "$property" ] && return 0

echo "on init"          >> $target

for p in $property; do
    key=$(echo $p | awk -F= '{print $1}')
    val=$(echo $p | awk -F= '{print $2}')
    echo "    setprop $key $val"  >> $target
done
