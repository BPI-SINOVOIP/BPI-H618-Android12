include $(LICHEE_DRAGONBAORD_DIR)/src/testcases/frontend/Rule_TV303.make

TARGET_LIB = $(INSTALL_DIR)/libtuner_R842.so
INCLUDES = 	-I./ \
            -I../../../include

#compile flags
CFLAGS += -fPIC -DBUILD_WITH_LINUX
LIBS  = -lc  -ltuner_base -L$(INSTALL_DIR)
OBJS := $(OBJ_DIR)/R842.o $(OBJ_DIR)/R842_AW_Hal.o

all : create_obj_dir $(OBJS) $(TARGET_LIB)

create_obj_dir:
	@mkdir -p $(OBJ_DIR)

$(TARGET_LIB):
	$(LD) -shared $(OBJS) -o $@

$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS)  $(INCLUDES) -c  $< -o $@

clean:
	@rm -fr $(TARGET_LIB) $(OBJS)

