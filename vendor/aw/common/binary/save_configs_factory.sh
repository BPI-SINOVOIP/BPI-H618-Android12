#!/sbin/busybox sh
BUSYBOX="/sbin/busybox"
if [ -f "/data/system/pap-secrets-eth0" ];then
    $BUSYBOX cp /data/system/pap-secrets-eth0 /cache/pap-secrets-eth0
fi

if [ -f "/data/system/pap-secrets-wlan0" ];then
    $BUSYBOX cp /data/system/pap-secrets-wlan0 /cache/pap-secrets-wlan0
fi

if [ -f "/data/system/chap-secrets-eth0" ];then
    $BUSYBOX cp /data/system/chap-secrets-eth0 /cache/chap-secrets-eth0
fi

if [ -f "/data/system/chap-secrets-wlan0" ];then
    $BUSYBOX cp /data/system/chap-secrets-wlan0 /cache/chap-secrets-wlan0
fi
exit 0
