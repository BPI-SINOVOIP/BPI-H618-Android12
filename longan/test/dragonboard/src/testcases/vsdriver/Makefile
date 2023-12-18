
# define sources root directory before everything
SRC_ROOT := ../..

LDFLAGS := -lscript

# include rule.mk
include $(SRC_ROOT)/rule.mk

# add your target(s) to all

.PHONY: all

ifeq ($(LICHEE_CHIP),sun50iw12p1)
LOCAL_PATH   := $(shell pwd)
VS_BUILD_OUT := $(LICHEE_PLAT_OUT)/vs
SRC_PATH     := $(VS_BUILD_OUT)/vs-modules
ROOTF_PATH   := $(LICHEE_DRAGONBAORD_DIR)/rootfs
CROSS_COMPILE:= $(LICHEE_TOOLCHAIN_PATH)/bin/$(LICHEE_CROSS_COMPILER)

all: mipsloader vsdriver

#make mipsloader
mipsloader:
	@if [ -d $(LICHEE_PLATFORM_DIR)/framework/display/mipsloader ]; then \
		mkdir -p $(ROOTF_PATH)/mips/; \
		rm -rf $(VS_BUILD_OUT)/libsec_storage; \
		cp -rf $(LICHEE_PLATFORM_DIR)/base/libsec_storage $(VS_BUILD_OUT)/; \
		cd $(VS_BUILD_OUT)/libsec_storage/src && make CC=$(CROSS_COMPILE)-gcc; \
		cp -rf $(VS_BUILD_OUT)/libsec_storage/src/libsst.so $(ROOTF_PATH)/usr/lib/; \
		rm -rf $(VS_BUILD_OUT)/mipsloader; \
		cp -rf $(LICHEE_PLATFORM_DIR)/framework/display/mipsloader $(VS_BUILD_OUT)/; \
		cd $(VS_BUILD_OUT)/mipsloader; \
		make toolprefix=$(CROSS_COMPILE) STATIC_EN=y \
				SECURE_STORAGE_EN=n \
				SECURE_STORAGE_LIB_PATH=$(VS_BUILD_OUT)/libsec_storage/src \
				SECURE_STORAGE_INC_PATH=$(VS_BUILD_OUT)/libsec_storage/src/include; \
		cp -rf $(VS_BUILD_OUT)/mipsloader/loadmips $(ROOTF_PATH)/mips/; \
	else \
		mkdir -p $(ROOTF_PATH)/mips/; \
		cp -rf $(LOCAL_PATH)/prebuilt/mips/loadmips $(ROOTF_PATH)/mips/; \
	fi

#get vs driver path
SRC_COPY_PATH:= $(shell \
				if [ -d $(LICHEE_TOP_DIR)/../vendor/vs ]; then \
					readlink -f $(LICHEE_TOP_DIR)/../vendor/vs; \
				elif [ -d $(LICHEE_PLATFORM_DIR)/vendor/vs ]; then \
					readlink -f $(LICHEE_PLATFORM_DIR)/vendor/vs; \
				fi)

# make vs driver
vsdriver:
ifeq ($(SRC_COPY_PATH),)
	@echo -e '\033[0;31mNo vs source code found, skip build...\033[0m'
else
	# copy vs driver file all to dragonboard/buildroot/build/vs-modules
	rm -rf $(VS_BUILD_OUT)/vs-modules
	mkdir -p $(VS_BUILD_OUT)/vs-modules
	cp -rf $(SRC_COPY_PATH)/* $(SRC_PATH)
	cp -rf ./build/* $(SRC_PATH)
	# build vs drivers
ifeq ($(LICHEE_ARCH),arm64)
	sed -i 's|^\s*CONFIG_64BIT_KO.*$$|CONFIG_64BIT_KO := 1|g' $(VS_BUILD_OUT)/vs-modules/Install/Core/Makefile.Inc
else
	sed -i 's|^\s*CONFIG_64BIT_KO.*$$|CONFIG_64BIT_KO := 0|g' $(VS_BUILD_OUT)/vs-modules/Install/Core/Makefile.Inc
endif
	find $(VS_BUILD_OUT)/vs-modules/ -name Android.bp -o -name Android.mk -o -name CleanSpec.mk | xargs rm -rf
	cd $(VS_BUILD_OUT)/vs-modules && ./build.sh
	# copy file to dragonboard rootf
	cp -rf $(VS_BUILD_OUT)/vs-modules/Install/Core/Kernel_Driver $(ROOTF_PATH)/lib/
	cp -rf $(VS_BUILD_OUT)/vs-modules/Install/Core/Lib/* $(ROOTF_PATH)/usr/lib/
	mkdir -p $(ROOTF_PATH)/etc/Dispmips_vs_bin
	cp -rf $(VS_BUILD_OUT)/vs-modules/Dispmips_vs_bin/*.TSE $(ROOTF_PATH)/etc/Dispmips_vs_bin
	cp -rf $(VS_BUILD_OUT)/vs-modules/Dispmips_vs_bin/*.bin $(ROOTF_PATH)/etc/Dispmips_vs_bin
	cp -rf $(VS_BUILD_OUT)/vs-modules/Dispmips_vs_bin/*.xml $(ROOTF_PATH)/etc/Dispmips_vs_bin
	cp $(LOCAL_PATH)/vsdriver.sh $(BINDIR)/
endif

else

all:
	@echo -e '\033[0;31mSkip build vs modules dure to unsupported chip ic[$(LICHEE_CHIP)]...\033[0m'

endif
