#
# Makefile to build Utility library:libUtility.so
#
#Directory definition
TOP_DIR := $(LICHEE_DRAGONBAORD_DIR)/src/testcases/vslibrarybuild
include $(TOP_DIR)/Build/Rule_TV303.make
include $(TOP_DIR)/Build/Makefile_TV303.Inc

S_NEW = $(VS_ROOT)/Utility/TridUtil/Src
CUR_DIR = $(VS_ROOT)/Utility/TridUtil
OBJ_DIR = $(TOP_DIR)/HAL_SX6/TridUtil/Obj
VSLIBRARY_DIR = $(TOP_DIR)/lib

INC_INSTALL_DIR := $(VS_ROOT)/Utility/TridUtil

DESTSHLIB := $(VSLIBRARY_DIR)/libUtility.so

INCLUDES = 	-I$(VS_ROOT)/Inc \
            -I$(VS_ROOT)/Application/Inc \
            -I$(CUR_DIR)/Inc \
            -I$(VS_ROOT)/Utility \

CFLAGS += -Werror $(INCLUDES) $(DEF_PROJECT_RULE) $(DEF_CFG) -Wno-pointer-to-int-cast
CPPFLAGS += -Werror -lstdc++ $(INCLUDES) $(DEF_PROJECT_RULE) $(DEF_CFG)
LDFLAGS += -ldl
include $(CUR_DIR)/util.lst

VPATH   := $(CUR_DIR)
SRCS    := $(S_NEW)
OBJS    := $(patsubst %.c, $(OBJ_DIR)/%.o, $(filter %.c, $(SRCS)))

FUSEMAP_INC:= $(CUR_DIR)/Src/fuse_data_map.inc
AUTOFUSE_H := $(CUR_DIR)/Inc/autofuse.h
GENFUSE    := $(CUR_DIR)/Script/genfuseheader.awk
CHIPID     := $(firstword $(subst -DPRO_,,$(filter -DPRO_%,$(DEF_PROJECT_RULE))))

all release debug: $(DESTSHLIB)

$(DESTSHLIB) : create_obj_dir $(AUTOFUSE_H) $(OBJS)
	rm -f $(DESTSHLIB)
	@echo "Linking $@"
	$(QUIET)$(LD) -shared $(LDFLAGS) $(OBJS) -lrt -ldl -Wl,-soname,libUtility.so -o $@

create_obj_dir:
	@mkdir -p $(OBJ_DIR)/Src

$(AUTOFUSE_H) : $(FUSEMAP_INC) $(GENFUSE)
	$(QUIET)$(CC) -DCONFIG_SIGMA_SOC_$(CHIPID) -E -x c $< | \
	$(GENFUSE) > $@

$(patsubst %.c, $(OBJ_DIR)/%.o, $(SRCS)): $(OBJ_DIR)%.o : $(CUR_DIR)%.c $(AUTOFUSE_H)
	@echo "Compiling $(PWD)/$<"
	$(QUIET)$(CC) $(CFLAGS) -c $< -o $@

clean cleanall:
	@rm  -fr *.o  $(OBJ_DIR) $(AUTOFUSE_H) $(DESTSHLIB)

