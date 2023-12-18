
#
#config file for sun55iw3
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

MODULE=mmc
CFG_SUNXI_SDMMC =y



