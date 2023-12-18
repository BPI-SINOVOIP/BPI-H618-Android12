
#
#config file for sun8iw21
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

MODULE=nand
CFG_SUNXI_NAND =y
CFG_SUNXI_SPINAND =y


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

