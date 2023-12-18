/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.in by autoheader.  */

/* 7-argument gethostbyaddr_r returns struct hostent* */
/* #undef GETHOSTBYADDR_R_RETURNS_HOSTENT_P */

/* 8-argument gethostbyaddr_r returns int */
//#define GETHOSTBYADDR_R_RETURNS_INT 1

/* C99 fixed-width int types available */
#define HAVE_C99_INTS 1

/* Are we running on a STREAMS system with DLPI? */
/* #undef HAVE_DLPI */

/* Define to 1 if you have the `inet_aton' function. */
#define HAVE_INET_ATON 1

/* Define to 1 if you have the `inet_pton' function. */
#define HAVE_INET_PTON 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `nsl' library (-lnsl). */
/* #undef HAVE_LIBNSL */

/* Define to 1 if you have the `pcap' library (-lpcap). */
#define HAVE_LIBPCAP 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <pcap.h> header file. */
#define HAVE_PCAP_H 1

/* Define to 1 if you have the <pcap/pcap.h> header file. */
/* #undef HAVE_PCAP_PCAP_H */

/* Define to 1 if you have the `regcomp' function. */
#define HAVE_REGCOMP 1

/* Define to 1 if you have the `select' function. */
#define HAVE_SELECT 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strdup' function. */
#define HAVE_STRDUP 1

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strspn' function. */
#define HAVE_STRSPN 1

/* Define to 1 if you have the <sys/inttypes.h> header file. */
/* #undef HAVE_SYS_INTTYPES_H */

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/sockio.h> header file. */
/* #undef HAVE_SYS_SOCKIO_H */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* The iftop version number */
#define IFTOP_VERSION "1.0pre2"

/* Enable default promiscuous mode to capture outgoing packets */
/* #undef NEED_PROMISCUOUS_FOR_OUTGOING */

/* Name of package */
#define PACKAGE "iftop"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME ""

/* Define to the full name and version of this package. */
#define PACKAGE_STRING ""

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME ""

/* Define to the version of this package. */
#define PACKAGE_VERSION ""

/* The size of `unsigned int', as computed by sizeof. */
/* #undef SIZEOF_UNSIGNED_INT */

/* The size of `unsigned long int', as computed by sizeof. */
/* #undef SIZEOF_UNSIGNED_LONG_INT */

/* The size of `unsigned short int', as computed by sizeof. */
/* #undef SIZEOF_UNSIGNED_SHORT_INT */

/* size of u_int16_t */
#define SIZEOF_U_INT16_T 0

/* size of u_int32_t */
#define SIZEOF_U_INT32_T 0

/* size of u_int8_t */
#define SIZEOF_U_INT8_T 0

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#define TIME_WITH_SYS_TIME 1

/* use ARES for name resolution */
/* #undef USE_ARES */

/* use a REALLY SUCKY forking resolver for name resolution */
/* #undef USE_FORKING_RESOLVER */

/* use gethostbyaddr for name resolution */
/* #undef USE_GETHOSTBYADDR */

/* use gethostbyaddr_r for name resolution */
//#define USE_GETHOSTBYADDR_R 1

/* use getnameinfo for name resolution */
#define USE_GETNAMEINFO 1

/* Version number of package */
#define VERSION "1.0pre2"

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */
