#!/sbin/busybox sh
BUSYBOX="/sbin/busybox"

if [ ! -d "/data/system" ];then
    $BUSYBOX mkdir -p /data/system
fi

$BUSYBOX chown 1000:1000 /data/system
$BUSYBOX chmod 0755 /data/system

if [ -f "/cache/pap-secrets-eth0" ];then
    $BUSYBOX cp /cache/pap-secrets-eth0 /data/system/
    $BUSYBOX chown 1000:1000 /data/system/pap-secrets-eth0
    $BUSYBOX chmod 0600 /data/system/pap-secrets-eth0
fi

if [ -f "/cache/pap-secrets-wlan0" ];then
    $BUSYBOX cp /cache/pap-secrets-wlan0 /data/system/
    $BUSYBOX chown 1000:1000 /data/system/pap-secrets-wlan0
    $BUSYBOX chmod 0600 /data/system/pap-secrets-wlan0
fi

if [ -f "/cache/chap-secrets-eth0" ];then
    $BUSYBOX cp /cache/chap-secrets-eth0 /data/system/
    $BUSYBOX chown 1000:1000 /data/system/chap-secrets-eth0
    $BUSYBOX chmod 0600 /data/system/chap-secrets-eth0
fi

if [ -f "/cache/chap-secrets-wlan0" ];then
    $BUSYBOX cp /cache/chap-secrets-wlan0 /data/system/
    $BUSYBOX chown 1000:1000 /data/system/chap-secrets-wlan0
    $BUSYBOX chmod 0600 /data/system/chap-secrets-wlan0
fi

exit 0

