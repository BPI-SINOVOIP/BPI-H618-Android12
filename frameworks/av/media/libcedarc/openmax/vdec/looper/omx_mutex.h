/*
 * =====================================================================================
 *   Copyright (c)  Allwinner Technology Co. Ltd.
 *   All rights reserved.
 *       Filename:  omx_mutex.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  08/02/2018 04:18:11 PM
 *       Revision:  none
 *
 *         Author:  Gan Qiuye
 *
 * =====================================================================================
 */
#ifndef __OMX_MUTEX_H__
#define __OMX_MUTEX_H__
#include <pthread.h>
#include "OMX_Types.h"
#ifdef __cplusplus
extern "C" {
#endif

struct OmxMutex_obj
{
    pthread_mutex_t   mutex;
    OMX_BOOL          bUsed;
};

typedef struct OmxMutex_obj *OmxMutexHandle;
int OmxCreateMutex(OmxMutexHandle* handler, OMX_BOOL bUsed);
int OmxDestroyMutex(OmxMutexHandle* handler);
int OmxTryAcquireMutex(OmxMutexHandle handler);
int OmxAcquireMutex(OmxMutexHandle handler);
int OmxReleaseMutex(OmxMutexHandle handler);
#ifdef __cplusplus
}
#endif

#endif
