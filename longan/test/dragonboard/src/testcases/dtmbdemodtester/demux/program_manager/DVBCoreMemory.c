#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <pthread.h>

#include "DVBCoreLog.h"
#include "DVBCoreMemory.h"

#if DVBCORE_ENABLE_MEMORY_LEAK_DEBUG

static MemoryBlockInfo gMemBlockInfo[DEBUG_MEM_BLOCK_INFO_NUM] = {{0}};
static uint32 gMemBlockNum = 0;
static uint32 gMemCallocOrder = 0;
static uint32 gMemPallocOrder = 0;
pthread_rwlock_t gMemDebugRwlock;

void  DVBCoreFreeDebug(/*void *arg*/void **arg, const char *function, uint32 line)
{
	DVBCORE_UNUSE(function);
	DVBCORE_UNUSE(line);
	void **ptr;
	int32 i, index;
	size_t pointer;
	ptr = (void **)arg;
	if(arg == NULL || (*ptr == NULL))
    	return ;
	
	pointer = (size_t)(*ptr);
	index = -1;
	pthread_rwlock_wrlock(&gMemDebugRwlock);
	//DVBCORE_LOGD("-------FREE ptr: %d\t\tline: %d      function: %s", (size_t)pointer, line, function);

	for(i = 0; i < DEBUG_MEM_BLOCK_INFO_NUM; i++)
	{
		if(gMemBlockInfo[i].ptr == pointer)
		{
    		index = i;
#if 1
    		DVBCORE_LOGD("FREE: ptr: %p, \tsize: %d, \t\tline: %d, \t\tnum: %d,\t\t order: %d",
    				gMemBlockInfo[i].ptr, gMemBlockInfo[i].size, gMemBlockInfo[i].line,
    				gMemBlockNum, gMemBlockInfo[i].order);
#endif
    		if(gMemBlockInfo[i].bIsPalloc == 1)
    		{
    			DVBCORE_LOGD("free error. palloc block: ptr: 0x%x\t size: %d, func: %s, line: %d , order: %d, file: %s",
    									(uint32)gMemBlockInfo[i].ptr, gMemBlockInfo[i].size, gMemBlockInfo[i].function,
    									gMemBlockInfo[i].line, gMemBlockInfo[i].order, strrchr(gMemBlockInfo[i].filename, '/')+1);
    		}
    		memset(&gMemBlockInfo[i], 0, sizeof(MemoryBlockInfo));
    		gMemBlockNum -= 1;
    		break;
		}
	}

	pthread_rwlock_unlock(&gMemDebugRwlock);
    free(*ptr);
    *ptr = NULL;
}

void* DVBCoreMallocDebug(uint32 size, const char *filename, const char *function, uint32 line)
{
    void* ptr;
    int32 i, index;
    index = 0;
    pthread_rwlock_wrlock(&gMemDebugRwlock);
    ptr = malloc(size);
    gMemCallocOrder += 1;
    gMemBlockNum += 1;
    for(i = 0; i < DEBUG_MEM_BLOCK_INFO_NUM; i++)
    {
    	if(gMemBlockInfo[i].flag == 0)
    	{
    		index = i;
    		break;
    	}
    }
    if(i < DEBUG_MEM_BLOCK_INFO_NUM)
    {
    	gMemBlockInfo[index].flag = 1;
    	gMemBlockInfo[index].ptr = (size_t)ptr;
    	gMemBlockInfo[index].size = size;
    	gMemBlockInfo[index].line = line;
    	gMemBlockInfo[index].bIsPalloc = 0;
    	gMemBlockInfo[index].filename = filename;
    	gMemBlockInfo[index].function = function;
    	gMemBlockInfo[index].order = gMemCallocOrder;
    }
    else
    {
    	DVBCORE_LOGD("DVBCORE_MEM_LEAK_DEBUG: MEM_BLOCK_INFO_NUM is too small !!!");
    }
    pthread_rwlock_unlock(&gMemDebugRwlock);
    DVBCORE_LOGD("MALLOC: ptr: %d 	line: %d	size: %d,\t\torder: %d ", (size_t)ptr, line, size, gMemCallocOrder);
    return ptr;
}

void* DVBCoreCallocDebug(uint32 count, uint32 size, const char *filename, const char *function, uint32 line)
{
    void* ptr;
    int32 i, index;
    index = 0;
    pthread_rwlock_wrlock(&gMemDebugRwlock);
    ptr = calloc(count, size);
    gMemCallocOrder += 1;
    gMemBlockNum += 1;
    for(i = 0; i < DEBUG_MEM_BLOCK_INFO_NUM; i++)
    {
    	if(gMemBlockInfo[i].flag == 0)
    	{
    		index = i;
    		break;
    	}
    }
    if(i < DEBUG_MEM_BLOCK_INFO_NUM)
    {
    	gMemBlockInfo[index].flag = 1;
    	gMemBlockInfo[index].ptr = (size_t)ptr;
    	gMemBlockInfo[index].size = size * count;
    	gMemBlockInfo[index].line = line;
    	gMemBlockInfo[index].bIsPalloc = 0;
    	gMemBlockInfo[index].filename = filename;
    	gMemBlockInfo[index].function = function;
    	gMemBlockInfo[index].order = gMemCallocOrder;
    }
    else
    {
    	DVBCORE_LOGD("DVBCORE_MEM_LEAK_DEBUG: MEM_BLOCK_INFO_NUM is too small !!!");
    }
    pthread_rwlock_unlock(&gMemDebugRwlock);
//    DVBCORE_LOGD("CALLOC: 	ptr: %d 	line: %d\t\tsize: %d\t\t order: %d", (size_t)ptr, line, size * count, gMemCallocOrder);
    return ptr;
}

void DVBCoreMeroryLeakDebugInfo(void)
{
	int32 num = 0, nPallocNum = 0;
	uint32 i;
	pthread_rwlock_wrlock(&gMemDebugRwlock);
	num = gMemBlockNum;

	DVBCORE_LOGD("--------------------- Memory debug information start ---------------------- ");
	DVBCORE_LOGD(" UNFREE memory block number: %d ", gMemBlockNum);
	DVBCORE_LOGD(" total memory block number:  %d ", gMemCallocOrder);
	DVBCORE_LOGD(" calloc/DVBCoreMalloc block number: %d ", gMemCallocOrder - gMemPallocOrder);
	DVBCORE_LOGD(" palloc block number:        %d ", gMemPallocOrder);
//	DVBCORE_LOGD("Memory Info: unfree memory block = %d, total calloc/DVBCoreMalloc number = %d, total palloc block: %d",
//			gMemBlockNum, gMemCallocOrder - gMemPallocOrder, gMemPallocOrder);
	DVBCORE_LOGD("----------------------- Memory debug information end ---------------------- ");
//	if(num != 0)
	{
		for(i = 0; i < DEBUG_MEM_BLOCK_INFO_NUM; i++)
		{
			if(gMemBlockInfo[i].flag == 1)
			{
				DVBCORE_LOGD("MEMORY LEAK unfree block: ptr: 0x%x\t size: %d, func: %s, line: %d , order: %d, file: %s",
						(uint32)gMemBlockInfo[i].ptr, gMemBlockInfo[i].size, gMemBlockInfo[i].function,
						gMemBlockInfo[i].line, gMemBlockInfo[i].order, strrchr(gMemBlockInfo[i].filename, '/')+1);
			}
		}
	}
	pthread_rwlock_unlock(&gMemDebugRwlock);
}

void DVBCoreMeroryDebugInit(void)
{
	pthread_rwlock_init(&gMemDebugRwlock, NULL);
}

void DVBCoreMeroryDebugClose(void)
{
	pthread_rwlock_destroy(&gMemDebugRwlock);
}

#else /* #if DVBCORE_ENABLE_MEMORY_LEAK_DEBUG */

void  DVBCoreFreeNormal(void **arg)
{
	void **ptr = (void **)arg;
	if(ptr == NULL || *ptr == NULL)
    	return ;
    free(*ptr);
    *ptr = NULL;
}

void* DVBCoreMallocNormal(uint32 size)
{
    void* ptr = malloc(size);
    return ptr;
}

void* DVBCoreCallocNormal(uint32 count, uint32 size)
{
    void* ptr = calloc(count, size);
    return ptr;
}

#endif /* #if DVBCORE_ENABLE_MEMORY_LEAK_DEBUG */
