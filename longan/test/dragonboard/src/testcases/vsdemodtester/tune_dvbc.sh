#!/bin/sh

export TRID_PLATFORM="DVB"
export LD_LIBRARY_PATH=./lib
export TRID_DEBUG_TUNERAPP=MAIN:3,DVB:3,ATSC:3,TUNER:3

insmod ./Driver/hidtvreg_dev.ko
insmod ./Driver/i2c_all_dev.ko

./Program/TestTuner DVBC $1 $2 $3 
