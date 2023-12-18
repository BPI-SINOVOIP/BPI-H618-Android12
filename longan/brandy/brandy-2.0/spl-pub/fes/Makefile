#
# (C) Copyright 2000-2011
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# (C) Copyright 2011
# Daniel Schwierzeck, daniel.schwierzeck@googlemail.com.
#
# (C) Copyright 2011
# Texas Instruments Incorporated - http://www.ti.com/
# Aneesh V <aneesh@ti.com>
#
# See the file COPYING in the root directory of the source tree for details.
#
# Based on top-level Makefile.
#

SKIP_AUTO_CONF:=yes
include $(TOPDIR)/mk/config.mk
include $(TOPDIR)/.module.common.mk
include $(TOPDIR)/board/$(CP_BOARD)/$(MAKECMDGOALS).mk


#This for Tina system, to copy bin to tina SDK
ifeq ($(TARGET_BUILD_VARIANT),tina)
tina_path=$(TOPDIR)/../../../target/allwinner/$(TARGET_PLATFORM)-common/bin
exist = $(shell if [ -d $(tina_path) ]; then echo "exist"; else echo "notexist"; fi;)
ifeq ($(exist),exist)
TARGETDIR=$(TOPDIR)/../../../target/allwinner/$(TARGET_PLATFORM)-common/bin
endif

tina_path=$(TOPDIR)/../../../device/config/chips/$(TARGET_PLATFORM)/bin
exist = $(shell if [ -d $(tina_path) ]; then echo "exist"; else echo "notexist"; fi;)
ifeq ($(exist),exist)
TARGETDIR=$(TOPDIR)/../../../device/config/chips/$(TARGET_PLATFORM)/bin
endif

else
#TOOLS_DIR  := $(TOPDIR)/../../../device/config
#TARGETDIR=$(TOPDIR)/../../../device/config/chips/$(PLATFORM)/bin
ifneq ($(LICHEE_BUSSINESS), )
       TARGETDIR=$(LICHEE_CHIP_CONFIG_DIR)/$(LICHEE_BUSSINESS)/bin
else
       TARGETDIR=$(LICHEE_CHIP_CONFIG_DIR)/bin
endif
endif
CPUDIR := $(TOPDIR)/arch/$(ARCH)/cpu/$(CPU)
FES_LDSCRIPT := $(CPUDIR)/fes1.lds

# We want the final binaries in this directory
FES1_DIR := $(TOPDIR)/fes/
obj := $(TOPDIR)/fes/

ifdef DRAM_TYPE
	DRAM_TYPE_NAME=_$(DRAM_TYPE)
endif


LIBS-y += arch/$(ARCH)/cpu/$(CPU)/libarch.o
LIBS-y += fes/main/libmain.o
LIBS := $(addprefix $(TOPDIR)/,$(sort $(LIBS-y)))

A-LIBS-$(CFG_SUNXI_FES)  := $(TOPDIR)/board/$(CP_BOARD)/lib$(PLATFORM)_fes.a

A-OBJS := $(A-LIBS-y:%.a=%.o)

ifdef CFG_WEAK_SYMBOL
COBJS-y += $(A-OBJS)
else
COBJS-y += $(A-LIBS-y)
endif
COBJS-y += $(TOPDIR)/fes/main/fes1_main.o

OBJS = $(sort $(COBJS-y))

# Special flags for CPP when processing the linker script.
# Pass the version down so we can handle backwards compatibility
# on the fly.
LDPPFLAGS += \
	-DFES1ADDR=$(CFG_FES1_RUN_ADDR)	 \
	-DCPUDIR=$(CPUDIR) \
	$(shell $(LD) --version | \
	  sed -ne 's/GNU ld version \([0-9][0-9]*\)\.\([0-9][0-9]*\).*/-DLD_MAJOR=\1 -DLD_MINOR=\2/p')


fes: $(LIBS) fes1.lds
ifdef CFG_WEAK_SYMBOL
	$(OBJCOPY) $(CFG_WEAK_SYMBOL) $(A-LIBS-y) $(A-OBJS)
endif
	$(LD) $(OBJS) $(PLATFORM_LIBGCC) $(LDFLAGS) $(LDFLAGS_GC) -Tfes1.lds -o fes1.elf -Map fes1.map
	$(OBJCOPY) $(OBJCFLAGS) -O binary fes1.elf fes1.bin
	$(Q)$(TOPDIR)/mk/gen_check_sum fes1.bin fes1_$(PLATFORM)$(DRAM_TYPE_NAME).bin > /dev/null
	$(Q)-cp -v fes1_$(PLATFORM)$(DRAM_TYPE_NAME).bin $(TARGETDIR)/
	$(Q)-cp -v fes1_$(PLATFORM)$(DRAM_TYPE_NAME).bin $(LICHEE_PLAT_OUT)/


$(LIBS): depend
	$(MAKE) -C $(dir $@)


fes1.lds: $(FES_LDSCRIPT)
	$(Q)$(CPP) $(ALL_CFLAGS) $(LDPPFLAGS) -ansi -D__ASSEMBLY__ -P - <$^ >$@


sinclude $(TOPDIR)/mk/target_for_conf.mk
depend: .depend build-confs
#########################################################################

# defines $(NBOOT_DIR).depend target
include $(TOPDIR)/mk/rules.mk

sinclude .depend

#########################################################################
