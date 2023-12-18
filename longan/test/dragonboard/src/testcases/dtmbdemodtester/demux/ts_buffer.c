
#define LOG_TAG "Miracast"
#include <stdlib.h>
#include <CdxAtomic.h>
#include <ts_buffer.h>
#include <stdio.h>
#include <cdx_log.h>
struct TSBufferS
{
    cdx_atomic_t ref;
    uint8_t *ptr;
    int size;
    AwPoolT *pool;
};

static void onBufferDestroy(TSBufferT * buf)
{
    Pfree(buf->pool, buf->ptr);
    Pfree(buf->pool, buf);
    return;
}

TSBufferT *TSBufferIncref(TSBufferT * buf)
{
    CdxAtomicInc(&buf->ref);
    return buf;
}

int TSBufferDecref(TSBufferT * buf)
{
    if (CdxAtomicDec(&buf->ref) == 0)
    {
        onBufferDestroy(buf);
    }
    return 0;
}

int TSBufferDestroy(TSBufferT * buf)
{
// if buf->ref != 1, should check code
    CDX_CHECK(CdxAtomicRead(&buf->ref) == 1);
    TSBufferDecref(buf);
    return 0;
}

uint8_t *TSBufferGetbufptr(TSBufferT *buf)
{
    return buf->ptr;
}

int TSBufferGetbufsize(TSBufferT *buf)
{
    return buf->size;
}

TSBufferT *TSBufferCreate(AwPoolT *pool, uint8_t *data, size_t size)
{
    struct TSBufferS *buf = NULL;

    buf = Palloc(pool, sizeof(*buf));
    if (!buf)
    {
        printf("err\n");
        return NULL;
    }

    buf->pool = pool;
    buf->size = size;
    buf->ptr = Palloc(pool, size);
    if (!buf->ptr)
    {
        printf("alloc mem failure, size '%u')", size);
        goto err_out;
    }

	memcpy(buf->ptr, data, size);

    CdxAtomicSet(&buf->ref, 1);

    return buf;

err_out:
    if (buf)
    {
        Pfree(pool, buf);
    }
    return NULL;
}

