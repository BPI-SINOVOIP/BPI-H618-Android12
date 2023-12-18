#
# Makefile to build libDisplay.so
#
#Directory definition
TOP_DIR := $(LICHEE_DRAGONBAORD_DIR)/src/testcases/vslibrarybuild
include $(TOP_DIR)/Build/Rule_TV303.make
include $(TOP_DIR)/Build/Makefile_TV303.Inc

CUR_DIR = $(VS_ROOT)/HAL_SX6/Display
DISP_SRC = $(CUR_DIR)/Src
OBJ_DIR := $(TOP_DIR)/HAL_SX6/Display/Obj
VSLIBRARY_DIR = $(TOP_DIR)/lib

DESTSHLIB = $(VSLIBRARY_DIR)/libDisplay.so

INCLUDES =  \
	-I$(VS_ROOT)/Inc \
	-I$(CUR_DIR)/Inc\
	-I$(CUR_DIR)/Src/reg_access\
	-I$(VS_ROOT)/Utility/TridUtil/ \
	-I$(VS_ROOT)/Utility/TridUtil/Inc \
	-I$(VS_ROOT)/HAL_SX6/Kernel_Driver/Inc \
	-I$(VS_ROOT)/HAL_SX6/CpuComm/Inc \
	-I$(VS_ROOT)/HAL_SX6/Kernel_Driver/cpu_comm \
	-I$(VS_ROOT)/HAL_SX6/Kernel_Driver/cpu_comm/Inc \
	-I$(VS_ROOT)/HAL_SX6/SPI \
	-I$(VS_ROOT)/HAL_SX6/Inc


CFLAGS += -Werror
CFLAGS += $(INCLUDES) $(DEF_PROJECT_RULE) -DNEW_TRACE
#LDFLAGS += 
CXXFLAGS +=$(shell echo $(CFLAGS) | sed -e "s/-Wstrict-prototypes//g" | sed -e "s/-fgnu89-inline//g" )

SRCS += thal_display_wce.c
SRCS += thal_display_pq.c
SRCS += thal_display_source.c
SRCS += thal_display_vbi.c
SRCS += thal_display_misc.c
SRCS += thal_display_backlight.c
SRCS += thal_display_hdmi.c
SRCS += thal_display_unuse.c

SRCS += display_pq_algorithm/display_pq_gamma.c
SRCS += display_pq_algorithm/BezierFit.c
SRCS += reg_access/reg_access.c

VPATH := $(CUR_DIR)/Src
#SRCS    := $(DISP_SRC)
OBJS    := $(patsubst %.c, $(OBJ_DIR)/%.o, $(filter %.c, $(SRCS)))
OBJS    += $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(filter %.cpp, $(SRCS)))

all release debug: $(DESTSHLIB)

$(DESTSHLIB) : create_obj_dir $(OBJS)
	@rm -f $(DESTSHLIB)
	@echo "Linking $@..."
	$(QUIET)$(LD) -shared  $(LDFLAGS) $(OBJS)  -Wl,-soname,libDisplay.so -o $@

create_obj_dir:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(OBJ_DIR)/display_pq_algorithm
	@mkdir -p $(OBJ_DIR)/reg_access
	@echo "$(SRCS)"	

$(patsubst %.c, $(OBJ_DIR)/%.o, $(SRCS)): $(OBJ_DIR)/%.o : $(CUR_DIR)/Src/%.c
	@echo "Compiling $(PWD)/$<..."
	$(QUIET)$(CC) $(CFLAGS) -c $< -o $@

clean cleanall:
	@rm  -fr *.o  $(OBJ_DIR) $(DESTSHLIB)