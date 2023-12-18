#!/system/bin/sh

umask 0
RESTORECON_LIST_PATH="/data/misc/installd/.awrestoreconlist"
retries=100
set -x
exec 1>> /data/misc/installd/.awpreloaddatalog2
exec 2>> /data/misc/installd/.awpreloaddatalog2

while [ -f "$RESTORECON_LIST_PATH" -a $retries -gt 0 ]; do
    echo "do restorecon package dir in /data/data/ of $retries"
    list=$(cmd package list package -3)
    for p in $list; do
        pname=${p:8}
        if [ -n "$(grep "^${pname}$" $RESTORECON_LIST_PATH)" ]; then
            restorecon -R "/data/data/${pname}"
            sed -i "/${pname}/d" "$RESTORECON_LIST_PATH"
        fi
    done
    if [ ! -s $RESTORECON_LIST_PATH ]; then
        rm $RESTORECON_LIST_PATH
    fi
    sleep 1
    let "retries--"
done
