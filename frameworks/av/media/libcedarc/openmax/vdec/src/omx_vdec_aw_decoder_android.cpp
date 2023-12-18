/*
 * =====================================================================================
 *   Copyright (c)  Allwinner Technology Co. Ltd.
 *   All rights reserved.
 *       Filename:  omx_vdec_aw_decoder_android.cpp
 *
 *    Description: 1. cpp's required because the android namespace ,
 *                        2. hardware decode with AW decoder
 *
 *        Version:  1.0
 *        Created:  08/02/2018 04:18:11 PM
 *       Revision:  none
 *
 *         Author:  Gan Qiuye
 *
 * =====================================================================================
 */

#define LOG_TAG "omx_vdec_aw"
#include "cdc_log.h"
#include "cdc_config.h"
#include "omx_vdec_decoder.h"
#include "vdecoder.h"
#include "omx_vdec_port.h"
//#include "gralloc_metadata/sunxi_metadata_def.h"
#include "omx_vdec_config.h"
#include "OMX_Video.h"
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include "memoryAdapter.h"
#include "veAdapter.h"
#include "veInterface.h"
#include "CdcIonUtil.h"
#include "omx_mutex.h"
#include "omx_sem.h"

#include <ion/ion.h>
#include <linux/ion.h>
#include <cutils/properties.h>
#ifdef GPU_PUBLIC_INCLUDE
#include GPU_PUBLIC_INCLUDE
#else
#include <hal_public.h>
#endif
#include <binder/IPCThreadState.h>
#include <media/stagefright/foundation/ADebug.h>
#include <ui/GraphicBufferMapper.h>
#include <ui/Rect.h>
#include <HardwareAPI.h>

#define REQUEST_SIZE (2*1024*1024 + 1)

#define SBM_SIZE_FOR_ZERO_COPY_MODE (32*1024*1024)

#ifdef CONF_Q_AND_NEWER
#include <hardware/graphics-sunxi.h>
#include <gralloc1.h>
#endif
using namespace android;

#if (defined CONF_KERNEL_VERSION_3_10) || \
    (defined CONF_KERNEL_VERSION_4_4) || \
    (defined CONF_KERNEL_VERSION_4_9)
    typedef ion_user_handle_t ion_handle_abstract_t;
    #define ION_NULL_VALUE (0)
#else
    typedef struct ion_handle* ion_handle_abstract_t;
    #define ION_NULL_VALUE (NULL)
#endif

#if defined(CONF_DI_V2X_SUPPORT)
#define DI_PROCESS_3_PICTURE (1)
#elif  defined(CONF_DI_300_SUPPORT)
#define DI_PROCESS_3_PICTURE (1)
#else
#define DI_PROCESS_3_PICTURE (0)
#endif

#define UPDATE_COLOR_ASPECTS_WITH_BITSTREAM  (1)

typedef struct OMXOutputBufferInfoA {
    ANativeWindowBuffer*  pWindowBuf;
#if (defined CONF_KERNEL_VERSION_3_10) || (defined CONF_KERNEL_VERSION_4_4) \
    || (defined CONF_KERNEL_VERSION_4_9)
    ion_user_handle_t handle_ion;
#else
    struct ion_handle *handle_ion;
#endif     //ion_handle_abstract_t handle_ion;
    OMX_BUFFERHEADERTYPE* pHeadBufInfo;
    char*                 pBufPhyAddr;
    char*                 pBufVirAddr;
    VideoPicture*         pVideoPicture;
    OutBufferStatus       mStatus;
    OMX_BOOL              mUseDecoderBufferToSetEosFlag;
    int                   nBufFd;
    int                   nVirAddrSize;
    void*                 pMetaDataVirAddr;
    int                   nMetaDataVirAddrSize;
    int                   nMetaDataMapFd;
    ion_handle_abstract_t metadata_handle_ion;
 }OMXOutputBufferInfoA;

typedef struct TranBufferInfo
{
    void* pAddr;
    int   nWidth;
    int   nHeight;
}TranBufferInfo;

typedef struct DiOutBufferInfo
{
    VideoPicture* pArrPic[DI_BUFFER_NUM_KEEP_IN_OMX];
    OMX_S32 nValidSize;
}DiOutBufferInfo;

typedef struct OmxAwDecoderContext
{
    OmxDecoder             base;
    VideoDecoder*          m_decoder;
    VideoStreamInfo        mStreamInfo;
    VConfig                mVideoConfig;
    OMX_S32                mGpuAlignStride;
    struct ScMemOpsS*      decMemOps;
    int                    mIonFd;
    OMX_U32                mMaxWidth;
    OMX_U32                mMaxHeight;

    OMX_CONFIG_RECTTYPE    mVideoRect; //* for display crop
#ifdef CONF_PIE_AND_NEWER
    ColorAspects           mDefaultColorAspects;
    ColorAspects           mBitStreamColorAspects;
    ColorAspects           mFinalColorAspects;
    HDRStaticInfo          mHdrStaticInfo;
#endif
    OMX_U32                mCodecSpecificDataLen;
    OMX_U8                 mCodecSpecificData[CODEC_SPECIFIC_DATA_LENGTH];
    OMXOutputBufferInfoA   mOutputBufferInfoArr[OMX_MAX_VIDEO_BUFFER_NUM];
    DecoderCallback        callback;
    void*                  pUserData;
    OMX_U32                mSetToDecoderBufferNum;
    OMX_U32                mNeedReSetToDecoderBufferNum;
    OMX_U32                mOutBufferNumDecoderNeed;
    OMX_S32                mExtraOutBufferNum;
    OMX_BOOL               bHadGetVideoFbmBufInfoFlag;
    OMX_BOOL               bHadInitDecoderFlag;
    OMX_BOOL               bInputEosFlag;
    OMX_BOOL               bIsSecureVideoFlag;
    OMX_BOOL               bSurpporNativeHandle;
    OMX_BOOL               bStoreOutputMetaDataFlag;
    OMX_BOOL               bUseAndroidBuffer;
    OMX_BOOL               bFlushFlag;
    OMX_BOOL               bUseZeroCopyInBuffer;
    OMX_BOOL               bUseZeroCopyOutBuffer;
    OMX_U32                nActualColorFormat;
    OMX_U32                nExtendFlag;

    AwOmxVdecPort*         pInPort;
    AwOmxVdecPort*         pOutPort;

    OmxMutexHandle         awMutexHandler;
    OmxSemHandle           mSemOutBuffer;
    OmxSemHandle           mSemInData;
    OmxSemHandle           mSemValidPic;

    VideoPicture*          pPicture;
    OMX_U32                mPicNum;
    OMX_U8*                pSbmStart;
    OMX_U8*                pSbmEnd;

    Deinterlace*           mDi;
    OMX_S32                mEnableDiFlag;
    OMX_S32                mDiProcessNum;   //* 1 or 2. the count calling CdxProcess with same inputs.
    OMX_S32                mDiProcessCount; //* from 0 to 1 for V2X DI.
    VideoPicture*          pPrePrePicture;
    VideoPicture*          pPrePicture;
    VideoPicture*          pCurPicture;
    DiOutBufferInfo        mDiOutBufferInfo;
#ifdef CONF_DI_300_SUPPORT
    OMX_BUFFERHEADERTYPE*  pOutBufHdr4DiOutPic1;
    OMX_S32                mDi300ProcessCount; //* accumulate count in DI300 process.
    OMX_U32                mDiBufferIndex1;
#endif
    OMX_S32                mSbmOffset; //*for SubmitZeroCopy
    OMX_BOOL               bSubmit;    //*for SubmitZeroCopy
    OMX_S64                nPreSubmitPts;//for caculate frameRate
    OMX_S32                nSpecificationFlag;
}OmxAwDecoderContext;

#if ENABLE_SHOW_BUFINFO_STATUS
static void showBufferInfoArrStatus(OmxAwDecoderContext *pCtx)
{
    int i = 0, nOwnedByUs = 0, nOwnedByDecoder = 0, nOwnedByRender = 0, nOwnedByTnr = 0;
    int nOwnedByPending = 0;
    //omx_logd("***********show status begin*********");
    for(i = 0; i < OMX_MAX_VIDEO_BUFFER_NUM; i++)
    {
        if(pCtx->mOutputBufferInfoArr[i].mStatus == OWNED_BY_NONE)
        {
            /*omx_logd("i:%d~%d,pHeadBufInfo:      0x0, pic:       0x0, "
                 "status: OWNED_BY_NONE",
                i, OMX_MAX_VIDEO_BUFFER_NUM - 1);*/
            break;
        }
        if(pCtx->mOutputBufferInfoArr[i].mStatus == OWNED_BY_US)
            nOwnedByUs++;
        else if(pCtx->mOutputBufferInfoArr[i].mStatus == OWNED_BY_DECODER)
            nOwnedByDecoder++;
        else if(pCtx->mOutputBufferInfoArr[i].mStatus == OWNED_BY_RENDER)
            nOwnedByRender++;
        else if(pCtx->mOutputBufferInfoArr[i].mStatus == OWNED_BY_TNR)
            nOwnedByTnr++;
        else if(pCtx->mOutputBufferInfoArr[i].mStatus == OWNED_BY_PENDING)
            nOwnedByPending++;
        /*omx_logv("i:%2d, pHeadBufInfo:%10p, pic:%10p, status: %s fd: %d",
            i,
            pCtx->mOutputBufferInfoArr[i].pHeadBufInfo,
            pCtx->mOutputBufferInfoArr[i].pVideoPicture,
            statusToString(pCtx->mOutputBufferInfoArr[i].mStatus),
            pCtx->mOutputBufferInfoArr[i].pVideoPicture->nBufFd);*/
    }
    omx_logd("buffers owned by render [%d] by decoder [%d] by us[%d] by tnr[%d] by pending[%d].",
            nOwnedByRender, nOwnedByDecoder, nOwnedByUs, nOwnedByTnr, nOwnedByPending);
    //omx_logd("***********show status end***********");
}
#endif

static void savePictureForDebug(OmxAwDecoderContext *pCtx,VideoPicture* pVideoPicture)
{
    int nTotalSavePicNum = 30;

#ifdef __ANDROID__
    char val[256];
    property_get("vendor.vdec.totalsavepicnum", val, "0");
    if(atoi(val) > 0 && atoi(val) <= 100)
        nTotalSavePicNum = atoi(val);
#endif

    pCtx->mPicNum++;
    if(pCtx->mPicNum <= nTotalSavePicNum)
    {
        char  path[1024] = {0};
        FILE* fpStream   = NULL;
        int   len = 0;
        OMX_PARAM_PORTDEFINITIONTYPE* outDef = getPortDef(pCtx->pOutPort);
        int   width = outDef->format.video.nFrameWidth;
        int   height = outDef->format.video.nFrameHeight;
        omx_logd("save picture: w[%d],h[%d],pVirBuf[%p]",
              width, height, pVideoPicture->pData0);

        sprintf (path,"/data/camera/pic%d.dat",(int)pCtx->mPicNum);
        fpStream = fopen(path, "wb");
        if(pVideoPicture->bEnableAfbcFlag)
            len = pVideoPicture->nAfbcSize;
        else
            len  = (width*height)*3/2;
        if(fpStream != NULL)
        {
            fwrite(pVideoPicture->pData0,1,len, fpStream);
            fclose(fpStream);
        }
        else
        {
            omx_logd("++the fpStream is null when save picture");
        }
    }

    return;
}
static void judgeSbmOffset(OmxAwDecoderContext *pCtx, OMX_U32 size)
{
    pCtx->bSubmit = OMX_TRUE;
    pCtx->mSbmOffset += size;
    if(pCtx->mSbmOffset > pCtx->mVideoConfig.nVbvBufferSize - REQUEST_SIZE)
    {
        pCtx->mSbmOffset = 0;
    }
}

static OMX_S32 setOutBufferStatus(OmxAwDecoderContext *pCtx, VideoPicture* pPicture,
                                     enum OutBufferStatus eStatus)
{
    OMX_U32 i = 0;

    if(pPicture == NULL)
        return -1;

    OMX_U32 num = getPortAllocSize(pCtx->pOutPort);

    for(i=0; i<num; i++)
    {
        if(pCtx->mOutputBufferInfoArr[i].pVideoPicture == pPicture)
            break;
    }
    if(i == num)
    {
        omx_loge("the picture request from decoder is not match");
        return -1;
    }

    pCtx->mOutputBufferInfoArr[i].mStatus = eStatus;
    return 0;
}

#if defined(CONF_DI_300_SUPPORT)
static OMX_S32 equalOutBufferStatus(OmxAwDecoderContext *pCtx, VideoPicture* pPicture,
                                     enum OutBufferStatus eStatus)
{
    OMX_U32 i = 0;

    if(pPicture == NULL)
        return 0;

    OMX_U32 num = getPortAllocSize(pCtx->pOutPort);

    for(i=0; i<num; i++)
    {
        if(pCtx->mOutputBufferInfoArr[i].pVideoPicture == pPicture)
            break;
    }
    if(i == num)
    {
        omx_loge("the picture request from decoder is not match");
        return 0;
    }

    if(pCtx->mOutputBufferInfoArr[i].mStatus == eStatus)
        return 1;
    else
        return 0;
}
#endif

static OMX_U32 anGetStreamFormat(OMX_VIDEO_CODINGTYPE videoCoding)
{
    switch(videoCoding)
    {
        case OMX_VIDEO_CodingMJPEG:
            return VIDEO_CODEC_FORMAT_MJPEG;//mjpeg
        case OMX_VIDEO_CodingMPEG1:
            return VIDEO_CODEC_FORMAT_MPEG1;//mpeg1
        case OMX_VIDEO_CodingMPEG2:
            return VIDEO_CODEC_FORMAT_MPEG2;//mpeg2
        case OMX_VIDEO_CodingMPEG4:
            return VIDEO_CODEC_FORMAT_XVID;//xvid-mpeg4
        case OMX_VIDEO_CodingMSMPEG4V1:
            return VIDEO_CODEC_FORMAT_MSMPEG4V1;//mpeg4v1
        case OMX_VIDEO_CodingMSMPEG4V2:
            return VIDEO_CODEC_FORMAT_MSMPEG4V2;//mpeg4v2
        case OMX_VIDEO_CodingMSMPEG4V3:
            return VIDEO_CODEC_FORMAT_DIVX3;//divx3-mpeg4v3
        case OMX_VIDEO_CodingDIVX4:
            return VIDEO_CODEC_FORMAT_DIVX4;//divx4
        case OMX_VIDEO_CodingRX:
            return VIDEO_CODEC_FORMAT_RX;//rx
        case OMX_VIDEO_CodingAVS:
            return VIDEO_CODEC_FORMAT_AVS;//avs
        case OMX_VIDEO_CodingDIVX:
            return VIDEO_CODEC_FORMAT_DIVX5;//divx5
        case OMX_VIDEO_CodingXVID:
            return VIDEO_CODEC_FORMAT_XVID;//xvid
        case OMX_VIDEO_CodingH263:
            return VIDEO_CODEC_FORMAT_H263;//h263
        case OMX_VIDEO_CodingS263:
            return VIDEO_CODEC_FORMAT_SORENSSON_H263;//sh263-sorensson
        case OMX_VIDEO_CodingRXG2:
            return VIDEO_CODEC_FORMAT_RXG2;//rxg2
        case OMX_VIDEO_CodingWMV1:
            return VIDEO_CODEC_FORMAT_WMV1;//wmv1
        case OMX_VIDEO_CodingWMV2:
            return VIDEO_CODEC_FORMAT_WMV2;//wmv2
        case OMX_VIDEO_CodingWMV:
            return VIDEO_CODEC_FORMAT_WMV3;//wmv3
        case OMX_VIDEO_CodingVP6:
            return VIDEO_CODEC_FORMAT_VP6;//vp6
        case OMX_VIDEO_CodingVP8:
            return VIDEO_CODEC_FORMAT_VP8;//vp8
        case OMX_VIDEO_CodingVP9:
            return VIDEO_CODEC_FORMAT_VP9;//vp9
        case OMX_VIDEO_CodingAVC:
            return VIDEO_CODEC_FORMAT_H264;//h264-avc
        case OMX_VIDEO_CodingHEVC:
            return VIDEO_CODEC_FORMAT_H265;//h265-hevc
        default:
        {
            omx_loge("unsupported OMX this format:%d", videoCoding);
            break;
        }
    }
    return VIDEO_CODEC_FORMAT_UNKNOWN;
}

void anGetStreamInfo(OmxAwDecoderContext *pCtx)
{
    OMX_PARAM_PORTDEFINITIONTYPE* inDef = getPortDef(pCtx->pInPort);
    OMX_VIDEO_PARAM_PORTFORMATTYPE* inFormatType = \
           getPortFormatType(pCtx->pInPort);

    pCtx->mStreamInfo.eCodecFormat = \
           anGetStreamFormat(inFormatType->eCompressionFormat);
    pCtx->mStreamInfo.nWidth  = inDef->format.video.nFrameWidth;
    pCtx->mStreamInfo.nHeight = inDef->format.video.nFrameHeight;
}

static int anDealWithInitData(OmxAwDecoderContext *pCtx)
{
    OMX_BUFFERHEADERTYPE* pInBufHdr = doRequestPortBuffer(pCtx->pInPort);

    if(pInBufHdr == NULL)
    {
        return -1;
    }

    if(pInBufHdr->nFlags & OMX_BUFFERFLAG_CODECCONFIG)
    {
        OMX_ASSERT((pInBufHdr->nFilledLen + pCtx->mCodecSpecificDataLen) <=
                CODEC_SPECIFIC_DATA_LENGTH);
        //omxAddNaluStartCodePrefix(pCtx, pInBufHdr);
        OMX_U8* pInBuffer = NULL;
        if(pCtx->bIsSecureVideoFlag)
        {
            if(pCtx->bSurpporNativeHandle)
            {
                native_handle_t *nh = (native_handle_t *)pInBufHdr->pBuffer;
                pInBuffer = (OMX_U8*)(uintptr_t)nh->data[0];
            }
            else
                pInBuffer = pInBufHdr->pBuffer;

            if(pCtx->decMemOps == NULL)
            {
                pCtx->decMemOps = SecureMemAdapterGetOpsS();
                CdcMemOpen(pCtx->decMemOps);
            }
            CdcMemCopy(pCtx->decMemOps,
                       pCtx->mCodecSpecificData + pCtx->mCodecSpecificDataLen,
                       pInBuffer, pInBufHdr->nFilledLen);
        }
        else
        {

            pInBuffer = pInBufHdr->pBuffer;
            memcpy(pCtx->mCodecSpecificData + pCtx->mCodecSpecificDataLen,
                   pInBuffer,
                   pInBufHdr->nFilledLen);
        }
        pCtx->mCodecSpecificDataLen += pInBufHdr->nFilledLen;
        pCtx->callback(pCtx->pUserData, AW_OMX_CB_EMPTY_BUFFER_DONE,
                        (void*)pInBufHdr);
    }
    else
    {
        omx_logd("++++++++++++++++pCtx->mCodecSpecificDataLen[%d]",
            (int)pCtx->mCodecSpecificDataLen);
        if(pCtx->mCodecSpecificDataLen > 0)
        {
            if(pCtx->mStreamInfo.pCodecSpecificData)
                free(pCtx->mStreamInfo.pCodecSpecificData);
            pCtx->mStreamInfo.nCodecSpecificDataLen = \
                pCtx->mCodecSpecificDataLen;
            pCtx->mStreamInfo.pCodecSpecificData    = \
                (char*)malloc(pCtx->mCodecSpecificDataLen);
            memset(pCtx->mStreamInfo.pCodecSpecificData, 0,
                     pCtx->mCodecSpecificDataLen);
            memcpy(pCtx->mStreamInfo.pCodecSpecificData,
                   pCtx->mCodecSpecificData, pCtx->mCodecSpecificDataLen);
        }
        else
        {
            pCtx->mStreamInfo.pCodecSpecificData    = NULL;
            pCtx->mStreamInfo.nCodecSpecificDataLen = 0;
        }
        doReturnPortBuffer(pCtx->pInPort);
        return 0;
    }
    return -1;
}

static int anSubmitZeroCopy(OmxAwDecoderContext *pCtx, OMX_BUFFERHEADERTYPE* pInBufHdr)
{
    char* pBuf0;
    char* pBuf1;
    int   size0;
    int   size1;
    int   require_size;
    char*   pData;
    native_handle_t *nh = NULL;
    OMX_U8* pInBuffer = NULL;
    VideoStreamDataInfo DataInfo;
    memset(&DataInfo, 0, sizeof(VideoStreamDataInfo));
    OmxAcquireMutex(pCtx->awMutexHandler);
    DataInfo.nLength      = pInBufHdr->nFilledLen;
    DataInfo.bIsFirstPart = 1;
    DataInfo.bIsLastPart  = 1;
    if(pInBufHdr->nFilledLen >= REQUEST_SIZE)
    {
        loge("pInBufHdr->nFilledLen:%lu, REQUEST_SIZE:%d",pInBufHdr->nFilledLen, REQUEST_SIZE);
    }
    if(pInBufHdr->nTimeStamp >= 0)
    {
        DataInfo.nPts   = pInBufHdr->nTimeStamp;
        DataInfo.bValid = 1;
    }
    else
    {
        DataInfo.nPts   = -1;
        DataInfo.bValid = 0;
    }
    if(pCtx->bIsSecureVideoFlag)
    {
        if(pCtx->bSurpporNativeHandle == OMX_TRUE)
        {
            nh = (native_handle_t *)pInBufHdr->pBuffer;
            pInBuffer = (OMX_U8*)(uintptr_t)nh->data[0];
        }
        else
            pInBuffer = pInBufHdr->pBuffer;
#if(ADJUST_ADDRESS_FOR_SECURE_OS_OPTEE)
        pData  = (char*)pInBuffer;
#else
        pData  = (char*)CdcMemGetVirtualAddressCpu(pCtx->decMemOps, pInBuffer);
#endif
    }
    else
    {
       pData = (char*)pInBufHdr->pBuffer;
    }
    if(pCtx->bSubmit)
    {
        pData += pCtx->mSbmOffset;
        OMX_ASSERT(pData <= (char*)pCtx->pSbmEnd);
        DataInfo.pData        = pData;
        SubmitVideoStreamData(pCtx->m_decoder, &DataInfo, 0);
    }
    require_size = REQUEST_SIZE;
    if(RequestVideoStreamBuffer(pCtx->m_decoder, require_size, &pBuf0,
                                &size0, &pBuf1, &size1,0)!= 0)
    {
        loge("req vbs fail! maybe vbs buffer is full! require_size[%d]",
             require_size);
        pCtx->bSubmit = OMX_FALSE;
        OmxReleaseMutex(pCtx->awMutexHandler);
        return -1;
    }
    //* not use ring buffer
    if(require_size != size0)
    {
        logw("the requestSize[%d] is not equal to needSize[%d]!",
              size0,require_size);
        memset(&DataInfo, 0, sizeof(VideoStreamDataInfo));
        DataInfo.nLength      = size0;
        DataInfo.bIsFirstPart = 1;
        DataInfo.bIsLastPart  = 1;
        DataInfo.nPts         = -1;
        DataInfo.bValid       = 0;
        if(pCtx->bSubmit)
            DataInfo.pData    = pData + pInBufHdr->nFilledLen;
        else
            DataInfo.pData    = pData + pCtx->mSbmOffset + pInBufHdr->nFilledLen;
        SubmitVideoStreamData(pCtx->m_decoder, &DataInfo, 0);
        pCtx->bSubmit = OMX_FALSE;
        OmxReleaseMutex(pCtx->awMutexHandler);
        return -1;
    }
    judgeSbmOffset(pCtx, pInBufHdr->nFilledLen);
    OmxTryPostSem(pCtx->mSemInData);
    OmxReleaseMutex(pCtx->awMutexHandler);
    return 0 ;
}

static int anSubmitCopy(OmxAwDecoderContext *pCtx, OMX_BUFFERHEADERTYPE* pInBufHdr)
{
    char* pBuf0;
    char* pBuf1;
    int   size0;
    int   size1;
    int   require_size;
    unsigned char*   pData;
    VideoStreamDataInfo DataInfo;
    memset(&DataInfo, 0, sizeof(VideoStreamDataInfo));
    require_size = pInBufHdr->nFilledLen;
    OmxAcquireMutex(pCtx->awMutexHandler);
    if(RequestVideoStreamBuffer(pCtx->m_decoder, require_size, &pBuf0,
                                &size0, &pBuf1, &size1,0)!= 0)
    {
        loge("req vbs fail! maybe vbs buffer is full! require_size[%d]",
             require_size);
        OmxReleaseMutex(pCtx->awMutexHandler);
        return -1;
    }
    if(require_size != (size0 + size1))
    {
        logw(" the requestSize[%d] is not equal to needSize[%d]!",
              (size0+size1),require_size);
        OmxReleaseMutex(pCtx->awMutexHandler);
        return -1;
    }
    DataInfo.nLength      = require_size;
    DataInfo.bIsFirstPart = 1;
    DataInfo.bIsLastPart  = 1;
    DataInfo.pData        = pBuf0;
    if(pInBufHdr->nTimeStamp >= 0)
    {
        DataInfo.nPts   = pInBufHdr->nTimeStamp;
        DataInfo.bValid = 1;
    }
    else
    {
        DataInfo.nPts   = -1;
        DataInfo.bValid = 0;
    }
    if(pCtx->bIsSecureVideoFlag)
    {
        OMX_U8* pInBuffer = NULL;
        if(pCtx->bSurpporNativeHandle == OMX_TRUE)
        {
            native_handle_t *nh = (native_handle_t *)pInBufHdr->pBuffer;
            pInBuffer = (OMX_U8*)(uintptr_t)nh->data[0];
        }
        else
            pInBuffer = pInBufHdr->pBuffer;
#if(ADJUST_ADDRESS_FOR_SECURE_OS_OPTEE)
        pData  = (unsigned char*)pInBuffer;
#else
        pData  = (unsigned char*)CdcMemGetVirtualAddressCpu(pCtx->decMemOps, pInBuffer);
#endif
        pData += pInBufHdr->nOffset;
        if(require_size <= size0)
        {
            CdcMemCopy(pCtx->decMemOps, pBuf0, pData, require_size);
        }
        else
        {
            CdcMemCopy(pCtx->decMemOps, pBuf0, pData, size0);
            pData += size0;
            CdcMemCopy(pCtx->decMemOps, pBuf1, pData, require_size - size0);
        }
    }
    else
    {
        pData = pInBufHdr->pBuffer + pInBufHdr->nOffset;
        if(require_size <= size0)
        {
            memcpy(pBuf0, pData, require_size);
        }
        else
        {
            memcpy(pBuf0, pData, size0);
            pData += size0;
            memcpy(pBuf1, pData, require_size - size0);
        }
    }
    SubmitVideoStreamData(pCtx->m_decoder, &DataInfo, 0);
    OmxTryPostSem(pCtx->mSemInData);
    OmxReleaseMutex(pCtx->awMutexHandler);
    return 0 ;
}

static int initDeinterlace(OmxAwDecoderContext *pCtx)
{
    #if(DISABLE_DEINTERLACE_FUNCTION == 1)
    pCtx->mEnableDiFlag = 0;

    omx_logd("**** initDeinterlace: enableFlag = %d, num = %d",
        (int)pCtx->mEnableDiFlag, (int)pCtx->mDiProcessNum);
    return 0;
    #endif

    if(CdcDiInit(pCtx->mDi) == 0)
    {
        int di_flag = CdcDiFlag(pCtx->mDi);
        pCtx->mDiProcessNum = (di_flag == DE_INTERLACE_HW) ? 2 : 1;
        pCtx->mEnableDiFlag = 1;
    }
    else
    {
        omx_loge(" open deinterlace failed , we not to use deinterlace!");
        pCtx->mEnableDiFlag = 0;
    }

    omx_logd("**** initDeinterlace: enableFlag = %d, num = %d",
        (int)pCtx->mEnableDiFlag, (int)pCtx->mDiProcessNum);

    return 0;
}

static int anGetVideoFbmBufInfo(OmxAwDecoderContext *pCtx)
{
   int bEnablePostProcess = 1;
    OmxAcquireMutex(pCtx->awMutexHandler);
    FbmBufInfo* pFbmBufInfo =  GetVideoFbmBufInfo(pCtx->m_decoder);
    OmxReleaseMutex(pCtx->awMutexHandler);

    if(pFbmBufInfo != NULL)
    {
        omx_logd("video buffer info: nWidth[%d],nHeight[%d],"
            "nBufferCount[%d],ePixelFormat[%d]",
              pFbmBufInfo->nBufWidth,pFbmBufInfo->nBufHeight,
              pFbmBufInfo->nBufNum,pFbmBufInfo->ePixelFormat);
        omx_logd("video buffer info: nAlignValue[%d],"
             "bProgressiveFlag[%d],bIsSoftDecoderFlag[%d]",
              pFbmBufInfo->nAlignValue,pFbmBufInfo->bProgressiveFlag,
              pFbmBufInfo->bIsSoftDecoderFlag);
        pCtx->nActualColorFormat = (OMX_U32)pFbmBufInfo->ePixelFormat;
        //if(pCtx->mEnableDiFlag == -1 && pFbmBufInfo->bProgressiveFlag == 0 && pCtx->mDi != NULL)
	#ifdef CONF_DI_300_SUPPORT
	     if(pFbmBufInfo->nBufWidth >= 1920 || pFbmBufInfo->nBufHeight >= 1080)
		 {  
		 	bEnablePostProcess = 0;        
		 }
    #endif
		if(pCtx->mEnableDiFlag == -1 && pFbmBufInfo->bProgressiveFlag == 0 && pCtx->mDi != NULL && bEnablePostProcess)
        {
            initDeinterlace(pCtx);
        }

        if(pFbmBufInfo->nBufNum > (OMX_MAX_VIDEO_BUFFER_NUM - 2))
            pFbmBufInfo->nBufNum = OMX_MAX_VIDEO_BUFFER_NUM - 2;

        pCtx->mOutBufferNumDecoderNeed  = pFbmBufInfo->nBufNum;

        OMX_U32 nCurExtendFlag = 0;

        if(pFbmBufInfo->b10bitVideoFlag == 1)
            nCurExtendFlag |= AW_VIDEO_10BIT_FLAG;

        if(pFbmBufInfo->bAfbcModeFlag == 1)
            nCurExtendFlag |= AW_VIDEO_AFBC_FLAG;

        if(pFbmBufInfo->bHdrVideoFlag == 1)
            nCurExtendFlag |= AW_VIDEO_HDR_FLAG;


        omx_logd("**** nExtendFlag = %lx", nCurExtendFlag);
        OMX_PARAM_PORTDEFINITIONTYPE* inDef  = getPortDef(pCtx->pInPort);
        OMX_PARAM_PORTDEFINITIONTYPE* outDef = getPortDef(pCtx->pOutPort);

        OMX_BOOL bBufferNumMatchFlag = OMX_FALSE;
        //* if init-buffer-num is equal to or more than real-buffer-num,
        //* not callback the event "OMX_EventPortSettingsChanged" to avoid remalloc buffer
        //* which cost some time
        if(outDef->nBufferCountActual >= (OMX_U32)pFbmBufInfo->nBufNum)
            bBufferNumMatchFlag = OMX_TRUE;
        omx_logd("*** bBufferNumMatchFlag = %d",bBufferNumMatchFlag);
        OMX_BOOL bFormatMatchFlag = OMX_FALSE;
        if(pCtx->mVideoConfig.eOutputPixelFormat == pFbmBufInfo->ePixelFormat &&
            pFbmBufInfo->b10bitVideoFlag == 0)
        {
            bFormatMatchFlag = OMX_TRUE;
        }
        OMX_U32 nInitWidht  =  outDef->format.video.nFrameWidth;
        OMX_U32 nInitHeight =  outDef->format.video.nFrameHeight;
        if(bBufferNumMatchFlag && bFormatMatchFlag
            && nInitWidht  == (OMX_U32)pFbmBufInfo->nBufWidth
            && nInitHeight == (OMX_U32)pFbmBufInfo->nBufHeight
            && nCurExtendFlag == pCtx->nExtendFlag)
        {
            omx_logd("buffer match!!!!");
            return 0;
        }
        omx_logd("************** port setting change ************");
        omx_logd("****  buffer num: %lu, %d; w&h: %lu, %lu, %d, %d",
              outDef->nBufferCountActual,
              pFbmBufInfo->nBufNum,
              nInitWidht, nInitHeight,
              pFbmBufInfo->nBufWidth,
              pFbmBufInfo->nBufHeight);

        if(inDef->format.video.nFrameWidth   == WIDTH_DEFAULT ||
            inDef->format.video.nFrameHeight == HEIGHT_DEFAULT)
        {
            inDef->format.video.nFrameWidth  = pFbmBufInfo->nBufWidth;
            inDef->format.video.nFrameHeight = pFbmBufInfo->nBufHeight;
        }

        //*ACodec will add the mExtraOutBufferNum to nBufferCountActual,
        //*so we decrease it here
        //*if here not - pCtx->mExtraOutBufferNum, it will keep in omx

        OMX_U32 num = pFbmBufInfo->nBufNum - pCtx->mExtraOutBufferNum;
        outDef->nBufferCountMin    = num;
        outDef->nBufferCountActual = num;
        outDef->format.video.nFrameWidth  = pFbmBufInfo->nBufWidth;
        outDef->format.video.nFrameHeight = pFbmBufInfo->nBufHeight;
        outDef->nBufferSize = pFbmBufInfo->nBufWidth*pFbmBufInfo->nBufHeight*3/2;
        pCtx->nExtendFlag = nCurExtendFlag;
        pCtx->mVideoConfig.eOutputPixelFormat = pFbmBufInfo->ePixelFormat;

        pCtx->mVideoRect.nLeft   = pFbmBufInfo->nLeftOffset;
        pCtx->mVideoRect.nTop    = pFbmBufInfo->nTopOffset;
        pCtx->mVideoRect.nWidth  = \
                        pFbmBufInfo->nRightOffset - pFbmBufInfo->nLeftOffset;
        pCtx->mVideoRect.nHeight = \
                        pFbmBufInfo->nBottomOffset - pFbmBufInfo->nTopOffset;
        pCtx->callback(pCtx->pUserData, AW_OMX_CB_NOTIFY_RECT, &(pCtx->mVideoRect));
        pCtx->callback(pCtx->pUserData, AW_OMX_CB_PORT_CHANGE, NULL);
    }
    else
    {
        omx_logv("pFbmBufInfo = NULL, m_decoder(%p)", pCtx->m_decoder);
        return -1;
    }

    return 0;
}

static void anReopenVideoEngine(OmxAwDecoderContext *pCtx)
{
    omx_logd("***ReopenVideoEngine!");

    if(pCtx->mStreamInfo.pCodecSpecificData != NULL)
    {
        free(pCtx->mStreamInfo.pCodecSpecificData);
        pCtx->mStreamInfo.pCodecSpecificData    = NULL;
        pCtx->mStreamInfo.nCodecSpecificDataLen = 0;
    }

    OmxAcquireMutex(pCtx->awMutexHandler);
    ReopenVideoEngine(pCtx->m_decoder, &pCtx->mVideoConfig, &(pCtx->mStreamInfo));
    OmxReleaseMutex(pCtx->awMutexHandler);
    pCtx->bHadGetVideoFbmBufInfoFlag = OMX_FALSE;
    return ;
}

static OMX_BOOL isToInitBuffer(OmxAwDecoderContext *pCtx,
                                 OMX_BUFFERHEADERTYPE* pOutBufHdr,
                                 OMX_U32* index)
{
    OMX_U32 i;
    OMX_BOOL ret = OMX_TRUE;
    for(i=0; i< OMX_MAX_VIDEO_BUFFER_NUM; i++)
    {
        OMX_BUFFERHEADERTYPE* tmpBufHdr = \
            pCtx->mOutputBufferInfoArr[i].pHeadBufInfo;
        if(pOutBufHdr == tmpBufHdr)
        {
            ret = OMX_FALSE;
            break;
        }
        if(tmpBufHdr == NULL)
        {
            break;
        }
    }
    //if i>= OMX_MAX_VIDEO_BUFFER_NUM ,
    //something is wrong or the OMX_MAX_VIDEO_BUFFER_NUM is not enough
    *index = i;
    return ret;
}

static int getVirAddrOfMetadataBuffer(OmxAwDecoderContext *pCtx,
                                buffer_handle_t buffer_handle,
                                ion_handle_abstract_t* pHandle_ion,
                                int* pMapfd,
                                int* pVirsize,
                                unsigned char** ppViraddress)
{
    ion_handle_abstract_t handle_ion = ION_NULL_VALUE;
    int nMapfd = -1;
    unsigned char* nViraddress = 0;
    int nVirsize = 0;

#if (defined(CONF_HIGH_DYNAMIC_RANGE_ENABLE) || defined(CONF_AFBC_ENABLE))
    int ret = 0;
    private_handle_t* hnd = (private_handle_t *)(buffer_handle);
    if(hnd != NULL)
    {
        //TODO: encapulate it.
        #ifdef CONF_SIMPLE_ION
        #else
        ret = ion_import(pCtx->mIonFd, hnd->metadata_fd, &handle_ion);
        if(ret < 0)
        {
            omx_loge("ion_import fail, maybe the buffer was free by display!");
            return -1;
        }
        nVirsize = hnd->ion_metadata_size;
        #endif
    }
    else
    {
        omx_logd("the hnd is wrong : hnd = %p", hnd);
        return -1;
    }
#ifdef CONF_SIMPLE_ION
    ret = CdcIonMmap(hnd->metadata_fd, hnd->ion_metadata_size, &nViraddress);
#else
    ret = ion_map(pCtx->mIonFd, handle_ion, nVirsize,
            PROT_READ | PROT_WRITE, MAP_SHARED, 0, &nViraddress, &nMapfd);
#endif
    if(ret < 0)
    {
        omx_loge("ion_map fail!");
#ifdef CONF_SIMPLE_ION
        CdcIonMunmap(hnd->ion_metadata_size, nViraddress);
#else
        if(nMapfd >= 0)
            close(nMapfd);
        ion_free(pCtx->mIonFd, handle_ion);
#endif
        *ppViraddress = 0;
        *pVirsize = 0;
        *pMapfd = 0;
        *pHandle_ion = 0;
        return -1;
    }
#else
    CEDARC_UNUSE(pCtx);
    CEDARC_UNUSE(buffer_handle);
    CEDARC_UNUSE(pHandle_ion);
    CEDARC_UNUSE(pMapfd);
    CEDARC_UNUSE(pVirsize);
    CEDARC_UNUSE(ppViraddress);
#endif

    *ppViraddress = nViraddress;
    *pVirsize = nVirsize;
    *pMapfd = nMapfd;
    *pHandle_ion = handle_ion;
    return 0;
}

static void setIonMetadataFlag(OmxAwDecoderContext *pCtx,
                                    buffer_handle_t buffer_handle)
{

#if (defined(CONF_HIGH_DYNAMIC_RANGE_ENABLE) || defined(CONF_AFBC_ENABLE))
    private_handle_t* hnd = (private_handle_t *)(buffer_handle);
    hnd->ion_metadata_flag = 0;

    if (pCtx->nExtendFlag & AW_VIDEO_HDR_FLAG)
    {
        hnd->ion_metadata_flag |= SUNXI_METADATA_FLAG_HDR_SATIC_METADATA;
    }

    if (pCtx->nExtendFlag & AW_VIDEO_AFBC_FLAG)
    {
        hnd->ion_metadata_flag |= SUNXI_METADATA_FLAG_AFBC_HEADER;
    }
#else
    CEDARC_UNUSE(pCtx);
    CEDARC_UNUSE(buffer_handle);
#endif
    return ;
}

//* this function just for 2D rotate case.
//* just init alignment buffer to black color.
//* when the display to 2D,the appearance of "Green edges" in ratation if not init.
//* if init the whole buffer, it would take too much time.
int initPartialGpuBuffer(OmxAwDecoderContext *pCtx, char* pDataBuf)
{
    OMX_PARAM_PORTDEFINITIONTYPE* outDef = getPortDef(pCtx->pOutPort);

    if(pCtx->nActualColorFormat == PIXEL_FORMAT_NV21)
    {
        //* Y
        int nRealHeight = outDef->format.video.nFrameHeight;
        int nInitHeight = pCtx->mGpuAlignStride;
        int nSkipLen = outDef->format.video.nStride*
            (nRealHeight - nInitHeight);
        int nCpyLenY = outDef->format.video.nStride*nInitHeight;
        memset(pDataBuf+nSkipLen, 0x10, nCpyLenY);

        //*UV
        nSkipLen += nCpyLenY;
        nSkipLen += (outDef->format.video.nStride)*
            (nRealHeight/2 - nInitHeight/2);
        int nCpyLenUV = (outDef->format.video.nStride)*(nInitHeight/2);
        memset(pDataBuf+nSkipLen, 0x80, nCpyLenUV);
    }
    else if(pCtx->nActualColorFormat == PIXEL_FORMAT_YV12)
    {
        //* Y
        int nRealHeight = outDef->format.video.nFrameHeight;
        int nInitHeight = pCtx->mGpuAlignStride;
        int nSkipLen = outDef->format.video.nStride*
            (nRealHeight - nInitHeight);
        int nCpyLenY = outDef->format.video.nStride*nInitHeight;
        memset(pDataBuf+nSkipLen, 0x10, nCpyLenY);

        //*U
        nSkipLen += nCpyLenY;
        nSkipLen += (outDef->format.video.nStride/2)*
            (nRealHeight/2 - nInitHeight/2);
        int nCpyLenU = (outDef->format.video.nStride/2)*
            (nInitHeight/2);
        memset(pDataBuf+nSkipLen, 0x80, nCpyLenU);

        //*V
        nSkipLen += nCpyLenU;
        nSkipLen += (outDef->format.video.nStride/2)*
            (nRealHeight/2 - nInitHeight/2);
        int nCpyLenV = (outDef->format.video.nStride/2)*
            (nInitHeight/2);
        memset(pDataBuf+nSkipLen, 0x80, nCpyLenV);
    }

    return 0;
}

static buffer_handle_t getBufferHandle(OmxAwDecoderContext *pCtx, OMX_BUFFERHEADERTYPE* pOutBufHdr)
{
    buffer_handle_t pBufferHandle = NULL;
#ifdef CONF_KITKAT_AND_NEWER
    if(pCtx->bStoreOutputMetaDataFlag)
    {
        VideoDecoderOutputMetaData *pMetaData
            = (VideoDecoderOutputMetaData*)pOutBufHdr->pBuffer;
        pBufferHandle = pMetaData->pHandle;
    }
    else
    {
        pBufferHandle = (buffer_handle_t)pOutBufHdr->pBuffer;
    }
#else
    pBufferHandle = (buffer_handle_t)pOutBufHdr->pBuffer;
#endif
    return pBufferHandle;
}

#if defined(CONF_MALI_GPU) || defined(CONF_IMG_GPU_USE_COMMON_STRUCT)
static int getBufFdFromMaliGpu(OmxAwDecoderContext *pCtx, buffer_handle_t pBufferHandle, int *nBufFd, ion_handle_abstract_t* handle_ion)
{
    private_handle_t* hnd = (private_handle_t *)(pBufferHandle);
    if(hnd == NULL)
    {
        omx_loge("the hnd is wrong : hnd = %p",hnd);
        return -1;
    }
#ifndef CONF_SIMPLE_ION
    ion_import(pCtx->mIonFd, hnd->share_fd, handle_ion);
    if(pCtx->mIonFd <= 0)
    {
        omx_logd("the ion fd is wrong : fd = %d",(int)pCtx->mIonFd);
        return -1;
    }
    *nBufFd = CdcIonMap(pCtx->mIonFd, (uintptr_t)(*handle_ion));
#else
    *nBufFd = hnd->share_fd;
#endif
    if(*nBufFd < 0)
    {
        omx_loge("get ion_buffer fd error\n");
        return -1;
    }
    return 0;
}
#endif

#if defined(CONF_IMG_GPU)
static int getBufFdFromImgGpu(OmxAwDecoderContext *pCtx, buffer_handle_t pBufferHandle, int *nBufFd, ion_handle_abstract_t* handle_ion)
{
    IMG_native_handle_t* hnd = (IMG_native_handle_t*)(pBufferHandle);
    if(hnd == NULL)
    {
        omx_loge("the hnd is wrong : hnd = %p",hnd);
        return -1;
    }
#ifndef CONF_SIMPLE_ION
    ion_import(pCtx->mIonFd, hnd->fd[0], handle_ion);
    if(pCtx->mIonFd <= 0)
    {
        omx_logd("the ion fd is wrong : fd = %d",(int)pCtx->mIonFd);
        return -1;
    }
    *nBufFd = CdcIonGetFd(pCtx->mIonFd, (uintptr_t)(*handle_ion));
#else
    *nBufFd = hnd->share_fd;
#endif
    if(*nBufFd < 0)
    {
        omx_loge("get ion_buffer fd error\n");
        return -1;
    }
    return 0;
}
#endif

static int getBufFd(OmxAwDecoderContext *pCtx, buffer_handle_t pBufferHandle, int *nBufFd, ion_handle_abstract_t* handle_ion)
{
#if defined(CONF_MALI_GPU) || defined(CONF_IMG_GPU_USE_COMMON_STRUCT)
    return getBufFdFromMaliGpu(pCtx, pBufferHandle, nBufFd, handle_ion);
#elif defined(CONF_IMG_GPU)
    return getBufFdFromImgGpu(pCtx, pBufferHandle, nBufFd, handle_ion);
#else
#error invalid GPU type config
#endif
}

/*
 *   use in Android platform,  through allocate buffer by GPU,
 *   which has private_handle_t, API use_buffer
 */
static int initBufferInfo(OmxAwDecoderContext *pCtx,
                                        OMX_BUFFERHEADERTYPE*   pOutBufHdr)
{
    OMX_U32 i;
    void* dst = NULL;
    ion_handle_abstract_t handle_ion    = ION_NULL_VALUE;
    uintptr_t             nPhyaddress   = -1;
    uintptr_t             nTEEaddress   = -1;
    buffer_handle_t       pBufferHandle = NULL;
    int nBufFd = -1;
    pBufferHandle = getBufferHandle(pCtx, pOutBufHdr);
    if(getBufFd(pCtx, pBufferHandle, &nBufFd, &handle_ion) < -1)
        return -1;

    for(i = 0; i<getPortAllocSize(pCtx->pOutPort); i++)
    {
        if(pCtx->mOutputBufferInfoArr[i].pHeadBufInfo == NULL)
            break;
    }
    if(!pCtx->bIsSecureVideoFlag)
    {
        CdcIonMmap(nBufFd,pOutBufHdr->nAllocLen,(unsigned char **)&dst);
        if(pCtx->mStreamInfo.eCodecFormat == VIDEO_CODEC_FORMAT_H265)
            initPartialGpuBuffer(pCtx,(char*)dst);
    }
    ion_handle_abstract_t metaData_handle_ion = ION_NULL_VALUE;
    int mMetaDataMapFd = -1;
    int mMetaDataVirSize = 0;
    unsigned char* pMetaDataVirAddr = NULL;

    getVirAddrOfMetadataBuffer(pCtx, pBufferHandle, &metaData_handle_ion,
                               &mMetaDataMapFd, & mMetaDataVirSize,
                               &pMetaDataVirAddr);

    setIonMetadataFlag(pCtx, pBufferHandle);
    //CdcMemGetVirByFd(pCtx->decMemOps, nBufFd, &nTEEaddress); //for debug
    //CdcMemGetPhyByFd(pCtx->decMemOps, nBufFd, &nPhyaddress); //for debug
    pCtx->mOutputBufferInfoArr[i].nVirAddrSize = pOutBufHdr->nAllocLen;
    pCtx->mOutputBufferInfoArr[i].handle_ion   = handle_ion;
    pCtx->mOutputBufferInfoArr[i].nBufFd       = nBufFd;
    if(!pCtx->bIsSecureVideoFlag)
        pCtx->mOutputBufferInfoArr[i].pBufVirAddr      = (char*)dst;
    else
        pCtx->mOutputBufferInfoArr[i].pBufVirAddr      = (char*)nTEEaddress;//not use
    pCtx->mOutputBufferInfoArr[i].pBufPhyAddr          = (char*)nPhyaddress;//not use
    pCtx->mOutputBufferInfoArr[i].pHeadBufInfo         = pOutBufHdr;
    pCtx->mOutputBufferInfoArr[i].pMetaDataVirAddr     = pMetaDataVirAddr;
    pCtx->mOutputBufferInfoArr[i].nMetaDataVirAddrSize = mMetaDataVirSize;
    pCtx->mOutputBufferInfoArr[i].nMetaDataMapFd       = mMetaDataMapFd;
    pCtx->mOutputBufferInfoArr[i].metadata_handle_ion  = metaData_handle_ion;
    return 0;
}

static int requestOutputBuffer(OmxAwDecoderContext *pCtx,
                                    VideoPicture* pPicBufInfo,
                                    OMX_BUFFERHEADERTYPE*   pOutBufHdr,
                                    OMX_BOOL bInitBufferFlag)
{
    OMX_U32 i;
    int mYsize;
    if(pOutBufHdr == NULL)
    {
        omx_loge(" the pOutBufHdr is null when request OutPut Buffer");
        return -1;
    }

    //* init the buffer
    if(bInitBufferFlag)
    {
        int ret = initBufferInfo(pCtx, pOutBufHdr);
        if(ret == -1)
            return ret;
    }
    OMX_U32 num = getPortAllocSize(pCtx->pOutPort);
    for(i = 0;i<num ;i++)
    {
        if(pCtx->mOutputBufferInfoArr[i].pHeadBufInfo == pOutBufHdr)
            break;
    }

    OMX_ASSERT(i!=num);

    OMX_PARAM_PORTDEFINITIONTYPE* outDef = getPortDef(pCtx->pOutPort);

    //* set the buffer address
    mYsize = outDef->format.video.nFrameWidth * outDef->format.video.nFrameHeight;
    pPicBufInfo->pData0      = pCtx->mOutputBufferInfoArr[i].pBufVirAddr;
    pPicBufInfo->phyYBufAddr = \
        (uintptr_t)pCtx->mOutputBufferInfoArr[i].pBufPhyAddr;

    if(pCtx->nActualColorFormat == PIXEL_FORMAT_P010_UV ||
       pCtx->nActualColorFormat == PIXEL_FORMAT_P010_VU)
    {
        pPicBufInfo->pData1       = pPicBufInfo->pData0 + mYsize*2;
        pPicBufInfo->pData2       = pPicBufInfo->pData1 + mYsize/2;
        pPicBufInfo->phyCBufAddr  = pPicBufInfo->phyYBufAddr + mYsize*2;
    }
    else
    {
        pPicBufInfo->pData1      = pPicBufInfo->pData0 + mYsize;
        pPicBufInfo->phyCBufAddr = pPicBufInfo->phyYBufAddr + mYsize;
    }

    pPicBufInfo->nBufId      = i;
    pPicBufInfo->pPrivate    = \
        (void*)(uintptr_t)pCtx->mOutputBufferInfoArr[i].handle_ion;
    pPicBufInfo->nBufFd      =  pCtx->mOutputBufferInfoArr[i].nBufFd;
    pPicBufInfo->nWidth      =  outDef->format.video.nFrameWidth;
    pPicBufInfo->nHeight     =  outDef->format.video.nFrameHeight;
    pPicBufInfo->nLineStride =  outDef->format.video.nFrameWidth;
    pPicBufInfo->pMetaData   =  pCtx->mOutputBufferInfoArr[i].pMetaDataVirAddr;

    pCtx->mOutputBufferInfoArr[i].mStatus = OWNED_BY_DECODER;

    return 0;
}

static int returnInitBuffer(OmxAwDecoderContext *pCtx,
                            OMX_BUFFERHEADERTYPE* pOutBufHdr,
                            OMX_U32 index)
{
    VideoPicture mVideoPicture;
    VideoPicture* pVideoPicture = NULL;

    // the buffer is used to deinterlace, so it cannot be used by decoder
    OMX_S32 nAsDiOutBufferFlag = 0;
    // the flag used to ReturnRelasePicture to decoder.
    // if is 1, decoder cannot use this buffer,
    // because it is reserved for di
    OMX_S32 nForbiddenUseFlag = 0;
    memset(&mVideoPicture, 0, sizeof(VideoPicture));

    if(pCtx->mDi && pCtx->mEnableDiFlag>0 &&
        pCtx->mDiOutBufferInfo.nValidSize < DI_BUFFER_NUM_KEEP_IN_OMX)
    {
        nAsDiOutBufferFlag = 1;
        nForbiddenUseFlag = 1;
    }
    omx_logv("nAsDiOutBufferFlag: %d, pCtx->mDiOutBufferInfo.nValidSize: %d",
        (int)nAsDiOutBufferFlag, (int)pCtx->mDiOutBufferInfo.nValidSize);

    if(pCtx->mSetToDecoderBufferNum >= pCtx->mOutBufferNumDecoderNeed)
    {
        omx_loge("the buffer number of ACodec is not equal to FBM buffer number,"
            "please check it");
        //abort();
        pCtx->mOutputBufferInfoArr[index].pHeadBufInfo = pOutBufHdr;
        pCtx->mOutputBufferInfoArr[index].mStatus      = OWNED_BY_US;
#if ENABLE_SHOW_BUFINFO_STATUS
        omx_logd("returnInitBuffer. showBufferInfoArrStatus");
        showBufferInfoArrStatus(pCtx);
#endif
        return 0;
    }
    else
    {
        int mRet = requestOutputBuffer(pCtx, &mVideoPicture, pOutBufHdr, OMX_TRUE);
        if(mRet == 0)
        {
            pCtx->mSetToDecoderBufferNum++;
            // in seek case
            omx_logv("pCtx->mNeedReSetToDecoderBufferNum: %lu", pCtx->mNeedReSetToDecoderBufferNum);
            if(pCtx->mNeedReSetToDecoderBufferNum > 0)
            {
                if(RequestReleasePicture(pCtx->m_decoder) != NULL)
                {
                    pCtx->mNeedReSetToDecoderBufferNum--;
                    pVideoPicture = ReturnReleasePicture(pCtx->m_decoder,
                                                        &mVideoPicture, nForbiddenUseFlag);
                    if(nAsDiOutBufferFlag == 1)
                    {
                        pCtx->mOutputBufferInfoArr[index].mStatus = OWNED_BY_US;
                    }
                }
                else
                {
                    omx_logw("** can not reqeust release picture");
                    return -1;
                }
            }
            else // start case:
            {
                pVideoPicture = SetVideoFbmBufAddress(pCtx->m_decoder,
                                                      &mVideoPicture,
                                                      nForbiddenUseFlag);
                omx_logd("*** call SetVideoFbmBufAddress: pVideoPicture(%p), bufFd: %d",
                    pVideoPicture, pVideoPicture->nBufFd);

                if(nAsDiOutBufferFlag == 1)
                {
                    pCtx->mOutputBufferInfoArr[index].mStatus = OWNED_BY_US;
                }
            }

            if(nAsDiOutBufferFlag == 1)
            {
                int i;
                for(i=0; i<DI_BUFFER_NUM_KEEP_IN_OMX; i++)
                {
                    if(pCtx->mDiOutBufferInfo.pArrPic[i] == NULL)
                        break;
                }

                if(i<DI_BUFFER_NUM_KEEP_IN_OMX)
                {
                    pCtx->mDiOutBufferInfo.pArrPic[i] = pVideoPicture;
                    pCtx->mDiOutBufferInfo.nValidSize ++;
                }
                else
                {
                    omx_loge("** the mDiOutBufferInfo is full **");
                    abort();
                }
            }

            OmxTryPostSem(pCtx->mSemOutBuffer);
            pCtx->mOutputBufferInfoArr[mVideoPicture.nBufId].pVideoPicture = pVideoPicture;

#if ENABLE_SHOW_BUFINFO_STATUS
            omx_logd("returnInitBuffer. showBufferInfoArrStatus");

            showBufferInfoArrStatus(pCtx);
#endif
            return 0;
        }
        else
            return mRet;
    }
}

static int returnArrangedBuffer(OmxAwDecoderContext *pCtx,
                                OMX_BUFFERHEADERTYPE* pOutBufHdr,
                                OMX_U32 index)
{
    VideoPicture mVideoPicture;
    VideoPicture* pVideoPicture = NULL;
    memset(&mVideoPicture, 0, sizeof(VideoPicture));

    int mRet = requestOutputBuffer(pCtx, &mVideoPicture, pOutBufHdr, OMX_FALSE);
    if(mRet == 0)
    {
        //* we should not return to decoder again if it had been used to set eos to render
        if(pCtx->mOutputBufferInfoArr[index].mUseDecoderBufferToSetEosFlag)
        {
            pCtx->mOutputBufferInfoArr[index].mUseDecoderBufferToSetEosFlag = OMX_FALSE;
            return 0;
        }
        pVideoPicture = pCtx->mOutputBufferInfoArr[index].pVideoPicture;

        // if the di buffer is not enough, the buffer should be used by di;
        if(pCtx->mDi && pCtx->mEnableDiFlag>0 &&
            pCtx->mDiOutBufferInfo.nValidSize < DI_BUFFER_NUM_KEEP_IN_OMX)
        {
            int i;
            for(i=0; i<DI_BUFFER_NUM_KEEP_IN_OMX; i++)
            {
                if(pCtx->mDiOutBufferInfo.pArrPic[i] == NULL)
                    break;
            }

            if(i<DI_BUFFER_NUM_KEEP_IN_OMX)
            {
                pCtx->mDiOutBufferInfo.pArrPic[i] = pVideoPicture;
                pCtx->mDiOutBufferInfo.nValidSize ++;
            }
            else
            {
                omx_loge("** the mDiOutBufferInfo is full **");
                abort();
            }
            pCtx->mOutputBufferInfoArr[index].mStatus = OWNED_BY_US;
        }
        else
        {
            ReturnPicture(pCtx->m_decoder, pVideoPicture);
            setOutBufferStatus(pCtx, pVideoPicture, OWNED_BY_DECODER);
        }
        OmxTryPostSem(pCtx->mSemOutBuffer);
    }
#if ENABLE_SHOW_BUFINFO_STATUS
    omx_logd("returnArrangedBuffer. showBufferInfoArrStatus");
    showBufferInfoArrStatus(pCtx);
#endif
    return mRet;
 }

static int returnBufferToDecdoer(OmxAwDecoderContext *pCtx)
{
    OMX_U32 index = 0xff;
    OMX_BOOL flag = OMX_FALSE;
    OMX_BUFFERHEADERTYPE*  pOutBufHdr = NULL;
    pOutBufHdr = doRequestPortBuffer(pCtx->pOutPort);

    if(pOutBufHdr != NULL)
    {
        flag = isToInitBuffer(pCtx, pOutBufHdr, &index);
        if(flag == OMX_TRUE)
        {
            return returnInitBuffer(pCtx, pOutBufHdr, index);
        }
        else
        {
            return returnArrangedBuffer(pCtx, pOutBufHdr, index);
        }
    }
    else
        return -1;
}

static int anCheckResolutionChange(OmxAwDecoderContext *pCtx, VideoPicture* picture)
{
    int nPicRealWidth  = picture->nRightOffset  - picture->nLeftOffset;
    int nPicRealHeight = picture->nBottomOffset - picture->nTopOffset;

    //* if the offset is not right, we should not use them to compute width and height
    if(nPicRealWidth <= 0 || nPicRealHeight <= 0)
    {
        nPicRealWidth  = picture->nWidth;
        nPicRealHeight = picture->nHeight;
    }

    int width_align  = picture->nWidth;
    int height_align = picture->nHeight;

    OMX_PARAM_PORTDEFINITIONTYPE* inDef  = getPortDef(pCtx->pInPort);
    OMX_PARAM_PORTDEFINITIONTYPE* outDef = getPortDef(pCtx->pOutPort);

    if(inDef->format.video.nFrameWidth == WIDTH_DEFAULT ||
       inDef->format.video.nFrameHeight == HEIGHT_DEFAULT)
    {
        inDef->format.video.nFrameHeight = nPicRealHeight;
        inDef->format.video.nFrameWidth  = nPicRealWidth;
    }

    pCtx->mVideoRect.nLeft   = picture->nLeftOffset;
    pCtx->mVideoRect.nTop    = picture->nTopOffset;
    pCtx->mVideoRect.nWidth  = nPicRealWidth;
    pCtx->mVideoRect.nHeight = nPicRealHeight;

    if(pCtx->bUseAndroidBuffer)
    {
        if((OMX_U32)nPicRealWidth  != outDef->format.video.nFrameWidth ||
           (OMX_U32)nPicRealHeight != outDef->format.video.nFrameHeight)
        {
            omx_logw("bUseAndroidBuffer: video picture size changed: "
                "org_height = %lu, org_width = %lu,"
                "new_height = %d, new_width = %d.",
                outDef->format.video.nFrameHeight,
                outDef->format.video.nFrameWidth,
                nPicRealHeight, nPicRealWidth);

            outDef->format.video.nFrameHeight = nPicRealHeight;
            outDef->format.video.nFrameWidth  = nPicRealWidth;
            outDef->nBufferSize = nPicRealWidth * nPicRealHeight * 3 / 2;
            pCtx->callback(pCtx->pUserData, AW_OMX_CB_NOTIFY_RECT, &(pCtx->mVideoRect));
            pCtx->callback(pCtx->pUserData, AW_OMX_CB_PORT_CHANGE, NULL);
            return 0;
         }
    }
    else
    {
        if((OMX_U32)nPicRealWidth  != outDef->format.video.nFrameWidth ||
           (OMX_U32)nPicRealHeight != outDef->format.video.nFrameHeight)
        {
            omx_logw("not bUseAndroidBuffer: video picture size changed: "
                "org_height = %lu, org_width = %lu,"
                "new_height = %d, new_width = %d.",
                outDef->format.video.nFrameHeight,
                outDef->format.video.nFrameWidth,
                nPicRealHeight, nPicRealWidth);
            OMX_U32 nAlignValue = pCtx->mGpuAlignStride - 1;
            if(((height_align + nAlignValue) & ~nAlignValue) == outDef->format.video.nFrameHeight
                    && ((width_align + nAlignValue) & ~nAlignValue) == outDef->format.video.nFrameWidth)
            {
                omx_logd("not bUseAndroidBuffer: %lu bit aligned width and height"
                        " equal. Not resolution_change.", pCtx->mGpuAlignStride);
                return -1;
            }
            outDef->format.video.nFrameHeight = nPicRealHeight;
            outDef->format.video.nFrameWidth  = nPicRealWidth;
            outDef->nBufferSize = nPicRealHeight*nPicRealWidth *3/2;
            pCtx->callback(pCtx->pUserData, AW_OMX_CB_NOTIFY_RECT, &(pCtx->mVideoRect));
            pCtx->callback(pCtx->pUserData, AW_OMX_CB_PORT_CHANGE, NULL);
            return 0;
        }
    }
    return -1;
}

static void copyPictureDataToAndroidBuffer(OmxAwDecoderContext *pCtx,
                                                    VideoPicture* picture,
                                                    OMX_BUFFERHEADERTYPE* pOutBufHdr)
{
    TranBufferInfo        mTranBufferInfo;
    memset(&mTranBufferInfo, 0 ,sizeof(TranBufferInfo));
    OMX_PARAM_PORTDEFINITIONTYPE* outDef = getPortDef(pCtx->pOutPort);
    void* dst;
    buffer_handle_t         pBufferHandle = NULL;
    android::GraphicBufferMapper &mapper = android::GraphicBufferMapper::get();
    //*when lock gui buffer, we must according gui buffer's width and height, not by decoder!
    android::Rect bounds(outDef->format.video.nFrameWidth,
                         outDef->format.video.nFrameHeight);

#ifdef CONF_KITKAT_AND_NEWER
    if(pCtx->bStoreOutputMetaDataFlag ==OMX_TRUE)
    {
        VideoDecoderOutputMetaData *pMetaData =
        (VideoDecoderOutputMetaData*)pOutBufHdr->pBuffer;
        pBufferHandle = pMetaData->pHandle;
    }
    else
    {
        dst = pOutBufHdr->pBuffer;
    }
#else
    pBufferHandle = (buffer_handle_t)pOutBufHdr->pBuffer;
#endif
    if(pBufferHandle && 0 != mapper.lock(pBufferHandle, GRALLOC_USAGE_SW_WRITE_OFTEN, bounds, &dst))
    {
        omx_logw("Lock GUIBuf fail. pBufferHandle [%p]!", pBufferHandle);
    }
    if(!dst)
    {
        omx_loge("the buffer dst is NULL. we would NOT copy picture to buffer.");
    }
    else
    {
        int nPicRealWidth = picture->nRightOffset - picture->nLeftOffset;
        int nPicRealHeight = picture->nBottomOffset - picture->nTopOffset;
        omx_logv("the dst = %p, dst buffer length=%d,",dst,pOutBufHdr->nAllocLen);
        memcpy(dst,picture->pData0,nPicRealWidth*nPicRealHeight*3/2);
    }

    if(pBufferHandle && 0 != mapper.unlock(pBufferHandle))
    {
        omx_logw("Unlock GUIBuf fail!");
    }
}

static void TransformYV12ToYUV420(VideoPicture* pPicture, TranBufferInfo* pTranBufferInfo)
{
    int i;
    int nPicRealWidth;
    int nPicRealHeight;
    int nSrcBufWidth;
    int nSrcBufHeight;
    int nDstBufWidth;
    int nDstBufHeight;
    int nCopyDataWidth;
    int nCopyDataHeight;
    unsigned char* dstPtr;
    unsigned char* srcPtr;
    dstPtr      = (unsigned char*)pTranBufferInfo->pAddr;
    srcPtr      = (unsigned char*)pPicture->pData0;

    nPicRealWidth  = pPicture->nRightOffset - pPicture->nLeftOffset;
    nPicRealHeight = pPicture->nBottomOffset - pPicture->nTopOffset;

    //* if the uOffset is not right, we should not use them to compute width and height
    if(nPicRealWidth <= 0 || nPicRealHeight <= 0)
    {
        nPicRealWidth  = pPicture->nWidth;
        nPicRealHeight = pPicture->nHeight;
    }

    nSrcBufWidth  = (pPicture->nWidth + 15) & ~15;
    nSrcBufHeight = (pPicture->nHeight + 15) & ~15;

    //* On chip-1673, the gpu is 32 align ,but here is not copy to gpu, so also 16 align.
    //* On other chip, gpu buffer is 16 align.
    //nDstBufWidth = (pTranBufferInfo->nWidth+ 15)&~15;
    nDstBufWidth  = pTranBufferInfo->nWidth;

    nDstBufHeight = pTranBufferInfo->nHeight;

    nCopyDataWidth  = nPicRealWidth;
    nCopyDataHeight = nPicRealHeight;

    omx_logd("nPicRealWidth & H = %d, %d,"
          "nSrcBufWidth & H = %d, %d,"
          "nDstBufWidth & H = %d, %d,"
          "nCopyDataWidth & H = %d, %d",
          nPicRealWidth,nPicRealHeight,
          nSrcBufWidth,nSrcBufHeight,
          nDstBufWidth,nDstBufHeight,
          nCopyDataWidth,nCopyDataHeight);

    //*copy y
    for(i=0; i < nCopyDataHeight; i++)
    {
        memcpy(dstPtr, srcPtr, nCopyDataWidth);
        dstPtr += nDstBufWidth;
        srcPtr += nSrcBufWidth;
    }

    //*copy u
    srcPtr = ((unsigned char*)pPicture->pData0) + nSrcBufWidth*nSrcBufHeight*5/4;
    nCopyDataWidth  = (nCopyDataWidth+1)/2;
    nCopyDataHeight = (nCopyDataHeight+1)/2;
    for(i=0; i < nCopyDataHeight; i++)
    {
        memcpy(dstPtr, srcPtr, nCopyDataWidth);
        dstPtr += nDstBufWidth/2;
        srcPtr += nSrcBufWidth/2;
    }

    //*copy v
    srcPtr = ((unsigned char*)pPicture->pData0) + nSrcBufWidth*nSrcBufHeight;
    for(i=0; i<nCopyDataHeight; i++)
    {
        memcpy(dstPtr, srcPtr, nCopyDataWidth);
        dstPtr += nDstBufWidth/2;    //a31 gpu, uv is half of y
        srcPtr += nSrcBufWidth/2;
    }

    return;
}

static void TransformNV21ToYUV420(VideoPicture* pPicture, TranBufferInfo* pTranBufferInfo)
{
    unsigned char* pDst;
    unsigned char* pSrc;
    pDst = (unsigned char*)pTranBufferInfo->pAddr;
    pSrc = (unsigned char*)pPicture->pData0;

    memcpy(pDst, pSrc, pTranBufferInfo->nWidth*pTranBufferInfo->nHeight);

    int y_dst_size = pTranBufferInfo->nWidth*pTranBufferInfo->nHeight;
    int u_dst_size = y_dst_size * 1 / 4;
    int dstPointer = y_dst_size;
    int y_src_size = pPicture->nWidth*pPicture->nHeight;
    int srcPointer = y_src_size;
    int count = u_dst_size;

    while (count > 0)
    {
        srcPointer++;
        pDst[dstPointer] = pSrc[srcPointer];
        dstPointer++;
        srcPointer++;
        count--;
    }

    srcPointer = y_src_size;

    count = u_dst_size;
    while (count > 0)
    {
        pDst[dstPointer] = pSrc[srcPointer];
        dstPointer++;
        srcPointer += 2;
        count--;
    }
   return;
}
#ifdef CONF_PIE_AND_NEWER
static OMX_BOOL isColorAspectsDiffer(const ColorAspects a, const ColorAspects b) {
    if (a.mRange != b.mRange
        || a.mPrimaries != b.mPrimaries
        || a.mTransfer != b.mTransfer
        || a.mMatrixCoeffs != b.mMatrixCoeffs) {
        return OMX_TRUE;
    }
    return OMX_FALSE;
}

static OMX_BOOL updateColorAspects(OmxAwDecoderContext *pCtx,
                                     const ColorAspects aspects) {
    ColorAspects newAspects;
    CEDARC_UNUSE(newAspects);
#if UPDATE_COLOR_ASPECTS_WITH_BITSTREAM
   //for tengxun app
    pCtx->mDefaultColorAspects = aspects;

#else
    newAspects.mRange = aspects.mRange != ColorAspects::RangeUnspecified ?
        aspects.mRange : pCtx->mDefaultColorAspects.mRange;
    newAspects.mPrimaries = aspects.mPrimaries != ColorAspects::PrimariesUnspecified ?
        aspects.mPrimaries : pCtx->mDefaultColorAspects.mPrimaries;
    newAspects.mTransfer = aspects.mTransfer != ColorAspects::TransferUnspecified ?
        aspects.mTransfer : pCtx->mDefaultColorAspects.mTransfer;
    newAspects.mMatrixCoeffs = aspects.mMatrixCoeffs != ColorAspects::MatrixUnspecified ?
        aspects.mMatrixCoeffs : pCtx->mDefaultColorAspects.mMatrixCoeffs;

    pCtx->mDefaultColorAspects = newAspects;
 #endif
     if(isColorAspectsDiffer(pCtx->mFinalColorAspects, pCtx->mDefaultColorAspects))
     {
         pCtx->mFinalColorAspects = pCtx->mDefaultColorAspects;
         return OMX_TRUE;
     }
     return OMX_FALSE;
}

static void adapteColorAspects(ColorAspects* aspects,
                                     int32_t mTransferCharacteristics,
                                     int32_t mMatrixCoeffs,
                                     int32_t mVideoFullRange,
                                     int32_t mPrimaries)
{
    uint32_t space = HAL_DATASPACE_UNKNOWN;

    switch (mTransferCharacteristics)
    {
        case VIDEO_TRANSFER_GAMMA2_2:
            aspects->mTransfer = ColorAspects::TransferGamma22;//*4
            space |= HAL_DATASPACE_TRANSFER_GAMMA2_2;
            break;
        case VIDEO_TRANSFER_GAMMA2_8:
            aspects->mTransfer = ColorAspects::TransferGamma28;//*5
            space |= HAL_DATASPACE_TRANSFER_GAMMA2_8;
            break;
        case VIDEO_TRANSFER_SMPTE_170M:
            aspects->mTransfer = ColorAspects::TransferSMPTE170M;//*3
            space |= HAL_DATASPACE_TRANSFER_SMPTE_170M;
            break;
        case VIDEO_TRANSFER_SMPTE_240M:
            aspects->mTransfer = ColorAspects::TransferSMPTE240M;//*0x40
            space |= HAL_DATASPACE_TRANSFER_UNSPECIFIED;
            break;
        case VIDEO_TRANSFER_LINEAR:
            aspects->mTransfer = ColorAspects::TransferLinear;//*1
            space |= HAL_DATASPACE_TRANSFER_LINEAR;
            break;
        case VIDEO_TRANSFER_BT1361:
            aspects->mTransfer = ColorAspects::TransferBT1361;//*0x42
            space |= HAL_DATASPACE_TRANSFER_UNSPECIFIED;
            break;
        case VIDEO_TRANSFER_SRGB:
            aspects->mTransfer = ColorAspects::TransferSRGB;//*2
            space |= HAL_DATASPACE_TRANSFER_SRGB;
            break;
        case VIDEO_TRANSFER_ST2084:
            aspects->mTransfer = ColorAspects::TransferST2084;//*6
            space |= HAL_DATASPACE_TRANSFER_ST2084;
            break;
        case VIDEO_TRANSFER_ST428_1:
            aspects->mTransfer = ColorAspects::TransferST428;//*0x43
            space |= HAL_DATASPACE_TRANSFER_UNSPECIFIED;
            break;
        case VIDEO_TRANSFER_HLG:
            aspects->mTransfer = ColorAspects::TransferHLG;//*7
            space |= HAL_DATASPACE_TRANSFER_HLG;
            break;
        case VIDEO_TRANSFER_IEC61966:
            aspects->mTransfer = ColorAspects::TransferXvYCC;//*0x41
            space |= HAL_DATASPACE_TRANSFER_UNSPECIFIED;
            break;
        case VIDEO_TRANSFER_LOGARITHMIC_0:
        case VIDEO_TRANSFER_LOGARITHMIC_1:
        case VIDEO_TRANSFER_BT1361_EXTENDED:
        case VIDEO_TRANSFER_BT2020_0:
        case VIDEO_TRANSFER_BT2020_1:
            aspects->mTransfer = ColorAspects::TransferOther;//*0xff
            space |= HAL_DATASPACE_TRANSFER_UNSPECIFIED;
            break;
        case VIDEO_TRANSFER_RESERVED_0:
        case VIDEO_TRANSFER_UNSPECIFIED:
        case VIDEO_TRANSFER_RESERVED_1:
        default:
            aspects->mTransfer = ColorAspects::TransferUnspecified;//*0
            space |= HAL_DATASPACE_TRANSFER_UNSPECIFIED;
            break;
    }
    switch (mMatrixCoeffs)
    {
        case VIDEO_MATRIX_COEFFS_BT709:
            aspects->mMatrixCoeffs = ColorAspects::MatrixBT709_5;//*1
            space |= HAL_DATASPACE_STANDARD_BT709;
            break;
        case VIDEO_MATRIX_COEFFS_BT470M:
            aspects->mMatrixCoeffs = ColorAspects::MatrixBT470_6M;//*2
            space |= HAL_DATASPACE_STANDARD_BT470M;
            break;
        case VIDEO_MATRIX_COEFFS_BT601_625_0:
        case VIDEO_MATRIX_COEFFS_BT601_625_1:
            aspects->mMatrixCoeffs = ColorAspects::MatrixBT601_6;//*3
            space |= HAL_DATASPACE_BT601_625;
            //space |= HAL_DATASPACE_BT601_525;
            break;
        case VIDEO_MATRIX_COEFFS_SMPTE_240M:
            aspects->mMatrixCoeffs = ColorAspects::MatrixSMPTE240M;//*4
            space |= HAL_DATASPACE_STANDARD_UNSPECIFIED;
            break;
        case VIDEO_MATRIX_COEFFS_BT2020:
            aspects->mMatrixCoeffs = ColorAspects::MatrixBT2020;//*5
            space |= HAL_DATASPACE_STANDARD_BT2020;
            break;
        case VIDEO_MATRIX_COEFFS_BT2020_CONSTANT_LUMINANCE:
            aspects->mMatrixCoeffs = ColorAspects::MatrixBT2020Constant;//*6
            space |= HAL_DATASPACE_STANDARD_BT2020_CONSTANT_LUMINANCE;
            break;
        case VIDEO_MATRIX_COEFFS_SOMPATE:
        case VIDEO_MATRIX_COEFFS_CD_NON_CONSTANT_LUMINANCE:
        case VIDEO_MATRIX_COEFFS_CD_CONSTANT_LUMINANCE:
        case VIDEO_MATRIX_COEFFS_BTICC:
        case VIDEO_MATRIX_COEFFS_YCGCO:
            aspects->mMatrixCoeffs = ColorAspects::MatrixOther;//*0xff
            space |= HAL_DATASPACE_STANDARD_UNSPECIFIED;
            break;
        case VIDEO_MATRIX_COEFFS_UNSPECIFIED_0:
        case VIDEO_MATRIX_COEFFS_RESERVED_0:
        case VIDEO_MATRIX_COEFFS_IDENTITY:
        default:
            space |= HAL_DATASPACE_STANDARD_UNSPECIFIED;
            aspects->mMatrixCoeffs = ColorAspects::MatrixUnspecified;//*0
            break;
    }
    switch (mVideoFullRange)
    {
        case VIDEO_FULL_RANGE_LIMITED:
            aspects->mRange = ColorAspects::RangeLimited;//*2
            space |= HAL_DATASPACE_RANGE_LIMITED;
            break;
        case VIDEO_FULL_RANGE_FULL:
            aspects->mRange = ColorAspects::RangeFull;//*1
            space |= HAL_DATASPACE_RANGE_FULL;
            break;
        default:
        {
            aspects->mRange = ColorAspects::RangeUnspecified;//*0
            omx_loge("should not be here, mVideoFullRange = %d", mVideoFullRange);
            break;
        }
    }
    switch(mPrimaries)//from ffmpeg
    {
        case 1:
            aspects->mPrimaries = ColorAspects::PrimariesBT709_5;//*1
            break;
        case 4:
            aspects->mPrimaries = ColorAspects::PrimariesBT470_6M;//*2
            break;
        case 5:
            aspects->mPrimaries = ColorAspects::PrimariesBT601_6_625;//*3
            break;
        case 6:
            aspects->mPrimaries = ColorAspects::PrimariesBT601_6_525;//*4
            break;
        case 7:
            aspects->mPrimaries = ColorAspects::PrimariesOther;//*0xff
            break;
        case 8:
            aspects->mPrimaries = ColorAspects::PrimariesGenericFilm;//*5
            break;
        case 9:
            aspects->mPrimaries = ColorAspects::PrimariesBT2020;//*6
            break;
        case 0:
        case 3:
        case 2:
        default:
        {
            aspects->mPrimaries = ColorAspects::PrimariesUnspecified;//*0
            break;
        }
    }
    return ;
}
#endif

static OMX_BUFFERHEADERTYPE* anDrainCopy(OmxAwDecoderContext *pCtx)
{
    VideoPicture*           pPicture     = NULL;
    OMX_BUFFERHEADERTYPE*   pOutBufHdr  = NULL;
    TranBufferInfo          mTranBufferInfo;
    memset(&mTranBufferInfo, 0 ,sizeof(TranBufferInfo));

    OmxAcquireMutex(pCtx->awMutexHandler);
    pPicture = NextPictureInfo(pCtx->m_decoder,0);
    OmxReleaseMutex(pCtx->awMutexHandler);
    if(pPicture == NULL)
        return NULL;

    if(anCheckResolutionChange(pCtx, pPicture) == 0)
    {
        return NULL;
    }
#ifdef CONF_PIE_AND_NEWER
    ColorAspects tmpColorAspects;
    adapteColorAspects(&tmpColorAspects,
                       pCtx->pPicture->transfer_characteristics,
                       pCtx->pPicture->matrix_coeffs,
                       pCtx->pPicture->video_full_range_flag,
                       pCtx->pPicture->colour_primaries);

    if(isColorAspectsDiffer(pCtx->mDefaultColorAspects, tmpColorAspects))
    {
         OMX_BOOL bUpdate = updateColorAspects(pCtx, tmpColorAspects);
          //*only h265 can decode coloraspects now
          if(bUpdate && pCtx->mStreamInfo.eCodecFormat == VIDEO_CODEC_FORMAT_H265)
          {
              pCtx->callback(pCtx->pUserData, AW_OMX_CB_DATASPACE_CHANGE, 0);
              return NULL;
          }

    }
#endif
    pOutBufHdr = doRequestPortBuffer(pCtx->pOutPort);
    if(pOutBufHdr == NULL)
    {
        return NULL;
    }

    OmxAcquireMutex(pCtx->awMutexHandler);
    pCtx->pPicture = RequestPicture(pCtx->m_decoder, 0);
    OmxReleaseMutex(pCtx->awMutexHandler);
    omx_logv("*** get picture[%p]",pCtx->pPicture);
    if(pCtx->pPicture == NULL)
    {
        omx_logw("the pPicture is null when request displayer picture!");
        return NULL;
    }
    omx_logv("*** picture info: w(%d),h(%d),offset,t(%d),b(%d),l(%d),r(%d)",
         pCtx->pPicture->nWidth,    pCtx->pPicture->nHeight,
         pCtx->pPicture->nTopOffset,pCtx->pPicture->nBottomOffset,
         pCtx->pPicture->nLeftOffset,pCtx->pPicture->nRightOffset);
    CdcMemFlushCache(pCtx->decMemOps, (void*)pCtx->pPicture->pData0,
                     pCtx->pPicture->nLineStride * pCtx->pPicture->nHeight*3/2);


#if(ENABLE_SAVE_PICTURE)
    savePictureForDebug(pCtx, pPicture);
#endif
    OMX_PARAM_PORTDEFINITIONTYPE* outDef = getPortDef(pCtx->pOutPort);

    if(!pCtx->bUseAndroidBuffer)
    {
        mTranBufferInfo.pAddr   = pOutBufHdr->pBuffer;
        mTranBufferInfo.nWidth  = outDef->format.video.nFrameWidth;
        mTranBufferInfo.nHeight = outDef->format.video.nFrameHeight;
        if (pPicture->ePixelFormat == PIXEL_FORMAT_YV12
            && pCtx->pOutPort->m_sPortFormatType.eColorFormat == COLOR_FORMAT_DEFAULT)
        {
            TransformYV12ToYUV420(pCtx->pPicture, &mTranBufferInfo);  // YUV420 planar
        }
        else if (pPicture->ePixelFormat == PIXEL_FORMAT_NV21
            && pCtx->pOutPort->m_sPortFormatType.eColorFormat == COLOR_FORMAT_DEFAULT)
        {
            TransformNV21ToYUV420(pCtx->pPicture, &mTranBufferInfo);  // YUV420 planar
        }
        else
        {
            omx_loge("Conversion from  pixelformat (%d) to YUV420 is not supported", pPicture->ePixelFormat);
            return NULL;
        }

    }
    else
    {
        copyPictureDataToAndroidBuffer(pCtx, pCtx->pPicture, pOutBufHdr);
    }
    pOutBufHdr->nTimeStamp = pCtx->pPicture->nPts;
    pOutBufHdr->nOffset    = 0;
    pOutBufHdr->nFilledLen = (outDef->format.video.nFrameWidth *
                              outDef->format.video.nFrameHeight) * 3 / 2;
    return pOutBufHdr;
}
#if !defined(CONF_DI_300_SUPPORT)
static OMX_BUFFERHEADERTYPE* drainOutBufferInterlace(OmxAwDecoderContext *pCtx)
{
    OMX_U32      i = 0;
    OMX_S32      nDiBufferIndex = -1;
    VideoPicture*           pPicture     = NULL;
    VideoPicture*           pDiOutPicture = NULL;
    OMX_BUFFERHEADERTYPE*   pOutBufHdr  = NULL;

    if(pCtx->mDiOutBufferInfo.nValidSize <= 0)
        return NULL;

    // 1. find a valid buffer for deinterlace output
    for(i=0; i<DI_BUFFER_NUM_KEEP_IN_OMX; i++)
    {
        if(pCtx->mDiOutBufferInfo.pArrPic[i] != NULL)
            break;
    }

    if(i>=DI_BUFFER_NUM_KEEP_IN_OMX)
    {
        omx_logw("cannot found the free di buffer");
        return NULL;
    }

    pDiOutPicture = pCtx->mDiOutBufferInfo.pArrPic[i];
    nDiBufferIndex = i;

    // 2. get picture from decoder
    if(pCtx->pCurPicture == NULL)
    {
        pCtx->mDiProcessCount = 0;

        pPicture = RequestPicture(pCtx->m_decoder, 0);
        if(pPicture == NULL)
        {
            omx_logw("the pPicture is null when request displayer picture!");
            return NULL;
        }
        omx_logv("*** picture info: w(%d),h(%d),offset,t(%d),b(%d),l(%d),r(%d)",
              pPicture->nWidth,pPicture->nHeight,pPicture->nTopOffset,
              pPicture->nBottomOffset,pPicture->nLeftOffset,pPicture->nRightOffset);

        //* we should return buffer to decoder if it was used as set eos to render
        if(pCtx->mOutputBufferInfoArr[i].mUseDecoderBufferToSetEosFlag)
        {
            omx_logw("detect the buffer had been used to set eos to render,\
                  here, we not callback again!");
            pCtx->mOutputBufferInfoArr[i].mStatus = OWNED_BY_RENDER;
            pCtx->mOutputBufferInfoArr[i].mUseDecoderBufferToSetEosFlag = OMX_FALSE;
            return NULL;
        }

        if(pPicture->bBottomFieldError || pPicture->bTopFieldError)
        {
            omx_logw("the picture error , discard this frame");
#if (DI_PROCESS_3_PICTURE)
            if(pCtx->pPrePrePicture != pCtx->pPrePicture && pCtx->pPrePrePicture != NULL)
            {
                ReturnPicture(pCtx->m_decoder, pCtx->pPrePrePicture);
                setOutBufferStatus(pCtx, pCtx->pPrePrePicture, OWNED_BY_DECODER);
            }
#endif

            if(pCtx->pPrePicture != pCtx->pCurPicture && pCtx->pPrePicture!= NULL)
            {
                ReturnPicture(pCtx->m_decoder, pCtx->pPrePicture);
                setOutBufferStatus(pCtx, pCtx->pPrePicture, OWNED_BY_DECODER);
            }

            pCtx->pPrePrePicture= NULL;
            pCtx->pPrePicture = NULL;
            pCtx->pCurPicture = NULL;
            return NULL;
        }

        pCtx->pCurPicture = pPicture;
        setOutBufferStatus(pCtx, pPicture, OWNED_BY_US);
    }

    #if (DI_PROCESS_3_PICTURE)
    if(pCtx->pPrePrePicture == NULL)
    {
        pCtx->pPrePrePicture = pCtx->pCurPicture;
        pCtx->pPrePicture = pCtx->pCurPicture;
    }
    #else
    if(pCtx->pPrePicture == NULL)
    {
        pCtx->pPrePicture = pCtx->pCurPicture;
    }
    #endif

    // 3. deinterlace process
#if (DI_PROCESS_3_PICTURE)
    int ret = CdcDiProcess2(pCtx->mDi, pCtx->pPrePrePicture, pCtx->pPrePicture,
                            pCtx->pCurPicture, pDiOutPicture, pCtx->mDiProcessCount);
    omx_logv("**deinterlace: cdc di processes 3 input pictures, %d, %d, %d",
        pCtx->pPrePrePicture->nBufFd, pCtx->pPrePicture->nBufFd, pCtx->pCurPicture->nBufFd);
#else
    int ret = CdcDiProcess(pCtx->mDi, pCtx->pPrePicture,
                            pCtx->pCurPicture, pDiOutPicture, pCtx->mDiProcessCount);
    omx_logv("**deinterlace: cdc di processes 2 input pictures");
#endif

    if(ret != 0)
    {
        omx_logw("deinterlace process failed, reset...");
        CdcDiReset(pCtx->mDi);
#if (DI_PROCESS_3_PICTURE)
        if(pCtx->pPrePrePicture != pCtx->pPrePicture)
        {
            ReturnPicture(pCtx->m_decoder, pCtx->pPrePrePicture);
            setOutBufferStatus(pCtx, pCtx->pPrePrePicture, OWNED_BY_DECODER);
        }
#endif

        if(pCtx->pPrePicture != pCtx->pCurPicture)
        {
            ReturnPicture(pCtx->m_decoder, pCtx->pPrePicture);
            setOutBufferStatus(pCtx, pCtx->pPrePicture, OWNED_BY_DECODER);
        }

#if (DI_PROCESS_3_PICTURE)
        pCtx->pPrePrePicture = pCtx->pCurPicture;
#endif
        pCtx->pPrePicture = pCtx->pCurPicture;
        pCtx->pCurPicture = NULL;
        pCtx->mDiProcessCount = 0;

        return NULL;
    }

#if(ENABLE_SAVE_PICTURE)
    savePictureForDebug(pCtx, pDiOutPicture);
#endif

    pCtx->mDiProcessCount++;

    // 4. return picture
    if(pCtx->mDiProcessNum == 1 || (pCtx->mDiProcessNum == 2 && pCtx->mDiProcessCount >=2))
    {
#if (DI_PROCESS_3_PICTURE)
        if(pCtx->pPrePrePicture != pCtx->pPrePicture && pCtx->pPrePrePicture != NULL)
        {
            ReturnPicture(pCtx->m_decoder, pCtx->pPrePrePicture);
            setOutBufferStatus(pCtx, pCtx->pPrePrePicture, OWNED_BY_DECODER);
        }

        pCtx->pPrePrePicture = pCtx->pPrePicture;

#else
        if(pCtx->pPrePicture != pCtx->pCurPicture && pCtx->pPrePicture!= NULL)
        {
        ReturnPicture(pCtx->m_decoder, pCtx->pPrePicture);
        setOutBufferStatus(pCtx, pCtx->pPrePicture, OWNED_BY_DECODER);
        }
#endif

        pCtx->pPrePicture = pCtx->pCurPicture;
        pCtx->pCurPicture = NULL;
    }

    OMX_U32 num = getPortAllocSize(pCtx->pOutPort);
    for(i=0; i<num; i++)
    {
        if(pCtx->mOutputBufferInfoArr[i].pVideoPicture == pDiOutPicture)
        break;
    }
    if(i == num)
    {
        omx_loge("the picture request from decoder is not match");
        abort();
    }

    pOutBufHdr = pCtx->mOutputBufferInfoArr[i].pHeadBufInfo;
    //* set output buffer info
    pOutBufHdr->nTimeStamp = pDiOutPicture->nPts;
    pOutBufHdr->nOffset    = 0;
    if(pOutBufHdr->pOutputPortPrivate != NULL)
    {
        AW_OMX_VIDEO_HDR_INFO* pHdrInfo
            = (AW_OMX_VIDEO_HDR_INFO*)pOutBufHdr->pOutputPortPrivate;

        pHdrInfo->nExtMatrixCoeffs = pDiOutPicture->matrix_coeffs;
        pHdrInfo->nExtVideoFullRangeFlag = pDiOutPicture->video_full_range_flag;
        pHdrInfo->nExtTransferCharacteristics = pDiOutPicture->transfer_characteristics;

        omx_logv("***hdr info: %lu, %lu, %lu",
            pHdrInfo->nExtMatrixCoeffs,
            pHdrInfo->nExtVideoFullRangeFlag,
            pHdrInfo->nExtTransferCharacteristics);
    }
    //setOutBufferStatus(pCtx, pDiOutPicture, OWNED_BY_RENDER);
    pCtx->mOutputBufferInfoArr[i].mStatus = OWNED_BY_RENDER;
    pCtx->mDiOutBufferInfo.pArrPic[nDiBufferIndex] = NULL;
    pCtx->mDiOutBufferInfo.nValidSize--;

#if ENABLE_SHOW_BUFINFO_STATUS
    omx_logd("drainOutBufferInterlace. showBufferInfoArrStatus");
    showBufferInfoArrStatus(pCtx);
#endif

    OMX_PARAM_PORTDEFINITIONTYPE* outDef = getPortDef(pCtx->pOutPort);
#ifdef CONF_KITKAT_AND_NEWER
    if(pCtx->bStoreOutputMetaDataFlag==OMX_TRUE)
    {
        pOutBufHdr->nFilledLen = sizeof(VideoDecoderOutputMetaData);
    }
    else
    {
        pOutBufHdr->nFilledLen = (outDef->format.video.nFrameWidth* \
                                  outDef->format.video.nFrameHeight)*3/2;
    }
#else
    pOutBufHdr->nFilledLen = (outDef->format.video.nFrameWidth *
                              outDef->format.video.nFrameHeight) * 3 / 2;
#endif

    return pOutBufHdr;
}
#else //For DI300
static OMX_BUFFERHEADERTYPE* drainOutBufferInterlace(OmxAwDecoderContext *pCtx)
{
    OMX_U32      i = 0;
    OMX_S32      nDiBufferIndex = -1;
    OMX_S32      nDiBufferIndex1 = -1;
    VideoPicture*           pPicture     = NULL;
    VideoPicture*           pDiOutPicture = NULL;
    VideoPicture*           pDiOutPicture1 = NULL;
    VideoPicture*           pDiOutTnrPicture = NULL;
    OMX_BUFFERHEADERTYPE*   pOutBufHdr  = NULL;
    OMX_BUFFERHEADERTYPE*   pOutBufHdr1  = NULL;
    OMX_U32      nFreeDiCount = 0;
    OMX_BOOL     bOnly1OutputPic = OMX_FALSE;

    if(pCtx->pOutBufHdr4DiOutPic1)
    {
        pOutBufHdr1 = pCtx->pOutBufHdr4DiOutPic1;
        pCtx->pOutBufHdr4DiOutPic1 = NULL;
        pCtx->mOutputBufferInfoArr[pCtx->mDiBufferIndex1].mStatus = OWNED_BY_RENDER;
        omx_logv("return out buffer stored. addr %p. pts %lld.",
             pOutBufHdr1, pOutBufHdr1->nTimeStamp/1000);
        return pOutBufHdr1;
    }

    if(pCtx->mDiOutBufferInfo.nValidSize <= 2)
        return NULL;

    // 1. find 3 valid buffer for deinterlace output
    for(i=0; i<DI_BUFFER_NUM_KEEP_IN_OMX; i++)
    {
        if(pCtx->mDiOutBufferInfo.pArrPic[i] != NULL &&
            !equalOutBufferStatus(pCtx, pCtx->mDiOutBufferInfo.pArrPic[i], OWNED_BY_TNR))
        {
            nFreeDiCount++;
            if(nFreeDiCount == 1)
            {
                pDiOutPicture = pCtx->mDiOutBufferInfo.pArrPic[i];
                nDiBufferIndex = i;
                if(pCtx->mDi300ProcessCount <= 1)
                {
                    bOnly1OutputPic = OMX_TRUE;
                    break;
                }
            }
            else if(nFreeDiCount == 2)
            {
                pDiOutPicture1 = pCtx->mDiOutBufferInfo.pArrPic[i];
                nDiBufferIndex1 = i;
            }
            else if(nFreeDiCount == 3)
            {
                pDiOutTnrPicture = pCtx->mDiOutBufferInfo.pArrPic[i];
                setOutBufferStatus(pCtx, pDiOutTnrPicture, OWNED_BY_TNR);
                break;
            }
            else
            {
                break;
            }
        }
    }

    if(i>=DI_BUFFER_NUM_KEEP_IN_OMX)
    {
        omx_logv("cannot found the free di buffer. FreeDiCount %lu.", nFreeDiCount);
        return NULL;
    }

    omx_logv("nFreeDiCount [%lu]. pDiOutPicture [%p]. pDiOutPicture1 [%p]. "
            "pDiOutTnrPicture [%p].", nFreeDiCount, pDiOutPicture,
            pDiOutPicture1, pDiOutTnrPicture);
    // 2. get picture from decoder
    if(pCtx->pCurPicture == NULL)
    {
        pPicture = RequestPicture(pCtx->m_decoder, 0);
        if(pPicture == NULL)
        {
            omx_logw("the pPicture is null when request displayer picture!");
            return NULL;
        }
        omx_logv("*** picture info: w(%d),h(%d),offset,t(%d),b(%d),l(%d),r(%d)",
              pPicture->nWidth,pPicture->nHeight,pPicture->nTopOffset,
              pPicture->nBottomOffset,pPicture->nLeftOffset,pPicture->nRightOffset);

        //* we should return buffer to decoder if it was used as set eos to render
        if(pCtx->mOutputBufferInfoArr[i].mUseDecoderBufferToSetEosFlag)
        {
            omx_logw("detect the buffer had been used to set eos to render,\
                  here, we not callback again!");
            pCtx->mOutputBufferInfoArr[i].mStatus = OWNED_BY_RENDER;
            pCtx->mOutputBufferInfoArr[i].mUseDecoderBufferToSetEosFlag = OMX_FALSE;
            return NULL;
        }

        if(pPicture->bBottomFieldError || pPicture->bTopFieldError)
        {
            omx_logw("the picture error , discard this frame");
            // In DI300, retrun or cancel pics.
            if(!pCtx->pPrePrePicture && pCtx->pPrePrePicture != pCtx->pPrePicture)
            {
                if(pCtx->mDi300ProcessCount <= 3)
                {
                    ReturnPicture(pCtx->m_decoder, pCtx->pPrePrePicture);
                    setOutBufferStatus(pCtx, pCtx->pPrePrePicture, OWNED_BY_DECODER);
                }
                else
                {
                    setOutBufferStatus(pCtx, pCtx->pPrePrePicture, OWNED_BY_US);
                }
            }
            if(!pCtx->pPrePicture && pCtx->pPrePicture != pCtx->pCurPicture)
            {
                if(pCtx->mDi300ProcessCount <= 2)
                {
                    ReturnPicture(pCtx->m_decoder, pCtx->pPrePicture);
                    setOutBufferStatus(pCtx, pCtx->pPrePicture, OWNED_BY_DECODER);
                }
                else
                {
                    setOutBufferStatus(pCtx, pCtx->pPrePicture, OWNED_BY_US);
                }
            }

            pCtx->pPrePrePicture= NULL;
            pCtx->pPrePicture = NULL;
            //pCtx->pCurPicture = NULL;
            pCtx->mDi300ProcessCount = 0;
            return NULL;
        }

        pCtx->pCurPicture = pPicture;
        setOutBufferStatus(pCtx, pPicture, OWNED_BY_US);
    }

    // In DI300, set 3 input pics.
    // pre-pre-pic is null ==> 1st round ==> P0 + P0 +P0
    if(pCtx->pPrePrePicture == NULL)
    {
        pCtx->pPrePrePicture = pCtx->pCurPicture;
        pCtx->pPrePicture = pCtx->pCurPicture;
    }

    // 3. deinterlace process
    // In DI300, process outputs 3 pics, output 2 pics and 1 tnr picture as next input pre-pic.
    // DiProcessCount is 0. 2 pics should be callbacked to display.

    int ret = CdcDiProcess3(pCtx->mDi, pCtx->pPrePrePicture, pCtx->pPrePicture,
                            pCtx->pCurPicture, pDiOutPicture,
                            pDiOutPicture1, pDiOutTnrPicture, 0/*pCtx->mDiProcessCount*/);
    if(bOnly1OutputPic)
        omx_logv("** di processed 3 input pictures, %d, %d, %d. output pics %d",
            pCtx->pPrePrePicture->nBufFd, pCtx->pPrePicture->nBufFd, pCtx->pCurPicture->nBufFd,
            pDiOutPicture->nBufFd/* , pDiOutPicture1->nBufFd, pDiOutTnrPicture->nBufFd*/);
    else
        omx_logv("** di processed 3 input pictures, %d, %d, %d. output pics %d %d %d.",
            pCtx->pPrePrePicture->nBufFd, pCtx->pPrePicture->nBufFd, pCtx->pCurPicture->nBufFd,
            pDiOutPicture->nBufFd , pDiOutPicture1->nBufFd, pDiOutTnrPicture->nBufFd);

    if(ret != 0)
    {
        omx_logw("deinterlace process failed, reset...");
        CdcDiReset(pCtx->mDi);
        //In DI300, if process fails, pics should return or cancel.
        //If process num <= 3, return pre-pre-pic to decoder. Otherwise, cancel it to OMX
        //If process num <=2, return pre-pic to decoder. Otherwise, cancel it to OMX
        if(!pCtx->pPrePrePicture && pCtx->pPrePrePicture != pCtx->pPrePicture)
        {
            if(pCtx->mDi300ProcessCount <= 3)
            {
                ReturnPicture(pCtx->m_decoder, pCtx->pPrePrePicture);
                setOutBufferStatus(pCtx, pCtx->pPrePrePicture, OWNED_BY_DECODER);
            }
            else
            {
                setOutBufferStatus(pCtx, pCtx->pPrePrePicture, OWNED_BY_US);
            }
        }
        if(!pCtx->pPrePicture && pCtx->pPrePicture != pCtx->pCurPicture)
        {
            if(pCtx->mDi300ProcessCount <= 2)
            {
                ReturnPicture(pCtx->m_decoder, pCtx->pPrePicture);
                setOutBufferStatus(pCtx, pCtx->pPrePicture, OWNED_BY_DECODER);
            }
            else
            {
                setOutBufferStatus(pCtx, pCtx->pPrePicture, OWNED_BY_US);
            }
        }

        // diff from previous DI reset operations. Now reset all inputs
        pCtx->pPrePrePicture = NULL;
        pCtx->pPrePicture = NULL;
        pCtx->mDi300ProcessCount = 0;

        return NULL;
    }

#if(ENABLE_SAVE_PICTURE)
    savePictureForDebug(pCtx, pDiOutPicture);
    if(pDiOutPicture1)
        savePictureForDebug(pCtx, pDiOutPicture1);
#endif

    // 4. return picture
    // In DI300, return or cancel pre-pre-pic. set pre pic as pre-pre-pic, set di tnr pic as pre-pic.
    if(pCtx->pPrePrePicture != pCtx->pPrePicture && pCtx->pPrePrePicture != NULL)
    {
        //if process num <=3, return pre-pre-pic to decoder.
        //Otherwise, cancel pre-pre-pic to omx.
        //return cur-pic to decoder anyway.
        if(pCtx->mDi300ProcessCount <= 3)
        {
            ReturnPicture(pCtx->m_decoder, pCtx->pPrePrePicture);
            setOutBufferStatus(pCtx, pCtx->pPrePrePicture, OWNED_BY_DECODER);
        }
        else
        {
            setOutBufferStatus(pCtx, pCtx->pPrePrePicture, OWNED_BY_US);
        }
        if(pCtx->mDi300ProcessCount >= 2)
        {
            ReturnPicture(pCtx->m_decoder, pCtx->pCurPicture);
            setOutBufferStatus(pCtx, pCtx->pCurPicture, OWNED_BY_DECODER);
            omx_logv("return picture [%d] of 3rd di input to decoder.", pCtx->pCurPicture->nBufFd);
        }
    }
    //if process num <=2, cur-pic is reset as pre-pic, otherwise, di tnr pic reset as pre-pic.
    if(pCtx->pPrePrePicture == pCtx->pPrePicture && pCtx->pPrePicture != NULL)
    {
        omx_logv("reset di pre pic by cur pic. pre-pre-picture=%p, pre-pic=%p, cur-pic=%p",
               pCtx->pPrePrePicture, pCtx->pPrePicture, pCtx->pCurPicture);
        pCtx->pPrePrePicture = pCtx->pPrePicture;
        pCtx->pPrePicture = pCtx->pCurPicture;
        pCtx->pCurPicture = NULL;
    }
    else
    {
        omx_logv("reset di pre pic by tnr pic. pre-pre-picture=%p, pre-pic=%p, cur-pic=%p, DiOutTnrPic=%p.",
                pCtx->pPrePrePicture, pCtx->pPrePicture, pCtx->pCurPicture, pDiOutTnrPicture);
        pCtx->pPrePrePicture = pCtx->pPrePicture;
        pCtx->pPrePicture = pDiOutTnrPicture;
        pDiOutTnrPicture = NULL;
        pCtx->pCurPicture = NULL;
    }
    pCtx->mDi300ProcessCount++;

    //Link 2 output pic with buffer header, store the second buffer header and
    //return it next time directly without di process.
    OMX_U32 num = getPortAllocSize(pCtx->pOutPort);
    for(i=0; i<num; i++)
    {
        if(pCtx->mOutputBufferInfoArr[i].pVideoPicture == pDiOutPicture)
            break;
    }
    if(i == num)
    {
        omx_loge("the picture request from decoder is not match");
        abort();
    }
    int k = 0;
    if(!bOnly1OutputPic)
    {
        for(k=0; k<num; k++)
        {
            if(pCtx->mOutputBufferInfoArr[k].pVideoPicture == pDiOutPicture1)
                break;
        }
        if(k == num)
        {
            omx_loge("the picture request from decoder is not match");
            abort();
        }
    }
    pOutBufHdr = pCtx->mOutputBufferInfoArr[i].pHeadBufInfo;
    //* set output buffer info
    pOutBufHdr->nTimeStamp = pDiOutPicture->nPts;
    pOutBufHdr->nOffset    = 0;
    if(!bOnly1OutputPic)
    {
        pOutBufHdr1 = pCtx->mOutputBufferInfoArr[k].pHeadBufInfo;
        //* set output buffer info
        pOutBufHdr1->nTimeStamp = pDiOutPicture1->nPts;
        pOutBufHdr1->nOffset    = 0;
        omx_logv("di out picture 1 pts %lld  picture 2 pts %lld.",
                pOutBufHdr->nTimeStamp/1000, pOutBufHdr1->nTimeStamp/1000);
    }
    if(pOutBufHdr->pOutputPortPrivate != NULL)
    {
        AW_OMX_VIDEO_HDR_INFO* pHdrInfo
            = (AW_OMX_VIDEO_HDR_INFO*)pOutBufHdr->pOutputPortPrivate;

        pHdrInfo->nExtMatrixCoeffs = pDiOutPicture->matrix_coeffs;
        pHdrInfo->nExtVideoFullRangeFlag = pDiOutPicture->video_full_range_flag;
        pHdrInfo->nExtTransferCharacteristics = pDiOutPicture->transfer_characteristics;

        omx_logv("***hdr info for di output pic 0: %lu, %lu, %lu",
            pHdrInfo->nExtMatrixCoeffs,
            pHdrInfo->nExtVideoFullRangeFlag,
            pHdrInfo->nExtTransferCharacteristics);
    }
    if(!bOnly1OutputPic && pOutBufHdr1->pOutputPortPrivate != NULL)
    {
        AW_OMX_VIDEO_HDR_INFO* pHdrInfo1
            = (AW_OMX_VIDEO_HDR_INFO*)pOutBufHdr1->pOutputPortPrivate;

        pHdrInfo1->nExtMatrixCoeffs = pDiOutPicture1->matrix_coeffs;
        pHdrInfo1->nExtVideoFullRangeFlag = pDiOutPicture1->video_full_range_flag;
        pHdrInfo1->nExtTransferCharacteristics = pDiOutPicture1->transfer_characteristics;

        omx_logv("***hdr info for di ouptut pic 1: %lu, %lu, %lu",
            pHdrInfo1->nExtMatrixCoeffs,
            pHdrInfo1->nExtVideoFullRangeFlag,
            pHdrInfo1->nExtTransferCharacteristics);
    }
    //setOutBufferStatus(pCtx, pDiOutPicture, OWNED_BY_RENDER);
    pCtx->mOutputBufferInfoArr[i].mStatus = OWNED_BY_RENDER;
    pCtx->mDiOutBufferInfo.pArrPic[nDiBufferIndex] = NULL;
    pCtx->mDiOutBufferInfo.nValidSize--;
    if(!bOnly1OutputPic)
    {
        pCtx->mOutputBufferInfoArr[k].mStatus = OWNED_BY_PENDING;
        pCtx->mDiBufferIndex1 = k;
        pCtx->mDiOutBufferInfo.pArrPic[nDiBufferIndex1] = NULL;
        pCtx->mDiOutBufferInfo.nValidSize--;
    }
#if ENABLE_SHOW_BUFINFO_STATUS
    omx_logd("drainOutBufferInterlace (DI300). showBufferInfoArrStatus");
    showBufferInfoArrStatus(pCtx);
#endif

    OMX_PARAM_PORTDEFINITIONTYPE* outDef = getPortDef(pCtx->pOutPort);
#ifdef CONF_KITKAT_AND_NEWER
    if(pCtx->bStoreOutputMetaDataFlag==OMX_TRUE)
    {
        pOutBufHdr->nFilledLen = sizeof(VideoDecoderOutputMetaData);
        if(!bOnly1OutputPic)
            pOutBufHdr1->nFilledLen = sizeof(VideoDecoderOutputMetaData);
    }
    else
    {
            pOutBufHdr->nFilledLen = (outDef->format.video.nFrameWidth* \
                                  outDef->format.video.nFrameHeight)*3/2;
            if(!bOnly1OutputPic)
                pOutBufHdr1->nFilledLen =  (outDef->format.video.nFrameWidth* \
                                  outDef->format.video.nFrameHeight)*3/2;
    }
#else
        pOutBufHdr->nFilledLen = (outDef->format.video.nFrameWidth *
                              outDef->format.video.nFrameHeight) * 3 / 2;
        if(!bOnly1OutputPic)
             pOutBufHdr1->nFilledLen = (outDef->format.video.nFrameWidth *
                              outDef->format.video.nFrameHeight) * 3 / 2;
#endif
    if(!bOnly1OutputPic)
        pCtx->pOutBufHdr4DiOutPic1 = pOutBufHdr1;
    omx_logv("return out buffer. addr %p. pts %lld.  %lu.", pOutBufHdr, pOutBufHdr->nTimeStamp/1000, pOutBufHdr->nFilledLen);
    return pOutBufHdr;
}

#endif

static OMX_BUFFERHEADERTYPE* drainOutBufferProgressive(OmxAwDecoderContext *pCtx)
{
    OMX_U32      i = 0;
    VideoPicture*           pPicture     = NULL;
    OMX_BUFFERHEADERTYPE*   pOutBufHdr  = NULL;
    int  nSavePictureFlag = 0;

    OmxAcquireMutex(pCtx->awMutexHandler);
    pPicture = NextPictureInfo(pCtx->m_decoder,0);
    OmxReleaseMutex(pCtx->awMutexHandler);
    if(pPicture == NULL)
        return NULL;

#ifdef CONF_PIE_AND_NEWER
    ColorAspects tmpColorAspects;
    adapteColorAspects(&tmpColorAspects,
                       pPicture->transfer_characteristics,
                       pPicture->matrix_coeffs,
                       pPicture->video_full_range_flag,
                       pPicture->colour_primaries);
    if(isColorAspectsDiffer(pCtx->mBitStreamColorAspects, tmpColorAspects))
    {
       pCtx->mBitStreamColorAspects = tmpColorAspects;
       OMX_BOOL bUpdate = updateColorAspects(pCtx, tmpColorAspects);
        //*only h265 can decode coloraspects now
        if(bUpdate && pCtx->mStreamInfo.eCodecFormat == VIDEO_CODEC_FORMAT_H265)
        {
            pCtx->callback(pCtx->pUserData, AW_OMX_CB_DATASPACE_CHANGE, 0);
            return NULL;
        }
    }

#endif

    pPicture = RequestPicture(pCtx->m_decoder, 0);
    if(pPicture == NULL)
    {
        omx_logw("the pPicture is null when request displayer picture!");
        return NULL;
    }
    omx_logv("*** picture info: w(%d),h(%d),offset,t(%d),b(%d),l(%d),r(%d)",
         pPicture->nWidth,pPicture->nHeight,
         pPicture->nTopOffset,pPicture->nBottomOffset,
         pPicture->nLeftOffset,pPicture->nRightOffset);

#if(ENABLE_SAVE_PICTURE)
    nSavePictureFlag = 1 ;
#endif
#ifdef __ANDROID__
    char val[256];
    if(nSavePictureFlag == 0)
    {
        property_get("vendor.vdec.savepicture", val, "0");
        nSavePictureFlag = atoi(val);
    }
#endif

    if(nSavePictureFlag)
    {
        savePictureForDebug(pCtx, pPicture);
    }

    OMX_U32 num = getPortAllocSize(pCtx->pOutPort);
    for(i = 0; i<num; i++)
    {
        if(pCtx->mOutputBufferInfoArr[i].pVideoPicture == pPicture)
            break;
    }

    OMX_ASSERT(i!=num);

    //* we should return buffer to decoder if it was used as set eos to render
    if(pCtx->mOutputBufferInfoArr[i].mUseDecoderBufferToSetEosFlag)
    {
        omx_logw("detect the buffer had been used to set eos to render,\
              here, we not callback again!");
        pCtx->mOutputBufferInfoArr[i].mStatus = OWNED_BY_RENDER;
        pCtx->mOutputBufferInfoArr[i].mUseDecoderBufferToSetEosFlag = OMX_FALSE;
        return NULL;
    }

    pOutBufHdr =  pCtx->mOutputBufferInfoArr[i].pHeadBufInfo;

    pCtx->mOutputBufferInfoArr[i].mStatus = OWNED_BY_RENDER;

    pOutBufHdr->nTimeStamp = pPicture->nPts;
    pOutBufHdr->nOffset    = 0;
    OMX_PARAM_PORTDEFINITIONTYPE* outDef = getPortDef(pCtx->pOutPort);

#ifdef CONF_KITKAT_AND_NEWER

    if(pOutBufHdr->pOutputPortPrivate != NULL)
    {
        AW_OMX_VIDEO_HDR_INFO* pHdrInfo =
            (AW_OMX_VIDEO_HDR_INFO*)pOutBufHdr->pOutputPortPrivate;

        pHdrInfo->nExtMatrixCoeffs = pPicture->matrix_coeffs;
        pHdrInfo->nExtVideoFullRangeFlag = pPicture->video_full_range_flag;
        pHdrInfo->nExtTransferCharacteristics = pPicture->transfer_characteristics;

        logv("***hdr info: %lu, %lu, %lu",
            pHdrInfo->nExtMatrixCoeffs,
            pHdrInfo->nExtVideoFullRangeFlag,
            pHdrInfo->nExtTransferCharacteristics);
    }

    if(pCtx->bStoreOutputMetaDataFlag)
    {
        pOutBufHdr->nFilledLen = sizeof(VideoDecoderOutputMetaData);
    }
    else
    {
        pOutBufHdr->nFilledLen = (outDef->format.video.nFrameWidth* \
                                  outDef->format.video.nFrameHeight)*3/2;
    }
#else
    pOutBufHdr->nFilledLen = (outDef->format.video.nFrameWidth *
                              outDef->format.video.nFrameHeight) * 3 / 2;
#endif
    return pOutBufHdr;
}

static OMX_BUFFERHEADERTYPE* anDrainZeroCopy(OmxAwDecoderContext *pCtx)
{
    if(pCtx->mEnableDiFlag>0 && pCtx->mDi)
        return drainOutBufferInterlace(pCtx);
    else
        return drainOutBufferProgressive(pCtx);
}

static int freeVirAddrOfMetadataBuffer(int mIonFd,
                                ion_handle_abstract_t handle_ion,
                                int mapfd,
                                int virsize,
                                void* viraddress)
{

#if (defined(CONF_HIGH_DYNAMIC_RANGE_ENABLE) || defined(CONF_AFBC_ENABLE))
    if (viraddress != 0) {
        CdcIonMunmap(virsize, (unsigned char*)viraddress);
    }
#ifndef CONF_SIMPLE_ION
    if (mapfd != 0) {
        close(mapfd);
    }
    if (handle_ion != 0) {
        ion_free(mIonFd, handle_ion);
    }
#endif
#else
    CEDARC_UNUSE(mIonFd);
    CEDARC_UNUSE(handle_ion);
    CEDARC_UNUSE(mapfd);
    CEDARC_UNUSE(virsize);
    CEDARC_UNUSE(viraddress);
#endif
    return 0;
}

static void freeOutputBufferInfoArr(OmxAwDecoderContext *pCtx)
{
    int i = 0; int ret = 0;
    for(i = 0; i < OMX_MAX_VIDEO_BUFFER_NUM; i++)
    {

        if(pCtx->mOutputBufferInfoArr[i].pBufVirAddr != NULL)
        {
            //omx_logd("ion_free: pBufVirAddr[%p],nBufFd %d",pCtx->mOutputBufferInfoArr[i].pBufVirAddr,pCtx->mOutputBufferInfoArr[i].nBufFd);
            if(pCtx->mOutputBufferInfoArr[i].nBufFd >= 0)
            {
                if(!pCtx->bIsSecureVideoFlag)
                {
                    int len = pCtx->mOutputBufferInfoArr[i].nVirAddrSize;
                    unsigned char *pVirAddr = \
                     (unsigned char *)pCtx->mOutputBufferInfoArr[i].pBufVirAddr;
                    CdcIonMunmap(len, pVirAddr);
                }
#ifndef CONF_SIMPLE_ION
                close(pCtx->mOutputBufferInfoArr[i].nBufFd);
#endif
            }
#ifndef CONF_SIMPLE_ION
            if(pCtx->mOutputBufferInfoArr[i].handle_ion != ION_NULL_VALUE)
            {
                ret = ion_free(pCtx->mIonFd, pCtx->mOutputBufferInfoArr[i].handle_ion);
            }
#endif
            if(ret != 0)
                omx_loge("ion_free( %d ) failed", pCtx->mIonFd);
            pCtx->mOutputBufferInfoArr[i].handle_ion = ION_NULL_VALUE;
        }

        if(pCtx->mOutputBufferInfoArr[i].pMetaDataVirAddr != NULL)
        {
            freeVirAddrOfMetadataBuffer(pCtx->mIonFd,
                             pCtx->mOutputBufferInfoArr[i].metadata_handle_ion,
                             pCtx->mOutputBufferInfoArr[i].nMetaDataMapFd,
                             pCtx->mOutputBufferInfoArr[i].nMetaDataVirAddrSize,
                             pCtx->mOutputBufferInfoArr[i].pMetaDataVirAddr);
        }
    }

    memset(&pCtx->mOutputBufferInfoArr, 0,
             sizeof(OMXOutputBufferInfoA)*OMX_MAX_VIDEO_BUFFER_NUM);
}

static int checkVideoSpecification(OmxAwDecoderContext *pCtx,
                                 OMX_BUFFERHEADERTYPE* pInBufHdr)
{
    int nFrameRate = 0;
    OmxAcquireMutex(pCtx->awMutexHandler);
    FbmBufInfo* pFbmBufInfo =  GetVideoFbmBufInfo(pCtx->m_decoder);
    OmxReleaseMutex(pCtx->awMutexHandler);
    if(pFbmBufInfo != NULL) {
        OMX_U32 nWidth;
        OMX_U32 nHeight;
         nWidth  = pFbmBufInfo->nRightOffset - pFbmBufInfo->nLeftOffset;
         nHeight = pFbmBufInfo->nBottomOffset - pFbmBufInfo->nTopOffset;
        if(pCtx->nPreSubmitPts == -1)
        {
            pCtx->nPreSubmitPts = pInBufHdr->nTimeStamp;
            logv("the pCtx->nPreSubmitPts = %lld",pCtx->nPreSubmitPts);
            return 0;
        }
        else
        {
            nFrameRate= 1000000/(pInBufHdr->nTimeStamp -pCtx->nPreSubmitPts)*1000;
            pCtx->nSpecificationFlag = 1;
            logv("the frame rate is %d,wight x height(%d x %d)",nFrameRate,nWidth,nHeight);
            if(!pCtx->mStreamInfo.nFrameRate)
                pCtx->mStreamInfo.nFrameRate = nFrameRate;
        }
        if((nWidth >= 3840 || nHeight >= 2160)
            && pCtx->mStreamInfo.eCodecFormat ==VIDEO_CODEC_FORMAT_VP9
            && nFrameRate > 30000)
        {
            loge("do not support the VP9 with %d fps",nFrameRate);
            pCtx->callback(pCtx->pUserData, AW_OMX_CB_EVENT_ERROR, NULL);
            return -1;
        }
    }
    return 0;
}

static void standbyOutBufferArr(OmxAwDecoderContext *pCtx)
{
    int i;
    OMX_BOOL        out_buffer_flag   = OMX_FALSE;

    for(i = 0; i < OMX_MAX_VIDEO_BUFFER_NUM; i++)
    {
        OutBufferStatus out_buffer_status = pCtx->mOutputBufferInfoArr[i].mStatus;
        out_buffer_flag = \
            pCtx->mOutputBufferInfoArr[i].mUseDecoderBufferToSetEosFlag;
        if(out_buffer_status == OWNED_BY_DECODER)
        {
            omx_logd("** fillBufferDone owned by decoer i[%d],pHeadBufInfo[%p],pPic[%p]", i,
                pCtx->mOutputBufferInfoArr[i].pHeadBufInfo,
                pCtx->mOutputBufferInfoArr[i].pVideoPicture);

            pCtx->callback(pCtx->pUserData, AW_OMX_CB_FILL_BUFFER_DONE,
                            pCtx->mOutputBufferInfoArr[i].pHeadBufInfo);

            pCtx->mOutputBufferInfoArr[i].mStatus = OWNED_BY_RENDER;
        }
        else if(out_buffer_status == OWNED_BY_RENDER
                && !out_buffer_flag && pCtx->bFlushFlag)
        {
            if(pCtx->mOutputBufferInfoArr[i].pVideoPicture != NULL)
            {

                omx_logw("** return pic owned by render,i[%d], pHeadBufInfo[%p], pPic[%p]", i,
                     pCtx->mOutputBufferInfoArr[i].pHeadBufInfo,
                     pCtx->mOutputBufferInfoArr[i].pVideoPicture);
                ReturnPicture(pCtx->m_decoder,
                              pCtx->mOutputBufferInfoArr[i].pVideoPicture);
                OmxTryPostSem(pCtx->mSemOutBuffer);
            }
        }
        else if(out_buffer_status == OWNED_BY_US || out_buffer_status == OWNED_BY_TNR
                || out_buffer_status== OWNED_BY_PENDING)
        {
            omx_logd("** fillbufferDone, i[%d], pHeadBufInfo[%p], pPic[%p], status[%s]", i,
                pCtx->mOutputBufferInfoArr[i].pHeadBufInfo,
                pCtx->mOutputBufferInfoArr[i].pVideoPicture,
                statusToString(out_buffer_status));
            pCtx->callback(pCtx->pUserData, AW_OMX_CB_FILL_BUFFER_DONE,
                            pCtx->mOutputBufferInfoArr[i].pHeadBufInfo);

            // in seek case, we should return the picture to decoder
            if(pCtx->mOutputBufferInfoArr[i].pVideoPicture != NULL
                && pCtx->m_decoder && pCtx->bFlushFlag)
            {
                omx_logd("** return pic, i[%d], pHeadBufInfo[%p], pPic[%p], status[%s]", i,
                    pCtx->mOutputBufferInfoArr[i].pHeadBufInfo,
                    pCtx->mOutputBufferInfoArr[i].pVideoPicture,
                    statusToString(out_buffer_status));
                ReturnPicture(pCtx->m_decoder,
                              pCtx->mOutputBufferInfoArr[i].pVideoPicture);
                pCtx->mOutputBufferInfoArr[i].mStatus = OWNED_BY_RENDER;
                OmxTryPostSem(pCtx->mSemOutBuffer);
            }
        }
    }
#ifdef CONF_DI_300_SUPPORT
    pCtx->pOutBufHdr4DiOutPic1 = NULL;
    pCtx->mDi300ProcessCount = 0;
#endif
    pCtx->pCurPicture    = NULL;
    pCtx->pPrePicture    = NULL;
    #if (DI_PROCESS_3_PICTURE)
    pCtx->pPrePrePicture = NULL;
    #endif
    memset(&pCtx->mDiOutBufferInfo, 0, sizeof(DiOutBufferInfo));
    freeOutputBufferInfoArr(pCtx);
}

static int __anCallback(OmxDecoder* pDec,
                                    DecoderCallback callback,
                                    void* pUserData)
{
    OmxAwDecoderContext *pCtx = (OmxAwDecoderContext*)pDec;
    pCtx->callback  = callback;
    pCtx->pUserData = pUserData;
    return 0;
}

static OMX_ERRORTYPE __anGetExtPara(OmxDecoder* pDec,
                                       AW_VIDEO_EXTENSIONS_INDEXTYPE eParamIndex,
                                       OMX_PTR pParamData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OmxAwDecoderContext *pCtx = (OmxAwDecoderContext*)pDec;
    OMX_PARAM_PORTDEFINITIONTYPE* inDef = getPortDef(pCtx->pInPort);
    switch(eParamIndex)
    {
        case AWOMX_IndexParamVideoGetAndroidNativeBufferUsage:
        {
            GetAndroidNativeBufferUsageParams* param;
            param = (GetAndroidNativeBufferUsageParams*)pParamData;
            if(inDef->format.video.eCompressionFormat == OMX_VIDEO_CodingVP6 ||
               inDef->format.video.eCompressionFormat == OMX_VIDEO_CodingWMV1||
               inDef->format.video.eCompressionFormat == OMX_VIDEO_CodingWMV2)
            {
                // software decoder, gpu use this usage to malloc buffer with cache.
                param->nUsage |= GRALLOC_USAGE_SW_WRITE_OFTEN;
            }
            //* gpu use this usage to allocate continuous physical buffer.
            param->nUsage |= GRALLOC_USAGE_HW_2D;

#ifdef CONF_Q_AND_NEWER
            param->nUsage |= GRALLOC1_PRODUCER_USAGE_VIDEO_DECODER;
#endif

            if(pCtx->nExtendFlag & AW_VIDEO_AFBC_FLAG)
            {
                param->nUsage |= GRALLOC_USAGE_AFBC_MODE;
            }
            omx_logd(" ===== get usage: %lx", param->nUsage);
            break;
        }
#ifndef CONF_PIE_AND_NEWER
        case AWOMX_IndexParamVideoGetAfbcHdrFlag:
        {
            omx_logd(" COMPONENT_GET_PARAMETER: AWOMX_IndexParamVideoGetAfbcHdrFlag:%lu",
                 pCtx->nExtendFlag);
            OMX_U32* pExtendFlag = (OMX_U32*)pParamData;
            *pExtendFlag = pCtx->nExtendFlag;
            *(pExtendFlag + 1) = pCtx->nExtendFlag;
            break;
        }
#endif
        default:
        {
            omx_logw("get_parameter: unknown param %08x\n", eParamIndex);
            eError =OMX_ErrorUnsupportedIndex;
            break;
        }
    }
    return eError;
}

static OMX_ERRORTYPE __anSetExtPara(OmxDecoder* pDec,
                                       AW_VIDEO_EXTENSIONS_INDEXTYPE eParamIndex,
                                       OMX_PTR pParamData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    OmxAwDecoderContext *pCtx = (OmxAwDecoderContext*)pDec;
    omx_logv("set_parameter: %x",eParamIndex);

    switch(eParamIndex)
    {
        case AWOMX_IndexParamVideoUseAndroidNativeBuffer2:
        {
            pCtx->bUseAndroidBuffer = OMX_TRUE;
            break;
        }
        case AWOMX_IndexParamVideoEnableAndroidNativeBuffers:
        {
            EnableAndroidNativeBuffersParams *EnableAndroidBufferParams
                = (EnableAndroidNativeBuffersParams*)pParamData;
            omx_logv(" enbleParam = %d\n",EnableAndroidBufferParams->enable);
            if(1==EnableAndroidBufferParams->enable)
            {
                pCtx->bUseAndroidBuffer = OMX_TRUE;
            }
            break;
        }
        case AWOMX_IndexParamVideoAllocateNativeHandle:
        {

            EnableAndroidNativeBuffersParams *EnableAndroidBufferParams
                =  (EnableAndroidNativeBuffersParams*) pParamData;
            omx_logv(" enbleParam = %d\n",EnableAndroidBufferParams->enable);
            if(1==EnableAndroidBufferParams->enable)
            {
                omx_logd("set bSurpporNativeHandle to OMX_TRUE");
                pCtx->bSurpporNativeHandle = OMX_TRUE;
            }
            break;
        }

        //* not perfectly support the meta data, so forbid it.
#if 0
        case AWOMX_IndexParamVideoUseStoreMetaDataInBuffer:
        {
            omx_logv("set_parameter: AWOMX_IndexParamVideoUseStoreMetaDataInBuffer");

            StoreMetaDataInBuffersParams *pStoreMetaData
                = (StoreMetaDataInBuffersParams*)pParamData;
            if(pStoreMetaData->nPortIndex != 1)
            {
                omx_logd("error: not support set "
                    "AWOMX_IndexParamVideoUseStoreMetaDataInBuffer for inputPort");
                eError = OMX_ErrorUnsupportedIndex;
            }
            if(pStoreMetaData->nPortIndex==1 &&
                pStoreMetaData->bStoreMetaData)
            {
                omx_logv("***set m_storeOutputMetaDataFlag to TRUE");
                pCtx->bStoreOutputMetaDataFlag = OMX_TRUE;
            }
            break;
        }
#endif

#ifdef CONF_KITKAT_AND_NEWER
        case AWOMX_IndexParamVideoUsePrepareForAdaptivePlayback:
        {
            PrepareForAdaptivePlaybackParams *pPlaybackParams;
            pPlaybackParams = (PrepareForAdaptivePlaybackParams *)pParamData;

            if(pPlaybackParams->nPortIndex==1 && pPlaybackParams->bEnable)
            {
                omx_logv("set adaptive playback ,maxWidth = %d, maxHeight = %d",
                       (int)pPlaybackParams->nMaxFrameWidth,
                       (int)pPlaybackParams->nMaxFrameHeight);

                pCtx->mMaxWidth  = pPlaybackParams->nMaxFrameWidth;
                pCtx->mMaxHeight = pPlaybackParams->nMaxFrameHeight;
            }
            break;
        }
#endif
        default:
        {
            omx_logw("Setparameter: unknown param %d\n", eParamIndex);
            eError = OMX_ErrorUnsupportedIndex;
            break;
        }
    }
    return eError;
}

static OMX_ERRORTYPE __anGetExtConfig(OmxDecoder* pDec,
                                       AW_VIDEO_EXTENSIONS_INDEXTYPE eConfigIndex,
                                       OMX_PTR pConfigData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OmxAwDecoderContext *pCtx = (OmxAwDecoderContext*)pDec;
    CEDARC_UNUSE(pCtx);
    switch(eConfigIndex)
    {
#ifdef CONF_PIE_AND_NEWER
        case AWOMX_IndexParamVideoDescribeColorAspects:
        {
            DescribeColorAspectsParams* colorAspectsParams =
                    (DescribeColorAspectsParams *)pConfigData;

            if (colorAspectsParams->nPortIndex != kOutputPortIndex) {
                eError = OMX_ErrorBadParameter;
                break;
            }

            colorAspectsParams->sAspects = pCtx->mFinalColorAspects;

            omx_logd("get ColorAspects: R:%d, P:%d, M:%d, T:%d",
            colorAspectsParams->sAspects.mRange,
            colorAspectsParams->sAspects.mPrimaries,
            colorAspectsParams->sAspects.mMatrixCoeffs,
            colorAspectsParams->sAspects.mTransfer);
            if (colorAspectsParams->bRequestingDataSpace || colorAspectsParams->bDataSpaceChanged) {
                eError = OMX_ErrorUnsupportedSetting;
                break;
            }
            break;
        }
        case AWOMX_IndexParamVideoDescribeHDRStaticInfo:
        {
            DescribeHDRStaticInfoParams* hdrStaticInfoParams =
                    (DescribeHDRStaticInfoParams *)pConfigData;

            if (hdrStaticInfoParams->nPortIndex != kOutputPortIndex) {
                eError = OMX_ErrorBadPortIndex;
                break;
            }

            hdrStaticInfoParams->sInfo = pCtx->mHdrStaticInfo;
            break;
        }
#endif
        default:
        {
            omx_logd("get_config: unknown param %d\n",eConfigIndex);
            eError = OMX_ErrorUnsupportedIndex;
            break;
        }
    }
    return eError;
}

static OMX_ERRORTYPE __anSetExtConfig(OmxDecoder* pDec,
                                       AW_VIDEO_EXTENSIONS_INDEXTYPE eConfigIndex,
                                       OMX_PTR pConfigData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    OmxAwDecoderContext *pCtx = (OmxAwDecoderContext*)pDec;

    CEDARC_UNUSE(pCtx);
    switch(eConfigIndex)
    {
#ifdef CONF_PIE_AND_NEWER
        case AWOMX_IndexParamVideoDescribeColorAspects:
        {
            const DescribeColorAspectsParams* colorAspectsParams =
                    (const DescribeColorAspectsParams *)pConfigData;

            if (colorAspectsParams->nPortIndex != kOutputPortIndex) {
                eError = OMX_ErrorBadParameter;
                break;
            }
            if (isColorAspectsDiffer(pCtx->mDefaultColorAspects, colorAspectsParams->sAspects)) {
                pCtx->mDefaultColorAspects = colorAspectsParams->sAspects;
                updateColorAspects(pCtx, pCtx->mBitStreamColorAspects);
            }
            logd("set ColorAspects: R:%d, P:%d, M:%d, T:%d",
            colorAspectsParams->sAspects.mRange,
            colorAspectsParams->sAspects.mPrimaries,
            colorAspectsParams->sAspects.mMatrixCoeffs,
            colorAspectsParams->sAspects.mTransfer);
            break;
        }
        case AWOMX_IndexParamVideoDescribeHDRStaticInfo:
        {
            const DescribeHDRStaticInfoParams* hdrStaticInfoParams =
                    (DescribeHDRStaticInfoParams *)pConfigData;

            if (hdrStaticInfoParams->nPortIndex != kOutputPortIndex) {
                eError =  OMX_ErrorBadPortIndex;
                break;
            }
            pCtx->mHdrStaticInfo = hdrStaticInfoParams->sInfo;
            //TODO
            //updatePortDefinitions(false);
            break;
        }
#endif
        default:
        {
            omx_logd("get_config: unknown param %d\n",eConfigIndex);
            eError = OMX_ErrorUnsupportedIndex;
            break;
        }
    }
    return eError;
}

static int __anPrepare(OmxDecoder* pDec)
{
    OmxAwDecoderContext *pCtx = (OmxAwDecoderContext*)pDec;

    int ret = -1;
    //In widevine L1 case, init data would not be dealt.
    if(!pCtx->bIsSecureVideoFlag)
    {
        if(-1 == anDealWithInitData(pCtx))
        {
            return -1;
        }
    }

    omx_logd("Prepare decoder begin!");
    anGetStreamInfo(pCtx);
    OmxAcquireMutex(pCtx->awMutexHandler);
    //*if mdecoder had closed before, we should create it
    if(pCtx->m_decoder==NULL)
    {
        pCtx->m_decoder = CreateVideoDecoder();
    }


    if(!pCtx->pOutPort->bAllocBySelfFlags || pCtx->bIsSecureVideoFlag)
    {
        pCtx->bUseZeroCopyOutBuffer = OMX_TRUE;
        pCtx->mVideoConfig.bGpuBufValid = 1;
    }

    if(pCtx->bIsSecureVideoFlag)
    {
        pCtx->mVideoConfig.bSecureosEn = 1;
    }

#ifdef CONF_OMX_USE_COPY_STRATEGY
    pCtx->bUseZeroCopyOutBuffer = OMX_FALSE;
    pCtx->mVideoConfig.bGpuBufValid = 0;
#endif

    pCtx->mVideoConfig.nAlignStride = pCtx->mGpuAlignStride;

    //pCtx->mVideoConfig.bCalledByOmxFlag   = 1;

#ifdef CONF_AFBC_ENABLE
    pCtx->mVideoConfig.eCtlAfbcMode = ENABLE_AFBC_JUST_BIG_SIZE;
#endif

#ifdef CONF_ENABLE_SCALEDOWN_WHEN_RESOLUTION_MOER_THAN_1080P
    // omx decoder make out buffer no more than 1920x1080
    if (pCtx->mStreamInfo.nWidth > 1920
        && pCtx->mStreamInfo.nHeight > 1088)
    {
        pCtx->mVideoConfig.bScaleDownEn = 1;
        pCtx->mVideoConfig.nHorizonScaleDownRatio = 1;
        pCtx->mVideoConfig.nVerticalScaleDownRatio = 1;
    }
#endif

    if(pCtx->mStreamInfo.eCodecFormat == VIDEO_CODEC_FORMAT_WMV3)
    {
        omx_logd("*** pCtx->mStreamInfo.bIsFramePackage to 1 when it is vc1");
        pCtx->mStreamInfo.bIsFramePackage = 1;
    }

    pCtx->mVideoConfig.bDispErrorFrame = 1;

    int nDiBufferNum = 0;
    //* add the di-buffer-num by default if picture size less than 4k, as we will enable di function
    //* (4k has no interlace video)
    if (pCtx->mStreamInfo.nWidth < 1920 && pCtx->mStreamInfo.nHeight < 1080)
        nDiBufferNum = DI_BUFFER_NUM_KEEP_IN_OMX;

    omx_logd("set nDisplayHoldingFrameBufferNum : %lu, nDiBufferNum = %d",
          pCtx->mExtraOutBufferNum, nDiBufferNum);

    pCtx->mVideoConfig.nDeInterlaceHoldingFrameBufferNum = nDiBufferNum;
    if(pCtx->bUseZeroCopyOutBuffer)
    {
        pCtx->mVideoConfig.nDisplayHoldingFrameBufferNum \
                             = pCtx->mExtraOutBufferNum; //* ACodec decide the value
        omx_logd("****mExtraOutBufferNum:%lu", pCtx->mExtraOutBufferNum);
    }
    pCtx->mVideoConfig.nRotateHoldingFrameBufferNum \
                         = NUM_OF_PICTURES_KEEPPED_BY_ROTATE;

#ifdef CONF_DI_300_SUPPORT
    if (pCtx->mStreamInfo.nWidth < 1920 && pCtx->mStreamInfo.nHeight < 1080)
    {
        pCtx->mVideoConfig.nDecodeSmoothFrameBufferNum \
                         = NUM_OF_PICTURES_FOR_EXTRA_SMOOTH + 2;
    }
    else
    {
        pCtx->mVideoConfig.nDecodeSmoothFrameBufferNum \
                         = NUM_OF_PICTURES_FOR_EXTRA_SMOOTH;
    }
#else
    pCtx->mVideoConfig.nDecodeSmoothFrameBufferNum \
                         = NUM_OF_PICTURES_FOR_EXTRA_SMOOTH;
#endif
    if(pCtx->bIsSecureVideoFlag)
    {
        pCtx->mVideoConfig.nDisplayHoldingFrameBufferNum \
                         = DISPLAY_HOLH_BUFFER_NUM_DEFAULT;
            pCtx->mVideoConfig.nVbvBufferSize = SBM_SIZE_FOR_ZERO_COPY_MODE;
    }

    pCtx->mVideoConfig.memops = pCtx->decMemOps;

    pCtx->mStreamInfo.bIsFramePackage = 1;
    ret = InitializeVideoDecoder(pCtx->m_decoder,
                                 &(pCtx->mStreamInfo),
                                 &pCtx->mVideoConfig);
    if(ret != 0)
    {
        DestroyVideoDecoder(pCtx->m_decoder);
        pCtx->m_decoder           = NULL;
        pCtx->callback(pCtx->pUserData, AW_OMX_CB_EVENT_ERROR, NULL);
        omx_loge("Idle transition failed, set_vstream_info() return fail.\n");
        OmxReleaseMutex(pCtx->awMutexHandler);
        return ret;
    }
    pCtx->bHadInitDecoderFlag = OMX_TRUE;
    if(pCtx->bIsSecureVideoFlag)
    {
        pCtx->pSbmStart = (OMX_U8*)VideoStreamBufferAddress(pCtx->m_decoder, 0);
        pCtx->pSbmEnd = pCtx->pSbmStart + pCtx->mVideoConfig.nVbvBufferSize;
        pCtx->mSbmOffset = 0;
    }
    OmxReleaseMutex(pCtx->awMutexHandler);
    OmxTryPostSem(pCtx->mSemInData);
    OmxTryPostSem(pCtx->mSemOutBuffer);
    return ret;
}

static int __anSubmit(OmxDecoder* pDec,
                                 OMX_BUFFERHEADERTYPE* pInBufHdr)
{
    OmxAwDecoderContext *pCtx = (OmxAwDecoderContext*)pDec;
    //* if the size is 0, we should not copy it to decoder
    int require_size = pInBufHdr->nFilledLen;
    int maxVideoStreamBufferSize = VideoStreamBufferSize(pCtx->m_decoder, 0);
    if(!pCtx->nSpecificationFlag)
        checkVideoSpecification(pCtx,pInBufHdr);

    if(require_size > maxVideoStreamBufferSize || require_size <= 0)
    {
        loge("unexpected required buffer size with [%d]! maxVideoStreamBufferSize [%d].",
                require_size, maxVideoStreamBufferSize);
        return -1;
    }
    if(pCtx->bUseZeroCopyInBuffer)
        return anSubmitZeroCopy(pCtx, pInBufHdr);
    else
        return anSubmitCopy(pCtx, pInBufHdr);

}

static inline void __anDecode(OmxDecoder* pDec)
{
    int     decodeResult;
    OmxAwDecoderContext *pCtx = (OmxAwDecoderContext*)pDec;
#if ENABLE_STATISTICS_TIME
    int64_t nTimeUs1;
    int64_t nTimeUs2;
    nTimeUs1 = OmxGetNowUs();
#endif
    OmxAcquireMutex(pCtx->awMutexHandler);
    decodeResult = DecodeVideoStream(pCtx->m_decoder,0,0,0,0);
    omx_logv("*****decodeResult: %d", decodeResult);
    OmxReleaseMutex(pCtx->awMutexHandler);
#if ENABLE_STATISTICS_TIME
    nTimeUs2 = OmxGetNowUs();
    omx_logd("decode use time:%lld", nTimeUs2 - nTimeUs1);
#endif
    if(decodeResult == VDECODE_RESULT_KEYFRAME_DECODED ||
       decodeResult == VDECODE_RESULT_FRAME_DECODED ||
       decodeResult == VDECODE_RESULT_OK)
    {
        OmxTryPostSem(pCtx->mSemValidPic);
    }
    else if(decodeResult == VDECODE_RESULT_NO_FRAME_BUFFER)
    {
        OmxTimedWaitSem(pCtx->mSemOutBuffer, 2);
    }
    else if(decodeResult == VDECODE_RESULT_NO_BITSTREAM ||
            decodeResult == VDECODE_RESULT_CONTINUE)
    {
        if((pCtx->bInputEosFlag && pCtx->bHadGetVideoFbmBufInfoFlag && pCtx->bUseZeroCopyOutBuffer)
            || (pCtx->bInputEosFlag && (!pCtx->bUseZeroCopyOutBuffer)))
        {
            pCtx->bInputEosFlag = OMX_FALSE;

            //*set eos to decoder ,decoder should flush all fbm
            int mRet = 0;
            int mDecodeCount = 0;
            while(mRet != VDECODE_RESULT_NO_BITSTREAM)
            {
                OmxAcquireMutex(pCtx->awMutexHandler);
                mRet = DecodeVideoStream(pCtx->m_decoder,1,0,0,0);
                OmxReleaseMutex(pCtx->awMutexHandler);
                if(mRet == VDECODE_RESULT_RESOLUTION_CHANGE)
                    goto resolution_change;
                usleep(1*1000);
                mDecodeCount++;
                if(mDecodeCount > 1000)
                {
                    omx_logw(" decoder timeOut when set eos to decoder!");
                    break;
                }
            }
            pCtx->callback(pCtx->pUserData, AW_OMX_CB_NOTIFY_EOS, NULL);

        }
        else
        {
            pCtx->callback(pCtx->pUserData, AW_OMX_CB_CONTINUE_SUBMIT, NULL);
            OmxTimedWaitSem(pCtx->mSemInData, 2);
        }
    }
    else if(decodeResult == VDECODE_RESULT_RESOLUTION_CHANGE)
    {
    resolution_change:
        int validPicNum = ValidPictureNum(pCtx->m_decoder, 0);
        pCtx->callback(pCtx->pUserData, AW_OMX_CB_FLUSH_ALL_PIC, &validPicNum);
        anReopenVideoEngine(pCtx);
        pCtx->callback(pCtx->pUserData, AW_OMX_CB_FININSH_REOPEN_VE, NULL);
    }
    else if(decodeResult < 0)
    {
        omx_logw("decode fatal error[%d]", decodeResult);
        //* report a fatal error.
        pCtx->callback(pCtx->pUserData, AW_OMX_CB_EVENT_ERROR, NULL);
    }
    else
    {
        omx_logd("decode ret[%d], ignore? why?", decodeResult);
    }
    return ;
}

static OMX_BUFFERHEADERTYPE* __anDrain(OmxDecoder* pDec)
{
    OmxAwDecoderContext *pCtx = (OmxAwDecoderContext*)pDec;
    int validPicNum = ValidPictureNum(pCtx->m_decoder, 0);

    if(validPicNum <= 0)
    {
        //pCtx->callback(pCtx->pUserData, AW_OMX_CB_CONTINUE_SUBMIT, NULL);
        if(pCtx->bHadGetVideoFbmBufInfoFlag)
            OmxTimedWaitSem(pCtx->mSemValidPic, 2);
        return NULL;
    }
    if(pCtx->bUseZeroCopyOutBuffer)
        return anDrainZeroCopy(pCtx);
    else
        return anDrainCopy(pCtx);
}

static void __anStandbyBuffer(OmxDecoder* pDec)
{
    OmxAwDecoderContext *pCtx = (OmxAwDecoderContext*)pDec;
    if(pCtx->bUseZeroCopyOutBuffer)
    {
        pCtx->mNeedReSetToDecoderBufferNum = 0;
        pCtx->mSetToDecoderBufferNum = 0;
        return standbyOutBufferArr(pCtx);
    }
    else
        return ;
}

static int __anReturnBuffer(OmxDecoder* pDec)
{
    OmxAwDecoderContext *pCtx = (OmxAwDecoderContext*)pDec;
    if(pCtx->bUseZeroCopyOutBuffer)
    {
        if(!pCtx->bHadGetVideoFbmBufInfoFlag)
        {
            if(0 == anGetVideoFbmBufInfo(pCtx))
            {
                pCtx->bHadGetVideoFbmBufInfoFlag = OMX_TRUE;
                return 0;
            }
            else
            {
                pCtx->callback(pCtx->pUserData, AW_OMX_CB_CONTINUE_SUBMIT, NULL);
                if(pCtx->bInputEosFlag) //for cts and sts test
                {
                    pCtx->callback(pCtx->pUserData, AW_OMX_CB_NOTIFY_EOS, NULL);
                }
            }
            return -1;
        }
        return returnBufferToDecdoer(pCtx);
    }
    else
    {
        if (!pCtx->bHadGetVideoFbmBufInfoFlag)
        {
            if(GetVideoFbmBufInfo(pCtx->m_decoder) != NULL)
                pCtx->bHadGetVideoFbmBufInfoFlag = OMX_TRUE;
        }
        if(pCtx->pPicture != NULL)
        {
            OmxAcquireMutex(pCtx->awMutexHandler);
            ReturnPicture(pCtx->m_decoder, pCtx->pPicture);
            pCtx->pPicture = NULL;
            OmxReleaseMutex(pCtx->awMutexHandler);
            OmxTryPostSem(pCtx->mSemOutBuffer);
        }
        return 0;
    }

}

static inline void __anFlush(OmxDecoder* pDec, OMX_U32 portIdx)
{
    logd("decoder flush");
    OmxAwDecoderContext *pCtx = (OmxAwDecoderContext*)pDec;
    //* we should reset the mInputEosFlag when flush vdecoder
    pCtx->bInputEosFlag = OMX_FALSE;

    OmxTryPostSem(pCtx->mSemInData);
    OmxTryPostSem(pCtx->mSemOutBuffer);

    OmxAcquireMutex(pCtx->awMutexHandler);
    if (portIdx == kInputPortIndex || portIdx == OMX_ALL)
    {
        if(pCtx->bIsSecureVideoFlag && pCtx->bUseZeroCopyInBuffer)
        {
            OMX_BUFFERHEADERTYPE*   pInBufHdr = doRequestPortBuffer(pCtx->pInPort);
            if(pInBufHdr != NULL)
            {
                if(-1 == anSubmitZeroCopy(pCtx, pInBufHdr))//can not use while
                {
                    logw("SBM maybe full!!!");
                    judgeSbmOffset(pCtx, pInBufHdr->nFilledLen);
                }
                pCtx->callback(pCtx->pUserData, AW_OMX_CB_EMPTY_BUFFER_DONE,
                            (void*)pInBufHdr);
            }
        }
    }
    if (portIdx == kOutputPortIndex || portIdx == OMX_ALL)
    {
    if(pCtx->m_decoder)
    {
        ResetVideoDecoder(pCtx->m_decoder);
    }
    else
    {
            logw(" fatal error, m_decoder is not malloc when flush all ports!");
    }
        if(pCtx->bUseZeroCopyOutBuffer)
    {
        pCtx->pCurPicture    = NULL;
        pCtx->pPrePicture    = NULL;
        #if(DI_PROCESS_3_PICTURE)
        pCtx->pPrePrePicture = NULL;
        #endif
        memset(&pCtx->mDiOutBufferInfo, 0, sizeof(DiOutBufferInfo));

        pCtx->bFlushFlag = OMX_TRUE;
        standbyOutBufferArr(pCtx);
        pCtx->bFlushFlag = OMX_FALSE;
        pCtx->mNeedReSetToDecoderBufferNum += pCtx->mSetToDecoderBufferNum;
        pCtx->mSetToDecoderBufferNum = 0;
        SetVideoFbmBufRelease(pCtx->m_decoder);
    }
    }
    OmxReleaseMutex(pCtx->awMutexHandler);
}

static  void __anClose(OmxDecoder* pDec)
{
    omx_logd("decoder close");
    OmxAwDecoderContext *pCtx = (OmxAwDecoderContext*)pDec;
    OmxTryPostSem(pCtx->mSemInData);
    OmxTryPostSem(pCtx->mSemOutBuffer);
    OmxAcquireMutex(pCtx->awMutexHandler);

    if(pCtx->m_decoder != NULL)
    {
        DestroyVideoDecoder(pCtx->m_decoder);
        pCtx->m_decoder = NULL;
    }
    pCtx->mCodecSpecificDataLen = 0;
    memset(pCtx->mCodecSpecificData, 0 , CODEC_SPECIFIC_DATA_LENGTH);
    pCtx->mNeedReSetToDecoderBufferNum = 0;
    pCtx->bHadGetVideoFbmBufInfoFlag   = OMX_FALSE;
    pCtx->bHadInitDecoderFlag          = OMX_FALSE;
    OmxReleaseMutex(pCtx->awMutexHandler);
}

static void __anSetInputEos(OmxDecoder* pDec, OMX_BOOL bEos)
{
    OmxAwDecoderContext *pCtx = (OmxAwDecoderContext*)pDec;
    pCtx->bInputEosFlag = bEos;
}

static int __anSetOutputEos(OmxDecoder* pDec)
{
    //*set eos flag, MediaCodec use this flag
    // to determine whether a playback is finished.
    OmxAwDecoderContext *pCtx = (OmxAwDecoderContext*)pDec;
    int validPicNum = ValidPictureNum(pCtx->m_decoder, 0);
    if(validPicNum > 0)
    {
        omx_logd("**validPicNum:%d , when setOutPutEos!!", validPicNum);
        return -1;
    }
    OMX_U32 nValidSize = getPortValidSize(pCtx->pOutPort);
    omx_logv("*** OutBufList.nSizeOfList = %lu", nValidSize);

    if(nValidSize > 0)
    {
        while (getPortValidSize(pCtx->pOutPort) > 0)
        {
            OMX_BUFFERHEADERTYPE* pOutBufHdr = doRequestPortBuffer(pCtx->pOutPort);

            if(pOutBufHdr==NULL)
                continue;

            if (pOutBufHdr->nFilledLen != 0)
            {
                pCtx->callback(pCtx->pUserData, AW_OMX_CB_FILL_BUFFER_DONE,
                                (void*)pOutBufHdr);
                pOutBufHdr = NULL;
            }
            else
            {
                omx_logd("++++ set output eos(normal)");
                pOutBufHdr->nFlags |= OMX_BUFFERFLAG_EOS;
                pCtx->callback(pCtx->pUserData, AW_OMX_CB_FILL_BUFFER_DONE,
                               (void*)pOutBufHdr);
                pOutBufHdr = NULL;

                break;
            }
        }
    }
    else
    {
        OMX_U32 i = 0;
        OMX_U32 nAllocSize = getPortAllocSize(pCtx->pOutPort);
        for(i = 0; i<nAllocSize; i++)
        {
            if(pCtx->mOutputBufferInfoArr[i].mStatus == OWNED_BY_DECODER)
                break;
        }
        if(i == nAllocSize)
        {
            omx_logw("** have no buffer to set eos, try again");
            return -1;
        }
        omx_logd("*** set out eos(use buffer owned by decoder), pic = %p",
             pCtx->mOutputBufferInfoArr[i].pVideoPicture);
        OMX_BUFFERHEADERTYPE* pOutBufHdr =
               pCtx->mOutputBufferInfoArr[i].pHeadBufInfo;
        pOutBufHdr->nFilledLen = 0;
        pOutBufHdr->nFlags |= OMX_BUFFERFLAG_EOS;
        pCtx->callback(pCtx->pUserData, AW_OMX_CB_FILL_BUFFER_DONE,
                        (void*)pOutBufHdr);
        pCtx->mOutputBufferInfoArr[i].mStatus = OWNED_BY_RENDER;
        pCtx->mOutputBufferInfoArr[i].mUseDecoderBufferToSetEosFlag = OMX_TRUE;
    }
    return 0;
}

static void __anUpdateFormat(OmxDecoder* pDec)
{
    OmxAwDecoderContext *pCtx = (OmxAwDecoderContext*)pDec;
    if(pCtx->bUseAndroidBuffer)
    {
        OMX_U32 color;
        switch(pCtx->nActualColorFormat)
        {
            case PIXEL_FORMAT_YV12:
            {
                color = HAL_PIXEL_FORMAT_YV12;
                if(pCtx->nExtendFlag & AW_VIDEO_10BIT_FLAG)
                    color = HAL_PIXEL_FORMAT_AW_YV12_10bit;
                break;
            }
            case PIXEL_FORMAT_NV21:
            {
                color = HAL_PIXEL_FORMAT_YCrCb_420_SP;
                if(pCtx->nExtendFlag & AW_VIDEO_10BIT_FLAG)
                    color = HAL_PIXEL_FORMAT_AW_NV21_10bit;
                break;
            }
            case PIXEL_FORMAT_NV12:
            {
                color = HAL_PIXEL_FORMAT_AW_NV12;
                if(pCtx->nExtendFlag & AW_VIDEO_10BIT_FLAG)
                    color = HAL_PIXEL_FORMAT_AW_NV12_10bit;
                break;
            }
            case PIXEL_FORMAT_P010_UV:
            {
                color = HAL_PIXEL_FORMAT_AW_P010_UV;
                break;
            }
            case PIXEL_FORMAT_P010_VU:
            {
                color = HAL_PIXEL_FORMAT_AW_P010_VU;
                break;
            }
            default:
            {
                //*it will init the pixelformat when omx start,
                //*here you can change the format to init the decoder and gpu.
                pCtx->mVideoConfig.eOutputPixelFormat = OUTPORT_FORMAT_DEFAULT;//Used to be YV12.
                color = OUT_PICTURE_COLOR_DEFAULT;
                break;
            }
        }
        omx_logv("*****acutalcolor:%lx, update:%lx", pCtx->nActualColorFormat, color);
        setPortColorFormat(pCtx->pOutPort, color);
    }
}

static void __anSetExtBufNum(OmxDecoder* pDec, OMX_S32 num)
{
    OmxAwDecoderContext *pCtx = (OmxAwDecoderContext*)pDec;
    pCtx->mExtraOutBufferNum  = num;
}

static OMX_U8* __anAllocatePortBuffer(OmxDecoder* pDec, AwOmxVdecPort* port, OMX_U32 size)
{
    OmxAwDecoderContext *pCtx = (OmxAwDecoderContext*)pDec;
    if(isInputPort(port) && pCtx->bIsSecureVideoFlag)
    {
        OMX_U8* pInBuffer = NULL;
        int nTimeout = 0;
        if(pCtx->decMemOps == NULL)
        {
            pCtx->decMemOps = SecureMemAdapterGetOpsS();
            CdcMemOpen(pCtx->decMemOps);
        }
        while(!pCtx->bHadInitDecoderFlag && nTimeout < OMX_MAX_TIMEOUTS)
        {
            nTimeout++;
            omx_logd("wait decoder to open....");
            usleep(10*1000);
        }
        if(nTimeout == OMX_MAX_TIMEOUTS)
        {
            omx_loge("time out to wait decoder to open.");
            return NULL;
        }
        if(!pCtx->bUseZeroCopyInBuffer)
        {
#if(ADJUST_ADDRESS_FOR_SECURE_OS_OPTEE)
        pInBuffer = (OMX_U8*)VideoDecoderPallocIonBuf(pCtx->m_decoder, size);
#else
        char* pSecureBuf = (char *)VideoDecoderPallocIonBuf(pCtx->m_decoder, size);
        pInBuffer = (OMX_U8*)CdcMemGetPhysicAddressCpu(pCtx->decMemOps, pSecureBuf);
#endif
        }
        else
        {
            pInBuffer = pCtx->pSbmStart;
        }
        if(pCtx->bSurpporNativeHandle)
        {
            native_handle_t *nh = native_handle_create(0 /*numFds*/, 1 /*numInts*/);
            nh->data[0] = (int)(uintptr_t)pInBuffer;
            return (OMX_U8*)nh;
        }
        else
        {
            return pInBuffer;
        }
    }
    else
    {
        OMX_U8* pBuffer = (OMX_U8*)malloc(size);
        return pBuffer;
    }

}

static void __anFreePortBuffer(OmxDecoder* pDec, AwOmxVdecPort* port, OMX_U8* pBuffer)
{
    OmxAwDecoderContext *pCtx = (OmxAwDecoderContext*)pDec;


    if(isInputPort(port) && pCtx->bIsSecureVideoFlag)
    {
        OMX_U8* pInBuffer = NULL;
        if(pCtx->bSurpporNativeHandle)
        {
            native_handle_t *nh =(native_handle_t*)pBuffer;
            pInBuffer = (OMX_U8*)(uintptr_t)nh->data[0];

            native_handle_close(nh);

            native_handle_delete(nh);
        }
        else
            pInBuffer = pBuffer;
        if(!pCtx->bUseZeroCopyInBuffer)
        {
#if(ADJUST_ADDRESS_FOR_SECURE_OS_OPTEE)
        char*   pVirtAddr = (char*)pInBuffer;
#else
        OMX_U8* pPhyAddr  = pInBuffer;
        char*   pVirtAddr = (char*)CdcMemGetVirtualAddressCpu(pCtx->decMemOps, pPhyAddr);
#endif

        VideoDecoderFreeIonBuf(pCtx->m_decoder, pVirtAddr);
        }
    }
    else
    {
        free(pBuffer);
    }
    return ;
}

static OmxDecoderOpsT mAwDecoderOps =
{
    .getExtPara   =   __anGetExtPara,
    .setExtPara   =   __anSetExtPara,
    .getExtConfig =   __anGetExtConfig,
    .setExtConfig =   __anSetExtConfig,
    .prepare      =   __anPrepare,
    .close        =   __anClose,
    .flush        =   __anFlush,
    .decode       =   __anDecode,
    .submit       =   __anSubmit,
    .drain        =   __anDrain,
    .callback     =   __anCallback,
    .setInputEos  =   __anSetInputEos,
    .setOutputEos =   __anSetOutputEos,
    .standbyBuf   =   __anStandbyBuffer,
    .returnBuf    =   __anReturnBuffer,
    .updateFormat =   __anUpdateFormat,
    .setExtBufNum =   __anSetExtBufNum,
    .allocPortBuf =   __anAllocatePortBuffer,
    .freePortBuf  =   __anFreePortBuffer,
};


OmxDecoder* OmxDecoderCreate(AwOmxVdecPort* in, AwOmxVdecPort* out, OMX_BOOL bIsSecureVideoFlag)
{
    omx_logi("OmxDecoderCreate.");
    OmxAwDecoderContext* pCtx;
    pCtx = (OmxAwDecoderContext*)malloc(sizeof(OmxAwDecoderContext));
    if(pCtx == NULL)
    {
        omx_loge("malloc memory fail.");
        return NULL;
    }
    memset(pCtx, 0, sizeof(OmxAwDecoderContext));
    memset(&pCtx->mOutputBufferInfoArr, 0,
             sizeof(OMXOutputBufferInfoA)*OMX_MAX_VIDEO_BUFFER_NUM);
    pCtx->pInPort  = in;
    pCtx->pOutPort = out;
    OmxCreateMutex(&pCtx->awMutexHandler, OMX_FALSE);
    pCtx->mSemInData    = OmxCreateSem("InDataSem",    0, 0, OMX_FALSE);
    pCtx->mSemOutBuffer = OmxCreateSem("OutBufferSem", 0, 0, OMX_FALSE);
    pCtx->mSemValidPic  = OmxCreateSem("ValidPicSem",  0, 0, OMX_FALSE);
    pCtx->nPreSubmitPts = -1;
    pCtx->nSpecificationFlag = 0;
    if(bIsSecureVideoFlag)
    {
        pCtx->decMemOps = SecureMemAdapterGetOpsS();
        //pCtx->bUseZeroCopyInBuffer = OMX_TRUE;//must sync to drm plugin
        pCtx->bUseZeroCopyInBuffer = OMX_FALSE;
        pCtx->mGpuAlignStride      = 64; //for g2d bug
        pCtx->bIsSecureVideoFlag   = OMX_TRUE;
    }
    else
    {
        pCtx->decMemOps = MemAdapterGetOpsS();
        pCtx->bUseZeroCopyInBuffer = OMX_FALSE;
        pCtx->mGpuAlignStride      = GPU_ALIGN_STRIDE;
        pCtx->bIsSecureVideoFlag   = OMX_FALSE;
    }
    CdcMemOpen(pCtx->decMemOps);

#ifndef CONF_SIMPLE_ION
    pCtx->mIonFd = -1;
    pCtx->mIonFd = ion_open();

    omx_logd("ion open fd = %d",pCtx->mIonFd);
    if(pCtx->mIonFd < -1)
    {
        omx_loge("ion open fail ! ");
        return NULL;
    }
#endif
    pCtx->bSubmit  = OMX_TRUE;
    #ifdef CONF_ENABLE_OPENMAX_DI_FUNCTION
    pCtx->mEnableDiFlag = -1;
    pCtx->mDi = DeinterlaceCreate_Omx();
    if(pCtx->mDi == NULL)
    {
        omx_loge("deinterlaceCreate_omx failed");
    }
    #endif

    pCtx->base.ops = &mAwDecoderOps;
    return (OmxDecoder*)&pCtx->base;
}

void OmxDestroyDecoder(OmxDecoder* pDec)
{
    if(pDec == NULL)
    {
        omx_logw("pdec is null");
        return;
    }
    OmxAwDecoderContext *pCtx = (OmxAwDecoderContext*)pDec;

    if(pCtx->m_decoder != NULL)
    {
        DestroyVideoDecoder(pCtx->m_decoder);
        pCtx->m_decoder = NULL;
    }

    OmxDestroyMutex(&pCtx->awMutexHandler);
    OmxDestroySem(&pCtx->mSemInData);
    OmxDestroySem(&pCtx->mSemOutBuffer);
    OmxDestroySem(&pCtx->mSemValidPic);
    if(pCtx->mStreamInfo.pCodecSpecificData)
        free(pCtx->mStreamInfo.pCodecSpecificData);

    freeOutputBufferInfoArr(pCtx);
#ifndef CONF_SIMPLE_ION
    if(pCtx->mIonFd > 0)
    {
        ion_close(pCtx->mIonFd);
    }
#endif
    if(pCtx->decMemOps)
    {
        CdcMemClose(pCtx->decMemOps);
    }
    if(pCtx->mDi)
    {
        CdcDiDestroy(pCtx->mDi);
    }
    free(pCtx);
    pCtx = NULL;
}
