#include <DVBCoreBitReader.h>
#include <DVBCoreMemory.h>
#include <DVBCoreLog.h>

struct DVBCoreBitReaderImplS
{
	struct DVBCoreBitReaderS base;
    const uint8 *mData;
    uint32 mSize;
    uint32 mReservoir;  // left-aligned bits
    uint32 mNumBitsLeft;    
};

dvbcore_void DVBCoreBitReaderDestroy(struct DVBCoreBitReaderS *br)
{
    DVBCORE_CHECK(br);
    struct DVBCoreBitReaderImplS *impl = NULL;
    impl = DVBCoreContainerOf(br, struct DVBCoreBitReaderImplS, base);
    DVBCoreFree(impl);
    return ;
}

static dvbcore_void onFillReservoir(struct DVBCoreBitReaderImplS *impl)
{
    uint32 i;

    DVBCORE_CHECK(impl->mSize > 0u);

    impl->mReservoir = 0;
    for (i = 0; impl->mSize > 0 && i < 4; ++i) {
        impl->mReservoir = (impl->mReservoir << 8) | *(impl->mData);

        ++(impl->mData);
        --(impl->mSize);
    }

    impl->mNumBitsLeft = 8 * i;
    impl->mReservoir <<= 32 - impl->mNumBitsLeft;
    return ;
}

uint32 DVBCoreBitReaderGetBits(struct DVBCoreBitReaderS *br, uint32 n)
{
    DVBCORE_CHECK(br);
    struct DVBCoreBitReaderImplS *impl = NULL;
    impl = DVBCoreContainerOf(br, struct DVBCoreBitReaderImplS, base);

    uint32 result = 0;
    DVBCORE_CHECK(n <= 32u);

    while (n > 0) {
        uint32 m;
        if (impl->mNumBitsLeft == 0) {
            onFillReservoir(impl);
        }

        m = n;
        if (m > impl->mNumBitsLeft) {
            m = impl->mNumBitsLeft;
        }

        result = (result << m) | (impl->mReservoir >> (32 - m));
        impl->mReservoir <<= m;
        impl->mNumBitsLeft -= m;

        n -= m;
    }

    return result;
}

dvbcore_void DVBCoreBitReaderSkipBits(struct DVBCoreBitReaderS *br, uint32 n)
{
    DVBCORE_CHECK(br);
    while (n > 32)
    {
        DVBCoreBitReaderGetBits(br, 32);
        n -= 32;
    }

    if (n > 0)
    {
        DVBCoreBitReaderGetBits(br, n);
    }
}

void DVBCoreBitReaderPutBits(struct DVBCoreBitReaderS *br, uint32 x, uint32 n)
{
    DVBCORE_CHECK(br);
    struct DVBCoreBitReaderImplS *impl = NULL;
    impl = DVBCoreContainerOf(br, struct DVBCoreBitReaderImplS, base);

    DVBCORE_CHECK(n < 32u);

    while (impl->mNumBitsLeft + n > 32) {
        impl->mNumBitsLeft -= 8;
        --impl->mData;
        ++impl->mSize;
    }

    impl->mReservoir = (impl->mReservoir >> n) | (x << (32 - n));
    impl->mNumBitsLeft += n;
}

uint32 DVBCoreBitReaderNumBitsLeft(struct DVBCoreBitReaderS *br) 
{
    DVBCORE_CHECK(br);
    struct DVBCoreBitReaderImplS *impl = NULL;
    impl = DVBCoreContainerOf(br, struct DVBCoreBitReaderImplS, base);

    return impl->mSize * 8 + impl->mNumBitsLeft;
}

const uint8 *DVBCoreBitReaderData(struct DVBCoreBitReaderS *br)
{
    DVBCORE_CHECK(br);
    struct DVBCoreBitReaderImplS *impl = NULL;
    impl = DVBCoreContainerOf(br, struct DVBCoreBitReaderImplS, base);

    return impl->mData - (impl->mNumBitsLeft + 7) / 8;
}

static struct DVBCoreBitReaderOpsS gBitReaderOps = 
{
    .destroy = DVBCoreBitReaderDestroy,
    .getBits = DVBCoreBitReaderGetBits,
    .skipBits = DVBCoreBitReaderSkipBits,
    .putBits = DVBCoreBitReaderPutBits,
    .numBitsLeft = DVBCoreBitReaderNumBitsLeft,
    .data = DVBCoreBitReaderData
};

DVBCoreBitReaderT *DVBCoreBitReaderCreate(const uint8 *data, uint32 size)
{
    struct DVBCoreBitReaderImplS *impl;
    impl = DVBCoreMalloc(sizeof(*impl));
    DVBCORE_FORCE_CHECK(impl);
    impl->mData = data;
    impl->mSize = size;
    impl->mReservoir = 0;
    impl->mNumBitsLeft = 0;
    impl->base.ops = &gBitReaderOps;
    return &impl->base;
}


