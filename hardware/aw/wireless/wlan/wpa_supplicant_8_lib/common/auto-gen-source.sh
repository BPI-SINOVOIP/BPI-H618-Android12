#!/usr/bin/env bash

allargs=(${@})
target=${allargs[-1]}
vendorlist=${allargs[@]::${#allargs[@]}-1}

echo target: $target

mkdir -p $(dirname $target)
echo "/* Auto-generated */"    > $target
echo "/* target: $target */"  >> $target
echo "/* date  : $(date) */"  >> $target
echo ""                       >> $target
echo "#include <stdio.h>"     >> $target
echo "#include <string.h>"    >> $target
echo "#include <stdlib.h>"    >> $target
echo "#include \"common.h\""  >> $target
echo "#include \"type.h\""    >> $target
echo ""                       >> $target

if [ "x$(basename $target)" == "xdriver_cmd_nl80211_stub.c" ]; then

for v in ${vendorlist[@]}; do
    echo "extern struct driver_nl80211_cb_info driver_nl80211_cb_info_${v};"  >> $target
done

echo ""                                                                       >> $target
echo "struct driver_nl80211_cb_info *driver_nl80211_cb_info_list[] = {"       >> $target
for v in ${vendorlist[@]}; do
    echo -e "\t&driver_nl80211_cb_info_${v},"                                 >> $target
done
echo "};"                                                                     >> $target

echo ""                                                                       >> $target
echo "uint32_t driver_nl80211_cb_info_count = sizeof(driver_nl80211_cb_info_list) / sizeof(driver_nl80211_cb_info_list[0]);" >> $target

elif [ "x$(basename $target)" == "xdriver_cmd_wext_stub.c" ]; then

for v in ${vendorlist[@]}; do
    echo "extern struct driver_wext_cb_info driver_wext_cb_info_${v};"        >> $target
done

echo ""                                                                       >> $target
echo "struct driver_wext_cb_info *driver_wext_cb_info_list[] = {"             >> $target
for v in ${vendorlist[@]}; do
    echo -e "\t&driver_nl80211_cb_info_${v},"                                 >> $target
done
echo "};"                                                                     >> $target

echo ""                                                                       >> $target
echo "uint32_t driver_wext_cb_info_count = sizeof(driver_wext_cb_info_list) / sizeof(driver_wext_cb_info_list[0]);" >> $target

fi
