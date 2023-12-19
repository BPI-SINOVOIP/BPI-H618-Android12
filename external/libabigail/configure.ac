# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
m4_define([version_major],   [2])
m4_define([version_minor],   [0])

AC_INIT([libabigail],
	[version_major.version_minor],
	[http://sourceware.org/bugzilla],
	[libabigail],
	[http://sourceware.org/libabigail])

AC_PREREQ([2.63])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_SRCDIR([README])
AC_CONFIG_MACRO_DIR([m4])

dnl Include some autoconf macros to check for python modules.
dnl
dnl These macros are coming from the autoconf archive at
dnl http://www.gnu.org/software/autoconf-archive

dnl This one is for the AX_CHECK_PYTHON_MODULES() macro.
m4_include([autoconf-archive/ax_check_python_modules.m4])

dnl These two below are for the AX_PROG_PYTHON_VERSION() module.
m4_include([autoconf-archive/ax_compare_version.m4])
m4_include([autoconf-archive/ax_prog_python_version.m4])

dnl This one is to be able to run "make check-valgrind"
dnl and have unit tests run under  der Valgrind.
m4_include([autoconf-archive/ax_valgrind_check.m4])

AM_INIT_AUTOMAKE([1.11.1 foreign subdir-objects tar-ustar parallel-tests])
AM_MAINTAINER_MODE([enable])

AM_SILENT_RULES([yes])

VERSION_MAJOR=version_major
VERSION_MINOR=version_minor
VERSION_REVISION=0

AC_SUBST(VERSION_MAJOR)
AC_SUBST(VERSION_MINOR)
AC_SUBST(VERSION_REVISION)

dnl This VERSION_SUFFIX environment variable is to allow appending
dnl arbitrary text to the libabigail version string representation.
dnl That is useful to identify custom versions of the library
dnl (e.g. development versions or versions of a particular origin).
dnl
dnl The feature can be enabled by passing VERSION_SUFFIX to `configure`,
dnl e.g.
dnl
dnl   $ configure VERSION_SUFFIX="-dev"
AC_SUBST(VERSION_SUFFIX)

AC_ARG_ENABLE(rpm,
	      AS_HELP_STRING([--enable-rpm=yes|no|auto],
			     [enable the support of rpm in abipkgdiff (default is auto)]),
	      ENABLE_RPM=$enableval,
	      ENABLE_RPM=auto)

AC_ARG_ENABLE(rpm415,
	      AS_HELP_STRING([--enable-rpm415=yes|no|auto],
			     [enable the support of rpm 4.15 or higher in abipkgdiff (default is auto)]),
	      ENABLE_RPM415=$enableval,
	      ENABLE_RPM415=auto)

AC_ARG_ENABLE(deb,
	      AS_HELP_STRING([--enable-deb=yes|no|auto],
			     [enable the support of deb in abipkgdiff (default is auto)]),
	      ENABLE_DEB=$enableval,
	      ENABLE_DEB=auto)

AC_ARG_ENABLE(tar,
	      AS_HELP_STRING([--enable-tar=yes|no|auto],
			     [enable the support of GNU tar archives in abipkgdiff (default is auto)]),
	      ENABLE_TAR=$enableval,
	      ENABLE_TAR=auto)

AC_ARG_ENABLE(apidoc,
	      AS_HELP_STRING([--enable-apidoc=yes|no|auto],
			     [enable generation of the apidoc in html]),
	      ENABLE_APIDOC=$enableval,
	      ENABLE_APIDOC=auto)

AC_ARG_ENABLE(manual,
	      AS_HELP_STRING([--enable-manual=yes|no|auto],
			     [enable generation of the manual in html]),
	      ENABLE_MANUAL=$enableval,
	      ENABLE_MANUAL=auto)

AC_ARG_ENABLE([bash-completion],
	      AS_HELP_STRING([--enable-bash-completion=yes|no|auto],
			     [enable using completion files for tools]),
	      ENABLE_BASH_COMPLETION=$enableval,
	      ENABLE_BASH_COMPLETION=auto)

AC_ARG_ENABLE([fedabipkgdiff],
	      AS_HELP_STRING([--enable-fedabipkgdiff=yes|no|auto],
			     [enable the fedabipkgdiff tool]),
	      ENABLE_FEDABIPKGDIFF=$enableval,
	      ENABLE_FEDABIPKGDIFF=auto)

AC_ARG_ENABLE([python3],
	      AS_HELP_STRING([--enable-python3=yes|no|auto],
			     [enable running abigail tools with python3 (default is auto)]),
	      ENABLE_PYTHON3=$enableval,
	      ENABLE_PYTHON3=auto)

AC_ARG_ENABLE(asan,
	      AS_HELP_STRING([--enable-asan=yes|no],
			     [enable the support of building with -fsanitize=address)]),
	      ENABLE_ASAN=$enableval,
	      ENABLE_ASAN=no)

AC_ARG_ENABLE(msan,
	      AS_HELP_STRING([--enable-msan=yes|no],
			     [enable the support of building with -fsanitize=memory)]),
	      ENABLE_MSAN=$enableval,
	      ENABLE_MSAN=no)

AC_ARG_ENABLE(tsan,
	      AS_HELP_STRING([--enable-tsan=yes|no],
			     [enable the support of building with -fsanitize=thread)]),
	      ENABLE_TSAN=$enableval,
	      ENABLE_TSAN=no)

AC_ARG_ENABLE(ubsan,
	      AS_HELP_STRING([--enable-ubsan=yes|no],
			     [enable the support of building with -fsanitize=undefined)]),
	      ENABLE_UBSAN=$enableval,
	      ENABLE_UBSAN=no)

dnl *************************************************
dnl check for dependencies
dnl *************************************************

AC_PROG_CXX
AC_USE_SYSTEM_EXTENSIONS
AC_PROG_INSTALL

LT_PREREQ([2.2])
LT_INIT

AC_LANG([C++])
AC_LANG_COMPILER_REQUIRE

dnl
dnl We use C++11
dnl
CXX_STANDARD=c++11

dnl
dnl check if the c++ compiler has support __attribute__((visibility("hidden")))
dnl
AC_MSG_NOTICE([checking for GCC visibility attribute support ...])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
struct __attribute__((visibility("hidden"))) Foo
{
  int m0;

  Foo()
   : m0()
  {}
};
                 ]])],
		 [SUPPORTS_GCC_VISIBILITY_ATTRIBUTE=yes],
		 [SUPPORTS_GCC_VISIBILITY_ATTRIBUTE=no]
)

if test x$SUPPORTS_GCC_VISIBILITY_ATTRIBUTE = xyes; then
   AC_MSG_NOTICE([GCC visibility attribute is supported])
   AC_DEFINE([HAS_GCC_VISIBILITY_ATTRIBUTE], 1,
   	     [Defined if the compiler supports the attribution visibility syntax __attribute__((visibility("hidden")))])
   VISIBILITY_FLAGS="-fvisibility=hidden"
else
   AC_MSG_NOTICE([GCC visibility attribute is not supported])
   VISIBILITY_FLAGS=
fi

AC_SUBST(VISIBILITY_FLAGS)

dnl Older glibc had a broken fts that didn't work with Large File Systems.
dnl We want the version that can handler LFS, but include workaround if we
dnl get a bad one. Add define to CFLAGS (not AC_DEFINE it) since we need to
dnl check it before including config.h (which might define _FILE_OFFSET_BITS).
AC_CACHE_CHECK([whether including fts.h with _FILE_OFFSET_BITS set breaks], ac_cv_bad_fts,
  [AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
	#define _FILE_OFFSET_BITS 64
	#include <fts.h>
	]])],
		     ac_cv_bad_fts=no, ac_cv_bad_fts=yes)])
AS_IF([test "x$ac_cv_bad_fts" = "xyes"],
      [CFLAGS="$CFLAGS -DBAD_FTS=1",
       CXXFLAGS="$CXXFLAGS -DBAD_FTS=1"])

dnl Check for dependency: libelf, libdw, libebl (elfutils)
dnl Note that we need to use at least elfutils 0.159 but
dnl at that time elfutils didnt have pkgconfig capabilities
dnl  to easily query for its version.
ELF_LIBS=
AC_CHECK_LIB([elf], [elf_end], [ELF_LIBS="-lelf"])
AC_CHECK_HEADER([libelf.h],
		[],
		[AC_MSG_ERROR([could not find libelf.h])])

DW_LIBS=
AC_CHECK_LIB(dw, dwfl_begin, [DW_LIBS=-ldw])
AC_CHECK_LIB(dw, dwarf_getalt,
	     [FOUND_DWARF_GETALT_IN_LIBDW=yes],
	     [FOUND_DWARF_GETALT_IN_LIBDW=no])

AC_CHECK_HEADER(elfutils/libdwfl.h,
		[],
		[AC_MSG_ERROR([could not find elfutils/libdwfl.h installed])])

dnl Allow users to compile with the NDEBUG macro defined,
dnl meaning they are compiling in a mode where the
dnl assert call does nothing.  With the directive below,
dnl users just need to pass the --disable-assert
dnl option to configure.
AC_HEADER_ASSERT

if test x$ELF_LIBS = x; then
   AC_MSG_ERROR([could not find elfutils elf library installed])
fi

if test x$DW_LIBS = x; then
   AC_MSG_ERROR([could not find elfutils dwarf library installed])
fi

if test x$FOUND_DWARF_GETALT_IN_LIBDW = xyes; then
   AC_DEFINE([LIBDW_HAS_DWARF_GETALT], 1,
	     [Defined if libdw has the function dwarf_getalt])
fi

AC_SUBST(DW_LIBS)
AC_SUBST([ELF_LIBS])

dnl Check for dependency: libxml
LIBXML2_VERSION=2.6.22
PKG_CHECK_MODULES(XML, libxml-2.0 >= $LIBXML2_VERSION)

AC_SUBST(LIBXML2_VERSION)
AC_SUBST(XML_LIBS)
AC_SUBST(XML_CFLAGS)

dnl Check for some programs like rm, mkdir, etc ...
AC_CHECK_PROG(HAS_RM, rm, yes, no)
if test x$HAS_RM = xno; then
   AC_MSG_ERROR([could not find the program 'rm' installed])
fi

AC_CHECK_PROG(HAS_MKDIR, mkdir, yes, no)
if test x$HAS_MKDIR = xno; then
   AC_MSG_ERROR([could not find the program 'mkdir' installed])
fi

dnl Check for the rpm2cpio and cpio programs
if test x$ENABLE_RPM = xyes -o x$ENABLE_RPM = xauto; then
  AC_CHECK_PROG(HAS_RPM2CPIO, rpm2cpio, yes, no)
  AC_CHECK_PROG(HAS_CPIO, cpio, yes, no)
  AC_CHECK_PROG(HAS_RPM, rpm, yes, no)

  if test x$HAS_RPM2CPIO = xyes -a x$HAS_CPIO = xyes -a x$HAS_RPM = xyes; then
     ENABLE_RPM=yes
  else
    if test x$ENABLE_RPM = xyes; then
      AC_MSG_ERROR([rpm support in abipkgdiff needs 'rpm2cpio', 'cpio' and 'rpm' programs to be installed])
    fi
    ENABLE_RPM=no
  fi
fi

if test x$ENABLE_RPM = xyes -a x$ENABLE_RPM415 = xauto; then
   rpmversion=$(rpm --version | sed "s/RPM version //")
   AC_MSG_NOTICE([detected rpm version: $rpmversion])
   if [[[ "$rpmversion" > "4.14.0" ]]]; then
      ENABLE_RPM415=yes
   else
      ENABLE_RPM415=no
   fi
fi

if test x$ENABLE_RPM = xyes; then
   AC_DEFINE([WITH_RPM], 1, [compile the rpm package support in abipkgdiff])
   AC_MSG_NOTICE([rpm support in abipkgdiff is enabled]);

   if test x$ENABLE_RPM415 = xyes; then
      AC_DEFINE([WITH_RPM_4_15], 1, [has RPM 4.15 at least])
      AC_MSG_NOTICE([rpm 4.15 support in abipkgdiff tests is enabled])
   fi
else
   AC_MSG_NOTICE([rpm support in abipkgdiff is disabled]);
fi

AM_CONDITIONAL(ENABLE_RPM, test x$ENABLE_RPM = xyes)

dnl Check for the dpkg program
if test x$ENABLE_DEB = xauto -o x$ENABLE_DEB = xyes; then
   AC_CHECK_PROG(HAS_DPKG, dpkg, yes, no)

   if test x$ENABLE_DEB = xauto; then
     if test x$HAS_DPKG = xyes; then
       ENABLE_DEB=yes
     else
       ENABLE_DEB=no
    fi
   fi
fi

if test x$ENABLE_DEB = xyes; then
   AC_DEFINE([WITH_DEB], 1, [compile the deb package support in abipkgdiff])
   AC_MSG_NOTICE(deb support in abipkgdiff is enabled);
else
   AC_MSG_NOTICE(deb support in abipkgdiff is disabled);
fi

AM_CONDITIONAL(ENABLE_DEB, test x$ENABLE_DEB = xyes)

dnl Check for the tar program
if test x$ENABLE_TAR = xauto -o x$ENABLE_TAR = xyes; then
   AC_CHECK_PROG(HAS_TAR, tar, yes, no)

   if test x$ENABLE_TAR = xauto; then
     if test x$HAS_TAR = xyes; then
       ENABLE_TAR=yes
    fi
   fi
fi

if test x$ENABLE_TAR = xyes; then
   AC_DEFINE([WITH_TAR], 1, [compile the GNU tar archive support in abipkgdiff])
   AC_MSG_NOTICE(GNU tar support in abipkgdiff is enabled);
else
   AC_MSG_NOTICE(GNU tar support in abipkgdiff is disabled);
fi

AM_CONDITIONAL(ENABLE_TAR, test x$ENABLE_TAR = xyes)

dnl Check for the bash-completion package
if test x$ENABLE_BASH_COMPLETION = xauto -o x$ENABLE_BASH_COMPLETION = xyes; then
   AC_CHECK_PROG(HAS_BASH_COMPLETION, bash-completion, yes, no)

   if test x$ENABLE_BASH_COMPLETION = xauto; then
     if test x$HAS_BASH_COMPLETION = xyes; then
       ENABLE_BASH_COMPLETION=yes
     else
       ENABLE_BASH_COMPLETION=no
    fi
   fi
fi

if test x$ENABLE_BASH_COMPLETION = xyes; then
   AC_MSG_NOTICE(bash-completion support in libabigail is enabled);
else
   AC_MSG_NOTICE(bash-completion support in libabigail is disabled);
fi

AM_CONDITIONAL(ENABLE_BASH_COMPLETION, test x$ENABLE_BASH_COMPLETION = xyes)

# The minimal python 2 version we want to support is 2.6.6 because EL6
# distributions have that version installed.
MINIMAL_PYTHON2_VERSION="2.6.6"

AC_PATH_PROG(PYTHON, python, no)
AX_PROG_PYTHON_VERSION($MINIMAL_PYTHON2_VERSION,
			 [MINIMAL_PYTHON_VERSION_FOUND=yes],
			 [MINIMAL_PYTHON_VERSION_FOUND=no])

# The minimal python 3 version we want to support is 3.5, which is
# available in Fedora releases and in EL7.
if test x$ENABLE_PYTHON3 != xno; then
  AC_CHECK_PROGS(PYTHON3_INTERPRETER, [python3 python3.5 python3.6 python3.7], no)
else
  PYTHON3_INTERPRETER=no
fi

if test x$ENABLE_PYTHON3 = xauto; then
    if test x$PYTHON3_INTERPRETER != xno; then
      ENABLE_PYTHON3=yes
    else
      # When enabling python3 is auto, tests only run if the
      # python3 interpreter was found on the system. Otherwise,
      # just ignore it.
	ENABLE_PYTHON3=no
      AC_MSG_NOTICE([Python 3 was not found. Skip running tests with Python 3.])
    fi
fi

if test x$ENABLE_PYTHON3 = xyes; then
    if  test x$PYTHON3_INTERPRETER != xno; then
      # We were asked to enable python3 implicitely (auto and
      # python3 was found) or explicitly.  So enable running tests
      # using python3 then.
      RUN_TESTS_WITH_PY3=yes
    else
       AC_MSG_ERROR([Python 3 was not found])
    fi
fi

if test x$PYTHON3_INTERPRETER = xyes; then
   MINIMAL_PYTHON_VERSION_FOUND=yes
fi

if test x$MINIMAL_PYTHON_VERSION_FOUND = xno; then
  AC_MSG_NOTICE([no minimal version of python found])
  if test x$PYTHON = xno; then
     AC_MSG_NOTICE([python binary wasn't found])
     if test x$PYTHON3_INTERPRETER != xno; then
     	  AC_MSG_NOTICE([using $PYTHON3_INTERPRETER instead])
	  PYTHON=$PYTHON3_INTERPRETER
	  MINIMAL_PYTHON_VERSION_FOUND=yes
	  MISSING_FEDABIPKGDIFF_DEP=no
     fi
  fi
else
  AC_MSG_NOTICE([a minimal version of python was found ...])
  if test x$PYTHON3_INTERPRETER != xno; then
   # We were instructed to use python3 and it's present on the
   # system.  Let's update the PYTHON variable that points to the
   # actual python interpreter we are going to be using
   AC_MSG_NOTICE([... and it was $PYTHON3_INTERPRETER])
   PYTHON=$PYTHON3_INTERPRETER
  fi
fi

dnl if --enable-fedabipkgdiff has the 'auto' value, then check for the required
dnl python modules.  If they are present, then enable the fedabipkgdiff program.
dnl If they are not then disable the program.
dnl
dnl If --enable-fedabipkgdiff has the 'yes' value, then check for the required
dnl python modules and whatever dependency fedabipkgdiff needs.  If they are
dnl not present then the configure script will error out.

if test x$ENABLE_FEDABIPKGDIFF = xauto -o x$ENABLE_FEDABIPKGDIFF = xyes; then
   CHECK_DEPS_FOR_FEDABIPKGDIFF=yes
else
   CHECK_DEPS_FOR_FEDABIPKGDIFF=no
fi

if test x$CHECK_DEPS_FOR_FEDABIPKGDIFF = xyes; then
  MISSING_FEDABIPKGDIFF_DEP=no

  if test x$ENABLE_FEDABIPKGDIFF = xyes; then
     MISSING_FEDABIPKGDIFF_DEP_FATAL=yes
  else
     MISSING_FEDABIPKGDIFF_DEP_FATAL=no
  fi

  AC_PATH_PROG(WGET, wget, no)

  if test x$WGET = xno; then
    ENABLE_FEDABIPKGDIFF=no
    if test x$MISSING_FEDABIPKGDIFF_DEP_FATAL = xyes; then
      AC_MSG_ERROR(could not find the wget program)
    else
      MISSING_FEDABIPKGDIFF_DEP=yes
      AC_MSG_NOTICE([could not find the wget program])
      AC_MSG_NOTICE([disabling fedabipkgdiff as a result])
    fi
  fi

  if test x$MINIMAL_PYTHON_VERSION_FOUND = xno; then
    MISSING_FEDABIPKGDIFF_DEP=yes
    if test x$MISSING_FEDABIPKGDIFF_DEP_FATAL = xyes; then
      AC_MSG_ERROR([could not find a python program of version at least $MINIMAL_PYTHON2_VERSION])
    fi
  fi

  ###################################################################
  # Now we are going to check the presence of the required python
  # modules using either python2 or python3 as required until now.
  ###################################################################

  # Grrr, the urlparse python2 module got renamed in python3 as
  # urllib.parse.  Oh well.
  if test x$PYTHON = xpython3; then
     URLPARSE_MODULE=urllib.parse
  else
     URLPARSE_MODULE=urlparse
  fi

  REQUIRED_PYTHON_MODULES_FOR_FEDABIPKGDIFF="\
   argparse logging os re subprocess sys $URLPARSE_MODULE \
   xdg koji mock rpm imp tempfile mimetypes shutil six"

  if test x$ENABLE_FEDABIPKGDIFF != xno; then
    AX_CHECK_PYTHON_MODULES([$REQUIRED_PYTHON_MODULES_FOR_FEDABIPKGDIFF],
			    [$PYTHON],
			    [FOUND_ALL_PYTHON_MODULES=yes],
			    [FOUND_ALL_PYTHON_MODULES=no])

    if test x$FOUND_ALL_PYTHON_MODULES = xno; then
       MISSING_FEDABIPKGDIFF_DEP=yes
       if test x$MISSING_FEDABIPKGDIFF_DEP_FATAL = xyes; then
         AC_MSG_ERROR([missing python modules: $MISSING_PYTHON_MODULES]);
       else
         AC_MSG_NOTICE([missing python modules: $MISSING_PYTHON_MODULES])
         AC_MSG_NOTICE([disabling fedabipkgdiff as a result])
       fi
       ENABLE_FEDABIPKGDIFF=no
    else
	# On some old platforms, the koji client object doesn't have
	# the required .read_config method.  Alas, that module doesn't
	# have any __version__ string either.  So we do as well as we
	# can to try and detect that case and disable fedabipkgdiff if
	# necessary.
        AC_MSG_CHECKING([checking if koji client is recent enough ...])
	$PYTHON -c "
import koji
koji.read_config('koji')"
	if test $? -eq 0; then
	   koji_version_check_ok=yes
	else
	   koji_version_check_ok=no
	fi

	if test x$koji_version_check_ok = xno; then
	  AC_MSG_RESULT([no, disabling fedpkgdiff])
	  MISSING_FEDABIPKGDIFF_DEP=yes
	else
          AC_MSG_RESULT(yes)
       fi
       if test x$MISSING_FEDABIPKGDIFF_DEP = xno; then
         ENABLE_FEDABIPKGDIFF=yes
       fi
    fi
  fi
fi

AM_CONDITIONAL(ENABLE_FEDABIPKGDIFF, test x$ENABLE_FEDABIPKGDIFF = xyes)
AM_CONDITIONAL(ENABLE_RUNNING_TESTS_WITH_PY3, test x$RUN_TESTS_WITH_PY3 = xyes)
AM_CONDITIONAL(ENABLE_PYTHON3_INTERPRETER, test x$PYTHON3_INTERPRETER != xno)
AC_SUBST(PYTHON)

DEPS_CPPFLAGS="$XML_CFLAGS"
AC_SUBST(DEPS_CPPFLAGS)

dnl Check for the presence of doxygen program

if test x$ENABLE_APIDOC != xno; then
  AC_CHECK_PROG(FOUND_DOXYGEN, doxygen, yes, no)
  if test x$ENABLE_APIDOC = xauto; then
    if test x$FOUND_DOXYGEN = xyes; then
      ENABLE_APIDOC=yes
    else
      ENABLE_APIDOC=no
    fi
  fi
fi
AM_CONDITIONAL(ENABLE_APIDOC, test x$ENABLE_APIDOC = xyes)

dnl Check for the presence of the sphinx-build program

if test x$ENABLE_MANUAL != xno; then
  AC_CHECK_PROG(FOUND_SPHINX_BUILD, sphinx-build, yes, no)
  if test x$ENABLE_MANUAL = xauto; then
    if test x$FOUND_SPHINX_BUILD = xyes; then
      ENABLE_MANUAL=yes
    else
      ENABLE_MANUAL=no
    fi
  fi
fi
AM_CONDITIONAL(ENABLE_MANUAL, test x$ENABLE_MANUAL = xyes)

dnl Check for the presence of Valgrind and do the plumbing to allow
dnl the running of "make check-valgrind".
AX_VALGRIND_DFLT(memcheck, on)
AX_VALGRIND_DFLT(helgrind, on)
AX_VALGRIND_DFLT(drd, off)
AX_VALGRIND_DFLT(sgcheck, off)

AX_VALGRIND_CHECK

dnl Set the list of libraries libabigail depends on

DEPS_LIBS="$XML_LIBS $ELF_LIBS $DW_LIBS"
AC_SUBST(DEPS_LIBS)

if test x$ABIGAIL_DEVEL != x; then
   CFLAGS="-g -Og -Wall -Wextra -Werror -D_FORTIFY_SOURCE=2"
   CXXFLAGS="-g -Og -Wall -Wextra -Werror -D_FORTIFY_SOURCE=2 -D_GLIBCXX_DEBUG"
fi

if test x$ABIGAIL_DEBUG != x; then
    CFLAGS="$CFLAGS -Og -g3 -ggdb"
    CXXFLAGS="$CXXFLAGS -Og -g3 -ggdb"
fi

if test x$ABIGAIL_NO_OPTIMIZATION_DEBUG != x; then
   CFLAGS="-g -O0 -Wall -Wextra -Werror"
   CXXFLAGS="-g -O0 -Wall -Wextra -Werror"
fi

if test x$ENABLE_ASAN = xyes; then
    CFLAGS="$CFLAGS -fsanitize=address"
    CXXFLAGS="$CXXFLAGS -fsanitize=address"
fi

if test x$ENABLE_MSAN = xyes; then
    CFLAGS="$CFLAGS -fsanitize=memory -fsanitize-memory-track-origins"
    CXXFLAGS="$CXXFLAGS -fsanitize=memory -fsanitize-memory-track-origins"
fi

if test x$ENABLE_TSAN = xyes; then
    CFLAGS="$CFLAGS -fsanitize=thread"
    CXXFLAGS="$CXXFLAGS -fsanitize=thread"
fi

if test x$ENABLE_UBSAN = xyes; then
    CFLAGS="$CFLAGS -fsanitize=undefined"
    CXXFLAGS="$CXXFLAGS -fsanitize=undefined"
fi

dnl Set the level of C++ standard we use.
CXXFLAGS="$CXXFLAGS -std=$CXX_STANDARD"

dnl Check if several decls and constant are defined in dependant
dnl libraries
HAS_EM_AARCH64=no
AC_CHECK_DECL([EM_AARCH64],
              [HAS_EM_AARCH64=yes],
              [HAS_EM_AARCH64=no],
              [[#include <elf.h>]])

if test x$HAS_EM_AARCH64 = xyes; then
  AC_DEFINE([HAVE_EM_AARCH64_MACRO],
                 1,
            [Defined to 1 if elf.h has EM_AARCH64 macro defined])
fi

HAS_EM_TILEPRO=no
AC_CHECK_DECL([EM_TILEPRO],
              [HAS_EM_TILEPRO=yes],
              [HAS_EM_TILEPRO=no],
              [[#include <elf.h>]])

if test x$HAS_EM_TILEPRO = xyes; then
  AC_DEFINE([HAVE_EM_TILEPRO_MACRO],
                 1,
            [Defined to 1 if elf.h has EM_TILEPR0 macro defined])
fi

HAS_EM_TILEGX=no
AC_CHECK_DECL([EM_TILEGX],
              [HAS_EM_TILEGX=yes],
              [HAS_EM_TILEGX=no],
              [[#include <elf.h>]])

if test x$HAS_EM_TILEGX = xyes; then
  AC_DEFINE([HAVE_EM_TILEGX_MACRO],
                 1,
            [Defined to 1 if elf.h has EM_TILEGX macro defined])
fi

HAS_R_AARCH64_ABS64=no
AC_CHECK_DECL([R_AARCH64_ABS64],
	      [HAS_R_AARCH64_ABS64=yes],
	      [HAS_R_AARCH64_ABS64=no],
	      [[#include <elf.h>]])

if test x$HAS_R_AARCH64_ABS64 = xyes; then
   AC_DEFINE([HAVE_R_AARCH64_ABS64_MACRO],
	     1,
	     [Defined to 1 if elf.h has R_AARCH64_ABS64 macro defined])
fi

HAS_R_AARCH64_PREL32=no
AC_CHECK_DECL([R_AARCH64_PREL32],
	      [HAS_R_AARCH64_PREL32=yes],
	      [HAS_R_AARCH64_PREL32=no],
	      [[#include <elf.h>]])

if test x$HAS_R_AARCH64_PREL32 = xyes; then
   AC_DEFINE([HAVE_R_AARCH64_PREL32_MACRO],
	     1,
	     [Defined to 1 if elf.h has R_AARCH64_PREL32 macro defined])
fi

HAS_DW_LANG_UPC=no
AC_CHECK_DECL([DW_LANG_UPC],
	      [HAS_DW_LANG_UPC=yes],
	      [HAS_DW_LANG_UPC=no],
	      [[#include <dwarf.h>]])
if test x$HAS_DW_LANG_UPC = xyes; then
   AC_DEFINE([HAVE_DW_LANG_UPC_enumerator],
	     1,
	     [Define to 1 if dwarf.h has the DW_LANG_UPC enumerator])
fi

HAS_DW_LANG_D=no
AC_CHECK_DECL([DW_LANG_D],
	      [HAS_DW_LANG_D=yes],
	      [HAS_DW_LANG_D=no],
	      [[#include <dwarf.h>]])
if test x$HAS_DW_LANG_D = xyes; then
   AC_DEFINE([HAVE_DW_LANG_D_enumerator],
             1,
	     [Define to 1 if dwarf.h has the DW_LANG_D enumerator])
fi

HAS_DW_LANG_Python=no
AC_CHECK_DECL([DW_LANG_Python],
	      [HAS_DW_LANG_Python=yes],
	      [HAS_DW_LANG_Python=no],
	      [[#include <dwarf.h>]])
if test x$HAS_DW_LANG_Python = xyes; then
   AC_DEFINE([HAVE_DW_LANG_Python_enumerator],
             1,
	     [Define to 1 if dwarf.h has the DW_LANG_Python enumerator])
fi

HAS_DW_LANG_Go=no
AC_CHECK_DECL([DW_LANG_Go],
	      [HAS_DW_LANG_Go=yes],
	      [HAS_DW_LANG_Go=no],
	      [[#include <dwarf.h>]])
if test x$HAS_DW_LANG_Go = xyes; then
   AC_DEFINE([HAVE_DW_LANG_Go_enumerator],
             1,
	     [Define to 1 if dwarf.h has the DW_LANG_Go enumerator])
fi

HAS_DW_LANG_C11=no
AC_CHECK_DECL([DW_LANG_C11],
	      [HAS_DW_LANG_C11=yes],
	      [HAS_DW_LANG_C11=no],
	      [[#include <dwarf.h>]])
if test x$HAS_DW_LANG_C11 = xyes; then
   AC_DEFINE([HAVE_DW_LANG_C11_enumerator],
             1,
	     [Define to 1 if dwarf.h has the DW_LANG_C11 enumerator])
fi

HAS_DW_LANG_C_plus_plus_03=no
AC_CHECK_DECL([DW_LANG_C_plus_plus_03],
	      [HAS_DW_LANG_C_plus_plus_03=yes],
	      [HAS_DW_LANG_C_plus_plus_03=no],
	      [[#include <dwarf.h>]])
if test x$HAS_DW_LANG_C_plus_plus_03 = xyes; then
   AC_DEFINE([HAVE_DW_LANG_C_plus_plus_03_enumerator],
             1,
	     [Define to 1 if dwarf.h has the DW_LANG_C_plus_plus_03 enumerator])
fi

HAS_DW_LANG_C_plus_plus_11=no
AC_CHECK_DECL([DW_LANG_C_plus_plus_11],
	      [HAS_DW_LANG_C_plus_plus_11=yes],
	      [HAS_DW_LANG_C_plus_plus_11=no],
	      [[#include <dwarf.h>]])
if test x$HAS_DW_LANG_C_plus_plus_11 = xyes; then
   AC_DEFINE([HAVE_DW_LANG_C_plus_plus_11_enumerator],
             1,
	     [Define to 1 if dwarf.h has the DW_LANG_C_plus_plus_11 enumerator])
fi

HAS_DW_LANG_C_plus_plus_14=no
AC_CHECK_DECL([DW_LANG_C_plus_plus_14],
	      [HAS_DW_LANG_C_plus_plus_14=yes],
	      [HAS_DW_LANG_C_plus_plus_14=no],
	      [[#include <dwarf.h>]])
if test x$HAS_DW_LANG_C_plus_plus_14 = xyes; then
   AC_DEFINE([HAVE_DW_LANG_C_plus_plus_14_enumerator],
             1,
	     [Define to 1 if dwarf.h has the DW_LANG_C_plus_plus_14 enumerator])
fi

HAS_DW_LANG_Mips_Assembler=no
AC_CHECK_DECL([DW_LANG_Mips_Assembler],
	      [HAS_DW_LANG_Mips_Assembler=yes],
	      [HAS_DW_LANG_Mips_Assembler=no],
	      [[#include <dwarf.h>]])
if test x$HAS_DW_LANG_Mips_Assembler = xyes; then
   AC_DEFINE([HAVE_DW_LANG_Mips_Assembler_enumerator],
             1,
	     [Define to 1 if dwarf.h has the DW_LANG_Mips_Assembler enumerator])
fi

HAS_DW_LANG_Rust=no
AC_CHECK_DECL([DW_LANG_Rust],
	      [HAS_DW_LANG_Rust=yes],
	      [HAS_DW_LANG_Rust=no],
	      [[#include <dwarf.h>]])

if test x$HAS_DW_LANG_Rust = xyes; then
  AC_DEFINE([HAVE_DW_LANG_Rust_enumerator], 
  	    1,
	    [Define to 1 if dwarf.h has the DW_LANG_Rust enumerator])
fi

HAS_DW_FORM_strx1=no
HAS_DW_FORM_strx2=no
HAS_DW_FORM_strx3=no
HAS_DW_FORM_strx4=no
HAS_DW_FORM_line_strp=no

AC_CHECK_DECL([DW_FORM_strx1],
	      [HAS_DW_FORM_strx1=yes],
	      [HAS_DW_FORM_strx1=no],
	      [[#include <dwarf.h>]])

if test x$HAS_DW_FORM_strx1 = xyes; then
   AC_DEFINE([HAVE_DW_FORM_strx1],
   	     1,
	     [Define to 1 if dwarf.h has the DW_FORM_strx1 enumerator])
fi

AC_CHECK_DECL([DW_FORM_strx2],
	      [HAS_DW_FORM_strx2=yes],
	      [HAS_DW_FORM_strx2=no],
	      [[#include <dwarf.h>]])

if test x$HAS_DW_FORM_strx2 = xyes; then
   AC_DEFINE([HAVE_DW_FORM_strx2],
   	     1,
	     [Define to 1 if dwarf.h has the DW_FORM_strx2 enumerator])
fi

AC_CHECK_DECL([DW_FORM_strx3],
	      [HAS_DW_FORM_strx3=yes],
	      [HAS_DW_FORM_strx3=no],
	      [[#include <dwarf.h>]])

if test x$HAS_DW_FORM_strx3 = xyes; then
   AC_DEFINE([HAVE_DW_FORM_strx3],
   	     1,
	     [Define to 1 if dwarf.h has the DW_FORM_strx3 enumerator])
fi

AC_CHECK_DECL([DW_FORM_strx4],
	      [HAS_DW_FORM_strx4=yes],
	      [HAS_DW_FORM_strx4=no],
	      [[#include <dwarf.h>]])

if test x$HAS_DW_FORM_strx4 = xyes; then
   AC_DEFINE([HAVE_DW_FORM_strx4],
   	     1,
	     [Define to 1 if dwarf.h has the DW_FORM_strx4 enumerator])
fi

AC_CHECK_DECL([DW_FORM_line_strp],
	      [HAS_DW_FORM_line_strp=yes],
	      [HAS_DW_FORM_line_strp=no],
	      [[#include <dwarf.h>]])

if test x$HAS_DW_FORM_line_strp = xyes; then
   AC_DEFINE([HAVE_DW_FORM_line_strp],
   	     1,
	     [Define to 1 if dwarf.h has the DW_FORM_line_strp enumerator])
fi

if test x$HAS_DW_FORM_strx1 = xyes -a \
	x$HAS_DW_FORM_strx2 = xyes -a \
	x$HAS_DW_FORM_strx3 = xyes -a \
	x$HAS_DW_FORM_strx4 = xyes ; then
   AC_DEFINE([HAVE_DW_FORM_strx],
   	     1,
	     [Define to 1 if dwarf.h has the DW_FORM_strx enumerators])
fi

dnl Set large files support
AC_SYS_LARGEFILE

AC_CONFIG_FILES([Makefile
libabigail.pc
  include/Makefile
  include/abg-version.h
  doc/Makefile
    doc/manuals/Makefile
  src/Makefile
  tools/Makefile
  tests/Makefile
    tests/data/Makefile
    bash-completion/Makefile])

dnl Some test scripts are generated by autofoo.
AC_CONFIG_FILES([tests/runtestcanonicalizetypes.sh],
		[chmod +x tests/runtestcanonicalizetypes.sh])
		AC_CONFIG_FILES([tests/runtestslowselfcompare.sh],
		[chmod +x tests/runtestslowselfcompare.sh])
AC_CONFIG_FILES([tests/mockfedabipkgdiff],
		[chmod +x tests/mockfedabipkgdiff])
AC_CONFIG_FILES([tests/runtestfedabipkgdiff.py],
		[chmod +x tests/runtestfedabipkgdiff.py])
AC_CONFIG_FILES([tests/runtestfedabipkgdiffpy3.sh],
		[chmod +x tests/runtestfedabipkgdiffpy3.sh])
AC_CONFIG_FILES([tests/runtestdefaultsupprs.py],
		[chmod +x tests/runtestdefaultsupprs.py])
AC_CONFIG_FILES([tests/runtestdefaultsupprspy3.sh],
		[chmod +x tests/runtestdefaultsupprspy3.sh])

AC_OUTPUT

AC_MSG_NOTICE([
=====================================================================
	Libabigail: $VERSION_MAJOR.$VERSION_MINOR.$VERSION_REVISION$VERSION_SUFFIX
=====================================================================

		Here is the configuration of the package:

    Prefix                                         : ${prefix}
    Source code location                           : ${srcdir}
    C Compiler                                     : ${CC}
    C++ Compiler		                   : ${CXX}
    GCC visibility attribute supported             : ${SUPPORTS_GCC_VISIBILITY_ATTRIBUTE}
    CXXFLAGS	   	     			   : ${CXXFLAGS}
    Python					   : ${PYTHON}

 OPTIONAL FEATURES:
    C++ standard level                             : ${CXX_STANDARD}
    libdw has the dwarf_getalt function            : ${FOUND_DWARF_GETALT_IN_LIBDW}
    Enable rpm support in abipkgdiff               : ${ENABLE_RPM}
    Enable rpm 4.15 support in abipkgdiff tests    : ${ENABLE_RPM415}
    Enable deb support in abipkgdiff               : ${ENABLE_DEB}
    Enable GNU tar archive support in abipkgdiff   : ${ENABLE_TAR}
    Enable bash completion	                   : ${ENABLE_BASH_COMPLETION}
    Enable fedabipkgdiff                           : ${ENABLE_FEDABIPKGDIFF}
    Enable python 3				   : ${ENABLE_PYTHON3}
    Enable running tests under Valgrind            : ${enable_valgrind}
    Enable build with -fsanitize=address    	   : ${ENABLE_ASAN}
    Enable build with -fsanitize=memory    	   : ${ENABLE_MSAN}
    Enable build with -fsanitize=thread    	   : ${ENABLE_TSAN}
    Enable build with -fsanitize=undefined  	   : ${ENABLE_UBSAN}
    Generate html apidoc	                   : ${ENABLE_APIDOC}
    Generate html manual	                   : ${ENABLE_MANUAL}
])
