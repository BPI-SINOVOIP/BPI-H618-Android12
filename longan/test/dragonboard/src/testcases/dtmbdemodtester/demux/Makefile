VS_TOP_DIR := $(LICHEE_DRAGONBAORD_DIR)/src/testcases/vslibrarybuild
include $(VS_TOP_DIR)/Build/Rule_TV303.make
include $(VS_TOP_DIR)/Build/Makefile_TV303.Inc

PATH_BUILD = $(LICHEE_TOOLCHAIN_PATH)/bin
CC = $(PATH_BUILD)/arm-linux-gnueabi-gcc
LD = $(PATH_BUILD)/arm-linux-gnueabi-ld

ROOTF_PATH=$(LICHEE_DRAGONBAORD_DIR)/rootfs
OBJ_DIR = $(LICHEE_DRAGONBAORD_DIR)/src/testcases/dtmbdemodtester/out/demux
INSTALL_DIR = $(OBJ_DIR)/../release
TARGET_LIB = $(INSTALL_DIR)/libdvb_demux.so

TARGET_SOURCES = $(wildcard *.c) \
                 $(wildcard program_manager/*.c) \
				 $(wildcard tsdemux_vs/*.c)

TARGET_OBJS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(filter %.c, $(TARGET_SOURCES)))

VS_INC = -I$(VS_ROOT)/HAL_SX6/Demux/Inc \
         -I$(VS_ROOT)/HAL_SX6/CpuComm/Inc \
		 -I$(VS_ROOT)/HAL_SX6/Kernel_Driver/cpu_comm \
		 -I$(VS_ROOT)/HAL_SX6/Kernel_Driver/Inc \
		 -I$(VS_ROOT)/Inc \
		 -I$(VS_ROOT)/Utility/TridUtil/Inc \

INCLUDES = -I./ \
           -I../demux/program_manager \
           -I../demod \
		   -I../demux \
		   -I../dvbcore \
		   -I../demux/tsdemux_vs \
		   $(VS_INC)

LIBS     = -lpthread  -lgcc_s -ldl -lUtility -lDemux -lCPUComm
CFLAGS  += $(INCLUDES) -g -fPIC -DBUILD_WITH_LINUX $(DEF_PROJECT_RULE) -Wno-unused-but-set-variable -Wno-unused-variable -Wno-unused-function
LDFLAGS += -L$(INSTALL_DIR) -L$(ROOTF_PATH)/usr/lib

all: $(TARGET_LIB)

$(TARGET_LIB): create_obj_dir $(TARGET_OBJS)
	$(LD) -shared $(TARGET_OBJS) -o $@

create_obj_dir:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(OBJ_DIR)/program_manager
	@mkdir -p $(OBJ_DIR)/tsdemux_vs

$(patsubst %.c, $(OBJ_DIR)/%.o, $(TARGET_SOURCES)): $(OBJ_DIR)/%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -fr $(TARGET_LIB) $(TARGET_OBJS)
	@rm -r $(OBJ_DIR)/program_manager $(OBJ_DIR)/tsdemux_vs $(OBJ_DIR)