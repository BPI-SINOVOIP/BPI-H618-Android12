#ifndef MESH_INCLUDE_COMMON_H_
#define MESH_INCLUDE_COMMON_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define UNUSED(expr) do { (void)(expr); } while (0)

#define UNUSED_ATTR __attribute__((unused))


#define SMP_DEBUG FALSE

#define MESH_ERROR(fmt, arg...)  fprintf(stderr, "%s:%d()" fmt "\n", __FILE__,__LINE__, ## arg)
#define MESH_WARNING(fmt, arg...)  fprintf(stderr, fmt"\n", ## arg)
#define MESH_EVENT(fmt, arg...)  fprintf(stderr, fmt"\n", ## arg)
#define MESH_DEBUG(fmt, arg...)  fprintf(stderr, fmt"\n", ## arg)
#define MESH_PRINT(param, ...) printf("%s " param "\n", __func__, ## __VA_ARGS__);

#define STREAM_TO_UINT8_BIG(u8, p)   {u8 = (UINT8)(*(p)); (p) += 1;}
#define STREAM_TO_UINT16_BIG(u16, p) {u16 = ((UINT16)(*((p) + 1)) + (((UINT16)(*(p))) << 8)); (p) += 2;}
#define STREAM_TO_UINT24_BIG(u32, p) {u32 = (((UINT32)(*((p) + 2))) + ((((UINT32)(*((p) + 1)))) << 8) + ((((UINT32)(*(p)))) << 16) ); (p) += 3;}
#define STREAM_TO_UINT32_BIG(u32, p) {u32 = (((UINT32)(*((p) + 3))) + ((((UINT32)(*((p) + 2)))) << 8) + ((((UINT32)(*((p) + 1)))) << 16) + ((((UINT32)(*(p)))) << 24)); (p) += 4;}

#define STREAM_TO_ARRAY32_BIG(a, p)  {register int ijk; register UINT8 *_pa = (UINT8 *)a; for (ijk = 0; ijk < 32; ijk++) *_pa++ = *p++;}
#define STREAM_TO_ARRAY16_BIG(a, p)  {register int ijk; register UINT8 *_pa = (UINT8 *)a; for (ijk = 0; ijk < 16; ijk++) *_pa++ = *p++;}
#define STREAM_TO_ARRAY8_BIG(a, p)   {register int ijk; register UINT8 *_pa = (UINT8 *)a; for (ijk = 0; ijk < 8; ijk++) *_pa++ = *p++;}


#define ARRAY32_TO_STREAM_BIG(p, a)  {register int ijk; for (ijk = 0; ijk < 32; ijk++) *(p)++ = (UINT8) a[ijk];}
#define ARRAY16_TO_STREAM_BIG(p, a)  {register int ijk; for (ijk = 0; ijk < 16; ijk++) *(p)++ = (UINT8) a[ijk];}
#define ARRAY8_TO_STREAM_BIG(p, a)   {register int ijk; for (ijk = 0; ijk < 8; ijk++) *(p)++ = (UINT8) a[ijk];}



#define UINT32_TO_STREAM_BIG(p, u32) {*(p)++ = (UINT8)((u32) >> 24); *(p)++ = (UINT8)((u32) >> 16); *(p)++ = (UINT8)((u32) >> 8);*(p)++ = (UINT8)(u32);}
#define UINT24_TO_STREAM_BIG(p, u24) {*(p)++ = (UINT8)((u24) >> 16); *(p)++ = (UINT8)((u24) >> 8); *(p)++ = (UINT8)(u24);}
#define UINT16_TO_STREAM_BIG(p, u16) {*(p)++ = (UINT8)((u16) >> 8); *(p)++ = (UINT8)(u16);}
#define UINT8_TO_STREAM_BIG(p, u8)   {*(p)++ = (UINT8)(u8);}





#endif  // MESH_INCLUDE_COMMON_H_
