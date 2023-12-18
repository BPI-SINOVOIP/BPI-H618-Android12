#! /vendor/bin/sh

# Initialize Edge Sense.
/vendor/bin/init.edge_sense.sh

setprop vendor.qcom.devup 1

version=`grep -ao "OEM_IMAGE_VERSION_STRING[ -~]*" \
              /vendor/firmware/adsp.b04 | \
         sed -e s/OEM_IMAGE_VERSION_STRING=ADSP.version.// -e s/\(.*\).//`
setprop vendor.sys.adsp.firmware.version "$version"
