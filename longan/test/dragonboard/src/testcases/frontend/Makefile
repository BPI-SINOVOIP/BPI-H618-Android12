include $(LICHEE_DRAGONBAORD_DIR)/src/testcases/frontend/Rule_TV303.make

.PHONY: all clean
ifeq ($(LICHEE_CHIP),sun50iw12p1)
all: create_obj_dir
	make -C tuner/base
	make -C tuner/tuner_driver/atbm/ATBM253SDK
	make -C tuner/tuner_driver/Rafael_Micro/R842
	make -C tuner/tuner_driver/Silicon_Laboratories/Si2151
	make -C tuner
	cp $(INSTALL_DIR)/lib* $(ROOTF_PATH)/usr/lib/ -v

create_obj_dir:
	@mkdir -p $(OBJ_DIR) $(INSTALL_DIR)

clean:
	make -C tuner/base clean
	make -C tuner/tuner_driver/atbm/ATBM253SDK clean
	make -C tuner/tuner_driver/Rafael_Micro/R842 clean
	make -C tuner/tuner_driver/Silicon_Laboratories/Si2151 clean
	make -C tuner clean
	@rm  -r $(OBJ_DIR)
else
all:
	@echo -e '\033[0;31mSkip build tuner modules due to unsupported chip ic[$(LICHEE_CHIP)]...\033[0m'
clean:
	@echo -e '\033[0;31mDo nothing...\033[0m'
endif
