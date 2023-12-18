#
#config file for sun8iw19
#

FILE_EXIST=$(shell if [ -f $(TOPDIR)/board/$(PLATFORM)/common.mk ]; then echo yes; else echo no; fi;)
EXT_FILE_EXIST=$(shell if [ -f $(TOPDIR)/board/$(PLATFORM)/common$(LICHEE_BOARD).mk ]; then echo yes; else echo no; fi;)
ifeq (x$(EXT_FILE_EXIST),xyes)
include $(TOPDIR)/board/$(PLATFORM)/common$(LICHEE_BOARD).mk
else ifeq (x$(FILE_EXIST),xyes)
include $(TOPDIR)/board/$(PLATFORM)/common.mk
else
include $(TOPDIR)/board/$(CP_BOARD)/common.mk
endif

MODULE=spinor
CFG_SUNXI_SPINOR =y
CFG_SUNXI_SPI =y
CFG_SUNXI_SPIF =y
CFG_SUNXI_DMA =y
CFG_SPI_USE_DMA =y
CFG_SPINOR_UBOOT_OFFSET=128

CFG_BOOT0_LOAD_KERNEL=y
CFG_KERNEL_BOOTIMAGE=y
CFG_KERNEL_CHECKSUM=n #y will check kernel checksum in bimage, but slower
CFG_SPINOR_KERNEL_OFFSET=0x20 #first partition, 0x20  4064+0x20 = 4096 sector = 2.0M
CFG_SPINOR_LOGICAL_OFFSET=4064
CFG_KERNEL_LOAD_ADDR=0x40007800
CFG_SUNXI_FDT_ADDR=0x41d00000

CFG_BOOT0_LOAD_FLASH=y
CFG_BOOT0_LOAD_ISPPARM=y
CFG_ISPPARAM_LOAD_ADDR=0x43BFFE00
CFG_ISPPARAM_SIZE=0x8    #byte
CFG_SPINOR_ISPPARAM_OFFSET=CFG_SPINOR_UBOOT_OFFSET - 16
