#Alex Li, 2011/5/4
#For both ATSC and DVB

#Directory definition
TOP_DIR := $(LICHEE_DRAGONBAORD_DIR)/src/testcases/vslibrarybuild
include $(TOP_DIR)/Build/Rule_TV303.make
include $(TOP_DIR)/Build/Makefile_TV303.Inc
ROOTF_PATH=$(LICHEE_DRAGONBAORD_DIR)/rootfs

VSLIBRARY_DIR = $(TOP_DIR)/lib
TARGET = TestTuner
TARGET_LIB = libTestTuner.so

DRAGONBORAD_OUTPUT_BIN_DIR := $(LICHEE_DRAGONBAORD_DIR)/output/bin

INCLUDES = 	\
			-I./	\
			-I$(VS_ROOT)/Inc\
			-I$(VS_ROOT)/Application/Inc\
			-I$(VS_ROOT)/Application/Inc/Tplfapi\
			-I$(VS_ROOT)/Utility/TridUtil \
			-I$(VS_ROOT)/Utility/TridUtil/Inc \
			-I$(VS_ROOT)/Utility/TridXMLParser \
			-I$(VS_ROOT)/HAL_SX6/Display/Inc \
			-I$(VS_ROOT)/Utility/TridXMLParser/Inc

INCLUDES += -I$(VS_ROOT)/Application/User_Driver/Tuner/DRX_Fusion/Inc

#compile flags
CFLAGS +=  $(DEF_PROJECT_RULE) $(INCLUDES)

#temporary not to show atv layer
#CFLAGS +=  -DTEST_ATV_LAYER
CPPFLAGS += $((DEF_PROJECT_RULE)  $(INCLUDES)

LIBS  = -pthread -ldl  \
		-L$(VSLIBRARY_DIR) \
		-L./ \
		-lUtility \
		-lCPUComm \
		-lTridXMLParser \
		-lTestTuner

ifneq (,$(findstring -DTEST_ATV_LAYER, $(CFLAGS)))
	LIBS += -lDisplay
endif

OBJ_DIR = ./Obj
OBJS := $(OBJ_DIR)/main.o
LIB_OBJS := $(OBJ_DIR)/dvb_tuner.o $(OBJ_DIR)/atsc_tuner.o

ifeq ($(LICHEE_CHIP),sun50iw12p1)
all : create_obj_dir $(OBJS) $(LIB_OBJS) $(TARGET_LIB) $(TARGET) install
create_obj_dir:
	mkdir -p $(OBJ_DIR);

$(TARGET_LIB): $(LIB_OBJS)
	$(LD) -shared -Wl,-soname,$@ $(LIB_OBJS) -o $@

$(TARGET): $(TARGET_LIB) ${OBJS}
	$(CXX)  $(CPPFLAGS)  ${OBJS}  -o $(TARGET) $(LIBS)

$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c  $< -o $@

install:
	cp $(TARGET_LIB) $(ROOTF_PATH)/usr/lib/
	cp $(TARGET) $(DRAGONBORAD_OUTPUT_BIN_DIR)/
	cp atv_tester.sh $(DRAGONBORAD_OUTPUT_BIN_DIR)/
	cp atsc_system.xml dvb_system.xml $(ROOTF_PATH)/etc/

clean:
	rm -fr $(TARGET) $(TARGET_LIB) $(OBJ_DIR)
else
all:
	@echo -e '\033[0;31mSkip build vs modules due to unsupported chip ic[$(LICHEE_CHIP)]...\033[0m'
clean:
	@echo -e '\033[0;31mDo nothing...\033[0m'
endif
