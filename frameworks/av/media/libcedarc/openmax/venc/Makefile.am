
include $(top_srcdir)/Makefile.inc

lib_LTLIBRARIES = libOmxVenc.la

libOmxVenc_la_SOURCES = omx_venc.c\
                        omx_tsem.c\
                        omx_venc_linux_adapter.c \
                        omx_common.c

libOmxVenc_la_CFLAGS = $(CFLAGS_CDCG)
libOmxVenc_la_CPPFLAGS = $(CPPFLAGS_CDCG)
LOCAL_INCLUDE = -I../include  \
                -I$(top_srcdir)/openmax/omxcore/include \
                -I$(top_srcdir)/include \
                -I$(top_srcdir)/base/include \
                -I$(top_srcdir)/ve/include

libOmxVenc_la_CFLAGS += $(LOCAL_INCLUDE)
libOmxVenc_la_CPPFLAGS += $(LOCAL_INCLUDE)

libOmxVenc_la_LDFLAGS = $(LDFLAGS_CDCG)

libOmxVenc_la_LIBADD = $(top_srcdir)/base/libcdc_base.la \
                       $(top_srcdir)/memory/libMemAdapter.la \
			$(top_srcdir)/vencoder/base/libvenc_base.la \
			$(top_srcdir)/vencoder/libvencoder.la

LOCAL_LIB = -ldl -lrt -lpthread -lVE
libOmxVenc_la_LDFLAGS += $(LOCAL_LIB)

