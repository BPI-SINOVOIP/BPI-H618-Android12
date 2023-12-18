#
# Makefile to build libDemux.so
#
#Directory definition
TOP_DIR := $(LICHEE_DRAGONBAORD_DIR)/src/testcases/vslibrarybuild
include $(TOP_DIR)/Build/Rule_TV303.make
include $(TOP_DIR)/Build/Makefile_TV303.Inc

CUR_DIR = $(VS_ROOT)/HAL_SX6/Demux
DEMUX_SRC = $(CUR_DIR)/Src
OBJ_DIR := $(TOP_DIR)/HAL_SX6/Demux/Obj
VSLIBRARY_DIR = $(TOP_DIR)/lib

DESTSHLIB = $(VSLIBRARY_DIR)/libDemux.so

INCLUDES =  \
			-I$(VS_ROOT)/Inc \
			-I$(VS_ROOT)/Utility/TridUtil/ \
			-I$(VS_ROOT)/Utility/TridUtil/Inc \
			-I$(VS_ROOT)/HAL_SX6/Kernel_Driver/Inc \
			-I$(VS_ROOT)/HAL_SX6/Demux/Inc \
			-I$(VS_ROOT)/HAL_SX6/CpuComm/Inc \
			-I$(VS_ROOT)/HAL_SX6/Kernel_Driver/cpu_comm \
			-I$(VS_ROOT)/HAL_SX6/Kernel_Driver/cpu_comm/Inc \
			-I$(VS_ROOT)/HAL_SX6/Kernel_Driver/demux/inc

ifeq ($(debug),yes)
DEMUX_DEBUG = -DDEBUG
endif

DEMUX_DEBUG += -Wno-unused-but-set-variable -Wno-unused-function -Wno-unused-variable
CFLAGS += -Werror -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast -Wno-overflow
CFLAGS += $(INCLUDES) $(DEF_PROJECT_RULE) -DNEW_TRACE $(DEMUX_DEBUG) -DDEMUX_SECTION_LIST_SUPPORT

include $(CUR_DIR)/demux.lst

VPATH   := $(CUR_DIR)/Src
SRCS    := $(DEMUX_SRC)
OBJS    := $(patsubst %.c, $(OBJ_DIR)/%.o, $(filter %.c, $(SRCS)))

all release debug: $(DESTSHLIB)

$(DESTSHLIB) : create_obj_dir $(OBJS)
	@rm -f $(DESTSHLIB)
	@echo "Linking $@..."
	$(QUIET)$(LD) -shared $(LDFLAGS) $(OBJS)  -Wl,-soname,libDemux.so -o $@

create_obj_dir:
	@mkdir -p $(OBJ_DIR)
	@echo "$(SRCS)"

$(patsubst %.c, $(OBJ_DIR)/%.o, $(SRCS)): $(OBJ_DIR)/%.o : $(CUR_DIR)/Src/%.c
	@echo "Compiling $(PWD)/$<..."
	$(QUIET)$(CC) $(CFLAGS) -c $< -o $@

clean cleanall:
	@rm  -fr *.o  $(OBJ_DIR) $(DESTSHLIB)

