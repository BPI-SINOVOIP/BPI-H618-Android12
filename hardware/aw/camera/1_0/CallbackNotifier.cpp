
#include "CameraDebug.h"
#if DBG_CALLBACK
#define LOG_NDEBUG 0
#endif
#define LOG_TAG "CallbackNotifier"
#include <cutils/log.h>

#include <cutils/properties.h>
#include <HardwareAPI.h>

#include "V4L2CameraDevice2.h"
#include "CallbackNotifier.h"
#include "PreviewWindow.h"
#include <system/camera.h>
#include <vencoder.h>
extern "C" int scaler(unsigned char * psrc, unsigned char * pdst, int src_w, int src_h, int dst_w, int dst_h, int fmt, int align);

extern "C" int AWJpecEnc(JpegEncInfo* pJpegInfo, EXIFInfo* pExifInfo, void* pOutBuffer, int* pOutBufferSize);
	
namespace android {

/* String representation of camera messages. */
static const char* lCameraMessages[] =
{
    "CAMERA_MSG_ERROR",
    "CAMERA_MSG_SHUTTER",
    "CAMERA_MSG_FOCUS",
    "CAMERA_MSG_ZOOM",
    "CAMERA_MSG_PREVIEW_FRAME",
    "CAMERA_MSG_VIDEO_FRAME",
    "CAMERA_MSG_POSTVIEW_FRAME",
    "CAMERA_MSG_RAW_IMAGE",
    "CAMERA_MSG_COMPRESSED_IMAGE",
    "CAMERA_MSG_RAW_IMAGE_NOTIFY",
    "CAMERA_MSG_PREVIEW_METADATA",
    "CAMERA_MSG_CONTINUOUSSNAP",
    "CAMERA_MSG_SNAP"
    "CAMERA_MSG_SNAP_THUMB"
    "CAMERA_MSG_SNAP_FD",
    "CAMERA_SMART_MSG_STATUS"
};
static const int lCameraMessagesNum = sizeof(lCameraMessages) / sizeof(char*);

/* Builds an array of strings for the given set of messages.
 * Param:
 *  msg - Messages to get strings for,
 *  strings - Array where to save strings
 *  max - Maximum number of entries in the array.
 * Return:
 *  Number of strings saved into the 'strings' array.
 */
static int GetMessageStrings(uint32_t msg, const char** strings, int max)
{
    int index = 0;
    int out = 0;
    while (msg != 0 && out < max && index < lCameraMessagesNum) {
        while ((msg & 0x1) == 0 && index < lCameraMessagesNum) {
            msg >>= 1;
            index++;
        }
        if ((msg & 0x1) != 0 && index < lCameraMessagesNum) {
            strings[out] = lCameraMessages[index];
            out++;
            msg >>= 1;
            index++;
        }
    }

    return out;
}

/* Logs messages, enabled by the mask. */
#if DEBUG_MSG
static void PrintMessages(uint32_t msg)
{
    const char* strs[lCameraMessagesNum];
    const int translated = GetMessageStrings(msg, strs, lCameraMessagesNum);
    for (int n = 0; n < translated; n++) {
        LOGV("    %s", strs[n]);
    }
}
#else
#define PrintMessages(msg)   (void(0))
#endif

/*static void formatToNV21(void *dst,
                       void *src,
                       int width,
                       int height,
                       size_t stride,
                       uint32_t offset,
                       unsigned int bytesPerPixel,
                       size_t length,
                       int pixelFormat)
{
    unsigned int alignedRow, row;
    unsigned char *bufferDst, *bufferSrc;
    unsigned char *bufferDstEnd, *bufferSrcEnd;
    uint16_t *bufferSrc_UV;

    unsigned long y_uv[2];
    y_uv[0] = (unsigned long)src;
    y_uv[1] = (unsigned long)src + width*height;

    // NV12 -> NV21
    if (pixelFormat == V4L2_PIX_FMT_NV12) {
        bytesPerPixel = 1;
        bufferDst = ( unsigned char * ) dst;
        bufferDstEnd = ( unsigned char * ) dst + width*height*bytesPerPixel;
        bufferSrc = ( unsigned char * ) y_uv[0] + offset;
        bufferSrcEnd = ( unsigned char * ) ( ( size_t ) y_uv[0] + length + offset);
        row = width*bytesPerPixel;
        alignedRow = stride-width;
        int stride_bytes = stride / 8;
        uint32_t xOff = offset % stride;
        uint32_t yOff = offset / stride;

        // going to convert from NV12 here and return
        // Step 1: Y plane: iterate through each row and copy
        for ( int i = 0 ; i < height ; i++) {
            memcpy(bufferDst, bufferSrc, row);
            bufferSrc += stride;
            bufferDst += row;
            if ( ( bufferSrc > bufferSrcEnd ) || ( bufferDst > bufferDstEnd ) ) {
                break;
            }
        }

        bufferSrc_UV = ( uint16_t * ) ((uint8_t*)y_uv[1] + (stride/2)*yOff + xOff);

        uint16_t *bufferDst_UV;

        // Step 2: UV plane: convert NV12 to NV21 by swapping U & V
        bufferDst_UV = (uint16_t *) (((uint8_t*)dst)+row*height);

        for (int i = 0 ; i < height/2 ; i++, bufferSrc_UV += alignedRow/2) {
            int n = width;
            asm volatile (
            "   pld [%[src], %[src_stride], lsl #2]                         \n\t"
            "   cmp %[n], #32                                               \n\t"
            "   blt 1f                                                      \n\t"
            "0: @ 32 byte swap                                              \n\t"
            "   sub %[n], %[n], #32                                         \n\t"
            "   vld2.8  {q0, q1} , [%[src]]!                                \n\t"
            "   vswp q0, q1                                                 \n\t"
            "   cmp %[n], #32                                               \n\t"
            "   vst2.8  {q0,q1},[%[dst]]!                                   \n\t"
            "   bge 0b                                                      \n\t"
            "1: @ Is there enough data?                                     \n\t"
            "   cmp %[n], #16                                               \n\t"
            "   blt 3f                                                      \n\t"
            "2: @ 16 byte swap                                              \n\t"
            "   sub %[n], %[n], #16                                         \n\t"
            "   vld2.8  {d0, d1} , [%[src]]!                                \n\t"
            "   vswp d0, d1                                                 \n\t"
            "   cmp %[n], #16                                               \n\t"
            "   vst2.8  {d0,d1},[%[dst]]!                                   \n\t"
            "   bge 2b                                                      \n\t"
            "3: @ Is there enough data?                                     \n\t"
            "   cmp %[n], #8                                                \n\t"
            "   blt 5f                                                      \n\t"
            "4: @ 8 byte swap                                               \n\t"
            "   sub %[n], %[n], #8                                          \n\t"
            "   vld2.8  {d0, d1} , [%[src]]!                                \n\t"
            "   vswp d0, d1                                                 \n\t"
            "   cmp %[n], #8                                                \n\t"
            "   vst2.8  {d0[0],d1[0]},[%[dst]]!                             \n\t"
            "   bge 4b                                                      \n\t"
            "5: @ end                                                       \n\t"
#ifdef NEEDS_ARM_ERRATA_754319_754320
            "   vmov s0,s0  @ add noop for errata item                      \n\t"
#endif
            : [dst] "+r" (bufferDst_UV), [src] "+r" (bufferSrc_UV), [n] "+r" (n)
            : [src_stride] "r" (stride_bytes)
            : "cc", "memory", "q0", "q1"
            );
        }
    }
}*/

static void NV12ToYVU420(void* psrc, void* pdst, int width, int height)
{
    uint8_t* psrc_y = (uint8_t*)psrc;
    uint8_t* pdst_y = (uint8_t*)pdst;
    uint8_t* psrc_uv = (uint8_t*)psrc + width * height;
    uint8_t* pdst_uv = (uint8_t*)pdst + width * height;

    for(int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            *(uint8_t*)(pdst_y + i * width + j) = *(uint8_t*)(psrc_y + i * width + j);
        }
    }

    for(int i = 0; i < height / 2; i++)
    {
        for (int j = 0; j < width / 2; j++)
        {
            *(uint8_t*)(pdst_uv + i * width / 2 + j) = *(uint8_t*)(psrc_uv + i * width + j * 2);
            *(uint8_t*)(pdst_uv + (width / 2) * (height / 2) + i * width / 2 + j) = *(uint8_t*)(psrc_uv + i * width + j * 2 + 1);
        }
    }

}

static void NV21ToYVU420(void* psrc, void* pdst, int width, int height)
{
    uint8_t* psrc_y = (uint8_t*)psrc;
    uint8_t* pdst_y = (uint8_t*)pdst;
    uint8_t* psrc_uv = (uint8_t*)psrc + width * height;
    uint8_t* pdst_uv = (uint8_t*)pdst + width * height;

    for(int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            *(uint8_t*)(pdst_y + i * width + j) = *(uint8_t*)(psrc_y + i * width + j);
        }
    }
    for(int i = 0; i < height / 2; i++)
    {
        for (int j = 0; j < width / 2; j++)
        {
            *(uint8_t*)(pdst_uv + i * width / 2 + j) = *(uint8_t*)(psrc_uv + i * width + j * 2 + 1);
            *(uint8_t*)(pdst_uv + (width / 2) * (height / 2) + i * width / 2 + j) = *(uint8_t*)(psrc_uv + i * width + j * 2);
        }
    }
}

static void YVU420ToNV21(void* psrc, void* pdst, int width, int height)
{
    uint8_t* psrc_y = (uint8_t*)psrc;
    uint8_t* pdst_y = (uint8_t*)pdst;
    uint8_t* psrc_uv = (uint8_t*)psrc + width * height;
    uint8_t* pdst_uv = (uint8_t*)pdst + width * height;

    for(int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            *(uint8_t*)(pdst_y + i * width + j) = *(uint8_t*)(psrc_y + i * width + j);
        }
    }

    for(int i = 0; i < height / 2; i++)
    {
        for (int j = 0; j < width / 2; j++)
        {
            *(uint8_t*)(pdst_uv + i * width + j * 2 + 1) = *(uint8_t*)(psrc_uv + i * width / 2 + j);
            *(uint8_t*)(pdst_uv + i * width + j * 2) = *(uint8_t*)(psrc_uv + (width / 2) * (height / 2) + i * width / 2 + j);
        }
    }

}

static void createMap(int s_w,int s_h, int d_w, int d_h,int* mMap_w,int* mMap_h)
{
    int i,j;
    int w_index_cur = -1;
    int w_index_new;
    int h_index_cur = -1;
    int h_index_new;
    memset((void *)mMap_w,0,d_w*sizeof(int));
    memset((void *)mMap_h,0,d_h*sizeof(int));

    i = 0;
    j = 0;
    while (i < s_w && j < d_w) {
        w_index_new = i*d_w/s_w;
        if (  w_index_new != w_index_cur) {
                mMap_w[j]= i;
                j++;
                w_index_cur = w_index_new;
            }
            i++;
        }
    j = 0;
    i = 0;
    while (i < s_h && j < d_h) {
            h_index_new = i*d_h/s_h;
            if (  h_index_new != h_index_cur) {
                    mMap_h[j]= i;
                    j++;
                    h_index_cur = h_index_new;
                }
            i++;
        }
}



static bool yuv420spDownScale(void* psrc, void* pdst, int src_w, int src_h, int dst_w, int dst_h)
{
    char * psrc_y = (char *)psrc;
    char * pdst_y = (char *)pdst;
    char * psrc_uv = (char *)psrc + src_w * src_h;
    char * pdst_uv = (char *)pdst + dst_w * dst_h;

    int scale = 1;
    int scale_w = src_w / dst_w;
    int scale_h = src_h / dst_h;
    int h, w;

    if (dst_w > src_w
        || dst_h > src_h)
    {
        LOGE("error size, %dx%d -> %dx%d\n", src_w, src_h, dst_w, dst_h);
        return false;
    }

    if (scale_w == scale_h)
    {
        scale = scale_w;
    }

    LOGV("scale = %d\n", scale);

    if (scale == 1)
    {
        if ((src_w == dst_w)
            && (src_h == dst_h))
        {
            memcpy((char*)pdst, (char*)psrc, dst_w * dst_h * 3/2);
        }
        else
        {
            // crop
            for (h = 0; h < dst_h; h++)
            {
                memcpy((char*)pdst + h * dst_w, (char*)psrc + h * src_w, dst_w);
            }
            for (h = 0; h < dst_h / 2; h++)
            {
                memcpy((char*)pdst_uv + h * dst_w, (char*)psrc_uv + h * src_w, dst_w);
            }
        }

        return true;
    }

    for (h = 0; h < dst_h; h++)
    {
        for (w = 0; w < dst_w; w++)
        {
            *(pdst_y + h * dst_w + w) = *(psrc_y + h * scale * src_w + w * scale);
        }
    }
    for (h = 0; h < dst_h / 2; h++)
    {
        for (w = 0; w < dst_w; w+=2)
        {
            *((short*)(pdst_uv + h * dst_w + w)) = *((short*)(psrc_uv + h * scale * src_w + w * scale));
        }
    }

    return true;
}

static bool yuv420spDown_noneScale(void* psrc, void* pdst, int src_w, int src_h, int dst_w, int dst_h)
{
    char * psrc_y = (char *)psrc;
    char * pdst_y = (char *)pdst;
    char * psrc_uv = (char *)psrc + src_w * src_h;
    char * pdst_uv = (char *)pdst + dst_w * dst_h;
    int y_h_index[dst_h];
    int y_w_index[dst_w];
    int uv_h_index[dst_h/2];
    int uv_w_index[dst_w];
    int h, w;
    if (dst_w > src_w
        || dst_h > src_h)
    {
        LOGE("error size, %dx%d -> %dx%d\n", src_w, src_h, dst_w, dst_h);
        return false;
    }

    //y
    createMap(src_w,src_h,dst_w,dst_h,y_w_index,y_h_index);
    for (h = 0; h < dst_h; h++)
    {
        for (w = 0; w < dst_w; w++)
        {
            *(pdst_y + h * dst_w + w) = *(psrc_y +y_h_index[h]* src_w + y_w_index[w]);
        }
    }

    //uv
    createMap(src_w/2,src_h/2,dst_w/2,dst_h/2,uv_w_index,uv_h_index);
    for (h = 0; h < dst_h / 2; h++)
    {
        for (w = 0; w < dst_w; w+=2)
        {
            *((short*)(pdst_uv + h * dst_w + w)) = *((short*)(psrc_uv +uv_h_index[h]* src_w + uv_w_index[w/2]*2));
        }
    }
    return true;

}


static bool yuv420pDownScale_align(void* psrc, void* pdst, int src_w, int src_h, int dst_w, int dst_h,int yStride,int uvStride)
{
    char * psrc_y = (char *)psrc;
    char * pdst_y = (char *)pdst;
    char * psrc_uv = (char *)psrc + src_w * src_h;
    char * pdst_uv = (char *)pdst + dst_w * dst_h;
    memset(pdst_uv,128,uvStride*dst_h/2);
    int scale_w = src_w / dst_w;
    int scale_h = src_h / dst_h;
    int h, w;


    if (dst_w > src_w
        || dst_h > src_h)
    {
        LOGE("error size, %dx%d -> %dx%d\n", src_w, src_h, dst_w, dst_h);
        return false;
    }

        LOGD("src_w :%d,src_h :%d,dst_w: %d, dst_h :%d,yStride :%d,uvStride:%d,scale_w: %d,scale_h %d",
            src_w, src_h, dst_w, dst_h, yStride,uvStride,scale_w,scale_h);
    //y

    int y_h_index[dst_h];
    int y_w_index[dst_w];
    createMap(src_w,src_h,dst_w,dst_h,y_w_index,y_h_index);
    for (h = 0; h < dst_h; h++)
    {
        for (w = 0; w < dst_w; w++)
        {
            *(pdst_y + h *yStride + w) = *(psrc_y + y_h_index[h]* src_w + y_w_index[w]);
        }
    }

    int uv_h_index[dst_h/2];
    int uv_w_index[dst_w/2];
    createMap(src_w/2,src_h/2,dst_w/2,dst_h/2,uv_w_index,uv_h_index);
    //V
    for (h = 0; h < dst_h/2 ; h++)
    {
        for (w = 0; w < dst_w/2; w++)
        {
            *(pdst_uv + h * uvStride+ w) = *(psrc_uv + uv_h_index[h]* src_w/2 + uv_w_index[w]);
        }
    }
    //u
    for (h = 0; h < dst_h/2 ; h++ )
    {
        for (w = 0; w < dst_w/2; w++)
        {
            *(pdst_uv + (h+dst_h/2)* uvStride+ w) = *(psrc_uv + src_w * src_h/4 + uv_h_index[h]* src_w/2 + uv_w_index[w]);
        }
    }

    return true;
}


CallbackNotifier::CallbackNotifier()
    : mNotifyCB(NULL),
      mDataCB(NULL),
      mDataCBTimestamp(NULL),
      mGetMemoryCB(NULL),
      mCallbackCookie(NULL),
      mMessageEnabler(0),
      mVideoRecEnabled(false),
      mSavePictureCnt(0),
      mSavePictureMax(0),
      mJpegQuality(90),
      mJpegRotate(0),
      mPictureWidth(640),
      mPictureHeight(480),
      mThumbWidth(0),
      mThumbHeight(0),
      mGpsLatitude(0.0),
      mGpsLongitude(0.0),
      mGpsAltitude(0.0),
      mGpsTimestamp(0),
      mFocalLength(0.0),
      mWhiteBalance(0),
      mFd(0),
      mCBWidth(0),
      mCBHeight(0),
      mBufferList(NULL),
      mSaveThreadExited(false),
      mIsSinglePicture(true),
      mISOSpeed(0),
	  mContinuousFdIndex(0),
      mMeteringMode(0),
      mFlashUsed(0),
      mCount(0),
      mExposureMode(0)
{
    LOGV("CallbackNotifier construct");
    memset(mContinuousFd,0,sizeof(mContinuousFd));
    memset(mGpsMethod, 0, sizeof(mGpsMethod));
    memset(mCallingProcessName, 0, sizeof(mCallingProcessName));

    strcpy(mExifMake, "MID MAKE");        // default
    strcpy(mExifModel, "MID MODEL");    // default
    memset(mDateTime, 0, sizeof(mDateTime));
    strcpy(mDateTime, "2012:10:24 17:30:00");

    memset(mFolderPath, 0, sizeof(mFolderPath));
    memset(mSnapPath, 0, sizeof(mSnapPath));
    memset(mCam_buff, 0, sizeof(camera_memory_t *) * NB_BUFFER);
}

CallbackNotifier::~CallbackNotifier()
{
    LOGV("CallbackNotifier disconstruct");
}

/****************************************************************************
 * Camera API
 ***************************************************************************/

void CallbackNotifier::setCallbacks(camera_notify_callback notify_cb,
                                    camera_data_callback data_cb,
                                    camera_data_timestamp_callback data_cb_timestamp,
                                    camera_request_memory get_memory,
                                    void* user)
{
    LOGV("%s: %p, %p, %p, %p (%p)",
         __FUNCTION__, notify_cb, data_cb, data_cb_timestamp, get_memory, user);

    Mutex::Autolock locker(&mObjectLock);
    mNotifyCB = notify_cb;
    mDataCB = data_cb;
    mDataCBTimestamp = data_cb_timestamp;
    mGetMemoryCB = get_memory;
    mCallbackCookie = user;

    mBufferList = new BufferListManager();
    if (mBufferList == NULL)
    {
        LOGE("create BufferListManager failed");
    }

    mSaveThreadExited = false;

    pthread_mutex_init(&mCountMutex, NULL);
    // init picture thread
    mSavePictureThread = new DoSavePictureThread(this);
    pthread_mutex_init(&mSavePictureMutex, NULL);
    pthread_cond_init(&mSavePictureCond, NULL);
    mSavePictureThread->startThread();
    pthread_mutex_init(&mPictureFdMutex, NULL);
    pthread_cond_init(&mPictureFdCond, NULL);
}

void CallbackNotifier::enableMessage(uint msg_type)
{
    LOGV("%s: msg_type = 0x%x", __FUNCTION__, msg_type);
    PrintMessages(msg_type);

    Mutex::Autolock locker(&mObjectLock);
    mMessageEnabler |= msg_type;
    LOGV("**** Currently enabled messages:");
    PrintMessages(mMessageEnabler);
}

void CallbackNotifier::disableMessage(uint msg_type)
{
    LOGV("%s: msg_type = 0x%x", __FUNCTION__, msg_type);
    PrintMessages(msg_type);

    Mutex::Autolock locker(&mObjectLock);
    mMessageEnabler &= ~msg_type;
    LOGV("**** Currently enabled messages:");
    PrintMessages(mMessageEnabler);
}

void CallbackNotifier::enableSmartMessage(uint msg_type)
{
    LOGV("%s: msg_type = 0x%x", __FUNCTION__, msg_type);
    PrintMessages(msg_type);

    Mutex::Autolock locker(&mObjectLock);
    mSmartMessageEnabler |= msg_type;
    LOGV("**** Currently enabled smart detection messages:");
    PrintMessages(mSmartMessageEnabler);
}

void CallbackNotifier::disableSmartMessage(uint msg_type)
{
    LOGV("%s: msg_type = 0x%x", __FUNCTION__, msg_type);
    PrintMessages(msg_type);

    Mutex::Autolock locker(&mObjectLock);
    mSmartMessageEnabler &= ~msg_type;
    LOGV("**** Currently disabled smart detection messages:");
    PrintMessages(mSmartMessageEnabler);
}

status_t CallbackNotifier::enableVideoRecording()
{
    F_LOG;
    for(int index = 0;index <NB_BUFFER; index++)
    {
        mCam_buff[index] = mGetMemoryCB(-1, sizeof(VideoNativeHandleMetadata), 1, mCallbackCookie);
        LOGV("mCam_buff[%d]->data = 0x%x !",index,(unsigned int)mCam_buff[index]->data);
    }
    mVideoRecEnabled = true;
    return NO_ERROR;
}

void CallbackNotifier::disableVideoRecording()
{
    F_LOG;
    if(mVideoRecEnabled == true)
    {
        mVideoRecEnabled = false;
        for(int index = 0;index <NB_BUFFER; index++)
        {
            mCam_buff[index]->release(mCam_buff[index]);
            LOGV("mCam_buff[%d]->data = 0x%x !",index,(unsigned int)mCam_buff[index]->data);
        }
    }
}

status_t CallbackNotifier::storeMetaDataInBuffers(bool enable)
{
    /* Return INVALID_OPERATION means HAL does not support metadata. So HAL will
     * return actual frame data with CAMERA_MSG_VIDEO_FRAME. Return
     * INVALID_OPERATION to mean metadata is not supported. */
    return UNKNOWN_ERROR;
}

/****************************************************************************
 * Public API
 ***************************************************************************/

void CallbackNotifier::cleanupCBNotifier()
{
    if (mSavePictureThread != NULL)
    {
        mSavePictureThread->stopThread();
        pthread_cond_signal(&mSavePictureCond);
        pthread_cond_signal(&mPictureFdCond);
        pthread_mutex_lock(&mSavePictureMutex);
        if (!mSaveThreadExited)
        {
            pthread_cond_wait(&mSavePictureCond, &mSavePictureMutex);
        }
        pthread_mutex_unlock(&mSavePictureMutex);

        mSavePictureThread.clear();
        mSavePictureThread = 0;

        pthread_mutex_destroy(&mSavePictureMutex);
        pthread_cond_destroy(&mSavePictureCond);
    }
    pthread_mutex_destroy(&mPictureFdMutex);
    pthread_cond_destroy(&mPictureFdCond);
    pthread_mutex_destroy(&mCountMutex);

    Mutex::Autolock locker(&mObjectLock);
    mMessageEnabler = 0;
    mNotifyCB = NULL;
    mDataCB = NULL;
    mDataCBTimestamp = NULL;
    mGetMemoryCB = NULL;
    mCallbackCookie = NULL;
    mVideoRecEnabled = false;
    mJpegQuality = 90;
    mJpegRotate = 0;
    mPictureWidth = 640;
    mPictureHeight = 480;
    mThumbWidth = 0;
    mThumbHeight = 0;
    mGpsLatitude = 0.0;
    mGpsLongitude = 0.0;
    mGpsAltitude = 0.0;
    mGpsTimestamp = 0;
    mFocalLength = 0.0;
    mWhiteBalance = 0;

    if (mBufferList != NULL)
    {
        delete mBufferList;
        mBufferList = NULL;
    }
}

void CallbackNotifier::onNextFrameAvailable(const void* frame,
                                            bool hw)
{
    V4L2BUF_t * pbuf = (V4L2BUF_t*)frame;

    if (hw) {
        onNextFrameHW(frame);
    } else {
        onNextFrameSW(frame);
    }
}


void CallbackNotifier::onNextFrameAvailable(buffer_handle_t* handlebuffer,
                                              const void* frame,int index)
{
    if (isMessageEnabled(CAMERA_MSG_VIDEO_FRAME)) {
        onNextFrameHW(handlebuffer,frame,index);
    }
    if (isMessageEnabled(CAMERA_MSG_PREVIEW_FRAME)) {
        onNextFrameSW(frame);
    }
}

bool CallbackNotifier::takePicture(const void* frame, void *memOpsS,bool is_continuous)
{
    F_LOG;
    buffer_node * pNode = NULL;
    V4L2BUF_t * pbuf = (V4L2BUF_t *)frame;
    void * pOutBuf = NULL;
    int bufSize = 0;

    int src_format = 0;
    unsigned long src_addr_phy = 0;
    unsigned long src_addr_vir = 0;
    int src_width = 0;
    int src_height = 0;
    RECT_t src_crop;

    DBG_TIME_BEGIN("CallbackNotifier taking picture", 0);
    if ((pbuf->isThumbAvailable == 1)
        && (pbuf->thumbUsedForPhoto == 1))
    {
        src_format            = pbuf->thumbFormat;
        src_addr_phy        = pbuf->thumbAddrPhyY;
        src_addr_vir        = pbuf->thumbAddrVirY;
        src_width            = pbuf->thumbWidth;
        src_height            = pbuf->thumbHeight;
        memcpy((void*)&src_crop, (void*)&pbuf->thumb_crop_rect, sizeof(RECT_t));
    }
    else
    {
        src_format            = pbuf->format;
        src_addr_phy        = pbuf->addrPhyY;
        src_addr_vir        = pbuf->addrVirY;
        src_width            = pbuf->width;
        src_height            = pbuf->height;
        memcpy((void*)&src_crop, (void*)&pbuf->crop_rect, sizeof(RECT_t));
    }

    JPEG_ENC_t jpeg_enc;
    memset(&jpeg_enc, 0, sizeof(jpeg_enc));
    jpeg_enc.addrY            = src_addr_phy;
    jpeg_enc.addrC            = src_addr_phy + ALIGN_16B(src_width) * src_height;
    jpeg_enc.src_w            = src_width;
    jpeg_enc.src_h            = src_height;
    jpeg_enc.pic_w            = mPictureWidth;
    jpeg_enc.pic_h            = mPictureHeight;
    jpeg_enc.colorFormat    = (src_format == V4L2_PIX_FMT_NV21) ? JPEG_COLOR_YUV420_NV21 : JPEG_COLOR_YUV420_NV12;
    jpeg_enc.quality        = mJpegQuality;
    jpeg_enc.rotate            = mJpegRotate;

    getCurrentDateTime();

    strcpy(jpeg_enc.CameraMake, mExifMake);
    strcpy(jpeg_enc.CameraModel, mExifModel);
    strcpy(jpeg_enc.DateTime, mDateTime);

    jpeg_enc.thumbWidth        = mThumbWidth;
    jpeg_enc.thumbHeight    = mThumbHeight;
    jpeg_enc.whitebalance   = mWhiteBalance;
    jpeg_enc.focal_length    = mFocalLength;

    if (0 != strlen(mGpsMethod))
    {
        jpeg_enc.enable_gps            = 1;
        jpeg_enc.gps_latitude        = mGpsLatitude;
        jpeg_enc.gps_longitude        = mGpsLongitude;
        jpeg_enc.gps_altitude        = mGpsAltitude;
        jpeg_enc.gps_timestamp        = mGpsTimestamp;
        strcpy(jpeg_enc.gps_processing_method, mGpsMethod);
        //memset(mGpsMethod, 0, sizeof(mGpsMethod));
    }
    else
    {
        jpeg_enc.enable_gps            = 0;
    }

    if ((src_crop.width != jpeg_enc.src_w)
        || (src_crop.height != jpeg_enc.src_h))
    {
        jpeg_enc.enable_crop        = 1;
        jpeg_enc.crop_x                = src_crop.left;
        jpeg_enc.crop_y                = src_crop.top;
        jpeg_enc.crop_w                = src_crop.width;
        jpeg_enc.crop_h                = src_crop.height;
    }
    else
    {
        jpeg_enc.enable_crop        = 0;
    }

    LOGD("addrY: %x, src: %dx%d, pic: %dx%d, quality: %d, rotate: %d, Gps method: %s,\
        thumbW: %d, thumbH: %d, thubmFactor: %d, crop: [%d, %d, %d, %d]",
        jpeg_enc.addrY,
        jpeg_enc.src_w, jpeg_enc.src_h,
        jpeg_enc.pic_w, jpeg_enc.pic_h,
        jpeg_enc.quality, jpeg_enc.rotate,
        jpeg_enc.gps_processing_method,
        jpeg_enc.thumbWidth,
        jpeg_enc.thumbHeight,
        jpeg_enc.scale_factor,
        jpeg_enc.crop_x,
        jpeg_enc.crop_y,
        jpeg_enc.crop_w,
        jpeg_enc.crop_h);

    pNode = mBufferList->allocBuffer(-1, mPictureWidth * mPictureHeight);
    if (pNode == NULL)
    {
        LOGE("malloc picture node failed");
        return false;
    }
    pOutBuf = pNode->data;
    if (pOutBuf == NULL)
    {
        LOGE("malloc picture memory failed");
        return false;
    }

    JpegEncInfo sjpegInfo;
    EXIFInfo   exifInfo;

    memset(&sjpegInfo, 0, sizeof(JpegEncInfo));
    memset(&exifInfo, 0, sizeof(EXIFInfo));

    sjpegInfo.sBaseInfo.nStride = src_width;
    sjpegInfo.sBaseInfo.nInputWidth = src_width;
    sjpegInfo.sBaseInfo.nInputHeight = src_height;
    sjpegInfo.sBaseInfo.nDstWidth = mPictureWidth;
    sjpegInfo.sBaseInfo.nDstHeight = mPictureHeight;
    sjpegInfo.sBaseInfo.memops = (ScMemOpsS*)memOpsS;
    sjpegInfo.pAddrPhyY = (unsigned char*)src_addr_phy;
    sjpegInfo.pAddrPhyC = (unsigned char*)src_addr_phy + ALIGN_16B(src_width) * src_height;
    sjpegInfo.sBaseInfo.eInputFormat = (src_format == V4L2_PIX_FMT_NV21) ? VENC_PIXEL_YVU420SP: VENC_PIXEL_YUV420SP;
    sjpegInfo.quality        = mJpegQuality;
    exifInfo.Orientation    = mJpegRotate;
    sjpegInfo.nShareBufFd = pbuf->nShareBufFd;


    if ((((unsigned int)src_crop.width ) != sjpegInfo.sBaseInfo.nInputWidth)
        || (src_crop.height != sjpegInfo.sBaseInfo.nInputHeight))
    {
        sjpegInfo.bEnableCorp        = 1;
        sjpegInfo.sCropInfo.nLeft    = src_crop.left;
        sjpegInfo.sCropInfo.nTop    = src_crop.top;
        sjpegInfo.sCropInfo.nWidth    = src_crop.width;
        sjpegInfo.sCropInfo.nHeight    = src_crop.height;
    }
    else
    {
        sjpegInfo.bEnableCorp        = 0;
    }

    exifInfo.ThumbWidth = mThumbWidth;
    exifInfo.ThumbHeight = mThumbHeight;

    LOGD("addrY: %x, src: %dx%d, pic: %dx%d, quality: %d, rotate: %d,\
        thumbW: %d, thumbH: %d,EnableCorp: %d,crop: [%d, %d, %d, %d],share_fd:%d",
        sjpegInfo.pAddrPhyY,
        sjpegInfo.sBaseInfo.nInputWidth, sjpegInfo.sBaseInfo.nInputHeight,
        sjpegInfo.sBaseInfo.nDstWidth, sjpegInfo.sBaseInfo.nDstHeight,
        sjpegInfo.quality, exifInfo.Orientation,
        exifInfo.ThumbWidth,
        exifInfo.ThumbHeight,
        sjpegInfo.bEnableCorp,
        sjpegInfo.sCropInfo.nLeft,
        sjpegInfo.sCropInfo.nTop,
        sjpegInfo.sCropInfo.nWidth,
        sjpegInfo.sCropInfo.nHeight,sjpegInfo.nShareBufFd);

    strcpy((char*)exifInfo.CameraMake,    mExifMake);
    strcpy((char*)exifInfo.CameraModel,    mExifModel);
    strcpy((char*)exifInfo.DateTime, mDateTime);
    LOGD("mGpsMethod:%s,mGpsLatitude:%lf,mGpsLongitude:%lf,mGpsAltitude:%lf,mGpsTimestamp:%ld", \
        mGpsMethod,
        mGpsLatitude,
        mGpsLongitude,
        mGpsAltitude,
        mGpsTimestamp);

    if (0 != strlen(mGpsMethod)){
        strcpy((char*)exifInfo.gpsProcessingMethod,mGpsMethod);
        exifInfo.enableGpsInfo = 1;
        exifInfo.gps_latitude = mGpsLatitude;
        exifInfo.gps_longitude = mGpsLongitude;
        exifInfo.gps_altitude = mGpsAltitude;
        exifInfo.gps_timestamp = mGpsTimestamp;
        memset(mGpsMethod, 0, sizeof(mGpsMethod));

    }
    else
        exifInfo.enableGpsInfo = 0;

    exifInfo.ExposureTime.num = mExposureTime.num;
    exifInfo.ExposureTime.den = mExposureTime.den;

    exifInfo.FNumber.num = mFNumber.num;
    exifInfo.FNumber.den = mFNumber.den;
    exifInfo.ISOSpeed = mISOSpeed;

    exifInfo.ExposureBiasValue.num= mExposureBiasValue.num;
    exifInfo.ExposureBiasValue.den= mExposureBiasValue.den;

    exifInfo.MeteringMode = mMeteringMode;
    exifInfo.FlashUsed = mFlashUsed;;

    exifInfo.FocalLength.num = mFocalLength_r.num;
    exifInfo.FocalLength.den = mFocalLength_r.den;

    exifInfo.DigitalZoomRatio.num = mDigitalZoomRatio.num;
    exifInfo.DigitalZoomRatio.den = mDigitalZoomRatio.den;

    exifInfo.WhiteBalance = mWhiteBalance;
    exifInfo.ExposureMode = mExposureMode;

    int ret = AWJpecEnc(&sjpegInfo,&exifInfo,pOutBuf,&bufSize);
    //int64_t lasttime = systemTime();
    if (ret < 0)
    {
        LOGE("JpegEnc failed");
        return false;
    }
    //LOGV("hw enc time: %lld(ms), size: %d", (systemTime() - lasttime)/1000000, bufSize);

    DBG_TIME_DIFF("enc");

    if (is_continuous)
    {
        pNode->id = mSavePictureCnt;
        pNode->size = bufSize;
        mBufferList->push(pNode);

        // cb number of pictures
        /*if (isMessageEnabled(CAMERA_MSG_CONTINUOUSSNAP))
        {
            mNotifyCB(CAMERA_MSG_CONTINUOUSSNAP, mSavePictureCnt, 0, mCallbackCookie);
        }*/

        pthread_cond_signal(&mSavePictureCond);

        mSavePictureCnt++;
    }
    else
    {
        if ((strlen(mSnapPath) > 0))
        {
            camera_memory_t* cb_buff;
            strcpy(pNode->priv, mSnapPath);
            pNode->id = -1;
            pNode->size = bufSize;
            mBufferList->push(pNode);
            //mNotifyCB(CAMERA_MSG_SNAP, 0, 0, mCallbackCookie);
            pthread_cond_signal(&mSavePictureCond);
        }
        else
        {
            camera_memory_t* jpeg_buff = mGetMemoryCB(-1, bufSize, 1, mCallbackCookie);
            if (NULL != jpeg_buff && NULL != jpeg_buff->data)
            {
                memcpy(jpeg_buff->data, (uint8_t *)pOutBuf, bufSize);
                mDataCB(CAMERA_MSG_COMPRESSED_IMAGE, jpeg_buff, 0, NULL, mCallbackCookie);
                jpeg_buff->release(jpeg_buff);
            }
            else
            {
                LOGE("%s: Memory failure in CAMERA_MSG_COMPRESSED_IMAGE", __FUNCTION__);
            }

            mBufferList->releaseBuffer(pNode);
        }
    }

    DBG_TIME_DIFF("photo end");
    LOGV("taking photo end");
    return true;
}

void CallbackNotifier::onNextFrameHW(const void* frame)
{
    V4L2BUF_t * pbuf = (V4L2BUF_t*)frame;
    VencInputBuffer sInputBuffer;

    int buffer_type = kMetadataBufferTypeCameraSource;//matadataType
    memset(&sInputBuffer, 0, sizeof(VencInputBuffer));

    sInputBuffer.pAddrVirY = (unsigned char*)pbuf->addrVirY;
    sInputBuffer.pAddrVirC = (unsigned char*)pbuf->addrVirC;
    sInputBuffer.pAddrPhyY = (unsigned char*)pbuf->addrPhyY;
    sInputBuffer.pAddrPhyC = (unsigned char*)pbuf->addrPhyC;
    sInputBuffer.nID = pbuf->index;
    sInputBuffer.nPts = pbuf->timeStamp;

//fengyun add start
#if 0
    if ((pbuf->bytesused > 0) && (SONIX_CAMERA_H264 == 1)) {
        sInputBuffer.nFlag = pbuf->bytesused;
    }
#endif
//fengyun add end
    if (pbuf->width == pbuf->crop_rect.width || pbuf->height== pbuf->crop_rect.height) {
        sInputBuffer.bEnableCorp = 0;
    } else {
        sInputBuffer.bEnableCorp = 1;
        sInputBuffer.sCropInfo.nLeft = pbuf->crop_rect.left;
        sInputBuffer.sCropInfo.nTop = pbuf->crop_rect.top;
        sInputBuffer.sCropInfo.nWidth  = pbuf->crop_rect.width;
        sInputBuffer.sCropInfo.nHeight = pbuf->crop_rect.height;
    }
    if (isMessageEnabled(CAMERA_MSG_VIDEO_FRAME) && isVideoRecordingEnabled()) {
        camera_memory_t* cam_buff = mGetMemoryCB(-1, (sizeof(VencInputBuffer) + 4), 1, mCallbackCookie);
        if (NULL != cam_buff && NULL != cam_buff->data) {
            /*pbuf->refMutex.lock();
            pbuf->refCnt++;
            pbuf->refMutex.unlock();*/
            memcpy((unsigned char *)cam_buff->data, &buffer_type, 4);
            memcpy((unsigned char *)cam_buff->data + 4, &sInputBuffer, sizeof(VencInputBuffer));

            mDataCBTimestamp(pbuf->timeStamp, CAMERA_MSG_VIDEO_FRAME,
                               cam_buff, 0, mCallbackCookie);
            cam_buff->release(cam_buff);
        } else {
            LOGE("%s: Memory failure in CAMERA_MSG_VIDEO_FRAME", __FUNCTION__);
        }
    }
}

int CallbackNotifier::releaseRecordingFrameLock()
{
    return pthread_mutex_trylock(&mCountMutex);
}

void CallbackNotifier::releaseRecordingFrameCount()
{
    mCount--;
}

void CallbackNotifier::releaseRecordingFrameUnLock()
{
    pthread_mutex_unlock(&mCountMutex);
}

void CallbackNotifier::onNextFrameHW(buffer_handle_t* handlebuffer,const void* frame,int index)
{
    F_LOG;
    VideoNativeHandleMetadata mVideoNativeMetadata;
    V4L2BUF_t * pbuf = (V4L2BUF_t*)frame;

    mVideoNativeMetadata.eType = kMetadataBufferTypeNativeHandleSource;
    mVideoNativeMetadata.pHandle = (native_handle_t *)(*handlebuffer);
    int try_num = 20;
    int lock_status;

    if (isMessageEnabled(CAMERA_MSG_VIDEO_FRAME) && isVideoRecordingEnabled())
    {
        if (NULL != mCam_buff[index] && NULL != mCam_buff[index]->data)
        {
            pbuf->refMutex.lock();
            pbuf->refCnt++;
            pbuf->refMutex.unlock();

            while(mCount != 0 && try_num--)
            {
                LOGW("mCount != 0,wait for a minute!");
                usleep(5000);
            }

CONTINUE_COUNT:
            lock_status = pthread_mutex_trylock(&mCountMutex);
            if(lock_status == 0)
            {
                LOGV("index = %d ,mCam_buff[index]->data = 0x%x,pbuf->refCnt = %d",index,(unsigned int)mCam_buff[index]->data,pbuf->refCnt);
                memcpy(mCam_buff[index]->data, &mVideoNativeMetadata, sizeof(VideoNativeHandleMetadata));
                LOGV("CallbackNotifier::onNextFrameHW index = %d,mVideoNativeMetadata.pHandle = 0x%x,mCam_buff[%d]->data = 0x%x",
                                  index,
                                  (unsigned int)mVideoNativeMetadata.pHandle,
                                  index,
                                  (unsigned int)mCam_buff[index]->data);
                mDataCBTimestamp(pbuf->timeStamp, CAMERA_MSG_VIDEO_FRAME,
                                  mCam_buff[index], 0, mCallbackCookie);

                if(try_num)
                {
                    mCount++;
                }
                pthread_mutex_unlock(&mCountMutex);
            }
            else
            {
                LOGW("pthread_mutex_trylock error! Wait for a minute!");
                usleep(5000);
                goto CONTINUE_COUNT;
            }
        }
        else
        {
            LOGE("%s: Memory failure in CAMERA_MSG_VIDEO_FRAME", __FUNCTION__);
        }
    }
    F_LOG;
}

void CallbackNotifier::onNextFrameSW(const void* frame)
{
    V4L2BUF_t * pbuf = (V4L2BUF_t*)frame;
    int framesize =0;
    int src_format = 0;
    unsigned long src_addr_phy = 0;
    unsigned long src_addr_vir = 0;
    int src_width = 0;
    int src_height = 0;
    RECT_t src_crop;

    if ((pbuf->isThumbAvailable == 1)
        && (pbuf->thumbUsedForPreview == 1))
    {
        src_format            = pbuf->thumbFormat;
        src_addr_phy        = pbuf->thumbAddrPhyY;
        src_addr_vir        = pbuf->thumbAddrVirY;
        src_width            = pbuf->thumbWidth;
        src_height            = pbuf->thumbHeight;
        memcpy((void*)&src_crop, (void*)&pbuf->thumb_crop_rect, sizeof(RECT_t));
    }
    else
    {
        src_format            = pbuf->format;
        src_addr_phy        = pbuf->addrPhyY;
        src_addr_vir        = pbuf->addrVirY;
        src_width            = pbuf->width;
        src_height            = pbuf->height;
        memcpy((void*)&src_crop, (void*)&pbuf->crop_rect, sizeof(RECT_t));
    }

    framesize = ALIGN_16B(src_width) * src_height * 3/2;
    if (isMessageEnabled(CAMERA_MSG_VIDEO_FRAME) && isVideoRecordingEnabled())
    {
        camera_memory_t* cam_buff = mGetMemoryCB(-1, framesize, 1, mCallbackCookie);
        if (NULL != cam_buff && NULL != cam_buff->data)
        {
            memcpy(cam_buff->data, (void *)src_addr_vir, framesize);
            mDataCBTimestamp(pbuf->timeStamp, CAMERA_MSG_VIDEO_FRAME,
                               cam_buff, 0, mCallbackCookie);
            cam_buff->release(cam_buff);        // star add
        } else {
            LOGE("%s: Memory failure in CAMERA_MSG_VIDEO_FRAME", __FUNCTION__);
        }
    }

    if (isMessageEnabled(CAMERA_MSG_PREVIEW_FRAME))
    {
        bool isYV12 = false;
        if (src_format == V4L2_PIX_FMT_YUV420 ) {
            LOGV("src format is YUV420");
            isYV12 = true;
        }
        if (src_format == V4L2_PIX_FMT_YVU420) {
            LOGV("src format is YVU420");
            isYV12 = true;
        }
        if (strcmp(mCallingProcessName, "com.android.facelock") == 0) {
            camera_memory_t* cam_buff = mGetMemoryCB(-1, 160 * 120 * 3 / 2, 1, mCallbackCookie);
            if (NULL != cam_buff && NULL != cam_buff->data) {
                yuv420spDownScale((void*)src_addr_vir, cam_buff->data,
                                ALIGN_16B(src_width), src_height,
                                160, 120);
                mDataCB(CAMERA_MSG_PREVIEW_FRAME, cam_buff, 0, NULL, mCallbackCookie);
                cam_buff->release(cam_buff);
            } else {
                LOGE("%s: Memory failure in CAMERA_MSG_PREVIEW_FRAME", __FUNCTION__);
            }
        } else if ((mCBWidth % 32 != 0) && isYV12) { // add for cts verify camera formats 176x144 YV12
                int w = mCBWidth;
                int h = mCBHeight;
                int yStride = ALIGN_16B(w);
                int uvStride = ALIGN_16B(w >> 1);
                int ySize = yStride * h;
                int uvSize = uvStride * h / 2;
                int reqBufSize = ySize+ uvSize*2;
                LOGV("ystride : %d, uvstride : %d, size : %d ",yStride,uvStride,reqBufSize);
                camera_memory_t* cam_buff = mGetMemoryCB(-1, reqBufSize, 1, mCallbackCookie);
                if (NULL != cam_buff && NULL != cam_buff->data) {
                    yuv420pDownScale_align((void*)src_addr_vir,cam_buff->data,
                        ALIGN_16B(src_width), src_height,w,h,yStride,uvStride);
                    mDataCB(CAMERA_MSG_PREVIEW_FRAME, cam_buff, 0, NULL, mCallbackCookie);
                    cam_buff->release(cam_buff);
                } else {
                    LOGE("%s: Memory failure in CAMERA_MSG_PREVIEW_FRAME", __FUNCTION__);
                }
        } else {
            camera_memory_t* cam_buff = mGetMemoryCB(-1, mCBWidth * mCBHeight * 3 / 2, 1, mCallbackCookie);
            if (NULL != cam_buff && NULL != cam_buff->data) {
                if ( src_width % mCBWidth == 0 &&  src_height % mCBHeight == 0)
                yuv420spDownScale((void*)src_addr_vir, cam_buff->data,
                                ALIGN_16B(src_width), src_height,
                                mCBWidth, mCBHeight);
                else
                yuv420spDown_noneScale((void*)src_addr_vir, cam_buff->data,
                                    ALIGN_16B(src_width), src_height,
                                    mCBWidth, mCBHeight);
                if (src_format == V4L2_PIX_FMT_NV12)
                {
                    // NV12 <--> NV21
                    /*formatToNV21(cam_buff->data,
                    cam_buff->data,
                    mCBWidth,
                    mCBHeight,
                    ALIGN_16B(mCBWidth),
                    0,
                    2,
                    ALIGN_16B(mCBWidth) * mCBHeight * 3/2,
                    src_format);*/
                }
                mDataCB(CAMERA_MSG_PREVIEW_FRAME, cam_buff, 0, NULL, mCallbackCookie);
                cam_buff->release(cam_buff);
            }
            else
            {
                LOGE("%s: Memory failure in CAMERA_MSG_PREVIEW_FRAME", __FUNCTION__);
            }
        }
    }
}

status_t CallbackNotifier::autoFocusMsg(bool success)
{
    if (isMessageEnabled(CAMERA_MSG_FOCUS))
    {
        mNotifyCB(CAMERA_MSG_FOCUS, success, 0, mCallbackCookie);
    }
    return NO_ERROR;
}

status_t CallbackNotifier::autoFocusContinuousMsg(bool success)
{
    if (isMessageEnabled(CAMERA_MSG_FOCUS_MOVE))
    {
        // in continuous focus mode
        // true for starting focus, false for focus ok
        mNotifyCB(CAMERA_MSG_FOCUS_MOVE, success, 0, mCallbackCookie);
    }
    return NO_ERROR;
}

status_t CallbackNotifier::faceDetectionMsg(camera_frame_metadata_t *face)
{
    if (isMessageEnabled(CAMERA_MSG_PREVIEW_METADATA))
    {
        camera_memory_t *cam_buff = mGetMemoryCB(-1, 1, 1, mCallbackCookie);
        mDataCB(CAMERA_MSG_PREVIEW_METADATA, cam_buff, 0, face, mCallbackCookie);
        cam_buff->release(cam_buff);
    }
    return NO_ERROR;
}

status_t CallbackNotifier::smartDetectionMsg(int32_t type)
{

    /*if (isSmartMessageEnabled(CAMERA_SMART_MSG_STATUS))
    {
        mNotifyCB(CAMERA_SMART_MSG_STATUS, type, 0, mCallbackCookie);
    }*/
    return NO_ERROR;

}
void CallbackNotifier::notifyPictureMsg(const void* frame)
{
    F_LOG;

    V4L2BUF_t * pbuf = (V4L2BUF_t*)frame;
    int framesize = pbuf->width * pbuf->height * 3/2;

    // shutter msg
    if (isMessageEnabled(CAMERA_MSG_SHUTTER))
    {
        F_LOG;
        mNotifyCB(CAMERA_MSG_SHUTTER, 0, 0, mCallbackCookie);
    }

    // raw image msg
    if (isMessageEnabled(CAMERA_MSG_RAW_IMAGE))
    {
        camera_memory_t *dummyRaw = mGetMemoryCB(-1, 1, 1, mCallbackCookie);
        if ( NULL == dummyRaw )
        {
            LOGE("%s: Memory failure in CAMERA_MSG_PREVIEW_FRAME", __FUNCTION__);
            return;
        }
        mDataCB(CAMERA_MSG_RAW_IMAGE, dummyRaw, 0, NULL, mCallbackCookie);
        dummyRaw->release(dummyRaw);
    }
    else if (isMessageEnabled(CAMERA_MSG_RAW_IMAGE_NOTIFY))
    {
        mNotifyCB(CAMERA_MSG_RAW_IMAGE_NOTIFY, 0, 0, mCallbackCookie);
    }

    // postview msg
    if (0 && isMessageEnabled(CAMERA_MSG_POSTVIEW_FRAME) )
    {
        F_LOG;
        camera_memory_t* cam_buff = mGetMemoryCB(-1, framesize, 1, mCallbackCookie);
        if (NULL != cam_buff && NULL != cam_buff->data)
        {
            memset(cam_buff->data, 0xff, framesize);
            mDataCB(CAMERA_MSG_POSTVIEW_FRAME, cam_buff, 0, NULL, mCallbackCookie);
            cam_buff->release(cam_buff);
        }
        else
        {
            LOGE("%s: Memory failure in CAMERA_MSG_PREVIEW_FRAME", __FUNCTION__);
            return;
        }
    }
}

void CallbackNotifier::startContinuousPicture()
{
    F_LOG;

    //
    mSavePictureCnt = 0;
}

void CallbackNotifier::stopContinuousPicture()
{
    // do nothing
}

void CallbackNotifier::setContinuousPictureCnt(int cnt)
{
    mSavePictureMax = cnt;
}

void CallbackNotifier::setFd(int fd)
{
    if (mIsSinglePicture)
    {
        pthread_mutex_lock(&mPictureFdMutex);
        mFd = fd;
        pthread_cond_signal(&mPictureFdCond);
        pthread_mutex_unlock(&mPictureFdMutex);
    }
    else
    {
        pthread_mutex_lock(&mPictureFdMutex);
        mContinuousFd[(mContinuousFdIndex++)%10] = fd;
        pthread_cond_signal(&mPictureFdCond);
        pthread_mutex_unlock(&mPictureFdMutex);
    }
}

bool CallbackNotifier::savePictureThread()
{
    int wait_cnt = 5;
    int wait_time = 3;        // 3s * 5 = 15s

    if (mIsSinglePicture)
    {
        pthread_mutex_lock(&mSavePictureMutex);
        if (mSavePictureThread->getThreadStatus() == THREAD_STATE_EXIT)
        {
            mSaveThreadExited = true;
            pthread_mutex_unlock(&mSavePictureMutex);

            pthread_cond_signal(&mSavePictureCond);

            LOGD("savePictureThread exit, line: %d", __LINE__);
            return false;
        }

        if (mBufferList->isListEmpty())
        {
            LOGV("wait for picture to save");
            pthread_cond_wait(&mSavePictureCond, &mSavePictureMutex);
            pthread_mutex_unlock(&mSavePictureMutex);
            return true;
        }
        pthread_mutex_unlock(&mSavePictureMutex);
    }
    else
    {
        if (mBufferList->isListEmpty())
        {
            pthread_mutex_lock(&mSavePictureMutex);

            if (mSavePictureThread->getThreadStatus() == THREAD_STATE_EXIT)
            {
                mSaveThreadExited = true;
                pthread_mutex_unlock(&mSavePictureMutex);

                pthread_cond_signal(&mSavePictureCond);

                LOGD("savePictureThread exit, line: %d", __LINE__);
                return false;
            }

            LOGV("wait for picture to save");
            pthread_cond_wait(&mSavePictureCond, &mSavePictureMutex);
            pthread_mutex_unlock(&mSavePictureMutex);
            return true;
        }
    }

    ALOGV("%d items left in the picture list.", mBufferList->getItemCnt());

    DBG_TIME_BEGIN("save picture", 0);

    char fname[128];
    FILE *pf = NULL;
    buffer_node * pNode = mBufferList->pop();
    if (pNode == NULL)
    {
        LOGE("list head is null");
        goto SAVE_PICTURE_END;
    }

    if (mIsSinglePicture)
    {
        // callback fd
        camera_memory_t* cb_buff;
        if (pNode->id >= 0)
    {
            sprintf(pNode->priv, "%s%03d.jpg", mFolderPath, pNode->id);
        }
        cb_buff = mGetMemoryCB(-1, strlen(pNode->priv), 1, mCallbackCookie);
        if (NULL != cb_buff && NULL != cb_buff->data)
        {
            memcpy(cb_buff->data, (uint8_t *)pNode->priv, strlen(pNode->priv));
            //mDataCB(CAMERA_MSG_SNAP_FD, cb_buff, 0, NULL, mCallbackCookie);
            cb_buff->release(cb_buff);
        }
        else
        {
            LOGE("%s: Memory failure in CAMERA_MSG_SNAP_FD", __FUNCTION__);
            goto SAVE_PICTURE_END;
        }

        // wait fd
        while(wait_cnt-- >= 0)
        {
            pthread_mutex_lock(&mPictureFdMutex);
            if (mFd <= 0)
            {
                struct timespec timeout;
                timeout.tv_sec=time(0) + wait_time;        // 3s timeout
                timeout.tv_nsec = 0;
                int ret = pthread_cond_timedwait(&mPictureFdCond, &mPictureFdMutex, &timeout);
                if (ret == ETIMEDOUT)
                {
                    LOGW("wait fd timeout");
                    pthread_mutex_unlock(&mPictureFdMutex);

                    pthread_mutex_lock(&mSavePictureMutex);
                    if (mSavePictureThread->getThreadStatus() == THREAD_STATE_EXIT)
                    {
                        mSaveThreadExited = true;
                        pthread_mutex_unlock(&mSavePictureMutex);

                        pthread_cond_signal(&mSavePictureCond);

                        LOGD("savePictureThread exit, line: %d", __LINE__);
                        return false;
                    }
                    pthread_mutex_unlock(&mSavePictureMutex);
                    continue;
                }
            }
            pNode->fd = mFd;
            mFd = 0;
            pthread_mutex_unlock(&mPictureFdMutex);
            break;
        }

        if (wait_cnt < 0 || pNode->fd <= 0)
        {
            LOGE("single picture wait fd failed");
            goto SAVE_PICTURE_END;
        }
    }
    else
    {
        // continuous picture wait fd
        pthread_mutex_lock(&mPictureFdMutex);
        if (mContinuousFd[pNode->id] <= 0)
        {
            struct timespec timeout;
            timeout.tv_sec=time(0) + 3;        // 3s timeout
            timeout.tv_nsec = 0;
            int ret = pthread_cond_timedwait(&mPictureFdCond, &mPictureFdMutex, &timeout);
            if (ret == ETIMEDOUT)
            {
                LOGW("wait fd timeout");
                pthread_mutex_unlock(&mPictureFdMutex);
                goto SAVE_PICTURE_END;
            }
        }
        pNode->fd = mContinuousFd[pNode->id];
        mContinuousFd[pNode->id] = 0;
        pthread_mutex_unlock(&mPictureFdMutex);

        if (pNode->fd <= 0)
        {
            LOGD("savePictureThread exit, line: %d", __LINE__);
            goto SAVE_PICTURE_END;
        }
    }
    // write
    if (pNode->id >= 0)
    {
        if(pNode->fd > 0)
        {
            int err = ::write(pNode->fd, (uint8_t *)pNode->data, pNode->size);
            if(err == -1)
            {
                LOGE("write failed: %s", strerror(errno));
                ::close(pNode->fd);
                goto SAVE_PICTURE_END;
            }
            ::close(pNode->fd);
            /*if (isMessageEnabled(CAMERA_MSG_CONTINUOUSSNAP))
            {
                mNotifyCB(CAMERA_MSG_CONTINUOUSSNAP, (pNode->id + 1000), 0, mCallbackCookie);
            }*/
        }
        else
        {
            sprintf(fname, "%s%03d.jpg", mFolderPath, pNode->id);
    pf = fopen(fname, "wb+");
    if (pf != NULL)
    {
        LOGV("open %s ok", fname);
        fwrite((uint8_t *)pNode->data, pNode->size, 1, pf);
        fflush(pf);
        fclose(pf);
    }
    else
    {
        LOGE("open %s failed, %s", fname, strerror(errno));
                goto SAVE_PICTURE_END;
            }
        }
    }
    else
    {
        if(pNode->fd > 0)
        {
            int err = ::write(pNode->fd, (uint8_t *)pNode->data, pNode->size);
            if(err == -1)
            {
                LOGE("write failed: %s", strerror(errno));
                ::close(pNode->fd);
                goto SAVE_PICTURE_END;
            }
            ::close(pNode->fd);
        }
        else
        {
            strcpy(fname, pNode->priv);
            pf = fopen(fname, "wb+");
            if (pf != NULL)
            {
                LOGV("open %s ok", fname);
                fwrite((uint8_t *)pNode->data, pNode->size, 1, pf);
                fflush(pf);
                fclose(pf);
            }
            else
            {
                LOGE("open %s failed, %s", fname, strerror(errno));
                goto SAVE_PICTURE_END;
            }
        }
    }

    DBG_TIME_DIFF("write file");

    if (pNode->id < 0)
    {
        camera_memory_t* cb_buff;
        cb_buff = mGetMemoryCB(-1, strlen(pNode->priv), 1, mCallbackCookie);
        if (NULL != cb_buff && NULL != cb_buff->data)
        {
            memcpy(cb_buff->data, (uint8_t *)pNode->priv, strlen(pNode->priv));
            //mDataCB(CAMERA_MSG_SNAP_THUMB, cb_buff, 0, NULL, mCallbackCookie);
            cb_buff->release(cb_buff);
        }
        else
        {
            LOGE("%s: Memory failure in CAMERA_MSG_SNAP_THUMB", __FUNCTION__);
        }
    }

SAVE_PICTURE_END:
    if (pNode != NULL)
    {
        mBufferList->releaseBuffer(pNode);
    }

    return true;
}


void CallbackNotifier::getCurrentDateTime()
{
    time_t t;
    struct tm *tm_t;
    time(&t);
    tm_t = localtime(&t);
    sprintf(mDateTime, "%4d:%02d:%02d %02d:%02d:%02d",
        tm_t->tm_year+1900, tm_t->tm_mon+1, tm_t->tm_mday,
        tm_t->tm_hour, tm_t->tm_min, tm_t->tm_sec);
}

void CallbackNotifier::onCameraDeviceError(int err)
{
    if (isMessageEnabled(CAMERA_MSG_ERROR) && mNotifyCB != NULL) {
        mNotifyCB(CAMERA_MSG_ERROR, err, 0, mCallbackCookie);
    }
}
void CallbackNotifier::setExifInfo(struct isp_exif_attribute exifinfo,int zoom_ratio,int exposure_bias)
{
    mExposureTime.num = exifinfo.exposure_time.numerator;
    mExposureTime.den = exifinfo.exposure_time.denominator;
    mFNumber.num = exifinfo.fnumber;    //eg:FNum=2.2, aperture = 220, --> num = 220,den = 100
    mFNumber.den = 100;
    mISOSpeed = exifinfo.iso_speed;
    mExposureBiasValue.num = exposure_bias;
    mExposureBiasValue.den = 1;
    mMeteringMode = 1;
    mFlashUsed = exifinfo.flash_fire;
    mFocalLength_r.num = exifinfo.focal_length;
    mFocalLength_r.den = 100;
    mDigitalZoomRatio.num = zoom_ratio;
    mDigitalZoomRatio.den = 100;
    mExposureMode = 0;
    LOGD("exif_attri fnumber: %d",mFNumber.num);
    LOGD("exif_attri exposure_time: %d  %d",mExposureTime.num,mExposureTime.den);
    LOGD("exif_attri iso_speed: %d",mISOSpeed);
    LOGD("exif_attri focal_length: %d",mFocalLength_r.num);
    LOGD("exif_attri flash_fire: %d",mFlashUsed);
    //LOGD("exif_attri mDigitalZoomRatio: %d",(int)mDigitalZoomRatio);
    LOGD("exif_attri exposure_bias: %d",mExposureBiasValue.num);
}

}; /* namespace android */
