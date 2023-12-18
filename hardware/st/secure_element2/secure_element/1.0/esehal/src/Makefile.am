EXTRA_DIST = linux/se_gemalto.h
CLEANFILES = *.gcov *.gcda *.gcno

HAVE_LINK_VERSION_SCRIPT=@HAVE_LINK_VERSION_SCRIPT@

AM_CPPFLAGS = \
	-include $(top_builddir)/config.h \
	-DSYSCONFDIR=\""$(sysconfdir)"\" \
	-DLIBEXECDIR=\""$(libexecdir)"\"

AM_CFLAGS = ${my_CFLAGS} \
	-fvisibility=hidden \
	-ffunction-sections \
	-fdata-sections

LIBSE_GTO_CURRENT=2
LIBSE_GTO_REVISION=0
LIBSE_GTO_AGE=2

pkginclude_HEADERS = se-gto/libse-gto.h
lib_LTLIBRARIES = libse-gto.la

libse_gto_la_SOURCES = \
	libse-gto.c        \
	libse-gto-private.h \
	iso7816_t1.c      \
	iso7816_t1.h      \
	checksum.c        \
	checksum.h        \
	transport.c       \
	transport.h       \
	spi.c             \
	spi.h             \
	log.c             \
	log.h             \
	compiler.h

libse_gto_la_LDFLAGS = $(AM_LDFLAGS) \
	-version-info $(LIBSE_GTO_CURRENT):$(LIBSE_GTO_REVISION):$(LIBSE_GTO_AGE)

if HAVE_LINK_VERSION_SCRIPT
# Deactivate usage of symbols version until Linux port further need it
#  EXTRA_DIST += libse-gto.sym
#  libse_gto_la_LDFLAGS += -Wl,--version-script=libse-gto.sym
#  libse_gto_la_DEPENDENCIES = libse-gto.sym
endif

SED_PROCESS = \
	$(AM_V_GEN)$(MKDIR_P) $(dir $@) && $(SED) \
	-e 's,@VERSION\@,$(VERSION),g' \
	-e 's,@prefix\@,$(prefix),g' \
	-e 's,@exec_prefix\@,$(exec_prefix),g' \
	-e 's,@libdir\@,$(libdir),g' \
	-e 's,@includedir\@,$(includedir),g' \
	< $< > $@ || rm $@

%.pc: %.pc.in Makefile
	$(SED_PROCESS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libse-gto.pc
EXTRA_DIST += libse-gto.pc.in
CLEANFILES += libse-gto.pc
