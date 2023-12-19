// Copyright (C) 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Utility functions that depend on bytesex. We define htonll and ntohll,
// as well as "Google" versions of all the standards: ghtonl, ghtons, and
// so on. These functions do exactly the same as their standard variants,
// but don't require including the dangerous netinet/in.h.

#ifndef ICING_PORTABLE_ENDIAN_H_
#define ICING_PORTABLE_ENDIAN_H_

#include <cstdint>

// IS_LITTLE_ENDIAN, IS_BIG_ENDIAN
#if defined OS_LINUX || defined OS_ANDROID || defined(__ANDROID__)
// _BIG_ENDIAN
#include <endian.h>

#elif defined(__APPLE__)

// BIG_ENDIAN
#include <machine/endian.h>  // NOLINT(build/include)

/* Let's try and follow the Linux convention */
#define __BYTE_ORDER BYTE_ORDER
#define __LITTLE_ENDIAN LITTLE_ENDIAN
#define __BIG_ENDIAN BIG_ENDIAN

#endif  // operating system

// defines __BYTE_ORDER for MSVC
#ifdef COMPILER_MSVC
#define __BYTE_ORDER __LITTLE_ENDIAN
#define IS_LITTLE_ENDIAN
#else  // COMPILER_MSVC

// define the macros IS_LITTLE_ENDIAN or IS_BIG_ENDIAN
// using the above endian definitions from endian.h if
// endian.h was included
#ifdef __BYTE_ORDER
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define IS_LITTLE_ENDIAN
#endif  // __BYTE_ORDER == __LITTLE_ENDIAN

#if __BYTE_ORDER == __BIG_ENDIAN
#define IS_BIG_ENDIAN
#endif  // __BYTE_ORDER == __BIG_ENDIAN

#else  // __BYTE_ORDER

#if defined(__LITTLE_ENDIAN__)
#define IS_LITTLE_ENDIAN
#elif defined(__BIG_ENDIAN__)
#define IS_BIG_ENDIAN
#endif  // __LITTLE_ENDIAN__ or __BIG_ENDIAN__

#endif  // __BYTE_ORDER
#endif  // COMPILER_MSVC

// byte swap functions (bswap_16, bswap_32, bswap_64).
// byte swap functions reverse the order of bytes, e.g.
//   byteswap of 102030405060708 = 807060504030201
//   byteswap of 1020304 = 4030201

// The following guarantees declaration of the byte swap functions
#ifdef COMPILER_MSVC
#include <stdlib.h>  // NOLINT(build/include)

#define bswap_16(x) _byteswap_ushort(x)
#define bswap_32(x) _byteswap_ulong(x)
#define bswap_64(x) _byteswap_uint64(x)

#elif defined(__APPLE__)
// Mac OS X / Darwin features
#include <libkern/OSByteOrder.h>

#define bswap_16(x) OSSwapInt16(x)
#define bswap_32(x) OSSwapInt32(x)
#define bswap_64(x) OSSwapInt64(x)

#elif defined(__GLIBC__) || defined(__BIONIC__) || defined(__ASYLO__)
#include <byteswap.h>  // IWYU pragma: export

#else  // built-in byteswap functions

static inline uint16 bswap_16(uint16 x) {
#ifdef __cplusplus
  return static_cast<uint16>(((x & 0xFF) << 8) | ((x & 0xFF00) >> 8));
#else   // __cplusplus
  return (uint16)(((x & 0xFF) << 8) | ((x & 0xFF00) >> 8));  // NOLINT
#endif  // __cplusplus
}
#define bswap_16(x) bswap_16(x)
static inline uint32 bswap_32(uint32 x) {
  return (((x & 0xFF) << 24) | ((x & 0xFF00) << 8) | ((x & 0xFF0000) >> 8) |
          ((x & 0xFF000000) >> 24));
}
#define bswap_32(x) bswap_32(x)
static inline uint64 bswap_64(uint64 x) {
  return (((x & (uint64_t)0xFF) << 56) | ((x & (uint64_t)0xFF00) << 40) |
          ((x & (uint64_t)0xFF0000) << 24) | ((x & (uint64_t)0xFF000000) << 8) |
          ((x & (uint64_t)0xFF00000000) >> 8) |
          ((x & (uint64_t)0xFF0000000000) >> 24) |
          ((x & (uint64_t)0xFF000000000000) >> 40) |
          ((x & (uint64_t)0xFF00000000000000) >> 56));
}
#define bswap_64(x) bswap_64(x)

#endif  // end byteswap functions

// Use compiler byte-swapping intrinsics if they are available.  32-bit
// and 64-bit versions are available in Clang and GCC as of GCC 4.3.0.
// The 16-bit version is available in Clang and GCC only as of GCC 4.8.0.
// For simplicity, we enable them all only for GCC 4.8.0 or later.
#if defined(__clang__) || \
    (defined(__GNUC__) && \
     ((__GNUC__ == 4 && __GNUC_MINOR__ >= 8) || __GNUC__ >= 5))

inline uint64_t gbswap_64(uint64_t host_int) {
  return __builtin_bswap64(host_int);
}
inline uint32_t gbswap_32(uint32_t host_int) {
  return __builtin_bswap32(host_int);
}
inline uint16_t gbswap_16(uint16_t host_int) {
  return __builtin_bswap16(host_int);
}

#else  // intrinsics available

inline uint64 gbswap_64(uint64 host_int) {
#if defined(__GNUC__) && defined(__x86_64__) && \
    !(defined(__APPLE__) && defined(__MACH__))
  // Adapted from /usr/include/byteswap.h.  Not available on Mac.
  if (__builtin_constant_p(host_int)) {
    return __bswap_constant_64(host_int);
  } else {
    uint64 result;
    __asm__("bswap %0" : "=r"(result) : "0"(host_int));
    return result;
  }
#elif defined(bswap_64)
  return bswap_64(host_int);
#else   // bswap_64
  return static_cast<uint64>(bswap_32(static_cast<uint32>(host_int >> 32))) |
         (static_cast<uint64>(bswap_32(static_cast<uint32>(host_int))) << 32);
#endif  // bswap_64
}
inline uint32 gbswap_32(uint32 host_int) { return bswap_32(host_int); }
inline uint16 gbswap_16(uint16 host_int) { return bswap_16(host_int); }

#endif  // intrinsics available

#ifdef IS_LITTLE_ENDIAN

// Definitions for ntohl etc. that don't require us to include
// netinet/in.h. We wrap gbswap_32 and gbswap_16 in functions rather
// than just #defining them because in debug mode, gcc doesn't
// correctly handle the (rather involved) definitions of bswap_32.
// gcc guarantees that inline functions are as fast as macros, so
// this isn't a performance hit.
inline uint16_t ghtons(uint16_t x) { return gbswap_16(x); }
inline uint32_t ghtonl(uint32_t x) { return gbswap_32(x); }
inline uint64_t ghtonll(uint64_t x) { return gbswap_64(x); }

#elif defined IS_BIG_ENDIAN

// These definitions are simpler on big-endian machines
// These are functions instead of macros to avoid self-assignment warnings
// on calls such as "i = ghtnol(i);".  This also provides type checking.
inline uint16 ghtons(uint16 x) { return x; }
inline uint32 ghtonl(uint32 x) { return x; }
inline uint64 ghtonll(uint64 x) { return x; }

#else  // bytesex
#error \
    "Unsupported bytesex: Either IS_BIG_ENDIAN or IS_LITTLE_ENDIAN must be defined"  // NOLINT
#endif  // bytesex

#ifndef htonll
// With the rise of 64-bit, some systems are beginning to define this.
#define htonll(x) ghtonll(x)
#endif  // htonll

// ntoh* and hton* are the same thing for any size and bytesex,
// since the function is an involution, i.e., its own inverse.
inline uint16_t gntohs(uint16_t x) { return ghtons(x); }
inline uint32_t gntohl(uint32_t x) { return ghtonl(x); }
inline uint64_t gntohll(uint64_t x) { return ghtonll(x); }

#ifndef ntohll
#define ntohll(x) htonll(x)
#endif  // ntohll

#endif  // ICING_PORTABLE_ENDIAN_H_
