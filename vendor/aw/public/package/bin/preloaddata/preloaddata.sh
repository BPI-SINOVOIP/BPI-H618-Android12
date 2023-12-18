#!/system/bin/sh

umask 0
TEMP_PATH="/data/.awtemp"
DATA_PATH="/data"
RESTORECON_LIST_PATH="/data/misc/installd/.awrestoreconlist"
retries=150
set -x
exec 1>> /data/misc/installd/.awpreloaddatalog
exec 2>> /data/misc/installd/.awpreloaddatalog

if [ -d "$TEMP_PATH" ]; then
    echo "preload data from $TEMP_PATH to $DATA_PATH"
    for file in $(find $TEMP_PATH -mindepth 1 -maxdepth 1); do
        dir="$DATA_PATH/$(basename $file)"
        if [ ! -d "$dir" ]; then
            mkdir "$dir"
        fi
        filelist=$(find $file -mindepth 1 -maxdepth 1)
        for f in $filelist; do
            cp -rf "$f" "$dir/"
            rm -rf "$f"
            fname=$(basename $f)
            chown -R system:system "$dir/$fname"
            chmod -R +rwx "$dir/$fname"
            if [ "$dir" == "/data/data" ]; then
                echo "$fname" >> "$RESTORECON_LIST_PATH"
            else
                restorecon -R $dir/$fname
            fi
        done
    done
    rm -rf $TEMP_PATH
fi
