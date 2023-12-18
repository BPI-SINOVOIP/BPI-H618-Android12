
include $(top_srcdir)/Makefile.inc

lib_LTLIBRARIES = libcdx_ion.la

libcdx_ion_la_SOURCES =   \
        ./ion_alloc.c

libcdx_ion_la_CFLAGS = $(CFLAGS_CDXG) -DCONF_KERNEL_IOMMU -DCONF_KERNEL_VERSION_4_9
LOCAL_INCLUDE = -I./include

libcdx_ion_la_CFLAGS += $(LOCAL_INCLUDE)


libcdx_ion_la_LDFLAGS = $(LDFLAGS_CDXG)


LOCAL_LIB = -lpthread -ldl
libcdx_ion_la_LDFLAGS += $(LOCAL_LIB)

