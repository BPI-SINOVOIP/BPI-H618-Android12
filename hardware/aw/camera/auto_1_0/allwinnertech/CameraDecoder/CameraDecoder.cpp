
#define LOG_TAG    "CameraDecoder"

#include "CameraDecoder.h"

#include<android/log.h>
#include <stdio.h>
#include <time.h>
#include <utils/Log.h>
#include<stdlib.h>
#include<string.h>
#include <dlfcn.h>
#define UNUSED(x) (void)x

namespace android {

#ifdef __cplusplus
extern "C" {
#endif

typedef void VDPluginFun(void);

static void InitVDLib(const char *lib)
{
    void *libFd = NULL;
    if (lib == NULL) {
        ALOGE(" open lib == NULL ");
        return;
    }

    libFd = dlopen(lib, RTLD_NOW);

    VDPluginFun *PluginInit = NULL;

    if (libFd == NULL) {
        ALOGE("dlopen '%s' fail: %s", lib, dlerror());
        return ;
    }

    PluginInit = (VDPluginFun*)dlsym(libFd, "CedarPluginVDInit");
    if (PluginInit == NULL) {
        ALOGE("Invalid plugin, CedarPluginVDInit not found.");
        return;
    }
    ALOGD("vdecoder open lib: %s", lib);
    PluginInit(); /* init plugin */
    return ;
}

static void AddVDLib(void)
{
#if 0//(CONF_ANDROID_MAJOR_VER >= 8)
    InitVDLib("/vendor/lib/libawh264.so");
    InitVDLib("/vendor/lib/libawmjpeg.so");
    InitVDLib("/vendor/lib/libawmjpegplus.so");
#else
    InitVDLib("/system/lib/libawh264.so");
    InitVDLib("/system/lib/libawmjpeg.so");
    InitVDLib("/system/lib/libawmjpegplus.so");
#endif

    return;
}

static int GetStreamData(const void* in, char*  buf0, int buf_size0, char* buf1,
    int buf_size1, VideoStreamDataInfo* dataInfo)
{
    ALOGV("Starting get stream data!!");
    if (dataInfo->nLength <= buf_size0) {
            ALOGV("The stream lengh is %d, the buf_size0 is %d", dataInfo->nLength, buf_size0);
            memcpy(buf0, in, dataInfo->nLength);
    } else {
        if (dataInfo->nLength <= (buf_size0+buf_size1)) {
            ALOGV("The stream lengh is %d, the buf_size0 is %d,the buf_size1 is %d",
                dataInfo->nLength, buf_size0, buf_size1);
            memcpy((void *)buf0, (void *)in, buf_size0);
            memcpy(buf1, ((void *)((unsigned char *)in + buf_size0)), (dataInfo->nLength - buf_size0));
        } else {
            return -1;
        }
    }
    dataInfo->bIsFirstPart = 1;
    dataInfo->bIsLastPart  = 1;
    dataInfo->pData        = buf0;
    dataInfo->nPts         = -1;
    dataInfo->nPcr         = -1;

    return 0;
}

void Libve_dec2(VideoDecoder** mVideoDecoder, const void *in, void *out, VideoStreamInfo* pVideoInfo,
    VideoStreamDataInfo* dataInfo, __attribute__((unused))VConfig* pVconfig, struct ScMemOpsS* memOpsS)
{
    int   ret;
    char* pBuf0;
    char* pBuf1;
    int size0;
    int size1;
    UNUSED(memOpsS);

    VideoPicture*     pPicture;
    if (*mVideoDecoder == NULL) {
        ALOGE("mVideoDecoder = NULL, return");
        return;
    }

    ret = RequestVideoStreamBuffer(*mVideoDecoder, dataInfo->nLength, &pBuf0, &size0, &pBuf1, &size1, 0);

    if (ret < 0) {
        ALOGE("FUNC:%s, LINE:%d, RequestVideoStreamBuffer fail!", __FUNCTION__, __LINE__);
        return;
    }

    GetStreamData(in, pBuf0, size0, pBuf1, size1, dataInfo);

    SubmitVideoStreamData(*mVideoDecoder, dataInfo, 0);

    //* decode stream.
    ret = DecodeVideoStream(*mVideoDecoder, 0 /*eos*/, 0/*key frame only*/, 0/*drop b frame*/, 0/*current time*/);

    if (ret == VDECODE_RESULT_FRAME_DECODED || ret == VDECODE_RESULT_KEYFRAME_DECODED) {
        pPicture = RequestPicture(*mVideoDecoder, 0/*the major stream*/);

        if (pPicture) {
            //memOpsS->flush_cache((void*)pPicture->pData0, pVideoInfo->nWidth * pVideoInfo->nHeight);
            //memOpsS->flush_cache((void*)pPicture->pData1, pVideoInfo->nWidth * pVideoInfo->nHeight / 2);
            memcpy(out, (void*)pPicture->pData0, pVideoInfo->nWidth * pVideoInfo->nHeight);
            memcpy((char*)out + pVideoInfo->nWidth * pVideoInfo->nHeight, (void*)pPicture->pData1,
                pVideoInfo->nWidth * pVideoInfo->nHeight / 2);

            ReturnPicture(*mVideoDecoder, pPicture);
        }
    }
}

int Libve_init2(VideoDecoder** mVideoDecoder, VideoStreamInfo* pVideoInfo, VConfig* pVconfig)
{
    if (*mVideoDecoder != NULL) {
        ALOGE("FUNC: %s fail, LINE: %d, mVideoDecoder is not NULL, please check it!", __FUNCTION__, __LINE__);
        return -1;
    }
    AddVDLib();
    *mVideoDecoder = CreateVideoDecoder();
    ALOGD("Libve_init2#########,%p", *mVideoDecoder);
    //* initialize the decoder.
    if (InitializeVideoDecoder(*mVideoDecoder, pVideoInfo, pVconfig) != 0) {
        ALOGE("initialize video decoder fail.");
        DestroyVideoDecoder(*mVideoDecoder);
        *mVideoDecoder = NULL;
        return -1;
    }

    return 0;
}

int Libve_exit2(VideoDecoder** mVideoDecoder)
{
    if (*mVideoDecoder == NULL) {
        ALOGE("FUNC: %s, LINE: %d, mVideoDecoder == NULL", __FUNCTION__, __LINE__);
        return -1;
    }

    DestroyVideoDecoder(*mVideoDecoder);
    *mVideoDecoder = NULL;

    return 0;
}
#ifdef __cplusplus
}
#endif

CameraDecoder::CameraDecoder()
    : mDecoder(NULL)
    , mMemOpsS(NULL)
{
    ALOGD("FUNC:%s, Line:%d", __FUNCTION__, __LINE__);
}

CameraDecoder::~CameraDecoder()
{
    ALOGD("FUNC:%s, Line:%d", __FUNCTION__, __LINE__);
}

int CameraDecoder::DecorderEnter(const void *in, void *out, int size, int64_t timestamp)
{

    mDataInfo.nLength = size;
    mDataInfo.nPts = timestamp;
    //mMemOpsS->flush_cache((void*)out, mVideoInfo.nWidth * mVideoInfo.nHeight * 3 / 2);
    Libve_dec2(&mDecoder, in, out,&mVideoInfo,
            &mDataInfo, &mVideoConf, mMemOpsS);
    //mMemOpsS->flush_cache((void*)out, mVideoInfo.nWidth * mVideoInfo.nHeight *3 / 2);
    return 0;
}

int CameraDecoder::DecorderOpen(int outFormat, int codecFormat, int framerate, int frameWidth, int frameHeight)
{
    ALOGD("FUNC:%s, Line:%d init Dec!", __FUNCTION__, __LINE__);
    memset(&mVideoConf, 0, sizeof(VConfig));
    memset(&mVideoInfo, 0, sizeof(VideoStreamInfo));
    memset(&mDataInfo, 0, sizeof(VideoStreamDataInfo));
    mDecoder = NULL;
    mMemOpsS = NULL;
    mVideoConf.eOutputPixelFormat = outFormat;//PIXEL_FORMAT_NV21;//PIXEL_FORMAT_YV12;PIXEL_FORMAT_YUV_MB32_420;//
    mVideoConf.bDisable3D = 1;

    mVideoConf.nVbvBufferSize = 0;

    mMemOpsS = MemAdapterGetOpsS();
    mMemOpsS->open();
    mVideoConf.memops = mMemOpsS;
    mVideoInfo.eCodecFormat = codecFormat;
    mVideoInfo.nWidth = frameWidth;;
    mVideoInfo.nHeight = frameHeight;
    mVideoInfo.nFrameRate = framerate;
    mVideoInfo.nFrameDuration = 1000 * 1000 / framerate;
    mVideoInfo.nAspectRatio = 1000;
    mVideoInfo.bIs3DStream = 0;
    mVideoInfo.nCodecSpecificDataLen = 0;
    mVideoInfo.pCodecSpecificData = NULL;

    Libve_init2(&mDecoder, &mVideoInfo, &mVideoConf);
    if (mDecoder == NULL) {
        ALOGE("FUNC:%s, Line:%d ", __FUNCTION__, __LINE__);
        return -1;
    }
    return 0;
}

int CameraDecoder::DecorderClose()
{
    Libve_exit2(&mDecoder);
    if (mMemOpsS != NULL) {
        mMemOpsS->close();
    }
    return 0;
}
}
