out-dir := $(CURDIR)/out

DEV_KIT_DIR ?= $(CURDIR)/../../dev_kit
include $(DEV_KIT_DIR)/../platform_config.mk
CA_DEV_KIT_DIR ?= $(DEV_KIT_DIR)/arm-plat-$(PLATFORM)/export-ca
TA_DEV_KIT_DIR ?= $(DEV_KIT_DIR)/arm-plat-$(PLATFORM)/export-ta_arm32

ARM_TOOL_CHAIN_PREFIX ?= $(shell \
	if [ -d "$(DEV_KIT_DIR)/../tools/toolchain/gcc-arm-gnueabi/bin" ]; then \
		echo "$(DEV_KIT_DIR)/../tools/toolchain/gcc-arm-gnueabi/bin/arm-linux-gnueabi-"; \
	elif [ -d "$(DEV_KIT_DIR)/../../out/gcc-linaro-5.3.1-2016.05-x86_64_arm-linux-gnueabi/bin" ]; then \
		echo "$(DEV_KIT_DIR)/../../out/gcc-linaro-5.3.1-2016.05-x86_64_arm-linux-gnueabi/bin/arm-linux-gnueabi-"; \
	else \
		echo "no_avaliable_toolchain_found!  "; \
	fi \
)
ARM64_TOOL_CHAIN_PREFIX ?= $(shell \
	if [ -d "$(DEV_KIT_DIR)/../tools/toolchain/gcc-aarch64/bin" ]; then \
		echo "$(DEV_KIT_DIR)/../tools/toolchain/gcc-aarch64/bin/aarch64-linux-gnu-"; \
	elif [ -d "$(DEV_KIT_DIR)/../../out/gcc-linaro-5.3.1-2016.05-x86_64_aarch64-linux-gnu/bin" ]; then \
		echo "$(DEV_KIT_DIR)/../../out/gcc-linaro-5.3.1-2016.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-"; \
	else \
		echo "no_avaliable_toolchain_found!  "; \
	fi \
)
ifeq ($(ATF_EXIST),y)
CROSS_COMPILE_HOST ?= $(ARM64_TOOL_CHAIN_PREFIX)
CROSS_COMPILE_TA   ?= $(ARM_TOOL_CHAIN_PREFIX)
else
CROSS_COMPILE_HOST ?= $(ARM_TOOL_CHAIN_PREFIX)
CROSS_COMPILE_TA   ?= $(ARM_TOOL_CHAIN_PREFIX)
endif

export CA_DEV_KIT_DIR
export TA_DEV_KIT_DIR

ifneq ($V,1)
	q := @
	echo := @echo
else
	q :=
	echo := @:
endif
# export 'q', used by sub-makefiles.
export q

# If _HOST or _TA specific compilers are not specified, then use CROSS_COMPILE
#CROSS_COMPILE_HOST ?= $(CROSS_COMPILE)
#CROSS_COMPILE_TA ?= $(CROSS_COMPILE)

.PHONY: all
ifneq ($(wildcard $(TA_DEV_KIT_DIR)/host_include/conf.mk),)
all: ca ta
else
all:
	$(q)echo "TA_DEV_KIT_DIR is not correctly defined" && false
endif

.PHONY: ca
ca:
	$(q)$(MAKE) -C ca CROSS_COMPILE="$(CROSS_COMPILE_HOST)" \
			     --no-builtin-variables \
			     O=$(out-dir)/ca

.PHONY: ta
ta:
	$(q)$(MAKE) -C ta CROSS_COMPILE="$(CROSS_COMPILE_TA)" \
			     O=$(out-dir)/ta
			

			
.PHONY: clean
RMDIR := rmdir --ignore-fail-on-non-empty
define rm-build-subdir
echo `[ -d $1 ] && { cd $1; $(RMDIR) $(out-dir)/ta $(out-dir) 2> /dev/null; true; }` > /dev/null
endef

ifneq ($(wildcard $(TA_DEV_KIT_DIR)/host_include/conf.mk),)
clean:
	-$(q)$(MAKE) -C ca O=$(out-dir)/ca $@
	-$(q)$(MAKE) -C ta O=$(out-dir)/ta $@
	-$(q)$(call rm-build-subdir,ta)
else
clean:
	$(q)echo "TA_DEV_KIT_DIR is not correctly defined"
	$(q)echo "You can remove manually $(out-dir)"
endif
