/*
 * Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : deinterlace.h
 * Description : deinterlace
 * History :
 *
 */

#ifndef DEINTERLACE_H
#define DEINTERLACE_H

#include <cdx_config.h>
#include <vdecoder.h>
#include "cdx_log.h"

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

#if defined(CONF_DI_300_SUPPORT)
typedef struct CdxDeinterlaceParamS CdxDeinterlaceParamT;
struct DiRectS
{
    int left;
    int top;
    int right;
    int bottom;
};

struct CdxDeinterlaceParamS
{
    int diEnableDemo;
    struct DiRectS diRect;
    int diEnableTnr;
    int diTnrMode;
    int diTnrStrength;
    int diEnableFmd;
};
#endif

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
                        int nField,
                        int nTnrOnly,
                        CdxDeinterlaceParamT *diParams);

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

static inline void CdxDiDestroy(Deinterlace* di)
{
    CDX_CHECK(di);
    CDX_CHECK(di->ops);
    CDX_CHECK(di->ops->destroy);
    return di->ops->destroy(di);
}

static inline int CdxDiInit(Deinterlace* di)
{
    CDX_CHECK(di);
    CDX_CHECK(di->ops);
    CDX_CHECK(di->ops->init);
    return di->ops->init(di);
}

static inline int CdxDiReset(Deinterlace* di)
{
    CDX_CHECK(di);
    CDX_CHECK(di->ops);
    CDX_CHECK(di->ops->reset);
    return di->ops->reset(di);
}

static inline enum EPIXELFORMAT CdxDiExpectPixelFormat(Deinterlace* di)
{
    CDX_CHECK(di);
    CDX_CHECK(di->ops);
    CDX_CHECK(di->ops->expectPixelFormat);
    return di->ops->expectPixelFormat(di);
}

static inline int CdxDiFlag(Deinterlace* di)
{
    CDX_CHECK(di);
    CDX_CHECK(di->ops);
    CDX_CHECK(di->ops->flag);
    return di->ops->flag(di);
}

#if defined(CONF_DI_V2X_SUPPORT)
static inline int CdxDiProcess2(Deinterlace* di,
                        VideoPicture *pPrePicture,
                        VideoPicture *pCurPicture,
                        VideoPicture *pNextPicture,
                        VideoPicture *pOutPicture,
                        int nField)
{
    CDX_CHECK(di);
    CDX_CHECK(di->ops);
    CDX_CHECK(di->ops->process2);
    return di->ops->process2(di, pPrePicture, pCurPicture,pNextPicture,
                    pOutPicture, nField);
}

#elif defined(CONF_DI_300_SUPPORT)
static inline int CdxDiProcess3(Deinterlace* di,
                    VideoPicture *pPrePrePicture,
                    VideoPicture *pPrePicture,
                    VideoPicture *pCurPicture,
                    VideoPicture *pOutPicture0,
                    VideoPicture *pOutPicture1,
                    VideoPicture *pOutTnr,
                    int nField,
                    int nTnrOnly,
                    CdxDeinterlaceParamT *diParams)
{
    CDX_CHECK(di);
    CDX_CHECK(di->ops);
    CDX_CHECK(di->ops->process3);
    return di->ops->process3(di, pPrePrePicture, pPrePicture, pCurPicture,
                            pOutPicture0, pOutPicture1, pOutTnr, nField, nTnrOnly, diParams);
}
#else
static inline int CdxDiProcess(Deinterlace* di,
                        VideoPicture *pPrePicture,
                        VideoPicture *pCurPicture,
                        VideoPicture *pOutPicture,
                        int nField)
{
    CDX_CHECK(di);
    CDX_CHECK(di->ops);
    CDX_CHECK(di->ops->process);
    return di->ops->process(di, pPrePicture, pCurPicture,
                    pOutPicture, nField);
}
#endif

#ifdef __cplusplus
}
#endif

#endif
