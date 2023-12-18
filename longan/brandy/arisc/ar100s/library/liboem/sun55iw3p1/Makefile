ifneq ($(wildcard $(dir)/arisc-private/Makefile),)
$(info use private dram source)
m := $(shell rm -rf $(dir)/libar100s.a)
m := $(shell : > .lib)
obj-y += arisc-private/

__build:$(dir)/libar100s.a

else ifneq ($(wildcard $(LICHEE_DRAMLIB_PATH)/sun55iw3p1/),)
$(info use external dramlib from $(LICHEE_DRAMLIB_PATH))
m := $(shell rm -rf $(dir)/libar100s.a)
m := $(shell cp $(LICHEE_DRAMLIB_PATH)/sun55iw3p1/arisc_liboem/libar100s.a $(dir)/libar100s.a)
m := $(shell echo -L$(dir) -lar100s > .lib)
else
$(info use locallib only for arisc developer)
m := $(shell echo -L$(dir) -lar100s > .lib)
endif

$(dir)/libar100s.a:$(in-target)
	$(Q)$(AR) rcs $@ $(in-target)

