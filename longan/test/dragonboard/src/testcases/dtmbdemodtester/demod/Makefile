VS_TOP_DIR := $(LICHEE_DRAGONBAORD_DIR)/src/testcases/vslibrarybuild
include $(VS_TOP_DIR)/Build/Rule_TV303.make
include $(VS_TOP_DIR)/Build/Makefile_TV303.Inc

PATH_BUILD = $(LICHEE_TOOLCHAIN_PATH)/bin
CC = $(PATH_BUILD)/arm-linux-gnueabi-gcc
LD = $(PATH_BUILD)/arm-linux-gnueabi-ld

ROOTF_PATH=$(LICHEE_DRAGONBAORD_DIR)/rootfs
OBJ_DIR = $(LICHEE_DRAGONBAORD_DIR)/src/testcases/dtmbdemodtester/out/demod
INSTALL_DIR = $(OBJ_DIR)/../release
TARGET_LIB = $(INSTALL_DIR)/libdvb_demod.so

TARGET_SOURCES = DTMBIP.c \
                 DTMBIP_User_Define.c \
                 dc_demod.c \
                 dc_wrapper_dtmbip.c \
                 dc_wrapper_Fusion.c

VS_INC = -I$(VS_ROOT)/Application/User_Driver/Tuner/DRX_Fusion/Inc \
         -I$(VS_ROOT)/Application/User_Driver/Tuner/DRX_Fusion/Inc/RF \
		 -I$(VS_ROOT)/Inc \
		 -I$(VS_ROOT)/Utility/TridUtil/Inc \
		 -I$(VS_ROOT)/Utility/TridXMLParser/Inc \
		 -I$(VS_ROOT)/Application/Inc \
         -I$(VS_ROOT)/Application/Inc/Tplfapi \
		 -I$(LICHEE_DRAGONBAORD_DIR)/src/testcases/vsdemodtester

TARGET_OBJS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(filter %.c, $(TARGET_SOURCES)))

INCLUDES = -I./ -I../dvbcore -I$(LICHEE_DRAGONBAORD_DIR)/src/testcases/frontend/tuner/include -DBUILD_WITH_LINUX $(VS_INC)
LIBS     = -lpthread -ltv_frontend -ltuner_base -latbm253 -ltuner_SI2151 -ltuner_R842 -lgcc_s -fPIC \
           -ldl -lUtility -lTridXMLParser -lTestTuner
CFLAGS  += $(INCLUDES) -g $(DEF_PROJECT_RULE)
LDFLAGS += -L$(INSTALL_DIR) -L$(ROOTF_PATH)/usr/lib

all: $(TARGET_LIB)

$(TARGET_LIB): create_obj_dir $(TARGET_OBJS)
	$(LD) -shared $(TARGET_OBJS) -o $@ $(LIBS) $(LDFLAGS)

create_obj_dir:
	@mkdir -p $(OBJ_DIR)
	echo $(TARGET_OBJS)

$(patsubst %.c, $(OBJ_DIR)/%.o, $(TARGET_SOURCES)): $(OBJ_DIR)/%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -fr $(TARGET_LIB) $(TARGET_OBJS)
	@rm -r $(OBJ_DIR)