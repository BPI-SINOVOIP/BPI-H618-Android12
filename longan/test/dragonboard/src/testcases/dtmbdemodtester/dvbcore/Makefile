PATH_BUILD = $(LICHEE_TOOLCHAIN_PATH)/bin
CC = $(PATH_BUILD)/arm-linux-gnueabi-gcc
LD = $(PATH_BUILD)/arm-linux-gnueabi-ld

ROOTF_PATH=$(LICHEE_DRAGONBAORD_DIR)/rootfs
OBJ_DIR = $(LICHEE_DRAGONBAORD_DIR)/src/testcases/dtmbdemodtester/out/dvbcore
INSTALL_DIR = $(OBJ_DIR)/../release
TARGET_LIB = $(INSTALL_DIR)/libdvb_core.so

TARGET_SOURCES = $(wildcard *.c)

TARGET_OBJS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(filter %.c, $(TARGET_SOURCES)))

INCLUDES = -I./ \
           -I../demux/program_manager \
           -I../demod \
		   -I../demux \
		   -I../demux/tsdemux_vs \
		   -DBUILD_WITH_LINUX

LIBS     = -lpthread  -lgcc_s -ldl
CFLAGS  += $(INCLUDES) -g -fPIC
LDFLAGS += -L$(INSTALL_DIR)

all: $(TARGET_LIB)

$(TARGET_LIB): create_obj_dir $(TARGET_OBJS)
	$(LD) -shared $(TARGET_OBJS) -o $@

create_obj_dir:
	@mkdir -p $(OBJ_DIR)
	echo $(TARGET_OBJS)

$(patsubst %.c, $(OBJ_DIR)/%.o, $(TARGET_SOURCES)): $(OBJ_DIR)/%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -fr $(TARGET_LIB) $(TARGET_OBJS)
	@rm -r $(OBJ_DIR)