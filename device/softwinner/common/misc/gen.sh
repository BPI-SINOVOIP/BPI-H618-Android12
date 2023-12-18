#!/bin/bash

dest=$1
size=16777216

function init_data()
{
	local dest=$1
	local size=$2
	local bs=4096
	dd if=/dev/zero of=$dest bs=$bs count=$((size/bs)) 2>/dev/null
}

function write_data()
{
	local dest=$1
	local offset=$2
	local data=$3
	local addr=$(printf "%d" $offset)
	for e in $data; do
		printf "\x$e" | dd of=$dest bs=1 count=1 seek=$addr conv=notrunc 2>/dev/null
		addr=$((addr+1))
	done
}

init_data  $dest $size

# boot from slot A
write_data $dest 0x0800  "61 00 00 00 42 43 41 42  01 02 00 00 9f 00 7f 00"
write_data $dest 0x0818  "00 00 00 00 7c fd 96 02"
write_data $dest 0x8000  "02 b0 0a 74 56 00 00 00"
