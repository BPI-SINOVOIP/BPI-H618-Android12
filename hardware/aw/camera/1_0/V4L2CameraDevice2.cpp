
#include "CameraDebug.h"
//#if DBG_V4L2_CAMERA
#define LOG_NDEBUG 0
//#endif
#define LOG_TAG "V4L2CameraDevice"
#include <cutils/log.h>

#include <sys/mman.h>
#include <sys/time.h>

#ifdef USE_MP_CONVERT
#include <g2d_driver.h>
#endif

#include "V4L2CameraDevice2.h"
#include "CallbackNotifier.h"
#include "PreviewWindow.h"
#include "CameraHardware2.h"
#include <memory/memoryAdapter.h>
#include <memory/sc_interface.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cutils/properties.h>
#include "math.h"
#include <utils/CallStack.h>
//#include "memory/memoryAdapter.h"
//#include "memory/sc_interface.h"
#ifdef USE_SUNXI_CAMERA_H
#include <sunxi_camera.h>
#endif
#ifdef USE_CSI_VIN_DRIVER
#include <sunxi_camera_v2.h>
#endif


#include "HALCameraFactory.h"


#define CHECK_NO_ERROR(a)                        \
    if (a != NO_ERROR) {                        \
        if (mCameraFd != NULL) {                \
            LOGE("CHECK_NO_ERROR mCameraFd  %d",mCameraFd);     \
            close(mCameraFd);                    \
            LOGE("error close device");            \
            mCameraFd = NULL;                    \
        }                                        \
        return EINVAL;                            \
    }

#define RAW_DATA_INTERFACE   104
#define RAW_DATA_SYSTEM      105
#define RAW_DATA_FORMAT      106
#define RAW_DATA_PIXELFORMAT 107
#define RAW_DATA_ROW         108
#define RAW_DATA_COLUMN      109
#define RAW_DATA_CH0_INDEX   110
#define RAW_DATA_CH1_INDEX   111
#define RAW_DATA_CH2_INDEX   112
#define RAW_DATA_CH3_INDEX   113
#define RAW_DATA_CH0_STATUS  114
#define RAW_DATA_CH1_STATUS  115
#define RAW_DATA_CH2_STATUS  116
#define RAW_DATA_CH3_STATUS  117

extern void PreviewCnr(unsigned int snr_level, unsigned int gain, int width, int height, char *src, char *dst);
extern void ColorDenoise(unsigned char *dst_plane, unsigned char *src_plane, int width, int height, int threshold);
extern    int Sharpen(unsigned char * image, int min_val, int level, int width, int height);
namespace android {

// defined in HALCameraFactory.cpp
extern void getCallingProcessName(char *name);
static int SceneNotifyCallback(int cmd, void* data, int* ret,void* user)
{

    V4L2CameraDevice* dev = (V4L2CameraDevice*)user;
    switch(cmd)
    {
        case SCENE_NOTIFY_CMD_GET_AE_STATE:
            LOGV("SCENE_NOTIFY_CMD_GET_AE_STATE");
            *ret = dev->getAeStat((struct isp_stat_buf *)data);
            break;

        case SCENE_NOTIFY_CMD_GET_HIST_STATE:
            LOGV("SCENE_NOTIFY_CMD_GET_HIST_STATE");
            *ret = dev->getHistStat((struct isp_stat_buf * )data);
            break;

        case SCENE_NOTIFY_CMD_SET_3A_LOCK:
            LOGV("SCENE_NOTIFY_CMD_SET_3A_LOCK: %ld",(long)data);
            *ret = dev->set3ALock((long)data);
            break;

        case SCENE_NOTIFY_CMD_SET_HDR_SETTING:
            LOGV("SENE_NOTIFY_CMD_SET_HDR_SETTING");
            *ret = dev->setHDRMode(data);
            break;

        case SCENE_NOTIFY_CMD_GET_HDR_FRAME_COUNT:
            //*ret = dev->getHDRFrameCnt((int*)data);
            int cnt;
            cnt = dev->getHDRFrameCnt();
            *(int *)data = cnt;
            *ret = 0;
            LOGD("SCENE_NOTIFY_CMD_GET_HDR_FRAME_COUNT: %ld",cnt);
            break;

        default:
            break;

    }

    return 0;
}

static void calculateCrop(Rect * rect, int new_zoom, int max_zoom, int width, int height)
{
    if (max_zoom == 0)
    {
        rect->left        = 0;
        rect->top        = 0;
        rect->right     = width -1;
        rect->bottom    = height -1;
    }
    else
    {
        int new_ratio = (new_zoom * 2 * 100 / max_zoom + 100);
        rect->left        = (width - (width * 100) / new_ratio)/2;
        rect->top        = (height - (height * 100) / new_ratio)/2;
        rect->right     = rect->left + (width * 100) / new_ratio -1;
        rect->bottom    = rect->top  + (height * 100) / new_ratio - 1;
    }

    // LOGD("crop: [%d, %d, %d, %d]", rect->left, rect->top, rect->right, rect->bottom);
}

static void YUYVToNV12(const void* yuyv, void *nv12, int width, int height)
{
    uint8_t* Y    = (uint8_t*)nv12;
    uint8_t* UV = (uint8_t*)Y + width * height;

    for(int i = 0; i < height; i += 2)
    {
        for (int j = 0; j < width; j++)
        {
            *(uint8_t*)((uint8_t*)Y + i * width + j) = *(uint8_t*)((uint8_t*)yuyv + i * width * 2 + j * 2);
            *(uint8_t*)((uint8_t*)Y + (i + 1) * width + j) = *(uint8_t*)((uint8_t*)yuyv + (i + 1) * width * 2 + j * 2);
            *(uint8_t*)((uint8_t*)UV + ((i * width) >> 1) + j) = *(uint8_t*)((uint8_t*)yuyv + i * width * 2 + j * 2 + 1);
        }
    }
}

static void YUYVToNV21(const void* yuyv, void *nv21, int width, int height)
{
    uint8_t* Y    = (uint8_t*)nv21;
    uint8_t* VU = (uint8_t*)Y + width * height;

    for(int i = 0; i < height; i += 2)
    {
        for (int j = 0; j < width; j++)
        {
            *(uint8_t*)((uint8_t*)Y + i * width + j) = *(uint8_t*)((uint8_t*)yuyv + i * width * 2 + j * 2);
            *(uint8_t*)((uint8_t*)Y + (i + 1) * width + j) = *(uint8_t*)((uint8_t*)yuyv + (i + 1) * width * 2 + j * 2);

            if (j % 2)
            {
                if (j < width - 1)
                {
                    *(uint8_t*)((uint8_t*)VU + ((i * width) >> 1) + j) = *(uint8_t*)((uint8_t*)yuyv + i * width * 2 + (j + 1) * 2 + 1);
                }
            }
            else
            {
                if (j > 1)
                {
                    *(uint8_t*)((uint8_t*)VU + ((i * width) >> 1) + j) = *(uint8_t*)((uint8_t*)yuyv + i * width * 2 + (j - 1) * 2 + 1);
                }
            }
        }
    }
}

#ifdef USE_MP_CONVERT
void V4L2CameraDevice::YUYVToYUV420C(const void* yuyv, void *yuv420, int width, int height)
{
    g2d_blt        blit_para;
    int         err;

    blit_para.src_image.addr[0]      = (unsigned long)yuyv;
    blit_para.src_image.addr[1]      = (unsigned long)yuyv + width * height;
    blit_para.src_image.w            = width;          /* src buffer width in pixel units */
    blit_para.src_image.h            = height;          /* src buffer height in pixel units */
    blit_para.src_image.format       = G2D_FMT_IYUV422;
    blit_para.src_image.pixel_seq    = G2D_SEQ_NORMAL;          /* not use now */
    blit_para.src_rect.x             = 0;                        /* src rect->x in pixel */
    blit_para.src_rect.y             = 0;                        /* src rect->y in pixel */
    blit_para.src_rect.w             = width;            /* src rect->w in pixel */
    blit_para.src_rect.h             = height;            /* src rect->h in pixel */

    blit_para.dst_image.addr[0]      = (unsigned long)yuv420;
    blit_para.dst_image.addr[1]      = (unsigned long)yuv420 + width * height;
    blit_para.dst_image.w            = width;          /* dst buffer width in pixel units */
    blit_para.dst_image.h            = height;          /* dst buffer height in pixel units */
    blit_para.dst_image.format       = G2D_FMT_PYUV420UVC;
    blit_para.dst_image.pixel_seq    = (mVideoFormat == V4L2_PIX_FMT_NV12) ? G2D_SEQ_NORMAL : G2D_SEQ_VUVU;          /* not use now */
    blit_para.dst_x                  = 0;                    /* dst rect->x in pixel */
    blit_para.dst_y                  = 0;                    /* dst rect->y in pixel */
    blit_para.color                  = 0xff;                  /* fix me*/
    blit_para.alpha                  = 0xff;                /* globe alpha */

    blit_para.flag = G2D_BLT_NONE; // G2D_BLT_FLIP_HORIZONTAL;

    err = ioctl(mG2DHandle, G2D_CMD_BITBLT, (unsigned long)&blit_para);
    if(err < 0)
    {
        LOGE("ioctl, G2D_CMD_BITBLT failed");
        return;
    }
}

void V4L2CameraDevice::NV21ToYV12(const void* nv21, void *yv12, int width, int height)
{
    g2d_blt        blit_para;
    int         err;
    int            u, v;
    if (mVideoFormat == V4L2_PIX_FMT_NV21)
    {
        u = 1;
        v = 2;
    }
    else
    {
        u = 2;
        v = 1;
    }

    blit_para.src_image.addr[0]      = (unsigned long)nv21;
    blit_para.src_image.addr[1]      = (unsigned long)nv21 + width * height;
    blit_para.src_image.w            = width;          /* src buffer width in pixel units */
    blit_para.src_image.h            = height;          /* src buffer height in pixel units */
    blit_para.src_image.format       = G2D_FMT_PYUV420UVC;
    blit_para.src_image.pixel_seq    = G2D_SEQ_NORMAL;//G2D_SEQ_VUVU;          /*  */
    blit_para.src_rect.x             = 0;                        /* src rect->x in pixel */
    blit_para.src_rect.y             = 0;                        /* src rect->y in pixel */
    blit_para.src_rect.w             = width;            /* src rect->w in pixel */
    blit_para.src_rect.h             = height;            /* src rect->h in pixel */

    blit_para.dst_image.addr[0]      = (unsigned long)yv12;                            // y
    blit_para.dst_image.addr[u]      = (unsigned long)yv12 + width * height;            // v
    blit_para.dst_image.addr[v]      = (unsigned long)yv12 + width * height * 5 / 4;    // u
    blit_para.dst_image.w            = width;          /* dst buffer width in pixel units */
    blit_para.dst_image.h            = height;          /* dst buffer height in pixel units */
    blit_para.dst_image.format       = G2D_FMT_PYUV420;
    blit_para.dst_image.pixel_seq    = G2D_SEQ_NORMAL;          /* not use now */
    blit_para.dst_x                  = 0;                    /* dst rect->x in pixel */
    blit_para.dst_y                  = 0;                    /* dst rect->y in pixel */
    blit_para.color                  = 0xff;                  /* fix me*/
    blit_para.alpha                  = 0xff;                /* globe alpha */

    blit_para.flag = G2D_BLT_NONE;

    err = ioctl(mG2DHandle , G2D_CMD_BITBLT, (unsigned long)&blit_para);
    if(err < 0)
    {
        LOGE("NV21ToYV12 ioctl, G2D_CMD_BITBLT failed");
        return;
    }
}
#endif

DBG_TIME_AVG_BEGIN(TAG_CONTINUOUS_PICTURE);

void  V4L2CameraDevice::showformat(int format,char *str)
{
    switch(format){
    case V4L2_PIX_FMT_YUYV:
        LOGV("The %s foramt is V4L2_PIX_FMT_YUYV",str);
        break;
    case V4L2_PIX_FMT_MJPEG:
        LOGV("The %s foramt is V4L2_PIX_FMT_MJPEG",str);
        break;
    case V4L2_PIX_FMT_YVU420:
        LOGV("The %s foramt is V4L2_PIX_FMT_YVU420",str);
        break;
    case V4L2_PIX_FMT_NV12:
        LOGV("The %s foramt is V4L2_PIX_FMT_NV12",str);
        break;
    case V4L2_PIX_FMT_NV21:
        LOGV("The %s foramt is V4L2_PIX_FMT_NV21",str);
        break;
    case V4L2_PIX_FMT_H264:
        LOGV("The %s foramt is V4L2_PIX_FMT_H264",str);
        break;
    default:
        LOGV("The %s format can't be showed",str);
    }
}
V4L2CameraDevice::V4L2CameraDevice(CameraHardware* camera_hal,
                                   PreviewWindow * preview_window,
                                   CallbackNotifier * cb)
    : mCameraHardware(camera_hal),
      mPreviewWindow(preview_window),
      mCallbackNotifier(cb),
      mCameraDeviceState(STATE_CONSTRUCTED),
      mCaptureThreadState(CAPTURE_STATE_NULL),
      mPreviewThreadState(PREVIEW_STATE_NULL),
      mCameraFd(0),
      mCameraType(CAMERA_TYPE_CSI),
      mTakePictureState(TAKE_PICTURE_NULL),
      mIsPicCopy(false),
      mFrameWidth(0),
      mFrameHeight(0),
      mThumbWidth(0),
      mThumbHeight(0),
      mCurFrameTimestamp(0),
      mBufferCnt(NB_BUFFER),
      mUseHwEncoder(false),
      mNewZoom(0),
      mLastZoom(-1),
      mMaxZoom(0xffffffff),
      mCaptureFormat(V4L2_PIX_FMT_NV21),
      mFrameRate(30),
	  mVideoFormat(V4L2_PIX_FMT_NV21),
#ifdef USE_MP_CONVERT
      ,mG2DHandle(0)
#endif
      mCanBeDisconnected(false)
	  ,mStartSmartTimeout(false)
      ,mContinuousPictureStarted(false)
	  ,mCurrentV4l2buf(NULL)
      ,mContinuousPictureCnt(0)
      ,mContinuousPictureMax(0)
      ,mContinuousPictureStartTime(0)
      ,mSmartPictureDone(true)
	  ,mContinuousPictureLast(0)
	  ,mContinuousPictureAfter(0)
      ,mFaceDectectLast(0)
      ,mFaceDectectAfter(0)
      ,mPreviewLast(0)
      ,mPreviewAfter(0)
      ,mVideoHint(false)
      ,mCurAvailBufferCnt(0)
      ,mStatisicsIndex(0)
      ,mNeedHalfFrameRate(false)
      ,mShouldPreview(true)
      ,mIsThumbUsedForVideo(false)
      ,mVideoWidth(640)
      ,mZoomRatio(100)
	  ,mVideoHeight(480)
	  ,mFlashMode(V4L2_FLASH_LED_MODE_NONE)
      ,mMemOpsS(NULL)
	  ,mSceneMode(NULL)
	  ,mExposureBias(0)
      ,mDiscardFrameNum(0)
	  ,releaseIndex(-1)
      ,nplanes(0)
      ,mDecoder(NULL)
#ifdef USE_DEINTERLACE_HW
      ,mDiProcess(NULL)
      ,mPreFrameIndex(-1)
      ,mPrePreFrameIndex(-1)
      ,mEnableYUV422(-1)
#endif
#ifdef USE_ISP
      ,mAWIspApi(NULL)
      ,mIspId(-1)
#endif
      ,mPicCnt(0)
      ,mOrgTime(0)
      ,mDebugLogCnt(0)
{
    LOGV("V4L2CameraDevice construct");

    memset(&mMapMem,0,sizeof(mMapMem));
    memset(&mVideoBuffer,0,sizeof(mVideoBuffer));

    memset(&mHalCameraInfo, 0, sizeof(mHalCameraInfo));
    memset(&mRectCrop, 0, sizeof(Rect));
    memset(&mTakePictureFlag,0,sizeof(mTakePictureFlag));
    memset(&mPicBuffer,0,sizeof(mPicBuffer));

#ifdef USE_DEINTERLACE_HW
    memset(&mDiOutPutBuffer, 0, sizeof(bufferManagerQD_t));
    for(int i = 0; i < NB_BUFFER; i++)
    {
        memset(&mV4l2DiBuf[i], 0, sizeof(V4L2BUF_t));
    }
#endif

    mMemOpsS = MemCamAdapterGetOpsS();
    mMemOpsS->open_cam();
    // init preview buffer queue
    OSAL_QueueCreate(&mQueueBufferPreview, NB_BUFFER);
    OSAL_QueueCreate(&mQueueBufferPicture, 2);


    // init capture thread
    mCaptureThread = new DoCaptureThread(this);
    pthread_mutex_init(&mCaptureMutex, NULL);
    pthread_cond_init(&mCaptureCond, NULL);
    mCaptureThreadState = CAPTURE_STATE_PAUSED;
    mCaptureThread->startThread();

    // init preview thread
    mPreviewThread = new DoPreviewThread(this);
    pthread_mutex_init(&mPreviewMutex, NULL);
    pthread_cond_init(&mPreviewCond, NULL);
    mPreviewThread->startThread();

    pthread_mutex_init(&mPreviewSyncMutex, NULL);
    pthread_cond_init(&mPreviewSyncCond, NULL);
    mPreviewThreadState = PREVIEW_STATE_STARTED;
    // init picture thread
    mPictureThread = new DoPictureThread(this);
    pthread_mutex_init(&mPictureMutex, NULL);
    pthread_cond_init(&mPictureCond, NULL);
    mPictureThread->startThread();

    pthread_mutex_init(&mConnectMutex, NULL);
    pthread_cond_init(&mConnectCond, NULL);
    // init continuous picture thread
    mContinuousPictureThread = new DoContinuousPictureThread(this);
    pthread_mutex_init(&mContinuousPictureMutex, NULL);
    pthread_cond_init(&mContinuousPictureCond, NULL);
    mContinuousPictureThread->startThread();
    mSmartPictureThread = new DoSmartPictureThread(this);
    pthread_mutex_init(&mSmartPictureMutex, NULL);
    pthread_cond_init(&mSmartPictureCond, NULL);
    mSmartPictureThread->startThread();
}

V4L2CameraDevice::~V4L2CameraDevice()
{
    LOGV("V4L2CameraDevice disconstruct");

    if (mCaptureThread != NULL)
    {
        mCaptureThread->stopThread();
        pthread_cond_signal(&mCaptureCond);
        mCaptureThread.clear();
        mCaptureThread = 0;
    }

    if (mPreviewThread != NULL)
    {
        mPreviewThread->stopThread();
        pthread_cond_signal(&mPreviewCond);
        mPreviewThread.clear();
        mPreviewThread = 0;
    }

    if (mPictureThread != NULL)
    {
        mPictureThread->stopThread();
        pthread_cond_signal(&mPictureCond);
        mPictureThread.clear();
        mPictureThread = 0;
    }

    if (mContinuousPictureThread != NULL)
    {
        mContinuousPictureThread->stopThread();
        pthread_cond_signal(&mContinuousPictureCond);
        mContinuousPictureThread.clear();
        mContinuousPictureThread = 0;
    }

    if (mSmartPictureThread != NULL)
    {
        mSmartPictureThread->stopThread();
        pthread_cond_signal(&mSmartPictureCond);
        mSmartPictureThread.clear();
        mSmartPictureThread = 0;
    }
    if (mMemOpsS != NULL)
    {
        mMemOpsS->close_cam();
    }

    pthread_mutex_destroy(&mCaptureMutex);
    pthread_cond_destroy(&mCaptureCond);

    pthread_mutex_destroy(&mPreviewMutex);
    pthread_cond_destroy(&mPreviewCond);

    pthread_mutex_destroy(&mPictureMutex);
    pthread_cond_destroy(&mPictureCond);

    pthread_mutex_destroy(&mConnectMutex);
    pthread_cond_destroy(&mConnectCond);

    pthread_mutex_destroy(&mContinuousPictureMutex);
    pthread_cond_destroy(&mContinuousPictureCond);
    pthread_mutex_destroy(&mSmartPictureMutex);
    pthread_cond_destroy(&mSmartPictureCond);

    OSAL_QueueTerminate(&mQueueBufferPreview);
    OSAL_QueueTerminate(&mQueueBufferPicture);
}

/****************************************************************************
 * V4L2CameraDevice interface implementation.
 ***************************************************************************/

status_t V4L2CameraDevice::connectDevice(HALCameraInfo * halInfo)
{
    F_LOG;

    Mutex::Autolock locker(&mObjectLock);

    if (isConnected())
    {
        LOGW("%s: camera device is already connected.", __FUNCTION__);
        return NO_ERROR;
    }

    // open v4l2 camera device
    int ret = openCameraDev(halInfo);
    if (ret != OK) {
            return ret;
    }
    //CallStack stack(LOG_TAG);
    if(mCameraType == CAMERA_TYPE_CSI || mCameraType == CAMERA_TYPE_VFE)
    {
        switch((v4l2_sensor_type)getSensorType()){
            case V4L2_SENSOR_TYPE_YUV:
                LOGD("the sensor is YUV sensor");
                mSensor_Type = V4L2_SENSOR_TYPE_YUV;
                break;
            case V4L2_SENSOR_TYPE_RAW:
                LOGD("the sensor is RAW sensor");
                mSensor_Type = V4L2_SENSOR_TYPE_RAW;
#ifdef USE_ISP
                mAWIspApi = new AWIspApi();
#endif
                break;
            default:
                LOGE("get the sensor type failed");
                mSensor_Type = V4L2_SENSOR_TYPE_YUV;
               // goto END_ERROR; modify by hxl
        }
        // halInfo->fast_picture_mode = (int)mSensor_Type;    //for data call back
        memcpy((void*)&mHalCameraInfo, (void*)halInfo, sizeof(HALCameraInfo));
        struct isp_exif_attribute exif_attri;
        getExifInfo(&exif_attri);
        mCameraHardware->setExifInfo(exif_attri);
    }

#ifdef USE_MP_CONVERT
    if (mCameraType == CAMERA_TYPE_UVC)
    {
        // open MP driver
        mG2DHandle = open("/dev/g2d", O_RDWR, 0);
        if (mG2DHandle < 0)
        {
            LOGE("open /dev/g2d failed");
            goto END_ERROR;
        }
        LOGV("open /dev/g2d OK");
    }
#endif

    ret = mMemOpsS->open_cam();
    if (ret < 0)
    {
        LOGE("ion_alloc_open failed");
        goto END_ERROR;
    }
    LOGV("ion_alloc_open ok");

#ifdef USE_DEINTERLACE_HW
	if (mCameraType == CAMERA_TYPE_TVIN_NTSC || mCameraType == CAMERA_TYPE_TVIN_PAL
            || mCameraType == CAMERA_TYPE_TVIN_YPBPR) {
        mDiProcess = new DiProcess();
        if (mDiProcess == NULL) {
        LOGE("new Diprocess failed");
        goto END_ERROR;
        }
        LOGD("new Diprocess OK");
        ret = mDiProcess->init();
        if (ret != NO_ERROR) {
            LOGE("Diprocess init failed");
            goto END_ERROR;
        }
    }
    for(int i = 0; i < NB_BUFFER; i++)
    {
        memset(&mV4l2DiBuf[i], 0, sizeof(V4L2BUF_t));
    }
#endif

    /* There is a device to connect to. */
    mCameraDeviceState = STATE_CONNECTED;

    return NO_ERROR;
END_ERROR:
    if (mCameraFd != NULL)
    {
        LOGE("END_ERROR mCameraFd  %d",mCameraFd); 
        close(mCameraFd);
        mCameraFd = NULL;
    }
#ifdef USE_MP_CONVERT
    if(mG2DHandle != NULL)
    {
        close(mG2DHandle);
        mG2DHandle = NULL;
    }
#endif
#ifdef USE_DEINTERLACE_HW
    if (mDiProcess != NULL) {
        delete mDiProcess;
        mDiProcess = NULL;
    }
#endif
#ifdef USE_ISP
    if (mAWIspApi != NULL) {
        delete mAWIspApi;
        mAWIspApi = NULL;
    }
#endif

    return UNKNOWN_ERROR;
}

status_t V4L2CameraDevice::disconnectDevice()
{
    F_LOG;

    Mutex::Autolock locker(&mObjectLock);

    if (!isConnected())
    {
        LOGW("%s: camera device is already disconnected.", __FUNCTION__);
        return NO_ERROR;
    }

    if (isStarted())
    {
        LOGE("%s: Cannot disconnect from the started device.", __FUNCTION__);
        return -EINVAL;
    }

    // close v4l2 camera device
    closeCameraDev();

#ifdef USE_MP_CONVERT
    if(mG2DHandle != NULL)
    {
        close(mG2DHandle);
        mG2DHandle = NULL;
    }
#endif
#ifdef USE_DEINTERLACE_HW
    if (mDiProcess != NULL) {
        delete mDiProcess;
        mDiProcess = NULL;
    }
#endif
    mMemOpsS->close_cam();

    /* There is no device to disconnect from. */
    mCameraDeviceState = STATE_CONSTRUCTED;

#ifdef USE_ISP
    if (mAWIspApi != NULL) {
        delete mAWIspApi;
        mAWIspApi = NULL;
    }
#endif

    return NO_ERROR;
}

status_t V4L2CameraDevice::startDevice(int width,
                                       int height,
                                       uint32_t pix_fmt,
                                       bool video_hint)
{
    LOGD("%s, wxh: %dx%d, fmt: %d", __FUNCTION__, width, height, pix_fmt);
	char prop_value[PROPERTY_VALUE_MAX];
	int ret = 0;
    Mutex::Autolock locker(&mObjectLock);

    if (!isConnected())
    {
        LOGE("%s: camera device is not connected.", __FUNCTION__);
        return EINVAL;
    }

    if (isStarted())
    {
        LOGE("%s: camera device is already started.", __FUNCTION__);
        return EINVAL;
    }
	
	char CallingProcessName[128];
	getCallingProcessName(CallingProcessName);
	LOGE("%s ++++++++++++%s===========", __FUNCTION__,CallingProcessName);
	if ((strcmp(CallingProcessName, "com.tencent.mobileqq") == 0)) {
		property_set("sys.qqmobile.online","true");
		LOGE("%s hxl =======================", __FUNCTION__);
		ret = property_get("sys.qqmobile.online", prop_value, ""); 
		ALOGD("hxl === startDevice prop_value =%s,ret =%d",prop_value,ret);
	}
    if (mPreviewWindow->mPreviewWindow == NULL) {
        LOGE("mPreviewWindow->mPreviewWindow == NULL, but still started, F:%s,L:%d", __FUNCTION__,__LINE__);
        mPreviewIsNull = true;
        return NO_ERROR;
    }
    mPreviewIsNull = false;

    // VE encoder need this format
    mVideoFormat = pix_fmt;
#ifdef USE_DEINTERLACE_HW
    if ((mCameraType == CAMERA_TYPE_TVIN_NTSC || mCameraType == CAMERA_TYPE_TVIN_PAL
        || mCameraType == CAMERA_TYPE_TVIN_YPBPR)){
        mCaptureFormat = pix_fmt;
    }
#endif

    mCurrentV4l2buf = NULL;

    mVideoHint = video_hint;
    mCanBeDisconnected = false;

#ifdef USE_DEINTERLACE_HW
    mPreFrameIndex = -1;
    mPrePreFrameIndex = -1;
#endif

//TOOLS FOR CAPTURE BITSTREAM
//We open the file in there ,if we want to capture the picture(one frame),
//we must change the resolution in APP UI,because
//the V4L2CameraDevice::startDevice only work in  mCaptureWidth != frame_width&& mCaptureHeight != frame_height..
//See status_t CameraHardware::doTakePicture() you can get more details..
#if DBG_CAPTURE_STREAM_FROM_V4L2DEVICE_FORNV21||DBG_CAPTURE_STREAM_FROM_V4L2DEVICE_FORYUYV||DBG_CAPTURE_STREAM_FROM_V4L2DEVICE_FORH264
    char fname0[128] ;
    char fname0_tail[128];
    frame_num0 = 0;
    v4l2device_tail_num++;

    sprintf(fname0,DBG_CAPTURE_V4L2DEVICE_PATH);
    sprintf(fname0_tail,"%d",v4l2device_tail_num);

    strcat(fname0,fname0_tail);

    fp_stream_from_v4l2device = fopen(fname0,"w");
    if(fp_stream_from_v4l2device == NULL) {
        LOGE("%s: zjw,Open DBG_CAPTURE_V4L2DEVICE_PATH fail!", __FUNCTION__);
    }else {
        LOGD("%s: zjw,Open DBG_CAPTURE_V4L2DEVICE_PATH success!v4l2device_tail_num :%d", __FUNCTION__,v4l2device_tail_num);
    }
#endif

#if DBG_CAPTURE_STREAM_AFTER_TRANSFORMATION
    char fname1[128] ;
    char fname1_tail[128];
    frame_num1 = 0;
    afterTransformation_tail_num++;

    sprintf(fname1,DBG_CAPTURE_AFTER_TRANSFORMATION_PATH);
    sprintf(fname1_tail,"%d",afterTransformation_tail_num);

    strcat(fname1,fname1_tail);

    fp_stream_after_transformation = fopen(fname1,"w");
    if(fp_stream_after_transformation == NULL) {
        LOGE("%s: zjw,Open DBG_CAPTURE_AFTER_TRANSFORMATION_PATH fail!", __FUNCTION__);
    }else {
        LOGD("%s: zjw,Open DBG_CAPTURE_AFTER_TRANSFORMATION_PATH success!afterTransformation_tail_num:%d", __FUNCTION__,afterTransformation_tail_num);
    }
#endif

    // set capture mode and fps
    // CHECK_NO_ERROR(v4l2setCaptureParams());    // do not check this error
    v4l2setCaptureParams();

    // set v4l2 device parameters, it maybe change the value of mFrameWidth and mFrameHeight.
    CHECK_NO_ERROR(v4l2SetVideoParams(width, height, pix_fmt));

    // v4l2 request buffers
    // We avoid 3 frame usb camera data for taking picture, so we allocate more buffer.
    int buf_cnt = (mTakePictureState == TAKE_PICTURE_NORMAL) ? 3 : NB_BUFFER;
    CHECK_NO_ERROR(v4l2ReqBufs(&buf_cnt));
    mBufferCnt = buf_cnt;
    mCurAvailBufferCnt = mBufferCnt;

    LOGD("window allocate!");
    mPreviewWindow->allocate(mBufferCnt,mFrameWidth,mFrameHeight,mVideoFormat);
    mPreviewWindow->enqueue_buffer_num = 0;

    // v4l2 query buffers
    CHECK_NO_ERROR(v4l2QueryBuf());

    memset(&mVideoConf, 0, sizeof(mVideoConf));

    if((mCaptureFormat == V4L2_PIX_FMT_MJPEG)||(mCaptureFormat == V4L2_PIX_FMT_H264))
    {
       // LOGD("FUNC:%s, Line:%d init Dec!,mCamMemOpsS :%x ",__FUNCTION__,__LINE__,mCamMemOpsS);
        mVideoConf.memops = NULL;
        //* all decoder support YV12 format. (PIXEL_FORMAT_YV12;PIXEL_FORMAT_YUV_MB32_420)
        mVideoConf.eOutputPixelFormat  = PIXEL_FORMAT_NV21;
        //* never decode two picture when decoding a thumbnail picture.
        mVideoConf.bDisable3D          = 1;
        mVideoConf.bScaleDownEn    = 0;
        mVideoConf.bRotationEn          = 0;
        mVideoConf.bSecOutputEn    = 0;

        //added by zhengjiangwei ,fix decoder H264 bitstream that is wrong,and to display normally.
        mVideoConf.bDispErrorFrame = 1;

        mVideoConf.nVbvBufferSize      = 0;
        mVideoConf.nAlignStride = 32;



        mVideoInfo.eCodecFormat = (mCaptureFormat == V4L2_PIX_FMT_MJPEG) ?
                                  VIDEO_CODEC_FORMAT_MJPEG :
                                  VIDEO_CODEC_FORMAT_H264;

        mVideoInfo.nWidth = width;
        mVideoInfo.nHeight = height;


        LOGV("FUNC:%s, Line:%d width = %d,height = %d,",__FUNCTION__,__LINE__,width,height);

        mVideoInfo.nFrameRate = mFrameRate;
        mVideoInfo.nFrameDuration = 1000*1000/mFrameRate;
        mVideoInfo.nAspectRatio = 1000;
        mVideoInfo.bIs3DStream = 0;
        mVideoInfo.nCodecSpecificDataLen = 0;
        mVideoInfo.pCodecSpecificData = NULL;
        mDecoder = NULL;

        Libve_init2(&mDecoder,&mVideoInfo,&mVideoConf);
        if(mDecoder == NULL){
            LOGE("FUNC:%s, Line:%d ",__FUNCTION__,__LINE__);
        }
    }

    mCameraDeviceState = STATE_STARTED;

    mContinuousPictureAfter = 1000000 / 10;
    mFaceDectectAfter = 1000000 / 15;
    mPreviewAfter = 1000000 / 24;
    set3ALock(0);

    // stream on the v4l2 device
    CHECK_NO_ERROR(v4l2StartStreaming());
#ifdef USE_ISP
    if (mSensor_Type == V4L2_SENSOR_TYPE_RAW) {
        mIspId = -1;
		LOGE("hxl === mHalCameraInfo.device_id =================== %d",mHalCameraInfo.device_id);
        mIspId = mAWIspApi->awIspGetIspId(0);
        if (mIspId >= 0) {
            mAWIspApi->awIspStart(mIspId);
        }
    }
#endif

    mDiscardFrameNum = 0;
    return NO_ERROR;
}

status_t V4L2CameraDevice::stopDevice()
{
    LOGD("V4L2CameraDevice::stopDevice");
	char CallingProcessName[128];
	getCallingProcessName(CallingProcessName);
	if ((strcmp(CallingProcessName, "com.tencent.mobileqq") == 0)) {
		property_set("sys.qqmobile.online","false");
	}
    pthread_mutex_lock(&mConnectMutex);
    if (!mCanBeDisconnected)
    {
        LOGW("wait until capture thread pause or exit");
        pthread_cond_wait(&mConnectCond, &mConnectMutex);
    }
    pthread_mutex_unlock(&mConnectMutex);

    Mutex::Autolock locker(&mObjectLock);

    if (!isStarted())
    {
        LOGW("%s: camera device is not started.", __FUNCTION__);
        return NO_ERROR;
    }
//TOOLS FOR CAPTURE BITSTREAM
//When the device close ,we also close the file...
#if DBG_CAPTURE_STREAM_FROM_V4L2DEVICE_FORNV21||DBG_CAPTURE_STREAM_FROM_V4L2DEVICE_FORYUYV||DBG_CAPTURE_STREAM_FROM_V4L2DEVICE_FORH264
    fclose(fp_stream_from_v4l2device);
#endif
#if DBG_CAPTURE_STREAM_AFTER_TRANSFORMATION
    fclose(fp_stream_after_transformation);
#endif

    // v4l2 device stop stream
    v4l2StopStreaming();
#ifdef USE_ISP
    if ((mSensor_Type == V4L2_SENSOR_TYPE_RAW) && (mIspId >= 0)) {
        mAWIspApi->awIspStop(mIspId);
    }
#endif

    // v4l2 device unmap buffers
    v4l2UnmapBuf();

    for(int i = 0; i < NB_BUFFER; i++)
    {
        memset(&mV4l2buf[i], 0, sizeof(V4L2BUF_t));
    }

    mCameraDeviceState = STATE_CONNECTED;

    mLastZoom = -1;

    mCurrentV4l2buf = NULL;

    if(mCaptureFormat == V4L2_PIX_FMT_MJPEG || mCaptureFormat == V4L2_PIX_FMT_H264)
    {
        Libve_exit2(&mDecoder);
    }

#ifdef USE_DEINTERLACE_HW
    mPreFrameIndex = -1;
    mPrePreFrameIndex = -1;
    mEnableYUV422 = -1;
#endif

    LOGV("%s: mPreviewWindow->deallocate.", __FUNCTION__);
    if(mCameraType == CAMERA_TYPE_CSI
        || mCameraType == CAMERA_TYPE_UVC
        || mCameraType == CAMERA_TYPE_VFE)
    {
        mPreviewWindow->deallocate(mBufferCnt);
    }

    return NO_ERROR;
}

status_t V4L2CameraDevice::startDeliveringFrames()
{
    F_LOG;



	//when mPreviewWindow null,can not singal to start capture thread
	if (mPreviewWindow->mPreviewWindow == NULL) 
	{
		return NO_ERROR;
	}


    pthread_mutex_lock(&mCaptureMutex);

    if (mCaptureThreadState == CAPTURE_STATE_NULL)
    {
        LOGE("error state of capture thread, %s", __FUNCTION__);
        pthread_mutex_unlock(&mCaptureMutex);
        return EINVAL;
    }

    if (mCaptureThreadState == CAPTURE_STATE_STARTED)
    {
        LOGW("capture thread has already started");
        pthread_mutex_unlock(&mCaptureMutex);
        return NO_ERROR;
    }

    // singal to start capture thread
    mCaptureThreadState = CAPTURE_STATE_STARTED;
    pthread_cond_signal(&mCaptureCond);
    pthread_mutex_unlock(&mCaptureMutex);
    pthread_mutex_lock(&mPreviewSyncMutex);
    mPreviewThreadState = PREVIEW_STATE_STARTED;
    LOGV("set mPreviewThreadState to %d\n",mPreviewThreadState);
    pthread_cond_signal(&mPreviewSyncCond);
    pthread_mutex_unlock(&mPreviewSyncMutex);

    return NO_ERROR;
}

status_t V4L2CameraDevice::stopDeliveringFrames()
{
    F_LOG;

    pthread_mutex_lock(&mCaptureMutex);
    if (mCaptureThreadState == CAPTURE_STATE_NULL)
    {
        LOGE("error state of capture thread, %s", __FUNCTION__);
        pthread_mutex_unlock(&mCaptureMutex);
        return EINVAL;
    }

    if (mCaptureThreadState == CAPTURE_STATE_PAUSED)
    {
        LOGW("capture thread has already paused");
        pthread_mutex_unlock(&mCaptureMutex);
        return NO_ERROR;
    }

    mCaptureThreadState = CAPTURE_STATE_PAUSED;
    pthread_mutex_unlock(&mCaptureMutex);

    return NO_ERROR;
}


/****************************************************************************
 * Worker thread management.
 ***************************************************************************/

int V4L2CameraDevice::v4l2WaitCameraReady()
{
    fd_set fds;
    struct timeval tv;
    int r;

    FD_ZERO(&fds);
    FD_SET(mCameraFd, &fds);

    /* Timeout */
    tv.tv_sec  = 2;
    tv.tv_usec = 0;

    r = select(mCameraFd + 1, &fds, NULL, NULL, &tv);
    if (r == -1)
    {
        LOGE("select err, %s", strerror(errno));
        return -1;
    }
    else if (r == 0)
    {
        LOGE("select timeout");
        return -1;
    }

    return 0;
}

void V4L2CameraDevice::singalDisconnect()
{
    pthread_mutex_lock(&mConnectMutex);
    mCanBeDisconnected = true;
    pthread_cond_signal(&mConnectCond);
    pthread_mutex_unlock(&mConnectMutex);
}

int V4L2CameraDevice::testFrameRate()
{
    mPicCnt++;

    if (mPicCnt >= 1500) {
        timeval cur_time;
        gettimeofday(&cur_time, NULL);
        const uint64_t now_time = cur_time.tv_sec * 1000000LL + cur_time.tv_usec;
        int framerate = mPicCnt * 1000 / ((now_time - mOrgTime) / 1000);
        if (framerate) {
            if (mCameraType == CAMERA_TYPE_UVC) {
                LOGD("Mic UVC framerate = %dfps@%dx%d, mCameraFd=%d, BufCnt:%d",
                    framerate, mFrameWidth, mFrameHeight, mCameraFd, mCurAvailBufferCnt);
            } else if (mCameraType == CAMERA_TYPE_CSI) {
                LOGD("Mic CSI framerate = %dfps@%dx%d, mCameraFd=%d, BufCnt:%d",
                    framerate, mFrameWidth, mFrameHeight, mCameraFd, mCurAvailBufferCnt);
            } else {
                LOGD("Mic TVIN framerate = %dfps@%dx%d, mCameraFd=%d, BufCnt:%d",
                    framerate, mFrameWidth, mFrameHeight, mCameraFd, mCurAvailBufferCnt);
            }
        }
        mOrgTime = now_time;
        mPicCnt = 0;
    }
    return 0;
}

bool V4L2CameraDevice::captureThread()
{
    pthread_mutex_lock(&mCaptureMutex);
    // stop capture
    if (mCaptureThreadState == CAPTURE_STATE_PAUSED)
    {
        singalDisconnect();
        // wait for signal of starting to capture a frame
        LOGV("capture thread paused");
        pthread_cond_wait(&mCaptureCond, &mCaptureMutex);
    }

    // thread exit
    if (mCaptureThreadState == CAPTURE_STATE_EXIT)
    {
        singalDisconnect();
        LOGV("capture thread exit");
        pthread_mutex_unlock(&mCaptureMutex);
        return false;
    }
    pthread_mutex_unlock(&mCaptureMutex);
    LOGV("in capture thread now!");
    unsigned long  time0 = systemTime() / 1000000; 
    LOGV("zjw,v4l2WaitCameraReady. before systemTime %ld ms",time0);
    int ret = v4l2WaitCameraReady();

    pthread_mutex_lock(&mCaptureMutex);
    // stop capture or thread exit
    if (mCaptureThreadState == CAPTURE_STATE_PAUSED
        || mCaptureThreadState == CAPTURE_STATE_EXIT)
    {
        singalDisconnect();
        LOGW("should stop capture now");
        pthread_mutex_unlock(&mCaptureMutex);
        return __LINE__;
    }

    if (ret != 0)
    {
        LOGW("wait v4l2 buffer time out");
        pthread_mutex_unlock(&mCaptureMutex);
        LOGW("preview queue has %d items.", OSAL_GetElemNum(&mQueueBufferPreview));
        //recoveryPreviewFrame();
        return __LINE__;
    }

    // get one video frame
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(v4l2_buffer));
    ret = getPreviewFrame(&buf);
#ifdef __A50__
    isp_hal_params parameter;
    memset(&parameter, 0, sizeof(isp_hal_params));
    mAWIspApi->awIspGetParams(&parameter);
#endif
    unsigned long  time1 = systemTime() / 1000000; 
    unsigned long delta = time1-time0;
    LOGV("zjw,v4l2WaitCameraReady. after systemTime %ld ms,time1- time0 = %ld ms",time1,delta);
    if(delta > 50)
    {
        LOGW("getPreviewFrame time out. framerate is lower than 20 fps.");
    }

    LOGV("zjw,getPreviewFrame buffer index = %d,",buf.index);

    if (ret != OK)
    {
        pthread_mutex_unlock(&mCaptureMutex);

        usleep(10000);
        return ret;
    }
    LOGV("The camera driver have %d availbuffer now.",mCurAvailBufferCnt);
    //modify for cts by clx
    mCurAvailBufferCnt--;
    if (mCurAvailBufferCnt <= 2)
        {
        mNeedHalfFrameRate = true;
        mStatisicsIndex = 0;
    }
    else if (mNeedHalfFrameRate)
            {
        mStatisicsIndex++;
        if (mStatisicsIndex >= STATISICS_CNT)
        {
            mNeedHalfFrameRate = false;
        }

    }

    // deal with this frame
#ifdef __CEDARX_FRAMEWORK_1__
    mCurFrameTimestamp = (int64_t)((int64_t)buf.timestamp.tv_usec + (((int64_t)buf.timestamp.tv_sec) * 1000000));
#elif defined __CEDARX_FRAMEWORK_2__
    mCurFrameTimestamp = (int64_t)systemTime();

#endif
    //get picture flag
    mTakePictureFlag.dwval = buf.reserved;

    if (mLastZoom != mNewZoom)
    {
        float widthRate = (float)mFrameWidth / (float)mVideoWidth;
        float heightRate = (float)mFrameHeight / (float)mVideoHeight;
        if (mIsThumbUsedForVideo && (widthRate > 4.0) && (heightRate > 4.0))
        {
            calculateCrop(&mRectCrop, mNewZoom, mMaxZoom, mFrameWidth/2, mFrameHeight/2);    // for cts, to do
        }
        else
        {
            // the main frame crop
            calculateCrop(&mRectCrop, mNewZoom, mMaxZoom, mFrameWidth, mFrameHeight);
        }
        mCameraHardware->setNewCrop(&mRectCrop);

        // the sub-frame crop
        if (mHalCameraInfo.fast_picture_mode)
        {
            calculateCrop(&mThumbRectCrop, mNewZoom, mMaxZoom, mThumbWidth, mThumbHeight);
        }

        mLastZoom = mNewZoom;
        mZoomRatio = (mNewZoom * 2 * 100 / mMaxZoom + 100);
        LOGV("CROP: [%d, %d, %d, %d]", mRectCrop.left, mRectCrop.top, mRectCrop.right, mRectCrop.bottom);
        LOGV("thumb CROP: [%d, %d, %d, %d]", mThumbRectCrop.left, mThumbRectCrop.top, mThumbRectCrop.right, mThumbRectCrop.bottom);
    }

#if DBG_CAPTURE_STREAM_FROM_V4L2DEVICE_FORMJPEG
	LOGD("%s: zjw,test DBG_CAPTURE_STREAM_FROM_V4L2DEVICE_FORMJPEG!", __FUNCTION__);

	char fnameMJPEG[128] ;
	char fnameMJPEG_tail[128];
	char MJPEG_tail[128] = ".jpg";

	if(frame_num0++ < LIMITED_NUM) {
            v4l2device_tail_num++;
            
            sprintf(fnameMJPEG,DBG_CAPTURE_V4L2DEVICE_PATH);
            sprintf(fnameMJPEG_tail,"%d",v4l2device_tail_num);
            
            strcat(fnameMJPEG,fnameMJPEG_tail);
            strcat(fnameMJPEG,MJPEG_tail);

            fp_stream_from_v4l2device = fopen(fnameMJPEG,"w");
            if(fp_stream_from_v4l2device == NULL) {
            	LOGE("%s: zjw,Open DBG_CAPTURE_V4L2DEVICE_PATH fail!", __FUNCTION__);
            } else {
            	fwrite(mMapMem.mem[buf.index],buf.bytesused,1,fp_stream_from_v4l2device);
            	fflush(fp_stream_from_v4l2device);
            	fclose(fp_stream_from_v4l2device);
            }
       }
#endif

    // V4L2BUF_t for preview and HW encoder
    V4L2BUF_t v4l2_buf;
    unsigned int length = 0;

//TOOLS FOR CAPTURE BITSTREAM
//if we capture the stream that is H264,we must keep the first frame ,
//because the first frame of H264 stream own the sps(Sequence Parameter Sets) and pps(Picture Parameter Set) seri
#if DBG_CAPTURE_STREAM_FROM_V4L2DEVICE_FORH264
	if((frame_num0++ < LIMITED_NUM)&&(fp_stream_from_v4l2device != NULL))
	{
		fwrite(mMapMem.mem[buf.index],buf.bytesused,1,fp_stream_from_v4l2device);
		fflush(fp_stream_from_v4l2device);
	}
#endif

    if((mCaptureFormat == V4L2_PIX_FMT_MJPEG) || (mCaptureFormat == V4L2_PIX_FMT_H264))
    {
        mDataInfo.nLength        =buf.bytesused;
        mDataInfo.nPts            =(int64_t)mCurFrameTimestamp/1000;
        Libve_dec2(&mDecoder,
        mMapMem.mem[buf.index],
        (void*)mVideoBuffer.buf_vir_addr[buf.index],
        &mVideoInfo,
        &mDataInfo,
        &mVideoConf);
        unsigned long  timedec = systemTime() / 1000000;
        unsigned long  delatedec = 	timedec - time1;
        LOGV("zjw,after Libve_dec2 for getting yuv stream.timedec:%ld,delatedec:%ld.",timedec,delatedec);
    }
    if (mVideoFormat != V4L2_PIX_FMT_YUYV
        && mCaptureFormat == V4L2_PIX_FMT_YUYV)
    {
#ifdef USE_MP_CONVERT
        YUYVToYUV420C((void*)buf.m.offset,
                      (void*)(mVideoBuffer.buf_phy_addr[buf.index] | 0x40000000),
                      mFrameWidth,
                      mFrameHeight);
#else
        YUYVToNV21(mMapMem.mem[buf.index],
                       (void*)mVideoBuffer.buf_vir_addr[buf.index],
                       mFrameWidth,
                       mFrameHeight);
#endif
    }

/*#ifndef USE_CSI_VIN_DRIVER
    if(mDiscardFrameNum < DISCARD_FRAME_NUM) {
        mDiscardFrameNum ++;
        goto DEC_REF;
    } else if(mDiscardFrameNum < 100) {
        mDiscardFrameNum ++;
    } else {
        mDiscardFrameNum = DISCARD_FRAME_NUM + 1;
    }
#endif*/


    if (mVideoFormat != V4L2_PIX_FMT_YUYV
        && mCaptureFormat == V4L2_PIX_FMT_YUYV)
    {
        v4l2_buf.addrPhyY        = mVideoBuffer.buf_phy_addr[buf.index];
        v4l2_buf.addrVirY        = mVideoBuffer.buf_vir_addr[buf.index];
        v4l2_buf.nShareBufFd = mVideoBuffer.nShareBufFd[buf.index];
    }
    else if(mVideoFormat != V4L2_PIX_FMT_YUYV
        &&( (mCaptureFormat == V4L2_PIX_FMT_MJPEG) || (mCaptureFormat == V4L2_PIX_FMT_H264) ))
    {
        // may be (addr - 0x20000000) or (addr & 0x0fffffff)
        v4l2_buf.addrVirY       =  mVideoBuffer.buf_vir_addr[buf.index];
        v4l2_buf.width          = mFrameWidth;//mVideoInfo.nWidth;
        v4l2_buf.height         = mFrameHeight;//mVideoInfo.nHeight;
    }
    else
    {
        v4l2_buf.addrPhyY        = buf.m.offset - BUFFER_PHY_OFFSET;
        v4l2_buf.addrVirY        = (unsigned long)mMapMem.mem[buf.index];
        v4l2_buf.nShareBufFd = mMapMem.nShareBufFd[buf.index];
    }
    v4l2_buf.index                = buf.index;
    v4l2_buf.timeStamp            = mCurFrameTimestamp;
    v4l2_buf.width                = mFrameWidth;
    v4l2_buf.height                = mFrameHeight;
    v4l2_buf.crop_rect.left        = mRectCrop.left;
    v4l2_buf.crop_rect.top        = mRectCrop.top;
    v4l2_buf.crop_rect.width    = mRectCrop.right - mRectCrop.left + 1;
    v4l2_buf.crop_rect.height    = mRectCrop.bottom - mRectCrop.top + 1;
    v4l2_buf.format                = mVideoFormat;

#ifdef USE_DEINTERLACE_HW
    if (mCameraType == CAMERA_TYPE_TVIN_NTSC || mCameraType == CAMERA_TYPE_TVIN_PAL
        || mCameraType == CAMERA_TYPE_TVIN_YPBPR) {
        v4l2_buf.refMutex.lock();
        v4l2_buf.refCnt++;
        v4l2_buf.refMutex.unlock();
        memcpy(&mV4l2DiBuf[v4l2_buf.index], &v4l2_buf, sizeof(V4L2BUF_t));

        if ((mPrePreFrameIndex >= 0) && (mPreFrameIndex >= 0) && (mDiProcess != NULL)) {

            if (mCaptureFormat == V4L2_PIX_FMT_NV61) {
                mDiProcess->diProcess((unsigned char*)mV4l2DiBuf[mPrePreFrameIndex].addrPhyY,
                                      (unsigned char*)mV4l2DiBuf[mPreFrameIndex].addrPhyY,
                                      (unsigned char *)v4l2_buf.addrPhyY,
                                      mFrameWidth, mFrameHeight, mCaptureFormat,
                                      (unsigned char *)mDiOutPutBuffer.buf_phy_addr[buf.index + NB_BUFFER],
                                      mFrameWidth, mFrameHeight, mCaptureFormat, 0);
                mMemOpsS->flush_cache_cam((void*)mDiOutPutBuffer.buf_vir_addr[buf.index + NB_BUFFER], ALIGN_16B(mFrameWidth) * mFrameHeight * 2);
            } else {
                mDiProcess->diProcess((unsigned char*)mV4l2DiBuf[mPrePreFrameIndex].addrPhyY,
                          (unsigned char*)mV4l2DiBuf[mPreFrameIndex].addrPhyY,
                          (unsigned char *)v4l2_buf.addrPhyY,
                          mFrameWidth, mFrameHeight, mCaptureFormat,
                          (unsigned char *)mDiOutPutBuffer.buf_phy_addr[buf.index],
                          mFrameWidth, mFrameHeight, mCaptureFormat, 0);

            }
            mDiProcess->diProcess((unsigned char*)mV4l2DiBuf[mPrePreFrameIndex].addrPhyY,
                                  (unsigned char*)mV4l2DiBuf[mPreFrameIndex].addrPhyY,
                                  (unsigned char *)v4l2_buf.addrPhyY,
                                  mFrameWidth, mFrameHeight, mCaptureFormat,
                                  (unsigned char *)mDiOutPutBuffer.buf_phy_addr[buf.index],
                                  mFrameWidth, mFrameHeight, mCaptureFormat, 1);

            v4l2_buf.addrPhyY = mDiOutPutBuffer.buf_phy_addr[buf.index];
            v4l2_buf.addrVirY = mDiOutPutBuffer.buf_vir_addr[buf.index];
            if (mCaptureFormat == V4L2_PIX_FMT_NV61) {
                mMemOpsS->flush_cache_cam((void*)mDiOutPutBuffer.buf_vir_addr[buf.index], ALIGN_16B(mFrameWidth) * mFrameHeight * 2);
            } else {
                mMemOpsS->flush_cache_cam((void*)mDiOutPutBuffer.buf_vir_addr[buf.index], ALIGN_16B(mFrameWidth) * mFrameHeight * 3 >> 1);
            }
        }
        mPrePreFrameIndex = mPreFrameIndex;
        mPreFrameIndex = buf.index;
    }
#endif

#if DBG_BUFFER_SAVE
    LOGD("Debug to save yuv data! Size:%dx%d",mFrameWidth,mFrameHeight);
    length = GPU_BUFFER_ALIGN(ALIGN_16B(mFrameWidth)*mFrameHeight*3/2);
    saveframe("/data/camera/capture.bin",(void *)v4l2_buf.addrVirY, length, 1);
    saveSize(mFrameWidth,mFrameHeight);
 
#endif

    if (mHalCameraInfo.fast_picture_mode)
    {
        v4l2_buf.isThumbAvailable        = 1;
        v4l2_buf.thumbUsedForPreview    = 1;
        v4l2_buf.thumbUsedForPhoto        = 0;
        if(mIsThumbUsedForVideo == true)
        {
            v4l2_buf.thumbUsedForVideo        = 1;
        }
        else
        {
            v4l2_buf.thumbUsedForVideo        = 0;
        }
        v4l2_buf.thumbAddrPhyY            = v4l2_buf.addrPhyY + ALIGN_4K(ALIGN_16B(mFrameWidth) * mFrameHeight * 3 / 2);    // to do
        v4l2_buf.thumbAddrVirY            = v4l2_buf.addrVirY + ALIGN_4K(ALIGN_16B(mFrameWidth) * mFrameHeight * 3 / 2);    // to do
        v4l2_buf.thumbWidth                = mThumbWidth;
        v4l2_buf.thumbHeight            = mThumbHeight;
        v4l2_buf.thumb_crop_rect.left    = mThumbRectCrop.left;
        v4l2_buf.thumb_crop_rect.top    = mThumbRectCrop.top;
        v4l2_buf.thumb_crop_rect.width    = mThumbRectCrop.right - mThumbRectCrop.left;
        v4l2_buf.thumb_crop_rect.height    = mThumbRectCrop.bottom - mThumbRectCrop.top;
        v4l2_buf.thumbFormat            = mVideoFormat;
    }
    else
    {
        v4l2_buf.isThumbAvailable        = 0;
    }

    v4l2_buf.refMutex.lock();
    v4l2_buf.refCnt = 1;
    v4l2_buf.refMutex.unlock();

    memcpy(&mV4l2buf[v4l2_buf.index], &v4l2_buf, sizeof(V4L2BUF_t));
    if ((!mVideoHint) && (mTakePictureState != TAKE_PICTURE_NORMAL))
    {
        // face detection only use when picture mode
        mCurrentV4l2buf = &mV4l2buf[v4l2_buf.index];
    }

//TOOLS FOR CAPTURE BITSTREAM
//We must be in there(after DISCARD_FRAME_NUM flag) ,because
//the first frame usb camera captured always is wrong..
#if DBG_CAPTURE_STREAM_FROM_V4L2DEVICE_FORMJPEG
   // if((frame_num0++ < LIMITED_NUM)&&(fp_stream_from_v4l2device != NULL))
   // {
   //     fwrite(mMapMem.mem[buf.index],buf.bytesused,1,fp_stream_from_v4l2device);
   //     fflush(fp_stream_from_v4l2device);
   // }
//The number of YUYV-bitstream we captured is 'mFrameWidth * mFrameHeight * 2'
//that(The number) is different from H264-bitstream and MJPEG-bitstream..
#elif DBG_CAPTURE_STREAM_FROM_V4L2DEVICE_FORYUYV
    if((frame_num0++ < LIMITED_NUM)&&(fp_stream_from_v4l2device != NULL))
    {
        fwrite(mMapMem.mem[buf.index],mFrameWidth * mFrameHeight * 2,1,fp_stream_from_v4l2device);
        fflush(fp_stream_from_v4l2device);
    }
#elif DBG_CAPTURE_STREAM_FROM_V4L2DEVICE_FORNV21
    if((frame_num0++ < LIMITED_NUM)&&(fp_stream_from_v4l2device != NULL))
    {
        fwrite(mMapMem.mem[buf.index],mFrameWidth * mFrameHeight * 3/2,1,fp_stream_from_v4l2device);
        fflush(fp_stream_from_v4l2device);
    }
#endif

#if DBG_CAPTURE_STREAM_AFTER_TRANSFORMATION
    //We only consider three formats(V4L2_PIX_FMT_YUYV
    //,V4L2_PIX_FMT_MJPEG,V4L2_PIX_FMT_H264).
    //Notice:
    //1.If mCaptureFormat == V4L2_PIX_FMT_NV21,
    //we cann't capture the bitstream in there.
    //2.@mFrameWidth * mFrameHeight *3/2 , NV12 has 12 bits == 1.5bytes ,
    //so , in there, we should write x*3/2==x*1.5 data to file.
    if((frame_num0++ < LIMITED_NUM)&&(fp_stream_after_transformation != NULL))
    {
        fwrite((void*)mVideoBuffer.buf_vir_addr[buf.index],1,mFrameWidth * mFrameHeight *3/2,fp_stream_after_transformation);
        fflush(fp_stream_after_transformation);
    }
#endif


// because the begin Several frames(1-3 frames) from vin is black must discard them
// fix cts testAllocationFromCameraFlexibleYuv failed
#if  (defined __PLATFORM_A50__) || (defined __PLATFORM_T5__)
    if(mDiscardFrameNum < DISCARD_FRAME_NUM) {
        mDiscardFrameNum ++;
        goto DEC_REF;
    } else if(mDiscardFrameNum < 100) {
        mDiscardFrameNum ++;
    } else {
        mDiscardFrameNum = DISCARD_FRAME_NUM + 1;
    }
#endif


    if (mTakePictureState == TAKE_PICTURE_NORMAL)
    {
#if (defined __PLATFORM_A50__) || (defined __PLATFORM_T5__)
        if(mDiscardFrameNum < PICTURE_DISCARD_FRAME_NUM) {
            mDiscardFrameNum ++;
            goto DEC_REF;
        } else {
            mDiscardFrameNum = 0;
        }
#endif
        // copy picture buffer
        // For some case using iommu, we allocate bigger than original buffer for align.
        int frame_size = GPU_BUFFER_ALIGN(ALIGN_16B(mFrameWidth)*ALIGN_16B(mFrameHeight)*3/2);
        if (frame_size > MAX_PICTURE_SIZE)
        {
            LOGE("picture buffer size(%d) is smaller than the frame buffer size(%d)", MAX_PICTURE_SIZE, frame_size);
            pthread_mutex_unlock(&mCaptureMutex);
            return false;
        }
        //copy picture buffer
        memcpy((void*)&mPicBuffer, &v4l2_buf, sizeof(V4L2BUF_t));
        mPicBuffer.addrVirY = (unsigned long)mMemOpsS->palloc_cam(frame_size,&mPicBuffer.nShareBufFd);
        if (mPicBuffer.addrVirY == 0) {
            LOGE("picture buffer addrVirY is NULL");
            return __LINE__;
        }

        //mMemOpsS->sync_cache_cam(mPicBuffer.nShareBufFd);

//For H6 A50, using iommu to encoder a jpeg picture, we should take care the align question.
/*#if (defined __PLATFORM_H6__) || (defined __PLATFORM_A50__)
        memcpy((void*)mPicBuffer.addrVirY, (void*)v4l2_buf.addrVirY, mFrameWidth*mFrameHeight);
        memcpy((void*)(mPicBuffer.addrVirY+ALIGN_16B(mFrameWidth)*ALIGN_16B(mFrameHeight)),
            (void*)(v4l2_buf.addrVirY+(mFrameWidth)*(mFrameHeight)),
            (mFrameWidth)*(mFrameHeight)/2);
#else*/
        memcpy((void*)mPicBuffer.addrVirY, (void*)v4l2_buf.addrVirY,frame_size);
//#endif

        mMemOpsS->sync_cache_cam(mPicBuffer.nShareBufFd);

#if DBG_CAPTURE_STREAM_FROM_V4L2DEVICE_TAKEPICTURE
        LOGD("%s: zheng jiangwei,test DBG_CAPTURE_STREAM_FROM_V4L2DEVICE_TAKEPICTURE!", __FUNCTION__);

        char fname0[128] ;
        char fname0_tail[128];
        frame_num0 = 0;
        v4l2device_tail_num++;

        sprintf(fname0,DBG_CAPTURE_V4L2DEVICE_PATH);
        sprintf(fname0_tail,"%d",v4l2device_tail_num);

        strcat(fname0,fname0_tail);

        fp_stream_from_v4l2device = fopen(fname0,"w");
        if(fp_stream_from_v4l2device == NULL) {
            LOGE("%s: zheng jiangwei,Open DBG_CAPTURE_V4L2DEVICE_PATH fail!", __FUNCTION__);
        } else {
            fwrite((void*)mPicBuffer.addrVirY,ALIGN_16B(mFrameWidth) * ALIGN_16B(mFrameHeight) * 3/2,1,fp_stream_from_v4l2device);
            fflush(fp_stream_from_v4l2device);
            fclose(fp_stream_from_v4l2device);
        }
#endif

        //get Exif info for driver
        struct isp_exif_attribute exif_attri;
        getExifInfo(&exif_attri);
        mCallbackNotifier->setExifInfo(exif_attri,mZoomRatio,mExposureBias);
        mCameraHardware->setExifInfo(exif_attri);

        // enqueue picture buffer
        ret = OSAL_Queue(&mQueueBufferPicture, &mPicBuffer);
        if (ret != 0)
        {
            LOGW("picture queue full");
            pthread_cond_signal(&mPictureCond);
            goto DEC_REF;
        }

        mIsPicCopy = true;
        mCaptureThreadState = CAPTURE_STATE_PAUSED;
        mTakePictureState = TAKE_PICTURE_NULL;
        pthread_cond_signal(&mPictureCond);

        goto DEC_REF;
    }
    else
    {
        mV4l2buf[v4l2_buf.index].nShareBufFd = mMapMem.nShareBufFd[v4l2_buf.index];
        ret = OSAL_Queue(&mQueueBufferPreview, &mV4l2buf[v4l2_buf.index]);
        if (ret != 0)
        {
            LOGW("preview queue full");
            goto DEC_REF;
        }

        // add reference count
        mV4l2buf[v4l2_buf.index].refMutex.lock();
        mV4l2buf[v4l2_buf.index].refCnt++;
        mV4l2buf[v4l2_buf.index].refMutex.unlock();

        // signal a new frame for preview
        LOGV("signal a new frame for preview!");
        pthread_cond_signal(&mPreviewCond);

        if (mTakePictureState == TAKE_PICTURE_SCENE_MODE)
        {
            int ret;
            int frame_size = mFrameWidth * mFrameHeight * 3 >> 1;
            if(mSceneMode == NULL)
            {
                pthread_mutex_unlock(&mCaptureMutex);
                return false;
            }
            if(mSceneMode->GetCurrentSceneMode() != SCENE_FACTORY_MODE_HDR &&
                mSceneMode->GetCurrentSceneMode() != SCENE_FACTORY_MODE_NIGHT)
                goto DEC_REF;
            #ifdef USE_ION_MEM_ALLOCATOR
            mMemOpsS->flush_cache_cam((void*)v4l2_buf.addrVirY, frame_size);
            #elif USE_SUNXI_MEM_ALLOCATOR
            sunxi_flush_cache((void*)v4l2_buf.addrVirY, frame_size);
            #endif
            ret = mSceneMode->GetCurrentFrameData((void *)v4l2_buf.addrVirY);
            //capture target frames finish
            if(ret == SCENE_CAPTURE_DONE || ret == SCENE_CAPTURE_FAIL){
                ret = OSAL_Queue(&mQueueBufferPicture, &mV4l2buf[v4l2_buf.index]);
                if (ret != 0) {
                    LOGW("picture queue full"); //?? no test ??
                    pthread_cond_signal(&mPictureCond);
                    goto DEC_REF;
                }

                mV4l2buf[v4l2_buf.index].refMutex.lock();
                mV4l2buf[v4l2_buf.index].refCnt++;
                mV4l2buf[v4l2_buf.index].refMutex.unlock();

                mTakePictureState = TAKE_PICTURE_NULL;  //stop take picture
                mIsPicCopy = false;
                pthread_cond_signal(&mPictureCond);
            }
        }
        if (mTakePictureState == TAKE_PICTURE_FAST
            || mTakePictureState == TAKE_PICTURE_RECORD)
        {
            //LOGD("xxxxxxxxxxxxxxxxxxxx buf.reserved: %x", buf.reserved);
            if (mHalCameraInfo.fast_picture_mode || mFlashMode)
            {
                //get Exif info for driver
                struct isp_exif_attribute exif_attri;
                static int flash_fire = 0;
                getExifInfo(&exif_attri);
                if(mTakePictureState == TAKE_PICTURE_FAST){
                    //in order to get the right flash status ,it must check the status here.
                    //beause the flash and the flag is asynchronous,it means current flash
                    //mode is on while flash_fire set at least one time.
                    if(exif_attri.flash_fire == 1)
                        flash_fire = 1;
                    //the current frame buffer is no the target frame,it can't be used for jpg encode
                    static int count = 0;
                    if (mFlashMode == V4L2_FLASH_LED_MODE_NONE  && \
                        mTakePictureFlag.bits.capture_ok == 0) {

                        LOGV("mTakePictureFlag.bits.capture_ok:%d\n",mTakePictureFlag.bits.capture_ok);
                        if(count++ > 20) {
                            count = 0;
                            setTakePictureCtrl(V4L2_TAKE_PICTURE_NORM);
                        }
                        goto DEC_REF;
                    } else {
                        count = 0;
                    }
                    //LOGD("mFlashMode:%d, mFlashCount:%d.\n", mFlashMode, mFlashCount);
                #ifdef __A50__
                    //when flash mode,the target frame buffer flag is 1
                    if (parameter.ith_params.image_para.bits.flash_ok != 1 &&
                        mFlashMode != V4L2_FLASH_LED_MODE_NONE)
                        goto DEC_REF;
                #endif
                    LOGV("mFlashMode:%d, the picture is ok!\n", mFlashMode);
                    exif_attri.flash_fire = flash_fire;
                    flash_fire = 0;
                }

                mCallbackNotifier->setExifInfo(exif_attri,mZoomRatio,mExposureBias);
                mCameraHardware->setExifInfo(exif_attri);

                int SnrValue = getSnrValue();
                int Gain = ((SnrValue >> V4L2_GAIN_SHIFT)&0xff);
                int SharpLevel = (SnrValue >> V4L2_SHARP_LEVEL_SHIFT) &0xfff;
                int SharpMin = (SnrValue >> V4L2_SHARP_MIN_SHIFT)&0x3f;
                int NdfTh = ((SnrValue >> V4L2_NDF_SHIFT) &0x1f) ;
                LOGD("Gain = %d, SharpLevel = %d, SharpMin = %d, NdfTh = %d!", Gain, SharpLevel, SharpMin, NdfTh);
                if (NdfTh > 1)
                {
                    unsigned char *p_addr =(unsigned char *)malloc((ALIGN_16B(mFrameWidth)*mFrameHeight)>>1);
                    ColorDenoise(p_addr, (unsigned char *)v4l2_buf.addrVirY+ALIGN_16B(mFrameWidth)*mFrameHeight, mFrameWidth, mFrameHeight >> 1, NdfTh + Gain / 24);//6);
                    memcpy((unsigned char *)v4l2_buf.addrVirY+ALIGN_16B(mFrameWidth)*mFrameHeight,p_addr,((ALIGN_16B(mFrameWidth)*mFrameHeight)>>1));
                    free(p_addr);
                }
                if (SharpLevel > 0)
                {
                    Sharpen((unsigned char *)v4l2_buf.addrVirY, SharpMin,SharpLevel, mFrameWidth,mFrameHeight);
                }
            }
            else{
                struct isp_exif_attribute exif_attri;
                getExifInfo(&exif_attri);
                mCallbackNotifier->setExifInfo(exif_attri,mZoomRatio,mExposureBias);
                mCameraHardware->setExifInfo(exif_attri);
            }

            // enqueue picture buffer
            ret = OSAL_Queue(&mQueueBufferPicture, &mV4l2buf[v4l2_buf.index]);
            if (ret != 0)
            {
                LOGW("picture queue full");
                pthread_cond_signal(&mPictureCond);
                goto DEC_REF;
            }

            // add reference count
            mV4l2buf[v4l2_buf.index].refMutex.lock();
            mV4l2buf[v4l2_buf.index].refCnt++;
            mV4l2buf[v4l2_buf.index].refMutex.unlock();

            mTakePictureState = TAKE_PICTURE_NULL;
            mIsPicCopy = false;
            pthread_cond_signal(&mPictureCond);
        }

        if (mTakePictureState == TAKE_PICTURE_SMART)
        {
            //get Exif info for driver
            if (mHalCameraInfo.fast_picture_mode)
                if (mTakePictureFlag.bits.fast_capture_ok == 0) {
                    goto DEC_REF;}
            struct isp_exif_attribute exif_attri;
            getExifInfo(&exif_attri);
            mCallbackNotifier->setExifInfo(exif_attri,mZoomRatio,mExposureBias);
            mCameraHardware->setExifInfo(exif_attri);

            // enqueue picture buffer
            ret = OSAL_Queue(&mQueueBufferPicture, &mV4l2buf[v4l2_buf.index]);
            if (ret != 0)
                {
                LOGW("picture queue full");
                pthread_cond_signal(&mSmartPictureCond);
                    goto DEC_REF;
                }
            // add reference count
            mV4l2buf[v4l2_buf.index].refMutex.lock();
            mV4l2buf[v4l2_buf.index].refCnt++;
            mV4l2buf[v4l2_buf.index].refMutex.unlock();
            //mTakePictureState = TAKE_PICTURE_NULL;
            mIsPicCopy = false;
            pthread_cond_signal(&mSmartPictureCond);
        }

        if ((mTakePictureState == TAKE_PICTURE_CONTINUOUS
            || mTakePictureState == TAKE_PICTURE_CONTINUOUS_FAST)
            && isContinuousPictureTime())
        {
            if (mTakePictureFlag.bits.fast_capture_ok == 0)
                goto DEC_REF;
            // enqueue picture buffer
            ret = OSAL_Queue(&mQueueBufferPicture, &mV4l2buf[v4l2_buf.index]);
            if (ret != 0)
            {
                // LOGV("continuous picture queue full");
                pthread_cond_signal(&mContinuousPictureCond);
                goto DEC_REF;
            }

            //get Exif info for driver
            struct isp_exif_attribute exif_attri;
            getExifInfo(&exif_attri);
            mCallbackNotifier->setExifInfo(exif_attri,mZoomRatio,mExposureBias);
            mCameraHardware->setExifInfo(exif_attri);
            // add reference count
            mV4l2buf[v4l2_buf.index].refMutex.lock();
            mV4l2buf[v4l2_buf.index].refCnt++;
            mV4l2buf[v4l2_buf.index].refMutex.unlock();
            mIsPicCopy = false;
            pthread_cond_signal(&mContinuousPictureCond);
        }
    }

DEC_REF:
    pthread_mutex_unlock(&mCaptureMutex);
#ifdef USE_DEINTERLACE_HW
    if ((mCameraType == CAMERA_TYPE_TVIN_NTSC || mCameraType == CAMERA_TYPE_TVIN_PAL
        || mCameraType == CAMERA_TYPE_TVIN_YPBPR) && (mPrePreFrameIndex >= 0) && (mDiProcess != NULL)) {
        releasePreviewFrame(mPrePreFrameIndex);
    }
#endif
    releasePreviewFrame(v4l2_buf.index);

    return true;
}

bool V4L2CameraDevice::previewThread()
{
    F_LOG;
    int releasebufferindex = -1;

    V4L2BUF_t * pbuf = (V4L2BUF_t *)OSAL_Dequeue(&mQueueBufferPreview);
    if (pbuf == NULL)
    {
        LOGV("preview queue no buffer, sleep...");
        pthread_mutex_lock(&mPreviewMutex);
        pthread_cond_wait(&mPreviewCond, &mPreviewMutex);
        pthread_mutex_unlock(&mPreviewMutex);
        return true;
    }
    //nsecs_t beforePrevew = (int64_t)systemTime();
    pthread_mutex_lock(&mPreviewSyncMutex);
    if(mPreviewThreadState == PREVIEW_STATE_PAUSED){
        LOGV("preview thread paused");
        pthread_cond_wait(&mPreviewSyncCond, &mPreviewSyncMutex);
    }

    Mutex::Autolock locker(&mObjectLock);
    if (mMapMem.mem[pbuf->index] == NULL
        /*|| pbuf->addrPhyY == 0*/)
    {
        LOGV("preview buffer have been released...");
        pthread_mutex_unlock(&mPreviewSyncMutex);
        return true;
    }

//workaroud  increase fps for cts
#if 0
    if (strcmp(mCameraHardware->mCallingProcessName, "android.camera.cts") == 0 ||
        strcmp(mCameraHardware->mCallingProcessName, "com.android.cts.hardware") == 0 ) {
        int sleep_duration_ms = 1000 /mFrameRate  ; //ms
        uint32_t used_duration_us;
        uint32_t sleep_duration_us = sleep_duration_ms*1000;
        LOGV("cts : duration set fps : %d, real fps : %d",mFrameRate,getFrameRate());
        LOGV("cts : sleep_duration_us : %ld us",sleep_duration_us);
        LOGV("cts :ElemNum : %d  num",OSAL_GetElemNum(&mQueueBufferPreview));
        int i = 0;

        if(OSAL_GetElemNum(&mQueueBufferPreview)){
            //mPreviewWindow->onNextFrameAvailable((void*)pbuf); //by hjs for test
            releasebufferindex = mPreviewWindow->onNextFrameAvailable2(pbuf->index);
            mCallbackNotifier->onNextFrameAvailable(mPreviewWindow->mBufferHandle[pbuf->index],(void*)pbuf,pbuf->index);
        }

        while(i < 10 && OSAL_GetElemNum(&mQueueBufferPreview) == 0){
            pbuf->timeStamp = systemTime();
            //mPreviewWindow->onNextFrameAvailable((void*)pbuf); //by hjs for test
            releasebufferindex = mPreviewWindow->onNextFrameAvailable2(pbuf->index);
            mCallbackNotifier->onNextFrameAvailable(mPreviewWindow->mBufferHandle[pbuf->index],(void*)pbuf,pbuf->index);
            used_duration_us = (uint32_t)ns2us(systemTime() - pbuf->timeStamp);
            if (sleep_duration_us > used_duration_us)
                usleep(sleep_duration_us -used_duration_us);
            LOGV("cts : i=%d   used_duration_us : %ld us",i, used_duration_us);
            LOGV("cts sleep_duration_us -used_duration_us: %d us",sleep_duration_us -used_duration_us);
            LOGV("cts :while  =  %d us",(uint32_t)ns2us(systemTime()-pbuf->timeStamp));
            i++;
        }
    }
    //else {
#endif
    if(mCameraType == CAMERA_TYPE_UVC
        || mCameraType == CAMERA_TYPE_CSI
        || mCameraType == CAMERA_TYPE_VFE)
    {
        int src_width,src_height = 0;
        if ((pbuf->isThumbAvailable == 1)
            && (pbuf->thumbUsedForPreview == 1))
        {
            src_width            = pbuf->thumbWidth;
            src_height            = pbuf->thumbHeight;
        }
        else
        {
            src_width            = pbuf->width;
            src_height            = pbuf->height;
        }
        if((src_width != 0) && (src_height != 0)) {
          mCallbackNotifier->onNextFrameAvailable(mPreviewWindow->mBufferHandle[pbuf->index],(void*)pbuf,pbuf->index);
          releasebufferindex = mPreviewWindow->onNextFrameAvailable2(pbuf->index, pbuf->crop_rect);
        }else {
          LOGW("skip error preview-frame ");
        }

        if(releasebufferindex >= 0){
            releasePreviewFrame(releasebufferindex);
        }

        pthread_mutex_unlock(&mPreviewSyncMutex);
    }

#ifdef USE_DEINTERLACE_HW
    else if(mCameraType == CAMERA_TYPE_TVIN_NTSC
            ||mCameraType == CAMERA_TYPE_TVIN_PAL
            ||mCameraType == CAMERA_TYPE_TVIN_YPBPR)
    {
        mCallbackNotifier->onNextFrameAvailable((void*)pbuf, mUseHwEncoder);
    // preview
//<< xqz@aw added, 180309
        if(mHalCameraInfo.device_id !=8 && (mCameraType == CAMERA_TYPE_TVIN_NTSC
            ||mCameraType == CAMERA_TYPE_TVIN_PAL
            ||mCameraType == CAMERA_TYPE_TVIN_YPBPR)
            && (mFrameWidth >= 720))
        {
            pbuf->width = mFrameWidth;
            pbuf->height = mFrameHeight;
            pbuf->crop_rect.left     = 40;
            pbuf->crop_rect.top      = 0;
            pbuf->crop_rect.width    = 640-1;
            pbuf->crop_rect.height   = 480-1;
            //mPreviewWindow->onNextFrameAvailable((void*)pbuf);
            //goto RELEASE;
        }
//>> xqz@aw added

        if (mCameraType == CAMERA_TYPE_TVIN_NTSC || mCameraType == CAMERA_TYPE_TVIN_PAL
            || mCameraType == CAMERA_TYPE_TVIN_YPBPR)
        {
            if(mCaptureFormat == V4L2_PIX_FMT_NV61)
            {
                if ((mPrePreFrameIndex >= 0) && (mPreFrameIndex >= 0) && (mDiProcess != NULL))
                {
                    if (!mNeedHalfFrameRate || mShouldPreview)
                    {
                        V4L2BUF_t pbufFrist;
                        memcpy(&pbufFrist, pbuf, sizeof(V4L2BUF_t));
                        pbufFrist.addrPhyY = mDiOutPutBuffer.buf_phy_addr[pbuf->index + NB_BUFFER];
                        pbufFrist.addrVirY = mDiOutPutBuffer.buf_vir_addr[pbuf->index + NB_BUFFER];
                        pbufFrist.nShareBufFd = mDiOutPutBuffer.nShareBufFd[pbuf->index + NB_BUFFER];
                        mPreviewWindow->onNextFrameAvailable((void*)(&pbufFrist));
                    }
                }
            }
            else
            {
                if ((mPrePreFrameIndex >= 0) && (mPreFrameIndex >= 0) && (mDiProcess != NULL))
                {
                    if (!mNeedHalfFrameRate || mShouldPreview)
                    {
                        V4L2BUF_t pbufFrist;
                        memcpy(&pbufFrist, pbuf, sizeof(V4L2BUF_t));
                        pbufFrist.addrPhyY = mDiOutPutBuffer.buf_phy_addr[pbuf->index];
                        pbufFrist.addrVirY = mDiOutPutBuffer.buf_vir_addr[pbuf->index];
                        pbufFrist.nShareBufFd = mDiOutPutBuffer.nShareBufFd[pbuf->index];
                        mPreviewWindow->onNextFrameAvailable((void*)(&pbufFrist));
                    }
                }
            }
        }
        mShouldPreview = mShouldPreview ? false : true;
        releasePreviewFrame(pbuf->index);
        pthread_mutex_unlock(&mPreviewSyncMutex);
    }
#endif

    return true;
}
void V4L2CameraDevice::stopPreviewThread()
{
    LOGV("stop preview thread!\n");
    pthread_mutex_lock(&mPreviewSyncMutex);
    if(mPreviewThreadState == PREVIEW_STATE_STARTED)
    {
        mPreviewThreadState = PREVIEW_STATE_PAUSED;
        LOGV("set mPreviewThreadState to %d\n",mPreviewThreadState);
    }
    pthread_mutex_unlock(&mPreviewSyncMutex);
}

// singal picture
bool V4L2CameraDevice::pictureThread()
{
    V4L2BUF_t * pbuf = (V4L2BUF_t *)OSAL_Dequeue(&mQueueBufferPicture);
    if (pbuf == NULL)
    {
        LOGV("picture queue no buffer, sleep...");
        pthread_mutex_lock(&mPictureMutex);
        pthread_cond_wait(&mPictureCond, &mPictureMutex);
        pthread_mutex_unlock(&mPictureMutex);
        return true;
    }

    DBG_TIME_BEGIN("taking picture", 0);

    // notify picture cb
    mCameraHardware->notifyPictureMsg((void*)pbuf);
    if(mSceneMode != NULL){
        if(mSceneMode->GetCurrentSceneMode() == SCENE_FACTORY_MODE_HDR){
            int ret;
            ret = mSceneMode->PostScenePicture((void*) pbuf->addrVirY);
            stopSceneModePicture();
        }
        else if(mSceneMode->GetCurrentSceneMode() == SCENE_FACTORY_MODE_NIGHT){
            int ret;
            ret = mSceneMode->PostScenePicture((void*) pbuf->addrVirY);
            stopSceneModePicture();
        }
    }

    mCallbackNotifier->takePicture((void*)pbuf,(void*)mMemOpsS);

    char str[128];
    sprintf(str, "hw picture size: %dx%d", pbuf->width, pbuf->height);
    DBG_TIME_DIFF(str);

    if (!mIsPicCopy)
    {
        releasePreviewFrame(pbuf->index);
    }
    if (mPicBuffer.addrVirY != 0)
    {
        mMemOpsS->pfree_cam((void*)mPicBuffer.addrVirY);
        mPicBuffer.addrPhyY = 0;
    }

    DBG_TIME_END("Take picture", 0);

    return true;
}
// blink picture
bool V4L2CameraDevice::smartPictureThread()
{
    V4L2BUF_t * pbuf = (V4L2BUF_t *)OSAL_Dequeue(&mQueueBufferPicture);

    if (pbuf == NULL)
    {
        pthread_mutex_lock(&mSmartPictureMutex);
        pthread_cond_wait(&mSmartPictureCond, &mSmartPictureMutex);
        pthread_mutex_unlock(&mSmartPictureMutex);
        return true;
    }

    // apk stop smart pictures
    if (mSmartPictureDone)
    {
        mTakePictureState = TAKE_PICTURE_NULL;
        if (!mIsPicCopy)
        {
            releasePreviewFrame(pbuf->index);
        }
        return true;
    }

    #if 0

    ALOGD("!! mCameraHardware->mBlinkPictureResult %d state %d", mCameraHardware->mBlinkPictureResult, mCameraHardware->mBlinkDetectionState);

    if ((mCameraHardware->mBlinkPictureResult == true) && (mCameraHardware->mBlinkDetectionState == FACE_DETECTION_PREPARED))
    {
        DBG_TIME_BEGIN("taking blink picture", 0);

        // notify picture cb
        mCameraHardware->notifyPictureMsg((void*)pbuf);

        DBG_TIME_DIFF("notifyPictureMsg");

        mCallbackNotifier->takePicture((void*)pbuf,(void*)mMemOpsS);

        stopSmartPicture();
        mTakePictureState = TAKE_PICTURE_NULL;
    }

    #endif

    #if 1

    ALOGD("!!! mCameraHardware->mSmilePictureResult %d, state %d", mCameraHardware->mSmilePictureResult, mCameraHardware->mSmileDetectionState);

    if ((mCameraHardware->mSmilePictureResult == true) && (mCameraHardware->mSmileDetectionState == FACE_DETECTION_PREPARED))
    {
        DBG_TIME_BEGIN("taking smile picture", 0);

        // notify picture cb
        mCameraHardware->notifyPictureMsg((void*)pbuf);

        DBG_TIME_DIFF("notifyPictureMsg");

        mCallbackNotifier->takePicture((void*)pbuf,(void*)mMemOpsS);

        stopSmartPicture();
        mTakePictureState = TAKE_PICTURE_NULL;
    }

    #endif

    #if 1

    if (mStartSmartTimeout == false)
    {
        if ((systemTime() / 1000000 - mStartSmartTimeMs) > 5000)    // 5s timeout
        {
            mStartSmartTimeout = true;

            ALOGV("taking smile picture time out!!!");

            DBG_TIME_BEGIN("taking smile picture time out!!!", 0);

            // notify picture cb
            mCameraHardware->notifyPictureMsg((void*)pbuf);

            DBG_TIME_DIFF("notifyPictureMsg");

            mCallbackNotifier->takePicture((void*)pbuf,(void*)mMemOpsS);

            stopSmartPicture();
            mTakePictureState = TAKE_PICTURE_NULL;
        }
    }

    #endif

    char str[128];
    sprintf(str, "hw picture size: %dx%d", pbuf->width, pbuf->height);
    DBG_TIME_DIFF(str);

    if (!mIsPicCopy)
    {
        releasePreviewFrame(pbuf->index);
    }

    DBG_TIME_END("Take smart picture", 0);


    return true;
}

void V4L2CameraDevice::startSmartPicture()
{
    F_LOG;

    mSmartPictureDone = false;
    mStartSmartTimeout = false;
    mStartSmartTimeMs = systemTime() / 1000000;

    DBG_TIME_AVG_INIT(TAG_SMART_PICTURE);
}

void V4L2CameraDevice::stopSmartPicture()
{
    F_LOG;

    if (mSmartPictureDone)
    {
        LOGD("Smart picture has already stopped");
        return;
    }
    mStartSmartTimeout = true;
    mSmartPictureDone = true;

    DBG_TIME_AVG_END(TAG_SMART_PICTURE, "picture enc");
}

status_t V4L2CameraDevice::openSceneMode(const char* scenemode)
{
    F_LOG;
    int ret;
    int mode = 0;
    //HDR sence
    if (!strcmp(scenemode, CameraParameters::SCENE_MODE_HDR)){
        mode = SCENE_FACTORY_MODE_HDR;
    }
    if(!strcmp(scenemode, CameraParameters::SCENE_MODE_NIGHT)){
        mode = SCENE_FACTORY_MODE_NIGHT;
    }

    mSceneMode = mSceneModeFactory.CreateSceneMode(mode);
    if(mSceneMode == NULL){
        LOGE("SceneModeFactory CreateSceneMode failed");
        mSceneModeFactory.DestorySceneMode(NULL);
        return -1;
    }
    mSceneMode->SetCallBack(SceneNotifyCallback,(void*)this);

    ret = mSceneMode->InitSceneMode(mFrameWidth,mFrameHeight);
    if(ret == -1){
        LOGE("SceneMode Init faided");
        return -1;
    }
    return 0;
}

void V4L2CameraDevice::closeSceneMode()
{
    F_LOG;

    if(mSceneMode != NULL){
        mSceneMode->ReleaseSceneMode();
        mSceneModeFactory.DestorySceneMode(mSceneMode);
        mSceneMode = NULL;
        //when close scene mode, it must restore the flash status
        setFlashMode(mFlashMode);
    }
}


void V4L2CameraDevice::startSceneModePicture(int scenemode)
{
    F_LOG
    mSceneMode->StartScenePicture();
}

void V4L2CameraDevice::stopSceneModePicture()
{
    F_LOG
    mSceneMode->StopScenePicture();
}

// continuous picture
bool V4L2CameraDevice::continuousPictureThread()
{
    V4L2BUF_t * pbuf = (V4L2BUF_t *)OSAL_Dequeue(&mQueueBufferPicture);
    if (pbuf == NULL)
    {
        LOGV("continuousPictureThread queue no buffer, sleep...");
        pthread_mutex_lock(&mContinuousPictureMutex);
        pthread_cond_wait(&mContinuousPictureCond, &mContinuousPictureMutex);
        pthread_mutex_unlock(&mContinuousPictureMutex);
        return true;
    }

    Mutex::Autolock locker(&mObjectLock);
    if (mMapMem.mem[pbuf->index] == NULL
        || pbuf->addrPhyY == 0)
    {
        LOGV("picture buffer have been released...");
        return true;
    }

    DBG_TIME_AVG_AREA_IN(TAG_CONTINUOUS_PICTURE);

    // reach the max number of pictures
    if (mContinuousPictureCnt >= mContinuousPictureMax)
    {
        mTakePictureState = TAKE_PICTURE_NULL;
        stopContinuousPicture();
        releasePreviewFrame(pbuf->index);
        return true;
    }

    // apk stop continuous pictures
    if (!mContinuousPictureStarted)
    {
        mTakePictureState = TAKE_PICTURE_NULL;
        releasePreviewFrame(pbuf->index);
        return true;
    }

    bool ret = mCallbackNotifier->takePicture((void*)pbuf,(void*)mMemOpsS, true);
    if (ret)
    {
        mContinuousPictureCnt++;

        DBG_TIME_AVG_AREA_OUT(TAG_CONTINUOUS_PICTURE);
    }
    else
    {
        // LOGW("do not encoder jpeg");
    }

    releasePreviewFrame(pbuf->index);

    return true;
}

void V4L2CameraDevice::startContinuousPicture()
{
    F_LOG;

    mContinuousPictureCnt = 0;
    mContinuousPictureStarted = true;
    mContinuousPictureStartTime = systemTime(SYSTEM_TIME_MONOTONIC);

    DBG_TIME_AVG_INIT(TAG_CONTINUOUS_PICTURE);
}

void V4L2CameraDevice::stopContinuousPicture()
{
    F_LOG;

    if (!mContinuousPictureStarted)
    {
        LOGD("Continuous picture has already stopped");
        return;
    }

    mContinuousPictureStarted = false;

    nsecs_t time = (systemTime(SYSTEM_TIME_MONOTONIC) - mContinuousPictureStartTime)/1000000;
    LOGD("Continuous picture cnt: %d, use time %lld(ms)", mContinuousPictureCnt, time);
    if (time != 0)
    {
        LOGD("Continuous picture %f(fps)", (float)mContinuousPictureCnt/(float)time * 1000);
    }

    DBG_TIME_AVG_END(TAG_CONTINUOUS_PICTURE, "picture enc");
}

void V4L2CameraDevice::setContinuousPictureCnt(int cnt)
{
    F_LOG;
    mContinuousPictureMax = cnt;
}

bool V4L2CameraDevice::isContinuousPictureTime()
{
    if (mTakePictureState == TAKE_PICTURE_CONTINUOUS_FAST)
    {
        return true;
    }

    timeval cur_time;
    gettimeofday(&cur_time, NULL);
    const uint64_t cur_mks = cur_time.tv_sec * 1000000LL + cur_time.tv_usec;
    if ((cur_mks - mContinuousPictureLast) >= mContinuousPictureAfter) {
        mContinuousPictureLast = cur_mks;
        return true;
    }
    return false;
}

bool V4L2CameraDevice::isPreviewTime()
{
    if (mVideoHint != true)
    {
        return true;
    }
    timeval cur_time;
    gettimeofday(&cur_time, NULL);
    const uint64_t cur_mks = cur_time.tv_sec * 1000000LL + cur_time.tv_usec;
    if ((cur_mks - mPreviewLast) >= mPreviewAfter) {
        mPreviewLast = cur_mks;
        return true;
    }
    return false;
}
void V4L2CameraDevice::waitFaceDectectTime()
{
    timeval cur_time;
    gettimeofday(&cur_time, NULL);
    const uint64_t cur_mks = cur_time.tv_sec * 1000000LL + cur_time.tv_usec;

    if ((cur_mks - mFaceDectectLast) >= mFaceDectectAfter)
    {
        mFaceDectectLast = cur_mks;
    }
    else
    {
        usleep(mFaceDectectAfter - (cur_mks - mFaceDectectLast));
        gettimeofday(&cur_time, NULL);
        mFaceDectectLast = cur_time.tv_sec * 1000000LL + cur_time.tv_usec;
    }
}

int V4L2CameraDevice::getCurrentFaceFrame(void* frame, int* width, int* height)
{
    if (frame == NULL)
    {
        LOGE("getCurrentFrame: error in null pointer");
        return -1;
    }

    pthread_mutex_lock(&mCaptureMutex);
    // stop capture
    if (mCaptureThreadState != CAPTURE_STATE_STARTED)
    {
        LOGW("capture thread dose not started");
        pthread_mutex_unlock(&mCaptureMutex);
        return -1;
    }
    pthread_mutex_unlock(&mCaptureMutex);

#ifdef WATI_FACEDETECT
    waitFaceDectectTime();
#endif

    Mutex::Autolock locker(&mObjectLock);

    if (mCurrentV4l2buf == NULL
        || mCurrentV4l2buf->addrVirY == 0)
    {
        LOGW("frame buffer not ready");
        return -1;
    }
    //LOGV("getCurrentFaceFrame: %dx%d", mCurrentV4l2buf->width, mCurrentV4l2buf->height);

    if ((mCurrentV4l2buf->isThumbAvailable == 1)
        && (mCurrentV4l2buf->thumbUsedForPreview == 1))
    {
        mMemOpsS->flush_cache_cam((char *)mCurrentV4l2buf->addrVirY + (ALIGN_16B(mCurrentV4l2buf->width) * mCurrentV4l2buf->height * 3 / 2), ALIGN_16B(mCurrentV4l2buf->thumbWidth) * mCurrentV4l2buf->thumbHeight);
        memcpy(frame,
                (char*)mCurrentV4l2buf->addrVirY + ALIGN_4K((ALIGN_16B(mCurrentV4l2buf->width) * mCurrentV4l2buf->height * 3 / 2)),
                ALIGN_16B(mCurrentV4l2buf->thumbWidth) * mCurrentV4l2buf->thumbHeight);
        *width = mCurrentV4l2buf->thumbWidth;
        *height = mCurrentV4l2buf->thumbHeight;
    }
    else
    {
        mMemOpsS->flush_cache_cam((void*)mCurrentV4l2buf->addrVirY, mCurrentV4l2buf->width * mCurrentV4l2buf->height);
        memcpy(frame, (void*)mCurrentV4l2buf->addrVirY, mCurrentV4l2buf->width * mCurrentV4l2buf->height);
        *width = mCurrentV4l2buf->width;
        *height = mCurrentV4l2buf->height;
    }
    //LOGV("getCurrentFaceFrame: %dx%d", *width, *height);
    return 0;
}

int V4L2CameraDevice::getTVINSystemType(int fd)
{
    struct v4l2_format format;
    struct v4l2_format format_set;
    struct v4l2_frmsizeenum frmsize;
    enum v4l2_buf_type type;
    int i = 0;
    int temp_height = 0;
    memset(&format, 0, sizeof(struct v4l2_format));
    memset(&format_set, 0, sizeof(struct v4l2_format));
    memset(&frmsize, 0, sizeof(struct v4l2_frmsizeenum));
    frmsize.pixel_format = V4L2_PIX_FMT_NV21;
    frmsize.index = 0;
    while ((!ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize))&&(i < 20))
    {
        LOGV("i = %d\n", i);
        LOGV("framesize width = %d, height = %d\n",frmsize.discrete.width, frmsize.discrete.height);
        i++;
        frmsize.index++;
    }
    memset(&format, 0, sizeof(struct v4l2_format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    i = 0;
    //while (ioctl(fd, VIDIOC_G_FMT, &format) &&(i++ < 20))
    while (!ioctl(fd, VIDIOC_G_FMT, &format) && (i++ < 5))
    {
        LOGV("get tvin signal %d times for system=%d.\n", i, format.fmt.raw_data[RAW_DATA_SYSTEM]);
    }
    memcpy(&format_set,&format,sizeof(struct v4l2_format));
    LOGD("---width= [%d], height =[%d],system=%d,i=%d\n",format.fmt.pix.width, format.fmt.pix.height,format.fmt.raw_data[RAW_DATA_SYSTEM],i);
    format_set.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format_set.fmt.pix.pixelformat = V4L2_PIX_FMT_NV21;

    format_set.fmt.pix.width= format.fmt.pix.width;
    format_set.fmt.pix.height= format.fmt.pix.height;
    if (ioctl(fd, VIDIOC_S_FMT, &format_set) == -1) {
            LOGE("set tvin image format failed\n");
            return -1 ;
    }
    if((format.fmt.raw_data[RAW_DATA_SYSTEM]==0))
    {
        LOGD("------------get tvin ntsc\n");
        return TVD_NTSC;
    }
    else if((format.fmt.raw_data[RAW_DATA_SYSTEM]==1))
    {
        LOGD("------------get tvin pal\n");
        return  TVD_PAL ;
    }
    else if((format.fmt.raw_data[RAW_DATA_SYSTEM]==2)||(format.fmt.raw_data[RAW_DATA_SYSTEM]==3))
    {
        LOGD("---------get tvin ypbpr\n");
        return   TVD_YPBPR;
    }
    LOGD("---------get no type \n");
    return   0;

}

// -----------------------------------------------------------------------------
// extended interfaces here <***** star *****>
// -----------------------------------------------------------------------------
int V4L2CameraDevice::openCameraDev(HALCameraInfo * halInfo)
{
    F_LOG;

    int ret = -1;
    struct v4l2_input inp;
    struct v4l2_capability cap;
    char dev_node[16];

    if (halInfo == NULL)
    {
        LOGE("error HAL camera info");
        return -1;
    }

    LOGD("##########halInfo->device_name:%s",halInfo->device_name);

    // open V4L2 device
    //-----------------------------------------------
    //If "video0" not exist, use others instead
    //Modified by Microphone
    //2013-11-14
    //-----------------------------------------------
    if((access(halInfo->device_name, F_OK)) == 0)
    {
        strcpy(dev_node,halInfo->device_name);
    }
    else
    {
        for (int i = 0; i < MAX_NUM_OF_CAMERAS; i++)
        {
            sprintf(dev_node, "/dev/video%d", i);
            ret = access(dev_node, F_OK);
            if(ret == 0)
            {
                break;
            }
        }
    }
    mCameraFd = open(dev_node, O_RDWR | O_NONBLOCK, 0);

    if (mCameraFd == -1)
    {
        LOGE("ERROR opening %s: %s", halInfo->device_name, strerror(errno));
        return -1;
    }

    LOGD("mCameraFd : %d opened",mCameraFd);

    // check v4l2 device capabilities
    ret = ioctl (mCameraFd, VIDIOC_QUERYCAP, &cap);
    if (ret < 0)
    {
        LOGE("Error opening device: unable to query device.");
        goto END_ERROR;
    }

    if (!strcmp((char *)cap.driver, "uvcvideo"))
    {
        mCameraType = CAMERA_TYPE_UVC;
        halInfo->is_uvc = true;
        LOGD("mCameraType = CAMERA_TYPE_UVC");
    }else if (!strcmp((char *)cap.driver, "sunxi-tvd")) {
        int tvinType;
        tvinType = getTVINSystemType(mCameraFd);
        if (tvinType == TVD_PAL) {
            mCameraType = CAMERA_TYPE_TVIN_PAL;
            LOGD("mCameraType = CAMERA_TYPE_TVIN_PAL");
        } else if (tvinType == TVD_NTSC) {
            mCameraType = CAMERA_TYPE_TVIN_NTSC;
            LOGD("mCameraType = CAMERA_TYPE_TVIN_NTSC");
        } else if (tvinType == TVD_YPBPR) {
            mCameraType = CAMERA_TYPE_TVIN_YPBPR;
            LOGD("mCameraType = CAMERA_TYPE_TVIN_YPBPR");
        } else {
            mCameraType = CAMERA_TYPE_TVIN_NTSC;
            LOGD("Default mCameraType = CAMERA_TYPE_TVIN_NTSC");
        }
    }
    else if(halInfo->is_vfe){
        mCameraType = CAMERA_TYPE_VFE;
        LOGD("mCameraType = CAMERA_TYPE_VFE");
    }
    else{
        mCameraType = CAMERA_TYPE_CSI;
        LOGD("mCameraType = CAMERA_TYPE_CSI");
    }

    if(mCameraType != CAMERA_TYPE_CSI)
    {
        LOGV("is usb or vfe tvin camera!");
        if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0)
        {
            LOGE("Error opening device: video capture not supported.");
            goto END_ERROR;
        }
    }
    else
    {
        LOGV("is csi or mipi camera!");
        if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE) == 0)
        {
            LOGE("Error opening device: video capture not supported.");
            goto END_ERROR;
        }
    }

    if ((cap.capabilities & V4L2_CAP_STREAMING) == 0)
    {
        LOGE("Capture device does not support streaming i/o");
        goto END_ERROR;
    }

    if (mCameraType == CAMERA_TYPE_VFE || mCameraType == CAMERA_TYPE_CSI)
    {
        // uvc do not need to set input
        inp.index = halInfo->device_id;
        if (-1 == ioctl (mCameraFd, VIDIOC_S_INPUT, &inp))
        {
            LOGE("VIDIOC_S_INPUT error!");
            goto END_ERROR;
        }
    }

    if (mCameraType == CAMERA_TYPE_UVC)
    {
        if (tryFmt(V4L2_PIX_FMT_NV21) == OK)
        {
            mCaptureFormat = V4L2_PIX_FMT_NV21;
            LOGV("capture format: V4L2_PIX_FMT_NV21");
        }
        else if(tryFmt(V4L2_PIX_FMT_MJPEG) == OK)
        {
            mCaptureFormat = V4L2_PIX_FMT_MJPEG;        // maybe usb camera
            LOGD("MJPEG Camera '%s' is Supported",cap.card);
            LOGV("capture format: V4L2_PIX_FMT_MJPEG");
        }
        else if(tryFmt(V4L2_PIX_FMT_YUYV) == OK)
        {
            mCaptureFormat = V4L2_PIX_FMT_YUYV;        // maybe usb camera
            LOGV("capture format: V4L2_PIX_FMT_YUYV");
        }
        else if(tryFmt(V4L2_PIX_FMT_H264) == OK)
        {
            mCaptureFormat = V4L2_PIX_FMT_H264;
            LOGV("capture format: V4L2_PIX_FMT_H264");
        }
        else
        {
            LOGE("driver should surpport NV21/NV12 or YUYV format, but it not!");
            goto END_ERROR;
        }
    }

    return OK;

END_ERROR:

    if (mCameraFd != NULL)
    {
        LOGD("openCameraDev END_ERROR mCameraFd : %d closed",mCameraFd);
        close(mCameraFd);
        mCameraFd = NULL;
    }

    return -1;
}

void V4L2CameraDevice::closeCameraDev()
{
    F_LOG;
    int ret_close = 0;
    if (mCameraFd != NULL)
    {
        ret_close = close(mCameraFd);
        LOGD("mCameraFd:%d, ret_close: %d, errno:%s",mCameraFd,ret_close,strerror(errno));
        mCameraFd = NULL;
    }
}

int V4L2CameraDevice::v4l2SetVideoParams(int width, int height, uint32_t pix_fmt)
{
    int ret = UNKNOWN_ERROR;
    struct v4l2_format format;

    LOGV("%s, line: %d, w: %d, h: %d, pfmt: %d",
        __FUNCTION__, __LINE__, width, height, pix_fmt);

    memset(&format, 0, sizeof(format));

    if(mCameraType == CAMERA_TYPE_CSI)
    {
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        format.fmt.pix_mp.width  = width;
        format.fmt.pix_mp.height = height;
        format.fmt.pix_mp.field = V4L2_FIELD_NONE;
        format.fmt.pix_mp.pixelformat = pix_fmt;
    }else{
        int i = 0;
        if(mCameraType == CAMERA_TYPE_TVIN_PAL || mCameraType == CAMERA_TYPE_TVIN_NTSC) {
            format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            while (ioctl(mCameraFd, VIDIOC_G_FMT, &format) && (i++ < 20)) {
                LOGD("+++get tvin signal failed.\n");
                return -1;
            }
        }
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        format.fmt.pix.width  = width;
        format.fmt.pix.height = height;
        if (mCaptureFormat == V4L2_PIX_FMT_YUYV) {
            format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
        } else if(mCaptureFormat == V4L2_PIX_FMT_MJPEG) {
            format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
        } else if(mCaptureFormat == V4L2_PIX_FMT_H264) {
            format.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
        } else {
            format.fmt.pix.pixelformat = pix_fmt;
        }
        format.fmt.pix.field = V4L2_FIELD_NONE;
    }

    ret = ioctl(mCameraFd, VIDIOC_S_FMT, &format);
    if (ret < 0)
    {
        LOGE("VIDIOC_S_FMT Failed: %s", strerror(errno));
        return ret;
    }

    if (mCameraType == CAMERA_TYPE_CSI) {
        ret = ioctl(mCameraFd, VIDIOC_G_FMT, &format);
        if (ret < 0) {
            LOGE("VIDIOC_G_FMT Failed: %s", strerror(errno));
            return ret;
        } else {
            nplanes = format.fmt.pix_mp.num_planes;
            LOGD("VIDIOC_G_FMT resolution = %d*%d num_planes = %d\n",format.fmt.pix_mp.width, format.fmt.pix_mp.height,format.fmt.pix_mp.num_planes);
        }
        mFrameWidth = format.fmt.pix_mp.width;
        mFrameHeight = format.fmt.pix_mp.height;
    } else {
        mFrameWidth = format.fmt.pix.width;
        mFrameHeight = format.fmt.pix.height;
    }

#ifdef USE_SUNXI_CAMERA_H
    struct rot_channel_cfg rot;
    struct v4l2_pix_format sub_fmt;

    if (mHalCameraInfo.fast_picture_mode)
    {
        memset(&sub_fmt, 0, sizeof(sub_fmt));
        memset(&rot, 0, sizeof(rot));

        int scale = getSuitableThumbScale(width, height);
        if (scale <= 0)
        {
            LOGE("error thumb scale: %d, src_size: %dx%d", scale, width, height);
            return UNKNOWN_ERROR;
        }
        int sub_width,sub_height;
        sub_width = format.fmt.pix.width / scale;
        sub_height = format.fmt.pix.height / scale;
        mCameraHardware->getPriviewSize(&sub_width,&sub_height,format.fmt.pix.width,format.fmt.pix.height);

        sub_fmt.width = sub_width;
        sub_fmt.height = sub_height;
        sub_fmt.pixelformat = pix_fmt;
        sub_fmt.field = V4L2_FIELD_NONE;

        rot.sel_ch                = 1;
        rot.rotation            = 0;

        ret = ioctl(mCameraFd, VIDIOC_SET_SUBCHANNEL, &sub_fmt);
        if (ret < 0)
        {
            LOGE("VIDIOC_SET_SUBCHANNEL Failed: %s", strerror(errno));
            return ret;
        }

        ret = ioctl(mCameraFd, VIDIOC_SET_ROTCHANNEL, &rot);
        if (ret < 0)
        {
            LOGE("VIDIOC_SET_ROTCHANNEL Failed: %s", strerror(errno));
            return ret;
        }

        mThumbWidth = sub_fmt.width;
        mThumbHeight = sub_fmt.height;
    }

    if (mHalCameraInfo.fast_picture_mode)
    {
        mThumbWidth = mFrameWidth;//format.fmt.pix.subchannel->width;
        mThumbHeight = mFrameHeight;//format.fmt.pix.subchannel->height;
    }

    LOGV("to camera params: w: %d, h: %d, sub: %dx%d, pfmt: %d, pfield: %d",
            format.fmt.pix.width, format.fmt.pix.height,
            sub_fmt.width, sub_fmt.height, pix_fmt, V4L2_FIELD_NONE);

    LOGV("camera params: w: %d, h: %d, sub: %dx%d, pfmt: %d, pfield: %d",
        mFrameWidth, mFrameHeight, mThumbWidth, mThumbHeight, pix_fmt, V4L2_FIELD_NONE);
#endif

    return OK;
}

int V4L2CameraDevice::v4l2ReqBufs(int * buf_cnt)
{
    F_LOG;
    int ret = UNKNOWN_ERROR;
    struct v4l2_requestbuffers rb;

    LOGV("TO VIDIOC_REQBUFS count: %d", *buf_cnt);

    memset(&rb, 0, sizeof(rb));
    rb.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (mCameraType == CAMERA_TYPE_CSI) {
        rb.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    }
    //rb.memory = V4L2_MEMORY_MMAP;
    //rb.memory =  (mIsUsbCamera == true) ? V4L2_MEMORY_MMAP : V4L2_MEMORY_USERPTR;
    if(mCameraType == CAMERA_TYPE_CSI || mCameraType == CAMERA_TYPE_VFE)
    {
        rb.memory = V4L2_MEMORY_USERPTR;
    }
    else
    {
        rb.memory = V4L2_MEMORY_MMAP;
    }
    rb.count  = *buf_cnt;

    ret = ioctl(mCameraFd, VIDIOC_REQBUFS, &rb);
    if (ret < 0)
    {
        LOGE("Init: VIDIOC_REQBUFS failed: %s", strerror(errno));
        return ret;
    }

    *buf_cnt = rb.count;
    LOGV("VIDIOC_REQBUFS count: %d", *buf_cnt);

    return OK;
}

int V4L2CameraDevice::v4l2QueryBuf()
{
    F_LOG;
    int ret = UNKNOWN_ERROR;
    struct v4l2_buffer buf;

    for (int i = 0; i < mBufferCnt; i++)
    {
        memset (&buf, 0, sizeof (struct v4l2_buffer));

        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (mCameraType == CAMERA_TYPE_CSI) {
            struct v4l2_plane planes[VIDEO_MAX_PLANES];
            memset(planes, 0, VIDEO_MAX_PLANES*sizeof(struct v4l2_plane));
            buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
            buf.length = nplanes;
            buf.m.planes = planes;
            if (NULL == buf.m.planes) {
                LOGE("buf.m.planes calloc failed!\n");
            }
        }
        //buf.memory = V4L2_MEMORY_MMAP;
        //buf.memory = (mIsUsbCamera == true) ? V4L2_MEMORY_MMAP : V4L2_MEMORY_USERPTR;
        if(mCameraType == CAMERA_TYPE_CSI || mCameraType == CAMERA_TYPE_VFE)
        {
            buf.memory = V4L2_MEMORY_USERPTR;
        }
        else
        {
            buf.memory = V4L2_MEMORY_MMAP;
        }
        buf.index  = i;

        ret = ioctl (mCameraFd, VIDIOC_QUERYBUF, &buf);
        if (ret < 0)
        {
            LOGE("Unable to query buffer (%s)", strerror(errno));
            return ret;
        }

        switch (buf.memory) {
            case V4L2_MEMORY_MMAP:
        LOGE("V4L2_MEMORY_MMAP");
                mMapMem.mem[i] = mmap (0, buf.length,
                            PROT_READ | PROT_WRITE,
                            MAP_SHARED,
                            mCameraFd,
                            buf.m.offset);
                mMapMem.length = buf.length;
                if (mMapMem.mem[i] == MAP_FAILED){
                    LOGE("Unable to map buffer (%s)", strerror(errno));
                    for(int j = 0;j < i;j++){
                        munmap(mMapMem.mem[j], mMapMem.length);
                    }
                    return -1;
                }
                LOGD("index: %d, mem: %lx, len: %x, offset: %x", i, (unsigned long)mMapMem.mem[i], buf.length, buf.m.offset);
                break;
            case V4L2_MEMORY_USERPTR:
        LOGE("V4L2_MEMORY_USERPTR");
                mMapMem.mem[i] = mPreviewWindow->ddr_vir[i];
                mMapMem.length = GPU_BUFFER_ALIGN(ALIGN_16B(mFrameWidth)*mFrameHeight*3/2);/*buf.length;*/
                mMapMem.length = buf.length;
                mMapMem.nShareBufFd[i] = mPreviewWindow->mPrivateHandle[i]->share_fd; //share fd to picture encode
                if (mMapMem.mem[i] == NULL){
                     LOGE("buffer allocate ERROR(%s)", strerror(errno));
                     return -1;
                }
                if(mCameraType == CAMERA_TYPE_CSI)
                {
                    mMapMem.length = buf.m.planes[0].length;
                    buf.m.planes[0].m.userptr = (unsigned long)mMapMem.mem[i];
                    buf.m.planes[0].length = mMapMem.length;
                }
                else
                {
                    buf.m.userptr = (unsigned long)mMapMem.mem[i];
                }
                LOGV("index: %d, mem: 0x%lx, mMapMem len: %d, fd: %d", i, (unsigned long)mMapMem.mem[i], mMapMem.length, mMapMem.nShareBufFd[i]);
                break;
            default:
                break;
            }

        // start with all buffers in queue
        ret = ioctl(mCameraFd, VIDIOC_QBUF, &buf);
        if (ret < 0)
        {
            LOGE("VIDIOC_QBUF Failed");
            return ret;
        }

        if (mCameraType == CAMERA_TYPE_UVC)        // star to do
        {
            int buffer_len = mFrameWidth * mFrameHeight * 3 / 2;
#ifdef USE_SHARE_BUFFER
            mMapMem.nShareBufFd[i] = mPreviewWindow->mPrivateHandle[i]->share_fd; //share fd to picture encode
            mVideoBuffer.buf_vir_addr[i] = (unsigned long)mPreviewWindow->ddr_vir[i];
#else 
#ifdef USE_ION_MEM_ALLOCATOR
            mVideoBuffer.buf_vir_addr[i] = (unsigned long)mMemOpsS->palloc_cam(buffer_len,&mVideoBuffer.nDmaBufFd[i]);
#elif USE_SUNXI_MEM_ALLOCATOR
            mVideoBuffer.buf_vir_addr[i] = (unsigned long)sunxi_alloc_alloc(buffer_len);
#endif/*USE_ION_MEM_ALLOCATOR*/
#endif/*USE_SHARE_BUFFER*/

            LOGD("index: %d, vir: %x, phy: %x, len: %x",
                    i, (unsigned int)mVideoBuffer.buf_vir_addr[i], mVideoBuffer.buf_phy_addr[i], buffer_len);

            memset((void*)mVideoBuffer.buf_vir_addr[i], 0x10, mFrameWidth * mFrameHeight);
            memset((char *)mVideoBuffer.buf_vir_addr[i] + mFrameWidth * mFrameHeight,
                    0x80, mFrameWidth * mFrameHeight / 2);
        }

#ifdef USE_DEINTERLACE_HW
        if (mCameraType == CAMERA_TYPE_TVIN_NTSC || mCameraType == CAMERA_TYPE_TVIN_PAL
            || mCameraType == CAMERA_TYPE_TVIN_YPBPR) {

            if (mCaptureFormat == V4L2_PIX_FMT_NV61) {

                int buffer_size = ALIGN_16B(mFrameWidth) * mFrameHeight * 2;

                mDiOutPutBuffer.buf_vir_addr[i] = (unsigned long)mMemOpsS->palloc_cam(buffer_size,
                    &mDiOutPutBuffer.nShareBufFd[i]);
                mDiOutPutBuffer.buf_phy_addr[i] = (unsigned long)mMemOpsS->cpu_get_phyaddr_cam((void*)mDiOutPutBuffer.buf_vir_addr[i]);

                LOGV("DiOutPutbuffer: index: %d, vir: %x, phy: %x, len: %x, V4L2_PIX_FMT_NV61",
                        i, mDiOutPutBuffer.buf_vir_addr[i], mDiOutPutBuffer.buf_phy_addr[i], buffer_size);

                memset((void*)mDiOutPutBuffer.buf_vir_addr[i], 0x10, ALIGN_16B(mFrameWidth) * mFrameHeight);
                memset((void*)(mDiOutPutBuffer.buf_vir_addr[i] + ALIGN_16B(mFrameWidth) * mFrameHeight),
                        0x80, mFrameWidth * mFrameHeight);

                mDiOutPutBuffer.buf_vir_addr[i + NB_BUFFER] = (unsigned long)mMemOpsS->palloc_cam(buffer_size,
                    &mDiOutPutBuffer.nShareBufFd[i + NB_BUFFER]);
                mDiOutPutBuffer.buf_phy_addr[i + NB_BUFFER] = (unsigned long)mMemOpsS->cpu_get_phyaddr_cam(
                (void*)mDiOutPutBuffer.buf_vir_addr[i + NB_BUFFER]);

                LOGV("DiOutPutbuffer: index: %d, vir: %x, phy: %x, len: %x, V4L2_PIX_FMT_NV61",
                    i + NB_BUFFER, mDiOutPutBuffer.buf_vir_addr[i + NB_BUFFER], mDiOutPutBuffer.buf_phy_addr[i + NB_BUFFER], buffer_size);

                memset((void*)mDiOutPutBuffer.buf_vir_addr[i + NB_BUFFER], 0x10, ALIGN_16B(mFrameWidth) * mFrameHeight);
                memset((void*)(mDiOutPutBuffer.buf_vir_addr[i + NB_BUFFER] + ALIGN_16B(mFrameWidth) * mFrameHeight),
                        0x80, mFrameWidth * mFrameHeight);
            }else {
                int buffer_size = ALIGN_16B(mFrameWidth) * mFrameHeight * 3 >> 1;

                mDiOutPutBuffer.buf_vir_addr[i] = (unsigned long)mMemOpsS->palloc_cam(buffer_size,
                    &mDiOutPutBuffer.nShareBufFd[i]);
                mDiOutPutBuffer.buf_phy_addr[i] = (unsigned long)mMemOpsS->cpu_get_phyaddr_cam((void*)mDiOutPutBuffer.buf_vir_addr[i]);

                LOGV("DiOutPutbuffer: index: %d, vir: %x, phy: %x, len: %x",
                        i, mDiOutPutBuffer.buf_vir_addr[i], mDiOutPutBuffer.buf_phy_addr[i], buffer_size);

                memset((void*)mDiOutPutBuffer.buf_vir_addr[i], 0x10, ALIGN_16B(mFrameWidth) * mFrameHeight);
                memset((void*)(mDiOutPutBuffer.buf_vir_addr[i] + ALIGN_16B(mFrameWidth) * mFrameHeight),
                        0x80, mFrameWidth * mFrameHeight / 2);
            }

        }
#endif

    }

    return OK;
}

int V4L2CameraDevice::v4l2StartStreaming()
{
    F_LOG;
    int ret = UNKNOWN_ERROR;
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (mCameraType == CAMERA_TYPE_CSI) {
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    }

    ret = ioctl (mCameraFd, VIDIOC_STREAMON, &type);
    if (ret < 0)
    {
        LOGE("StartStreaming: Unable to start capture: %s", strerror(errno));
        return ret;
    }

    return OK;
}

int V4L2CameraDevice::v4l2StopStreaming()
{
    F_LOG;
    int ret = UNKNOWN_ERROR;
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (mCameraType == CAMERA_TYPE_CSI) {
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    }
    ret = ioctl (mCameraFd, VIDIOC_STREAMOFF, &type);
    if (ret < 0)
    {
        LOGE("StopStreaming: Unable to stop capture: %s", strerror(errno));
        return ret;
    }
    LOGV("V4L2Camera::v4l2StopStreaming OK");

    return OK;
}

int V4L2CameraDevice::v4l2UnmapBuf()
{
    F_LOG;
    int ret = UNKNOWN_ERROR;


    for (int i = 0; i < mBufferCnt; i++)
    {
        if (mCameraType == CAMERA_TYPE_UVC || mCameraType == CAMERA_TYPE_TVIN_NTSC ||
            mCameraType == CAMERA_TYPE_TVIN_PAL || mCameraType == CAMERA_TYPE_TVIN_YPBPR)
        {
            LOGV("v4l2UnmapBuf v4l2CloseBuf Unmap ");
            ret = munmap(mMapMem.mem[i], mMapMem.length);
            if (ret < 0) {
                LOGE("v4l2CloseBuf Unmap failed");
                return ret;
            }
        }
        mMapMem.mem[i] = NULL;

        if (mVideoBuffer.buf_vir_addr[i] != 0)
        {
#ifdef USE_ION_MEM_ALLOCATOR
            mMemOpsS->pfree_cam((void*)mVideoBuffer.buf_vir_addr[i]);
            mVideoBuffer.buf_phy_addr[i] = 0;
#elif USE_SUNXI_MEM_ALLOCATOR
            sunxi_alloc_free((void*)mVideoBuffer.buf_vir_addr[i]);

            mVideoBuffer.buf_phy_addr[i] = 0;
#endif
        }
#ifdef USE_DEINTERLACE_HW
        if (mDiOutPutBuffer.buf_vir_addr[i] != 0) {
            mMemOpsS->pfree_cam((void*)mDiOutPutBuffer.buf_vir_addr[i]);
            mDiOutPutBuffer.buf_phy_addr[i] = 0;
            mDiOutPutBuffer.buf_vir_addr[i] = 0;
        }
        if (mDiOutPutBuffer.buf_vir_addr[i + NB_BUFFER] != 0) {
            mMemOpsS->pfree_cam((void*)mDiOutPutBuffer.buf_vir_addr[i + NB_BUFFER]);
            mDiOutPutBuffer.buf_phy_addr[i + NB_BUFFER] = 0;
            mDiOutPutBuffer.buf_vir_addr[i + NB_BUFFER] = 0;
        }
#endif
    }
    mVideoBuffer.buf_unused = NB_BUFFER;
    mVideoBuffer.read_id = 0;
    mVideoBuffer.write_id = 0;

    return OK;
}

void V4L2CameraDevice::releasePreviewFrame(native_handle_t *handle)
{
    private_handle_t* privatehandle;

    privatehandle = (private_handle_t*) handle;
    if(mCameraType == CAMERA_TYPE_CSI
        || mCameraType == CAMERA_TYPE_UVC
        || mCameraType == CAMERA_TYPE_VFE)
    {
        for(int i = 0; i < mBufferCnt; i ++)
        {
            if(privatehandle->aw_buf_id == mPreviewWindow->mPrivateHandle[i]->aw_buf_id)
                releaseIndex = i;
        }
        LOGV("CB release handle index:%d",releaseIndex);
        if(releaseIndex != -1){
            releasePreviewFrame(releaseIndex);
        }
    }

    //native_handle_close(handle);
    //native_handle_delete(handle);
}

void V4L2CameraDevice::releasePreviewFrame(int index)
{
    int ret = UNKNOWN_ERROR;
    struct v4l2_buffer buf;

    pthread_mutex_lock(&mCaptureMutex);

    // Decrease buffer reference count first.
    mV4l2buf[index].refMutex.lock();
    mV4l2buf[index].refCnt--;
    mV4l2buf[index].refMutex.unlock();

    // If the reference count is equal 0, release it.
    if (mV4l2buf[index].refCnt == 0)
    {
        memset(&buf, 0, sizeof(v4l2_buffer));
#ifdef USE_CSI_VIN_DRIVER
        struct v4l2_plane planes[VIDEO_MAX_PLANES];
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_USERPTR;
        buf.length = nplanes;
        buf.m.planes = planes;
        buf.m.planes[0].m.userptr = (unsigned long)mMapMem.mem[index];
        buf.m.planes[0].length = mMapMem.length;
#else
        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.m.userptr = (unsigned long)mMapMem.mem[index]; //the buffer virtual address
        buf.length = mMapMem.length; //the buffer size
#endif
        buf.index = index;

        LOGV("Qbuf:%d, addr:%x, len:%d", buf.index, mMapMem.mem[index],buf.length);
        ret = ioctl(mCameraFd, VIDIOC_QBUF, &buf);
        if (ret != 0)
        {
            LOGE("releasePreviewFrame: VIDIOC_QBUF Failed: index = %d, ret = %d, addr:%x, len:%d, %s",
                buf.index, ret, (unsigned int)mMapMem.mem[index],buf.length,strerror(errno));
        }
        else
        {
            mCurAvailBufferCnt++;
        }
    }
    pthread_mutex_unlock(&mCaptureMutex);
}

void V4L2CameraDevice::recoveryPreviewFrame()
{
    int ret = UNKNOWN_ERROR;
    struct v4l2_buffer buf;

    pthread_mutex_lock(&mCaptureMutex);

    for(int i=0; i<NB_BUFFER;i++)
    {
        if(((int64_t)systemTime()-mV4l2buf[i].timeStamp)>1000000000)
        //if(mV4l2buf[i].refCnt>0)
        {
            //LOGW("release buff: %d",i);
            memset(&buf, 0, sizeof(v4l2_buffer));
            buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            if (mCameraType == CAMERA_TYPE_CSI) {
                struct v4l2_plane planes[VIDEO_MAX_PLANES];
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
                buf.length = nplanes;
                buf.m.planes = planes;
            }
            if(mCameraType == CAMERA_TYPE_UVC || mCameraType == CAMERA_TYPE_TVIN_NTSC ||
            mCameraType == CAMERA_TYPE_TVIN_PAL || mCameraType == CAMERA_TYPE_TVIN_YPBPR)
            {
                buf.memory = V4L2_MEMORY_MMAP;
            }
            else
            {
                buf.memory = V4L2_MEMORY_USERPTR;
            }
            buf.index = i;

            ret = ioctl(mCameraFd, VIDIOC_QBUF, &buf);
            ALOGD("---------------RecoveryPreviewFrame idx: %d",i);
            if (ret != 0)
            {
                LOGE("recoveryPreviewFrame: VIDIOC_QBUF Failed: index = %d, ret = %d, %s",
                    buf.index, ret, strerror(errno));
            }
            else
            {
                mV4l2buf[i].refCnt =0;
                mCurAvailBufferCnt++;
            }
        }
    }
    pthread_mutex_unlock(&mCaptureMutex);
}

int V4L2CameraDevice::getPreviewFrame(v4l2_buffer *buf)
{
    int ret = UNKNOWN_ERROR;

    if (mCameraType == CAMERA_TYPE_CSI) {
        struct v4l2_plane planes[VIDEO_MAX_PLANES];
        buf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf->memory = V4L2_MEMORY_USERPTR;
        buf->length = nplanes;
        buf->m.planes =planes;
    }
    else
    {
        buf->type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if(mCameraType == CAMERA_TYPE_UVC || mCameraType == CAMERA_TYPE_TVIN_NTSC ||
            mCameraType == CAMERA_TYPE_TVIN_PAL || mCameraType == CAMERA_TYPE_TVIN_YPBPR)
        {
            buf->memory = V4L2_MEMORY_MMAP;
        }
        else
        {
            buf->memory = V4L2_MEMORY_USERPTR;
        }
        //buf->memory =  (mIsUsbCamera == true) ? V4L2_MEMORY_MMAP : V4L2_MEMORY_USERPTR;;
    }

    ret = ioctl(mCameraFd, VIDIOC_DQBUF, buf);

    if (ret < 0)
    {
        if (mDebugLogCnt == 0 || mDebugLogCnt >= 1500) {
            LOGW("GetPreviewFrame: VIDIOC_DQBUF Failed, %s, mCurAvailBufferCnt %d",
                strerror(errno), mCurAvailBufferCnt);
            if (mDebugLogCnt >= 1500) {
                mDebugLogCnt = 0;
            }
        }
        mDebugLogCnt++;
        return __LINE__;            // can not return false
    }
    mDebugLogCnt = 0;
    testFrameRate();
    return OK;
}

int V4L2CameraDevice::tryFmt(int format)
{
    struct v4l2_fmtdesc fmtdesc;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (mCameraType == CAMERA_TYPE_CSI) {
        fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    }

    for(int i = 0; i < 12; i++)
    {
        fmtdesc.index = i;
        if (-1 == ioctl (mCameraFd, VIDIOC_ENUM_FMT, &fmtdesc))
        {
            break;
        }
        LOGV("format index = %d, name = %s, v4l2 pixel format = %x\n",
            i, fmtdesc.description, fmtdesc.pixelformat);

        if ((int)fmtdesc.pixelformat == format)
        {
            return OK;
        }
    }

    return -1;
}

int V4L2CameraDevice::tryFmtSize(int * width, int * height)
{
    F_LOG;
    int ret = -1;
    struct v4l2_format fmt;

    LOGV("Before V4L2Camera::TryFmtSize: w: %d, h: %d", *width, *height);


    memset(&fmt, 0, sizeof(fmt));

    if (mCameraType == CAMERA_TYPE_CSI) {
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        fmt.fmt.pix_mp.pixelformat = mVideoFormat;
        fmt.fmt.pix_mp.width  = *width;
        fmt.fmt.pix_mp.height = *height;
        fmt.fmt.pix_mp.field = V4L2_FIELD_NONE;
        fmt.fmt.pix_mp.pixelformat = mVideoFormat;
    } else {
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width  = *width;
        fmt.fmt.pix.height = *height;
        if (mCaptureFormat == V4L2_PIX_FMT_MJPEG) {
            fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
        } else if (mCaptureFormat == V4L2_PIX_FMT_YUYV) {
            fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
        } else if (mCaptureFormat == V4L2_PIX_FMT_H264) {
            fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
        } else {
            fmt.fmt.pix.pixelformat = mVideoFormat;
        }
        fmt.fmt.pix.field = V4L2_FIELD_NONE;
    }

    ret = ioctl(mCameraFd, VIDIOC_TRY_FMT, &fmt);
    if (ret < 0)
    {
        LOGE("VIDIOC_TRY_FMT Failed: %s", strerror(errno));
        return ret;
    }

    // driver surpport this size
    if (mCameraType == CAMERA_TYPE_CSI) {
        *width = fmt.fmt.pix_mp.width;
        *height = fmt.fmt.pix_mp.height;
    } else {
        *width = fmt.fmt.pix.width;
        *height = fmt.fmt.pix.height;
    }

    return 0;
}

int V4L2CameraDevice::setFrameRate(int rate)
{
    mFrameRate = rate;
    return OK;
}

int V4L2CameraDevice::getFrameRate()
{
    F_LOG;
    int ret = -1;

    struct v4l2_streamparm parms;
    parms.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (mCameraType == CAMERA_TYPE_CSI) {
        parms.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    }

    ret = ioctl (mCameraFd, VIDIOC_G_PARM, &parms);
    if (ret < 0)
    {
        LOGE("VIDIOC_G_PARM getFrameRate error, %s", strerror(errno));
        return ret;
    }

    int numerator = parms.parm.capture.timeperframe.numerator;
    int denominator = parms.parm.capture.timeperframe.denominator;

    LOGV("frame rate: numerator = %d, denominator = %d", numerator, denominator);

    if (numerator != 0
        && denominator != 0)
    {
        return denominator / numerator;
    }
    else
    {
        LOGW("unsupported frame rate: %d/%d", denominator, numerator);
        return 30;
    }
}

int V4L2CameraDevice::setImageEffect(int effect)
{
    F_LOG;
    int ret = -1;
    struct v4l2_control ctrl;

    ctrl.id = V4L2_CID_COLORFX;
    ctrl.value = effect;
    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
        LOGV("setImageEffect failed!");
    else
        LOGV("setImageEffect ok");

    return ret;
}

int V4L2CameraDevice::setWhiteBalance(int wb)
{
    struct v4l2_control ctrl;
    int ret = -1;

    ctrl.id = V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE;
    ctrl.value = wb;
    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
        LOGV("setWhiteBalance failed, %s", strerror(errno));
    else
        LOGV("setWhiteBalance ok");

    return ret;
}

int V4L2CameraDevice::setTakePictureCtrl(enum v4l2_take_picture value)
{
    struct v4l2_control ctrl;
    int ret = -1;
    //if (mHalCameraInfo.fast_picture_mode){
    if (1){
      LOGD("setTakePictureCtrl value = %d",value);
      ctrl.id = V4L2_CID_TAKE_PICTURE;
          ctrl.value = value;
      ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
      if (ret < 0)
          LOGV("setTakePictureCtrl failed, %s", strerror(errno));
      else
          LOGV("setTakePictureCtrl ok");

      return ret;
    }
    return 0;
}

// ae mode
int V4L2CameraDevice::setExposureMode(int mode)
{
    F_LOG;
    int ret = -1;
    struct v4l2_control ctrl;

    ctrl.id = V4L2_CID_EXPOSURE_AUTO;
    ctrl.value = mode;
    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
        LOGV("setExposureMode failed, %s", strerror(errno));
    else
        LOGV("setExposureMode ok");

    return ret;
}

// ae compensation
int V4L2CameraDevice::setExposureCompensation(int val)
{
    F_LOG;
    int ret = -1;
    struct v4l2_control ctrl;
    mExposureBias = val;
    ctrl.id = V4L2_CID_AUTO_EXPOSURE_BIAS;
    ctrl.value = val + 4;
    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
        LOGV("setExposureCompensation failed, %s", strerror(errno));
    else
        LOGV("setExposureCompensation ok");

    return ret;
}

// ae compensation
#ifdef USE_SUNXI_CAMERA_H
int V4L2CameraDevice::setExposureWind(int num, void *wind)
{
    F_LOG;
    int ret = -1;
    struct v4l2_win_setting set_para;

    set_para.win_num = num;
    set_para.coor[0] = *(struct v4l2_win_coordinate*)wind;

    ret = ioctl(mCameraFd, VIDIOC_AUTO_EXPOSURE_WIN, &set_para);
    if (ret < 0)
        LOGE("setExposureWind failed, %s", strerror(errno));
    else
        LOGV("setExposureWind ok");

    return ret;
}
#endif

// flash mode
int V4L2CameraDevice::setFlashMode(int mode)
{
    F_LOG;
    int ret = -1;
    struct v4l2_control ctrl;
    mFlashMode = mode;

    //scene mode must close the flash,eg: HDR,night mode
    //it ought to do in application,it must do it again here
    //in order to insure the status in driver. --by henrisk
    if(mSceneMode != NULL && \
      (mSceneMode->GetCurrentSceneMode() == SCENE_FACTORY_MODE_HDR || \
       mSceneMode->GetCurrentSceneMode() == SCENE_FACTORY_MODE_NIGHT))
        mode = V4L2_FLASH_LED_MODE_NONE;

    ctrl.id = V4L2_CID_FLASH_LED_MODE;
    ctrl.value = mode;
    LOGD("setFlashMode mode = %d",mode);

    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
        LOGV("setFlashMode failed, %s", strerror(errno));
    else
        LOGV("setFlashMode ok");

    return ret;
}

// af init
int V4L2CameraDevice::setAutoFocusInit()
{
    F_LOG;
    int ret = -1;
    struct v4l2_control ctrl;

    if(mCameraType == CAMERA_TYPE_CSI) {
        ctrl.id = V4L2_CID_AUTO_FOCUS_INIT;
        ctrl.value = 0;
        ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
        if (ret < 0)
            LOGE("setAutoFocusInit failed, %s", strerror(errno));
        else
            LOGV("setAutoFocusInit ok");
    }
    return ret;
}

// af release
int V4L2CameraDevice::setAutoFocusRelease()
{
    F_LOG;
    int ret = -1;
    struct v4l2_control ctrl;

    ctrl.id = V4L2_CID_AUTO_FOCUS_RELEASE;
    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
        LOGE("setAutoFocusRelease failed, %s", strerror(errno));
    else
        LOGV("setAutoFocusRelease ok");

    return ret;
}

// af range
int V4L2CameraDevice::setAutoFocusRange(int af_range)
{
    F_LOG;
    int ret = -1;
    struct v4l2_control ctrl;

    ctrl.id = V4L2_CID_FOCUS_AUTO;
    ctrl.value = 1;
    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
        LOGV("setAutoFocusRange id V4L2_CID_FOCUS_AUTO failed, %s", strerror(errno));
    else
        LOGV("setAutoFocusRange id V4L2_CID_FOCUS_AUTO ok");

    ctrl.id = V4L2_CID_AUTO_FOCUS_RANGE;
    ctrl.value = af_range;
    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
        LOGV("setAutoFocusRange id V4L2_CID_AUTO_FOCUS_RANGE failed, %s", strerror(errno));
    else
        LOGV("setAutoFocusRange id V4L2_CID_AUTO_FOCUS_RANGE ok");

    return ret;
}

// af wind
#ifdef USE_SUNXI_CAMERA_H
int V4L2CameraDevice::setAutoFocusWind(int num, void *wind)
{
    F_LOG;
    int ret = -1;
    struct v4l2_win_setting set_para;

    set_para.win_num = num;
    set_para.coor[0] = *(struct v4l2_win_coordinate*)wind;

    ret = ioctl(mCameraFd, VIDIOC_AUTO_FOCUS_WIN, &set_para);
    if (ret < 0)
        LOGE("setAutoFocusCtrl failed, %s", strerror(errno));
    else
        LOGV("setAutoFocusCtrl ok");

    return ret;
}
#endif

// af start
int V4L2CameraDevice::setAutoFocusStart()
{
    F_LOG;
    int ret = -1;
    struct v4l2_control ctrl;

    ctrl.id = V4L2_CID_AUTO_FOCUS_START;
    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
        LOGE("setAutoFocusStart failed, %s", strerror(errno));
    else
        LOGV("setAutoFocusStart ok");

    return ret;
}

// af stop
int V4L2CameraDevice::setAutoFocusStop()
{
    F_LOG;
    int ret = -1;
    struct v4l2_control ctrl;

    ctrl.id = V4L2_CID_AUTO_FOCUS_STOP;
    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
        LOGE("setAutoFocusStart failed, %s", strerror(errno));
    else
        LOGV("setAutoFocusStart ok");

    return ret;
}

// get af statue
int V4L2CameraDevice::getAutoFocusStatus()
{
    //F_LOG;
    int ret = -1;
    struct v4l2_control ctrl;

    if (mCameraFd == NULL)
    {
        return 0xFF000000;
    }

    ctrl.id = V4L2_CID_AUTO_FOCUS_STATUS;
    ret = ioctl(mCameraFd, VIDIOC_G_CTRL, &ctrl);
    if (ret >= 0)
    {
        //LOGV("getAutoFocusCtrl ok");
    }

    return ret;
}
int V4L2CameraDevice::getSnrValue()
{
    //F_LOG;
    int ret = -1;
    struct v4l2_control ctrl;
    struct v4l2_queryctrl qc_ctrl;

    if (mCameraFd == NULL)
    {
        return 0xFF000000;
    }

    ctrl.id = V4L2_CID_GAIN;
    qc_ctrl.id = V4L2_CID_GAIN;

    if (-1 == ioctl (mCameraFd, VIDIOC_QUERYCTRL, &qc_ctrl))
    {
       return 0;
    }

    ret = ioctl(mCameraFd, VIDIOC_G_CTRL, &ctrl);
    return ctrl.value;
}



int V4L2CameraDevice::getGainValue() //get gain (trait specially, need the last 8 bits)
{
    //F_LOG;
    int ret = -1;
    struct v4l2_control ctrl;
    struct v4l2_queryctrl qc_ctrl;

    if (mCameraFd == NULL)
    {
        return 0xFF000000;
    }

    ctrl.id = V4L2_CID_GAIN;
    qc_ctrl.id = V4L2_CID_GAIN;

    if (-1 == ioctl (mCameraFd, VIDIOC_QUERYCTRL, &qc_ctrl))
    {
       return 0;
    }

    ret = ioctl(mCameraFd, VIDIOC_G_CTRL, &ctrl);
    ctrl.value = ctrl.value &0xff;
    return ctrl.value;
}

int V4L2CameraDevice::getExpValue() //get gain (trait specially, need the last 8 bits)
{
    //F_LOG;
    int ret = -1;
    struct v4l2_control ctrl;
    struct v4l2_queryctrl qc_ctrl;

    if (mCameraFd == NULL)
    {
        return 0xFF000000;
    }

    ctrl.id = V4L2_CID_EXPOSURE;
    qc_ctrl.id = V4L2_CID_EXPOSURE;

    if (-1 == ioctl (mCameraFd, VIDIOC_QUERYCTRL, &qc_ctrl))
    {
       return 0;
    }

    ret = ioctl(mCameraFd, VIDIOC_G_CTRL, &ctrl);
    return ctrl.value;
}

int V4L2CameraDevice::setHDRMode(void *hdr_setting)
{
    int ret = -1;
    struct isp_hdr_ctrl hdr_ctrl;

    hdr_ctrl.flag = HDR_CTRL_SET;
    hdr_ctrl.count = 0;
    hdr_ctrl.hdr_t = *(struct isp_hdr_setting_t*)hdr_setting;
    if (mCameraFd == NULL)
    {
        return 0xFF000000;
    }

    ret = ioctl(mCameraFd, VIDIOC_HDR_CTRL, &hdr_ctrl);
    return ret;
}

int V4L2CameraDevice::getAeStat(struct isp_stat_buf *AeBuf)
{
    int ret = -1;
    if (mCameraFd == NULL)
    {
        return 0xFF000000;
    }
    //AeBuf->buf = malloc(0xc00);
    //memset(AeBuf->buf,0,0xc00);
    //LOGD("AeBuf->buf == %x\n",AeBuf->buf);
    if(AeBuf->buf == NULL){
        return -1;
    }
    ret = ioctl(mCameraFd, VIDIOC_ISP_AE_STAT_REQ, AeBuf);
    return ret;
}
int V4L2CameraDevice::getGammaStat(struct isp_stat_buf *GammaBuf)
{
    int ret = -1;
    #if 1
    if (mCameraFd == NULL)
    {
        return 0xFF000000;
    }
    GammaBuf->buf = malloc(0x200);
    memset(GammaBuf->buf,0,0x200);
    LOGD("GammaBuf->buf == %x\n",GammaBuf->buf);
    if(GammaBuf->buf == NULL){
        return -1;
    }
    //ret = ioctl(mCameraFd, VIDIOC_ISP_GAMMA_REQ, GammaBuf);
    #endif
    return ret;
}

int V4L2CameraDevice::getHistStat(struct isp_stat_buf *HistBuf)
{
    int ret = -1;
    if (mCameraFd == NULL)
    {
        return 0xFF000000;
    }
    //HistBuf->buf = malloc(0x200);
    //memset(HistBuf->buf,0,0x200);
    if(HistBuf->buf == NULL){
        return -1;
    }
    ret = ioctl(mCameraFd, VIDIOC_ISP_HIST_STAT_REQ, HistBuf);
    return ret;
}

int V4L2CameraDevice::setGainValue(int Gain)
{
    F_LOG;
    int ret = -1;
    struct v4l2_control ctrl;

    ctrl.id = V4L2_CID_GAIN;
    ctrl.value = Gain;
    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
        LOGE("setGain failed, %s", strerror(errno));
    else
        LOGV("setGain ok");
    return ret;
}

int V4L2CameraDevice::setExpValue(int Exp)
{
    F_LOG;
    int ret = -1;
    struct v4l2_control ctrl;
    ctrl.id = V4L2_CID_EXPOSURE;
    ctrl.value = Exp;

    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
        LOGE("Set V4L2_CID_EXPOSURE failed, %s", strerror(errno));
    else
        LOGV("Set V4L2_CID_EXPOSURE ok");

    return ret;
}

int V4L2CameraDevice::set3ALock(int lock)
{
    F_LOG;
    int ret = -1;
    struct v4l2_control ctrl;

    ctrl.id = V4L2_CID_3A_LOCK;
    ctrl.value = lock;
    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    if (ret >= 0)
        LOGV("set3ALock ok");

    return ret;
}

int V4L2CameraDevice::v4l2setCaptureParams()
{
    F_LOG;
    int ret = -1;

    struct v4l2_streamparm params;
    params.parm.capture.timeperframe.numerator = 1;
    params.parm.capture.timeperframe.denominator = mFrameRate;
    params.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (mCameraType == CAMERA_TYPE_CSI) {
        params.parm.capture.reserved[0] = 0;
        params.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    }

    if (mTakePictureState == TAKE_PICTURE_NORMAL)
    {
#if (defined __PLATFORM_A50__) || (defined __PLATFORM_T5__)
        params.parm.capture.capturemode = V4L2_MODE_VIDEO;
#else
        params.parm.capture.capturemode = V4L2_MODE_IMAGE;
#endif
    }
    else
    {
        if(mVideoHint == true)
        {
        params.parm.capture.capturemode = V4L2_MODE_VIDEO;
        }
        else
        {
            params.parm.capture.capturemode = V4L2_MODE_PREVIEW;
        }
    }

    LOGV("VIDIOC_S_PARM mFrameRate: %d, capture mode: %d", mFrameRate, params.parm.capture.capturemode);

    ret = ioctl(mCameraFd, VIDIOC_S_PARM, &params);
    if (ret < 0)
        LOGE("v4l2setCaptureParams failed, %s", strerror(errno));
    else
        LOGV("v4l2setCaptureParams ok");

    return ret;
}

int V4L2CameraDevice::enumSize(char * pSize, int len)
{
    F_LOG;
    struct v4l2_frmsizeenum size_enum;
    size_enum.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (mCameraType == CAMERA_TYPE_CSI) {
        size_enum.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    }
    size_enum.pixel_format = mCaptureFormat;

    if (pSize == NULL)
    {
        LOGE("error input params");
        return -1;
    }

    char str[16];
    memset(str, 0, 16);
    memset(pSize, 0, len);

    for(int i = 0; i < 20; i++)
    {
        size_enum.index = i;
        if (-1 == ioctl (mCameraFd, VIDIOC_ENUM_FRAMESIZES, &size_enum))
        {
            break;
        }
        // LOGV("format index = %d, size_enum: %dx%d", i, size_enum.discrete.width, size_enum.discrete.height);
        sprintf(str, "%dx%d", size_enum.discrete.width, size_enum.discrete.height);
        if (i != 0)
        {
            strcat(pSize, ",");
        }
        strcat(pSize, str);
    }

    return OK;
}

int V4L2CameraDevice::getFullSize(int * full_w, int * full_h)
{
    F_LOG;
    struct v4l2_frmsizeenum size_enum;
    size_enum.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (mCameraType == CAMERA_TYPE_CSI) {
        size_enum.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    }
    size_enum.pixel_format = mCaptureFormat;
    size_enum.index = 0;
    if (-1 == ioctl (mCameraFd, VIDIOC_ENUM_FRAMESIZES, &size_enum))
    {
        LOGE("getFullSize failed");
        return -1;
    }

    *full_w = size_enum.discrete.width;
    *full_h = size_enum.discrete.height;

    LOGV("getFullSize: %dx%d", *full_w, *full_h);

    return OK;
}

int V4L2CameraDevice::getSuitableThumbScale(int full_w, int full_h)
{
    F_LOG;
    int scale = 1;
    if(mIsThumbUsedForVideo == true)
    {
        scale = 2;
    }
    //TODO: Get the screen size to calculate the scaler factor
    if (full_w*full_h > 10*1024*1024)        //maybe 12m,13m,16m
        return 2;
    else if(full_w*full_h > 4.5*1024*1024)    //maybe 5m,8m
        return 2;
    else return scale;                        //others
#if 0
    if ((full_w == 4608)
        && (full_h == 3456))
    {
        return 4;    // 1000x750
    }
    if ((full_w == 3840)
        && (full_h == 2160))
    {
        return 4;    // 1000x750
    }
    if ((full_w == 4000)
        && (full_h == 3000))
    {
        return 4;    // 1000x750
    }
    else if ((full_w == 3264)
        && (full_h == 2448))
    {
        return 2;    // 1632x1224
    }
    else if ((full_w == 2592)
        && (full_h == 1936))
    {
        return 2;    // 1296x968
    }
    else if ((full_w == 1280)
        && (full_h == 960))
    {
        return 1 * scale;    // 1280x960
    }
    else if ((full_w == 1920)
        && (full_h == 1080))
    {
        return 2;    // 960x540
    }
    else if ((full_w == 1280)
        && (full_h == 720))
    {

        return 1 * scale;    // 1280x720
    }
    else if ((full_w == 640)
        && (full_h == 480))
    {
        return 1;    // 640x480
    }

    LOGW("getSuitableThumbScale unknown size: %dx%d", full_w, full_h);
    return 1;        // failed
#endif
}
void V4L2CameraDevice::getThumbSize(int* sub_w, int* sub_h)
{
    F_LOG;
    *sub_w= mThumbWidth;
    *sub_h= mThumbHeight;
}

int V4L2CameraDevice::getSensorType()
{
    F_LOG;
    int ret = -1;
    struct v4l2_control ctrl;
    struct v4l2_queryctrl qc_ctrl;

    if (mCameraFd == NULL)
    {
        return 0xFF000000;
    }

    ctrl.id = V4L2_CID_SENSOR_TYPE;
    qc_ctrl.id = V4L2_CID_SENSOR_TYPE;

    if (-1 == ioctl (mCameraFd, VIDIOC_QUERYCTRL, &qc_ctrl))
    {
        LOGE("query sensor type ctrl failed");
        return -1;
    }
    ret = ioctl(mCameraFd, VIDIOC_G_CTRL, &ctrl);
    return ctrl.value;
}
int V4L2CameraDevice::getExifInfo(struct isp_exif_attribute *exif_attri)
{
    int ret = -1;

    if (mCameraFd == NULL)
    {
        return 0xFF000000;
    }
    ret = ioctl(mCameraFd, VIDIOC_ISP_EXIF_REQ, exif_attri);

    if(exif_attri->focal_length < -1)
    {
        exif_attri->focal_length = 100;
    }

    return ret;
}

int V4L2CameraDevice::getCameraType()
{
    return mCameraType;
}

}; /* namespace android */
