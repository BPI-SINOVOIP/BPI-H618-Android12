## SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
h=$(abs_srcdir)

bin_PROGRAMS = abidiff abilint abidw abicompat abipkgdiff kmidiff

if ENABLE_FEDABIPKGDIFF
  dist_bin_SCRIPTS = fedabipkgdiff
else
  noinst_SCRIPTS = fedabipkgdiff
endif

noinst_PROGRAMS = abisym abinilint

abidiff_SOURCES = abidiff.cc
abidiffdir = $(bindir)
abidiff_LDADD = ../src/libabigail.la

abilint_SOURCES = abilint.cc
abilintdir = $(bindir)
abilint_LDADD = ../src/libabigail.la

abidw_SOURCES = abidw.cc
abidwdir = $(bindir)
abidw_LDADD = ../src/libabigail.la

abisym_SOURCES = abisym.cc
abisymdir = $(bindir)
abisym_LDADD = ../src/libabigail.la

abinilint_SOURCES = binilint.cc
abinilintdir = $(bindir)
abinilint_LDADD = $(abs_top_builddir)/src/libabigail.la

abicompat_SOURCES = abicompat.cc
abicompatdir = $(bindir)
abicompat_LDADD = $(abs_top_builddir)/src/libabigail.la

abipkgdiff_SOURCES = abipkgdiff.cc
abipkgdiffdir = $(bindir)
abipkgdiff_LDADD = $(abs_top_builddir)/src/libabigail.la
abipkgdiff_LDFLAGS = -pthread

kmidiff_SOURCES = kmidiff.cc
kmidiffdir = $(bindir)
kmidiff_LDADD = $(abs_top_builddir)/src/libabigail.la
kmidiff_LDFLAGS = -pthread

AM_CXXFLAGS = \
$(VISIBILITY_FLAGS) -I$(abs_top_srcdir)/include \
-I$(abs_top_srcdir)/tools -fPIC
