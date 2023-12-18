#
# Makefile to build
#
#Directory definition
include $(LICHEE_DRAGONBAORD_DIR)/src/testcases/frontend/Rule_TV303.make

#----------------------------------------------------------------------
# TARGET share HAL Test Demux  Lib
#----------------------------------------------------------------------
DESTSHLIB = $(INSTALL_DIR)/libtv_frontend.so

#-----------------------------------------------------------------------
# Includes path definition
#-----------------------------------------------------------------------
INCLUDES = -I./include

#-----------------------------------------------------------------------
# compiler flag definition
#-----------------------------------------------------------------------
CFLAGS += -Werror $(INCLUDES) -fPIC -DBUILD_WITH_LINUX
LDFLAGS = -L$(INSTALL_DIR)

#-----------------------------------------------------------------------
# Sources definition
#-----------------------------------------------------------------------
SRCS := tv_frontend.c

#-----------------------------------------------------------------------
# objects definition
#-----------------------------------------------------------------------
OBJS    := $(patsubst %.c, $(OBJ_DIR)/%.o, $(filter %.c, $(SRCS)))

#-----------------------------------------------------------------------
# library definition
#-----------------------------------------------------------------------
LIBS = -latbm253 -ltuner_SI2151 -ltuner_R842

#-----------------------------------------------------------------------
# build command definition
#-----------------------------------------------------------------------

all: $(DESTSHLIB)

$(DESTSHLIB) : create_obj_dir $(OBJS)
	$(shell rm -f $(DESTSHLIB))
	$(LD) -shared $(LDFLAGS) $(LIBS) $(OBJS) -o $@

create_obj_dir:
	@mkdir -p $(OBJ_DIR)

$(patsubst %.c, $(OBJ_DIR)/%.o, $(SRCS)): $(OBJ_DIR)/%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -fr $(OBJS) $(DESTSHLIB)
