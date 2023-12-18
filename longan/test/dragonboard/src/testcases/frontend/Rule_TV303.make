PATH_BUILD = $(LICHEE_TOOLCHAIN_PATH)/bin
CC = $(PATH_BUILD)/arm-linux-gnueabi-gcc
LD = $(PATH_BUILD)/arm-linux-gnueabi-ld

ROOTF_PATH  = $(LICHEE_DRAGONBAORD_DIR)/rootfs
OBJ_DIR     = $(LICHEE_DRAGONBAORD_DIR)/src/testcases/frontend/out
INSTALL_DIR = $(OBJ_DIR)/release