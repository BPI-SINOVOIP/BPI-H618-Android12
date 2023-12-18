#include <DVBCoreTypes.h>
#include <DVBCoreAtomic.h>
#include <DVBCoreBuffer.h>

struct DVBCoreBufferImplS
{
    struct DVBCoreBufferS base;

    AwPoolT *pool;
    dvbcore_bool selfPool;
    
    uint8 *mData;
    uint32 mCapacity;
    uint32 mRangeOffset;
    uint32 mRangeLen;
    dvbcore_atomic_t mRef;
};

static uint32 Align2Power(uint32 val)
{
    uint32 ret = 1024;
    while (ret < val)
    {
       ret = ret << 2; 
    }
    return ret;
}

static dvbcore_void onWrite(struct DVBCoreBufferImplS *impl, uint32 offset, 
                                const uint8 *data, uint32 len)
{
    if (impl->mCapacity < offset + len)
    {
        impl->mCapacity = Align2Power(offset + len);
        impl->mData = Prealloc(impl->pool, impl->mData, impl->mCapacity);
        DVBCORE_FORCE_CHECK(impl->mData);
    }
    memcpy(impl->mData + offset, data, len);
    return ;
}

static dvbcore_void onSetRange(struct DVBCoreBufferImplS *impl, 
                            uint32 offset, uint32 len)
{    
    DVBCORE_CHECK(offset + len <= impl->mCapacity);
    impl->mRangeOffset = offset;
    impl->mRangeLen = len;
    return ;
}

static uint8 *__DVBCoreBufferGetData(DVBCoreBufferT *buf)
{
    struct DVBCoreBufferImplS *impl = NULL;
    
    DVBCORE_CHECK(buf);
    impl = DVBCoreContainerOf(buf, struct DVBCoreBufferImplS, base);
    return impl->mData + impl->mRangeOffset;
}

static uint32 __DVBCoreBufferGetSize(DVBCoreBufferT *buf)
{
    struct DVBCoreBufferImplS *impl = NULL;
    
    DVBCORE_CHECK(buf);
    impl = DVBCoreContainerOf(buf, struct DVBCoreBufferImplS, base);
    return impl->mRangeLen;
}

dvbcore_void __DVBCoreBufferSetRange(DVBCoreBufferT *buf, uint32 offset, uint32 len)
{
    struct DVBCoreBufferImplS *impl = NULL;
    
    DVBCORE_CHECK(buf);
    impl = DVBCoreContainerOf(buf, struct DVBCoreBufferImplS, base);
    
    onSetRange(impl, offset, len);
    return ;
}

static dvbcore_void __DVBCoreBufferDecRef(DVBCoreBufferT *buf)
{
    struct DVBCoreBufferImplS *impl = NULL;
    
    DVBCORE_CHECK(buf);
    impl = DVBCoreContainerOf(buf, struct DVBCoreBufferImplS, base);
    if (DVBCoreAtomicDec(&impl->mRef) == 0) 
    {
        Pfree(impl->pool, impl->mData);
        if (impl->selfPool)
        {    
            AwPoolT *pool = impl->pool;
            Pfree(impl->pool, impl);
            AwPoolDestroy(pool);
        }
        else
        {
            Pfree(impl->pool, impl);
        }
    }
    return ;
}

static dvbcore_void __DVBCoreBufferAppend(DVBCoreBufferT *buf, const uint8 *data, 
                                    uint32 len)
{
    struct DVBCoreBufferImplS *impl = NULL;
    
    DVBCORE_CHECK(buf);
    impl = DVBCoreContainerOf(buf, struct DVBCoreBufferImplS, base);

    onWrite(impl, impl->mRangeOffset + impl->mRangeLen, data, len);
    onSetRange(impl, impl->mRangeOffset, impl->mRangeLen + len);
    return ;
}

static struct DVBCoreBufferOpsS gBufferOps = 
{
    .decRef = __DVBCoreBufferDecRef,
    .getData = __DVBCoreBufferGetData,
    .getSize = __DVBCoreBufferGetSize,
    .append = __DVBCoreBufferAppend,
    .setRange = __DVBCoreBufferSetRange,
};

DVBCoreBufferT *__DVBCoreBufferCreate(AwPoolT *pool, uint32 capacity, uint8 *initData, 
                            uint32 len, char *file, int line)
{
    struct DVBCoreBufferImplS *impl = NULL;
    char *callFrom = file;
    DVBCORE_CHECK(capacity != 0);
    DVBCORE_CHECK(capacity >= len);

#ifdef MEMORY_LEAK_CHK
    callFrom = malloc(512);
    sprintf(callFrom, "%s:%s", file, __FUNCTION__);
#endif

    if (pool)
    {
        impl = AwPalloc(pool, sizeof(struct DVBCoreBufferImplS), callFrom, line);
        memset(impl, 0x00, sizeof(struct DVBCoreBufferImplS));
        impl->pool = pool;
    }
    else
    {
        AwPoolT *privPool = __AwPoolCreate(NULL, callFrom, line);
        impl = Palloc(pool, sizeof(struct DVBCoreBufferImplS));
        memset(impl, 0x00, sizeof(struct DVBCoreBufferImplS));
        
        impl->pool = privPool;
        impl->selfPool = DVBCORE_TRUE;
    }
    
    DVBCORE_FORCE_CHECK(impl);
    impl->mCapacity = Align2Power(capacity);    
    impl->mData = Palloc(impl->pool, impl->mCapacity);
    DVBCORE_FORCE_CHECK(impl->mData);
    impl->mRangeOffset = 0;
    impl->mRangeLen = 0;
    if (initData && (len > 0)) 
    {
        memcpy(impl->mData, initData, len);
        impl->mRangeLen = len;
    }
    impl->base.ops = &gBufferOps;
    DVBCoreAtomicSet(&impl->mRef, 1);
    
    return &impl->base;
}

dvbcore_void DVBCoreBufferDestroy(DVBCoreBufferT *buf)
{
    struct DVBCoreBufferImplS *impl = NULL;
    
    DVBCORE_CHECK(buf);
    impl = DVBCoreContainerOf(buf, struct DVBCoreBufferImplS, base);
    
    __DVBCoreBufferDecRef(buf);
    return ;
}

