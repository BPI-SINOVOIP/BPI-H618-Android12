#ifndef DVBCORE_BUFFER_H
#define DVBCORE_BUFFER_H
#include <DVBCoreTypes.h>
#include <DVBCoreLog.h>
//#include <DVBCoreMeta.h>
#include <DVBCoreAwPool.h>
#include <DVBCoreList.h>
#include <string.h>

/*
  *下面函数，返回失败的时候，对应的buf 还是合法，
  *可以继续使用，当然也要正常释放。
  */

struct DVBCoreBufferItemS
{
    DVBCoreBufferT *val;
    DVBCoreListNodeT node;
};

struct DVBCoreBufferOpsS
{
    dvbcore_void (*decRef)(DVBCoreBufferT *);
    uint8 *(*getData)(DVBCoreBufferT *);
    uint32 (*getSize)(DVBCoreBufferT *);
    dvbcore_void (*append)(DVBCoreBufferT *, const uint8 * /*data*/, uint32 /*len*/ );
    dvbcore_void (*setRange)(DVBCoreBufferT *, uint32 /*offset*/, uint32 /*len*/);
};

struct DVBCoreBufferS
{
    struct DVBCoreBufferOpsS *ops;
};

static inline uint8 *DVBCoreBufferGetData(DVBCoreBufferT *buf)
{
    DVBCORE_CHECK(buf);
    DVBCORE_CHECK(buf->ops);
    DVBCORE_CHECK(buf->ops->getData);
    return buf->ops->getData(buf);
}

static inline uint32 DVBCoreBufferGetSize(DVBCoreBufferT *buf)
{
    DVBCORE_CHECK(buf);
    DVBCORE_CHECK(buf->ops);
    DVBCORE_CHECK(buf->ops->getSize);
    return buf->ops->getSize(buf);
}

static inline dvbcore_void DVBCoreBufferAppend(DVBCoreBufferT *buf, const uint8 *data, 
                                    uint32 len)
{
    DVBCORE_CHECK(buf);
    DVBCORE_CHECK(buf->ops);
    DVBCORE_CHECK(buf->ops->append);
    buf->ops->append(buf, data, len);
}

static inline dvbcore_void DVBCoreBufferSetRange(DVBCoreBufferT *buf, 
                                    uint32 offset, uint32 len)
{
    DVBCORE_CHECK(buf);
    DVBCORE_CHECK(buf->ops);
    DVBCORE_CHECK(buf->ops->setRange);
    buf->ops->setRange(buf, offset, len);
}

static inline dvbcore_void DVBCoreBufferDecRef(DVBCoreBufferT *buf)
{
    DVBCORE_CHECK(buf);
    DVBCORE_CHECK(buf->ops);
    DVBCORE_CHECK(buf->ops->decRef);
    return buf->ops->decRef(buf);
}

#define DVBCoreBufferCreate(pool, capacity, initData, len) \
    __DVBCoreBufferCreate(pool, capacity, initData, len, __FILE__, __LINE__)
    
#ifdef __cplusplus
extern "C" {
#endif

DVBCoreBufferT *__DVBCoreBufferCreate(AwPoolT *pool, uint32 capacity, uint8 *initData, 
                            uint32 len, char *file, int line);

dvbcore_void DVBCoreBufferDestroy(DVBCoreBufferT *buf);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  // DVBCORE_BUFFER_H
