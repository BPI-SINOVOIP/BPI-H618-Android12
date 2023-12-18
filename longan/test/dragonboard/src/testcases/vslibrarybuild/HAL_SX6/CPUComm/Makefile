#
# Makefile to build libCPUComm.so for 32bit
#
#Directory definition
TOP_DIR := $(LICHEE_DRAGONBAORD_DIR)/src/testcases/vslibrarybuild
include $(TOP_DIR)/Build/Rule_TV303.make
include $(TOP_DIR)/Build/Makefile_TV303.Inc

CUR_DIR = $(VS_ROOT)/HAL_SX6/CpuComm
CPUCOM_SRC = $(CUR_DIR)/Src
OBJ_DIR := $(TOP_DIR)/HAL_SX6/CPUComm/Obj
VSLIBRARY_DIR = $(TOP_DIR)/lib

DESTSHLIB = $(VSLIBRARY_DIR)/libCPUComm.so

INCLUDES =  \
			-I$(VS_ROOT)/Inc\
			-I$(VS_ROOT)/HAL_SX6/Inc \
			-I$(VS_ROOT)/HAL_SX6/Kernel_Driver \
			-I$(VS_ROOT)/Utility/TridUtil \
			-I$(VS_ROOT)/HAL_SX6/CpuComm/Inc \
			-I$(VS_ROOT)/HAL_SX6/Kernel_Driver/cpu_comm \
			-I$(VS_ROOT)/HAL_SX6/Kernel_Driver/cpu_comm/Inc \
			-I$(VS_ROOT)/HAL_SX6/Kernel_Driver/Inc \
			-I$(VS_ROOT)/Utility/TridUtil/Inc

CFLAGS  += -Werror -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast
CFLAGS  += $(INCLUDES) $(DEF_PROJECT_RULE) -DNEW_TRACE
LDFLAGS +=

include $(CUR_DIR)/cpu_comm.lst

VPATH   := $(CUR_DIR)
SRCS    := $(CPUCOM_SRC)
OBJS    := $(patsubst %.c, $(OBJ_DIR)/%.o, $(filter %.c, $(SRCS)))

all release debug: $(DESTSHLIB)

$(DESTSHLIB) : create_obj_dir $(OBJS)
	@rm -f $(DESTSHLIB)
	@echo "Linking $@..."
	$(QUIET)$(LD) -shared $(LDFLAGS) $(OBJS)  -Wl,-soname,libCPUComm.so -o $@

create_obj_dir:
	@mkdir -p $(OBJ_DIR)/Src
	@echo "$(SRCS)"

$(patsubst %.c, $(OBJ_DIR)/%.o, $(SRCS)): $(OBJ_DIR)/%.o : $(CUR_DIR)/%.c
	@echo "Compiling $(PWD)/$<..."
	$(QUIET)$(CC) $(CFLAGS) -c $< -o $@

clean cleanall:
	@rm  -fr *.o  $(OBJ_DIR) $(DESTSHLIB)

