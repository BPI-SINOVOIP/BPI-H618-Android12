#
# Makefile to build libdrxsx7.so
#
#Directory definition
TOP_DIR := $(LICHEE_DRAGONBAORD_DIR)/src/testcases/vslibrarybuild
include $(TOP_DIR)/Build/Rule_TV303.make
include $(TOP_DIR)/Build/Makefile_TV303.Inc

DRX_FUSION_DIR = $(VS_ROOT)/Application/User_Driver/Tuner/DRX_Fusion
OBJ_DIR := Obj
VSLIBRARY_DIR = $(TOP_DIR)/lib
ROOTF_PATH=$(LICHEE_DRAGONBAORD_DIR)/rootfs
DESTLIB2 = $(VSLIBRARY_DIR)/libdrxsx7.so

ifneq (,$(filter -DTV303, $(DEF_PROJECT_RULE)))
CFLAGS   += -DSOC_IHBB_REG_BASE=0x06140000 -DDRXFSN_REG_BASE__A=0x06100000 -DDRXBSP_HST_TIME_SCALER=0
CXXFLAGS += -DSOC_IHBB_REG_BASE=0x06140000 -DDRXFSN_REG_BASE__A=0x06100000 -DDRXBSP_HST_TIME_SCALER=0
else
CFLAGS   += -DSOC_IHBB_REG_BASE=0xF5032000 -DDRXFSN_REG_BASE__A=0xF500EB00 -DDRXBSP_HST_TIME_SCALER=0
CXXFLAGS += -DSOC_IHBB_REG_BASE=0xF5032000 -DDRXFSN_REG_BASE__A=0xF500EB00 -DDRXBSP_HST_TIME_SCALER=0
endif

#check special project settings
#ATBM253 source
ATBM253_FOLDER  = RF/AltoBeam_ATBM253
ATBM253_SOURCES = $(ATBM253_FOLDER)/tuneratbm253_AW.c \
				  $(ATBM253_FOLDER)/ATBM253Driver.c \
				  $(ATBM253_FOLDER)/ATBM253Api.c \
				  $(ATBM253_FOLDER)/ATBM253Porting.c

#si2151 source
Si2151_FOLDER  = RF/SiliconLab_Si2151
Si2151_SOURCES = $(Si2151_FOLDER)/Si2151_Properties_Strings.c \
				 $(Si2151_FOLDER)/Si2151_L2_API.c \
				 $(Si2151_FOLDER)/Si2151_L1_Properties.c \
				 $(Si2151_FOLDER)/Silabs_L0_Connection.c \
				 $(Si2151_FOLDER)/Si2151_User_Properties.c \
				 $(Si2151_FOLDER)/Si2151_L1_API.c \
				 $(Si2151_FOLDER)/Si2151_L3_Test.c \
				 $(Si2151_FOLDER)/Si2151_L1_Commands.c \
				 $(Si2151_FOLDER)/platform.c \
				 $(Si2151_FOLDER)/i2c.c	\
				 $(Si2151_FOLDER)/Si2151_Hal.c \
				 $(Si2151_FOLDER)/tunerSI2151_AW.c

INCLUDES =      -I$(VS_ROOT)/Inc/ \
                -I$(VS_ROOT)/HAL_SX6/Kernel_Driver \
                -I$(VS_ROOT)/Utility/TridUtil \
                -I$(VS_ROOT)/Utility/TridUtil/Inc \
                -I$(VS_ROOT)/Application/Inc \
                -I$(DRX_FUSION_DIR)/Inc \
                -I$(DRX_FUSION_DIR)/Inc/RF \
                -I$(DRX_FUSION_DIR)/Inc/RF/AltoBeam_ATBM253 \
                -I$(DRX_FUSION_DIR)/Inc/RF/SiliconLab_Si2151 \
				-I$(LICHEE_DRAGONBAORD_DIR)/src/testcases/frontend/tuner/include

#compile flags
CFLAGS   += $(INCLUDES) -DTRID_BSP -DUSE_PROCESS_MEMORY_MAP $(DEF_TUNER_RULE) -DDRXDRIVER_DEBUG=1 -DBSP_I2C_USE_SOC=1 $(DEF_PROJECT_RULE) -DBUILD_WITH_LINUX
CXXFLAGS += $(INCLUDES) -DTRID_BSP -DUSE_PROCESS_MEMORY_MAP $(DEF_TUNER_RULE) -DDRXDRIVER_DEBUG=1 -DBSP_I2C_USE_SOC=1 $(DEF_PROJECT_RULE) -DBUILD_WITH_LINUX

#tuner tables
TUNER_FLAGS =	\
				-DTUNER_ALTOBEAM_ATBM253 \
				-DTUNER_SILICONLAB_SI2151

CFLAGS   += $(TUNER_FLAGS)
CXXFLAGS += $(TUNER_FLAGS)

SRCS    := bsp_host.c bsp_i2c.c drx_driver.c drx_dap_ihbb.c drxfsn.c Trid_DRX_API.c RF/bsp_tuner.c RF/tunercanned.c
SRCS_T  := $(Si2151_SOURCES) $(ATBM253_SOURCES)
SRCS    := $(addprefix Src/, $(SRCS))
SRCS_T  := $(addprefix Src/, $(SRCS_T))

CFLAGS  += $(DEF_CFG)
VPATH   := $(DRX_FUSION_DIR)/Src
OBJS    := $(patsubst Src/%.c, $(OBJ_DIR)/%.o, $(filter Src/%.c, $(SRCS)))
OBJS_T 	:= $(patsubst Src/%.c, $(OBJ_DIR)/%.o, $(filter Src/%.c, $(SRCS_T)))
LDFLAGS := -latbm253 -ltv_frontend -ltuner_base -latbm253 -ltuner_SI2151 -ltuner_R842 -L$(ROOTF_PATH)/usr/lib
all release debug: $(DESTLIB2)

$(DESTLIB2) : create_obj_dir $(OBJS) $(OBJS_T)
	rm -f $(DESTLIB2)
	$(LD) -shared $(LDFLAGS) $(OBJS) $(OBJS_T) -Wl,-soname,libdrxsx7.so -o $@

create_obj_dir:
	mkdir -p $(OBJ_DIR)
	mkdir -p $(OBJ_DIR)/RF
	mkdir -p $(OBJ_DIR)/$(ATBM253_FOLDER)
	mkdir -p $(OBJ_DIR)/$(Si2151_FOLDER)

$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) $(LDGLAGS) -c $< -o $@

clean:
	rm  -fr *.o  $(OBJ_DIR) $(DESTLIB2)


