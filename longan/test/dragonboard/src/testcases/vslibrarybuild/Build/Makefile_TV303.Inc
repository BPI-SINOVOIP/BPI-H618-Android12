#Use the default one of toolchain.
#TV303 has the same toolchain
CROSS_ROOT := ${LICHEE_TOOLCHAIN_PATH}
CROSS_COMPILE := $(CROSS_ROOT)/bin/arm-linux-gnueabi-
ifneq (,$(CCACHE))
CCACHE +=
endif

LD      :=$(CCACHE)$(CROSS_COMPILE)gcc
CC      :=$(CCACHE)$(CROSS_COMPILE)gcc
CXX     :=$(CCACHE)$(CROSS_COMPILE)g++
CPP     :=$(CC) -E
AR      :=$(CROSS_COMPILE)ar
NM      :=$(CROSS_COMPILE)nm
STRIP   :=$(CROSS_COMPILE)strip
OBJCOPY :=$(CROSS_COMPILE)objcopy
OBJDUMP :=$(CROSS_COMPILE)objdump
RANLIB	:=$(CROSS_COMPILE)ranlib

ifneq ( , $(CROSS_ROOT))
TOOLCHAIN_ROOT := --sysroot=$(CROSS_ROOT)/arm-linux-gnueabi/libc
endif

CPUARCH_FLAGS := -mcpu=cortex-a53 -marm -mlittle-endian -mfpu=neon #-mfloat-abi=hard
CPUARCH_FLAGS_64BIT := -mcpu=cortex-a53 -mlittle-endian
DEFS := -fPIC -D_REENTRANT -DPURE_LITTLE_ENDIAN
OPT := -O2 -fno-strict-aliasing $(CPUARCH_FLAGS)
OPT_64BIT := -O2 -fno-strict-aliasing $(CPUARCH_FLAGS_64BIT)
BASE_FLAGS := -Wall -Wno-error=unused-variable -Wno-error=unused-but-set-variable -Wno-unused-local-typedefs \
				-Wno-sizeof-pointer-memaccess -Wno-maybe-uninitialized -fno-delete-null-pointer-checks -Wno-error=pointer-to-int-cast -Wno-error=int-to-pointer-cast
CFLAGS := $(TOOLCHAIN_ROOT) $(OPT) $(DEBUG) $(DEFS) $(BASE_FLAGS) -Wstrict-prototypes -fgnu89-inline
CPPFLAGS := $(TOOLCHAIN_ROOT) $(OPT) $(DEBUG) $(DEFS) $(BASE_FLAGS)
KERNEL_CFLAGS := $(OPT_64BIT) -DCONFIG_ENDIAN_LITTLE $(BASE_FLAGS)
ARFLAGS := ru
LDFLAGS := $(TOOLCHAIN_ROOT)
HOST := arm-linux
#64KO_TOOLCHAIN_PATH:=/opt/cross-arm/linaro64/bin/
CONFIG_64BIT_KO := 1
