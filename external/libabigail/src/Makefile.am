## SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
lib_LTLIBRARIES=libabigail.la
libabigaildir=$(libdir)

AM_CXXFLAGS = $(VISIBILITY_FLAGS)

VIZ_SOURCES = abg-viz-common.cc	\
		abg-viz-dot.cc	\
		abg-viz-svg.cc

libabigail_la_SOURCES =			\
abg-internal.h				\
abg-traverse.cc				\
abg-ir-priv.h				\
abg-ir.cc				\
abg-corpus-priv.h			\
abg-corpus.cc				\
abg-diff-utils.cc			\
abg-comparison.cc			\
abg-comparison-priv.h			\
abg-reporter-priv.h			\
abg-reporter-priv.cc			\
abg-default-reporter.cc			\
abg-leaf-reporter.cc			\
abg-suppression-priv.h			\
abg-suppression.cc			\
abg-comp-filter.cc			\
abg-reader.cc				\
abg-dwarf-reader.cc			\
abg-libxml-utils.cc			\
abg-hash.cc				\
abg-writer.cc				\
abg-config.cc				\
abg-ini.cc				\
abg-workers.cc				\
abg-tools-utils.cc			\
abg-elf-helpers.h			\
abg-elf-helpers.cc			\
abg-regex.cc				\
abg-symtab-reader.h			\
abg-symtab-reader.cc			\
$(VIZ_SOURCES)

libabigail_la_LIBADD = $(DEPS_LIBS)
libabigail_la_LDFLAGS = -lpthread -Wl,--as-needed -no-undefined

CUSTOM_MACROS = -DABIGAIL_ROOT_SYSTEM_LIBDIR=\"${libdir}\"

AM_CPPFLAGS=\
$(CUSTOM_MACROS) $(DEPS_CPPFLAGS) \
-Wall -I$(abs_top_srcdir) -I$(abs_top_srcdir)/include \
-I$(abs_top_builddir)/include -I$(abs_top_builddir)
