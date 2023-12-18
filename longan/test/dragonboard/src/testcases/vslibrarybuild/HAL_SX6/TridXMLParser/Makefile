#
# Makefile to build Utility library:libTridXMLParser.so
#
# Directory definition
TOP_DIR := $(LICHEE_DRAGONBAORD_DIR)/src/testcases/vslibrarybuild
include $(TOP_DIR)/Build/Rule_TV303.make
include $(TOP_DIR)/Build/Makefile_TV303.Inc

CUR_DIR := $(VS_ROOT)/Utility/TridXMLParser
OBJ_DIR = $(TOP_DIR)/HAL_SX6/TridXMLParser/Obj
VSLIBRARY_DIR = $(TOP_DIR)/lib

#CPPFLAGS += -DTIXML_USE_STL
CPPFLAGS += -I$(CUR_DIR)/Inc \
			-I$(VS_ROOT)/Inc \
			-I$(VS_ROOT)/Utility/TridUtil \
			-I$(VS_ROOT)/Utility/TridUtil/Inc

CXXFLAGS += -Wall
LDFLAGS  += -fPIC

DESTSHLIB = $(VSLIBRARY_DIR)/libTridXMLParser.so

VPATH   := $(CUR_DIR)
CPP_SRCS:= Src/tinystr.cpp Src/tinyxml.cpp Src/tinyxmlerror.cpp Src/tinyxmlparser.cpp Src/tinyxmlparser_api.cpp
OBJS    := $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(filter %.cpp, $(CPP_SRCS)))

all release debug: $(DESTSHLIB)

$(DESTSHLIB) : create_obj_dir $(OBJS)
	@rm -f $(DESTSHLIB)
	@echo "Linking $@..."
	$(QUIET)$(LD) -shared $(LDFLAGS) $(OBJS) -Wl,-soname,libTridXMLParser.so -o $@

create_obj_dir:
	@mkdir -p $(OBJ_DIR)/Src

$(patsubst %.cpp, $(OBJ_DIR)/%.o, $(CPP_SRCS)): $(OBJ_DIR)%.o : $(CUR_DIR)/%.cpp
	@echo "Compiling $(PWD)/$<..."
	$(QUIET)$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

clean cleanall:
	@rm  -fr *.so $(OBJ_DIR) $(DESTSHLIB)
