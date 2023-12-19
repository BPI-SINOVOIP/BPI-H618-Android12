/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Defined if the compiler supports the attribution visibility syntax
   __attribute__((visibility("hidden"))) */
#define HAS_GCC_VISIBILITY_ATTRIBUTE 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if dwarf.h has the DW_FORM_line_strp enumerator */
#define HAVE_DW_FORM_line_strp 1

/* Define to 1 if dwarf.h has the DW_FORM_strx enumerators */
#define HAVE_DW_FORM_strx 1

/* Define to 1 if dwarf.h has the DW_FORM_strx1 enumerator */
#define HAVE_DW_FORM_strx1 1

/* Define to 1 if dwarf.h has the DW_FORM_strx2 enumerator */
#define HAVE_DW_FORM_strx2 1

/* Define to 1 if dwarf.h has the DW_FORM_strx3 enumerator */
#define HAVE_DW_FORM_strx3 1

/* Define to 1 if dwarf.h has the DW_FORM_strx4 enumerator */
#define HAVE_DW_FORM_strx4 1

/* Define to 1 if dwarf.h has the DW_LANG_C11 enumerator */
#define HAVE_DW_LANG_C11_enumerator 1

/* Define to 1 if dwarf.h has the DW_LANG_C_plus_plus_03 enumerator */
#define HAVE_DW_LANG_C_plus_plus_03_enumerator 1

/* Define to 1 if dwarf.h has the DW_LANG_C_plus_plus_11 enumerator */
#define HAVE_DW_LANG_C_plus_plus_11_enumerator 1

/* Define to 1 if dwarf.h has the DW_LANG_C_plus_plus_14 enumerator */
#define HAVE_DW_LANG_C_plus_plus_14_enumerator 1

/* Define to 1 if dwarf.h has the DW_LANG_D enumerator */
#define HAVE_DW_LANG_D_enumerator 1

/* Define to 1 if dwarf.h has the DW_LANG_Go enumerator */
#define HAVE_DW_LANG_Go_enumerator 1

/* Define to 1 if dwarf.h has the DW_LANG_Mips_Assembler enumerator */
#define HAVE_DW_LANG_Mips_Assembler_enumerator 1

/* Define to 1 if dwarf.h has the DW_LANG_Python enumerator */
#define HAVE_DW_LANG_Python_enumerator 1

/* Define to 1 if dwarf.h has the DW_LANG_Rust enumerator */
#define HAVE_DW_LANG_Rust_enumerator 1

/* Define to 1 if dwarf.h has the DW_LANG_UPC enumerator */
#define HAVE_DW_LANG_UPC_enumerator 1

/* Defined to 1 if elf.h has EM_AARCH64 macro defined */
#define HAVE_EM_AARCH64_MACRO 1

/* Defined to 1 if elf.h has EM_TILEGX macro defined */
#define HAVE_EM_TILEGX_MACRO 1

/* Defined to 1 if elf.h has EM_TILEPR0 macro defined */
#define HAVE_EM_TILEPRO_MACRO 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Defined to 1 if elf.h has R_AARCH64_ABS64 macro defined */
#define HAVE_R_AARCH64_ABS64_MACRO 1

/* Defined to 1 if elf.h has R_AARCH64_PREL32 macro defined */
#define HAVE_R_AARCH64_PREL32_MACRO 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Defined if libdw has the function dwarf_getalt */
#define LIBDW_HAS_DWARF_GETALT 1

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#define LT_OBJDIR ".libs/"

/* Define to 1 if assertions should be disabled. */
/* #undef NDEBUG */

/* Name of package */
#define PACKAGE "libabigail"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "http://sourceware.org/bugzilla"

/* Define to the full name of this package. */
#define PACKAGE_NAME "libabigail"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "libabigail 2.0"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "libabigail"

/* Define to the home page for this package. */
#define PACKAGE_URL "http://sourceware.org/libabigail"

/* Define to the version of this package. */
#define PACKAGE_VERSION "2.0"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Enable extensions on AIX 3, Interix.  */
#ifndef _ALL_SOURCE
# define _ALL_SOURCE 1
#endif
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif
/* Enable threading extensions on Solaris.  */
#ifndef _POSIX_PTHREAD_SEMANTICS
# define _POSIX_PTHREAD_SEMANTICS 1
#endif
/* Enable extensions on HP NonStop.  */
#ifndef _TANDEM_SOURCE
# define _TANDEM_SOURCE 1
#endif
/* Enable general extensions on Solaris.  */
#ifndef __EXTENSIONS__
# define __EXTENSIONS__ 1
#endif


/* Version number of package */
#define VERSION "2.0"

/* compile the deb package support in abipkgdiff */
#define WITH_DEB 1

/* compile the rpm package support in abipkgdiff */
#define WITH_RPM 1

/* has RPM 4.15 at least */
#define WITH_RPM_4_15 1

/* compile the GNU tar archive support in abipkgdiff */
#define WITH_TAR 1

/* Enable large inode numbers on Mac OS X 10.5.  */
#ifndef _DARWIN_USE_64_BIT_INODE
# define _DARWIN_USE_64_BIT_INODE 1
#endif

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Define to 1 if on MINIX. */
/* #undef _MINIX */

/* Define to 2 if the system does not provide POSIX.1 features except with
   this defined. */
/* #undef _POSIX_1_SOURCE */

/* Define to 1 if you need to in order for `stat' and other things to work. */
/* #undef _POSIX_SOURCE */
