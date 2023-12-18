O:= $(CURDIR)/out

TA_DIR := ta
CA_DIR := ca
DEV_KIT_DIR ?= $(CURDIR)/../../dev_kit

export DEV_KIT_DIR
include $(DEV_KIT_DIR)/../platform_config.mk
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
TA_DEV_KIT_DIR ?= $(DEV_KIT_DIR)/arm-plat-$(PLATFORM)/export-ta_arm32
export TA_DEV_KIT_DIR

ifneq ($V,1)
	q := @
	echo := @echo
else
	q :=
	echo := @:
endif
export q

.PHONY: all
all: ta ca demo

.PHONY: demo
demo:ca
	$(q)$(MAKE) demo -C $(CA_DIR) O=$(O)/ca CROSS_COMPILE="$(CROSS_COMPILE_HOST)"

.PHONY: ca
ca:
	$(q)$(MAKE) ca -C $(CA_DIR) O=$(O)/ca CROSS_COMPILE="$(CROSS_COMPILE_HOST)"

.PHONY: ta
ta:
	$(q)$(foreach dir,$(TA_DIR), $(MAKE) -C $(dir) O=$(O)/$(dir) CROSS_COMPILE="$(CROSS_COMPILE_TA)" &&) true

.PHONY: clean
clean:
	$(q)$(foreach dir,$(TA_DIR), $(MAKE) -C $(dir) O=$(O)/$(dir) $@ &&) true
	$(q)$(MAKE) -C $(CA_DIR) O=$(O)/ca $@
	$(q)rm -rf $(O)/ca
	$(q)rm -rf $(O)
