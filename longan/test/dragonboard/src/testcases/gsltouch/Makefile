
# define sources root directory before everything
SRC_ROOT := ../..

# include rule.mk
include $(SRC_ROOT)/rule.mk

# add you test case here
.PHONY: all
all:
	mkdir -p $(FIRMWARE)/gsl_firmware
	cp -rax firmware/* $(FIRMWARE)/gsl_firmware
