#
# defines
#
topdir=$(shell pwd)/..
include ../Make.Rules
#
# Library version
#
CAPLIBNAME=$(LIBTITLE).so
STACAPLIBNAME=$(LIBTITLE).a
#
PSXLIBNAME=libpsx.so
STAPSXLIBNAME=libpsx.a

CAPFILES=cap_alloc cap_proc cap_extint cap_flag cap_text cap_file
PSXFILES=../psx/psx

INCLS=libcap.h cap_names.h $(INCS)
GPERF_OUTPUT = _caps_output.gperf

CAPOBJS=$(addsuffix .o, $(CAPFILES))
MAJCAPLIBNAME=$(CAPLIBNAME).$(VERSION)
MINCAPLIBNAME=$(MAJCAPLIBNAME).$(MINOR)

PSXOBJS=$(addsuffix .o, $(PSXFILES))
MAJPSXLIBNAME=$(PSXLIBNAME).$(VERSION)
MINPSXLIBNAME=$(MAJPSXLIBNAME).$(MINOR)

all: pcs $(STACAPLIBNAME)
ifeq ($(SHARED),yes)
	$(MAKE) $(CAPLIBNAME)
endif
ifeq ($(PTHREADS),yes)
	$(MAKE) $(STAPSXLIBNAME)
ifeq ($(SHARED),yes)
	$(MAKE) $(PSXLIBNAME)
endif
endif

pcs: libcap.pc
ifeq ($(PTHREADS),yes)
	$(MAKE) libpsx.pc
endif

ifeq ($(BUILD_GPERF),yes)
USE_GPERF_OUTPUT = $(GPERF_OUTPUT)
INCLUDE_GPERF_OUTPUT = -DINCLUDE_GPERF_OUTPUT='"$(GPERF_OUTPUT)"'
endif

libcap.pc: libcap.pc.in
	sed -e 's,@prefix@,$(prefix),' \
		-e 's,@exec_prefix@,$(exec_prefix),' \
		-e 's,@libdir@,$(LIBDIR),' \
		-e 's,@includedir@,$(inc_prefix)/include,' \
		-e 's,@VERSION@,$(VERSION).$(MINOR),' \
		-e 's,@deps@,$(DEPS),' \
		$< >$@

libpsx.pc: libpsx.pc.in
	sed -e 's,@prefix@,$(prefix),' \
		-e 's,@exec_prefix@,$(exec_prefix),' \
		-e 's,@libdir@,$(LIBDIR),' \
		-e 's,@includedir@,$(inc_prefix)/include,' \
		-e 's,@VERSION@,$(VERSION).$(MINOR),' \
		-e 's,@deps@,$(DEPS),' \
		$< >$@

_makenames: _makenames.c cap_names.list.h
	$(BUILD_CC) $(BUILD_CFLAGS) $< -o $@

cap_names.h: _makenames
	./_makenames > cap_names.h

$(GPERF_OUTPUT): cap_names.list.h
	perl -e 'print "struct __cap_token_s { const char *name; int index; };\n%{\nconst struct __cap_token_s *__cap_lookup_name(const char *, size_t);\n%}\n%%\n"; while ($$l = <>) { $$l =~ s/[\{\"]//g; $$l =~ s/\}.*// ; print $$l; }' < $< | gperf --ignore-case --language=ANSI-C --readonly --null-strings --global-table --hash-function-name=__cap_hash_name --lookup-function-name="__cap_lookup_name" -c -t -m20 $(INDENT) > $@
	sed -e 's/unsigned int len/size_t len/' -i $@

# Intention is that libcap keeps up with torvalds' tree, as reflected
# by this maintained version of the kernel header. libcap dynamically
# trims the meaning of "all" capabilities down to that of the running
# kernel as of 2.30.
UAPI_HEADER := $(topdir)/libcap/include/uapi/linux/capability.h
cap_names.list.h: Makefile $(UAPI_HEADER)
	@echo "=> making $@ from $(UAPI_HEADER)"
	perl -e 'while ($$l=<>) { if ($$l =~ /^\#define[ \t](CAP[_A-Z]+)[ \t]+([0-9]+)\s+$$/) { $$tok=$$1; $$val=$$2; $$tok =~ tr/A-Z/a-z/; print "{\"$$tok\",$$val},\n"; } }' $(UAPI_HEADER) | fgrep -v 0x > $@

$(STACAPLIBNAME): $(CAPOBJS)
	$(AR) rcs $@ $^
	$(RANLIB) $@

$(STAPSXLIBNAME): $(PSXOBJS) include/sys/psx_syscall.h
	$(AR) rcs $@ $(PSXOBJS)
	$(RANLIB) $@

ifeq ($(SHARED),yes)
$(CAPLIBNAME) $(MAJCAPLIBNAME) $(MINCAPLIBNAME): $(CAPOBJS)
	$(LD) $(CFLAGS) $(LDFLAGS) -Wl,-soname,$(MAJCAPLIBNAME) -o $(MINCAPLIBNAME) $^
	ln -sf $(MINCAPLIBNAME) $(MAJCAPLIBNAME)
	ln -sf $(MAJCAPLIBNAME) $(CAPLIBNAME)

$(PSXLIBNAME) $(MAJPSXLIBNAME) $(MINPSXLIBNAME): $(PSXOBJS) include/sys/psx_syscall.h
	$(LD) $(CFLAGS) $(LDFLAGS) -Wl,-soname,$(MAJPSXLIBNAME) -o $(MINPSXLIBNAME) $(PSXOBJS) $(PSXLINKFLAGS)
	ln -sf $(MINPSXLIBNAME) $(MAJPSXLIBNAME)
	ln -sf $(MAJPSXLIBNAME) $(PSXLIBNAME)
endif

%.o: %.c $(INCLS)
	$(CC) $(CFLAGS) $(IPATH) -c $< -o $@

cap_text.o: cap_text.c $(USE_GPERF_OUTPUT) $(INCLS)
	$(CC) $(CFLAGS) $(IPATH) $(INCLUDE_GPERF_OUTPUT) -c $< -o $@

cap_test: cap_test.c libcap.h
	$(CC) $(CFLAGS) $(IPATH) $< -o $@

test: cap_test
	./cap_test

install: install-static
ifeq ($(SHARED),yes)
	$(MAKE) install-shared
endif

install-static: install-static-cap
ifeq ($(PTHREADS),yes)
	$(MAKE) install-static-psx
endif

install-shared: install-shared-cap
ifeq ($(PTHREADS),yes)
	$(MAKE) install-shared-psx
endif

install-cap: install-static-cap
ifeq ($(SHARED),yes)
	$(MAKE) install-shared-cap
endif

install-psx: install-static-psx
ifeq ($(SHARED),yes)
	$(MAKE) install-shared-psx
endif

install-static-cap: install-common-cap $(STACAPLIBNAME)
	install -m 0644 $(STACAPLIBNAME) $(FAKEROOT)$(LIBDIR)/$(STACAPLIBNAME)

install-shared-cap: install-common-cap $(MINCAPLIBNAME)
	install -m 0644 $(MINCAPLIBNAME) $(FAKEROOT)$(LIBDIR)/$(MINCAPLIBNAME)
	ln -sf $(MINCAPLIBNAME) $(FAKEROOT)$(LIBDIR)/$(MAJCAPLIBNAME)
	ln -sf $(MAJCAPLIBNAME) $(FAKEROOT)$(LIBDIR)/$(CAPLIBNAME)
ifeq ($(FAKEROOT),)
	-/sbin/ldconfig
endif

install-static-psx: install-common-psx $(STAPSXLIBNAME)
	install -m 0644 $(STAPSXLIBNAME) $(FAKEROOT)$(LIBDIR)/$(STAPSXLIBNAME)

install-shared-psx: install-common-psx $(MINPSXLIBNAME)
	install -m 0644 $(MINPSXLIBNAME) $(FAKEROOT)$(LIBDIR)/$(MINPSXLIBNAME)
	ln -sf $(MINPSXLIBNAME) $(FAKEROOT)$(LIBDIR)/$(MAJPSXLIBNAME)
	ln -sf $(MAJPSXLIBNAME) $(FAKEROOT)$(LIBDIR)/$(PSXLIBNAME)
ifeq ($(FAKEROOT),)
	-/sbin/ldconfig
endif

install-common-cap: install-common libcap.pc
	install -m 0644 include/sys/capability.h $(FAKEROOT)$(INCDIR)/sys
	install -m 0644 libcap.pc $(FAKEROOT)$(PKGCONFIGDIR)/libcap.pc

include/sys/psx_syscall.h: ../psx/psx_syscall.h
	rm -f $@
	ln -s ../../../psx/psx_syscall.h $@

install-common-psx: install-common libpsx.pc include/sys/psx_syscall.h
	install -m 0644 include/sys/psx_syscall.h $(FAKEROOT)$(INCDIR)/sys
	install -m 0644 libpsx.pc $(FAKEROOT)$(PKGCONFIGDIR)/libpsx.pc

install-common:
	mkdir -p -m 0755 $(FAKEROOT)$(INCDIR)/sys
	mkdir -p -m 0755 $(FAKEROOT)$(PKGCONFIGDIR)
	mkdir -p -m 0755 $(FAKEROOT)$(LIBDIR)

clean:
	$(LOCALCLEAN)
	rm -f $(CAPOBJS) $(CAPLIBNAME)* $(STACAPLIBNAME) libcap.pc
	rm -f $(PSXOBJS) $(PSXLIBNAME)* $(STAPSXLIBNAME) libpsx.pc
	rm -f cap_names.h cap_names.list.h _makenames $(GPERF_OUTPUT) cap_test
	rm -f include/sys/psx_syscall.h
	cd include/sys && $(LOCALCLEAN)
