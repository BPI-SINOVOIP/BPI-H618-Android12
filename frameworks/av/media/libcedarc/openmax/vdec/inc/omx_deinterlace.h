/*
 * Copyright (c) 2008-2018 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : deinterlace.h
 * Description : deinterlace
 * History :
 *
 */

#ifndef OMX_DEINTERLACE_H
#define OMX_DEINTERLACE_H
#include "vdecoder.h"

#ifdef __cplusplus
extern "C" {
#endif

enum DE_INTERLACE_FLAG
{
    DE_INTERLACE_NONE,
    DE_INTERLACE_HW,
    DE_INTERLACE_SW
};

typedef struct CdxDeinterlaceS Deinterlace;

typedef struct DeinterlaceOps DeinterlaceOpsT;
struct DeinterlaceOps
{
    void (*destroy)(Deinterlace* di);

    int (*init)(Deinterlace* di);

    int (*reset)(Deinterlace* di);

    enum EPIXELFORMAT (*expectPixelFormat)(Deinterlace* di);

    int (*flag)(Deinterlace* di);

#if defined(CONF_DI_V2X_SUPPORT)
    int (*process2)(Deinterlace* di,
                        VideoPicture *pPrePicture,
                        VideoPicture *pCurPicture,
                        VideoPicture *pNextPicture,
                        VideoPicture *pOutPicture,
                        int nField);

#elif defined(CONF_DI_300_SUPPORT)
    int (*process3)(Deinterlace* di,
                        VideoPicture *pPrePrePicture,
                        VideoPicture *pPrePicture,
                        VideoPicture *pCurPicture,
                        VideoPicture *pOutPicture0,
                        VideoPicture *pOutPicture1,
                        VideoPicture *pOutPicture2,
                        int nField);

#else
int (*process)(Deinterlace* di,
                    VideoPicture *pPrePicture,
                    VideoPicture *pCurPicture,
                    VideoPicture *pOutPicture,
                    int nField);
#endif
};

struct CdxDeinterlaceS
{
    struct DeinterlaceOps *ops;
};

static inline void CdcDiDestroy(Deinterlace* di)
{
    return di->ops->destroy(di);
}

static inline int CdcDiInit(Deinterlace* di)
{
    return di->ops->init(di);
}

static inline int CdcDiReset(Deinterlace* di)
{
    return di->ops->reset(di);
}

static inline enum EPIXELFORMAT CdcDiExpectPixelFormat(Deinterlace* di)
{
    return di->ops->expectPixelFormat(di);
}

static inline int CdcDiFlag(Deinterlace* di)
{
    return di->ops->flag(di);
}

#if defined(CONF_DI_V2X_SUPPORT)
static inline int CdcDiProcess2(Deinterlace* di,
                        VideoPicture *pPrePicture,
                        VideoPicture *pCurPicture,
                        VideoPicture *pNextPicture,
                        VideoPicture *pOutPicture,
                        int nField)
{

    return di->ops->process2(di, pPrePicture, pCurPicture,pNextPicture,
                    pOutPicture, nField);
}

#elif defined(CONF_DI_300_SUPPORT)
static inline int CdcDiProcess3(Deinterlace* di,
                    VideoPicture *pPrePrePicture,
                    VideoPicture *pPrePicture,
                    VideoPicture *pCurPicture,
                    VideoPicture *pOutPicture0,
                    VideoPicture *pOutPicture1,
                    VideoPicture *pOutTnr,
                    int nField)
{

    return di->ops->process3(di, pPrePrePicture, pPrePicture, pCurPicture,
                            pOutPicture0, pOutPicture1, pOutTnr, nField);
}
#else
static inline int CdcDiProcess(Deinterlace* di,
                        VideoPicture *pPrePicture,
                        VideoPicture *pCurPicture,
                        VideoPicture *pOutPicture,
                        int nField)
{

    return di->ops->process(di, pPrePicture, pCurPicture,
                    pOutPicture, nField);
}
#endif
Deinterlace* DeinterlaceCreate_Omx();

#ifdef __cplusplus
}
#endif

#endif

