
#
#config file for sun8iw21 fastboot
#
#stroage
FILE_EXIST=$(shell if [ -f $(TOPDIR)/board/$(PLATFORM)/common.mk ]; then echo yes; else echo no; fi;)
EXT_FILE_EXIST=$(shell if [ -f $(TOPDIR)/board/$(PLATFORM)/common$(LICHEE_BOARD).mk ]; then echo yes; else echo no; fi;)
ifeq (x$(EXT_FILE_EXIST),xyes)
include $(TOPDIR)/board/$(PLATFORM)/common$(LICHEE_BOARD).mk
else ifeq (x$(FILE_EXIST),xyes)
include $(TOPDIR)/board/$(PLATFORM)/common.mk
else
include $(TOPDIR)/board/$(CP_BOARD)/common.mk
endif


MODULE=mmcfastboot
CFG_SUNXI_SDMMC =y


CFG_SUNXI_FDT=y

CFG_BOOT0_LOAD_KERNEL=y
CFG_KERNEL_BOOTIMAGE=y
CFG_KERNEL_CHECKSUM=n #y will check kernel checksum in bimage, but slower
CFG_MMC_KERNEL_OFFSET=0x80 #first partition
CFG_MMC_LOGICAL_OFFSET=40960
CFG_KERNEL_LOAD_ADDR=0x40007800
CFG_SUNXI_FDT_ADDR=0x41d00000
CFG_SPINOR_KERNEL_BACKUP_OFFSET=0x1BA0 # Adjusted according to sys_partition file


#IR STATE
CFG_BOOT0_WRITE_IRSATTE_TO_ISP=y
CFG_SUNXI_PHY_KEY=y
CFG_GPADC_KEY=y

CFG_BOOT0_LOAD_FLASH=y
CFG_BOOT0_LOAD_ISPPARM=y
CFG_ISPPARAM_LOAD_ADDR=0x43BFE000
CFG_ISPPARAM_SIZE=0x10    #sector
CFG_MMC_ISPPARAM_OFFSET=13312

CFG_SET_GPIO_NEW=y

#ISP
CFG_BOOT0_WIRTE_RTC_TO_ISP=y
CFG_ISPFLAG_RTC_INDEX=0x1
CFG_ISPFLAG_RTC_VALUE=0x1

CFG_SUNXI_EFUSE =y
CFG_MELISELF_LOAD_ADDR=0x43080000