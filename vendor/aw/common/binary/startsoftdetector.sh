#!/system/bin/sh

echo "start softdetector probe service"

dataFile="/data/data/com.cmcc.mid.softdetector/lib/libpcapcmcc.so"
systemFile="/system/bin/libpcapcmcc.so"

echo "while"

while [ ! -f $dataFile ]&&[ ! -f $systemFile ]
do
    echo "in while"
    sleep 3
done

echo "if"
sleep 1
if [ -f $dataFile ]; then
    echo "dataFile excute"
    /data/data/oom.cmcc.mid.softdetector/lib/libpcapcmcc.so
else
    echo "systemFile excute"
    /system/bin/libpcapcmcc.so
fi