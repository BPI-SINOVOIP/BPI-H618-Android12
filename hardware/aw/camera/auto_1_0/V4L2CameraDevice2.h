
#ifndef __HAL_V4L2_CAMERA_DEVICE_H__
#define __HAL_V4L2_CAMERA_DEVICE_H__

/*
 * Contains declaration of a class V4L2CameraDevice.
 */

#include <ui/Rect.h>
#include <fcntl.h>
#include <utils/Thread.h>
#include <hardware/camera.h>
#include <type_camera.h>

#include "CameraPlatform.h"
#include "OSAL_Queue.h"
#include "SceneFactory/SceneModeFactory.h"
#include <memory/memoryAdapter.h>
#include <memory/sc_interface.h>
#include "CameraDecoder.h"
#include "tvd_interface.h"
#ifdef USE_DEINTERLACE_HW
#include "DiProcess.h"
#endif
#ifdef USE_SUNXI_CAMERA_H
#include <sunxi_camera.h>
#else
//#include <videodev2_34.h>
#endif

#ifdef USE_CSI_VIN_DRIVER
#include <sunxi_camera_v2.h>
#endif

#ifdef AW_ADAS_MODULE
#include <ADASAPI.h>
#include <Adas_interface.h>
#endif

#include "AWIspApi.h"


#define NB_BUFFER 6
#define    MAX_YUV_SENSOR_PICTURE_SIZE (2592*1936*3>>1)    //just for yuv sensor
#define MAX_PICTURE_SIZE (2560*1440*3>>1)//(4608*3456*3>>1)
#define MAX_HDR_PICTURE_SIZE (4608*3456*3>>1)
#define SONIX_CAMERA_H264 0
#define CAMERA_DECODE 1
#define TAKE_PIC_USE_COPY 1

namespace android {

class CameraHardware;
class CallbackNotifier;
class PreviewWindow;
class AWIspApi;

#ifdef CAMERA_MANAGER_ENABLE
class CameraManager;
#endif

#ifdef USE_DEINTERLACE_HW
class DiProcess;
#endif

#ifdef CAMERA_DECODE
class CameraDecoder;
#endif


/*
 *
 */
typedef struct HALCameraInfo{
    char    device_name[16];        // device node name, such as "/dev/video0"
    int     device_id;                // device id for camera share with the same CSI
    int     facing;                 // facing front or back
    int     orientation;            //
    bool    fast_picture_mode;        //
    bool    is_uvc;                    // usb camera
    bool    is_vfe;
}HALCameraInfo;

enum TAKE_PICTURE_STATE {
    TAKE_PICTURE_NULL,            // do not take picture
    TAKE_PICTURE_NORMAL,        // stream off -> stream on -> take picture -> stream off -> stream on
    TAKE_PICTURE_FAST,            // normal mode but do not need to stream off/on
    TAKE_PICTURE_RECORD,         // take picture in recording
    TAKE_PICTURE_CONTINUOUS,
    TAKE_PICTURE_CONTINUOUS_FAST,
    TAKE_PICTURE_SMART,            // take smart picture
    TAKE_PICTURE_SCENE_MODE,    // take scene mode picture
};

enum CAMERA_TYPE {
    CAMERA_TYPE_CSI,
    CAMERA_TYPE_UVC,
    CAMERA_TYPE_TVIN_NTSC,
    CAMERA_TYPE_TVIN_PAL,
    CAMERA_TYPE_TVIN_YPBPR,
    CAMERA_TYPE_VFE,
    CAMERA_TYPE_VIR,
};

/*
 * V4L2CameraDevice
 */
class V4L2CameraDevice {
public:
    /* Constructs V4L2CameraDevice instance. */
    V4L2CameraDevice(CameraHardware* camera_hal,
                     PreviewWindow * preview_window,
                     CallbackNotifier * cb);

    /* Destructs V4L2CameraDevice instance. */
    ~V4L2CameraDevice();

    /***************************************************************************
     * V4L2Camera device abstract interface implementation.
     * See declarations of these methods in V4L2Camera class for
     * information on each of these methods.
     **************************************************************************/

public:
    struct ScCamMemOpsS* mMemOpsS;
    /* Connects to the camera device.
     * Since there is no real device to connect to, this method does nothing,
     * but changes the state.
     */
    status_t connectDevice(HALCameraInfo * halInfo);

    /* Disconnects from the camera device.
     * Since there is no real device to disconnect from, this method does
     * nothing, but changes the state.
     */
    status_t disconnectDevice();

    /* Starts the camera device. */
    status_t startDevice(int width, int height, uint32_t pix_fmt, bool video_hint);

    /* Stops the camera device. */
    status_t stopDevice();

    /* Gets current preview fame into provided buffer. */
    status_t getPreviewFrame(void* buffer);

    /* Starts delivering frames captured from the camera device.
     * This method will start the worker thread that would be pulling frames from
     * the camera device, and will deliver the pulled frames back to the emulated
     * camera via onNextFrameAvailable callback. This method must be called on a
     * connected instance of this class with a started camera device. If it is
     * called on a disconnected instance, or camera device has not been started,
     * this method must return a failure.
     * Param:
     *  one_burst - Controls how many frames should be delivered. If this
     *      parameter is 'true', only one captured frame will be delivered to the
     *      V4L2Camera. If this parameter is 'false', frames will keep
     *      coming until stopDeliveringFrames method is called. Typically, this
     *      parameter is set to 'true' only in order to obtain a single frame
     *      that will be used as a "picture" in takePicture method of the
     *      V4L2Camera.
     * Return:
     *  NO_ERROR on success, or an appropriate error status.
     */
    status_t startDeliveringFrames();

    /* Stops delivering frames captured from the camera device.
     * This method will stop the worker thread started by startDeliveringFrames.
     * Return:
     *  NO_ERROR on success, or an appropriate error status.
     */
    status_t stopDeliveringFrames();

    /***************************************************************************
     * Worker thread management overrides.
     * See declarations of these methods in V4L2Camera class for
     * information on each of these methods.
     **************************************************************************/

protected:

    // -------------------------------------------------------------------------
    // extended interfaces here <***** star *****>
    // -------------------------------------------------------------------------

    class DoCaptureThread : public Thread {
        V4L2CameraDevice*    mV4l2CameraDevice;
        bool                mRequestExit;
    public:
        DoCaptureThread(V4L2CameraDevice* dev) :
            Thread(false),
            mV4l2CameraDevice(dev),
            mRequestExit(false) {
        }
        void startThread() {
            run("CameraCaptureThread", PRIORITY_URGENT_DISPLAY);
        }
        void stopThread() {
            mRequestExit = true;
        }
        virtual bool threadLoop() {
            if (mRequestExit) {
                return false;
            }
            return mV4l2CameraDevice->captureThread();
        }
    };

    class DoPreviewThread : public Thread {
        V4L2CameraDevice*    mV4l2CameraDevice;
        bool                mRequestExit;
    public:
        DoPreviewThread(V4L2CameraDevice* dev) :
            Thread(false),
            mV4l2CameraDevice(dev),
            mRequestExit(false) {
        }
        void startThread() {
            run("CameraPreviewThread", PRIORITY_URGENT_DISPLAY);
        }
        void stopThread() {
            mRequestExit = true;
        }
        virtual bool threadLoop() {
            if (mRequestExit) {
                return false;
            }
            return mV4l2CameraDevice->previewThread();
        }
    };

    class DoPictureThread : public Thread {
        V4L2CameraDevice*    mV4l2CameraDevice;
        bool                mRequestExit;
    public:
        DoPictureThread(V4L2CameraDevice* dev) :
            Thread(false),
            mV4l2CameraDevice(dev),
            mRequestExit(false) {
        }
        void startThread() {
            run("CameraPictrueThread", PRIORITY_URGENT_DISPLAY);
        }
        void stopThread() {
            mRequestExit = true;
        }
        virtual bool threadLoop() {
            if (mRequestExit) {
                return false;
            }
            return mV4l2CameraDevice->pictureThread();
        }
    };

    class DoContinuousPictureThread : public Thread {
        V4L2CameraDevice*    mV4l2CameraDevice;
        bool                mRequestExit;
    public:
        DoContinuousPictureThread(V4L2CameraDevice* dev) :
            Thread(false),
            mV4l2CameraDevice(dev),
            mRequestExit(false) {
        }
        void startThread() {
            run("CameraContinuousPictrueThread", PRIORITY_URGENT_DISPLAY);
        }
        void stopThread() {
            mRequestExit = true;
        }
        virtual bool threadLoop() {
            if (mRequestExit) {
                return false;
            }
            return mV4l2CameraDevice->continuousPictureThread();
        }
    };

    class DoSmartPictureThread : public Thread {
        V4L2CameraDevice*    mV4l2CameraDevice;
        bool                mRequestExit;
    public:
        DoSmartPictureThread(V4L2CameraDevice* dev) :
            Thread(false),
            mV4l2CameraDevice(dev),
            mRequestExit(false) {
        }
        void startThread() {
            run("CameraSmartPictrueThread", PRIORITY_URGENT_DISPLAY);
        }
        void stopThread() {
            mRequestExit = true;
        }
        virtual bool threadLoop() {
            if (mRequestExit) {
                return false;
            }
            return mV4l2CameraDevice->smartPictureThread();
        }
    };
public:

    bool captureThread();
    bool previewThread();
    bool pictureThread();
    bool continuousPictureThread();
    bool smartPictureThread();
    int tryFmt(int format);                            // check if driver support this format
    int tryFmtSize(int * width, int * height);        // check if driver support this size
    int setFrameRate(int rate);                        // set frame rate from camera.cfg
    int getFrameRate();                                // get v4l2 device current frame rate
    int testFrameRate();

    int enumSize(char * pSize, int len);
    void getThumbSize(int* sub_w, int* sub_h);
    int getFullSize(int * full_w, int * full_h);
    int getSuitableThumbScale(int full_w, int full_h);

    int setImageEffect(int effect);
    int setWhiteBalance(int wb);

    int setTakePictureCtrl(enum v4l2_take_picture value);

    // exposure
    int setExposureMode(int mode);
    int setExposureCompensation(int val);
    int setExposureWind(int num, void *areas);

    int setFlashMode(int mode);

    // af
    int setAutoFocusInit();
    int setAutoFocusRelease();
    int setAutoFocusRange(int range);
    int setAutoFocusWind(int num, void *areas);
    int setAutoFocusStart();
    int setAutoFocusStop();
    int getAutoFocusStatus();

    int getGainValue();
    int getExpValue();
    int setGainValue(int Gain);
    int setExpValue(int Exp);
    int setHDRMode(void *hdr_setting);
    int getAeStat(struct isp_stat_buf *AeBuf);
    int getGammaStat(struct isp_stat_buf *GammaBuf);
    int getHistStat(struct isp_stat_buf *HistBuf);
    int getSnrValue();
    int set3ALock(int lock);
    void releasePreviewFrame(native_handle_t *handle); //release buffer handle
    void releasePreviewFrame(int index);            // Q buffer for encoder

    int getCurrentFaceFrame(void* frame, int* width, int* height);
    int getSensorType();        //get sensor type: raw or yuv
    int getExifInfo(struct isp_exif_attribute *exif_attri);
    int getHDRFrameCnt()
    {
        return mTakePictureFlag.bits.hdr_cnt;
    }
    int getTVINSystemType(int fd);
    int getCameraType();
    void setThumbUsedForVideo(bool isThumbUsedForVideo)
    {
        mIsThumbUsedForVideo = isThumbUsedForVideo;
    }

    void setVideoSize(int w, int h)
    {
        mVideoWidth = w;
        mVideoHeight = h;
    }

    inline void setCrop(int new_zoom, int max_zoom)
    {
        mNewZoom = new_zoom;
        mMaxZoom = max_zoom;
    }

    inline int getCaptureFormat()
    {
        return mCaptureFormat;
    }

    inline void setHwEncoder(bool hw)
    {
        mUseHwEncoder = hw;
    }

    inline void setTakePictureState(TAKE_PICTURE_STATE state)
    {
        pthread_mutex_lock(&mCaptureMutex);
        LOGV("setTakePictureState %d", state);
        mTakePictureState = state;
        pthread_mutex_unlock(&mCaptureMutex);
    }

    inline void setCount(int *value) {
        pthread_mutex_lock(&mCaptureMutex);
        (*value)++;
        pthread_mutex_unlock(&mCaptureMutex);
    }

#ifdef CAMERA_MANAGER_ENABLE
    inline int getFrameWidth()
    {
        return mFrameWidth;
    }

    inline int getFrameHeight()
    {
        return mFrameHeight;
    }
#endif
    void startContinuousPicture();
    void stopContinuousPicture();
    void setContinuousPictureCnt(int cnt);
    void startSmartPicture();
    void stopSmartPicture();

    void startSceneModePicture(int scenemode);
    void stopSceneModePicture();
    status_t openSceneMode(const char* scenemode);
    void closeSceneMode();
    void stopPreviewThread();
    /*
     * State checkers.
     */
    inline bool isConnected() const {
        /* Instance is connected when its status is either"connected", or
         * "started". */
        return mCameraDeviceState == STATE_CONNECTED || mCameraDeviceState == STATE_STARTED;
    }
    inline bool isStarted() const {
        return mCameraDeviceState == STATE_STARTED;
    }
#ifdef CAMERA_MANAGER_ENABLE
    void setCameraManager(CameraManager * manager);
    void startPreview_d();
    void stopPreview_d();
    void releaseCamera_d();
    void takePicInputBuffer(V4L2BUF_t *pic_buffer);
#endif

private:
    int openCameraDev(HALCameraInfo * halInfo);
    void closeCameraDev();
    int v4l2SetVideoParams(int width, int height, uint32_t pix_fmt);
    int v4l2setCaptureParams();
    int v4l2ReqBufs(int * buf_cnt);
    int v4l2QueryBuf();
    int v4l2StartStreaming();
    int v4l2StopStreaming();
    int v4l2UnmapBuf();

    int v4l2WaitCameraReady();
    int getPreviewFrame(v4l2_buffer *buf);

    void dealWithVideoFrameSW(V4L2BUF_t * pBuf);
    void dealWithVideoFrameHW(V4L2BUF_t * pBuf);
    void dealWithVideoFrameTest(V4L2BUF_t * pBuf);

    /* Checks if it's the time to push new frame to continuous picture.
     * Note that this method must be called while object is locked. */
    bool isContinuousPictureTime();

    bool isPreviewTime();
    void waitFaceDectectTime();

    void singalDisconnect();
public:
#ifdef USE_MP_CONVERT
    // use for YUYV to YUV420C
    void YUYVToYUV420C(const void* yuyv, void *yuv420, int width, int height);
    void NV21ToYV12(const void* nv21, void *yv12, int width, int height);

#endif
    void showformat(int format,char *str);
    void recoveryPreviewFrame();

private:
    // -------------------------------------------------------------------------
    // private data
    // -------------------------------------------------------------------------

    /* Locks this instance for parameters, state, etc. change. */
    Mutex                           mObjectLock;

    // instance of CameraHardware
    CameraHardware *                 mCameraHardware;

    // instance of PreviewWindow
    PreviewWindow *                    mPreviewWindow;

    // instance of CallbackNotifier
    CallbackNotifier *                mCallbackNotifier;

    HALCameraInfo                    mHalCameraInfo;

    /* Defines possible states of the V4L2Camera device object.
     */
    enum CameraDeviceState {
        /* Object has been constructed. */
        STATE_CONSTRUCTED,
        /* Object has been connected to the physical device. */
        STATE_CONNECTED,
        /* Camera device has been started. */
        STATE_STARTED,
    };

    /* Object state. */
    CameraDeviceState               mCameraDeviceState;

    /* Defines possible states of the V4L2CameraDevice capture thread.
     */
    enum CaptureThreadState {
        /* Do not capture frame. */
        CAPTURE_STATE_NULL,
        /* Do not capture frame. */
        CAPTURE_STATE_PAUSED,
        /* Start capture frame. */
        CAPTURE_STATE_STARTED,
        /* exit thread*/
        CAPTURE_STATE_EXIT,
    };

    enum PreviewThreadState {
        /* Do not capture frame. */
        PREVIEW_STATE_NULL,
        /* Do not capture frame. */
        PREVIEW_STATE_PAUSED,
        /* Start capture frame. */
        PREVIEW_STATE_STARTED,
        /* exit thread*/
        PREVIEW_STATE_EXIT,
    };
    /* capture thread state. */
    CaptureThreadState                mCaptureThreadState;
    PreviewThreadState                mPreviewThreadState;

    // v4l2 device handle
    int                                mCameraFd;
    int                                mCameraId;
    // CameraType
    int                         mCameraType;
    v4l2_sensor_type                mSensor_Type;

    int                                mFrameRate;

    TAKE_PICTURE_STATE                mTakePictureState;
    V4L2BUF_t                        mPicBuffer;
    bool                            mIsPicCopy;

    /* Frame width */
    int                             mFrameWidth;

    /* Frame height */
    int                             mFrameHeight;

    /* thumb Frame width */
    int                             mThumbWidth;

    /* thumb Frame height */
    int                             mThumbHeight;

    /* Timestamp of the current frame. */
    nsecs_t                         mCurFrameTimestamp;

    typedef struct v4l2_mem_map_t{
        void *    mem[NB_BUFFER];
        int     length;
        int             nShareBufFd[NB_BUFFER];
        int             nDmaBufFd[NB_BUFFER];
    }v4l2_mem_map_t;
    v4l2_mem_map_t                    mMapMem;

    /* the number of pictrue-addr we can get from V4L2,the value rely on the
    * macro we set to V4L2,in v4l2SetVideoParams Parameter pix_fmt
    * 1.nplanes = 1:we get yuv addr buf.m.planes[0]
    * 2.nplanes = 2:we get y addr from buf.m.planes[0] ,get uv addr from buf.m.planes[1]...
    * 3.nplanes = 3:we get y addr from buf.m.planes[0] ,get u addr from buf.m.planes[1],get v addr from buf.m.planes[2]...
    */
    unsigned int nPlanes;

    // actually buffer counts
    int                                mBufferCnt;

    bool                            mUseHwEncoder;

    Rect                            mRectCrop;
    Rect                            mThumbRectCrop;
    int                                mNewZoom;
    int                                mLastZoom;
    int                                mMaxZoom;
    int                             mZoomRatio;

    int                                mCaptureFormat;        // the driver capture format
    int                                mVideoFormat;        // the user request format, it should convert video buffer format
                                                        // if mVideoFormat is different from mCaptureFormat.
    int                               mExposureBias;
    typedef struct bufferManagerQ_t
    {
        unsigned long            buf_vir_addr[NB_BUFFER];
        unsigned long            buf_phy_addr[NB_BUFFER];
        int            write_id;
        int            read_id;
        int            buf_unused;
        int            nShareBufFd[NB_BUFFER];
        int            nDmaBufFd[NB_BUFFER];
    }bufferManagerQ_t;

    typedef struct bufferManagerQD_t {
        unsigned long            buf_vir_addr[NB_BUFFER * 2];
        unsigned long            buf_phy_addr[NB_BUFFER * 2];
        int            write_id;
        int            read_id;
        int            buf_unused;
        int            nShareBufFd[NB_BUFFER * 2];
        int            nDmaBufFd[NB_BUFFER * 2];
    } bufferManagerQD_t;

    bufferManagerQ_t                mVideoBuffer;        // for usb camera

#ifdef USE_MP_CONVERT
    int                             mG2DHandle;
#endif
    OSAL_QUEUE                        mQueueBufferPreview;
    OSAL_QUEUE                        mQueueBufferPicture;
    V4L2BUF_t                        mV4l2buf[NB_BUFFER];

    sp<DoCaptureThread>                mCaptureThread;
    pthread_mutex_t                 mCaptureMutex;
    pthread_cond_t                    mCaptureCond;

    sp<DoPreviewThread>                mPreviewThread;
    pthread_mutex_t                 mPreviewMutex;
    pthread_cond_t                    mPreviewCond;
    pthread_mutex_t                    mPreviewSyncMutex;
    pthread_cond_t                    mPreviewSyncCond;

    sp<DoPictureThread>                mPictureThread;
    pthread_mutex_t                 mPictureMutex;
    pthread_cond_t                    mPictureCond;

    pthread_mutex_t                 mConnectMutex;
    pthread_cond_t                    mConnectCond;
    bool                            mCanBeDisconnected;

    sp<DoContinuousPictureThread>    mContinuousPictureThread;
    pthread_mutex_t                 mContinuousPictureMutex;
    pthread_cond_t                    mContinuousPictureCond;
    bool                            mContinuousPictureStarted;
    int                                mContinuousPictureCnt;
    int                                mContinuousPictureMax;
    nsecs_t                         mContinuousPictureStartTime;

    sp<DoSmartPictureThread>        mSmartPictureThread;
    pthread_mutex_t                 mSmartPictureMutex;
    pthread_cond_t                    mSmartPictureCond;
    bool                            mSmartPictureDone;
    int64_t                         mStartSmartTimeMs;
    bool                            mStartSmartTimeout;
    /* Timestamp (abs. microseconds) when last frame has been pushed to the
     * preview window. */
    uint64_t                        mContinuousPictureLast;

    /* Preview frequency in microseconds. */
    uint32_t                        mContinuousPictureAfter;

    uint64_t                        mFaceDectectLast;
    uint32_t                        mFaceDectectAfter;

    uint64_t                        mPreviewLast;
    uint32_t                        mPreviewAfter;
    V4L2BUF_t *                        mCurrentV4l2buf;

    bool                            mVideoHint;
#define STATISICS_CNT    60
    int                                mCurAvailBufferCnt;
    int                                mStatisicsIndex;
    bool                            mNeedHalfFrameRate;
    bool                            mIsThumbUsedForVideo;
    int                                mVideoWidth;            // for cts
    int                                mVideoHeight;
    IMAGE_FLAG_t                    mTakePictureFlag;
    int                                mFlashMode;
    ISceneMode*                        mSceneMode;
    SceneModeFactory                mSceneModeFactory;
    int releaseIndex; //handle index for encoder callback to release
#ifdef CAMERA_MANAGER_ENABLE
    CameraManager *mCameraManager;
#endif

#ifdef USE_DEINTERLACE_HW
    DiProcess                        *mDiProcess;
    int                               mPreFrameIndex;
    int                               mPrePreFrameIndex;
    bufferManagerQD_t                 mDiOutPutBuffer;

public:
    int                               mEnableYUV422;

private:
    V4L2BUF_t                         mV4l2DiBuf[NB_BUFFER];
#endif

    AWIspApi                          *mAWIspApi;
    int                               mIspId;

#ifdef CAMERA_DECODE
    CameraDecoder                     *mCameraDecoder;
#endif
    int                             mPicCnt;
    uint64_t                        mOrgTime;
    int                             mDebugLogCnt;
};

}; /* namespace android */

#endif  /* __HAL_V4L2_CAMERA_DEVICE_H__ */
