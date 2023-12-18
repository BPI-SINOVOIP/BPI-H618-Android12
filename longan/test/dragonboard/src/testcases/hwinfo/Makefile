
# define sources root directory before everything
SRC_ROOT := ../..

# change compiler and linker option before you include rule.mk
#
# link to libscript.a when you need to fetch configuration
# from test_script
#
#CFLAGS := $(CFLAGS) -g
CFLAGS = -I. -Ilib -Wall -Wno-strict-aliasing
LDFLAGS := -lscript -lrt -lpthread
CFLAGS  += -DWIFI_USE_RTL8723BS_VQ0
CFLAGS  += -DPROP_PREFIX=\"persist.log.tag.\"

# include rule.mk
include $(SRC_ROOT)/rule.mk

# define objects collection variable
lib = \
	minIni/dev/minIni.o \
	utils.o

wl_info_objs = \
	utils.o \
	libhwinfo.o \
	hwinfo.o

# add your target(s) to all
.PHONY: all
all: wl_info

# define you target, the target will be output to dragonboard/output/bin
# directory
wl_info: $(wl_info_objs) $(lib)
	$(LINK_MSG)
	$(LINKX)

# change objs to your objects collection variable
$(wl_info_objs): %.o: %.c
	$(COMPILE_MSG)
	$(COMPILEX)

# clean temp files
clean:
	-rm -rf $(wl_info_objs) $(wl_info_objs:.o=.d)
	-rm -rf $(lib) $(lib:.o=.d)

