#ifndef DVBCORE_BIT_READER_H
#define DVBCORE_BIT_READER_H

#include <DVBCoreTypes.h>

typedef struct DVBCoreBitReaderS DVBCoreBitReaderT;

struct DVBCoreBitReaderOpsS
{    
    dvbcore_void (*destroy)(DVBCoreBitReaderT *);
    
    uint32 (*getBits)(DVBCoreBitReaderT *, uint32);
    
    dvbcore_void (*skipBits)(DVBCoreBitReaderT *, uint32);
    
    void (*putBits)(DVBCoreBitReaderT *, uint32, uint32);
    
    uint32 (*numBitsLeft)(DVBCoreBitReaderT *) ;
    
    const uint8 *(*data)(DVBCoreBitReaderT *);
};

struct DVBCoreBitReaderS
{
    struct DVBCoreBitReaderOpsS *ops;
};

#ifdef __cplusplus
extern "C"
{
#endif

DVBCoreBitReaderT *DVBCoreBitReaderCreate(const uint8 *data, uint32 size);
dvbcore_void DVBCoreBitReaderDestroy(struct DVBCoreBitReaderS *br);
uint32 DVBCoreBitReaderGetBits(struct DVBCoreBitReaderS *br, uint32 n);
dvbcore_void DVBCoreBitReaderSkipBits(struct DVBCoreBitReaderS *br, uint32 n);
void DVBCoreBitReaderPutBits(struct DVBCoreBitReaderS *br, uint32 x, uint32 n);
uint32 DVBCoreBitReaderNumBitsLeft(struct DVBCoreBitReaderS *br) ;
const uint8 *DVBCoreBitReaderData(struct DVBCoreBitReaderS *br);

#ifdef __cplusplus
}
#endif

#endif
