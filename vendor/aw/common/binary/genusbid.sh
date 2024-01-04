#!/sbin/busybox sh

BUSYBOX="/sbin/busybox"

if [ ! -e /data/.usbrandom ] ; then
    echo "do genusbid job"
    $BUSYBOX echo -e "$((RANDOM%10))$((RANDOM%10))$((RANDOM%10))$((RANDOM%10))$((RANDOM%10))$((RANDOM%10))$((RANDOM%10))$((RANDOM%10))\c" > /data/.usbrandom
    $BUSYBOX touch /data/.usbrandom
    $BUSYBOX cat  /data/.usbrandom  > /sys/class/android_usb/android0/iSerial
    echo "gen ok"
else
	$BUSYBOX cat  /data/.usbrandom  > /sys/class/android_usb/android0/iSerial
    echo "set id ok"
fi

