/******************************************************************************
 *
 *  Copyright(C), 2015, Xradio Technology Co., Ltd.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  Filename:      type.h
 *
 *  Description:   Data type definition
 *
 ******************************************************************************/

#ifndef _TYPE_H_
#define _TYPE_H_

typedef signed char             s8;
typedef signed short            s16;
typedef signed int              s32;
typedef signed long long        s64;
typedef unsigned char           u8;
typedef unsigned short          u16;
typedef unsigned int            u32;
typedef unsigned long long      u64;

typedef signed char             int8;
typedef signed short            int16;
typedef signed int              int32;
typedef signed long long        int64;
typedef unsigned char           uint8;
typedef unsigned short          uint16;
typedef unsigned int            uint32;
typedef unsigned long long      uint64;

#define RETURN_OK      (0)
#define RETURN_FAIL    (-1)
#define RETURN_TIMEOUT (-2)
#define RETURN_VERFAIL (-3)
#define RETURN_NOIMAGE (-4)

#define SZ_512       (0x00000200U)
#define SZ_1K        (0x00000400U)
#define SZ_2K        (0x00000800U)
#define SZ_4K        (0x00001000U)
#define SZ_8K        (0x00002000U)
#define SZ_16K       (0x00004000U)
#define SZ_32K       (0x00008000U)
#define SZ_64K       (0x00010000U)
#define SZ_128K      (0x00020000U)
#define SZ_256K      (0x00040000U)
#define SZ_512K      (0x00080000U)
#define SZ_1M        (0x00100000U)
#define SZ_2M        (0x00200000U)
#define SZ_4M        (0x00400000U)
#define SZ_8M        (0x00800000U)
#define SZ_16M       (0x01000000U)
#define SZ_32M       (0x02000000U)
#define SZ_64M       (0x04000000U)
#define SZ_128M      (0x08000000U)
#define SZ_256M      (0x10000000U)
#define SZ_512M      (0x20000000U)
#define SZ_1G        (0x40000000U)
#define SZ_2G        (0x80000000U)
#define SZ_4G        (0x0100000000ULL)
#define SZ_8G        (0x0200000000ULL)
#define SZ_16G       (0x0400000000ULL)
#define SZ_32G       (0x0800000000ULL)
#define SZ_64G       (0x1000000000ULL)

#define ARRAY_SIZE(n) (sizeof(n)/sizeof(n[0]))

#define SWAP16(d) (((d & 0xff) << 8) | ((d & 0xff00) >> 8))
#define SWAP32(d) (((d & 0xff) << 24) | ((d & 0xff00) << 8)  \
                | ((d & 0xff0000) >> 8) | ((d & 0xff000000) >> 24))


#endif
