# This file is for copy fastboot related binaries and libs to vendor ramdisk.
# Why doing this? We have to support GKI, but GKI does not include fastbootd.
# So we have to copy fastboot related files to vendor ramdisk, then  we could
# use fastboot after gki is flashed to boot partiton, otherwise we won't be 
# able to get vts done(it breaks at fastboot test for fastboot not connected).
# This is a temporary solution. When build system support specify module to be
# copied to vendor ramdisk, then we should use build paramerter instead of this.

PRODUCT_COPY_FILES += $(call find-copy-subdir-files,*,$(LOCAL_PATH)/system/,$(TARGET_COPY_OUT_VENDOR_RAMDISK)/system/)
