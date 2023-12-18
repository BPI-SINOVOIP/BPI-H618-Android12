VS_TOP_DIR := $(LICHEE_DRAGONBAORD_DIR)/src/testcases/vslibrarybuild
include $(VS_TOP_DIR)/Build/Rule_TV303.make
include $(VS_TOP_DIR)/Build/Makefile_TV303.Inc

PATH_BUILD = $(LICHEE_TOOLCHAIN_PATH)/bin
CC = $(PATH_BUILD)/arm-linux-gnueabi-gcc
LD = $(PATH_BUILD)/arm-linux-gnueabi-ld

ROOTF_PATH=$(LICHEE_DRAGONBAORD_DIR)/rootfs
OBJ_DIR = $(LICHEE_DRAGONBAORD_DIR)/src/testcases/dtmbdemodtester/out
DRAGONBORAD_OUTPUT_BIN_DIR := $(LICHEE_DRAGONBAORD_DIR)/output/bin
INSTALL_DIR = $(OBJ_DIR)/release
TARGET = $(INSTALL_DIR)/ut_demod

TARGET_SOURCES = tc_demod.c \
                 tc_main.c

TARGET_OBJS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(filter %.c, $(TARGET_SOURCES)))

INCLUDES = -I./demod -I./tuner/include -I./demux -I./dvbcore
LIBS    = -lpthread -latbm253 -ltv_frontend -ltuner_base -latbm253 -ltuner_SI2151 -ltuner_R842 \
          -lgcc_s -ldvb_core -ldvb_demod -ldvb_demux -lUtility  \
          -lDemux -lCPUComm -lTestTuner -lTridXMLParser
CFLAGS  = $(INCLUDES) -g -DBUILD_WITH_LINUX -fPIC $(DEF_PROJECT_RULE)
LDFLAGS = -L$(INSTALL_DIR) -L$(ROOTF_PATH)/usr/lib

.PHONY: all clean
ifeq ($(LICHEE_CHIP),sun50iw12p1)
all: $(TARGET) install

$(TARGET): create_obj_dir $(TARGET_OBJS)
	make -C demod
	make -C demux
	make -C dvbcore
	$(CXX) ${CFLAGS} ${TARGET_OBJS}  -o ${TARGET} $(LDFLAGS) $(LIBS)

create_obj_dir:
	@mkdir -p $(INSTALL_DIR)
	echo $(TARGET_OBJS)

install:
	cp $(INSTALL_DIR)/lib* $(ROOTF_PATH)/usr/lib/ -v
	cp $(TARGET) dtmb_tester.sh dvbc_tester.sh $(DRAGONBORAD_OUTPUT_BIN_DIR)

$(patsubst %.c, $(OBJ_DIR)/%.o, $(TARGET_SOURCES)): $(OBJ_DIR)/%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -fr $(TARGET) $(TARGET_OBJS)
	make -C demod clean
	make -C demux clean
	make -C dvbcore clean
	@rm -r out
else
all:
	@echo -e '\033[0;31mSkip build vs modules due to unsupported chip ic[$(LICHEE_CHIP)]...\033[0m'
clean:
	@echo -e '\033[0;31mDo nothing...\033[0m'
endif
