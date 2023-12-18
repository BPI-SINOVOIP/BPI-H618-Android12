OBJTREE		:= $(CURDIR)
SRCTREE		:= $(CURDIR)
TOPDIR		:= $(SRCTREE)
LNDIR		:= $(OBJTREE)
SRCTREE		:= $(CURDIR)
TOPDIR		:= $(SRCTREE)
COMMONDIR   := $(CURDIR)/..

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

objects = update_config_to_img.o firmware/imgdecode.o script/script.o


edit:$(objects)
	gcc -o update_config_to_img $(objects) -static
	rm -rf $(objects)

clean:
	rm -rf update_config_to_img $(objects)