/*
 * =====================================================================================
 *
 *      Copyright (c) 2008-2018 Allwinner Technology Co. Ltd.
 *      All rights reserved.
 *
 *       Filename:  omx_venc_android.cpp
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
#define LOG_TAG "omx_venc"
#include "MetadataBufferType.h"
#include <ion/ion.h>
#include <HardwareAPI.h>
#include <utils/CallStack.h>
#include <binder/IPCThreadState.h>
#include <ui/Rect.h>
#include <ui/GraphicBufferMapper.h>
#include <ui/GraphicBuffer.h>
#include <ui/Fence.h>
#include <cutils/properties.h>

#ifdef GPU_PUBLIC_INCLUDE
#include GPU_PUBLIC_INCLUDE
#else
#include <hal_public.h>
#endif

#ifdef CONF_MARSHMALLOW_AND_NEWER
#include <OMX_IndexExt.h>
#endif
#include "omx_venc_adapter.h"
#include "vdecoder.h"

#include <HardwareAPI.h>

using namespace android;

#define FENCE_TIMEOUT_MS  1000
#define ALIGN_16B(x) (((x) + (15)) & ~(15))
#define SAVE_PICTURE (0)

#ifdef CONF_NOUGAT_AND_NEWER
#ifdef CONF_ARMV7_A_NEON

extern "C" void ImgRGBA2YUV420SP_neon(unsigned char *pu8RgbBuffer,
                                      unsigned char** pu8SrcYUV,
                                      int *l32Width_stride,
                                      int l32Height);
#endif
#endif

int parseEncInputBuffer(AwOmxVenc *impl,
                                      long long aw_buf_id,
                                      int share_fd,
                                      unsigned long *phy_addr)
{
    int i;
#ifndef CONF_SIMPLE_ION
    int ret;
    aw_ion_user_handle_t ion_handle;
#endif

    for(i=0; i<NUM_MAX_IN_BUFFERS; i++)
    {
        //parse a new share fd
        if(-1 == impl->mInputBufInfo[i].nAwBufId)
        {
            impl->mInputBufInfo[i].nAwBufId = aw_buf_id;
#ifndef CONF_SIMPLE_ION
            //get ion buffer handle_ion
            ret = CdcIonImport(impl->mIonFd, share_fd, &ion_handle);
            if(ret < 0)
            {
                omx_loge("use CdcIonImport get ion_handle error\n");
                return -1;
            }
            impl->mInputBufInfo[i].handle_ion = ion_handle;
            impl->mInputBufInfo[i].nShareFd = share_fd;

            //get ion buffer fd
            ret = CdcIonMap(impl->mIonFd, (uintptr_t)ion_handle);
            if(ret < 0)
            {
                omx_loge("use CdcIonImport get ion_handle error\n");
                return -1;
            }
#endif
#ifdef CONF_SIMPLE_ION
            impl->mInputBufInfo[i].nBufFd = share_fd;
#else
            impl->mInputBufInfo[i].nBufFd = ret;
#endif

            //get phy address
            if(CdcIonGetMemType() == MEMORY_IOMMU)
            {
                user_iommu_param sIommuBuf;
                sIommuBuf.fd = impl->mInputBufInfo[i].nBufFd;
                VideoEncoderGetVeIommuAddr(impl->m_encoder, &sIommuBuf);
                impl->mInputBufInfo[i].nBufPhyAddr = (unsigned long)sIommuBuf.iommu_addr;
            }
            else
            {
#ifdef CONF_SIMPLE_ION
                //TODO, probably never since IOMMU is always used in android R and above
#else
                impl->mInputBufInfo[i].nBufPhyAddr = CdcIonGetPhyAddr(impl->mIonFd,
                                                        (uintptr_t)ion_handle);
#endif
            }
            break;
        }
        //get already parsed share fd's index
        else if(aw_buf_id == impl->mInputBufInfo[i].nAwBufId)
        {
            break;
        }
    }
    if(NUM_MAX_IN_BUFFERS == i)
    {
        omx_loge("the omx_venc inputBuffer num is bigger than NUM_MAX_IN_BUFFERS[%d]\n",
                                                    NUM_MAX_IN_BUFFERS);
        return -1;
    }

    *phy_addr = impl->mInputBufInfo[i].nBufPhyAddr;
    omx_logv("mInputBufInfo[%d], nShareFd:%d, nBufPhyAddr:%lx, handle:%lx, nBufFd:%d\n",
                                            i,
                                            impl->mInputBufInfo[i].nShareFd,
                                            impl->mInputBufInfo[i].nBufPhyAddr,
                                            (long)impl->mInputBufInfo[i].handle_ion,
                                            impl->mInputBufInfo[i].nBufFd);
    return 0;
}

int deparseEncInputBuffer(int nIonFd,
                                            VideoEncoder *pEncoder,
                                            OMXInputBufferInfoT *pInputBufInfo)
{
    (void)nIonFd;
    OMX_S32 i;
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

#ifdef CONF_SIMPLE_ION
            //No need to free fd and unmap.
#else
            int ret;
            //close buf fd
            if(p->nBufFd != -1)
            {
                ret = CdcIonClose(p->nBufFd);
                if(ret < 0)
                {
                    omx_loge("CdcIonClose close buf fd error\n");
                    return -1;
                }
            }

            //free ion handle
            if(p->handle_ion)
            {
                ret = CdcIonFree(nIonFd, p->handle_ion);
                if(ret < 0)
                {
                    omx_loge("CdcIonFree free ion_handle error\n");
                    return -1;
                }
            }
#endif
            p->nShareFd = -1;
        }
    }
    return 0;
}

//Since the CustomParams are set by setDeafaultParameter in Android case,
// they would be got only in Android case.
//TODO: figure out in linux case whether the custom params are required!!!
OMX_ERRORTYPE getDefaultParameter(AwOmxVenc* impl, OMX_IN OMX_INDEXTYPE eParamIndex,
                                           OMX_IN OMX_PTR pParamData)
{
    omx_logv("function %s. line %d.", __FUNCTION__, __LINE__);
    OMX_ERRORTYPE eError = OMX_ErrorNone;
#ifdef CONF_MARSHMALLOW_AND_NEWER
    if((OMX_INDEXEXTTYPE)eParamIndex == OMX_IndexParamConsumerUsageBits)
    {
        omx_logd("get_parameter: OMX_IndexParamConsumerUsageBits");
        OMX_U32 *usageBits = (OMX_U32 *)pParamData;
        *usageBits = GRALLOC_USAGE_HW_2D;
        return eError;
    }
#endif

    switch ((VIDENC_CUSTOM_INDEX)eParamIndex)
    {
        case VideoEncodeCustomParamextendedVideo:
        {
            omx_logd("get_parameter: VideoEncodeCustomParamextendedVideo");
            memcpy(pParamData,&impl->mVideoExtParams,  sizeof(OMX_VIDEO_PARAMS_EXTENDED));
            break;
        }
        case VideoEncodeCustomParamextendedVideoSuperframe:
        {
            omx_logd("get_parameter: VideoEncodeCustomParamextendedVideoSuperframe");
            memcpy(pParamData,&impl->mVideoSuperFrame,  sizeof(OMX_VIDEO_PARAMS_SUPER_FRAME));
            break;
        }
        case VideoEncodeCustomParamextendedVideoSVCSkip:
        {
            omx_logd("get_parameter: VideoEncodeCustomParamextendedVideoSVCSkip");
            memcpy(pParamData, &impl->mVideoSVCSkip,  sizeof(OMX_VIDEO_PARAMS_SVC));
            break;
        }
        case VideoEncodeCustomParamextendedVideoVBR:
        {
            omx_logd("get_parameter: VideoEncodeCustomParamextendedVideoVBR\n");
            memcpy(pParamData, &impl->mVideoVBR, sizeof(OMX_VIDEO_PARAMS_VBR));
            break;
        }
        case VideoEncodeCustomParamStoreANWBufferInMetadata:
        {
            break;
        }
        case VideoEncodeCustomParamextendedVideoPSkip:
        {
            omx_logd("get_parameter: VideoEncodeCustomParamextendedVideoPSkip\n");
            break;
        }
        default:
        {
            switch ((OMX_INDEXEXTTYPE)eParamIndex)
            {
                case OMX_IndexParamVideoHevc:
                {
                    omx_logv(" get_parameter: OMX_IndexParamVideoHevc");
                    OMX_VIDEO_PARAM_HEVCTYPE* pComponentParam
                                            = (OMX_VIDEO_PARAM_HEVCTYPE*)pParamData;

                    if (pComponentParam->nPortIndex == impl->m_sH265Type.nPortIndex)
                    {
                        memcpy(pComponentParam,
                                &impl->m_sH265Type, sizeof(OMX_VIDEO_PARAM_HEVCTYPE));
                    }
                    else
                    {
                        eError = OMX_ErrorBadPortIndex;
                    }
                    break;
                }
                default:
                {
                    omx_loge("get_parameter: unknown param %p\n", pParamData);
                    eError = OMX_ErrorUnsupportedIndex;
                    break;
                }
            }
        }
        break;
    }
    return eError;
}

#ifdef CONF_PIE_AND_NEWER
static void configColorAspects(VencH264VideoSignal *pVencH264VideoSignal,
                                     int32_t mTransferCharacteristics,
                                     int32_t mMatrixCoeffs,
                                     int32_t mVideoFullRange,
                                     int32_t mPrimaries)
{
    switch (mTransferCharacteristics)
    {
        case ColorAspects::TransferGamma22:
            pVencH264VideoSignal->transfer_characteristics = VIDEO_TRANSFER_GAMMA2_2;
            break;
        case ColorAspects::TransferGamma28:
            pVencH264VideoSignal->transfer_characteristics = VIDEO_TRANSFER_GAMMA2_8;
            break;
        case ColorAspects::TransferSMPTE170M:
            pVencH264VideoSignal->transfer_characteristics = VIDEO_TRANSFER_SMPTE_170M;
            break;
        case ColorAspects::TransferSMPTE240M:
            pVencH264VideoSignal->transfer_characteristics = VIDEO_TRANSFER_SMPTE_240M;
            break;
        case ColorAspects::TransferLinear:
            pVencH264VideoSignal->transfer_characteristics = VIDEO_TRANSFER_LINEAR;
            break;
        case ColorAspects::TransferBT1361:
            pVencH264VideoSignal->transfer_characteristics = VIDEO_TRANSFER_BT1361;
            break;
        case ColorAspects::TransferSRGB:
            pVencH264VideoSignal->transfer_characteristics = VIDEO_TRANSFER_SRGB;
            break;
        case ColorAspects::TransferST2084:
            pVencH264VideoSignal->transfer_characteristics = VIDEO_TRANSFER_ST2084;
            break;
        case ColorAspects::TransferST428:
            pVencH264VideoSignal->transfer_characteristics = VIDEO_TRANSFER_ST428_1;
            break;
        case ColorAspects::TransferHLG://*7
            pVencH264VideoSignal->transfer_characteristics = VIDEO_TRANSFER_HLG;
            break;
        case ColorAspects::TransferXvYCC://*0x41
            pVencH264VideoSignal->transfer_characteristics = VIDEO_TRANSFER_IEC61966;
            break;
        case ColorAspects::TransferOther:
            pVencH264VideoSignal->transfer_characteristics = VIDEO_TRANSFER_BT1361_EXTENDED;
            break;
        case ColorAspects::TransferUnspecified:
            pVencH264VideoSignal->transfer_characteristics =  VIDEO_TRANSFER_UNSPECIFIED;
            break;
        default:
            pVencH264VideoSignal->transfer_characteristics =  VIDEO_TRANSFER_UNSPECIFIED;
            break;
    }
    switch (mMatrixCoeffs)
    {
        case ColorAspects::MatrixBT709_5://*1
            pVencH264VideoSignal->matrix_coefficients = VIDEO_MATRIX_COEFFS_BT709;
            break;
        case ColorAspects::MatrixBT470_6M://*2
            pVencH264VideoSignal->matrix_coefficients = VIDEO_MATRIX_COEFFS_BT470M;
            break;
        case ColorAspects::MatrixBT601_6://*3
            pVencH264VideoSignal->matrix_coefficients = VIDEO_MATRIX_COEFFS_BT601_625_0;
            break;
        case ColorAspects::MatrixSMPTE240M://*4
            pVencH264VideoSignal->matrix_coefficients = VIDEO_MATRIX_COEFFS_SMPTE_240M;
            break;
        case ColorAspects::MatrixBT2020://*5
            pVencH264VideoSignal->matrix_coefficients = VIDEO_MATRIX_COEFFS_BT2020;
            break;
        case ColorAspects::ColorAspects::MatrixBT2020Constant://*6
            pVencH264VideoSignal->matrix_coefficients = VIDEO_MATRIX_COEFFS_BT2020_CONSTANT_LUMINANCE;
            break;
        case ColorAspects::MatrixOther://*0xff
            pVencH264VideoSignal->matrix_coefficients = VIDEO_MATRIX_COEFFS_SOMPATE;
            break;
        case ColorAspects::MatrixUnspecified://*0
            pVencH264VideoSignal->matrix_coefficients = VIDEO_MATRIX_COEFFS_UNSPECIFIED_0;
            break;
        default:
        {
            pVencH264VideoSignal->matrix_coefficients = VIDEO_MATRIX_COEFFS_UNSPECIFIED_0;
            break;
        }
    }
    switch (mVideoFullRange)
    {
        case ColorAspects::RangeLimited://*2
            pVencH264VideoSignal->full_range_flag = 0;
            break;
        case ColorAspects::RangeFull://*1
            pVencH264VideoSignal->full_range_flag = 1;
            break;
        default:
        {
            pVencH264VideoSignal->full_range_flag = 0;
            omx_loge("should not be here, mVideoFullRange = %d", mVideoFullRange);
            break;
        }
    }
    switch(mPrimaries)//from ffmpeg
    {
        case ColorAspects::PrimariesBT709_5://*1
            pVencH264VideoSignal->dst_colour_primaries = (VENC_COLOR_SPACE)1;
            break;
        case ColorAspects::PrimariesBT470_6M://*2
            pVencH264VideoSignal->dst_colour_primaries = (VENC_COLOR_SPACE)4;
            break;
        case ColorAspects::PrimariesBT601_6_625://*3
            pVencH264VideoSignal->dst_colour_primaries = (VENC_COLOR_SPACE)5;
            break;
        case ColorAspects::PrimariesBT601_6_525://*4
            pVencH264VideoSignal->dst_colour_primaries = (VENC_COLOR_SPACE)6;
            break;
        case ColorAspects::PrimariesOther://*0xff
            pVencH264VideoSignal->dst_colour_primaries = (VENC_COLOR_SPACE)7;
            break;
        case ColorAspects::PrimariesGenericFilm://*5
            pVencH264VideoSignal->dst_colour_primaries = (VENC_COLOR_SPACE)8;
            break;
        case ColorAspects::PrimariesBT2020://*6
            pVencH264VideoSignal->dst_colour_primaries = (VENC_COLOR_SPACE)9;
            break;
        case ColorAspects::PrimariesUnspecified://*0
        default:
        {
            pVencH264VideoSignal->dst_colour_primaries = (VENC_COLOR_SPACE)0;
            break;
        }
    }
    return ;
}
#endif

OMX_ERRORTYPE setDefaultConfig(AwOmxVenc* impl, OMX_IN OMX_INDEXTYPE eParamIndex,
                                           OMX_IN OMX_PTR pParamData)
{
     OMX_ERRORTYPE eError = OMX_ErrorNone;
     omx_logv("function %s. line %d. param index %d.", __FUNCTION__, __LINE__, (int)eParamIndex);
     switch ((VIDENC_CUSTOM_INDEX)eParamIndex)
     {
#ifdef CONF_PIE_AND_NEWER
             case VideoEncodeCustomParamdescribeColorAspects:
             {
                   const DescribeColorAspectsParams* colorAspectsParams =
                             (const DescribeColorAspectsParams *)pParamData;

                   ColorAspects     mColorAspects;
                   mColorAspects = colorAspectsParams->sAspects;
                   omx_logd("set ColorAspects: R:%d, P:%d, M:%d, T:%d",
                   colorAspectsParams->sAspects.mRange,
                   colorAspectsParams->sAspects.mPrimaries,
                   colorAspectsParams->sAspects.mMatrixCoeffs,
                   colorAspectsParams->sAspects.mTransfer);

                   impl->bAspectEnable = 1;
                   memset(&impl->mVencH264VideoSignal, 0, sizeof(VencH264VideoSignal));
                   impl->mVencH264VideoSignal.video_format = DEFAULT;
                   configColorAspects(&impl->mVencH264VideoSignal,
                       colorAspectsParams->sAspects.mTransfer,
                       colorAspectsParams->sAspects.mMatrixCoeffs,
                       colorAspectsParams->sAspects.mRange,
                       colorAspectsParams->sAspects.mPrimaries);
                   impl->mVencH264VideoSignal.src_colour_primaries = \
                       impl->mVencH264VideoSignal.dst_colour_primaries;

                   omx_logd("set to encoder ColorAspects: R:%d, P:%d, M:%d, T:%d",
                   impl->mVencH264VideoSignal.full_range_flag,
                   impl->mVencH264VideoSignal.dst_colour_primaries,
                   impl->mVencH264VideoSignal.matrix_coefficients,
                   impl->mVencH264VideoSignal.transfer_characteristics);
#if 0
                   impl->mVencH264VideoSignal.full_range_flag = colorAspectsParams->sAspects.mRange;
                   impl->mVencH264VideoSignal.src_colour_primaries = \
                                (VENC_COLOR_SPACE)colorAspectsParams->sAspects.mPrimaries;
                   impl->mVencH264VideoSignal.dst_colour_primaries = \
                                (VENC_COLOR_SPACE)colorAspectsParams->sAspects.mPrimaries;
                   impl->mVencH264VideoSignal.transfer_characteristics = \
                                colorAspectsParams->sAspects.mTransfer;
                   impl->mVencH264VideoSignal.matrix_coefficients = \
                                (VENC_COLOR_SPACE)colorAspectsParams->sAspects.mMatrixCoeffs;
#endif
                   break;
             }
#endif
             default:
            {
                 omx_logd("get_config: unknown param %d\n",eParamIndex);
                 eError = OMX_ErrorUnsupportedIndex;
                 break;
            }
       }
       return eError;
}

OMX_ERRORTYPE setDefaultParameter(AwOmxVenc* impl, OMX_IN OMX_INDEXTYPE eParamIndex,
                                           OMX_IN OMX_PTR pParamData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    omx_logv("function %s. line %d. param index %d.", __FUNCTION__, __LINE__, (int)eParamIndex);
    switch ((VIDENC_CUSTOM_INDEX)eParamIndex)
    {
        case VideoEncodeCustomParamStoreMetaDataInBuffers:
        {
            OMX_BOOL bFlag = ((StoreMetaDataInBuffersParams*)pParamData)->bStoreMetaData;;
            if (((StoreMetaDataInBuffersParams*)pParamData)->nPortIndex == 0)
            {
                impl->m_useMetaDataInBuffers = bFlag;
            }
            else
            {
                if (bFlag)
                {
                    eError = OMX_ErrorUnsupportedSetting;
                }
            }
            omx_logd("set_parameter: VideoEncodeCustomParamStoreMetaDataInBuffers %d",
                 impl->m_useMetaDataInBuffers);
            break;
        }

        case VideoEncodeCustomParamPrependSPSPPSToIDRFrames:
        {
            impl->m_prependSPSPPSToIDRFrames =
                ((PrependSPSPPSToIDRFramesParams*)pParamData)->bEnable;
            omx_logd("set_parameter: VideoEncodeCustomParamPrependSPSPPSToIDRFrames %d",
                 impl->m_prependSPSPPSToIDRFrames);
            break;
        }

        case VideoEncodeCustomParamEnableAndroidNativeBuffers:
        {
            OMX_BOOL bFlag = ((EnableAndroidNativeBuffersParams*)pParamData)->enable;
            OMX_U32  index = ((EnableAndroidNativeBuffersParams*)pParamData)->nPortIndex;

            omx_logd("set_parameter: VideoEncodeCustomParamEnableAndroidNativeBuffers, \
                 nPortIndex: %d,enable:%d", (int)index, (int)bFlag);
            if(index == 0)
                impl->m_useAndroidBuffer = bFlag;

            break;
        }
        case VideoEncodeCustomParamextendedVideo:
        {
            omx_logd("set_parameter: VideoEncodeCustomParamextendedVideo");
            memcpy(&impl->mVideoExtParams, pParamData, sizeof(OMX_VIDEO_PARAMS_EXTENDED));
            break;
        }
        case VideoEncodeCustomParamextendedVideoSuperframe:
        {
            omx_logd("set VideoEncodeCustomParamextendedVideoSuperframe");
            memcpy(&impl->mVideoSuperFrame, pParamData, sizeof(OMX_VIDEO_PARAMS_SUPER_FRAME));
            break;
        }
        case VideoEncodeCustomParamextendedVideoSVCSkip:
        {
            omx_logd("set_parameter: VideoEncodeCustomParamextendedVideoSVCSkip\n");
            memcpy(&impl->mVideoSVCSkip, pParamData, sizeof(OMX_VIDEO_PARAMS_SVC));
            break;
        }
        case VideoEncodeCustomParamextendedVideoVBR:
        {
            omx_logd("set_parameter: VideoEncodeCustomParamextendedVideoVBR\n");
            memcpy(&impl->mVideoVBR, pParamData, sizeof(OMX_VIDEO_PARAMS_VBR));
            break;
        }
        case VideoEncodeCustomParamStoreANWBufferInMetadata:
        {
            OMX_BOOL bFlag = ((StoreMetaDataInBuffersParams*)pParamData)->bStoreMetaData;
            OMX_U32  index = ((StoreMetaDataInBuffersParams*)pParamData)->nPortIndex;
            omx_logd("set_parameter: VideoEncodeCustomParamStoreANWBufferInMetadata, flag: %d", bFlag);
            if(index == 0)
                impl->m_useAndroidBuffer = bFlag;
            break;
        }
        case VideoEncodeCustomParamextendedVideoPSkip:
        {
            omx_logd("set_parameter: VideoEncodeCustomParamextendedVideoPSkip\n");
            impl->m_usePSkip = (*(OMX_BOOL *)pParamData);
            break;
        }
        default:
        {
            switch ((OMX_INDEXEXTTYPE)eParamIndex)
            {
                case OMX_IndexParamVideoHevc:
                {
                    omx_logv(" set_parameter: OMX_IndexParamVideoHevc");
                    OMX_VIDEO_PARAM_HEVCTYPE* pComponentParam
                                            = (OMX_VIDEO_PARAM_HEVCTYPE*)pParamData;

                    if (pComponentParam->nPortIndex == impl->m_sH265Type.nPortIndex)
                    {
                        memcpy(&impl->m_sH265Type,
                                pComponentParam, sizeof(OMX_VIDEO_PARAM_HEVCTYPE));
                        //CalculateBufferSize(pCompPortOut->pPortDef, pComponentPrivate);
                    }
                    else
                    {
                        eError = OMX_ErrorBadPortIndex;
                    }
                    break;
                }
                default:
                {
                    omx_loge("set_parameter: unknown param %x\n", eParamIndex);
                    eError = OMX_ErrorUnsupportedIndex;
                    break;
                }
            }
        }
    }
    return eError;
}

void getAndAddInputBuffer(AwOmxVenc* impl, OMX_BUFFERHEADERTYPE* pInBufHdr,
                                        VencInputBuffer* sInputBuffer)
{
    int buffer_type =  *(int*)(pInBufHdr->pBuffer+pInBufHdr->nOffset);
    buffer_handle_t bufferHanle = NULL;
    unsigned long phyaddress = 0;
    int ret = 0;
    int share_fd = 0;
    int result;

#ifdef CONF_NOUGAT_AND_NEWER
    if((buffer_type == kMetadataBufferTypeGrallocSource) || \
        (buffer_type == kMetadataBufferTypeANWBuffer) || \
        (buffer_type == kMetadataBufferTypeNativeHandleSource))
#else
    if(buffer_type == kMetadataBufferTypeGrallocSource)
#endif
    {
        if (buffer_type == kMetadataBufferTypeGrallocSource)
        {
            bufferHanle = *(buffer_handle_t*)(pInBufHdr->pBuffer +
                                              pInBufHdr->nOffset + 4);
        }
#ifdef CONF_NOUGAT_AND_NEWER
        else if(buffer_type == kMetadataBufferTypeANWBuffer)
        {
            VideoNativeMetadata &nativeMeta = \
            *(VideoNativeMetadata *)(pInBufHdr->pBuffer + \
            pInBufHdr->nOffset);
            ANativeWindowBuffer *buffer = \
            (ANativeWindowBuffer *)nativeMeta.pBuffer;

            bufferHanle = buffer->handle;

            if (nativeMeta.nFenceFd >= 0) {
                sp<Fence> fence = new Fence(nativeMeta.nFenceFd);
                nativeMeta.nFenceFd = -1;
                status_t err = fence->wait(FENCE_TIMEOUT_MS);
                if (err != OK) {
                    omx_loge("Timed out waiting on input fence");
                    //return NULL;
                }
            }
        }
        else if(buffer_type == kMetadataBufferTypeNativeHandleSource)
        {
            VideoNativeHandleMetadata &nativeHandleMeta = \
            *(VideoNativeHandleMetadata *)(pInBufHdr->pBuffer + \
            pInBufHdr->nOffset);
            native_handle_t *nativeHandle = \
            (native_handle_t *)nativeHandleMeta.pHandle;
            bufferHanle = (buffer_handle_t)nativeHandle;
        }
#endif
        if (impl->m_sInPortFormatType.eColorFormat !=
            OMX_COLOR_FormatAndroidOpaque)
        {
            omx_logw("do not support this input format: %d",
                 impl->m_sInPortFormatType.eColorFormat);
        }

        if(bufferHanle == NULL){
            omx_loge("input buffer handle is null\n");
        }else{
        }
        if (bufferHanle)
        {
            int colorFormat;
            //for mali GPU
            private_handle_t* hnd = (private_handle_t *)(bufferHanle);
            colorFormat = hnd->format;
            logv("GPU buffer width %d, height %d, stride %d.", hnd->width, hnd->height, hnd->stride);

            switch (colorFormat)
            {
                case HAL_PIXEL_FORMAT_RGBA_8888:
                case HAL_PIXEL_FORMAT_RGBX_8888:
                {
                    if (impl->mFirstInputFrame)
                    {
                        impl->m_vencColorFormat = VENC_PIXEL_ABGR;
                        post_message_to_venc_and_wait(
                            impl, OMX_Venc_Cmd_ChangeColorFormat);
                        impl->mFirstInputFrame = OMX_FALSE;
                        omx_logd("set input color format as ABGR");
                    }
                    break;
                }
                case HAL_PIXEL_FORMAT_YCrCb_420_SP:
                {
                    if (impl->mFirstInputFrame)
                    {
                        impl->m_vencColorFormat = VENC_PIXEL_YVU420SP;
                        post_message_to_venc_and_wait(
                            impl, OMX_Venc_Cmd_ChangeColorFormat);
                        impl->mFirstInputFrame = OMX_FALSE;
                        omx_logd("set input color format as VENC_PIXEL_YVU420SP");
                    }
                    break;
                }
                case HAL_PIXEL_FORMAT_BGRA_8888:
                {
                    omx_logv("do nothing, defalt is ARGB");
                    break;
                }
                case HAL_PIXEL_FORMAT_YV12:
                {
                    if (impl->mFirstInputFrame)
                    {
                        impl->m_vencColorFormat = VENC_PIXEL_YVU420P;
                        post_message_to_venc_and_wait(
                            impl, OMX_Venc_Cmd_ChangeColorFormat);
                        impl->mFirstInputFrame = OMX_FALSE;
                        omx_logd("set input color format as VENC_PIXEL_YV12");
                    }
                    break;
                }
                case HAL_PIXEL_FORMAT_YCbCr_420_888:
                {
                  omx_logw("input format is HAL_PIXEL_FORMAT_YCbCr_420_888,[0x%x]", colorFormat);
                  if (impl->mFirstInputFrame)
                  {
                    impl->m_vencColorFormat = VENC_PIXEL_YUV420SP;
                    post_message_to_venc_and_wait(
                        impl, OMX_Venc_Cmd_ChangeColorFormat);
                    impl->mFirstInputFrame = OMX_FALSE;
                    omx_logd("set input color format as VENC_PIXEL_YUV420SP");
                  }
                  break;
                }

                default:
                {
                   omx_logw("do not support this input format [0x%x]", colorFormat);
                    break;
                }
            }

            {
                int ret = 0;
                int share_fd = hnd->share_fd;
                long long aw_buf_id = hnd->aw_buf_id;
                ret = parseEncInputBuffer(impl,
                                            aw_buf_id,
                                            share_fd,
                                            &phyaddress);
                if(ret < 0)
                    omx_loge("parseEncInputBuffer error\n");
            }

        }
        else
        {
            omx_loge("input bufferHanle is null");
        }

        omx_logv("phyaddress: %lx", phyaddress);
        if(phyaddress == 0)
        {
            omx_loge("phyaddress == 0 exit input buffer!");
            goto EXIT_INPUT_BUFFER;
        }
        // only support ARGB now
        sInputBuffer->pAddrPhyY =
                      (unsigned char *)phyaddress;
        sInputBuffer->pAddrPhyC =
            sInputBuffer->pAddrPhyY +
            ALIGN_16B(impl->m_sInPortDefType.format.video.nStride) *
            ALIGN_16B(impl->m_sInPortDefType.format.video.nFrameHeight);

    }
    else
    {
        if (buffer_type != kMetadataBufferTypeCameraSource)
        {
            omx_logw("skip this input buffer, error buffer type: %d",
                 buffer_type);
            /*impl->m_Callbacks.EmptyBufferDone(&impl->base.mOmxComp,
                                               impl->m_pAppData,
                                               pInBufHdr);*/
            callbackEmptyBufferDone(impl, pInBufHdr);
            //pInBufHdr = NULL;
        }
        else
        {
            memcpy(sInputBuffer,
                   (pInBufHdr->pBuffer+pInBufHdr->nOffset + 4),
                   sizeof(VencInputBuffer));

            share_fd = sInputBuffer->nShareBufFd;
            ret = parseEncInputBuffer(impl,
                    (long long)sInputBuffer->nID,
                    share_fd,
                    &phyaddress);
            if(ret < 0)
                omx_loge("parseEncInputBuffer error\n");

            sInputBuffer->pAddrPhyY = (unsigned char*)phyaddress;
            sInputBuffer->pAddrPhyC = sInputBuffer->pAddrPhyY +
            impl->m_sInPortDefType.format.video.nStride *
            impl->m_sInPortDefType.format.video.nFrameHeight;
        }
    }

    //* clear flag
    sInputBuffer->nFlag = 0;
    sInputBuffer->nPts = pInBufHdr->nTimeStamp;
    sInputBuffer->nID = (unsigned long)pInBufHdr;

    if (pInBufHdr->nFlags & OMX_BUFFERFLAG_EOS)
    {
        sInputBuffer->nFlag |= VENC_BUFFERFLAG_EOS;
    }

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

    result = AddOneInputBuffer(impl->m_encoder, sInputBuffer);
    if (result!=0)
    {
        impl->mInputBufferStep = OMX_VENC_STEP_ADD_BUFFER_TO_ENC;
    }
    else
    {
        impl->mInputBufferStep = OMX_VENC_STEP_GET_INPUTBUFFER;
    }
EXIT_INPUT_BUFFER:
    omx_logv("phyaddress: %lx,exit getAndAddInputBuffer!",phyaddress);
}

void getInputBufferFromBufHdr (AwOmxVenc* impl, OMX_BUFFERHEADERTYPE* pInBufHdr,
                                        VencInputBuffer* sInputBuffer)
{
    if (impl->m_sInPortFormatType.eColorFormat ==
        OMX_COLOR_FormatAndroidOpaque)
    {
        void* bufAddr;
        buffer_handle_t bufferHanle;

        int width;
        int height;

        //* get the argb buffer.
        bufferHanle = *(buffer_handle_t*)(pInBufHdr->pBuffer +
                                          pInBufHdr->nOffset + 4);
        android::Rect rect(
            (int)impl->m_sInPortDefType.format.video.nStride,
            (int)impl->m_sInPortDefType.format.video.nFrameHeight);
        GraphicBufferMapper::get().lock(
             bufferHanle,
             (GRALLOC_USAGE_HW_VIDEO_ENCODER |
                  GRALLOC_USAGE_SW_WRITE_OFTEN),
              rect, &bufAddr);

        width = impl->m_sInPortDefType.format.video.nStride;
        height = impl->m_sInPortDefType.format.video.nFrameHeight;
#ifdef CONF_NOUGAT_AND_NEWER
#ifdef CONF_ARMV7_A_NEON
        if (width % 32 == 0)
        {
            int widthandstride[2];
            unsigned char* addr[2];

            widthandstride[0] = width;
            widthandstride[1] = width;

            addr[0] = sInputBuffer->pAddrVirY;
            addr[1] = sInputBuffer->pAddrVirC;

            ImgRGBA2YUV420SP_neon((unsigned char *)bufAddr,
                                  addr, widthandstride, height);
        }
        else
        {
            int widthandstride[2];
            unsigned char* addr[2];

            widthandstride[0] = width;
            widthandstride[1] = (width + 31) & (~31);

            addr[0] = sInputBuffer->pAddrVirY;
            addr[1] = sInputBuffer->pAddrVirC;

            ImgRGBA2YUV420SP_neon((unsigned char *)bufAddr, addr,
                                  widthandstride, height);
        }
#endif
#endif

        CdcMemFlushCache(impl->memops, sInputBuffer->pAddrVirY,
                         width * height);
        CdcMemFlushCache(impl->memops, sInputBuffer->pAddrVirC,
                         width * height / 2);
        GraphicBufferMapper::get().unlock(bufferHanle);
    }
    else
    {
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
               pInBufHdr->pBuffer + pInBufHdr->nOffset + impl->mSizeY,
               impl->mSizeC);
    }
#if SAVE_PICTURE
    FILE* fp;
    char name[128];
    sprintf(name, "/data/camera/venc_pic_%lux%lu_%lu.dat",
            impl->m_sInPortDefType.format.video.nFrameWidth,
            impl->m_sInPortDefType.format.video.nFrameHeight,impl->m_nSavePicCount);
    fp = fopen(name, "ab");
    if(fp != NULL)
    {
        omx_logd("saving picture, size: %lu x %lu, venc format: %d, count: %lu",
                impl->m_sInPortDefType.format.video.nFrameWidth,
                impl->m_sInPortDefType.format.video.nFrameHeight, impl->m_vencColorFormat,
                impl->m_nSavePicCount);
        OMX_U32 wh = impl->m_sInPortDefType.format.video.nFrameWidth*\
            impl->m_sInPortDefType.format.video.nFrameHeight;
        fwrite(sInputBuffer->pAddrVirY, 1, wh, fp);
        fwrite(sInputBuffer->pAddrVirC, 1, wh / 2, fp);
        fclose(fp);
        impl->m_nSavePicCount++;
    }
    else
    {
        omx_loge("saving picture failed: open file error, frame number = %lu", impl->m_nSavePicCount);
    }
#endif

}

void getInputBufferFromBufHdrWithoutCrop (AwOmxVenc* impl, OMX_BUFFERHEADERTYPE* pInBufHdr,
                                        VencInputBuffer* sInputBuffer)
{
    getInputBufferFromBufHdr (impl, pInBufHdr, sInputBuffer);
}

void determineVencColorFormat(AwOmxVenc* impl)
{
    switch (impl->m_sInPortFormatType.eColorFormat)
    {
#ifdef CONF_NOUGAT_AND_NEWER
        //* android 7.0 cannot get mIsFromCts, so we extend OMX_COLOR_FormatYVU420SemiPlanar
        case OMX_COLOR_FormatYUV420SemiPlanar:
        {
            impl->m_vencColorFormat = VENC_PIXEL_YUV420SP;
            break;
        }
        case OMX_COLOR_FormatYVU420SemiPlanar:
        {
            impl->m_vencColorFormat = VENC_PIXEL_YVU420SP;
            break;
        }
#else
        case OMX_COLOR_FormatYUV420SemiPlanar:
        {
            if(impl->mIsFromCts)
            {
                impl->m_vencColorFormat = VENC_PIXEL_YUV420SP;
            }
            else//* from camera
            {
                impl->m_vencColorFormat = VENC_PIXEL_YVU420SP;
            }
            break;
        }
#endif
        case OMX_COLOR_FormatAndroidOpaque:
        {
            impl->m_vencColorFormat = VENC_PIXEL_ARGB; //maybe change this later;
            break;
        }
        default:
        {
            break;
        }
    }
        omx_logd("venc color format [%d]. while venc YUV420SP [%d]. YVU420SP [%d]. ARGB [%d].",
            impl->m_vencColorFormat, VENC_PIXEL_YUV420SP, VENC_PIXEL_YVU420SP, VENC_PIXEL_ARGB);
        omx_logd("Inport format [0x%08x]. while OMX YUV420SP [0x%08x],\
             YVU420SP [0x%08x], Android-opaque [0x%08x].",
            impl->m_sInPortFormatType.eColorFormat, OMX_COLOR_FormatYUV420SemiPlanar,
            OMX_COLOR_FormatYVU420SemiPlanar, OMX_COLOR_FormatAndroidOpaque);
}

void determineVencBufStrideIfNecessary(AwOmxVenc* impl, VencBaseConfig* pBaseConfig)
{
    CEDARC_UNUSE(impl);
    CEDARC_UNUSE(pBaseConfig);
#ifdef CONF_IMG_GPU
    if (impl->m_sInPortFormatType.eColorFormat == OMX_COLOR_FormatAndroidOpaque)
        pBaseConfig->nStride = (pBaseConfig->nStride + 31) & (~31);
#endif
}

void updateOmxDebugFlag(AwOmxVenc* impl)
{
    impl->bSaveStreamFlag = OMX_FALSE;
    impl->bOpenStatisticFlag = OMX_FALSE;
    impl->bShowFrameSizeFlag = OMX_FALSE;
    #ifdef __ANDROID__
    char val[PROPERTY_VALUE_MAX];
    property_get("vendor.venc.debug", val, "0");
    if(strstr(val, "encstream")){
        impl->bSaveStreamFlag = OMX_TRUE;
        omx_logd("include encstream, save enc stream in data/camera as bistream.dat");
    }
    if(strstr(val, "encstatistic")){
        impl->bOpenStatisticFlag = OMX_TRUE;
        omx_logd("include encstatistic.");
    }
    if(strstr(val, "encframesize")){
        impl->bShowFrameSizeFlag = OMX_TRUE;
        omx_logd("include encframesize.");
    }
    #endif
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

