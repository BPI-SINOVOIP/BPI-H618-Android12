## SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
SUBDIRS = data

AM_CXXFLAGS = $(VISIBILITY_FLAGS)

FEDABIPKGDIFF_TEST =
if ENABLE_FEDABIPKGDIFF
if ENABLE_RUNNING_TESTS_WITH_PY3
FEDABIPKGDIFF_TEST += runtestfedabipkgdiffpy3.sh
else
FEDABIPKGDIFF_TEST += runtestfedabipkgdiff.py
endif
endif

RUN_TEST_SLOW_SELF_COMPARE=runtestslowselfcompare.sh

# rather expensive tests (keep in this order), > 1s runtime
TESTS=				\
$(RUN_TEST_SLOW_SELF_COMPARE)   \
runtestdiffsuppr		\
runtesttypesstability		\
runtestdiffpkg			\
runtestannotate			\
runtestdifffilter		\
runtestreaddwarf	        \
runtestcanonicalizetypes.sh

# rather cheap tests
TESTS+=				\
runtestabicompat		\
runtestabidiff			\
runtestabidiffexit		\
runtestaltdwarf			\
runtestcorediff			\
runtestcxxcompat		\
runtestdiffdwarf		\
runtestdiffdwarfabixml		\
runtestelfhelpers		\
runtestini			\
runtestkmiwhitelist		\
runtestlookupsyms		\
runtestreadwrite		\
runtestsymtab			\
runtestsymtabreader		\
runtesttoolsutils		\
runtestsvg			\
$(FEDABIPKGDIFF_TEST)


if ENABLE_RUNNING_TESTS_WITH_PY3
TESTS += runtestdefaultsupprspy3.sh
else
TESTS += runtestdefaultsupprs.py
endif

EXTRA_DIST = \
runtestcanonicalizetypes.sh.in \
runtestfedabipkgdiff.py.in \
runtestslowselfcompare.sh.in \
mockfedabipkgdiff.in \
test-valgrind-suppressions.supp

if ENABLE_RUNNING_TESTS_WITH_PY3
EXTRA_DIST += \
runtestfedabipkgdiffpy3.sh.in 	\
runtestdefaultsupprspy3.sh.in
endif

CLEANFILES = \
runtestcanonicalizetypes.output.txt \
runtestcanonicalizetypes.output.final.txt

noinst_PROGRAMS= $(TESTS) testirwalker testdiff2 printdifftree
noinst_SCRIPTS = mockfedabipkgdiff
noinst_LTLIBRARIES = libtestutils.la libcatch.la

libtestutils_la_SOURCES=	\
test-utils.h 			\
test-utils.cc

libtestutils_la_CXXFLAGS=		\
-DABIGAIL_SRC_DIR=\"${abs_top_srcdir}\"  	\
-DABIGAIL_BUILD_DIR=\"${abs_top_builddir}\"

libcatch_la_SOURCES = lib/catch.cc lib/catch.hpp

runtestreadwrite_SOURCES=test-read-write.cc
runtestreadwrite_LDADD=libtestutils.la $(top_builddir)/src/libabigail.la

runtestreaddwarf_SOURCES=test-read-dwarf.cc
runtestreaddwarf_LDADD=libtestutils.la $(top_builddir)/src/libabigail.la
runtestreaddwarf_LDFLAGS=-pthread

runtestannotate_SOURCES=test-annotate.cc
runtestannotate_LDADD=libtestutils.la $(top_builddir)/src/libabigail.la

runtestlookupsyms_SOURCES=test-lookup-syms.cc
runtestlookupsyms_LDADD=libtestutils.la $(top_builddir)/src/libabigail.la

runtestaltdwarf_SOURCES=test-alt-dwarf-file.cc
runtestaltdwarf_LDADD=libtestutils.la $(top_builddir)/src/libabigail.la

runtestcorediff_SOURCES=test-core-diff.cc
runtestcorediff_LDADD=libtestutils.la $(top_builddir)/src/libabigail.la

runtestabidiff_SOURCES = test-abidiff.cc
runtestabidiff_LDADD = libtestutils.la $(top_builddir)/src/libabigail.la

runtestabidiffexit_SOURCES = test-abidiff-exit.cc
runtestabidiffexit_LDADD = libtestutils.la $(top_builddir)/src/libabigail.la

runtestdiffdwarf_SOURCES = test-diff-dwarf.cc
runtestdiffdwarf_LDADD = libtestutils.la $(top_builddir)/src/libabigail.la

runtestdifffilter_SOURCES = test-diff-filter.cc
runtestdifffilter_LDADD = libtestutils.la $(top_builddir)/src/libabigail.la

runtestdiffsuppr_SOURCES = test-diff-suppr.cc
runtestdiffsuppr_LDADD = libtestutils.la $(top_builddir)/src/libabigail.la

runtestdiffdwarfabixml_SOURCES = test-diff-dwarf-abixml.cc
runtestdiffdwarfabixml_LDADD = libtestutils.la $(top_builddir)/src/libabigail.la

runtestabicompat_SOURCES = test-abicompat.cc
runtestabicompat_LDADD = libtestutils.la $(top_builddir)/src/libabigail.la

runtestdiffpkg_SOURCES = test-diff-pkg.cc
runtestdiffpkg_LDADD = libtestutils.la $(top_builddir)/src/libabigail.la

runtesttypesstability_SOURCES = test-types-stability.cc
runtesttypesstability_LDADD = libtestutils.la $(top_builddir)/src/libabigail.la

runtestini_SOURCES = test-ini.cc
runtestini_LDADD = libtestutils.la $(top_builddir)/src/libabigail.la

runtesttoolsutils_SOURCES = test-tools-utils.cc
runtesttoolsutils_LDADD = libtestutils.la $(top_builddir)/src/libabigail.la

runtestkmiwhitelist_SOURCES = test-kmi-whitelist.cc
runtestkmiwhitelist_LDADD = libtestutils.la libcatch.la $(top_builddir)/src/libabigail.la

runtestelfhelpers_SOURCES = test-elf-helpers.cc
runtestelfhelpers_LDADD = libcatch.la $(top_builddir)/src/libabigail.la

runtestcxxcompat_SOURCES = test-cxx-compat.cc
runtestcxxcompat_LDADD = libcatch.la $(top_builddir)/src/libabigail.la

runtestsymtab_SOURCES = test-symtab.cc
runtestsymtab_LDADD = libtestutils.la libcatch.la $(top_builddir)/src/libabigail.la

runtestsymtabreader_SOURCES = test-symtab-reader.cc
runtestsymtabreader_LDADD = libcatch.la $(top_builddir)/src/libabigail.la

runtestsvg_SOURCES=test-svg.cc
runtestsvg_LDADD=$(top_builddir)/src/libabigail.la

runtestdot_SOURCES = test-dot.cc
runtestdot_LDADD = $(top_builddir)/src/libabigail.la

testirwalker_SOURCES=test-ir-walker.cc
testirwalker_LDADD=$(top_builddir)/src/libabigail.la

testdiff2_SOURCES=test-diff2.cc
testdiff2_LDADD=$(top_builddir)/src/libabigail.la

printdifftree_SOURCES = print-diff-tree.cc
printdifftree_LDADD = $(top_builddir)/src/libabigail.la

runtestslowselfcompare_sh_SOURCES =
runtestslowselfcompare.sh$(EXEEXT):

runtestcanonicalizetypes_sh_SOURCES =
runtestcanonicalizetypes.sh$(EXEEXT):

runtestfedabipkgdiff_py_SOURCES =
runtestfedabipkgdiff.py$(EXEEXT):

runtestdefaultsupprs_py_SOURCES =
runtestdefaultsupprs.py$(EXEEXT):

runtestfedabipkgdiffpy3_sh_SOURCES =
runtestfedabipkgdiffpy3.sh$(EXEEXT):

runtestdefaultsupprspy3_sh_SOURCES =
runtestdefaultsupprspy3.sh$(EXEEXT):

AM_CPPFLAGS=-I${abs_top_srcdir}/include -I${abs_top_srcdir}/src \
-I${abs_top_builddir}/include -I${abs_top_srcdir}/tools -fPIC

clean-local: clean-local-check
.PHONY: clean-local-check

clean-local-check:
	-rm -rf ${builddir}/output *.svg *.gv

@VALGRIND_CHECK_RULES@
VALGRIND_SUPPRESSIONS_FILES = ${srcdir}/test-valgrind-suppressions.supp

# These are flags passed to Valgrind so that it follows children
# processes of the test programs.  This is because many test programs
# actually fork libabigail command line tools, and we want to valgrind
# those libabigail command line tools as well.
#
# The problem though is that there are many other command line
# programs that are forked too.  For instance, /bin/sh is forked
# because tests are using the system() function to execute commands.
# So we must tell Valgrind to avoid following a bunch of command line
# programs that we don't care about and that take a lot of time to
# valgrind.
RECURSIVE_VALGRIND_FLAGS = \
--num-callers=30 \
--trace-children=yes \
--trace-children-skip=/bin/diff,/bin/rm,/bin/mkdir,/bin/cd,*cpio,/bin/dpkg,/bin/rpm,/bin/test,/bin/tar,/bin/sed,/bin/ls,/bin/g++,/*collect*,/bin/ld

# Use the target below to run tests under Valgrind's memcheck tool by
# telling it to follow children process, so that it can also check
# libabigail tools that are forked by the tests.  This usually takes a
# while.  So, to launch just one test in this configuration, if you
# are in the top-most source directory you can do:
#
#  make -C <build-directory>/tests check-valgrind-memcheck-recursive TESTS=runtestdiffsuppr

check-valgrind-memcheck-recursive:
	$(MAKE) check-valgrind-memcheck VALGRIND_FLAGS="${RECURSIVE_VALGRIND_FLAGS}"

check-valgrind-helgrind-recursive:
	$(MAKE) check-valgrind-helgrind VALGRIND_FLAGS="${RECURSIVE_VALGRIND_FLAGS}"

check-self-compare:
	$(MAKE) check TESTS=runtestslowselfcompare.sh ENABLE_SLOW_TEST=yes

