 /*
  * =====================================================================================
  *
  *      Copyright (c) 2008-2018 Allwinner Technology Co. Ltd.
  *      All rights reserved.
  *
  *       Filename:  omx_venc_linux_adapter.c
  *
  *    Description:
  *
  *        Version:  1.0
  *        Created:  2018/08
  *       Revision:  none
  *       Compiler:  gcc
  *
  *         Author:  SWC-PPD
  *      Company: Allwinner Technology Co. Ltd.
  *
  *       History :
  *        Author : AL3
  *        Date   : 2013/05/05
  *    Comment : complete the design code
  *
  * =====================================================================================
  */

#include "omx_venc_adapter.h"
OMX_ERRORTYPE getDefaultParameter(AwOmxVenc* impl, OMX_IN OMX_INDEXTYPE eParamIndex,
                                           OMX_IN OMX_PTR pParamData)
{
    CEDARC_UNUSE(impl);
    CEDARC_UNUSE(eParamIndex);
    CEDARC_UNUSE(pParamData);
    loge("getDefaultParameter. UnsupportedIndex %d. unkonwn param %p\n", eParamIndex, pParamData);
    OMX_ERRORTYPE eError = OMX_ErrorUnsupportedIndex;
    return eError;
}
OMX_ERRORTYPE setDefaultParameter(AwOmxVenc* impl, OMX_IN OMX_INDEXTYPE eParamIndex,
                                           OMX_IN OMX_PTR pParamData)
{
    CEDARC_UNUSE(impl);
    CEDARC_UNUSE(eParamIndex);
    CEDARC_UNUSE(pParamData);
    loge("setDefaultParameter. UnsupportedIndex %d. unkonwn param %p\n", eParamIndex, pParamData);
    OMX_ERRORTYPE eError = OMX_ErrorUnsupportedIndex;
    return eError;
}

void getAndAddInputBuffer(AwOmxVenc* impl, OMX_BUFFERHEADERTYPE* pInBufHdr,
                                    VencInputBuffer* sInputBuffer)
{
    CEDARC_UNUSE(impl);
    CEDARC_UNUSE(pInBufHdr);
    CEDARC_UNUSE(sInputBuffer);
    loge("getAndAddInputBuffer: do not support metadata input buffer");
}

void getInputBufferFromBufHdr (AwOmxVenc* impl, OMX_BUFFERHEADERTYPE* pInBufHdr,
                                                                VencInputBuffer* sInputBuffer)
{
    logd("function %s. line %d.", __FUNCTION__, __LINE__);
    if (impl->mVideoExtParams.bEnableCropping)
    {
        if (impl->mVideoExtParams.ui16CropLeft ||
            impl->mVideoExtParams.ui16CropTop)
        {
            sInputBuffer->bEnableCorp = 1;
            sInputBuffer->sCropInfo.nLeft =
                impl->mVideoExtParams.ui16CropLeft;
            sInputBuffer->sCropInfo.nWidth =
                impl->m_sOutPortDefType.format.video.nFrameWidth -
                impl->mVideoExtParams.ui16CropLeft -
                impl->mVideoExtParams.ui16CropRight;
            sInputBuffer->sCropInfo.nTop =
                impl->mVideoExtParams.ui16CropTop;
            sInputBuffer->sCropInfo.nHeight =
                impl->m_sOutPortDefType.format.video.nFrameHeight -
                impl->mVideoExtParams.ui16CropTop -
                impl->mVideoExtParams.ui16CropBottom;
        }
    }
    memcpy(sInputBuffer->pAddrVirY,
           pInBufHdr->pBuffer + pInBufHdr->nOffset, impl->mSizeY);
    memcpy(sInputBuffer->pAddrVirC,
           pInBufHdr->pBuffer + pInBufHdr->nOffset + impl->mSizeY, impl->mSizeC);
    impl->mSizeY = 0;
    impl->mSizeC = 0;

}

void getInputBufferFromBufHdrWithoutCrop(AwOmxVenc* impl, OMX_BUFFERHEADERTYPE* pInBufHdr,
                                                                VencInputBuffer* sInputBuffer)
{
    logd("function %s. line %d.", __FUNCTION__, __LINE__);
    memcpy(sInputBuffer->pAddrVirY,
           pInBufHdr->pBuffer + pInBufHdr->nOffset, impl->mSizeY);
    memcpy(sInputBuffer->pAddrVirC,
           pInBufHdr->pBuffer + pInBufHdr->nOffset + impl->mSizeY, impl->mSizeC);
    impl->mSizeY = 0;
    impl->mSizeC = 0;
}

void determineVencColorFormat(AwOmxVenc* impl)
{
    switch (impl->m_sInPortFormatType.eColorFormat)
    {
        case OMX_COLOR_FormatYUV420SemiPlanar:
        {
            logv("color format: VENC_PIXEL_YUV420SP");
            impl->m_vencColorFormat = VENC_PIXEL_YUV420SP;
            break;
        }
        case OMX_COLOR_FormatYVU420Planar:
        {
            logv("color format: VENC_PIXEL_YVU420P");
            impl->m_vencColorFormat = VENC_PIXEL_YVU420P;
            break;
        }
        default:
        {
            logw("determine none format!!!!!!!!!!!");
            break;
        }

    }
}

void determineVencBufStrideIfNecessary(AwOmxVenc* impl, VencBaseConfig* pBaseConfig)
{
    CEDARC_UNUSE(impl);
    CEDARC_UNUSE(pBaseConfig);
    //do nothing.
}

int deparseEncInputBuffer(int nIonFd,
                                VideoEncoder *pEncoder,
                                OMXInputBufferInfoT *pInputBufInfo)
{
    OMX_S32 i;
    int ret;
    for(i=0; i<NUM_MAX_IN_BUFFERS; i++)
    {
        OMXInputBufferInfoT *p;
        p = pInputBufInfo + i;

        if(p->nAwBufId != -1)
        {
            if(CdcIonGetMemType() == MEMORY_IOMMU)
            {
                struct user_iommu_param sIommuBuf;
                sIommuBuf.fd = p->nBufFd;
                VideoEncoderFreeVeIommuAddr(pEncoder, &sIommuBuf);
             }

            //close buf fd
            if(p->nBufFd != -1)
            {
                ret = CdcIonClose(p->nBufFd);
                if(ret < 0)
                {
                    loge("CdcIonClose close buf fd error\n");
                    return -1;
                }
            }

            //free ion handle
            if(p->handle_ion)
            {
                ret = CdcIonFree(nIonFd, p->handle_ion);
                if(ret < 0)
                {
                    loge("CdcIonFree free ion_handle error\n");
                    return -1;
                }
            }

            p->nShareFd = -1;
        }
    }
    return 0;
}

void updateOmxDebugFlag(AwOmxVenc* impl)
{
    impl->bSaveStreamFlag = OMX_FALSE;
    impl->bOpenStatisticFlag = OMX_FALSE;
    impl->bShowFrameSizeFlag = OMX_FALSE;
    #if SAVE_BITSTREAM
    impl->bSaveStreamFlag = OMX_TRUE;
    #endif
    #if OPEN_STATISTICS
    impl->bOpenStatisticFlag = OMX_TRUE;
    #endif
    #if PRINTF_FRAME_SIZE
    impl->bShowFrameSizeFlag = OMX_TRUE;
    #endif
}

