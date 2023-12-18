#ifndef TS_BUFFER_H
#define TS_BUFFER_H

#include <stdint.h>
#include <AwPool.h>

typedef struct TSBufferS TSBufferT;

#ifdef __cplusplus
extern "C" {
#endif

TSBufferT *TSBufferIncref(TSBufferT * buf);

int TSBufferDecref(TSBufferT * buf);

int TSBufferDestroy(TSBufferT * buf);

uint8_t *TSBufferGetbufptr(TSBufferT *buf);

int TSBufferGetbufsize(TSBufferT *buf);

TSBufferT *TSBufferCreate(AwPoolT *pool, uint8_t *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif
