
# define sources root directory before everything
SRC_ROOT := ../..

# change compiler and linker option before you include rule.mk
#
# link to libscript.a when you need to fetch configuration
# from test_script
#
CFLAGS := $(CFLAGS) -g
CFLAGS += -Wno-packed-bitfield-compat
LDFLAGS := -lscript

# include rule.mk
include $(SRC_ROOT)/rule.mk

# define objects collection variable
all_objs = bootctl.o

# target
target := bootctl

# add your target(s) to all
.PHONY: all
all: $(target)

# define you target, the target will be output to dragonboard/output/bin
# directory
$(target): $(all_objs)
	$(LINK_MSG)
	$(LINKX)

# change 'example_objs' to your objects collection variable
$(all_objs): %.o: %.c
	$(COMPILE_MSG)
	$(COMPILEX)

# clean temp files
clean:
	-rm -f $(target) $(target).d* $(target).o* $(all_objs)

