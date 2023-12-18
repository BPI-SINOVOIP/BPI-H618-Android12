#ifndef DVBCORE_MEMORY_H
#define DVBCORE_MEMORY_H
#include "DVBCoreTypes.h"
#include <sys/types.h>
#define	DVBCORE_ENABLE_MEMORY_LEAK_DEBUG 0

#if DVBCORE_ENABLE_MEMORY_LEAK_DEBUG
#define DVBCoreMalloc(size) 		DVBCoreMallocDebug(size,__FILE__,__func__,__LINE__)
#define DVBCoreCalloc(count,size) DVBCoreCallocDebug(count,size,__FILE__,__func__,__LINE__)
#define DVBCoreFree(ptr)		DVBCoreFreeDebug(/*ptr*/(void **)(&(ptr)),__func__,__LINE__)

#define DEBUG_MEM_BLOCK_INFO_NUM 10000
typedef struct
{
	//uint32 ptr;
	size_t	ptr;
	uint32 size;
	uint32 line;
	uint32 order;
	uint32 bIsPalloc;
	const char *function;
	const char *filename;
	uint32 flag;
}MemoryBlockInfo;

void  DVBCoreFreeDebug(/*void *arg*/void **arg, const char *function, uint32 line);
void* DVBCoreMallocDebug(uint32 size, const char *filename, const char *function, uint32 line);
void* DVBCoreCallocDebug(uint32 count, uint32 size, const char *filename, const char *function, uint32 line);

void DVBCoreMeroryLeakDebugInfo(void);
void DVBCoreMeroryDebugInit(void);
void DVBCoreMeroryDebugClose(void);
#else

#define DVBCoreMalloc(size) 		DVBCoreMallocNormal(size)
#define DVBCoreCalloc(count,size) DVBCoreCallocNormal(count,size)
#define DVBCoreFree(ptr)		DVBCoreFreeNormal((void **)(&(ptr)))


void* DVBCoreMallocNormal(uint32 size);
void* DVBCoreCallocNormal(uint32 count, uint32 size);
void  DVBCoreFreeNormal(void **arg);


#endif /* DVBCORE_ENABLE_MEMORY_LEAK_DEBUG */

#endif /* DVBCORE_MEMORY_H */

