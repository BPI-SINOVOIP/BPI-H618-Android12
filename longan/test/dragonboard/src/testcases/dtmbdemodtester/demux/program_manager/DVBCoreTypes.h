#ifndef DVBCORE_TYPES_H
#define DVBCORE_TYPES_H

#ifdef NULL
#undef NULL
#endif
#define NULL 0

#define DVBCORE_SUCCESS 0
#define DVBCORE_FAILURE (-1)

typedef long long int64;
typedef unsigned long long uint64;

typedef int int32;
typedef unsigned int uint32;

typedef short int16;
typedef unsigned short uint16;

typedef unsigned char uint8;
typedef char int8;

typedef unsigned long ulong;
//typedef long long;

typedef char dvbcore_char;
typedef int dvbcore_bool;
typedef void dvbcore_void;
typedef unsigned long dvbcore_size;
typedef signed long dvbcore_ssize;

typedef float dvbcore_float;

typedef struct DVBCoreBufferS DVBCoreBufferT;
typedef struct DVBCoreListNodeS DVBCoreListNodeT;
typedef struct DVBCoreListS DVBCoreListT;

#ifdef AWP_DEBUG
#define DVBCORE_INTERFACE
#else
#define DVBCORE_INTERFACE static inline
#endif

typedef int32 dvbcore_err;

#define DVBCORE_TRUE 1
#define DVBCORE_FALSE 0

#define CedarXMin(a, b) ((a) < (b) ? (a) : (b))

#define DVBCoreOffsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER) 

#define DVBCoreContainerOf(ptr, type, member) ({ \
    const typeof(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - DVBCoreOffsetof(type,member) ); })

enum DVBCoreMediaTypeE
{
    DVBCORE_MEDIA_UNKNOWN = -1,
    DVBCORE_MEDIA_VIDEO = 0,
    DVBCORE_MEDIA_AUDIO,
    DVBCORE_MEDIA_SUBTITLE,
    DVBCORE_MEDIA_DATA,
};

#endif
