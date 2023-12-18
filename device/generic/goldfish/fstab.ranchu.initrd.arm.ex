# Android fstab file.
#<dev>  <mnt_point> <type>  <mnt_flags options> <fs_mgr_flags>
system   /system     ext4    ro,barrier=1     wait,logical,first_stage_mount
vendor   /vendor     ext4    ro,barrier=1     wait,logical,first_stage_mount
product  /product    ext4    ro,barrier=1     wait,logical,first_stage_mount
system_ext  /system_ext  ext4   ro,barrier=1   wait,logical,first_stage_mount
/dev/block/platform/a003c00.virtio_mmio/by-name/metadata    /metadata    ext4    noatime,nosuid,nodev    wait,formattable,first_stage_mount
