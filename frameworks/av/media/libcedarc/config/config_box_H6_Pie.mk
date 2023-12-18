############################################
## configurations: BOX-H6-9.0
############################################
include $(MODULE_TOP)/config/config_common.mk

## 1. linux kernel version
LOCAL_CFLAGS += -DCONF_KERNEL_VERSION_4_9

## 2. gpu
LOCAL_CFLAGS += -DCONF_MALI_GPU
#LOCAL_CFLAGS += -DCONF_IMG_GPU

## 3. new display framework
NEW_DISPLAY = yes

## 4. support iommu
##LOCAL_CFLAGS += -DCONF_USE_IOMMU

## 5. now, the function of openmax-di just work well on h6 platform.
LOCAL_CFLAGS += -DCONF_ENABLE_OPENMAX_DI_FUNCTION

## 6. config kernel bitwide value explicitly as 64 FOR DI process.
LOCAL_CFLAGS += -DCONF_KERN_BITWIDE=64

## 7. DI process 3 input pictures. Otherwise, 2 input pictures would be processed anyway.
LOCAL_CFLAGS += -DCONF_DI_PROCESS_3_PICTURE

## 8. for enable afbc
LOCAL_CFLAGS += -DCONF_AFBC_ENABLE

## 9. for the function of VE frequency setup
LOCAL_CFLAGS += -DCONF_VE_FREQ_ENABLE_SETUP

##10. for 10bit and hdr
LOCAL_CFLAGS += -DCONF_HIGH_DYNAMIC_RANGE_ENABLE
