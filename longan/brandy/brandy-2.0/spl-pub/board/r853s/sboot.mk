#
#config file for sun8iw21
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

MODULE=sboot
CFG_SUNXI_MMC =y
CFG_SUNXI_NAND =y
CFG_SUNXI_SPINAND =y
CFG_SUNXI_CE_23 =y
CFG_SUNXI_SMC_11 =y

CFG_SUNXI_SBOOT =y
CFG_SUNXI_ITEM_HASH =y

CFG_SUNXI_JTAG_DISABLE =y
CFG_SUNXI_VERIFY_MALLCO_SIZE = 4096


#power
CFG_SUNXI_POWER=y
CFG_SUNXI_TWI=y
CFG_SUNXI_AUTO_TWI=y
CFG_SUNXI_TWI_BIAS_1V8=y
CFG_SUNXI_PMIC=y
CFG_AXP2101_POWER=y

#chipid
CFG_SUNXI_CHIPID=y
CFG_SUNXI_EFUSE =y

#standby
CFG_SUNXI_STANDBY=y
CFG_SUNXI_WATCHDOG=y

