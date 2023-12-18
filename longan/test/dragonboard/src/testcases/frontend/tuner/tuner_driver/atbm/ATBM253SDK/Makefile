include $(LICHEE_DRAGONBAORD_DIR)/src/testcases/frontend/Rule_TV303.make

TARGET = $(INSTALL_DIR)/atbm253_test
DESTSHLIB = $(INSTALL_DIR)/libatbm253.so

TARGET_SOURCES = ATBM253Api.c ATBM253Driver.c ATBM253Porting.c ATBM253Sample.c
TARGET_OBJS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(filter %.c, $(TARGET_SOURCES)))

DESTSHLIB_SOURCES = ATBM253Api.c ATBM253Driver.c ATBM253Porting.c Atbm253_AW_Hal.c
DESTSHLIB_OBJS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(filter %.c, $(DESTSHLIB_SOURCES)))

INCLUDES += -I./ \
            -I./../../../include

CFLAGS += $(INCLUDES) -fPIC -g
CFLAGS += -DBUILD_WITH_LINUX
LDFLAGS += -ltuner_base -L$(INSTALL_DIR)

all: $(DESTSHLIB)

$(TARGET): create_obj_dir
	$(CC) $(CFLAGS) -c ATBM253Api.c -o $(OBJ_DIR)/ATBM253Api.o
	$(CC) $(CFLAGS) -c ATBM253Driver.c -o $(OBJ_DIR)/ATBM253Driver.o
	$(CC) $(CFLAGS) -c ATBM253Porting.c -o $(OBJ_DIR)/ATBM253Porting.o
	$(CC) $(CFLAGS) -c ATBM253Sample.c -o $(OBJ_DIR)/ATBM253Sample.o
	$(CC) ${CFLAGS} ${TARGET_OBJS}  -o $(OBJ_DIR)/${TARGET} $(LDFLAGS) $(LIBS)

$(DESTSHLIB): create_obj_dir
	$(CC) $(CFLAGS) -c ATBM253Api.c -o $(OBJ_DIR)/ATBM253Api.o
	$(CC) $(CFLAGS) -c ATBM253Driver.c -o $(OBJ_DIR)/ATBM253Driver.o
	$(CC) $(CFLAGS) -c ATBM253Porting.c -o $(OBJ_DIR)/ATBM253Porting.o
	$(CC) $(CFLAGS) -c Atbm253_AW_Hal.c -o $(OBJ_DIR)/Atbm253_AW_Hal.o
	$(LD) -shared $(LDFLAGS) $(DESTSHLIB_OBJS) -o $@

create_obj_dir:
	@mkdir -p $(OBJ_DIR)

clean:
	@rm -fr $(DESTSHLIB) $(DESTSHLIB_OBJS)
