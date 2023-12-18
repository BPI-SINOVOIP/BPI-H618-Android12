#define LOG_TAG    "Libve_decorder2"

#include "include/Libve_Decoder2.h"
#include <android/log.h>
#include <stdio.h>
#include <time.h>
#include "vdecoder.h"

#define USE_ION_MEM_ALLOCATOR

extern struct ScMemOpsS* MemAdapterGetOpsS();

#if  DBG_ENABLE
     #define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
     #define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
     #define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
     #define LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
     #define LOGF(...)  __android_log_print(ANDROID_LOG_FATAL,LOG_TAG,__VA_ARGS__)
     #define LOGV(...)  __android_log_print(ANDROID_LOG_VERBOSE,LOG_TAG,__VA_ARGS__)
#else
     #define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
     #define LOGD(...)
     #define LOGI(...)
     #define LOGW(...)
     #define LOGF(...)
     #define LOGV(...)
#endif

static int GetStreamData(const void* in,
                         char*  buf0,
                         int buf_size0,
                         char* buf1,
                         int buf_size1,
                         VideoStreamDataInfo* dataInfo)
{
    LOGV("Starting get stream data!!");
    if(dataInfo->nLength <= buf_size0) {
            //LOGV("The stream length is %d, the buf_size0 is %d",
            //    dataInfo->nLength, buf_size0);
            memcpy(buf0, in, dataInfo->nLength);
    }
    else {
        if(dataInfo->nLength <= (buf_size0+buf_size1)){
            //LOGV("The stream lengh is %d, the buf_size0 is %d,the buf_size1 is %d",
            //    dataInfo->nLength,buf_size0,buf_size1);
            memcpy(buf0, in, buf_size0);
            memcpy(buf1,((void*)((long)in+buf_size0)),(dataInfo->nLength -buf_size0));
        }
        else
            return -1;
    }
    dataInfo->bIsFirstPart = 1;
    dataInfo->bIsLastPart  = 1;
    dataInfo->pData       = buf0;
    dataInfo->nPts        = -1;
    dataInfo->nPcr        = -1;

    return 0;
}

int Libve_dec2(VideoDecoder** mVideoDecoder,
                const void *in,
                void *outY,
                void *outC,
                VideoStreamInfo* pVideoInfo,
                VideoStreamDataInfo* dataInfo,
                VConfig* pVconfig)
{
    LOGV("%s", __func__);
    int   ret;
    char* pBuf0;
    char* pBuf1;
    int size0;
    int size1;
    VideoPicture*     pPicture;

    if(*mVideoDecoder == NULL)
    {
        LOGE("mVideoDecoder = NULL, return");
        return -1;
    }

    ret = RequestVideoStreamBuffer(*mVideoDecoder,
                                   dataInfo->nLength,
                                   &pBuf0,
                                   &size0,
                                   &pBuf1,
                                   &size1,
                                   0);
    if(ret < 0)
    {
        LOGE("FUNC:%s, LINE:%d, RequestVideoStreamBuffer fail!",__FUNCTION__,__LINE__);
        return -1;
    }
    if(in == NULL){
        LOGE("return because of in is NULL");
        return -1;
    }
    GetStreamData(in, pBuf0, size0, pBuf1, size1, dataInfo);
    SubmitVideoStreamData(*mVideoDecoder, dataInfo, 0);

    //* decode stream.
    ret = DecodeVideoStream(*mVideoDecoder,
                            0 /*eos*/,
                            0 /*key frame only*/,
                            0 /*drop b frame*/,
                            0 /*current time*/);

    if(ret == VDECODE_RESULT_FRAME_DECODED || ret == VDECODE_RESULT_KEYFRAME_DECODED)
    {
        pPicture = RequestPicture(*mVideoDecoder, 0/*the major stream*/);

        if(pPicture)
        {
            int y_size = ALIGN_16B(pVideoInfo->nWidth) * ALIGN_16B(pVideoInfo->nHeight);
            int c_size = ALIGN_16B(pVideoInfo->nWidth) * ALIGN_16B(pVideoInfo->nHeight) / 2;
            if (pPicture->pData0 != NULL) {
                CdcMemFlushCache(pVconfig->memops, (void*)pPicture->pData0, y_size);
            }
            if (pPicture->pData1 != NULL) {
                CdcMemFlushCache(pVconfig->memops, (void*)pPicture->pData1, c_size);
            }
            memcpy(outY, (void*)pPicture->pData0, y_size);
            memcpy((char*)outC, (void*)pPicture->pData1, c_size);
            ReturnPicture(*mVideoDecoder, pPicture);
            LOGD("get valid picture!");
            return 0;
        } else {
            LOGD("no valid picture!");
            return -1;
        }
    } else {
        LOGD("DecodeVideoStream is not in decoded status, ret = %d", ret);
        return -1;
    }
}

static int GetLocalPathFromProcessMaps(char *localPath, int len)
{
#define LOCAL_LIB "libvdecoder.so"
#define LOCAL_LINUX_LIB "libcdc_vdecoder.so"

#define errno 1


    char path[512] = {0};
    char line[1024] = {0};
    FILE *file = NULL;
    char *strLibPos = NULL;
    int ret = -1;

    memset(localPath, 0x00, len);

    sprintf(path, "/proc/%d/maps", getpid());
    file = fopen(path, "r");
    if (file == NULL)
    {
        LOGE("FUNC:%s, LINE:%d, fopen failure!",__FUNCTION__,__LINE__);
        ret = -1;
        goto out;
    }

    while (fgets(line, 1023, file) != NULL)
    {
        if ((strLibPos = strstr(line, LOCAL_LIB)) != NULL ||
            (strLibPos = strstr(line, LOCAL_LINUX_LIB)) != NULL)
        {
            char *rootPathPos = NULL;
            int localPathLen = 0;
            rootPathPos = strchr(line, '/');
            if (rootPathPos == NULL)
            {
                LOGE("FUNC:%s, LINE:%d, some thing error!",__FUNCTION__,__LINE__);
                ret = -1;
                goto out;
            }

            localPathLen = strLibPos - rootPathPos - 1;
            if (localPathLen > len -1)
            {
                LOGE("FUNC:%s, LINE:%d, some thing error!",__FUNCTION__,__LINE__);
                LOGE("localPath too long :%s, LINE:%d, some thing error!",localPath,__LINE__);
                ret = -1;
                goto out;
            }

            memcpy(localPath, rootPathPos, localPathLen);
            ret = 0;
            goto out;
        }
    }
    LOGE("FUNC:%s, LINE:%d, Are you kidding? not found?!",__FUNCTION__,__LINE__);

out:
    if (file)
    {
        fclose(file);
    }
    return ret;
}

typedef void VDPluginEntry(void);

void AddVDPluginSingle(char *lib)
{
    void *libFd = NULL;
    if(lib == NULL)
    {
        LOGE("FUNC:%s, LINE:%d, open lib == NULL !",__FUNCTION__,__LINE__);
        return;
    }

    libFd = dlopen(lib, RTLD_NOW);

    VDPluginEntry *PluginInit = NULL;

    if (libFd == NULL)
    {
        LOGE("FUNC:%s, LINE:%d, dlopen!",__FUNCTION__,__LINE__);
        LOGE("dlopen '%s', LINE:%d, ",lib,__LINE__);
        LOGE("dlopen '%s', LINE:%d, ",dlerror(),__LINE__);

        return ;
    }

    PluginInit = dlsym(libFd, "CedarPluginVDInit");
    if (PluginInit == NULL)
    {
        LOGW("Invalid plugin, CedarPluginVDInit not found.: %s, LINE:%d, ",__FUNCTION__,__LINE__);

        return;
    }
    LOGD("vdecoder open lib: %s, LINE:%d, ",lib,__LINE__);

    PluginInit(); /* init plugin */
    return ;
}

/* executive when load */
//static void AddVDPlugin(void) __attribute__((constructor));
void AddVDPlugin(void)
{
    char localPath[512];
    char slash[4] = "/";
    char loadLib[512];
    struct dirent **namelist = NULL;
    int num = 0, index = 0;
    int pathLen = 0;
    int ret;

    memset(localPath, 0, 512);
    memset(loadLib, 0, 512);
//scan_local_path:
    ret = GetLocalPathFromProcessMaps(localPath, 512);
    if (ret != 0)
    {
        LOGW("get local path failure, scan /system/lib ,: %s, LINE:%d, ",__FUNCTION__,__LINE__);

        goto scan_system_lib;
    }

    num = scandir(localPath, &namelist, NULL, NULL);
    if (num <= 0)
    {
        LOGW("scandir failure, errnoscan /system/lib: %s, LINE:%d, ",__FUNCTION__,__LINE__);

        goto scan_system_lib;
    }
    strcat(localPath, slash);
    pathLen = strlen(localPath);
    strcpy(loadLib, localPath);
    LOGW("Camera HAL get local path:: %s, LINE:%d, ",__FUNCTION__,__LINE__);
    LOGW("Camera HAL get local path:: %s, LINE:%d, ",localPath,__LINE__);

    for(index = 0; index < num; index++)
    {
        if(((strstr((namelist[index])->d_name, "libaw") != NULL) ||
            (strstr((namelist[index])->d_name, "libcdc_vd") != NULL) ||
            (strstr((namelist[index])->d_name, "librv") != NULL))
            && (strstr((namelist[index])->d_name, ".so") != NULL))
        {
            loadLib[pathLen] = '\0';
            strcat(loadLib, (namelist[index])->d_name);

            LOGW("1117 load so: %s, LINE:%d, ",__FUNCTION__,__LINE__);
            LOGW("1117 load so: %s , LINE:%d, ",loadLib,__LINE__);
            AddVDPluginSingle(loadLib);
        }
        free(namelist[index]);
        namelist[index] = NULL;
    }

scan_system_lib:
    // TODO: scan /system/lib

    return;
}

int Libve_init2(VideoDecoder** mVideoDecoder, VideoStreamInfo* pVideoInfo, VConfig* pVconfig)
{
    if(*mVideoDecoder != NULL)
    {
        LOGE("FUNC: %s fail, LINE: %d, mVideoDecoder is not NULL, please check it!",
            __FUNCTION__, __LINE__);
        return -1;
    }

    AddVDPlugin();

    *mVideoDecoder = CreateVideoDecoder();

    //* initialize the decoder.
    if(InitializeVideoDecoder(*mVideoDecoder, pVideoInfo, pVconfig) != 0)
    {
        LOGE("initialize video decoder fail.");
        DestroyVideoDecoder(*mVideoDecoder);
        *mVideoDecoder = NULL;
        return -1;
    }

    return 0;
}

int Libve_exit2(VideoDecoder** mVideoDecoder)
{
    if(*mVideoDecoder == NULL)
    {
        LOGE("FUNC: %s, LINE: %d, mVideoDecoder == NULL",__FUNCTION__, __LINE__);
        return -1;
    }

    DestroyVideoDecoder(*mVideoDecoder);
    *mVideoDecoder = NULL;

    return 0;
}

