
#include "CameraDebug.h"
#if DBG_V4L2_CAMERA
#define LOG_NDEBUG 0
#endif
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

#include "math.h"
//#include "memory/memoryAdapter.h"
//#include "memory/sc_interface.h"
#ifdef USE_SUNXI_CAMERA_H
#include <sunxi_camera.h>
#else
//#include <videodev2_34.h>
#endif
#ifdef USE_CSI_VIN_DRIVER
#include <sunxi_camera_v2.h>
#endif
#define CHECK_NO_ERROR(a)                        \
    if (a != NO_ERROR) {                        \
        if (mCameraFd != NULL) {                \
            ALOGE("Camaera[%d]CHECK_NO_ERROR",mCameraId);     \
            close(mCameraFd);                    \
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
            ALOGV("SCENE_NOTIFY_CMD_GET_AE_STATE");
            *ret = dev->getAeStat((struct isp_stat_buf *)data);
            break;

        case SCENE_NOTIFY_CMD_GET_HIST_STATE:
            ALOGV("SCENE_NOTIFY_CMD_GET_HIST_STATE");
            *ret = dev->getHistStat((struct isp_stat_buf * )data);
            break;

        case SCENE_NOTIFY_CMD_SET_3A_LOCK:
            ALOGV("SCENE_NOTIFY_CMD_SET_3A_LOCK: %ld",(long)data);
            *ret = dev->set3ALock((long)data);
            break;

        case SCENE_NOTIFY_CMD_SET_HDR_SETTING:
            ALOGV("SENE_NOTIFY_CMD_SET_HDR_SETTING");
            *ret = dev->setHDRMode(data);
            break;

        case SCENE_NOTIFY_CMD_GET_HDR_FRAME_COUNT:
            //*ret = dev->getHDRFrameCnt((int*)data);
            int cnt;
            cnt = dev->getHDRFrameCnt();
            *(int *)data = cnt;
            *ret = 0;
            ALOGD("SCENE_NOTIFY_CMD_GET_HDR_FRAME_COUNT: %ld",cnt);
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

    // ALOGD("crop: [%d, %d, %d, %d]", rect->left, rect->top, rect->right, rect->bottom);
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
        ALOGD("ioctl, G2D_CMD_BITBLT failed");
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
        ALOGD("NV21ToYV12 ioctl, G2D_CMD_BITBLT failed");
        return;
    }
}
#endif

DBG_TIME_AVG_BEGIN(TAG_CONTINUOUS_PICTURE);

void  V4L2CameraDevice::showformat(int format,char *str)
{
    switch(format){
    case V4L2_PIX_FMT_YUYV:
        ALOGD("Camera[%d] The %s foramt is V4L2_PIX_FMT_YUYV",mCameraId,str);
        break;
    case V4L2_PIX_FMT_MJPEG:
        ALOGD("Camera[%d] The %s foramt is V4L2_PIX_FMT_MJPEG",mCameraId,str);
        break;
    case V4L2_PIX_FMT_YVU420:
        ALOGD("Camera[%d] The %s foramt is V4L2_PIX_FMT_YVU420",mCameraId,str);
        break;
    case V4L2_PIX_FMT_NV12:
        ALOGD("Camera[%d] The %s foramt is V4L2_PIX_FMT_NV12",mCameraId,str);
        break;
    case V4L2_PIX_FMT_NV21:
        ALOGD("Camera[%d] The %s foramt is V4L2_PIX_FMT_NV21",mCameraId,str);
        break;
    case V4L2_PIX_FMT_H264:
        ALOGD("Camera[%d] The %s foramt is V4L2_PIX_FMT_H264",mCameraId,str);
        break;
    default:
        ALOGD("Camera[%d] The %s format can't be showed",mCameraId,str);
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
      mCameraId(0),
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
      mVideoFormat(V4L2_PIX_FMT_NV21),
      mFrameRate(30),
      mStartSmartTimeout(false)
#ifdef USE_MP_CONVERT
      ,mG2DHandle(0)
#endif
      ,mCurrentV4l2buf(NULL)
      ,mCanBeDisconnected(false)
      ,mContinuousPictureStarted(false)
      ,mContinuousPictureCnt(0)
      ,mContinuousPictureMax(0)
      ,mContinuousPictureStartTime(0)
      ,mContinuousPictureLast(0)
      ,mContinuousPictureAfter(0)
      ,mSmartPictureDone(true)
      ,mFaceDectectLast(0)
      ,mFaceDectectAfter(0)
      ,mPreviewLast(0)
      ,mPreviewAfter(0)
      ,mVideoHint(false)
      ,mCurAvailBufferCnt(0)
      ,mStatisicsIndex(0)
      ,mNeedHalfFrameRate(false)
      ,mIsThumbUsedForVideo(false)
      ,mVideoWidth(640)
      ,mVideoHeight(480)
      ,mSceneMode(NULL)
      ,mFlashMode(V4L2_FLASH_LED_MODE_NONE)
      ,mZoomRatio(100)
      ,mExposureBias(0)
      ,mMemOpsS(NULL)
      ,releaseIndex(-1)
#ifdef CAMERA_MANAGER_ENABLE
      ,mCameraManager(NULL)
#endif
      ,nPlanes(0)
#ifdef USE_DEINTERLACE_HW
      ,mDiProcess(NULL)
      ,mPreFrameIndex(-1)
      ,mPrePreFrameIndex(-1)
      ,mEnableYUV422(-1)
#endif
      ,mAWIspApi(NULL)
      ,mIspId(-1)
#ifdef CAMERA_DECODE
      ,mCameraDecoder(NULL)
#endif
      ,mPicCnt(0)
      ,mOrgTime(0)
      ,mDebugLogCnt(0)
{
    ALOGV("V4L2CameraDevice construct");

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

    pthread_mutex_init(&mCaptureMutex, NULL);
    pthread_cond_init(&mCaptureCond, NULL);
    mCaptureThreadState = CAPTURE_STATE_PAUSED;

    pthread_mutex_init(&mPreviewMutex, NULL);
    pthread_cond_init(&mPreviewCond, NULL);

    pthread_mutex_init(&mPreviewSyncMutex, NULL);
    pthread_cond_init(&mPreviewSyncCond, NULL);
    mPreviewThreadState = PREVIEW_STATE_STARTED;

    pthread_mutex_init(&mPictureMutex, NULL);
    pthread_cond_init(&mPictureCond, NULL);

    pthread_mutex_init(&mConnectMutex, NULL);
    pthread_cond_init(&mConnectCond, NULL);

    pthread_mutex_init(&mContinuousPictureMutex, NULL);
    pthread_cond_init(&mContinuousPictureCond, NULL);

    pthread_mutex_init(&mSmartPictureMutex, NULL);
    pthread_cond_init(&mSmartPictureCond, NULL);

    // init capture thread
    mCaptureThread = new DoCaptureThread(this);
    mCaptureThread->startThread();

    // init preview thread
    mPreviewThread = new DoPreviewThread(this);
    mPreviewThread->startThread();

    // init picture thread
    mPictureThread = new DoPictureThread(this);
    mPictureThread->startThread();

    // init continuous picture thread
    //disnable
   // mContinuousPictureThread = new DoContinuousPictureThread(this);
   // mContinuousPictureThread->startThread();
   // mSmartPictureThread = new DoSmartPictureThread(this);
   // mSmartPictureThread->startThread();
}

V4L2CameraDevice::~V4L2CameraDevice()
{
    ALOGD("Camera[%d]V4L2CameraDevice disconstruct",mCameraId);

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
    Mutex::Autolock locker(&mObjectLock);

    if (isConnected())
    {
        ALOGW("%s: camera device is already connected.", __FUNCTION__);
        return NO_ERROR;
    }
    memcpy((void*)&mHalCameraInfo, (void*)halInfo, sizeof(HALCameraInfo));
    ALOGD("Camera[%d]F:%s,L:%d" ,mHalCameraInfo.device_id,__FUNCTION__, __LINE__);
    // open v4l2 camera device
    int ret = openCameraDev(halInfo);
    if (ret != OK) {
        ALOGE("Camera[%d]open Camera failed! F:%s, L:%d",mCameraId, __FUNCTION__, __LINE__);
        return ret;
    }

    if (mCameraType == CAMERA_TYPE_CSI) {
        mSensor_Type = V4L2_SENSOR_TYPE_YUV;
        switch((v4l2_sensor_type)getSensorType()) {
            case V4L2_SENSOR_TYPE_YUV:
                ALOGD("Camera[%d]the sensor is YUV sensor",mCameraId);
                mSensor_Type = V4L2_SENSOR_TYPE_YUV;
                break;
            case V4L2_SENSOR_TYPE_RAW:
                ALOGD("Camera[%d]the sensor is RAW sensor",mCameraId);
                mSensor_Type = V4L2_SENSOR_TYPE_RAW;
                mAWIspApi = new AWIspApi();
                break;
            default:
                ALOGE("Camera[%d]get the sensor type failed",mCameraId);
                goto END_ERROR;
        }
        //struct isp_exif_attribute exif_attri;
        //getExifInfo(&exif_attri);
        //mCameraHardware->setExifInfo(exif_attri);
    }

#ifdef USE_MP_CONVERT
    if (mCameraType == CAMERA_TYPE_UVC)
    {
        // open MP driver
        mG2DHandle = open("/dev/g2d", O_RDWR, 0);
        if (mG2DHandle < 0)
        {
            ALOGE("Camera[%d]open /dev/g2d failed",mCameraId);
            goto END_ERROR;
        }
        ALOGD("Camera[%d]open /dev/g2d OK",mCameraId);
    }
#endif

    ret = mMemOpsS->open_cam();
    if (ret < 0)
    {
        ALOGE("Camera[%d]ion_alloc_open failed",mCameraId);
        goto END_ERROR;
    }
    ALOGD("Camera[%d]ion_alloc_open ok",mCameraId);

#ifdef USE_DEINTERLACE_HW
    if (mCameraType == CAMERA_TYPE_TVIN_NTSC || mCameraType == CAMERA_TYPE_TVIN_PAL
        || mCameraType == CAMERA_TYPE_TVIN_YPBPR) {
        mDiProcess = new DiProcess();
        if (mDiProcess == NULL) {
            ALOGD("new Diprocess failed");
            goto END_ERROR;
        }
        ALOGD("new Diprocess OK");
        ret = mDiProcess->init();
        if (ret != NO_ERROR) {
            ALOGD("Diprocess init failed");
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

    if (mAWIspApi != NULL) {
        delete mAWIspApi;
        mAWIspApi = NULL;
    }

    return UNKNOWN_ERROR;
}

status_t V4L2CameraDevice::disconnectDevice()
{
    ALOGD("Camera[%d] %s enter",mCameraId,__FUNCTION__);
    Mutex::Autolock locker(&mObjectLock);

    if (!isConnected())
    {
        ALOGW("Camera[%d]%s: camera device is already disconnected.", mCameraId,__FUNCTION__);
        return NO_ERROR;
    }

    if (isStarted())
    {
        ALOGE("Camera[%d]%s: Cannot disconnect from the started device.",mCameraId, __FUNCTION__);
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
    ALOGD("Camera[%d]%s, wxh: %dx%d, fmt: %d",mCameraId, __FUNCTION__, width, height, pix_fmt);

    Mutex::Autolock locker(&mObjectLock);

    if (!isConnected())
    {
        ALOGE("Camera[%d] %s: camera device is not connected.",mCameraId, __FUNCTION__);
        return EINVAL;
    }

    if (isStarted())
    {
        ALOGD("Camera[%d] %s: camera device is already started.", mCameraId,__FUNCTION__);
        return EINVAL;
    }

    // VE encoder need this format
    mVideoFormat = pix_fmt;
#ifdef USE_DEINTERLACE_HW
    if ((mCameraType == CAMERA_TYPE_TVIN_NTSC || mCameraType == CAMERA_TYPE_TVIN_PAL
        || mCameraType == CAMERA_TYPE_TVIN_YPBPR) && (width == 720) && (mEnableYUV422 > 0)) {
        if ((mCameraManager == NULL) || ((mCameraManager != NULL) && !mCameraManager->isOviewEnable())) {
            mVideoFormat = V4L2_PIX_FMT_NV61;
            mCaptureFormat = V4L2_PIX_FMT_NV61;
            ALOGD("F:%s, L:%d, CaptureFormat set to V4L2_PIX_FMT_NV61", __FUNCTION__, __LINE__);
        }
    } else if ((mCameraType == CAMERA_TYPE_TVIN_NTSC || mCameraType == CAMERA_TYPE_TVIN_PAL
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

    // set capture mode and fps
    // CHECK_NO_ERROR(v4l2setCaptureParams());    // do not check this error
    v4l2setCaptureParams();

    // set v4l2 device parameters, it maybe change the value of mFrameWidth and mFrameHeight.
    CHECK_NO_ERROR(v4l2SetVideoParams(width, height, mVideoFormat));

    // v4l2 request buffers
    int buf_cnt = (mTakePictureState == TAKE_PICTURE_NORMAL) ? 1 : NB_BUFFER;
    CHECK_NO_ERROR(v4l2ReqBufs(&buf_cnt));
    mBufferCnt = buf_cnt;
    mCurAvailBufferCnt = mBufferCnt;

    // v4l2 query buffers
    CHECK_NO_ERROR(v4l2QueryBuf());
#ifdef CAMERA_MANAGER_ENABLE
    if((mCameraManager != NULL) && mCameraManager->isOviewEnable())
    {
        mCameraManager->setFrameSize(mHalCameraInfo.device_id,mFrameWidth,mFrameHeight);
        ALOGD("Camera[%d]CameraManager F:%s,L:%d",mCameraId,__FUNCTION__,__LINE__);
    }
#endif
    // stream on the v4l2 device
    CHECK_NO_ERROR(v4l2StartStreaming());
    if ((mCameraType == CAMERA_TYPE_CSI) && (mSensor_Type == V4L2_SENSOR_TYPE_RAW)) {
        mIspId = -1;
        mIspId = mAWIspApi->awIspGetIspId(mHalCameraInfo.device_id);
        if (mIspId >= 0) {
            mAWIspApi->awIspStart(mIspId);
        }
    }

    mContinuousPictureAfter = 1000000 / 10;
    mFaceDectectAfter = 1000000 / 15;
    mPreviewAfter = 1000000 / 24;
    set3ALock(0);

#ifdef CAMERA_DECODE
    if (mCaptureFormat == V4L2_PIX_FMT_MJPEG) {
        mCameraDecoder = new CameraDecoder;
        mCameraDecoder->DecorderOpen(PIXEL_FORMAT_NV21,
            VIDEO_CODEC_FORMAT_MJPEG,
            mFrameRate,
            mFrameWidth,
            mFrameHeight);
    } else if (mCaptureFormat == V4L2_PIX_FMT_H264) {
        mCameraDecoder = new CameraDecoder;
        mCameraDecoder->DecorderOpen(PIXEL_FORMAT_NV21,
            VIDEO_CODEC_FORMAT_H264,
            mFrameRate,
            mFrameWidth,
            mFrameHeight);
    }
#endif
    mCameraDeviceState = STATE_STARTED;
    ALOGD("Camera[%d] %s end",mCameraId,__FUNCTION__);
    return NO_ERROR;
}

status_t V4L2CameraDevice::stopDevice()
{
    ALOGD("Camera[%d] %s enter",mCameraId,__FUNCTION__);

    pthread_mutex_lock(&mConnectMutex);
    if (!mCanBeDisconnected)
    {
        ALOGW("Camera[%d]wait until capture thread pause or exit",mCameraId);
        pthread_cond_wait(&mConnectCond, &mConnectMutex);
    }
    pthread_mutex_unlock(&mConnectMutex);

    Mutex::Autolock locker(&mObjectLock);

    if (!isStarted())
    {
        ALOGW("Camera[%d]%s: camera device is not started.",mCameraId, __FUNCTION__);
        return NO_ERROR;
    }

    // v4l2 device stop stream
    v4l2StopStreaming();

    if ((mCameraType == CAMERA_TYPE_CSI) && (mSensor_Type == V4L2_SENSOR_TYPE_RAW)
        && (mIspId >= 0)) {
        mAWIspApi->awIspStop(mIspId);
    }

    // v4l2 device unmap buffers
    v4l2UnmapBuf();

    for(int i = 0; i < NB_BUFFER; i++)
    {
        memset(&mV4l2buf[i], 0, sizeof(V4L2BUF_t));
    }

    mCameraDeviceState = STATE_CONNECTED;

    mLastZoom = -1;

    mCurrentV4l2buf = NULL;
    int buf_cnt = 0;
    CHECK_NO_ERROR(v4l2ReqBufs(&buf_cnt));

#ifdef AW_ADAS_MODULE
    mCurV4l2Buf = NULL;
#endif
#ifdef CAMERA_DECODE
    if ((mCaptureFormat == V4L2_PIX_FMT_MJPEG) || (mCaptureFormat == V4L2_PIX_FMT_H264)) {
        mCameraDecoder->DecorderClose();
        delete mCameraDecoder;
        mCameraDecoder = NULL;
    }
#endif

#ifdef USE_DEINTERLACE_HW
    mPreFrameIndex = -1;
    mPrePreFrameIndex = -1;
    mEnableYUV422 = -1;
#endif
    ALOGD("Camera[%d] %s end",mCameraId,__FUNCTION__);
    return NO_ERROR;
}

status_t V4L2CameraDevice::startDeliveringFrames()
{
    ALOGD("Camera[%d] %s enter",mCameraId,__FUNCTION__);

    pthread_mutex_lock(&mCaptureMutex);

    if (mCaptureThreadState == CAPTURE_STATE_NULL)
    {
        ALOGE("Camera[%d]error state of capture thread, %s",mCameraId, __FUNCTION__);
        pthread_mutex_unlock(&mCaptureMutex);
        return EINVAL;
    }

    if (mCaptureThreadState == CAPTURE_STATE_STARTED)
    {
        ALOGW("Camera[%d]capture thread has already started",mCameraId);
        pthread_mutex_unlock(&mCaptureMutex);
        return NO_ERROR;
    }

    // singal to start capture thread
    mCaptureThreadState = CAPTURE_STATE_STARTED;
    pthread_cond_signal(&mCaptureCond);
    pthread_mutex_unlock(&mCaptureMutex);
    pthread_mutex_lock(&mPreviewSyncMutex);
    mPreviewThreadState = PREVIEW_STATE_STARTED;
    ALOGD("Camera[%d]set mPreviewThreadState to %d\n",mCameraId,mPreviewThreadState);
    pthread_cond_signal(&mPreviewSyncCond);
    pthread_mutex_unlock(&mPreviewSyncMutex);

    return NO_ERROR;
}

status_t V4L2CameraDevice::stopDeliveringFrames()
{
    ALOGD("Camera[%d] %s enter",mCameraId,__FUNCTION__);

    pthread_mutex_lock(&mCaptureMutex);
    if (mCaptureThreadState == CAPTURE_STATE_NULL)
    {
        ALOGE("Camera[%d]error state of capture thread, %s", mCameraId,__FUNCTION__);
        pthread_mutex_unlock(&mCaptureMutex);
        return EINVAL;
    }

    if (mCaptureThreadState == CAPTURE_STATE_PAUSED)
    {
        ALOGW("Camera[%d]capture thread has already paused",mCameraId);
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
    int availBufferCntBak = 0;
    FD_ZERO(&fds);
    FD_SET(mCameraFd, &fds);

    /* Timeout */
    tv.tv_sec  = 2;
    tv.tv_usec = 0;
    availBufferCntBak = mCurAvailBufferCnt;
    r = select(mCameraFd + 1, &fds, NULL, NULL, &tv);
    if (r == -1) {
        ALOGE("Camera[%d] bufferCntBak=%d,curBufferCnt=%d,select err=%s", mCameraId,mCurAvailBufferCnt,availBufferCntBak,strerror(errno));
        return -1;
    } else if (r == 0) {
        ALOGE("Camera[%d] bufferCntBak=%d,curBufferCnt=%d,select timeout", mCameraId,mCurAvailBufferCnt,availBufferCntBak);
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
                ALOGD("Camera[%d]CAMERA_TYPE_UVC framerate = %dfps@%dx%d",mCameraId,
                    framerate, mFrameWidth, mFrameHeight);
            } else if (mCameraType == CAMERA_TYPE_CSI) {
                ALOGD("Camera[%d]CAMERA_TYPE_CSI framerate = %dfps@%dx%d",mCameraId,
                    framerate, mFrameWidth, mFrameHeight);
            } else {
                ALOGD("Camera[%d]other type framerate = %dfps@%dx%d",mCameraId,
                    framerate, mFrameWidth, mFrameHeight);
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
        ALOGD("Camera[%d] capture thread paused",mCameraId);
        pthread_cond_wait(&mCaptureCond, &mCaptureMutex);
    }

    // thread exit
    if (mCaptureThreadState == CAPTURE_STATE_EXIT)
    {
        singalDisconnect();
        ALOGD("Camaera[%d] capture thread exit",mCameraId);
        pthread_mutex_unlock(&mCaptureMutex);
        return false;
    }
    pthread_mutex_unlock(&mCaptureMutex);

    int ret = v4l2WaitCameraReady();

    //ALOGD("FUNC:%s, CamID=%d, mCurAvailBufferCnt =%d", __FUNCTION__, mHalCameraInfo.device_id,mCurAvailBufferCnt);
    pthread_mutex_lock(&mCaptureMutex);
    // stop capture or thread exit
    if (mCaptureThreadState == CAPTURE_STATE_PAUSED
        || mCaptureThreadState == CAPTURE_STATE_EXIT)
    {
        singalDisconnect();
        ALOGW("Camera[%d]should stop capture now",mCameraId);
        pthread_mutex_unlock(&mCaptureMutex);
        return __LINE__;
    }

    if (ret != 0)
    {
        ALOGW("Camera[%d] wait v4l2 buffer time out",mCameraId);
        ALOGW("Camera[%d] mCurAvailBufferCnt has %d",mCameraId, mCurAvailBufferCnt);
        pthread_mutex_unlock(&mCaptureMutex);

        recoveryPreviewFrame();
        return __LINE__;
    }

    // get one video frame
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(v4l2_buffer));
    ret = getPreviewFrame(&buf);
    if (ret != OK)
    {
        pthread_mutex_unlock(&mCaptureMutex);

        usleep(10000);
        return ret;
    }
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
        ALOGV("CROP: [%d, %d, %d, %d]", mRectCrop.left, mRectCrop.top, mRectCrop.right, mRectCrop.bottom);
        ALOGV("thumb CROP: [%d, %d, %d, %d]", mThumbRectCrop.left, mThumbRectCrop.top, mThumbRectCrop.right, mThumbRectCrop.bottom);
    }
#ifdef CAMERA_DECODE
    if ((mCaptureFormat == V4L2_PIX_FMT_MJPEG) || (mCaptureFormat == V4L2_PIX_FMT_H264)) {
        mCameraDecoder->DecorderEnter(mMapMem.mem[buf.index],
            (void*)mVideoBuffer.buf_vir_addr[buf.index],
            buf.bytesused,
            (int64_t)mCurFrameTimestamp/1000);
    }
#endif

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

    // V4L2BUF_t for preview and HW encoder
    V4L2BUF_t v4l2_buf;
    if (mVideoFormat != V4L2_PIX_FMT_YUYV
        && mCaptureFormat == V4L2_PIX_FMT_YUYV)
    {
        v4l2_buf.addrPhyY        = mVideoBuffer.buf_phy_addr[buf.index];
        v4l2_buf.addrVirY        = mVideoBuffer.buf_vir_addr[buf.index];
        v4l2_buf.nShareBufFd = mVideoBuffer.nShareBufFd[buf.index];
    }
#ifdef CAMERA_DECODE
    else if(mVideoFormat != V4L2_PIX_FMT_YUYV
        &&(( mCaptureFormat == V4L2_PIX_FMT_MJPEG)|| (mCaptureFormat == V4L2_PIX_FMT_H264)))
    {
        v4l2_buf.addrPhyY       = mVideoBuffer.buf_phy_addr[buf.index];//- 0x20000000;
        v4l2_buf.addrVirY       = mVideoBuffer.buf_vir_addr[buf.index];
        v4l2_buf.width          = mFrameWidth;//mVideoInfo.nWidth;
        v4l2_buf.height         = mFrameHeight;//mVideoInfo.nHeight;
        v4l2_buf.nShareBufFd = mVideoBuffer.nShareBufFd[buf.index];
    }
#endif
    else
    {
        v4l2_buf.addrPhyY        = buf.m.offset - BUFFER_PHY_OFFSET;
        if (mCameraType == CAMERA_TYPE_CSI) {
            v4l2_buf.addrPhyY        = buf.m.planes[0].m.mem_offset- BUFFER_PHY_OFFSET;
        }
        v4l2_buf.addrVirY        = (unsigned long)mMapMem.mem[buf.index];
        v4l2_buf.nShareBufFd = mMapMem.nShareBufFd[buf.index];
        //ALOGD("Camera[%d]v4l2_buf.nShareBufFd=0x%x,index:%d",mCameraId,v4l2_buf.nShareBufFd,buf.index);
    }
    v4l2_buf.index = buf.index;
    v4l2_buf.timeStamp            = mCurFrameTimestamp;
    v4l2_buf.width                = mFrameWidth;
    v4l2_buf.height                = mFrameHeight;
    v4l2_buf.crop_rect.left        = mRectCrop.left;
    v4l2_buf.crop_rect.top        = mRectCrop.top;
    v4l2_buf.crop_rect.width    = mRectCrop.right - mRectCrop.left + 1;
    v4l2_buf.crop_rect.height    = mRectCrop.bottom - mRectCrop.top + 1;
    v4l2_buf.format                = mVideoFormat;
    v4l2_buf.refCnt = 1;

    mMemOpsS->flush_cache_cam((void*)v4l2_buf.addrVirY, ALIGN_16B(mFrameWidth) * mFrameHeight * 3 >> 1);

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
    ALOGD("Debug to save yuv data! Size:%dx%d",mFrameWidth,mFrameHeight);
    unsigned int length = 0;
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

    //memcpy(&mV4l2buf[v4l2_buf.index], &v4l2_buf, sizeof(V4L2BUF_t));
#ifdef CAMERA_MANAGER_ENABLE
    if((mCameraManager != NULL) && mCameraManager->isOviewEnable())
    {
        mCameraManager->queueCameraBuf(mHalCameraInfo.device_id,&v4l2_buf);
        if (mTakePictureState == TAKE_PICTURE_FAST
            || mTakePictureState == TAKE_PICTURE_RECORD
            || mTakePictureState == TAKE_PICTURE_NORMAL)
        {
            ALOGD("Camera[%d]F:%s, L:%d, need take pic!",mCameraId, __FUNCTION__, __LINE__);
            if(mHalCameraInfo.device_id == mCameraManager->mStartCameraID) {
                mCameraManager->mTakePicState = true;
                ALOGD("Camera[%d]F:%s, L:%d, set mCameraManager mTakePicState = true!", mCameraId,__FUNCTION__, __LINE__);
            }
        }
        //ALOGV("CameraDebug,F:%s,L:%d,id:%d,fd:%d +++++++++++",__FUNCTION__,__LINE__,mHalCameraInfo.device_id,mCameraFd);
        memcpy(&mV4l2buf[v4l2_buf.index], &v4l2_buf, sizeof(V4L2BUF_t));
        goto DEC_REF;
    }
#endif
    memcpy(&mV4l2buf[v4l2_buf.index], &v4l2_buf, sizeof(V4L2BUF_t));
    if ((!mVideoHint) && (mTakePictureState != TAKE_PICTURE_NORMAL))
    {
        // face detection only use when picture mode
        mCurrentV4l2buf = &mV4l2buf[v4l2_buf.index];
    }

    if (mTakePictureState == TAKE_PICTURE_NORMAL)
    {
        //copy picture buffer
        unsigned long phy_addr = mPicBuffer.addrPhyY;
        unsigned long vir_addr = mPicBuffer.addrVirY;
        int share_fd = mPicBuffer.nShareBufFd;

        int frame_size = GPU_BUFFER_ALIGN(ALIGN_16B(mFrameWidth)*mFrameHeight*3/2);
        if (frame_size > MAX_PICTURE_SIZE)
        {
            ALOGE("Camera[%d]picture buffer size(%d) is smaller than the frame buffer size(%d)",mCameraId, MAX_PICTURE_SIZE, frame_size);
            pthread_mutex_unlock(&mCaptureMutex);
            return false;
        }
        //copy picture buffer
        memcpy((void*)&mPicBuffer, &v4l2_buf, sizeof(V4L2BUF_t));
        mPicBuffer.addrPhyY = phy_addr;
        mPicBuffer.addrVirY = vir_addr;
        mPicBuffer.nShareBufFd = share_fd;

        mMemOpsS->flush_cache_cam((void*)v4l2_buf.addrVirY, frame_size);

        memcpy((void*)mPicBuffer.addrVirY, (void*)v4l2_buf.addrVirY, frame_size);
        mMemOpsS->flush_cache_cam((void*)mPicBuffer.addrVirY, frame_size);


        //get Exif info for driver
        struct isp_exif_attribute exif_attri;
        getExifInfo(&exif_attri);
        mCallbackNotifier->setExifInfo(exif_attri,mZoomRatio,mExposureBias);
        mCameraHardware->setExifInfo(exif_attri);

        // enqueue picture buffer
        ret = OSAL_Queue(&mQueueBufferPicture, &mPicBuffer);
        if (ret != 0)
        {
            ALOGW("Camera[%d]picture queue full",mCameraId);
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
        // add reference count
        mV4l2buf[v4l2_buf.index].refMutex.lock();
        mV4l2buf[v4l2_buf.index].refCnt++;
        mV4l2buf[v4l2_buf.index].refMutex.unlock();

        //mV4l2buf[v4l2_buf.index].nShareBufFd = mMapMem.nShareBufFd[v4l2_buf.index];
        ret = OSAL_Queue(&mQueueBufferPreview, &mV4l2buf[v4l2_buf.index]);
        if (ret != 0)
        {
            ALOGW("Camera[%d]preview queue full",mCameraId);
            goto DEC_REF;
        }

        // signal a new frame for preview
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
                    ALOGW("Camera[%d]picture queue full....",mCameraId); //?? no test ??
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
           //ALOGD("xxxxxxxxxxxxxxxxxxxx buf.reserved: %x", buf.reserved);
            if (mHalCameraInfo.fast_picture_mode)
            {
                //ALOGD("af_sharp: %d, hdr_cnt:%d, flash_ok: %d, capture_ok: %d.", mTakePictureFlag.bits.af_sharp, \
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

                        ALOGV("mTakePictureFlag.bits.capture_ok:%d\n",mTakePictureFlag.bits.capture_ok);
                        if(count++ > 20) {
                            count = 0;
                            setTakePictureCtrl(V4L2_TAKE_PICTURE_NORM);
                        }
                        goto DEC_REF;
                    } else {
                        count = 0;
                    }
                    //when flash mode,the target frame buffer flag is 1
                    if (mFlashMode != V4L2_FLASH_LED_MODE_NONE && \
                        mTakePictureFlag.bits.flash_ok == 0 )
                        goto DEC_REF;

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
                ALOGD("Gain = %d, SharpLevel = %d, SharpMin = %d, NdfTh = %d!", Gain, SharpLevel, SharpMin, NdfTh);
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
#ifdef TAKE_PIC_USE_COPY
            //copy picture buffer
            unsigned long phy_addr = mPicBuffer.addrPhyY;
            unsigned long vir_addr = mPicBuffer.addrVirY;
            int share_fd = mPicBuffer.nShareBufFd;

            int frame_size = GPU_BUFFER_ALIGN(ALIGN_16B(mFrameWidth) * mFrameHeight * 3 / 2);
            if (frame_size > MAX_PICTURE_SIZE) {
                ALOGD("picture buffer size(%d) is smaller than the frame buffer size(%d)", MAX_PICTURE_SIZE, frame_size);
            } else {
                //copy picture buffer
                memcpy((void*)&mPicBuffer, &v4l2_buf, sizeof(V4L2BUF_t));
                mPicBuffer.addrPhyY = phy_addr;
                mPicBuffer.addrVirY = vir_addr;
                mPicBuffer.nShareBufFd = share_fd;

                mMemOpsS->flush_cache_cam((void*)v4l2_buf.addrVirY, frame_size);
                memcpy((void*)mPicBuffer.addrVirY, (void*)v4l2_buf.addrVirY, frame_size);
                mMemOpsS->flush_cache_cam((void*)mPicBuffer.addrVirY, frame_size);

                // enqueue picture buffer
                ret = OSAL_Queue(&mQueueBufferPicture, &mPicBuffer);
                if (ret != 0) {
                    LOGW("picture queue full");
                    pthread_cond_signal(&mPictureCond);
                    goto DEC_REF;
                }

                mIsPicCopy = true;
                mTakePictureState = TAKE_PICTURE_NULL;
                pthread_cond_signal(&mPictureCond);
            }
#else
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
#endif
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
                // ALOGV("continuous picture queue full");
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
    int releasebufferindex = -1;

    V4L2BUF_t * pbuf = (V4L2BUF_t *)OSAL_Dequeue(&mQueueBufferPreview);
    if (pbuf == NULL)
    {
        ALOGV("Camera[%d]preview queue no buffer, sleep...",mCameraId);
        pthread_mutex_lock(&mPreviewMutex);
        pthread_cond_wait(&mPreviewCond, &mPreviewMutex);
        pthread_mutex_unlock(&mPreviewMutex);
        return true;
    }
    //nsecs_t beforePrevew = (int64_t)systemTime();
    pthread_mutex_lock(&mPreviewSyncMutex);
    if(mPreviewThreadState == PREVIEW_STATE_PAUSED){
        ALOGV("Camera[%d]preview thread paused",mCameraId);
        pthread_cond_wait(&mPreviewSyncCond, &mPreviewSyncMutex);
    }

    Mutex::Autolock locker(&mObjectLock);
    if (pbuf->addrVirY == 0)
    {
        ALOGV("Camera[%d]preview buffer have been released...",mCameraId);
        pthread_mutex_unlock(&mPreviewSyncMutex);
        return true;
    }

    if (mCameraType == CAMERA_TYPE_UVC)
    {
        mCallbackNotifier->onNextFrameAvailable((void*)pbuf, mUseHwEncoder, NOT_NEED_RELEASE_INDEX);
    }
    else
    {
        mCallbackNotifier->onNextFrameAvailable((void*)pbuf, mUseHwEncoder, NEED_RELEASE_INDEX);
    }
    releasePreviewFrame(pbuf->index);
    pthread_mutex_unlock(&mPreviewSyncMutex);

    return true;
}
void V4L2CameraDevice::stopPreviewThread()
{
    ALOGD("Camera[%d]stop preview thread!\n",mCameraId);
    pthread_mutex_lock(&mPreviewSyncMutex);
    pthread_cond_signal(&mPreviewSyncCond);
    ALOGD("Camera[%d]stop preview mPreviewSyncCond!\n",mCameraId);
    if(mPreviewThreadState == PREVIEW_STATE_STARTED)
    {
        mPreviewThreadState = PREVIEW_STATE_PAUSED;
        ALOGD("Camera[%d]set mPreviewThreadState to %d\n",mCameraId,mPreviewThreadState);
    }
    pthread_mutex_unlock(&mPreviewSyncMutex);
}

// singal picture
bool V4L2CameraDevice::pictureThread()
{
    V4L2BUF_t * pbuf = (V4L2BUF_t *)OSAL_Dequeue(&mQueueBufferPicture);
    if (pbuf == NULL)
    {
        ALOGV("Camera[%d]picture queue no buffer, sleep...",mCameraId);
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

    if((mCameraManager != NULL) && (mCameraManager->isOviewEnable())
        && (mHalCameraInfo.device_id == mCameraManager->mStartCameraID)) {
        ALOGD("Camera[%d]F:%s, L:%d, mCameraManager take pic ok!",mCameraId, __FUNCTION__, __LINE__);
    } else {
        if (!mIsPicCopy) {
            releasePreviewFrame(pbuf->index);
        }
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

    #if 0

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

    #if 0

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
        ALOGD("Smart picture has already stopped");
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
        ALOGD("SceneModeFactory CreateSceneMode failed");
        mSceneModeFactory.DestorySceneMode(NULL);
        return -1;
    }
    mSceneMode->SetCallBack(SceneNotifyCallback,(void*)this);

    ret = mSceneMode->InitSceneMode(mFrameWidth,mFrameHeight);
    if(ret == -1){
        ALOGD("SceneMode Init faided");
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
        ALOGV("continuousPictureThread queue no buffer, sleep...");
        pthread_mutex_lock(&mContinuousPictureMutex);
        pthread_cond_wait(&mContinuousPictureCond, &mContinuousPictureMutex);
        pthread_mutex_unlock(&mContinuousPictureMutex);
        return true;
    }

    Mutex::Autolock locker(&mObjectLock);
    if (mMapMem.mem[pbuf->index] == NULL
        || pbuf->addrPhyY == 0)
    {
        ALOGV("picture buffer have been released...");
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
        ALOGD("Continuous picture has already stopped");
        return;
    }

    mContinuousPictureStarted = false;

    nsecs_t time = (systemTime(SYSTEM_TIME_MONOTONIC) - mContinuousPictureStartTime)/1000000;
    ALOGD("Continuous picture cnt: %d, use time %lld(ms)", mContinuousPictureCnt, time);
    if (time != 0)
    {
        ALOGD("Continuous picture %f(fps)", (float)mContinuousPictureCnt/(float)time * 1000);
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
        ALOGD("getCurrentFrame: error in null pointer");
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
    //ALOGV("getCurrentFaceFrame: %dx%d", mCurrentV4l2buf->width, mCurrentV4l2buf->height);

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
    //ALOGV("getCurrentFaceFrame: %dx%d", *width, *height);
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
        ALOGV("i = %d\n", i);
        ALOGV("framesize width = %d, height = %d\n",frmsize.discrete.width, frmsize.discrete.height);
        i++;
        frmsize.index++;
    }
    memset(&format, 0, sizeof(struct v4l2_format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    i = 0;
    while (ioctl(fd, VIDIOC_G_FMT, &format) &&(i++ < 20))
    {
        ALOGV("get tvin signal failed.\n");
        return -1;
    }
    memcpy(&format_set,&format,sizeof(struct v4l2_format));
    ALOGV("---width= [%d], height =[%d],system=%d,i=%d\n",format.fmt.pix.width, format.fmt.pix.height,format.fmt.raw_data[RAW_DATA_SYSTEM],i);
    format_set.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format_set.fmt.pix.pixelformat = V4L2_PIX_FMT_NV21;

    format_set.fmt.pix.width= format.fmt.pix.width;
    format_set.fmt.pix.height= format.fmt.pix.height;
    if (ioctl(fd, VIDIOC_S_FMT, &format_set) == -1) {
            ALOGV("set tvin image format failed\n");
            return -1 ;
    }
    if((format.fmt.raw_data[RAW_DATA_SYSTEM]==0))
    {
        ALOGD("------------get tvin ntsc\n");
        return TVD_NTSC;
    }
    else if((format.fmt.raw_data[RAW_DATA_SYSTEM]==1))
    {
        ALOGD("------------get tvin pal\n");
        return  TVD_PAL ;
    }
    else if((format.fmt.raw_data[RAW_DATA_SYSTEM]==2)||(format.fmt.raw_data[RAW_DATA_SYSTEM]==3))
    {
        ALOGD("---------get tvin ypbpr\n");
        return   TVD_YPBPR;
    }
    ALOGD("---------get no type \n");
    return   0;

}

// -----------------------------------------------------------------------------
// extended interfaces here <***** star *****>
// -----------------------------------------------------------------------------
int V4L2CameraDevice::openCameraDev(HALCameraInfo * halInfo)
{
    int ret = -1;
    struct v4l2_input inp;
    struct v4l2_capability cap;

    if (halInfo == NULL)
    {
        ALOGD("error HAL camera info");
        return -1;
    }

    // open V4L2 device
    ALOGD("Camera[%d] openCameraDev enter",halInfo->device_id);
    mCameraFd = open(halInfo->device_name, O_RDWR | O_NONBLOCK, 0);
    if (mCameraFd == -1)
    {
        ALOGE("ERROR opening %s: %s", halInfo->device_name, strerror(errno));
        return -1;
    }
    mCameraId = halInfo->device_id;
    // check v4l2 device capabilities
    ret = ioctl (mCameraFd, VIDIOC_QUERYCAP, &cap);
    if (ret < 0)
    {
        ALOGE("Camera[%d] Error opening device: unable to query device.",mCameraId);
        goto END_ERROR;
    }

    if (!strcmp((char *)cap.driver, "uvcvideo"))
    {
        mCameraType = CAMERA_TYPE_UVC;
        halInfo->is_uvc = true;
        ALOGD("Camera[%d] mCameraType = CAMERA_TYPE_UVC",mCameraId);
    }
    else if (!strcmp((char *)cap.driver, "sunxi-tvd" ))
    {
        int tvinType;
        tvinType = getTVINSystemType(mCameraFd);
        if(tvinType == TVD_PAL )
        {
            mCameraType = CAMERA_TYPE_TVIN_PAL;
            ALOGD("Camera[%d] mCameraType = CAMERA_TYPE_TVIN_PAL",mCameraId);
        }
        else if(tvinType == TVD_NTSC )
        {
            mCameraType = CAMERA_TYPE_TVIN_NTSC;
            ALOGD("Camera[%d] mCameraType = CAMERA_TYPE_TVIN_NTSC",mCameraId);
        }
        else if(tvinType == TVD_YPBPR )
        {
            mCameraType = CAMERA_TYPE_TVIN_YPBPR;
            ALOGD("Camera[%d] mCameraType = CAMERA_TYPE_TVIN_YPBPR",mCameraId);
        }
        else
        {
            mCameraType = CAMERA_TYPE_TVIN_NTSC;
            ALOGD("Camera[%d] Default mCameraType = CAMERA_TYPE_TVIN_NTSC",mCameraId);
        }
    }
    else
    {
        mCameraType = CAMERA_TYPE_CSI;
        ALOGD("Camera[%d] mCameraType = CAMERA_TYPE_CSI",mCameraId);
    }
    if (mCameraType != CAMERA_TYPE_CSI) {
        if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0)
        {
            ALOGD("Camera[%d] Error opening device: video capture not supported.",mCameraId);
            goto END_ERROR;
        }

        if ((cap.capabilities & V4L2_CAP_STREAMING) == 0)
        {
            ALOGD("Camera[%d] Capture device does not support streaming i/o",mCameraId);
            goto END_ERROR;
        }
    }
    if (mCameraType == CAMERA_TYPE_CSI)
    {
        // uvc do not need to set input
        inp.index = 0;//halInfo->device_id;
        if (-1 == ioctl (mCameraFd, VIDIOC_S_INPUT, &inp))
        {
            ALOGD("Camera[%d] VIDIOC_S_INPUT error!",mCameraId);
            goto END_ERROR;
        }

        halInfo->facing = CAMERA_FACING_FRONT;
       //mIsThumbUsedForVideo = true;
    }

    if (mCameraType == CAMERA_TYPE_UVC)
    {
        // try to support this format: NV21, YUYV
        // we do not support mjpeg camera now
        if (tryFmt(V4L2_PIX_FMT_NV21) == OK)
        {
              mCaptureFormat = V4L2_PIX_FMT_NV21;
              ALOGD("Camera[%d] capture format: V4L2_PIX_FMT_NV21",mCameraId);
        }

        else if(tryFmt(V4L2_PIX_FMT_H264) == OK)
        {
            mCaptureFormat = V4L2_PIX_FMT_H264;
            ALOGD("Camera[%d] capture format: V4L2_PIX_FMT_H264",mCameraId);
        }

        else  if(tryFmt(V4L2_PIX_FMT_MJPEG) == OK)
        {
            mCaptureFormat = V4L2_PIX_FMT_MJPEG;        // maybe usb camera
            ALOGD("Camera[%d] capture format: V4L2_PIX_FMT_MJPEG",mCameraId);
        }
        else if(tryFmt(V4L2_PIX_FMT_YUYV) == OK)
        {
            mCaptureFormat = V4L2_PIX_FMT_YUYV;     // maybe usb camera
            ALOGD("Camera[%d] capture format: V4L2_PIX_FMT_YUYV",mCameraId);
        }
        else
        {
            ALOGD("Camera[%d] driver should surpport NV21/NV12 or YUYV format, but it not!",mCameraId);
            goto END_ERROR;
        }
    }
    ALOGD("openCameraDev[%d] name:%s succesfully",mCameraId,halInfo->device_name);
    return OK;

END_ERROR:

    if (mCameraFd != NULL)
    {
        close(mCameraFd);
        mCameraFd = NULL;
    }

    return -1;
}

void V4L2CameraDevice::closeCameraDev()
{
    ALOGD("Camera[%d] closeCameraDev",mCameraId);
    if (mCameraFd != NULL)
    {
        close(mCameraFd);
        mCameraFd = NULL;
    }
}


int V4L2CameraDevice::v4l2SetVideoParams(int width, int height, uint32_t pix_fmt)
{
    int ret = UNKNOWN_ERROR;
    struct v4l2_format format;

    ALOGD("Camera[%d] %s, line: %d, w: %d, h: %d, pfmt: %d",mCameraId,
        __FUNCTION__, __LINE__, width, height, pix_fmt);

    memset(&format, 0, sizeof(format));
    if (mCameraType == CAMERA_TYPE_CSI) {
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        format.fmt.pix_mp.width  = width;
        format.fmt.pix_mp.height = height;
        format.fmt.pix_mp.field = V4L2_FIELD_NONE;
        format.fmt.pix_mp.pixelformat = pix_fmt;
    } else {
        int i = 0;
        if(mCameraType == CAMERA_TYPE_TVIN_PAL || mCameraType == CAMERA_TYPE_TVIN_NTSC)
        {
            format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            while (ioctl(mCameraFd, VIDIOC_G_FMT, &format) &&(i++ < 20))
            {
                    ALOGD("+++get tvin signal failed.\n");
                    return -1;
            }
        }
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        format.fmt.pix.width  = width;
        format.fmt.pix.height = height;
        if (mCaptureFormat == V4L2_PIX_FMT_YUYV)
        {
            format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
        }
        else if(mCaptureFormat == V4L2_PIX_FMT_MJPEG)
        {
            format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
        }
        else if(mCaptureFormat == V4L2_PIX_FMT_H264)
        {
            format.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
        }
        else
        {
            format.fmt.pix.pixelformat = pix_fmt;
        }
        format.fmt.pix.field = V4L2_FIELD_NONE;
    }
    ret = ioctl(mCameraFd, VIDIOC_S_FMT, &format);
    if (ret < 0)
    {
        ALOGE("Camera[%d] VIDIOC_S_FMT Failed: %s", mCameraId,strerror(errno));
        return ret;
    }
    if (mCameraType == CAMERA_TYPE_CSI) {
        ret = ioctl(mCameraFd, VIDIOC_G_FMT, &format);
        if (ret < 0)
        {
            ALOGE("Camera[%d]  VIDIOC_G_FMT Failed: %s", mCameraId,strerror(errno));
            return ret;
        }
        else
        {
            nPlanes = format.fmt.pix_mp.num_planes;
            ALOGD("Camera[%d] VIDIOC_G_FMT resolution = %d*%d num_planes = %d\n",mCameraId,format.fmt.pix_mp.width, format.fmt.pix_mp.height,format.fmt.pix_mp.num_planes);
        }
        mFrameWidth = format.fmt.pix_mp.width;
        mFrameHeight = format.fmt.pix_mp.height;
    } else {
        mFrameWidth = format.fmt.pix.width;
        mFrameHeight = format.fmt.pix.height;
    }
    return OK;
}

int V4L2CameraDevice::v4l2ReqBufs(int * buf_cnt)
{
    int ret = UNKNOWN_ERROR;
    struct v4l2_requestbuffers rb;

    ALOGD("Camera[%d] v4l2ReqBufs count: %d", mCameraId,*buf_cnt);

    memset(&rb, 0, sizeof(rb));
    rb.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (mCameraType == CAMERA_TYPE_CSI) {
        rb.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    }
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
        ALOGE("Camera[%d]Init: VIDIOC_REQBUFS failed: %s",mCameraId, strerror(errno));
        return ret;
    }

    *buf_cnt = rb.count;
     ALOGD("Camera[%d] v4l2ReqBufs count: %d,memory type:%d succesfully",mCameraId, *buf_cnt,rb.memory);
    return OK;
}

int V4L2CameraDevice::v4l2QueryBuf()
{
    int ret = UNKNOWN_ERROR;
    struct v4l2_buffer buf;
    ALOGD("Camera[%d] v4l2QueryBuf cnt:%d enter",mCameraId,mBufferCnt);
    for (int i = 0; i < mBufferCnt; i++)
    {
        memset (&buf, 0, sizeof (struct v4l2_buffer));
        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (mCameraType == CAMERA_TYPE_CSI) {
            struct v4l2_plane planes[VIDEO_MAX_PLANES];
            memset(planes, 0, VIDEO_MAX_PLANES*sizeof(struct v4l2_plane));
            buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
            buf.length = nPlanes;
            buf.m.planes = planes;
            if (NULL == buf.m.planes) {
                ALOGE("Camera[%d] v4l2QueryBuf calloc failed!",mCameraId);
            }
        }
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
            ALOGE("Camera[%d] v4l2QueryBuf Unable to query buffer (%s)", mCameraId,strerror(errno));
            return ret;
        }

        switch (buf.memory) {
            case V4L2_MEMORY_MMAP:
                mMapMem.mem[i] = mmap (0, buf.length,
                            PROT_READ | PROT_WRITE,
                            MAP_SHARED,
                            mCameraFd,
                            buf.m.offset);
                mMapMem.length = buf.length;
                if (mMapMem.mem[i] == MAP_FAILED){
                    ALOGE("Camera[%d] Unable to map buffer (%s)", mCameraId,strerror(errno));
                    for(int j = 0;j < i;j++){
                        munmap(mMapMem.mem[j], mMapMem.length);
                    }
                    return -1;
                }
                ALOGD("Camera[%d] mem[%d]: %lx, len: %x, offset: %x",mCameraId, i, (unsigned long)mMapMem.mem[i], buf.length, buf.m.offset);
                break;
            case V4L2_MEMORY_USERPTR:
                mMapMem.length = GPU_BUFFER_ALIGN(ALIGN_16B(mFrameWidth)*mFrameHeight*3/2);/*buf.length;*/
                mMapMem.mem[i] = mMemOpsS->palloc_cam(mMapMem.length,&mMapMem.nShareBufFd[i]);
                mMapMem.length = buf.length;
                if (mMapMem.mem[i] == NULL){
                     ALOGE("Camera[%d] v4l2QueryBuf buffer allocate ERROR(%s)",mCameraId, strerror(errno));
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
                ALOGD("Camera[%d] v4l2QueryBuf mem[%d]:0x%lx,len:%d,fd:%d,w*h:%d,%d", mCameraId,i, (unsigned long)mMapMem.mem[i], mMapMem.length, mMapMem.nShareBufFd[i],mFrameWidth,mFrameHeight);
                break;
            default:
                break;
            }

        // start with all buffers in queue
        ret = ioctl(mCameraFd, VIDIOC_QBUF, &buf);
        if (ret < 0)
        {
            ALOGE("Camera[%d] v4l2QueryBuf VIDIOC_QBUF Failed",mCameraId);
            return ret;
        }

        if (mCameraType == CAMERA_TYPE_UVC )        // star to do
        {
            int buffer_len = mFrameWidth * mFrameHeight * 3 / 2;
            mVideoBuffer.buf_vir_addr[i] = (unsigned long)mMemOpsS->palloc_cam(buffer_len,&mVideoBuffer.nShareBufFd[i]);
            mVideoBuffer.buf_phy_addr[i] = (unsigned long)mMemOpsS->cpu_get_phyaddr_cam((void*)mVideoBuffer.buf_vir_addr[i]);
            ALOGD("Camera[%d] video buffer: index: %d, vir: %x, phy: %x, len: %x",mCameraId,
                    i, mVideoBuffer.buf_vir_addr[i], mVideoBuffer.buf_phy_addr[i], buffer_len);

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

                ALOGV("DiOutPutbuffer: index: %d, vir: %x, phy: %x, len: %x, V4L2_PIX_FMT_NV61",
                        i, mDiOutPutBuffer.buf_vir_addr[i], mDiOutPutBuffer.buf_phy_addr[i], buffer_size);

                memset((void*)mDiOutPutBuffer.buf_vir_addr[i], 0x10, ALIGN_16B(mFrameWidth) * mFrameHeight);
                memset((void*)(mDiOutPutBuffer.buf_vir_addr[i] + ALIGN_16B(mFrameWidth) * mFrameHeight),
                        0x80, mFrameWidth * mFrameHeight);

                mDiOutPutBuffer.buf_vir_addr[i + NB_BUFFER] = (unsigned long)mMemOpsS->palloc_cam(buffer_size,
                     &mDiOutPutBuffer.nShareBufFd[i + NB_BUFFER]);
                mDiOutPutBuffer.buf_phy_addr[i + NB_BUFFER] = (unsigned long)mMemOpsS->cpu_get_phyaddr_cam(
                    (void*)mDiOutPutBuffer.buf_vir_addr[i + NB_BUFFER]);

                ALOGV("DiOutPutbuffer: index: %d, vir: %x, phy: %x, len: %x, V4L2_PIX_FMT_NV61",
                    i + NB_BUFFER, mDiOutPutBuffer.buf_vir_addr[i + NB_BUFFER], mDiOutPutBuffer.buf_phy_addr[i + NB_BUFFER], buffer_size);

                memset((void*)mDiOutPutBuffer.buf_vir_addr[i + NB_BUFFER], 0x10, ALIGN_16B(mFrameWidth) * mFrameHeight);
                memset((void*)(mDiOutPutBuffer.buf_vir_addr[i + NB_BUFFER] + ALIGN_16B(mFrameWidth) * mFrameHeight),
                        0x80, mFrameWidth * mFrameHeight);
            }else {
                int buffer_size = ALIGN_16B(mFrameWidth) * mFrameHeight * 3 >> 1;

                mDiOutPutBuffer.buf_vir_addr[i] = (unsigned long)mMemOpsS->palloc_cam(buffer_size,
                     &mDiOutPutBuffer.nShareBufFd[i]);
                mDiOutPutBuffer.buf_phy_addr[i] = (unsigned long)mMemOpsS->cpu_get_phyaddr_cam((void*)mDiOutPutBuffer.buf_vir_addr[i]);

                ALOGV("DiOutPutbuffer: index: %d, vir: %x, phy: %x, len: %x",
                        i, mDiOutPutBuffer.buf_vir_addr[i], mDiOutPutBuffer.buf_phy_addr[i], buffer_size);

                memset((void*)mDiOutPutBuffer.buf_vir_addr[i], 0x10, ALIGN_16B(mFrameWidth) * mFrameHeight);
                memset((void*)(mDiOutPutBuffer.buf_vir_addr[i] + ALIGN_16B(mFrameWidth) * mFrameHeight),
                        0x80, mFrameWidth * mFrameHeight / 2);
            }

        }
#endif

    }
    mPicBuffer.addrVirY = (unsigned long)mMemOpsS->palloc_cam(MAX_PICTURE_SIZE,  &mPicBuffer.nShareBufFd);
    mPicBuffer.addrPhyY = (unsigned long)mMemOpsS->cpu_get_phyaddr_cam((void*)mPicBuffer.addrVirY);

    return OK;
}

int V4L2CameraDevice::v4l2StartStreaming()
{
    ALOGD("Camera[%d] v4l2StartStreaming enter",mCameraId);
    int ret = UNKNOWN_ERROR;
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (mCameraType == CAMERA_TYPE_CSI) {
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    }

    ret = ioctl (mCameraFd, VIDIOC_STREAMON, &type);
    if (ret < 0)
    {
        ALOGE("Camera[%d] StartStreaming: Unable to start capture: %s", mCameraId,strerror(errno));
        return ret;
    }

    return OK;
}

int V4L2CameraDevice::v4l2StopStreaming()
{
    ALOGD("Camera[%d] v4l2StopStreaming enter",mCameraId);
    int ret = UNKNOWN_ERROR;
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (mCameraType == CAMERA_TYPE_CSI) {
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    }
    ret = ioctl (mCameraFd, VIDIOC_STREAMOFF, &type);
    if (ret < 0)
    {
        ALOGE("Camera[%d] StopStreaming: Unable to stop capture: %s", mCameraId,strerror(errno));
        return ret;
    }
    ALOGD("Camera[%d] V4L2Camera::v4l2StopStreaming OK",mCameraId);
    return OK;
}

int V4L2CameraDevice::v4l2UnmapBuf()
{
    ALOGD("Camera[%d] %s enter",mCameraId,__FUNCTION__);
    int ret = UNKNOWN_ERROR;
    for (int i = 0; i < mBufferCnt; i++)
    {
        ret = munmap(mMapMem.mem[i], mMapMem.length);
        if (ret < 0)
        {
            ALOGE("Camera[%d] v4l2UnmapBuf Unmap failed",mCameraId);
            return ret;
        }

        mMapMem.mem[i] = NULL;
        if (mVideoBuffer.buf_vir_addr[i] != 0)
        {
            mMemOpsS->pfree_cam((void*)mVideoBuffer.buf_vir_addr[i]);
            mVideoBuffer.buf_phy_addr[i] = 0;
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

    if (mPicBuffer.addrVirY != 0) {
        mMemOpsS->pfree_cam((void*)mPicBuffer.addrVirY);
        mPicBuffer.addrVirY = 0;
    }
    ALOGD("Camera[%d] %s end",mCameraId,__FUNCTION__);
    return OK;
}

void V4L2CameraDevice::releasePreviewFrame(int index)
{
    int ret = UNKNOWN_ERROR;
    struct v4l2_buffer buf;

    if (index == NOT_NEED_RELEASE_INDEX)
    {
        ALOGV("Camera[%d] type:%d, index:%x", mCameraId, mCameraType, index);
        return;
    }

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
        buf.length = nPlanes;
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

        ALOGV("Camera[%d]Qbuf:%d, addr:%x, len:%d", mCameraId,buf.index, mMapMem.mem[index],buf.length);
        ret = ioctl(mCameraFd, VIDIOC_QBUF, &buf);
        if (ret != 0)
        {
            ALOGE("Camera[%d]releasePreviewFrame: VIDIOC_QBUF Failed: index = %d, ret = %d, addr:%x, len:%d, %s",mCameraId,
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
                buf.length = nPlanes;
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
            ALOGD("Camera[%d]RecoveryPreviewFrame idx: %d",mCameraId,i);
            if (ret != 0)
            {
                ALOGE("Camera[%d] recoveryPreviewFrame: VIDIOC_QBUF Failed: index = %d, ret = %d, %s",mCameraId,
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
        buf->length = nPlanes;
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
    }

    ret = ioctl(mCameraFd, VIDIOC_DQBUF, buf);

    if (ret < 0)
    {
        if (mDebugLogCnt == 0 || mDebugLogCnt >= 1500) {
            ALOGW("Camera[%d]GetPreviewFrame: VIDIOC_DQBUF Failed, %s, mCurAvailBufferCnt %d",mCameraId,
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

    ALOGD("Camera[%d] %s enter",mCameraId,__FUNCTION__);
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
        ALOGD("Camera[%d]tryFmt index = %d, name = %s, v4l2 pixel format = %x\n",mCameraId,
            i, fmtdesc.description, fmtdesc.pixelformat);

        if (fmtdesc.pixelformat == format)
        {
            return OK;
        }
    }

    return -1;
}

int V4L2CameraDevice::tryFmtSize(int * width, int * height)
{
    ALOGD("Camera[%d] %s enter",mCameraId,__FUNCTION__);
    int ret = -1;
    struct v4l2_format fmt;

    ALOGD("Camera[%d]Before tryFmtSize: w: %d, h: %d", mCameraId,*width, *height);
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
        if(mCaptureFormat == V4L2_PIX_FMT_MJPEG)
        {
            fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
        }
        else if (mCaptureFormat == V4L2_PIX_FMT_YUYV)
        {
            fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
        }
        else if (mCaptureFormat == V4L2_PIX_FMT_H264)
        {
            fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
        }
        else
        {
            fmt.fmt.pix.pixelformat = mVideoFormat;
        }
        fmt.fmt.pix.field = V4L2_FIELD_NONE;
    }
    ret = ioctl(mCameraFd, VIDIOC_TRY_FMT, &fmt);
    if (ret < 0)
    {
        ALOGE("Camera[%d]VIDIOC_TRY_FMT Failed: %s", mCameraId,strerror(errno));
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
    ALOGD("Camera[%d]After tryFmtSize: w: %d, h: %d",mCameraId, *width, *height);
    return 0;
}

int V4L2CameraDevice::setFrameRate(int rate)
{
    ALOGD("Camera[%d] %s rate:%d enter",mCameraId,__FUNCTION__,rate);
    mFrameRate = rate;
    return OK;
}

int V4L2CameraDevice::getFrameRate()
{
    ALOGD("Camera[%d] %s enter",mCameraId,__FUNCTION__);
    int ret = -1;

    struct v4l2_streamparm parms;
    parms.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (mCameraType == CAMERA_TYPE_CSI) {
        parms.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    }

    ret = ioctl (mCameraFd, VIDIOC_G_PARM, &parms);
    if (ret < 0)
    {
        ALOGE("Camera[%d] getFrameRate VIDIOC_G_PARM error, %s",mCameraId, strerror(errno));
        return ret;
    }

    int numerator = parms.parm.capture.timeperframe.numerator;
    int denominator = parms.parm.capture.timeperframe.denominator;

    if (numerator != 0
        && denominator != 0)
    {
        ALOGD("Camera[id]getFrameRate:%d fps",mCameraId,denominator / numerator);
        return denominator / numerator;
    }
    else
    {
        ALOGW("Camera[id] unsupported frame return defualt:25fps",mCameraId);
        return 25;
    }
}

int V4L2CameraDevice::setImageEffect(int effect)
{
    ALOGD("Camera[%d] %s enter",mCameraId,__FUNCTION__);
    int ret = -1;
    struct v4l2_control ctrl;

    ctrl.id = V4L2_CID_COLORFX;
    ctrl.value = effect;
    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
        ALOGE("Camera[%d] setImageEffect failed!",mCameraId);
    else
        ALOGD("Camera[%d] setImageEffect ok",mCameraId);
    return ret;
}

int V4L2CameraDevice::setWhiteBalance(int wb)
{
    struct v4l2_control ctrl;
    int ret = -1;
    ALOGD("Camera[%d] %s enter",mCameraId,__FUNCTION__);
    ctrl.id = V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE;
    ctrl.value = wb;
    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
        ALOGE("Camera[%d] setWhiteBalance failed, %s",mCameraId, strerror(errno));
    else
        ALOGD("Camera[%d] setWhiteBalance ok",mCameraId);

    return ret;
}

int V4L2CameraDevice::setTakePictureCtrl(enum v4l2_take_picture value)
{
    struct v4l2_control ctrl;
    int ret = -1;
    ALOGD("Camera[%d] %s:%d enter",mCameraId,__FUNCTION__,value);
    if (mHalCameraInfo.fast_picture_mode)
    {
        ctrl.id = V4L2_CID_TAKE_PICTURE;
        ctrl.value = value;
        ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
        if (ret < 0)
            ALOGE("Camera[%d] setTakePictureCtrl failed, %s", mCameraId,strerror(errno));
        else
            ALOGD("Camera[%d] setTakePictureCtrl ok",mCameraId);
        return ret;
    }
    return 0;
}

// ae mode
int V4L2CameraDevice::setExposureMode(int mode)
{
    ALOGD("Camera[%d] %s:%d enter",mCameraId,__FUNCTION__,mode);
    int ret = -1;
    struct v4l2_control ctrl;

    ctrl.id = V4L2_CID_EXPOSURE_AUTO;
    ctrl.value = mode;
    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
        
ALOGE("Camera[%d]setExposureMode failed, %s",mCameraId, strerror(errno));
    else
        ALOGD("Camera[%d]setExposureMode ok",mCameraId);

    return ret;
}

// ae compensation
int V4L2CameraDevice::setExposureCompensation(int val)
{
    ALOGD("Camera[%d] %s enter",mCameraId,__FUNCTION__);
    int ret = -1;
    struct v4l2_control ctrl;
    mExposureBias = val;
    ctrl.id = V4L2_CID_AUTO_EXPOSURE_BIAS;
    ctrl.value = val + 4;
    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
        ALOGE("Camera[%d]setExposureCompensation failed, %s",mCameraId, strerror(errno));
    else
        ALOGD("Camera[%d]setExposureCompensation ok",mCameraId);

    return ret;
}

int V4L2CameraDevice::setExposureWind(int num, void *wind)
{
    F_LOG;
    int ret = -1;
    return ret;
}

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
    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
        ALOGE("setFlashMode failed, %s", strerror(errno));
    else
        ALOGV("setFlashMode ok");

    return ret;
}

// af init
int V4L2CameraDevice::setAutoFocusInit()
{
    F_LOG;
    int ret = -1;
    struct v4l2_control ctrl;

    if(mCameraType == CAMERA_TYPE_CSI)
    {
        ctrl.id = V4L2_CID_AUTO_FOCUS_INIT;
        ctrl.value = 0;
        ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
        if (ret < 0)
            ALOGE("setAutoFocusInit failed, %s", strerror(errno));
        else
            ALOGV("setAutoFocusInit ok");
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
        ALOGE("setAutoFocusRelease failed, %s", strerror(errno));
    else
        ALOGV("setAutoFocusRelease ok");

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
        ALOGV("setAutoFocusRange id V4L2_CID_FOCUS_AUTO failed, %s", strerror(errno));
    else
        ALOGV("setAutoFocusRange id V4L2_CID_FOCUS_AUTO ok");

    ctrl.id = V4L2_CID_AUTO_FOCUS_RANGE;
    ctrl.value = af_range;
    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
        ALOGV("setAutoFocusRange id V4L2_CID_AUTO_FOCUS_RANGE failed, %s", strerror(errno));
    else
        ALOGV("setAutoFocusRange id V4L2_CID_AUTO_FOCUS_RANGE ok");

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
        ALOGD("setAutoFocusCtrl failed, %s", strerror(errno));
    else
        ALOGV("setAutoFocusCtrl ok");

    return ret;
}
#else
int V4L2CameraDevice::setAutoFocusWind(int num, void *wind)
{
    F_LOG;
    int ret = -1;
#if 0
    struct v4l2_control ctrl;

    ctrl.id = V4L2_CID_AUTO_FOCUS_WIN_NUM;
    ctrl.value = num;
    ctrl.user_pt = (unsigned int)wind;
    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
        ALOGD("setAutoFocusCtrl failed, %s", strerror(errno));
    else
        ALOGV("setAutoFocusCtrl ok");
#endif

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
        ALOGD("setAutoFocusStart failed, %s", strerror(errno));
    else
        ALOGV("setAutoFocusStart ok");

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
        ALOGD("setAutoFocusStart failed, %s", strerror(errno));
    else
        ALOGV("setAutoFocusStart ok");

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
        //ALOGV("getAutoFocusCtrl ok");
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

#ifdef USE_SUNXI_CAMERA_H
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
#else
int V4L2CameraDevice::setHDRMode(void *hdr_setting)
{
    int ret = -1;
#if 0

    struct v4l2_control ctrl;
    struct v4l2_queryctrl qc_ctrl;

    ctrl.value = 0;
    ctrl.user_pt = (unsigned int)hdr_setting;
    if (mCameraFd == NULL)
    {
        return 0xFF000000;
    }
    ctrl.id = V4L2_CID_HDR;
    qc_ctrl.id = V4L2_CID_HDR;

    if (-1 == ioctl (mCameraFd, VIDIOC_QUERYCTRL, &qc_ctrl))
    {
       return 0;
    }
    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
#endif
    return ret;
}
#endif

int V4L2CameraDevice::getAeStat(struct isp_stat_buf *AeBuf)
{
    int ret = -1;
    if (mCameraFd == NULL)
    {
        return 0xFF000000;
    }
    //AeBuf->buf = malloc(0xc00);
    //memset(AeBuf->buf,0,0xc00);
    //ALOGD("AeBuf->buf == %x\n",AeBuf->buf);
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
    ALOGD("GammaBuf->buf == %x\n",GammaBuf->buf);
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
        ALOGD("setGain failed, %s", strerror(errno));
    else
        ALOGV("setGain ok");
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
        ALOGD("Set V4L2_CID_EXPOSURE failed, %s", strerror(errno));
    else
        ALOGV("Set V4L2_CID_EXPOSURE ok");

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
        ALOGV("set3ALock ok");

    return ret;
}

int V4L2CameraDevice::v4l2setCaptureParams()
{
    ALOGD("Camera[%d] %s enter",mCameraId,__FUNCTION__);
    int ret = -1;
    struct v4l2_streamparm params;
    memset(&(params), 0, sizeof (params));
    params.parm.capture.timeperframe.numerator = 1;
    params.parm.capture.timeperframe.denominator = mFrameRate;
    params.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    params.parm.capture.reserved[0] = 0;/*when different video have the same sensor source, 1:use sensor current win, 0:find the nearest win*/
    params.parm.capture.reserved[1] = 0;/*2:command, 1: wdr, 0: normal*/

    if (mCameraType == CAMERA_TYPE_CSI) {
        params.parm.capture.reserved[0] = 0;
        params.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    }
    if (mTakePictureState == TAKE_PICTURE_NORMAL)
    {
        params.parm.capture.capturemode = V4L2_MODE_IMAGE;
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

    ALOGD("Camera[%d]v4l2setCaptureParams rate:%d fps,capture mode:%d", mCameraId,mFrameRate, params.parm.capture.capturemode);

    ret = ioctl(mCameraFd, VIDIOC_S_PARM, &params);
    if (ret < 0)
        ALOGE("Camera[%d]v4l2setCaptureParams failed, %s", mCameraId,strerror(errno));
    else
        ALOGD("Camera[%d]v4l2setCaptureParams ok",mCameraId);

    return ret;
}

int V4L2CameraDevice::enumSize(char * pSize, int len)
{
    ALOGD("Camera[%d] %s enter",mCameraId,__FUNCTION__);
    struct v4l2_frmsizeenum size_enum;
    size_enum.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (mCameraType == CAMERA_TYPE_CSI) {
        size_enum.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    }
    size_enum.pixel_format = mCaptureFormat;

    if (pSize == NULL)
    {
        ALOGE("Camera[%d]error input params",mCameraId);
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
        // ALOGV("format index = %d, size_enum: %dx%d", i, size_enum.discrete.width, size_enum.discrete.height);
        sprintf(str, "%dx%d", size_enum.discrete.width, size_enum.discrete.height);
        if (i != 0)
        {
            strcat(pSize, ",");
        }
        strcat(pSize, str);
    }
    ALOGD("Camera[%d] enumSize:%s end",mCameraId,pSize);
    return OK;
}

int V4L2CameraDevice::getFullSize(int * full_w, int * full_h)
{
    ALOGD("Camera[%d] %s enter",mCameraId,__FUNCTION__);
    struct v4l2_frmsizeenum size_enum;
    size_enum.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (mCameraType == CAMERA_TYPE_CSI) {
        size_enum.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    }
    size_enum.pixel_format = mCaptureFormat;
    size_enum.index = 0;
    if (-1 == ioctl (mCameraFd, VIDIOC_ENUM_FRAMESIZES, &size_enum))
    {
        ALOGE("Camera[%d]getFullSize failed",mCameraId);
        return -1;
    }

    *full_w = size_enum.discrete.width;
    *full_h = size_enum.discrete.height;

    ALOGD("Camera[%d] getFullSize: %dx%d end",mCameraId, *full_w, *full_h);

    return OK;
}

int V4L2CameraDevice::getSuitableThumbScale(int full_w, int full_h)
{
    ALOGD("Camera[%d] %s enter",mCameraId,__FUNCTION__);
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
    ALOGD("Camera[%d] %s:%dx%d enter",mCameraId,__FUNCTION__,mThumbWidth,mThumbHeight);
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
        ALOGD("query sensor type ctrl failed");
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

#ifdef CAMERA_MANAGER_ENABLE
void V4L2CameraDevice::setCameraManager(CameraManager * manager)
{
    ALOGD("Camera[%d]setCameraManager manager=0x%x",mCameraId,manager);
    mCameraManager = manager;

}

void V4L2CameraDevice::startPreview_d()
{
    //ALOGD("CameraDebug,F:%s,L:%d,id:%d,En:%d",__FUNCTION__,__LINE__,mHalCameraInfo.device_id,mCameraManager->isOviewEnable());
    ALOGD("Camera[%d] %s enter",mCameraId,__FUNCTION__);
    if((mCameraManager != NULL) && (mCameraManager->isOviewEnable())
        && (mHalCameraInfo.device_id == mCameraManager->mStartCameraID))
    {
        mCameraManager->startPreview();
        ALOGD("Camera[%d]F:%s,L:%d",mCameraId,__FUNCTION__,__LINE__);
    }
}

void V4L2CameraDevice::stopPreview_d()
{
    ALOGD("Camera[%d] %s enter",mCameraId,__FUNCTION__);
    if((mCameraManager != NULL) && (mCameraManager->isOviewEnable())
        && (mHalCameraInfo.device_id == mCameraManager->mStartCameraID))
    {
        mCameraManager->stopPreview();
        ALOGD("Camera[%d]F:%s,L:%d",mCameraId,__FUNCTION__,__LINE__);
    }
}

void V4L2CameraDevice::releaseCamera_d()
{
    ALOGD("Camera[%d] %s enter",mCameraId,__FUNCTION__);
    if((mCameraManager != NULL) && (mCameraManager->isOviewEnable())
        && (mHalCameraInfo.device_id == mCameraManager->mStartCameraID))
    {
        mCameraManager->releaseCamera();
        ALOGD("Camera[%d]F:%s,L:%d",mCameraId,__FUNCTION__,__LINE__);
    }
}
void V4L2CameraDevice::takePicInputBuffer(V4L2BUF_t *pic_buffer)
{
    int ret;
    ret = OSAL_Queue(&mQueueBufferPicture, pic_buffer);
    if (ret != 0) {
        ALOGW("Camera[%d]Rec queue full",mCameraId);
        return;
    }
    struct isp_exif_attribute exif_attri;
    getExifInfo(&exif_attri);
    mCallbackNotifier->setExifInfo(exif_attri, mZoomRatio, mExposureBias);
    mCameraHardware->setExifInfo(exif_attri);
    mTakePictureState = TAKE_PICTURE_NULL;
    pthread_cond_signal(&mPictureCond);
}

#endif
}; /* namespace android */
