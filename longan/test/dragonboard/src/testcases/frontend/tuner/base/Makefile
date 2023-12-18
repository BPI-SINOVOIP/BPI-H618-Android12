include $(LICHEE_DRAGONBAORD_DIR)/src/testcases/frontend/Rule_TV303.make

DESTSHLIB = $(INSTALL_DIR)/libtuner_base.so
DESTSHLIB_SOURCES = os_adapter.c
DESTSHLIB_OBJS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(filter %.c, $(DESTSHLIB_SOURCES)))

INCLUDES += -I./ -I../include/

CFLAGS += -DBUILD_WITH_LINUX -g
CFLAGS += $(INCLUDES) -fPIC
LDFLAGS +=

LIBS =

all: $(DESTSHLIB)

$(DESTSHLIB): create_obj_dir
	$(CC) $(CFLAGS) -c os_adapter.c -o $(OBJ_DIR)/os_adapter.o
	$(LD) -shared $(LDFLAGS) $(DESTSHLIB_OBJS) -o $@

create_obj_dir:
	@mkdir -p $(OBJ_DIR)

clean:
	@rm -fr $(DESTSHLIB) $(DESTSHLIB_OBJS)
