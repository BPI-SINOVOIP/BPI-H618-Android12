#!/sbin/busybox sh

BUSYBOX="/sbin/busybox"

if [ ! -e /data/system.notfirstrun ] ; then
    echo "do preinstall job"

    /system/bin/sh /system/bin/pm preinstall /system/preinstall
    /system/bin/sh /system/bin/pm preinstall /sdcard/preinstall

    $BUSYBOX touch /data/system.notfirstrun

    echo "preinstall ok"
else
    echo "do nothing"
fi
qw --daemon
