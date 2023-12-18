# This is to speed up presubmit of media related tests by asking emulator
# to run the media.adb script upon boot complete to copy those media files
# to sdcard/test

PRODUCT_COPY_FILES += \
    device/generic/goldfish/data/media/test/media.adb:data/adbscripts/media.adb

PRODUCT_COPY_FILES += \
    device/generic/goldfish/data/media/test/swirl_136x144_mpeg4.mp4:data/media/0/test/CtsMediaTestCases-1.4/swirl_136x144_mpeg4.mp4 \
    device/generic/goldfish/data/media/test/swirl_132x130_mpeg4.mp4:data/media/0/test/CtsMediaTestCases-1.4/swirl_132x130_mpeg4.mp4 \
    device/generic/goldfish/data/media/test/swirl_130x132_mpeg4.mp4:data/media/0/test/CtsMediaTestCases-1.4/swirl_130x132_mpeg4.mp4 \
    device/generic/goldfish/data/media/test/swirl_144x136_mpeg4.mp4:data/media/0/test/CtsMediaTestCases-1.4/swirl_144x136_mpeg4.mp4 \
    device/generic/goldfish/data/media/test/swirl_128x128_mpeg4.mp4:data/media/0/test/CtsMediaTestCases-1.4/swirl_128x128_mpeg4.mp4


PRODUCT_COPY_FILES += \
    device/generic/goldfish/data/etc/empty_data_disk:data/empty_data_disk

