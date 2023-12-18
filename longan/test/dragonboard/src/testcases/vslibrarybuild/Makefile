# build vslibrary
ROOTF_PATH=$(LICHEE_DRAGONBAORD_DIR)/rootfs
TMP_PATH=$(shell pwd)

.PHONY: all clean
ifeq ($(LICHEE_CHIP),sun50iw12p1)
all:
	make -C HAL_SX6/TridUtil
	make -C HAL_SX6/TridXMLParser
	make -C HAL_SX6/CPUComm
	make -C HAL_SX6/Demux
	#make -C HAL_SX6/Display
	make -C DRX_Fusion
	cp lib/*.so $(ROOTF_PATH)/usr/lib/ -v

clean:
	make -C HAL_SX6/TridUtil clean
	make -C HAL_SX6/TridXMLParser clean
	make -C HAL_SX6/CPUComm clean
	make -C HAL_SX6/Demux clean
	#make -C HAL_SX6/Display clean
	make -C DRX_Fusion clean
else
all:
	@echo -e '\033[0;31mSkip build vs modules due to unsupported chip ic[$(LICHEE_CHIP)]...\033[0m'
clean:
	@echo -e '\033[0;31mDo nothing...\033[0m'
endif
