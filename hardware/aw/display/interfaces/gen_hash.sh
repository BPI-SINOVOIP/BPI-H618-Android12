#!/bin/bash


echo -e "Generating hidl hash for vendor.display.config\n"

hidl-gen -L hash -r vendor.display:hardware/aw/display/interfaces -r android.hardware:hardware/interfaces -r android.hidl:system/libhidl/transport vendor.display.config@1.0 > current.txt
if [ $? -eq 0 ]; then
    echo -e "Hidl hash for vendor.display.config has been updated\n"
    echo -e "cat current.txt\n"
    cat current.txt
else
    echo -e "Generate hidl hash for vendor.display.config fail\n"
fi
