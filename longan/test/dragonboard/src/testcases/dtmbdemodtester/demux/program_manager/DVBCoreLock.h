#ifndef DVBCORE_LOCK_H
#define DVBCORE_LOCK_H
#include <pthread.h>

typedef pthread_mutex_t DVBCoreMutexT;
typedef pthread_cond_t DVBCoreCondT;

#define DVBCoreMutexInit(mutex) pthread_mutex_init(mutex, NULL) 
#define DVBCoreMutexDestroy(mutex) pthread_mutex_destroy(mutex)
#define DVBCoreMutexLock(mutex) pthread_mutex_lock(mutex)
#define DVBCoreMutexTrylock(mutex)pthread_mutex_trylock(mutex)
#define DVBCoreMutexUnlock(mutex) pthread_mutex_unlock(mutex)

#define DVBCoreCondInit(cond) pthread_cond_init(cond, NULL)
#define DVBCoreCondDestroy(cond) pthread_cond_destroy(cond)
#define DVBCoreCondWait(cond, mutex) pthread_cond_wait(cond, mutex)
#define DVBCoreCondTimedwait(cond, mutex, time) \
                            pthread_cond_timedwait(cond, mutex, time)
#define DVBCoreCondSignal(cond) pthread_cond_signal(cond)
#define DVBCoreCondBroadcast(cond) pthread_cond_broadcast(cond)

#endif
