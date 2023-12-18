#pragma once

#ifndef __linux__
// Make sure these are defined and don't change anything if used.
enum {
    SOCK_CLOEXEC = 0,
#ifndef __APPLE__
    O_CLOEXEC = 0
#endif
};
#endif  // !__linux__

#ifdef _MSC_VER

#include <windows.h>
#include <BaseTsd.h>

#include <direct.h>
#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <stdint.h>
#include <sys/stat.h>
#include <time.h>
#include <winsock.h>

typedef SSIZE_T ssize_t;

typedef int mode_t;
#ifdef _WIN64
typedef int64_t pid_t;
#else
typedef int pid_t;
#endif
#define STDIN_FILENO _fileno(stdin)
#define STDOUT_FILENO _fileno(stdout)
#define STDERR_FILENO _fileno(stderr)
#define lseek(a, b, c) _lseek(a, b, c)
#define lseek64 _lseeki64

struct FileTime {
  uint32_t dwLowDateTime;
  uint32_t dwHighDateTime;
};

// Need <dirent.h>

// Define for convenience only in mingw. This is
// convenient for the _access function in Windows.
#define F_OK 0 /* Check for file existence */
#define X_OK 1 /* Check for execute permission (not supported in Windows) */
#define W_OK 2 /* Check for write permission */
#define R_OK 4 /* Check for read permission */

typedef int mode_t;
#ifdef _WIN64
typedef int64_t pid_t;
#else
typedef int pid_t;
#endif
#define STDIN_FILENO _fileno(stdin)
#define STDOUT_FILENO _fileno(stdout)
#define STDERR_FILENO _fileno(stderr)
#define lseek(a, b, c) _lseek(a, b, c)
#define lseek64 _lseeki64

// These functions were deprecated and replaced with ISO C++ conformant ones
// in MSVC 2017.
/*
#define strdup _strdup
#define mkdir _mkdir
#define rmdir _rmdir
#define getcwd _getcwd
#define getpid _getpid
#define close _close
#define open _open
#define read _read
#define write _write
#define creat _creat
*/

// From <fcntl.h>
#define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)

// From <sys/types.h>
typedef int64_t off64_t;

// From <sys/cdefs.h>
#ifdef __cplusplus
#define __BEGIN_DECLS extern "C" {
#define __END_DECLS }
#else
#define __BEGIN_DECLS /* empty */
#define __END_DECLS   /* empty */
#endif


typedef  void (*SystemTime)(FileTime*);

// From <sys/time.h>
struct timezone {
    int tz_minuteswest; /* of Greenwich */
    int tz_dsttime;     /* type of dst correction to apply */
};

// From <strings.h>
#define strcasecmp _stricmp
#define strncasecmp _strnicmp

// From <stdio.h>
#define fseeko64 _fseeki64
#define ftello64 _ftelli64

// From <linux/limits.h>
#define PATH_MAX MAX_PATH

__BEGIN_DECLS


extern SystemTime getSystemTime;
extern int gettimeofday(struct timeval* tp, struct timezone* tz);
extern int asprintf(char** buf, const char* format, ...);
extern int vasprintf(char** buf, const char* format, va_list args);
extern int mkstemp(char* t);

__END_DECLS

#endif
