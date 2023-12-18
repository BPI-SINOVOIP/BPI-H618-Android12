
# define sources root directory before everything
SRC_ROOT := ../..

# change compiler and linker option before you include rule.mk
#
# link to libscript.a when you need to fetch configuration
# from test_script
#
#CFLAGS := $(CFLAGS) -g
LDFLAGS := -lscript -lm

# include rule.mk
include $(SRC_ROOT)/rule.mk

# define objects collection variable
hallsensortester_objs = hallsensortester.o

.PHONY: all
all: hallsensortester
	cp hallsensor.sh $(BINDIR)/

# define you target, the target will be output to dragonboard/output/bin
# directory
hallsensortester: $(hallsensortester_objs)
	$(LINK_MSG)
	$(LINKX)

# change 'hallsensortester_objs' to your objects collection variable
$(hallsensortester_objs): %.o: %.c
	$(COMPILE_MSG)
	$(COMPILEX)

