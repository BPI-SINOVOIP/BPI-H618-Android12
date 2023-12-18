include $(LICHEE_DRAGONBAORD_DIR)/src/testcases/frontend/Rule_TV303.make

DESTSHLIB = $(INSTALL_DIR)/libtuner_SI2151.so

SRCS := \
	Si2151_Properties_Strings.c \
	Si2151_L2_API.c \
	Si2151_L1_Properties.c \
	Silabs_L0_Connection.c \
	Si2151_User_Properties.c \
	Si2151_L1_API.c \
	Si2151_L3_Test.c \
	Si2151_L1_Commands.c \
	Si2151_AW_Hal.c \
	#Si2151_Test.c \

DESTSHLIB_SOURCES = $(SRCS)
DESTSHLIB_OBJS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(filter %.c, $(DESTSHLIB_SOURCES)))

INCLUDES += -I./ \
            -I./../../../include

CFLAGS += $(INCLUDES) -fPIC -DBUILD_WITH_LINUX
LDFLAGS += -ltuner_base -L$(INSTALL_DIR)
LIBS =

all: $(DESTSHLIB)

$(DESTSHLIB): create_obj_dir
	$(CC) $(CFLAGS) -c Si2151_Properties_Strings.c -o $(OBJ_DIR)/Si2151_Properties_Strings.o
	#$(CC) $(CFLAGS) -c Si2151_Test.c -o $(OBJ_DIR)/Si2151_Test.o
	$(CC) $(CFLAGS) -c Si2151_L2_API.c -o $(OBJ_DIR)/Si2151_L2_API.o
	$(CC) $(CFLAGS) -c Si2151_L1_Properties.c -o $(OBJ_DIR)/Si2151_L1_Properties.o
	$(CC) $(CFLAGS) -c Silabs_L0_Connection.c -o $(OBJ_DIR)/Silabs_L0_Connection.o
	$(CC) $(CFLAGS) -c Si2151_User_Properties.c -o $(OBJ_DIR)/Si2151_User_Properties.o
	$(CC) $(CFLAGS) -c Si2151_L1_API.c -o $(OBJ_DIR)/Si2151_L1_API.o
	$(CC) $(CFLAGS) -c Si2151_L3_Test.c -o $(OBJ_DIR)/Si2151_L3_Test.o
	$(CC) $(CFLAGS) -c Si2151_L1_Commands.c -o $(OBJ_DIR)/Si2151_L1_Commands.o
	$(CC) $(CFLAGS) -c Si2151_AW_Hal.c -o $(OBJ_DIR)/Si2151_AW_Hal.o
	$(LD) -shared $(LDFLAGS) $(DESTSHLIB_OBJS) -o $@

create_obj_dir:
	@mkdir -p $(OBJ_DIR)

clean:
	@rm -fr $(DESTSHLIB) $(DESTSHLIB_OBJS)
