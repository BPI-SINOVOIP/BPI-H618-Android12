#include <DVBCoreAwPool.h>
#include <DVBCoreLog.h>
#include <DVBCoreList.h>
#include <DVBCoreAtomic.h>
#include <DVBCoreLock.h>
//#include <DVBCoreMemory.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#define POOL_BLOCK_SIZE 8192
#define POOL_ALIGNMENT 8
#define POOL_LARGE_SIZE 4095

#define AwAlign(d, a) (((d) + (a - 1)) & ~(a - 1)) /*ÏòÉÏ¶ÔÆë*/

struct PoolDataS
{
    char *last;
    char *end;
    DVBCoreListNodeT node;
    DVBCoreListT pmList;
    dvbcore_atomic_t ref;
    int failed;
};

struct AwPoolS
{
    struct PoolDataS *current;
    DVBCoreListT largeList; /* large PoolMemoryS list */
    DVBCoreListT pdList; /* PoolDataS list */
    DVBCoreListT childList; /* child pool list */
    DVBCoreListNodeT node; /* in father's child list */
    DVBCoreMutexT mutex;
    char *file;
    int line;
};

struct PoolMemoryS
{
    struct PoolDataS *owner;
    char *file;
    int line;
    DVBCoreListNodeT node;
    int size;
};

static AwPoolT *gGolbalPool = NULL;

static inline struct PoolDataS *PoolDataIncRef(struct PoolDataS *poolData)
{
    DVBCoreAtomicInc(&poolData->ref);
    return poolData;
}

static inline void PoolDataDecRef(struct PoolDataS *poolData)
{
    if (DVBCoreAtomicDec(&poolData->ref) == 0)
    {
        DVBCoreListDel(&poolData->node);
        free(poolData);
    }
}

static void *PallocBlock(AwPoolT *pool, int size, char *file, int line)
{
    struct PoolDataS *pd = NULL, *newPd = NULL, *currentPd = NULL;
    DVBCoreListNodeT *pbNode = NULL, *nPbNode = NULL;   
    struct PoolMemoryS *pm = NULL;

    newPd = malloc(POOL_BLOCK_SIZE);
//    newPd = memalign(POOL_ALIGNMENT, POOL_BLOCK_SIZE);
    if (newPd == NULL)
    {
        DVBCORE_LOGE("memalign alloc %d failure errno(%d)", POOL_BLOCK_SIZE, errno);
        return NULL;
    }

    newPd->end = ((char *)newPd) + POOL_BLOCK_SIZE;
    newPd->last = ((char *)newPd) + AwAlign(sizeof(struct PoolDataS), POOL_ALIGNMENT);
    newPd->failed = 0;
    DVBCoreListInit(&newPd->pmList);
    DVBCoreAtomicSet(&newPd->ref, 1);

    for (pbNode = &pool->current->node, nPbNode = pbNode->next;
         pbNode != (DVBCoreListNodeT *)&pool->pdList; 
         pbNode = nPbNode, nPbNode = pbNode->next)
    {
        pd = DVBCoreListEntry(pbNode, struct PoolDataS, node);
        if (pd->failed++ > 4)
        {
            if (pd->node.next != (DVBCoreListNodeT *)&pool->pdList)
            {
                currentPd = DVBCoreListEntry(pd->node.next, struct PoolDataS, node);
            }
            else
            {
                currentPd = newPd;
            }
            PoolDataDecRef(pd);
        }
    }

    DVBCoreListAddTail(&newPd->node, &pool->pdList);
    pool->current = currentPd ? currentPd : pool->current;

    pm = (struct PoolMemoryS *)newPd->last;
    pm->owner = newPd;
    pm->file = file;
    pm->line = line;
    pm->size = size;
    
    newPd->last += AwAlign(sizeof(*pm) + size, POOL_ALIGNMENT);
    DVBCoreListAddTail(&pm->node, &newPd->pmList);
    PoolDataIncRef(newPd);
    
    return pm + 1;
}

static void *PallocLarge(AwPoolT *pool, int size, char *file, int line)
{
    struct PoolMemoryS *pm;

    pm = malloc(size + sizeof(*pm));
    if (pm == NULL) 
    {
        DVBCORE_LOGE("malloc size(%d) failure, errno(%d)", size, errno);
        return NULL;
    }

    pm->owner = NULL;
    pm->file = file;
    pm->line = line;
    pm->size = size;
    DVBCoreListAdd(&pm->node, &pool->largeList);
    return pm + 1;
}

AwPoolT *PoolNodeCreate(char *file, int line)
{
    AwPoolT *pool; 
    struct PoolDataS *poolData;

    pool = malloc(1024);
    poolData = malloc(POOL_BLOCK_SIZE);

//    pool = memalign(POOL_ALIGNMENT, 1024);
//    poolData = memalign(POOL_ALIGNMENT, POOL_BLOCK_SIZE);
    if ((!pool) || (!poolData)) 
    {
        DVBCORE_LOGE("memalign alloc %d failure errno(%d)", POOL_BLOCK_SIZE, errno);
		if(pool)
			free(pool);
		if(poolData)
			free(poolData);
        return NULL;
    }

    poolData->last = (char *) poolData + sizeof(*poolData);
    poolData->end = (char *) poolData + POOL_BLOCK_SIZE;
    poolData->failed = 0;
    DVBCoreListInit(&poolData->pmList);
    
    DVBCoreListInit(&pool->largeList);
    DVBCoreListInit(&pool->pdList);
    pool->current = poolData;
    DVBCoreListInit(&pool->childList);

    pthread_mutexattr_t attr;
    if (pthread_mutexattr_init(&attr) != 0)
    {
        DVBCORE_LOGE("init thread mutex attr failure...\n");
        return NULL;
    }
    if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK_NP) != 0)
    {
        DVBCORE_LOGE("pthread_mutexattr_settype failure...\n");
        return NULL;
    }
    pthread_mutex_init(&pool->mutex, &attr);

    DVBCoreListAddTail(&poolData->node, &pool->pdList);
    DVBCoreAtomicSet(&poolData->ref, 1); /*pool.data*/

    pool->file = file;
    pool->line = line;

    return pool;
}

AwPoolT *__AwPoolCreate(AwPoolT *father, char *file, int line)
{
    AwPoolT  *pool;
    if (!father)
    {
        if (!gGolbalPool)
        {
            gGolbalPool = PoolNodeCreate(__FILE__, __LINE__);
        }
        father = gGolbalPool;
    }

    pool = PoolNodeCreate(file, line);
    
    DVBCoreListAdd(&pool->node, &father->childList);
    
    return pool;
}

void AwPoolDestroy(AwPoolT *pool)
{
    AwPoolT *p, *nP;
    struct PoolMemoryS *pm, *nPm;
    struct PoolDataS *pd, *nPd;

    /*destroy child  pool*/
    DVBCoreListForEachEntrySafe(p, nP, &pool->childList, node)
    {
        DVBCoreListDel(&p->node);
        AwPoolDestroy(p);
    }

    if (pool != gGolbalPool)
    {
        DVBCoreListDel(&pool->node); /* cut from father's list */
    }
    
    DVBCoreListForEachEntrySafe(pm, nPm, &pool->largeList, node)
    {
        DVBCORE_LOGW("memory leak @<%s:%d>", strrchr(pm->file, '/') + 1, pm->line);        
        DVBCoreListDel(&pm->node);
        free(pm);
    }

    DVBCoreListForEachEntrySafe(pd, nPd, &pool->pdList, node)
    {
        if (pd->failed > 5)
        {
            PoolDataIncRef(pd);
        }
        
        if (DVBCoreAtomicRead(&pd->ref) != 1)
        {
            struct PoolMemoryS *pm;
            DVBCoreListForEachEntry(pm, &pd->pmList, node)
            {
                DVBCORE_LOGW("memory leak @<%s:%d>", strrchr(pm->file, '/') + 1, pm->line);        
                PoolDataDecRef(pd);
            }
        }
        
        DVBCORE_LOG_CHECK(DVBCoreAtomicRead(&pd->ref) == 1, "ref(%d), failed(%d)", 
                    DVBCoreAtomicRead(&pd->ref), pd->failed);
        PoolDataDecRef(pd);
    }

    DVBCoreMutexDestroy(&pool->mutex);

    free(pool);
    return ;
}

void *AwPalloc(AwPoolT *pool, int size, char *file, int line)
{
    struct PoolDataS *pd;
    int pmSize;
    void *ret;
        
    if (!pool)
    {
        if (!gGolbalPool)
        {
            gGolbalPool = PoolNodeCreate(file, line);
        }
        pool = gGolbalPool;
    }
    DVBCoreMutexLock(&pool->mutex);
    
    pmSize = AwAlign(sizeof(struct PoolMemoryS) + size, POOL_ALIGNMENT);
    
    if (pmSize <= POOL_LARGE_SIZE) 
    {
        DVBCoreListNodeT *pbNode = NULL;
        struct PoolMemoryS *pm = NULL;
    	for (pbNode = &pool->current->node;
             pbNode != (DVBCoreListNodeT *)&pool->pdList; 
    	     pbNode = pbNode->next)
        {
            pd = DVBCoreListEntry(pbNode, struct PoolDataS, node);
            if ((int)(pd->end - pd->last) >= pmSize)
            {
                pm = (struct PoolMemoryS *)pd->last; 
                pm->owner = pd;
                pm->file = file;
                pm->line = line;
                pm->size = size;
                PoolDataIncRef(pm->owner);
                DVBCoreListAddTail(&pm->node, &pd->pmList);
                pd->last += pmSize;
                ret = pm + 1;
                goto out;
            }
        }

        ret = PallocBlock(pool, size, file, line);
        goto out;
    }

    ret = PallocLarge(pool, size, file, line);

out:
    DVBCoreMutexUnlock(&pool->mutex);
    return ret;
}

void *AwRealloc(AwPoolT *pool, void *p, int size, char *file, int line)
{
    struct PoolMemoryS *pm;
    int freeSize = 0;
    void *newP;
    
    if (!pool)
    {
        pool = gGolbalPool;
    }
    
    DVBCoreMutexLock(&pool->mutex);

    pm = ((struct PoolMemoryS *)p) - 1;

    DVBCORE_LOG_CHECK(size > pm->size, "invalid size, (%d, %d)", size, pm->size);
    
    if (pm->size > POOL_LARGE_SIZE) /*in large memory*/
    {
        newP = PallocLarge(pool, size, file, line);
        if (!newP)
        {
            DVBCORE_LOGE("realloc failure...\n");
            goto out;
        }
        memcpy(newP, p, pm->size);
        DVBCoreListDel(&pm->node);
        free(pm);
        goto out;
    }

    if (size > POOL_LARGE_SIZE)
    {
        newP = PallocLarge(pool, size, file, line);
        if (!newP)
        {
            DVBCORE_LOGE("realloc failure...\n");
            goto out;
        }
        memcpy(newP, p, pm->size);
        AwPfree(pool, p);
        goto out;
    }
    
    if (pm->node.next == (DVBCoreListNodeT *)&pm->owner->pmList)
    {
        freeSize = ((char *)(pm->owner)) + POOL_BLOCK_SIZE - ((char *)p);
    }
    else
    {
        struct PoolMemoryS *nextPm;
        nextPm = DVBCoreListEntry(pm->node.next, struct PoolMemoryS, node);
        freeSize = ((char *)nextPm) - ((char *)p);
    }
    
    if (freeSize >= size)
    {
        pm->size = size;
        pm->file = file;
        pm->line = line;
        newP = p;
        if (pm->node.next == (DVBCoreListNodeT *)&pm->owner->pmList)
        {
            pm->owner->last = ((char *)pm) + AwAlign(sizeof(struct PoolMemoryS) + size, POOL_ALIGNMENT);
        }
        goto out;
    }

    newP = AwPalloc(pool, size, file, line);
    memcpy(newP, p, pm->size);
    AwPfree(pool, p);

out:
    DVBCoreMutexUnlock(&pool->mutex);    
    return newP;
}

void AwPfree(AwPoolT *pool, void *p)
{
    struct PoolMemoryS *pm;
    
    if (!pool)
    {
        pool = gGolbalPool;
    }

    DVBCoreMutexLock(&pool->mutex);

    pm = ((struct PoolMemoryS *)p) - 1;
    if (pm->size > POOL_LARGE_SIZE)
    {
        DVBCoreListDel(&pm->node);
        free(pm);
    }
    else
    {
        DVBCoreListDel(&pm->node);
        PoolDataDecRef(pm->owner);
    }

    DVBCoreMutexUnlock(&pool->mutex);    
    return ;
}

void AwPoolReset(void)
{
    if (gGolbalPool)
    {
        AwPoolDestroy(gGolbalPool);
        gGolbalPool = NULL;
    }
    else
    {
        DVBCORE_LOGW("global pool not initinal...\n");
    }
    return ;
}

