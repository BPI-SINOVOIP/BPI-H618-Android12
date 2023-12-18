PATH_TAG := Tools
TOP_DIR := $(shell pwd -P | sed "s@/$(PATH_TAG)*@@g")

SRC_PATH=${LICHEE_PLAT_OUT}/vs/vs-modules
include ${SRC_PATH}/Build/RuleAll.mk
include ${SRC_PATH}/Install/Core/Rule.make
include ${SRC_PATH}/Install/Core/Makefile.Inc


INC_INSTALL_DIR := ${SRC_PATH}/Install

ifneq (,$(findstring -DTV303, $(DEF_PROJECT_RULE)))
SUBDIRS := DebugServer Regtest
endif

.PHONY: subdirs $(SUBDIRS)

all debug release: subdirs

subdirs:$(SUBDIRS)

$(SUBDIRS):
	make $(MAKECMDGOALS) -C $@ || exit 1;

clean:
#	for dir in $(SUBDIRS); do ( make clean -C $$dir) ||exit 1; done
