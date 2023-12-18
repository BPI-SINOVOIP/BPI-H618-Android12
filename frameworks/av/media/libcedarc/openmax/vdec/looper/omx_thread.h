/*
 * =====================================================================================
 *   Copyright (c)  Allwinner Technology Co. Ltd.
 *   All rights reserved.
 *       Filename:  omx_thread.h
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
#ifndef __OMX_THREAD_H__
#define __OMX_THREAD_H__

#include <pthread.h>
#include "OMX_Types.h"
#include <semaphore.h>
#include "omx_mutex.h"
#include "omx_sem.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum OmxThread_Priority_t
{
    OMXTHREAD_PRIORITY_NORMAL,
    OMXTHREAD_PRIORITY_HIGH
}OmxThread_Priority_t;

typedef OMX_BOOL (*fnThreadLoop)(void *priv);
typedef OMX_BOOL (*fnReadyToRun)(void* priv);

typedef struct omx_thread
{
    fnThreadLoop mfnThreadLoop;
    fnReadyToRun mfnReadyToRun;
    void *mPriv;
    char *mName;
    OMX_S32 mPriority;
    OMX_U32 mStackSize;

    OMX_BOOL bRepeatToReady;
    OMX_BOOL bRunning;
    OMX_BOOL bExitPending;
    pthread_t    mThread;

    OmxSemHandle mSemStopRun;
    OmxSemHandle mSemExit;
    OmxSemHandle mSemSuspend;
    OmxMutexHandle lock;

    OMX_BOOL bSuspendRequest;

    int (*run)(struct omx_thread *self);
    int (*setFunc_readyToRun)(struct omx_thread *self, fnReadyToRun fn);
    int (*setParm_Priority)(struct omx_thread *self, OmxThread_Priority_t priority);
    int (*setParm_StackSize)(struct omx_thread *self, int stackSize);
    int (*suspend)(struct omx_thread *self);
    int (*resume)(struct omx_thread *self);
    int (*requestExit)(struct omx_thread *self);
    int (*requestExitAndWait)(struct omx_thread *self, OMX_S32 timeout);
}OmxThread_t;

typedef OmxThread_t* OmxThreadHandle;

OmxThreadHandle OmxCreateThread(const char* name, fnThreadLoop fn, void *priv);
void OmxDestroyThread(OmxThreadHandle *pSelf);
#ifdef __cplusplus
}
#endif

#endif
