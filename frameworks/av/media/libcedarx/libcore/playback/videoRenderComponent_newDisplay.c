/*
 * Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : videoRenderComponent_newDisplay.cpp
 * Description : video render component
 * History :
 *
 */

//#define CONFIG_LOG_LEVEL    OPTION_LOG_LEVEL_DETAIL
//#define LOG_TAG "videoRenderComponent_newDisplay"
#include "cdx_log.h"

#include <pthread.h>
#include <semaphore.h>
#include <malloc.h>
#include <memory.h>
#include <time.h>

#include "baseComponent.h"
#include "videoRenderComponent.h"
#include "AwMessageQueue.h"
#include "layerControl.h"
#include "memoryAdapter.h"
#include <deinterlace.h>
#include <iniparserapi.h>

#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include <linux/ioctl.h>
#include <CdxTime.h>
#ifdef __ANDROID__
#include <cutils/properties.h>
#endif
#define CANCEL_BUFFER_NUM 8
enum VIDEO_RENDER_RESULT
{
    VIDEO_RENDER_MESSAGE_COME      = 1,
    VIDEO_RENDER_DEINTERLACE_RESET = 2,
    VIDEO_RENDER_DROP_THE_PICTURE  = 3,
    VIDEO_RENDER_THREAD_CONTINUE   = 4,
};

struct VideoRenderComp
{
    AwMessageQueue      *mq;
    BaseCompCtx          base;

    pthread_t            sRenderThread;

    enum EPLAYERSTATUS   eStatus;

    LayerCtrl*           pLayerCtrl;
    VideoDecComp*        pDecComp;

    enum EPICTURE3DMODE  ePicture3DMode;
    enum EDISPLAY3DMODE  eDisplay3DMode;

    //* objects set by user.
    AvTimer*             pAvTimer;
    PlayerCallback       callback;
    void*                pUserData;
    int                  bEosFlag;

    //*
    int                  bResolutionChange;

    int                  bHadSetLayerInfoFlag;
    int                  bProtectedBufferFlag;//* 1: mean the video picture is secure

    //* for 3D video stream
    int                  bVideoWithTwoStream;

    //* for deinterlace
    Deinterlace         *di;
    int                  bDeinterlaceFlag;
    FbmBufInfo           mFbmBufInfo;

    int                  bSeekClosest;
    int                  nSeekTimeMs;

    int                  eVideoCodecFormat;

    //******
    int                  bFirstPictureShowed;
    int                  bNeedResetLayerParams;
    int                  bHideVideo;
    VideoPicture*        pPicture;
    VideoPicture*        pPrePicture;
    VideoPicture*        pPrePrePicture;
    int                  bHadGetVideoFbmBufInfoFlag;
    int                  nDeinterlaceDispNum;
    int                  nGpuBufferNum;
    int                  bHadSetBufferToDecoderFlag;
    VideoPicture*        pCancelPicture[CANCEL_BUFFER_NUM];
    int                  nCancelPictureNum;

    int                  nSetToDecoderBufferNum;

    VideoPicture*        pDiOutPicture;
#if defined(CONF_DI_300_SUPPORT)
    VideoPicture*        pDiOutPicture1;
    VideoPicture*        pDiOutTnr;//tnr
    int                  nProcessNum;
    CdxDeinterlaceParamT diParams;
#endif
    int                  bResetBufToDecoderFlag;
    int                  bHadRequestReleasePicFlag;
    int                  nNeedReleaseBufferNum;

    FramerateEstimater  *pFrameRateEstimater;

    int                  ptsSecs;

    int                  bHoldLastPicture;
    int                  bFirstPtsNotified;
    float                fPlayRate;

    int                  nDropFrame;
    int                  bHadGetBufferQueueToSF;
    int                  bBufferQueueToSF; //get videoBuffer whether QueueToSurfaceFlinger;
    int                  bDisableDeinterlace;
    int                  bProcessWithDi300;
    int                  bProgressiveFlag;
};

static void handleStart(AwMessage *msg, void *arg);
static void handleStop(AwMessage *msg, void *arg);
static void handlePause(AwMessage *msg, void *arg);
static void handleReset(AwMessage *msg, void *arg);
static void handleSetEos(AwMessage *msg, void *arg);
static void handleQuit(AwMessage *msg, void *arg);
static void doRender(AwMessage *msg, void *arg);

static void* VideoRenderThread(void* arg);


static void NotifyVideoSizeAndSetDisplayRegion(VideoRenderComp* p);

static inline int ProcessVideoSync(VideoRenderComp* p, VideoPicture* pPicture);
static int QueueBufferToShow(VideoRenderComp* p, VideoPicture* pPicture);
static int ProcessDeinterlace(VideoRenderComp* p, int nDeinterlaceTime);
static int RenderGetVideoFbmBufInfo(VideoRenderComp* p);

static int SetGpuBufferToDecoder(VideoRenderComp *p);

static int ResetBufToDecoder(VideoRenderComp *p);

static int InitDeinterlace(VideoRenderComp* p);

#ifdef __ANDROID__
//#defined DEBUG_DI
#if defined(DEBUG_DI)
//for debug
static int setDiParam()
{
    char state[PROPERTY_VALUE_MAX] = {0};
    property_get("mediasw.setdiparam", state, "0");

    return atoi(state);
}
#endif
static int shouldStopDeinterlace()
{
    char state[PROPERTY_VALUE_MAX] = {0};
    property_get("mediasw.stopdeinterlace", state, "0");
    if (state[0] == '1')
    {
        logi("stopdeinterlace");
        return 1;
    }
    return 0;
}
#endif

VideoRenderComp* VideoRenderCompCreate(void)
{
    VideoRenderComp* p;
    int err;

    p = (VideoRenderComp*)calloc(1, sizeof(*p));
    if(p == NULL)
    {
        loge("memory alloc fail.");
        return NULL;
    }

    p->nDeinterlaceDispNum = 1;
    p->bVideoWithTwoStream = -1;
    p->mq = AwMessageQueueCreate(4, "VideoRenderMq");
    if(p->mq == NULL)
    {
        loge("video render component create message queue fail.");
        free(p);
        return NULL;
    }

    BaseMsgHandler handler = {
        .start = handleStart,
        .stop = handleStop,
        .pause = handlePause,
        .reset = handleReset,
        .setEos = handleSetEos,
        .quit = handleQuit,
        .render = doRender,
    };

    if (BaseCompInit(&p->base, "video render", p->mq, &handler))
    {
        AwMessageQueueDestroy(p->mq);
        free(p);
        return NULL;
    }

    p->eStatus = PLAYER_STATUS_STOPPED;
    p->ptsSecs = -1;
    p->fPlayRate = 1.0f;

    //p->di = CdxDiCreate();
    //if (!p->di)
    //{
    //    logw("No deinterlace...");
    //}

    err = pthread_create(&p->sRenderThread, NULL, VideoRenderThread, p);
    if(err != 0)
    {
        loge("video render component create thread fail.");
        BaseCompDestroy(&p->base);
        AwMessageQueueDestroy(p->mq);
        free(p);
        return NULL;
    }

#ifdef __ANDROID__
    if(shouldStopDeinterlace() == 1)
        p->bDisableDeinterlace = 1;
#endif

#if defined(CONF_DI_300_SUPPORT)
    p->bProcessWithDi300 = 1;

    p->diParams.diEnableDemo = 0;
    p->diParams.diRect.left = 0;
    p->diParams.diRect.top = 0;
    p->diParams.diRect.right = 0;
    p->diParams.diRect.bottom = 0;
    p->diParams.diEnableTnr = 1;
    p->diParams.diTnrMode = 2;// FIX_MODE
    p->diParams.diTnrStrength = 0;// HIGH_LEVEL
    p->diParams.diEnableFmd = 1;
#endif

    return p;
}

int VideoRenderCompDestroy(VideoRenderComp* p)
{
    BaseCompQuit(&p->base, NULL, NULL);
    pthread_join(p->sRenderThread, NULL);
    BaseCompDestroy(&p->base);

    AwMessageQueueDestroy(p->mq);
    free(p);

    return 0;
}

int VideoRenderCompStart(VideoRenderComp* p)
{
    return BaseCompStart(&p->base, NULL, NULL);
}

int VideoRenderCompStop(VideoRenderComp* p)
{
    return BaseCompStop(&p->base, NULL, NULL);
}

int VideoRenderCompPause(VideoRenderComp* p)
{
    return BaseCompPause(&p->base, NULL, NULL);
}

enum EPLAYERSTATUS VideoRenderCompGetStatus(VideoRenderComp* p)
{
    return p->eStatus;
}

int VideoRenderCompReset(VideoRenderComp* p)
{
    return BaseCompReset(&p->base, 0, NULL, NULL);
}

int VideoRenderCompSetEOS(VideoRenderComp* p)
{
    return BaseCompSetEos(&p->base, NULL, NULL);
}

int VideoRenderCompSetSeekInfo(VideoRenderComp* p,int bSeekClosest,int nSeekTimeMs)
{
    p->bSeekClosest = bSeekClosest;
    p->nSeekTimeMs = nSeekTimeMs;
    return 0;
}

int VideoRenderCompSetCallback(VideoRenderComp* p, PlayerCallback callback, void* pUserData)
{
    p->callback  = callback;
    p->pUserData = pUserData;

    return 0;
}

int VideoRenderCompSetTimer(VideoRenderComp* p, AvTimer* timer)
{
    p->pAvTimer = timer;
    return 0;
}

static inline void ContinueRender(VideoRenderComp* p)
{
    if (p->eStatus == PLAYER_STATUS_STARTED ||
            (p->eStatus == PLAYER_STATUS_PAUSED &&
             p->bFirstPictureShowed == 0))
        BaseCompContinue(&p->base);
}

static void handleSetDeinterlace(AwMessage *msg, void *arg)
{
    VideoRenderComp *p = arg;
    p->di = msg->opaque;
    ContinueRender(p);
}

int VideoRenderCompSetDeinterlace(VideoRenderComp* p, Deinterlace* pDi)
{
    AwMessage msg = {
        .messageId = MESSAGE_ID_SET_DI,
        .replySem = NULL,
        .opaque = pDi,
        .execute = handleSetDeinterlace,
    };

    logd("video render component setting deinterlace: %p",pDi);

    if(AwMessageQueuePostMessage(p->mq, &msg) != 0)
    {
        loge("fatal error, video render component post message fail.");
        abort();
    }

    return 0;
}

static void returnOldPic(VideoRenderComp *p)
{

#if defined(CONF_DI_V2X_SUPPORT)
    if(p->pPrePrePicture != NULL)
    {
        VideoDecCompReturnPicture(p->pDecComp, p->pPrePrePicture);
        p->pPrePrePicture = NULL;
    }
#elif defined(CONF_DI_300_SUPPORT)
    logv("returnOldPic");
    if(p->bProgressiveFlag == 0)
    {
     /*
         *note: dequeue 5 buffer at most, so pic_4list_num should be 6.
         *process times: 0(bob)-----1(bob)-------2(60HZ)-------3(60HZ)---------4(60HZ)-----5(60HZ)---
                      p0--   --f0      p0--   --f0      p0--  --f0        p1--     --f0    tnr0--  --f0      tnr1--  --f0
                      p0--DI--         p0--DI--        p1--DI--f1    tnr0--DI--f1       tnr1--DI--f1     tnr2--DI--f1
                      p0--   --         p1--   --         p2--  --tnr0    p3--  --tnr1        p4--  --tnr2     p5--  --tnr
        */
        if(p->pPrePicture != p->pPrePrePicture && p->pPrePrePicture != NULL)
        {
            if(p->nProcessNum <= 3)//f0, f1, f2; f0, f1 returned from here.
            {
                logv("debugxx returnOldPic p->nProcessNum=%d, p->pPrePicture=%p,%p", p->nProcessNum,
                p->pPrePicture,p->pPrePrePicture);
                VideoDecCompReturnPicture(p->pDecComp, p->pPrePrePicture);
                p->pPrePrePicture = NULL;
            }
            else //queue pDiOutTnr.
            {
                LayerQueueBuffer(p->pLayerCtrl, p->pPrePrePicture, 0);
                p->pPrePrePicture = NULL;
                logv("debugxx returnOldPic p->nProcessNum=%d", p->nProcessNum);
            }
        }
        else if(p->pPrePicture == p->pPrePrePicture && p->pPrePrePicture != NULL)
        {
            VideoDecCompReturnPicture(p->pDecComp, p->pPrePrePicture);
            p->pPrePrePicture = NULL;
            p->pPrePicture = NULL;
            if(p->pPrePicture == p->pPicture)
            {
                p->pPicture = NULL;
            }
        }

        if(p->pPrePicture != NULL && p->pPrePicture != p->pPicture)
        {
            if(p->nProcessNum <= 2)//f0, f1, f2; f0, f1 returned from here.
            {
                VideoDecCompReturnPicture(p->pDecComp, p->pPrePicture);
                p->pPrePicture = NULL;
                p->pPrePrePicture = NULL;
            }
            else //queue pDiOutTnr.
            {
                LayerQueueBuffer(p->pLayerCtrl, p->pPrePicture, 0);
                p->pPrePicture = NULL;
            }
        }
    }
#endif

#if !defined(CONF_DI_300_SUPPORT)
    if(p->pPrePicture != NULL && p->pPrePicture != p->pPicture)
    {
        logv("return pPrePicture=%p", p->pPrePicture);
        VideoDecCompReturnPicture(p->pDecComp, p->pPrePicture);
        p->pPrePicture = NULL;
    }
#endif

     if(p->pPicture != NULL)
    {
        logv("return pPicture=%p", p->pPicture);
        VideoDecCompReturnPicture(p->pDecComp, p->pPicture);
        p->pPicture = NULL;
    }

    if(p->pDiOutPicture != NULL)
    {
        if(p->pLayerCtrl != NULL)
            LayerQueueBuffer(p->pLayerCtrl, p->pDiOutPicture, 0);
        p->pDiOutPicture = NULL;
    }

#if defined(CONF_DI_300_SUPPORT)
    if(p->pDiOutPicture1 != NULL)
    {
        if(p->pLayerCtrl != NULL)
            LayerQueueBuffer(p->pLayerCtrl, p->pDiOutPicture1, 0);
        p->pDiOutPicture1 = NULL;
    }
    if(p->pDiOutTnr != NULL)
    {
        if(p->pLayerCtrl != NULL)
            LayerQueueBuffer(p->pLayerCtrl, p->pDiOutTnr, 0);
        p->pDiOutTnr = NULL;
    }
    p->nProcessNum = 0;
#endif
}

static void setLayerInfo(VideoRenderComp *p)
{
    enum EPIXELFORMAT eDisplayPixelFormat = PIXEL_FORMAT_DEFAULT;
    FbmBufInfo* pFbmBufInfo = &p->mFbmBufInfo;

    //* we init deinterlace device here
    if(p->di != NULL && p->bDisableDeinterlace == 0 && (pFbmBufInfo->bProgressiveFlag == 0 || p->bProcessWithDi300 == 1))
        InitDeinterlace(p);

    eDisplayPixelFormat = p->bDeinterlaceFlag ?
        CdxDiExpectPixelFormat(p->di) : (enum EPIXELFORMAT)pFbmBufInfo->ePixelFormat;

    LayerSetDisplayPixelFormat(p->pLayerCtrl, eDisplayPixelFormat);
    LayerSetVideoWithTwoStreamFlag(p->pLayerCtrl, p->bVideoWithTwoStream);
    LayerSetIsSoftDecoderFlag(p->pLayerCtrl, pFbmBufInfo->bIsSoftDecoderFlag);
    LayerSetDisplayBufferSize(p->pLayerCtrl, pFbmBufInfo->nBufWidth, pFbmBufInfo->nBufHeight);
    p->nGpuBufferNum = LayerSetDisplayBufferCount(p->pLayerCtrl, pFbmBufInfo->nBufNum);
    p->bProgressiveFlag = pFbmBufInfo->bProgressiveFlag;

    p->bHadSetLayerInfoFlag = 1;
}

static void handleSetWindow(AwMessage *msg, void *arg)
{
    VideoRenderComp *p = arg;
    logv("process MESSAGE_ID_SET_WINDOW message, p->pPicture(%p)",p->pPicture);

    returnOldPic(p);
    LayerCtrl *lc = msg->opaque;

    // if the video Window change
    if(p->pLayerCtrl != NULL) //old nativewindow
    {
        FbmBufInfo* pFbmBufInfo =  VideoDecCompGetVideoFbmBufInfo(p->pDecComp);
        if(pFbmBufInfo == NULL)
        {
            LayerResetNativeWindow(p->pLayerCtrl, NULL);
            goto set_nativeWindow_exit;
        }

        //* 1. get the buffers in display and return to decoder
        VideoPicture* pPicture = NULL;
        int nWhileNum = 0;
        while(1)
        {
            nWhileNum++;
            if(nWhileNum >= 100)
            {
                loge("get pic node time more than 100, it is wrong");
                break;
            }

            pPicture = LayerGetBufferOwnedByGpu(p->pLayerCtrl);
            logv("test pPicture=%p, nWhileNum=%d", pPicture, nWhileNum);
            if(pPicture == NULL)
            {
                break;
            }
            VideoDecCompReturnPicture(p->pDecComp, pPicture);
        }

        //2. set video decoder release flag
        VideoDecCompSetVideoFbmBufRelease(p->pDecComp);
        p->bResetBufToDecoderFlag = 1;
        p->nNeedReleaseBufferNum  = p->nGpuBufferNum;
        LayerResetNativeWindow(p->pLayerCtrl, NULL);

        goto set_nativeWindow_exit;
    }
    else
    {
        p->pLayerCtrl = lc;
        LayerResetNativeWindow(p->pLayerCtrl, NULL);
    }

    if(p->pLayerCtrl != NULL)
        LayerSetSecureFlag(p->pLayerCtrl, p->bProtectedBufferFlag);

    p->bNeedResetLayerParams = 1;

    if(p->bHadSetLayerInfoFlag == 0)
    {
        enum EVIDEOCODECFORMAT eVideoCodecFormat = VIDEO_CODEC_FORMAT_UNKNOWN;
        eVideoCodecFormat = (enum EVIDEOCODECFORMAT)p->eVideoCodecFormat;
        LayerControl(p->pLayerCtrl, CDX_LAYER_CMD_SET_VIDEOCODEC_FORMAT, &eVideoCodecFormat);
    }

    //* we should set layer info here if it hadn't set it
    if(p->bHadSetLayerInfoFlag == 0 && p->mFbmBufInfo.nBufNum != 0)
        setLayerInfo(p);

set_nativeWindow_exit:
    sem_post(msg->replySem);
    ContinueRender(p);
}

int VideoRenderCompSetWindow(VideoRenderComp* p, LayerCtrl* lc)
{
    if(lc == NULL)
        return -1;

    sem_t replySem;
    sem_init(&replySem, 0, 0);

    AwMessage msg = {
        .messageId = MESSAGE_ID_SET_WINDOW,
        .replySem = &replySem,
        .opaque = lc,
        .execute = handleSetWindow,
    };

    logv("video render component setting window: %p", lc);

    if(AwMessageQueuePostMessage(p->mq, &msg) != 0)
    {
        loge("fatal error, video render component post message fail.");
        abort();
    }

    if(SemTimedWait(&replySem, -1) < 0)
    {
        loge("video render component wait for setting window finish failed.");
        sem_destroy(&replySem);
        return -1;
    }

    sem_destroy(&replySem);
    return 0;
}

int VideoRenderCompSetDecodeComp(VideoRenderComp* p, VideoDecComp* d)
{
    p->pDecComp  = d;
    return 0;
}

int VideoRenderSet3DMode(VideoRenderComp* p,
                         enum EPICTURE3DMODE ePicture3DMode,
                         enum EDISPLAY3DMODE eDisplay3DMode)
{
    logv("video render component setting 3d mode.");

    // These two variables are useless now.
    p->ePicture3DMode = ePicture3DMode;
    p->eDisplay3DMode = eDisplay3DMode;
    return 0;
}

int VideoRenderGet3DMode(VideoRenderComp* p,
                         enum EPICTURE3DMODE* ePicture3DMode,
                         enum EDISPLAY3DMODE* eDisplay3DMode)
{
    *ePicture3DMode = p->ePicture3DMode;
    *eDisplay3DMode = p->eDisplay3DMode;
    return 0;
}

int VideoRenderSetPlayRate(VideoRenderComp* p,float rate)
{
    p->fPlayRate = rate;
    return 0;
}

static void handleVideoHide(AwMessage *msg, void *arg)
{
    VideoRenderComp *p = arg;

    p->bHideVideo = msg->int64Value;

    if (p->pLayerCtrl != NULL)
    {
        if (p->bHideVideo)
            LayerCtrlHideVideo(p->pLayerCtrl);
        else if (p->bFirstPictureShowed == 1)
            LayerCtrlShowVideo(p->pLayerCtrl);
    }

    sem_post(msg->replySem);
    ContinueRender(p);
}

int VideoRenderVideoHide(VideoRenderComp* p, int bHideVideo)
{
    sem_t replySem;
    sem_init(&replySem, 0, 0);

    AwMessage msg = {
        .messageId = MESSAGE_ID_SET_VIDEO_HIDE,
        .replySem = &replySem,
        .int64Value = bHideVideo,
        .execute = handleVideoHide,
    };

    logv("video render component setting video hide(%d).", bHideVideo);

    if(AwMessageQueuePostMessage(p->mq, &msg) != 0)
    {
        loge("fatal error, video render component post message fail.");
        abort();
    }

    if(SemTimedWait(&replySem, -1) < 0)
    {
        loge("video render component wait for setting 3d mode finish failed.");
        sem_destroy(&replySem);
        return -1;
    }

    sem_destroy(&replySem);
    return 0;
}

static void handleSetHoldLastPicture(AwMessage *msg, void *arg)
{
    VideoRenderComp *p = arg;
    p->bHoldLastPicture = msg->int64Value;

    if (p->pLayerCtrl && p->eStatus == PLAYER_STATUS_STOPPED)
    {
        LayerCtrlHoldLastPicture(p->pLayerCtrl, p->bHoldLastPicture);

        if (!p->bHoldLastPicture)
            LayerCtrlHideVideo(p->pLayerCtrl);
    }

    ContinueRender(p);
}

int VideoRenderSetHoldLastPicture(VideoRenderComp* p, int bHold)
{
    AwMessage msg = {
        .messageId = MESSAGE_ID_SET_HOLD_LAST_PICTURE,
        .replySem = NULL,
        .int64Value = bHold,
        .execute = handleSetHoldLastPicture,
    };

    logv("video render component setting hold last picture(bHold=%d).", bHold);

    if(AwMessageQueuePostMessage(p->mq, &msg) != 0)
    {
        loge("fatal error, video render component post message fail.");
        abort();
    }

    return 0;
}

void VideoRenderCompSetProtecedFlag(VideoRenderComp* p, int bProtectedFlag)
{
    p->bProtectedBufferFlag = bProtectedFlag;
    return;
}

int VideoRenderCompSetVideoStreamInfo(VideoRenderComp* v, VideoStreamInfo* pStreamInfo)
{
    CEDARX_UNUSE(v);
    CEDARX_UNUSE(pStreamInfo);

    v->eVideoCodecFormat = pStreamInfo->eCodecFormat;

    return 0;
}

int VideoRenderCompSetSyncFirstPictureFlag(VideoRenderComp* v, int bSyncFirstPictureFlag)
{
    //*TODO
    (void)v;
    (void)bSyncFirstPictureFlag;
    return 0;
}

int VideoRenderCompSetFrameRateEstimater(VideoRenderComp* p, FramerateEstimater* fe)
{
    p->pFrameRateEstimater = fe;
    return 0;
}

static void* VideoRenderThread(void* arg)
{
    VideoRenderComp *p = arg;
    AwMessage msg;

    while (AwMessageQueueGetMessage(p->mq, &msg) == 0)
    {
        if (msg.execute != NULL)
            msg.execute(&msg, p);
        else
            loge("msg with msg_id %d doesn't have a handler", msg.messageId);
    }

    return NULL;
}

static void handleStart(AwMessage *msg, void *arg)
{
    VideoRenderComp *p = arg;

    if(p->eStatus == PLAYER_STATUS_STARTED)
    {
        logw("already in started status.");
        *msg->result = -1;
        sem_post(msg->replySem);
        BaseCompContinue(&p->base);
        return;
    }

    if(p->eStatus == PLAYER_STATUS_STOPPED)
    {
        p->bFirstPictureShowed = 0;
        p->bFirstPtsNotified = 0;
        p->bEosFlag = 0;
    }

    p->eStatus = PLAYER_STATUS_STARTED;
    *msg->result = 0;
    sem_post(msg->replySem);

    BaseCompContinue(&p->base);
}

static void handleStop(AwMessage *msg, void *arg)
{
    VideoRenderComp *p = arg;

    if(p->eStatus == PLAYER_STATUS_STOPPED)
    {
        logw("already in stopped status.");
        *msg->result = -1;
        sem_post(msg->replySem);
        return;
    }

    returnOldPic(p);

    if(p->pLayerCtrl != NULL)
    {
        LayerCtrlHoldLastPicture(p->pLayerCtrl, p->bHoldLastPicture);

        if (!p->bHoldLastPicture)
            LayerCtrlHideVideo(p->pLayerCtrl);
    }

    //* set status to stopped.
    p->eStatus = PLAYER_STATUS_STOPPED;
    *msg->result = 0;
    sem_post(msg->replySem);
}

static void handlePause(AwMessage *msg, void *arg)
{
    VideoRenderComp *p = arg;

    if(p->eStatus != PLAYER_STATUS_STARTED  &&
       !(p->eStatus == PLAYER_STATUS_PAUSED && p->bFirstPictureShowed == 0))
    {
        logw("not in started status, pause operation invalid.");
        *msg->result = -1;
        sem_post(msg->replySem);
        return;
    }

    p->eStatus = PLAYER_STATUS_PAUSED;

    *msg->result = 0;
    sem_post(msg->replySem);

    if(p->bFirstPictureShowed == 0)
        BaseCompContinue(&p->base);
}

static void handleQuit(AwMessage *msg, void *arg)
{
    VideoRenderComp *p = arg;

    returnOldPic(p);

    sem_post(msg->replySem);
    p->eStatus = PLAYER_STATUS_STOPPED;

    if (p->pLayerCtrl != NULL)
    {
        LayerRelease(p->pLayerCtrl);
        p->pLayerCtrl = NULL;
    }

    pthread_exit(NULL);
}

static void handleReset(AwMessage *msg, void *arg)
{
    VideoRenderComp *p = arg;

    returnOldPic(p);

    p->bEosFlag = 0;
    p->bFirstPictureShowed = 0;
    p->bFirstPtsNotified = 0;

#if defined(CONF_PTS_TOSF)
    if(p->bBufferQueueToSF == 1)
    {
        if(p->pLayerCtrl)
            LayerControl(p->pLayerCtrl, CDX_LAYER_CMD_RESTART_SCHEDULER, NULL);
    }
#endif

    *msg->result = 0;
    sem_post(msg->replySem);
    ContinueRender(p);
}

static void handleSetEos(AwMessage *msg, void *arg)
{
    VideoRenderComp *p = arg;

    p->bEosFlag = 1;
    sem_post(msg->replySem);

    ContinueRender(p);
}

static int requestPicture(VideoRenderComp *p)
{
    while(p->pPicture == NULL)
    {
        p->pPicture = VideoDecCompRequestPicture(p->pDecComp, 0, &p->bResolutionChange);
        logv("get picture, picture %p", p->pPicture);
        if(p->pPicture != NULL || (p->pPicture == NULL && p->bEosFlag))
            break;

        if(p->bResolutionChange)
        {
            //* reopen the video engine.
            VideoDecCompReopenVideoEngine(p->pDecComp);
            //* reopen the layer.
            if(p->pLayerCtrl != NULL)
            {
                LayerReset(p->pLayerCtrl);
                p->bNeedResetLayerParams = 1;
            }
            p->bResolutionChange          = 0;
            p->bHadSetLayerInfoFlag       = 0;
            p->bHadGetVideoFbmBufInfoFlag = 0;
            p->bHadSetBufferToDecoderFlag = 0;

            BaseCompContinue(&p->base);
            return -1;
        }

        if (AwMessageQueueWaitMessage(p->mq, 5) > 0)
            return -1;
    }

    if(p->pPicture == NULL && p->bEosFlag == 1)
    {
        p->callback(p->pUserData, PLAYER_VIDEO_RENDER_NOTIFY_EOS, NULL);
        return -1;
    }

    return 0;
}

static int notifyFirstPts(VideoRenderComp *p)
{
    /* this callback may block because the player need wait
     * audio first frame to sync.
     */
    int ret = p->callback(p->pUserData, PLAYER_VIDEO_RENDER_NOTIFY_FIRST_PICTURE,
            (void*)&p->pPicture->nPts);
    if(ret == TIMER_DROP_VIDEO_DATA)
    {
        //* video first frame pts small (too much) than the audio,
        //* discard this frame to catch up the audio.
        VideoDecCompReturnPicture(p->pDecComp, p->pPicture);
        p->pPicture = NULL;
        BaseCompContinue(&p->base);
        return -1;
    }
    else if(ret == TIMER_NEED_NOTIFY_AGAIN)
    {
        /* waiting process for first frame sync with audio is
         * broken by a new message to player, so the player tell
         * us to notify again later.
         */
        if (AwMessageQueueWaitMessage(p->mq, 10) <= 0)
            BaseCompContinue(&p->base);
        return -1;
    }

    p->bFirstPtsNotified = 1;
    return 0;
}

static int showProgressivePicture(VideoRenderComp *p)
{
    if (GetConfigParamterInt("show_1th_frame_quick", 0) == 1)
    {
        //* the first picture is showed unsychronized.
        if(p->bFirstPictureShowed == 0)
        {
            VideoPicture* pReturnPicture = NULL;

            QueueBufferToShow(p, p->pPicture);

            if(p->nSetToDecoderBufferNum < p->nGpuBufferNum)
            {
                // set all of the nativeBuffer to decoder
                int ret;
                int i;
                VideoPicture mTmpVideoPicture;
                VideoPicture* pTmpVideoPicture = &mTmpVideoPicture;
                ret = LayerDequeueBuffer(p->pLayerCtrl, &pTmpVideoPicture, 1);
                if(ret == 0)
                {
                    p->nSetToDecoderBufferNum ++;
                    VideoDecCompSetVideoFbmBufAddress(p->pDecComp, pTmpVideoPicture, 0);
                }
                else
                {
                    logw("LayerDequeueBuffer failed");
                    BaseCompContinue(&p->base);
                    return -1;
                }

                if(p->nSetToDecoderBufferNum == p->nGpuBufferNum)
                {
                    for (i = 0; i < p->nCancelPictureNum; ++i)
                    {
                        LayerQueueBuffer(p->pLayerCtrl, p->pCancelPicture[i], 0);
                    }
                }
            }
            else
            {
                LayerDequeueBuffer(p->pLayerCtrl, &pReturnPicture, 0);
                VideoDecCompReturnPicture(p->pDecComp, pReturnPicture);
            }
            p->pPicture = NULL;

            return 0;
        }
    }

    //* wait according to the presentation time stamp.
    if(p->bFirstPictureShowed == 0)
        return 0;

    int ret = ProcessVideoSync(p, p->pPicture);
    if(ret == VIDEO_RENDER_MESSAGE_COME)
    {
        return -1;
    }
    else if(ret == VIDEO_RENDER_DROP_THE_PICTURE)
    {
        VideoDecCompReturnPicture(p->pDecComp, p->pPicture);
        p->pPicture = NULL;
        BaseCompContinue(&p->base);
        return -1;
    }
    else
    {
        VideoPicture* pReturnPicture = NULL;
        QueueBufferToShow(p, p->pPicture);

        p->pPicture = NULL;
        if(p->nSetToDecoderBufferNum < p->nGpuBufferNum)
        {
            // set all of the nativeBuffer to decoder
            int ret;
            int i;
            VideoPicture mTmpVideoPicture;
            VideoPicture* pTmpVideoPicture = &mTmpVideoPicture;
            ret = LayerDequeueBuffer(p->pLayerCtrl, &pTmpVideoPicture, 1);
            if(ret == 0)
            {
                p->nSetToDecoderBufferNum ++;
                VideoDecCompSetVideoFbmBufAddress(p->pDecComp, pTmpVideoPicture, 0);
            }
            else
            {
                logw("LayerDequeueBuffer failed");
                BaseCompContinue(&p->base);
                return -1;
            }

            if(p->nSetToDecoderBufferNum == p->nGpuBufferNum)
            {
                for (i = 0; i < p->nCancelPictureNum; ++i)
                {
                    LayerQueueBuffer(p->pLayerCtrl, p->pCancelPicture[i], 0);
                }
            }
        }
        else
        {
            LayerDequeueBuffer(p->pLayerCtrl, &pReturnPicture, 0);
            VideoDecCompReturnPicture(p->pDecComp, pReturnPicture);
        }
    }

    return 0;
}

#if defined(CONF_DI_300_SUPPORT)
static int showInterlacedPictureDi300(VideoRenderComp *p)
{
    int result = 0;
    int ret = 0;

begin:
    ret = ProcessDeinterlace(p, 0);
    if(ret == VIDEO_RENDER_DEINTERLACE_RESET)
    {
        logd("VIDEO_RENDER_DEINTERLACE_RESET");
        goto begin;
    }
    else if(ret == -1)
    {
        loge("process deinterlace fail!");
        CdxDiReset(p->di);
        returnOldPic(p);
        BaseCompContinue(&p->base);
        return -1;
    }

    //* display pDiOutPicture
    if(p->bFirstPictureShowed != 0)
    {
        ret = ProcessVideoSync(p, p->pDiOutPicture);
        if(ret == VIDEO_RENDER_MESSAGE_COME)
        {
            returnOldPic(p);
            return -1;
        }
        else if(ret == VIDEO_RENDER_DROP_THE_PICTURE)
        {
            returnOldPic(p);
            BaseCompContinue(&p->base);
            return -1;
        }
    }

    QueueBufferToShow(p, p->pDiOutPicture);
    p->pDiOutPicture = NULL;

    //* display pDiOutPicture1
    if(p->pPrePicture != p->pPrePrePicture && p->pDiOutPicture1 != NULL)
    {
        ret = ProcessVideoSync(p, p->pDiOutPicture1);
        if(ret == VIDEO_RENDER_MESSAGE_COME)
        {
            returnOldPic(p);
            return -1;
        }
        else if(ret == VIDEO_RENDER_DROP_THE_PICTURE)
        {
            returnOldPic(p);
            BaseCompContinue(&p->base);
            return -1;
        }
        QueueBufferToShow(p, p->pDiOutPicture1);
        p->pDiOutPicture1 = NULL;
    }

/*
 *note: dequeue 5 buffer at most, so pic_4list_num should be 6.
 *process times: 0(bob)-----1(bob)-------2(60HZ)-------3(60HZ)---------4(60HZ)-----5(60HZ)---
              p0--   --f0      p0--   --f0      p0--  --f0        p1--     --f0    tnr0--  --f0      tnr1--  --f0
              p0--DI--         p0--DI--        p1--DI--f1    tnr0--DI--f1       tnr1--DI--f1     tnr2--DI--f1
              p0--   --         p1--   --         p2--  --tnr0    p3--  --tnr1        p4--  --tnr2     p5--  --tnr
*/
    if(p->pPrePicture != p->pPrePrePicture && p->pPrePrePicture != NULL)
    {
        if(p->nProcessNum <= 3)//f0, f1, f2; f0, f1 returned from here.
        {
            VideoDecCompReturnPicture(p->pDecComp, p->pPrePrePicture);
            logv("debugxx VideoDecCompReturnPicture pPrePrePicture=%p, p->nProcessNum=%d", p->pPrePrePicture, p->nProcessNum);
            p->pPrePrePicture = NULL;
        }
        else //queue pDiOutTnr.
        {
            LayerQueueBuffer(p->pLayerCtrl, p->pPrePrePicture, 0);
            logv("debugxx LayerQueueBuffer p->pPrePrePicture(preTnr)=%p, p->nProcessNum=%d", p->pPrePrePicture, p->nProcessNum);
            p->pPrePrePicture = NULL;
        }
        VideoDecCompReturnPicture(p->pDecComp, p->pPicture); // f2, f3,... returned.
        logv("debugxx VideoDecCompReturnPicture pPicture=%p, p->nProcessNum=%d", p->pPrePrePicture, p->nProcessNum);
    }

    if(p->pPrePicture == p->pPrePrePicture && p->pPrePicture != NULL) //1, 2
    {
        logv("p->pPrePicture=%p, %p, %p", p->pPrePrePicture, p->pPrePicture, p->pPicture);
        p->pPrePrePicture = p->pPrePicture;
        p->pPrePicture = p->pPicture;
    }
    else // >=3
    {
        p->pPrePrePicture = p->pPrePicture;
        p->pPrePicture = p->pDiOutTnr;
        p->pDiOutTnr = NULL;
    }

    p->pPicture = NULL;
    p->nProcessNum++;

    return result;
}

static int showProgressivePictureDi300(VideoRenderComp *p)
{
    //if you play a video with a player online , the resolution may change in auto mode, for example, 480p->720p
    //Func SetGpuBufferToDecoder will dequeue `nGpuBufferNum-1` 720p buffers at most, for gpu is holding one 480p buffer to display
    //We need to QueueBufferToShow one 720p picture to take the place of 480p which is displaying
    //Then we can dequeueTheInitGpuBuffer the last 720p buffer.
    //Totally we dequeueTheInitGpuBuffer `nGpuBufferNum` 720p buffers
    if(p->nSetToDecoderBufferNum < p->nGpuBufferNum)
    {
        showProgressivePicture(p);
        return 0;
    }

    int result = 0;
    int ret = 0;

begin:
    ret = ProcessDeinterlace(p, 0);
    if(ret == VIDEO_RENDER_DEINTERLACE_RESET)
    {
        logd("VIDEO_RENDER_DEINTERLACE_RESET");
        goto begin;
    }
    else if(ret == -1)
    {
        loge("process deinterlace fail!");
        CdxDiReset(p->di);
        returnOldPic(p);
        BaseCompContinue(&p->base);
        return -1;
    }

    //* display pDiOutPicture
    if(p->bFirstPictureShowed != 0)
    {
        ret = ProcessVideoSync(p, p->pDiOutTnr);
        if(ret == VIDEO_RENDER_MESSAGE_COME)
        {
            returnOldPic(p);
            return -1;
        }
        else if(ret == VIDEO_RENDER_DROP_THE_PICTURE)
        {
            returnOldPic(p);
            BaseCompContinue(&p->base);
            return -1;
        }
    }

    QueueBufferToShow(p, p->pDiOutTnr);
    p->pPrePicture = p->pDiOutTnr;
    p->pDiOutTnr = NULL;

    VideoDecCompReturnPicture(p->pDecComp, p->pPicture);
    p->pPicture = NULL;
    p->nProcessNum++;

    return result;
}

#endif

#if !defined(CONF_DI_300_SUPPORT)
static int showInterlacedPicture(VideoRenderComp *p)
{
    int result = 0;
    int n;
    for(n = 0; n < p->nDeinterlaceDispNum; n++)
    {
        int ret = ProcessDeinterlace(p, n);
        if(ret == VIDEO_RENDER_DEINTERLACE_RESET)
        {
            n = 0;
            continue;
        }
        else if (ret == -1)
        {
            loge("process deinterlace fail!");
            p->pDiOutPicture = NULL;
            CdxDiReset(p->di);
            BaseCompContinue(&p->base);
            result = -1;
            break;
        }

        // ***************** field error
        if(p->pPicture->bTopFieldError && n == 0)
        {
            logd("++++ top field error");
            LayerQueueBuffer(p->pLayerCtrl, p->pDiOutPicture, 0);
            p->pDiOutPicture = NULL;
            CdxDiReset(p->di);
            continue;
        }

        if((p->pPicture->bTopFieldError || p->pPicture->bBottomFieldError)
            && n == 1)
        {
            logd("+++++ bottom field error");
            LayerQueueBuffer(p->pLayerCtrl, p->pDiOutPicture, 0);
            p->pDiOutPicture = NULL;
            CdxDiReset(p->di);
            BaseCompContinue(&p->base);
            result = -1;
            break;
        }

        if(p->bFirstPictureShowed != 0)
        {
            ret = ProcessVideoSync(p, p->pDiOutPicture);
            if(ret == VIDEO_RENDER_MESSAGE_COME)
            {
                LayerQueueBuffer(p->pLayerCtrl, p->pDiOutPicture, 0);
                p->pDiOutPicture = NULL;
                result = -1;
                break;
            }
            else if(ret == VIDEO_RENDER_DROP_THE_PICTURE)
            {
                LayerQueueBuffer(p->pLayerCtrl, p->pDiOutPicture, 0);
                p->pDiOutPicture = NULL;
                BaseCompContinue(&p->base);
                result = -1;
                break;
            }
        }

        QueueBufferToShow(p, p->pDiOutPicture);
        p->pDiOutPicture = NULL;
    }

#if defined(CONF_DI_V2X_SUPPORT)

    if(p->pPrePicture != p->pPrePrePicture && p->pPrePrePicture != NULL)
    {
        VideoDecCompReturnPicture(p->pDecComp, p->pPrePrePicture);
    }

#else

    if(p->pPicture != p->pPrePicture && p->pPrePicture != NULL)
    {
        VideoDecCompReturnPicture(p->pDecComp, p->pPrePicture);
    }

#endif

#if defined(CONF_DI_V2X_SUPPORT)
    p->pPrePrePicture = p->pPrePicture;
#endif

    p->pPrePicture = p->pPicture;
    p->pPicture = NULL;

    return result;
}
#endif

static int checkFlags(VideoRenderComp *p)
{
    // when nativeWindow change, we should reset buffer to decoder
    while(p->bResetBufToDecoderFlag == 1)
    {
        int ret = ResetBufToDecoder(p);
        logv("bResetBufToDecoderFlag=1, ret=%d, bHadGetVideoFbmBufInfoFlag=%d",
            ret, p->bHadGetVideoFbmBufInfoFlag);
        if(ret == VIDEO_RENDER_THREAD_CONTINUE)
        {
            if (AwMessageQueueWaitMessage(p->mq, 5) <= 0)
                BaseCompContinue(&p->base);
            return -1;
        }
        else if (AwMessageQueueWaitMessage(p->mq, 5) > 0)
        {
            return -1;
        }
    }
    logv("bHadGetVideoFbmBufInfoFlag %d", p->bHadGetVideoFbmBufInfoFlag);

    //* get video fbm buf info
    while(p->bHadGetVideoFbmBufInfoFlag == 0)
    {
        if(p->bEosFlag == 1)
        {
            p->callback(p->pUserData, PLAYER_VIDEO_RENDER_NOTIFY_EOS, NULL);
            return -1;
        }

        int ret = RenderGetVideoFbmBufInfo(p);
        if(ret == 0)
        {
            p->bHadGetVideoFbmBufInfoFlag = 1;
        }
        else if(ret == -1)
        {
            if (AwMessageQueueWaitMessage(p->mq, 5) > 0)
                return -1;
        }
        else
        {
            return -1;
        }
    }

    logv("bHadSetBufferToDecoderFlag %d",p->bHadSetBufferToDecoderFlag);
    //* set buffer to decoder
    while(p->bHadSetBufferToDecoderFlag == 0)
    {
        if(p->bHadSetLayerInfoFlag)
        {
            if(SetGpuBufferToDecoder(p))
            {
                p->callback(p->pUserData, PLAYER_VIDEO_RENDER_NOTIFY_ERROR, NULL);
            }
            p->bHadSetBufferToDecoderFlag = 1;
        }
        else if (AwMessageQueueWaitMessage(p->mq, 5) > 0)
        {
            return -1;
        }
    }

    if(p->bHadGetBufferQueueToSF == 0 && p->pLayerCtrl != NULL)
    {
        LayerControl(p->pLayerCtrl, CDX_LAYER_CMD_GET_WHETHER_QUEUETOSF, (void*)&p->bBufferQueueToSF);
        p->bHadGetBufferQueueToSF =1;
    }

    return 0;
}

static void doRender(AwMessage *msg, void *arg)
{
    VideoRenderComp *p = arg;
    (void)msg;

    if(p->eStatus != PLAYER_STATUS_STARTED &&
      !(p->eStatus == PLAYER_STATUS_PAUSED && p->bFirstPictureShowed == 0))
    {
        logw("not in started status, render message ignored.");
        return;
    }

    if (checkFlags(p) != 0)
        return;

    if (p->pPicture == NULL)
    {
        if (requestPicture(p) != 0)
        {
            return;
        }
        else
        {
            if(p->bSeekClosest == 1)
            {
                if((int64_t)(p->nSeekTimeMs*1000) - p->pPicture->nPts > 16666)
                {
                    VideoDecCompReturnPicture(p->pDecComp, p->pPicture);
                    p->pPicture = NULL;
                    BaseCompContinue(&p->base);
                    return;
                }
            }
        }

        if(p->bFirstPictureShowed == 0 || p->bNeedResetLayerParams == 1)
        {
            NotifyVideoSizeAndSetDisplayRegion(p);
            p->bNeedResetLayerParams = 0;
        }
    }

    /************************************************************
     * notify the first sync frame to set timer. the first sync
     * frame is the second picture, the first picture need to be
     * showed as soon as we can.(unsynchroized)
     ***********************************************************/
    if(p->bFirstPictureShowed && p->bFirstPtsNotified == 0)
    {
        if (notifyFirstPts(p) != 0)
            return;
    }

    //******************************************************
    //* sync and show the picture
    //******************************************************
    logv("** p->bDeinterlaceFlag[%d]",p->bDeinterlaceFlag);
    if(p->bDeinterlaceFlag == 0)
    {
        if (showProgressivePicture(p) != 0)
            return;
    }
    else
    {
#if defined(CONF_DI_300_SUPPORT)
        if(p->bProgressiveFlag == 0)
        {
            if (showInterlacedPictureDi300(p) != 0)
                return;
        }
        else
        {
            if (showProgressivePictureDi300(p) != 0) //TNR process
                return;
        }
#else
        if (showInterlacedPicture(p) != 0)
            return;
#endif
    }

    if(p->bFirstPictureShowed == 0)
        p->bFirstPictureShowed = 1;

    if(p->eStatus == PLAYER_STATUS_STARTED)
    {
        BaseCompContinue(&p->base);
    }
    else
    {
        logi("first picture showed at paused status.");
    }
}

static int IsVideoWithTwoStream(VideoDecComp* pDecComp)
{
    VideoStreamInfo videoStreamInfo;
    if(VideoDecCompGetVideoStreamInfo(pDecComp, &videoStreamInfo) == 0)
        return videoStreamInfo.bIs3DStream;
    return 0;
}

static void NotifyVideoSizeAndSetDisplayRegion(VideoRenderComp* p)
{
    int size[4];

    if((p->pPicture->nRightOffset - p->pPicture->nLeftOffset) > 0 &&
       (p->pPicture->nBottomOffset - p->pPicture->nTopOffset) > 0)
    {
        int width = p->pPicture->nRightOffset - p->pPicture->nLeftOffset;
        int height = p->pPicture->nBottomOffset - p->pPicture->nTopOffset;

        size[0] = p->pPicture->nLeftOffset;
        size[1] = p->pPicture->nTopOffset;
        size[2] = width;
        size[3] = height;
        p->callback(p->pUserData, PLAYER_VIDEO_RENDER_NOTIFY_VIDEO_CROP, (void*)size);

        if(p->pLayerCtrl != NULL)
        {
            LayerSetDisplayRegion(p->pLayerCtrl, size[0], size[1], size[2], size[3]);
        }

        size[0] = width;
        size[1] = height;
        size[2] = 0;
        size[3] = 0;
        p->callback(p->pUserData, PLAYER_VIDEO_RENDER_NOTIFY_VIDEO_SIZE, (void*)size);
        if((width > 2048 || height > 1088) && p->bDeinterlaceFlag == 1)
        {
            p->bDeinterlaceFlag = 0;
            p->nDeinterlaceDispNum = 1;
        }
    }
    else
    {
        logw("the offsets of picture are not right, we set bufferWidht and \
              bufferHeight as video size, this maybe wrong, offset: %d, %d, %d, %d",
              p->pPicture->nLeftOffset,p->pPicture->nRightOffset,
              p->pPicture->nTopOffset,p->pPicture->nBottomOffset);

        if(p->pLayerCtrl != NULL)
        {
            LayerSetDisplayRegion(p->pLayerCtrl,
                                  0,
                                  0,
                                  p->pPicture->nWidth,
                                  p->pPicture->nHeight);
        }

        size[0] = p->pPicture->nWidth;
        size[1] = p->pPicture->nHeight;
        size[2] = 0;
        size[3] = 0;
        p->callback(p->pUserData, PLAYER_VIDEO_RENDER_NOTIFY_VIDEO_SIZE, (void*)size);

    }

    return ;
}

static inline int ProcessVideoSync(VideoRenderComp* p, VideoPicture* pPicture)
{
    int nWaitTime;

    nWaitTime = p->callback(p->pUserData, PLAYER_VIDEO_RENDER_NOTIFY_PICTURE_PTS,
            (void*)&pPicture->nPts);

#if defined(CONF_PTS_TOSF)
    if(p->bBufferQueueToSF == 1)
    {
        return 0;
    }
#endif

    double frameRate = 0.0;
    if (p->pFrameRateEstimater)
        frameRate = FramerateEstimaterGetFramerate(p->pFrameRateEstimater);
    if (frameRate <= 0.0)
        frameRate = pPicture->nFrameRate;
    if (frameRate > 1000.0)
        frameRate /= 1000.0;

    double frameDuration = 0.0;
    if (frameRate > 0.0)
        frameDuration = 1000.0 / frameRate;

    if(nWaitTime > 0)
    {
        if (p->fPlayRate == 1.0)
        {
            /*In order to be compatible,when the frame rate is too low,
             *cause insufficient waiting time and the playback rate
             *will be accelerated.*/
            if(nWaitTime < 4000)
            {
                if ((frameDuration > 0.0) && (nWaitTime > frameDuration * 2))
                    nWaitTime = frameDuration * 2;

                if (nWaitTime > 500)
                    nWaitTime = 500;
            }
        }
        if (AwMessageQueueWaitMessage(p->mq, nWaitTime) > 0)
            return VIDEO_RENDER_MESSAGE_COME;
    }
#if !defined(CONF_PRODUCT_STB)
    else if(nWaitTime < -50)
    {
        p->nDropFrame ++;
        if(p->nDropFrame % 2)
        {
            //* if it is expired, we should drop it
            loge("need to drop this frame, nWaitTime: %d", nWaitTime);
            return VIDEO_RENDER_DROP_THE_PICTURE;
        }
        else
        {
            return 0;
        }
    }
#else
    else if(nWaitTime < -10 && p->bDeinterlaceFlag == 1)
    {
        //* if it is deinterlace and expired, we should drop it
        loge("need to drop this frame");
        return VIDEO_RENDER_DROP_THE_PICTURE;
    }
    else
    {
        int nDispFPS = LayerGetDisplayFPS(p->pLayerCtrl);
        if ((nDispFPS > 0) && (frameRate > nDispFPS) && (nWaitTime < -frameDuration))
        {
            logv("drop frame nWaitTime=%d, nDispFPS=%d, frameRate=%f.",
                   nWaitTime, nDispFPS, frameRate);
            return VIDEO_RENDER_DROP_THE_PICTURE;
        }
    }
#endif

    return 0;
}

static int QueueBufferToShow(VideoRenderComp* p, VideoPicture* pPicture)
{
    if(p->pLayerCtrl != NULL)
    {
#if defined(CONF_PTS_TOSF)
        if(p->bBufferQueueToSF == 1)
        {
            int64_t ptsAbs = p->pAvTimer->PtsToSystemTime(p->pAvTimer, pPicture->nPts);
            LayerSetBufferTimeStamp(p->pLayerCtrl, ptsAbs);
        }
#endif

        LayerQueueBuffer(p->pLayerCtrl, pPicture, 1);
    }

    int ptsSecs = (int)(pPicture->nPts/1000000);
    if (p->ptsSecs != ptsSecs)
    {
        p->ptsSecs = ptsSecs;
        logd("video pts(%.3f) ", pPicture->nPts/1000000.0);
    }

    if(p->pLayerCtrl != NULL && p->bHideVideo == 0)
        LayerCtrlShowVideo(p->pLayerCtrl);

    return 0;
}

static int ProcessDeinterlace(VideoRenderComp* p,
                                  int nDeinterlaceTime)
{
    //* deinterlace process
    int ret = -1;

    if(p->pPrePicture == NULL)
    {
        p->pPrePicture          = p->pPicture;
    }

#if defined(CONF_DI_V2X_SUPPORT) || defined(CONF_DI_300_SUPPORT)
    if(p->bProgressiveFlag == 0)
    {
        if(p->pPrePrePicture == NULL)
        {
            p->pPrePrePicture = p->pPrePicture;
        }
    }
#endif

    if(p->bProgressiveFlag == 0)
    {
        logv("debugxx dequeue pDiOutPicture");
        ret = LayerDequeueBuffer(p->pLayerCtrl, &p->pDiOutPicture, 0);
        if(ret != 0)
        {
            loge("** dequeue buffer failed when process deinterlace");
            return -1;
        }
    }

#if defined(CONF_DI_300_SUPPORT)
    if(p->bProgressiveFlag == 0)
    {
        if(p->pPrePicture != p->pPrePrePicture)
        {
            logv("debugxx dequeue pDiOutPicture1");
            ret = LayerDequeueBuffer(p->pLayerCtrl, &p->pDiOutPicture1, 0);
            if(ret != 0)
            {
                loge("dequeue buffer failed when process deinterlace");
                LayerQueueBuffer(p->pLayerCtrl, p->pDiOutPicture, 0);
                return -1;
            }
            logv("debugxx dequeue pDiOutTnr");
            ret = LayerDequeueBuffer(p->pLayerCtrl, &p->pDiOutTnr, 0);
            if(ret != 0)
            {
                loge("dequeue buffer failed when process deinterlace");
                if(p->pDiOutPicture != NULL)
                    LayerQueueBuffer(p->pLayerCtrl, p->pDiOutPicture, 0);
                if(p->pDiOutPicture1 != NULL)
                    LayerQueueBuffer(p->pLayerCtrl, p->pDiOutPicture1, 0);
                return -1;
            }
        }
    }
    else
    {
        logv("debugxx dequeue pDiOutTnr");
        ret = LayerDequeueBuffer(p->pLayerCtrl, &p->pDiOutTnr, 0);
        if(ret != 0)
        {
            loge("dequeue buffer failed when process deinterlace");
            return -1;
        }
    }


#endif

#if defined(CONF_DI_V2X_SUPPORT)

    int diret = CdxDiProcess2(p->di,
                      p->pPrePrePicture,
                      p->pPrePicture,
                      p->pPicture,
                      p->pDiOutPicture,
                      nDeinterlaceTime);

    logv("** ProcessDeinterlace CdxDiProcess2 diret[%d]",diret);

    if (diret != 0)
    {
        CdxDiReset(p->di);

        VideoDecCompReturnPicture(p->pDecComp, p->pPrePrePicture);
        VideoDecCompReturnPicture(p->pDecComp, p->pPrePicture);
        LayerQueueBuffer(p->pLayerCtrl, p->pDiOutPicture, 0);
        p->pPrePrePicture = NULL;
        p->pPrePicture = NULL;
        p->pDiOutPicture = NULL;
        return VIDEO_RENDER_DEINTERLACE_RESET;
    }
#elif defined(CONF_DI_300_SUPPORT)
/*
 *1. for Interlace movies(bProgressiveFlag: 0): BOB BOB 60Hz ...
 *note: dequeue 5 buffer at most, so pic_4list_num should be 6.
 *process times: 0(bob)-----1(bob)-------2(60HZ)-------3(60HZ)---------4(60HZ)-----5(60HZ)---
              p0--   --f0      p0--   --f0      p0--  --f0        p1--     --f0    tnr0--  --f0      tnr1--  --f0
              p0--DI--         p0--DI--        p1--DI--f1    tnr0--DI--f1       tnr1--DI--f1     tnr2--DI--f1
              p0--   --         p1--   --         p2--  --tnr0    p3--  --tnr1        p4--  --tnr2     p5--  --tnr

*2. for Progress movies(bProgressiveFlag: 1): TNR ...
*process times: 0(TNR)-----1(TNR)-------2(TNR)--
              p0--   --f0      f0--    --f1      f1--  --f2
              p0--DI            p1--DI            p2--DI
*/
    logv("debugxx p->pPrePrePicture=%p, %p, %p, p->pDiOutPicture=%p, %p, %p, p->nProcessNum=%d, "
         "diEnableDemo=%d, rect(%d, %d, %d, %d), "
         "diEnableTnr=%d, diTnrMode=%d, diTnrStrength=%d, diEnableFmd=%d",
        p->pPrePrePicture, p->pPrePicture, p->pPicture, p->pDiOutPicture,
        p->pDiOutPicture1, p->pDiOutTnr, p->nProcessNum,
        p->diParams.diEnableDemo, p->diParams.diRect.left, p->diParams.diRect.top, p->diParams.diRect.right,
        p->diParams.diRect.bottom, p->diParams.diEnableTnr, p->diParams.diTnrMode, p->diParams.diTnrStrength, p->diParams.diEnableFmd);
#ifdef __ANDROID__
#if defined(DEBUG_DI)
    //for debug
    ret = setDiParam();
    if(ret == 0)
    {
        VideoRenderCompSetDiDemo(p, 1, 0, 0, p->pPicture->nWidth, p->pPicture->nHeight);
        VideoRenderCompSetDiTnr(p, 1, 1, 0);
        VideoRenderCompSetDiFmd(p, 1);
    }
    else if(ret == 1)//close demo
    {
        VideoRenderCompSetDiDemo(p, 0, 0, 0, 0, 0);
    }
    else if(ret == 2)//open demo
    {
        VideoRenderCompSetDiDemo(p, 1, 0, 0, p->pPicture->nWidth/2, p->pPicture->nHeight/2);
    }
    else if(ret == 3)//close tnr
    {
        VideoRenderCompSetDiTnr(p, 0, 0, 0);
    }
    else if(ret == 4)//open tnr
    {
        VideoRenderCompSetDiTnr(p, 1, 1, 0);
    }
    else if(ret == 5)//close fmd
    {
        VideoRenderCompSetDiFmd(p, 0);
    }
    else if(ret == 6)//open fmd
    {
        VideoRenderCompSetDiFmd(p, 1);
    }

#endif
#endif
    logv("ret=%d, diEnableDemo=%d, rect(%d, %d, %d, %d), "
         "diEnableTnr=%d, diTnrMode=%d, diTnrStrength=%d, diEnableFmd=%d",
        ret, p->diParams.diEnableDemo, p->diParams.diRect.left, p->diParams.diRect.top, p->diParams.diRect.right,
        p->diParams.diRect.bottom, p->diParams.diEnableTnr, p->diParams.diTnrMode, p->diParams.diTnrStrength, p->diParams.diEnableFmd);
    cdx_int64 t0 = CdxGetNowUs();
    int diret = CdxDiProcess3(p->di,
                      p->pPrePrePicture,
                      p->pPrePicture,
                      p->pPicture,
                      p->pDiOutPicture,
                      p->pDiOutPicture1,
                      p->pDiOutTnr,
                      nDeinterlaceTime,
                      p->bProgressiveFlag == 1 ? 1 : 0,
                      &p->diParams);
    if (diret != 0)
    {
        logw("CdxDiProcess3 failed.");
        if(p->bProgressiveFlag == 0)
        {
            CdxDiReset(p->di);
            if(p->pPrePrePicture != NULL && p->pPrePrePicture != p->pPrePicture)
            {
                if(p->nProcessNum <= 3)//f0, f1, f2; f0, f1 returned from here.
                {
                    VideoDecCompReturnPicture(p->pDecComp, p->pPrePrePicture);
                }
                else //queue pDiOutTnr.
                {
                    LayerQueueBuffer(p->pLayerCtrl, p->pPrePrePicture, 0);
                }
            }

            if(p->pPrePicture != NULL && p->pPrePicture != p->pPicture)
            {
                if(p->nProcessNum <= 2)//f0, f1, f2; f0, f1 returned from here.
                {
                    VideoDecCompReturnPicture(p->pDecComp, p->pPrePicture);
                }
                else //queue pDiOutTnr.
                {
                    LayerQueueBuffer(p->pLayerCtrl, p->pPrePicture, 0);
                }
            }

            if(p->pDiOutPicture != NULL)
                LayerQueueBuffer(p->pLayerCtrl, p->pDiOutPicture, 0);
            if(p->pDiOutPicture1 != NULL)
                LayerQueueBuffer(p->pLayerCtrl, p->pDiOutPicture1, 0);
            if(p->pDiOutTnr != NULL)
                LayerQueueBuffer(p->pLayerCtrl, p->pDiOutTnr, 0);
            p->pPrePrePicture = NULL;
            p->pPrePicture = NULL;
            p->pDiOutPicture = NULL;
            p->pDiOutPicture1 = NULL;
            p->pDiOutTnr = NULL;
        }
        else
        {
            CdxDiReset(p->di);
            if(p->pDiOutTnr != NULL)
                LayerQueueBuffer(p->pLayerCtrl, p->pDiOutTnr, 0);
            p->pPrePicture = NULL;
            p->pDiOutTnr = NULL;
        }

        return VIDEO_RENDER_DEINTERLACE_RESET;
    }
    cdx_int64 t1 = CdxGetNowUs();
    logv("CdxDiProcess3 time: %lld ms", (t1-t0)/1000);
#else

    int diret = CdxDiProcess(p->di, p->pPrePicture,
                      p->pPicture,
                      p->pDiOutPicture,
                      nDeinterlaceTime);
    if (diret != 0)
    {
        CdxDiReset(p->di);

        VideoDecCompReturnPicture(p->pDecComp, p->pPrePicture);
        LayerQueueBuffer(p->pLayerCtrl, p->pDiOutPicture, 0);
        p->pPrePicture = NULL;
        p->pDiOutPicture = NULL;
        return VIDEO_RENDER_DEINTERLACE_RESET;
    }

#endif

    return 0;
}

static int RenderGetVideoFbmBufInfo(VideoRenderComp* p)
{
    enum EPIXELFORMAT eDisplayPixelFormat = PIXEL_FORMAT_DEFAULT;
    FbmBufInfo* pFbmBufInfo =  VideoDecCompGetVideoFbmBufInfo(p->pDecComp);

    logv("pFbmBufInfo = %p",pFbmBufInfo);

    if(pFbmBufInfo == NULL)
        return -1;

#if !defined(CONF_HIGH_DYNAMIC_RANGE_ENABLE)
    //if(pFbmBufInfo->b10bitVideoFlag || pFbmBufInfo->bHdrVideoFlag)
    if(pFbmBufInfo->bHdrVideoFlag)
    {
        logd("***video render hdr stream unsupport.");
        p->callback(p->pUserData, PLAYER_VIDEO_RENDER_NOTIFY_10BIT_UNSUPPORT, NULL);
        return -2;
    }
#endif

    p->mFbmBufInfo = *pFbmBufInfo;
     //* We check whether it is a 3D stream here,
    //* because Layer must know whether it 3D stream at the beginning;
    p->bVideoWithTwoStream = IsVideoWithTwoStream(p->pDecComp);

    logd("video buffer info: nWidth[%d],nHeight[%d],nBufferCount[%d],ePixelFormat[%d]",
          pFbmBufInfo->nBufWidth,pFbmBufInfo->nBufHeight,
          pFbmBufInfo->nBufNum,pFbmBufInfo->ePixelFormat);
    logd("video buffer info: nAlignValue[%d],bProgressiveFlag[%d],bIsSoftDecoderFlag[%d]",
          pFbmBufInfo->nAlignValue,pFbmBufInfo->bProgressiveFlag,
          pFbmBufInfo->bIsSoftDecoderFlag);

    if (p->pLayerCtrl == NULL)
    {
        logw("lc is NULL");
        return 0;
    }

    //* we init deinterlace device here
    if(p->di != NULL && p->bDisableDeinterlace == 0 && (pFbmBufInfo->bProgressiveFlag == 0 || p->bProcessWithDi300 == 1))
        InitDeinterlace(p);

    eDisplayPixelFormat = p->bDeinterlaceFlag ?
        CdxDiExpectPixelFormat(p->di) : (enum EPIXELFORMAT)pFbmBufInfo->ePixelFormat;

    LayerControl(p->pLayerCtrl, CDX_LAYER_CMD_SET_HDR_INFO, pFbmBufInfo);
    LayerSetDisplayPixelFormat(p->pLayerCtrl, eDisplayPixelFormat);
    LayerSetDisplayBufferSize(p->pLayerCtrl, pFbmBufInfo->nBufWidth,
            pFbmBufInfo->nBufHeight);
    LayerSetVideoWithTwoStreamFlag(p->pLayerCtrl, p->bVideoWithTwoStream);
    LayerSetIsSoftDecoderFlag(p->pLayerCtrl, pFbmBufInfo->bIsSoftDecoderFlag);
    p->nGpuBufferNum = LayerSetDisplayBufferCount(p->pLayerCtrl,
            pFbmBufInfo->nBufNum);

    p->bHadSetLayerInfoFlag  = 1;
    p->bProgressiveFlag = pFbmBufInfo->bProgressiveFlag;

    return 0;
}

static int SetGpuBufferToDecoder(VideoRenderComp*p)
{
    VideoPicture mTmpVideoPicture;
    int i;
    VideoPicture* pTmpVideoPicture = &mTmpVideoPicture;
    int nLayerBufferNum = LayerGetBufferNumHoldByGpu(p->pLayerCtrl);
    memset(pTmpVideoPicture, 0, sizeof(VideoPicture));
    p->nSetToDecoderBufferNum = 0;
    p->nCancelPictureNum = 0;

    for(i = 0; i< p->nGpuBufferNum; i++)
    {
        int ret = LayerDequeueBuffer(p->pLayerCtrl, &pTmpVideoPicture, 1);
        if(ret == 0)
        {
            if (i >= p->nGpuBufferNum - nLayerBufferNum)
            {
                if(p->nCancelPictureNum >= CANCEL_BUFFER_NUM)
                {
                    loge("the pCancelPicture is out of range");
                    abort();
                }
                p->pCancelPicture[p->nCancelPictureNum] =
                    VideoDecCompSetVideoFbmBufAddress(p->pDecComp, pTmpVideoPicture, 1);
                p->nCancelPictureNum ++;
            }
            else
            {
                VideoDecCompSetVideoFbmBufAddress(p->pDecComp, pTmpVideoPicture, 0);
            }
            p->nSetToDecoderBufferNum ++;
        }
        else
        {
            if(i < p->nGpuBufferNum - 1)
            {
                logw("*** dequeue %d-th buffer for total %d buffers failed.",
                                i, p->nGpuBufferNum);
                return -1;
            }
            // we cannot dequeue the last buffer form native window right now
            // in the case of  resolution change. so dequeue it after queue the
            // first picture to native window
            logw("*** dequeue buffer failed when set-buffer-to-decoder");
            //abort();
        }
    }

    if(p->nGpuBufferNum == p->nSetToDecoderBufferNum)
    {
        for (i = 0; i < nLayerBufferNum; ++i)
        {
            LayerQueueBuffer(p->pLayerCtrl, p->pCancelPicture[i], 0);
        }
    }

    return 0;
}

// release the buffer from the old layer;
// deuqeuebuffer from new layer;
// set the buffer to decoder
static int ResetBufToDecoder(VideoRenderComp*p)
{
    int ret = 0;
    VideoPicture* pReleasePicture = NULL;

    if(p->bHadRequestReleasePicFlag == 0)
    {
        // 1. release the buffer from the old layer;
        pReleasePicture = VideoDecCompRequestReleasePicture(p->pDecComp);

        logv("*** pReleasePicture(%p),nNeedReleaseBufferNum(%d)",
             pReleasePicture,p->nNeedReleaseBufferNum);

        if(pReleasePicture != NULL)
        {
            LayerReleaseBuffer(p->pLayerCtrl, pReleasePicture); //old
        }
        else
        {
            //* we drop the picture here, or the decoder will block and
            //* can not return all the ReleasePicture
            VideoPicture* pRequestPic = NULL;
            int nResolutionChange = 0;
            pRequestPic = VideoDecCompRequestPicture(p->pDecComp, 0, &nResolutionChange);
            logv("pRequestPic=%p", pRequestPic);
            if(pRequestPic != NULL)
                VideoDecCompReturnPicture(p->pDecComp, pRequestPic);
        }
    }

    if(p->bHadRequestReleasePicFlag == 1 || pReleasePicture != NULL)
    {
        VideoPicture mTmpReturnPicture;
        memset(&mTmpReturnPicture, 0, sizeof(VideoPicture));
        VideoPicture* pTmpReturnPicture = &mTmpReturnPicture;

        // 2. dequeuebuffer from new layer;
        ret = LayerDequeueBuffer(p->pLayerCtrl, &pTmpReturnPicture, 1);
        if(ret == 0)
        {
            if(p->nNeedReleaseBufferNum <= LayerGetBufferNumHoldByGpu(p->pLayerCtrl))
            {
                // 4. set the buffer to decoder and keep Num buffers for GPU.
                pTmpReturnPicture = VideoDecCompReturnRelasePicture(p->pDecComp,
                        pTmpReturnPicture, 1);
                LayerQueueBuffer(p->pLayerCtrl, pTmpReturnPicture, 0);
            }
            else
            {
                // 3. set the buffer to decoder
                VideoDecCompReturnRelasePicture(p->pDecComp, pTmpReturnPicture, 0);
            }

            if(p->bHadRequestReleasePicFlag == 1)
            {
                p->bHadRequestReleasePicFlag = 0;
            }

            p->nNeedReleaseBufferNum--;

            if(p->nNeedReleaseBufferNum <= 0)
            {
                p->bResetBufToDecoderFlag = 0;
                p->callback(p->pUserData, PLAYER_VIDEO_RENDER_RESET_BUFFER_FINISHED, 0);
            }
        }
        else
        {
            p->bHadRequestReleasePicFlag = 1;
            logd("LayerDequeueBuffer fail.");
            return VIDEO_RENDER_THREAD_CONTINUE;
        }
    }
    return 0;
}

#if defined(CONF_DI_300_SUPPORT)
/**
* @enable: open or close di demo,0: close; 1: open
* @(left, top)(right, bottom): demo window coordinate
*/
void VideoRenderCompSetDiDemo(VideoRenderComp* p, int enable, int left, int top, int right, int bottom)
{
    p->diParams.diEnableDemo = enable;

    if(enable == 0) //close demo
    {
        VideoStreamInfo videoStreamInfo;
        VideoDecCompGetVideoStreamInfo(p->pDecComp, &videoStreamInfo);
        p->diParams.diRect.left = 0;
        p->diParams.diRect.top = 0;
        p->diParams.diRect.right = videoStreamInfo.nWidth;
        p->diParams.diRect.bottom = videoStreamInfo.nHeight;
    }
    else
    {
        p->diParams.diRect.left = left;
        p->diParams.diRect.top = top;
        p->diParams.diRect.right = right;
        p->diParams.diRect.bottom = bottom;
    }

    p->diParams.diEnableTnr = 1;
    p->diParams.diTnrMode = 2;// FIX_MODE
    p->diParams.diTnrStrength = 0;// HIGH_LEVEL
    p->diParams.diEnableFmd = 1;

    return;
}

/**
* @enable: open or close tnr, 0: close; 1: open
* @mode: 1: MODE_ADAPTIVE; 2: MODE_FIX
* @strength: 0: LEVEL_HIGH; 1: LEVEL_MIDDLE; 2: LEVEL_LOW
*/
/* @mode:
enum {
    DI_TNR_MODE_INVALID = 0,
    DI_TNR_MODE_ADAPTIVE,
    DI_TNR_MODE_FIX,
};

* @strength:
enum {
    DI_TNR_LEVEL_HIGH = 0,
    DI_TNR_LEVEL_MIDDLE,
    DI_TNR_LEVEL_LOW,
};
*/
void VideoRenderCompSetDiTnr(VideoRenderComp* p, int enable, int mode, int strength)
{
    p->diParams.diEnableTnr = enable;

    if(enable == 0) //close tnr
    {
        VideoStreamInfo videoStreamInfo;
        VideoDecCompGetVideoStreamInfo(p->pDecComp, &videoStreamInfo); //dit_demo may need
        p->diParams.diRect.left = 0;
        p->diParams.diRect.top = 0;
        p->diParams.diRect.right = videoStreamInfo.nWidth;
        p->diParams.diRect.bottom = videoStreamInfo.nHeight;
    }
    else
    {
        if(mode < 1 || mode > 2)
        {
            logw("invalid tnr mode(%d), check...", mode);
            mode = 2;
        }
        if(strength < 0 || strength > 2)
        {
            logw("invalid tnr strength(%d), check...", strength);
            strength = 0;
        }
        p->diParams.diTnrMode = mode;
        p->diParams.diTnrStrength = strength;
    }

    return;
}

/**
* @enable: open or close fmd, 0: close; 1: open
*/
void VideoRenderCompSetDiFmd(VideoRenderComp* p, int enable)
{
    p->diParams.diEnableFmd = enable;
    return;
}
#endif

static int InitDeinterlace(VideoRenderComp* p)
{
    VideoStreamInfo videoStreamInfo;
    VideoDecCompGetVideoStreamInfo(p->pDecComp, &videoStreamInfo);

#if defined(CONF_HIGH_QUALITY_DEINTERLACE)
    int thresholdwidth = 2048;//high quality
    int thresholdheight = 1088;
#else
    int thresholdwidth = 1280;//default
    int thresholdheight = 720;
#endif

    if(videoStreamInfo.nWidth <= thresholdwidth && videoStreamInfo.nHeight <= thresholdheight &&
        videoStreamInfo.nFrameRate <= 30000)
    {
#if defined(CONF_DI_300_SUPPORT) //di300 need yv12 input, vp9 and 3d stream not support.
        if (videoStreamInfo.eCodecFormat == VIDEO_CODEC_FORMAT_VP9 || videoStreamInfo.bIs3DStream == 1)
        {
            logi("can not output yv12.");
            return 0;
        }
#endif
        if (CdxDiInit(p->di) == 0)
        {
            int di_flag = CdxDiFlag(p->di);
            p->bDeinterlaceFlag   = 1;
            p->nDeinterlaceDispNum      = (di_flag == DE_INTERLACE_HW) ? 2 : 1;
        }
        else
        {
            logw("open deinterlace failed , we not to use deinterlace!");
        }
    }
    else
    {
        logw("do not need open deinterlace, video_size-%dx%d > threshold_size-%dx%d! rate =%d!",
                       videoStreamInfo.nWidth, videoStreamInfo.nHeight,thresholdwidth,thresholdheight,videoStreamInfo.nFrameRate);
    }

    return 0;
}
