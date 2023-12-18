
# define sources root directory before everything
SRC_ROOT := ../..

# change compiler and linker option before you include rule.mk
#
# link to libscript.a when you need to fetch configuration
# from test_script
#
#CFLAGS := $(CFLAGS) -g
LDFLAGS := -lscript -lpthread -lsalsa

# include rule.mk
include $(SRC_ROOT)/rule.mk

ifeq ($(LICHEE_CHIP),sun50iw12p1)
CFLAGS := $(CFLAGS) -I$(LIBDIR)/salsa-lib-0.0.22/src

CFLAGS := $(CFLAGS) -I./tinyalsa/include

LOCAL_PATH := $(shell pwd)

tinyalsa_objs += \
	./tinyalsa/pcm.o \
	./tinyalsa/mixer.o

audiocodectester_objs += \
	./sun50iw12p1/audiocodec_test.o

i2stester_objs += \
	./sun50iw12p1/daudio_test.o

spdiftester_objs += \
	./sun50iw12p1/spdif_test.o

# add stdc++ lib and math lib
CFLAGS := $(CFLAGS) -lstdc++ -lm

# add your target(s) to all
.PHONY: all
all: audiocodectester i2stester spdiftester install

# define you target, the target will be output to dragonboard/output/bin
# directory
audiocodectester: $(tinyalsa_objs) $(audiocodectester_objs)
	$(LINK_MSG)
	$(LINKX)

i2stester: $(tinyalsa_objs) $(i2stester_objs)
	$(LINK_MSG)
	$(LINKX)

spdiftester: $(tinyalsa_objs) $(spdiftester_objs)
	$(LINK_MSG)
	$(LINKX)

install:
	cp $(LOCAL_PATH)/soundtester.sh $(BINDIR)/

# change 'example_objs' to your objects collection variable
$(tinyalsa_objs): %.o: %.c
	$(COMPILE_MSG)
	$(COMPILEX)

# change 'example_objs' to your objects collection variable
$(audiocodectester_objs): %.o: %.c
	$(COMPILE_MSG)
	$(COMPILEX)

# change 'example_objs' to your objects collection variable
$(i2stester_objs): %.o: %.c
	$(COMPILE_MSG)
	$(COMPILEX)

# change 'example_objs' to your objects collection variable
$(spdiftester_objs): %.o: %.c
	$(COMPILE_MSG)
	$(COMPILEX)
else
all:
	@echo
endif
