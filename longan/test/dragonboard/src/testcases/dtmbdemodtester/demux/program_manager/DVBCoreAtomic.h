#ifndef DVBCORE_ATOMIC_H
#define DVBCORE_ATOMIC_H

#include <DVBCoreTypes.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    volatile dvbcore_ssize counter;
} dvbcore_atomic_t;

/*���ü���+ 1 �����ؽ������ֵ*/
static inline int32 DVBCoreAtomicInc(dvbcore_atomic_t *ref) 
{
    return __sync_add_and_fetch(&ref->counter, 1L);
}

static inline int32 DVBCoreAtomicAdd(dvbcore_atomic_t *ref, dvbcore_ssize val) 
{
    return __sync_add_and_fetch(&ref->counter, val);
}

/*���ü���- 1 �����ؽ������ֵ*/
static inline int32 DVBCoreAtomicDec(dvbcore_atomic_t *ref)
{
    return __sync_sub_and_fetch(&ref->counter, 1L);
}

static inline int32 DVBCoreAtomicSub(dvbcore_atomic_t *ref, dvbcore_ssize val)
{
    return __sync_sub_and_fetch(&ref->counter, val);
}

/*�������ü���Ϊval����������ǰ��ֵ*/
static inline int32 DVBCoreAtomicSet(dvbcore_atomic_t *ref, dvbcore_ssize val)
{
    return __sync_lock_test_and_set(&ref->counter, val);
}

/*��ȡ���ü�����ֵ*/
static inline int32 DVBCoreAtomicRead(dvbcore_atomic_t *ref)
{
    return __sync_or_and_fetch(&ref->counter, 0L);
}

static inline dvbcore_bool DVBCoreAtomicCAS(dvbcore_atomic_t *ref, dvbcore_ssize oldVal, dvbcore_ssize newVal)
{
    return __sync_bool_compare_and_swap(&ref->counter, oldVal, newVal);
}



#ifdef __cplusplus
}
#endif

#endif
