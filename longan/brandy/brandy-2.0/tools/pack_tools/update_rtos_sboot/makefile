
##
## Makefile for Sunxi Secure Boot
##


MAJOR = 2014
MINOR = 05
SUBLEVEL =
EXTRAVERSION = -rc1
SECURE_BOOT_VERSION = $(MAJOR).$(MINOR).$(SUBLEVEL)$(EXTRAVERSION)

OBJTREE		:= $(CURDIR)
SRCTREE		:= $(CURDIR)
TOPDIR		:= $(SRCTREE)
LNDIR		:= $(OBJTREE)
SRCTREE		:= $(CURDIR)
TOPDIR		:= $(SRCTREE)
COMMONDIR   := $(CURDIR)/..
SBROMDIR    := $(CURDIR)/../..
TOOLSDIR    := $(CURDIR)/../../..


export	TOPDIR SRCTREE OBJTREE TOOLSDIR COMMONDIR

ifneq ($(OBJTREE),$(SRCTREE))
obj := $(OBJTREE)/
src := $(SRCTREE)/
else
obj :=
src :=
endif
export obj src

include $(TOPDIR)/config.mk

#
# We let the C compiler driver to take care of .s files. This is done in
# order to be excused from maintaining a separate set of architecture
# dependent assembler flags.
#

OBJS =

LIBS  = main/libmain.o
LIBS += $(COMMONDIR)/common/libcommon.o

LIBS := $(addprefix $(obj),$(sort $(LIBS)))
.PHONY : $(LIBS)

ALL-y += $(obj)update_rtos_sboot

all: $(ALL-y)

$(obj)update_rtos_sboot:  $(LIBS)
	$(CC) $(LIBS) $(LDFLAGS)  -static -o update_rtos_sboot
	@cp -v update_rtos_sboot $(TOOLSDIR)/../tools/pack-bintools/src/update_rtos_sboot

$(LIBS):
		@$(MAKE) -C $(dir $(subst $(obj),,$@))

clean:
	@find $(OBJTREE) $(COMMONDIR)/common/ -type f \
		\( -name '*.depend' -o 	\
		   -name 'core'     -o 	\
		   -name '*.bak'    -o 	\
		   -name '*~'       -o  \
		   -name '*.o'	    -o  \
		   -name '*.exe'	\) -print \
		| xargs rm -f
	@rm -f update_rtos_sboot

# DO NOT DELETE THIS LINE -- make depend depends on it.
