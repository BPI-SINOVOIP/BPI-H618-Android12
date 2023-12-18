#include "CameraDebug.h"
#if DBG_CAMERA_HARDWARE
#define LOG_NDEBUG 0
#endif
#define LOG_TAG "CameraHardware"
#include <cutils/log.h>

#include <cutils/properties.h>
#include <ui/Rect.h>

//#include <drv_display.h>
#include <stdlib.h>
#include <stdio.h>
#include "CameraHardware2.h"
#include "V4L2CameraDevice2.h"
#include "PreviewWindow.h"
#include "CallbackNotifier.h"
#ifdef __CEDARX_FRAMEWORK_2__
#include "vencoder.h"
#include "MetadataBufferType.h"
#endif

#define BASE_ZOOM    0

namespace android {

// defined in HALCameraFactory.cpp
extern void getCallingProcessName(char *name);

#if DEBUG_PARAM
/* Calculates and logs parameter changes.
 * Param:
 *  current - Current set of camera parameters.
 *  new_par - String representation of new parameters.
 */
static void PrintParamDiff(const CameraParameters& current, const char* new_par);
#else
#define PrintParamDiff(current, new_par)   (void(0))
#endif  /* DEBUG_PARAM */

/* A helper routine that adds a value to the camera parameter.
 * Param:
 *  param - Camera parameter to add a value to.
 *  val - Value to add.
 * Return:
 *  A new string containing parameter with the added value on success, or NULL on
 *  a failure. If non-NULL string is returned, the caller is responsible for
 *  freeing it with 'free'.
 */
static char* AddValue(const char* param, const char* val);

static int faceNotifyCb(int cmd, void * data, void * user)
{
    CameraHardware* camera_hw = (CameraHardware *)user;
    int len = 0;
    int width, height;
#if DBG_CAMERA_HARDWARE
    FILE *fd;
    int oriention;
    int old_oriention=-1,new_oriention=0;
    char buf[10];
    char path[128];
    HALCameraInfo *halinfo;
#endif
    camera_frame_metadata_t *result = (camera_frame_metadata_t*)data;

    switch (cmd)
    {
        case FACE_NOTITY_CMD_REQUEST_FRAME:
            width = 0;
            height = 0;
            camera_hw->getCurrentFaceFrame(data, &width, &height);
            len = width*height;
#if DBG_CAMERA_HARDWARE
            LOGV("width: %d,height: %d\n",width,height);
            camera_hw->getCurrentOriention(&oriention);
            new_oriention = oriention;

            if(new_oriention!=old_oriention) {
                sprintf(buf, "%d", new_oriention);
                strcpy(path , "/data/camera/");
                strcat(path, buf);
                halinfo = camera_hw->get_halinfo();
                if(halinfo->facing == CAMERA_FACING_BACK)
                    strcat(path,"back.bin");
                else
                    strcat(path,"front.bin");
                saveframe(path, data, len, 1);
                old_oriention = new_oriention;
            }
#endif
            if (len >0)
            {
                memset(camera_hw->mFrameData, 0, width*height);
                if (len < 1024*1024*4)
                    memcpy(camera_hw->mFrameData, (char *)data, len);
                else
                    ALOGD("Be careful: frame buffer size > 4*1024*1024");
                return 0;
            }else {
                return -1;
            }
        case FACE_NOTITY_CMD_RESULT:
            pthread_mutex_lock(&camera_hw->mFaceDetectionMutex);
            camera_hw->mFrameFaceData.facePositions = &camera_hw->mFacePosition;
            memcpy(camera_hw->mFrameFaceData.frameData, camera_hw->mFrameData, camera_hw->mPreviewWidth*camera_hw->mPreviewHeight);
            camera_hw->mFrameFaceData.faceNum = result->number_of_faces;
            // rotation angle
            //camera_hw->mFrameFaceData.angle = camera_hw->mPreviewRotation;
            camera_hw->getCurrentOriention((int*)(&camera_hw->mFrameFaceData.angle),180, 1, 0);
            // preview size
            camera_hw->mFrameFaceData.frameWidth = camera_hw->mPreviewWidth;
            camera_hw->mFrameFaceData.frameHeight = camera_hw->mPreviewHeight;
#if 0
            saveframe("/data/camera/smile.bin", camera_hw->mFrameFaceData.frameData, camera_hw->mPreviewWidth*camera_hw->mPreviewHeight, 1);
            fd = fopen("/data/camera/smile.data","wt");
            fprintf(fd,"faceTopLeftX=%d\n",camera_hw->mFacePosition.faceTopLeftX);
            fprintf(fd,"faceTopLeftX=%d\n",camera_hw->mFacePosition.faceTopLeftY);
            fprintf(fd,"faceTopLeftX=%d\n",camera_hw->mFacePosition.faceWidth);
            fprintf(fd,"faceTopLeftX=%d\n",camera_hw->mFacePosition.faceHeigth);
            fprintf(fd,"number_of_faces=%d\n",result->number_of_faces);
            fprintf(fd,"angle=%d\n",camera_hw->mFrameFaceData.angle);
            fprintf(fd,"frameWidth=%d\n",camera_hw->mPreviewWidth);
            fprintf(fd,"frameHeight=%d\n",camera_hw->mPreviewHeight);
            fclose(fd);
#endif
            pthread_mutex_unlock(&camera_hw->mFaceDetectionMutex);
            return camera_hw->faceDetection((camera_frame_metadata_t*)data);

        case FACE_NOTITY_CMD_POSITION:
            {
                FocusArea_t * pdata = (FocusArea_t *)data;
                camera_hw->mFacePosition.faceTopLeftX = pdata->x;
                camera_hw->mFacePosition.faceTopLeftY= pdata->y;
                camera_hw->mFacePosition.faceWidth= pdata->x1 - pdata->x;
                camera_hw->mFacePosition.faceHeigth= pdata->y1 - pdata->y;
                char face_area[128];
                sprintf(face_area, "(%d, %d, %d, %d, 1)",
                        pdata->x, pdata->y, pdata->x1, pdata->y1);
                return camera_hw->parse_focus_areas(face_area, true);
            }
        case FACE_NOTITY_CMD_REQUEST_ORIENTION:
            camera_hw->getCurrentOriention((int*)data);
            break;
        default:
            break;
    }
    return 0;
}
/*static int smileNotifyCb(int cmd, void * data, void * user)
{
    CameraHardware* camera_hw = (CameraHardware *)user;
    //camera_face_smile_status_t mSmile;
    struct Status * mStatus = (struct Status *) data;
    mSmile.number_of_smiles = mStatus->num;
    mSmile.smiles = mStatus->sta;
    switch (cmd)
    {
        case SMILE_NOTITY_CMD_RESULT:

            return camera_hw->smileDetection(&mSmile);

        default:
            break;
    }

    return 0;
}*/

/*static int blinkNotifyCb(int cmd, void * data, void * user)
{
    CameraHardware* camera_hw = (CameraHardware *)user;
    camera_face_blink_status_t mBlink;
    struct Status * mStatus = (struct Status *) data;
    mBlink.number_of_blinks= mStatus->num;
    mBlink.blinks= mStatus->sta;

    switch (cmd)
    {
        case EYE_BLINK_NOTITY_CMD_RESULT:

            return camera_hw->blinkDetection(&mBlink);

        default:
            break;
    }

    return 0;
}*/

static int ApperceiveNotifyCb(int cmd, void * data, void * user)
{
    CameraHardware* camera_hw = (CameraHardware *)user;
    long result;
    switch (cmd)
    {
        case APPERCEIVEPEOPLE_NOTITY_CMD_REQUEST_FRAME:
            ALOGV("notify request frame");
            break;

        /*case APPERCEIVEPEOPLE_NOTITY_CMD_RESULT:
            result = *((long*)data);
            ALOGD("notify result %ld", result);
            return camera_hw->smartDetection(result);*/

        case APPERCEIVEPEOPLE_NOTITY_CMD_POSITION:
            ALOGV("notify position");
            break;
        case APPERCEIVEPEOPLE_NOTITY_CMD_REQUEST_ORIENTION:
            ALOGV("notify request orientation");
            camera_hw->getHWOrientionInfo(data);
            //LOGV("-------info->scree_oriention %d",((struct APPERCEIVEPEOPLE_INFO*)data)->scree_oriention);
            //LOGV("+++++++info->buffer_oriention %d",((struct APPERCEIVEPEOPLE_INFO*)data)->buffer_oriention);
            break;

        default:
            break;
    }

    return 0;
}

// Parse string like "640x480" or "10000,20000"
static int parse_pair(const char *str, int *first, int *second, char delim,
                      char **endptr = NULL)
{
    // Find the first integer.
    char *end;
    int w = (int)strtol(str, &end, 10);
    // If a delimeter does not immediately follow, give up.
    if (*end != delim) {
        LOGE("Cannot find delimeter (%c) in str=%s", delim, str);
        return -1;
    }

    // Find the second integer, immediately after the delimeter.
    int h = (int)strtol(end+1, &end, 10);

    *first = w;
    *second = h;

    if (endptr) {
        *endptr = end;
    }

    return 0;
}

CameraHardware::CameraHardware(struct hw_module_t* module, CCameraConfig* pCameraCfg)
        : mPreviewWindow(),
          mCallbackNotifier(),
          mCameraConfig(pCameraCfg),
          mIsCameraIdle(true),
          mFirstSetParameters(true),
          mFullSizeWidth(0),
          mFullSizeHeight(0),
          mCaptureWidth(0),
          mCaptureHeight(0),
          mVideoCaptureWidth(0),
          mVideoCaptureHeight(0),
          mUseHwEncoder(false),
          mFaceDetection(NULL),
          mSmileDetection(NULL),
          mBlinkDetection(NULL),
          mSmartDetection(NULL),
          mFocusStatus(FOCUS_STATUS_IDLE),
          mSmileDetectionState(FACE_DETECTION_UNINITIALIZED),
          mSmileDetectionEnable(false),
		  mBlinkDetectionState(FACE_DETECTION_UNINITIALIZED),
          mBlinkDetectionEnable(false),
          mSmartDetectionEnable(false),
          mSmartMode(0x01),
          mIsSingleFocus(false),
		  mSmartDiscardFrameNum(0),
          mOriention(0),
          mIsSupportFocus(false),
		  mZoomRatio(100),
          mIsSupportEffect(false),
          mIsSupportFlash(false),
          mIsSupportScene(false),
          mIsSupportWhiteBlance(false),
          mIsSupportExposure(false),
          mAutoFocusThreadExit(true),
          mFrameData(NULL),
		  mFaceDetectionThreadExit(true),
          mSmartData(NULL),
		  mIsImageCaptureIntent(false),
          mPreviewRotation(0),
          mPreviewWidth(0),
          mPreviewHeight(0),
          mStartPreviewFlag(false),
          mSmileDetectionCmdEnable(false),
          mBlinkDetectionCmdEnable(false),
          mBlinkPictureStarted(false),
          mBlinkPictureResult(false),
          mSmilePictureResult(false),
          mSmartThreadExit(true)
{
    /*
     * Initialize camera_device descriptor for this object.
     */
    F_LOG;

    /* Common header */
    common.tag = HARDWARE_DEVICE_TAG;
    common.version = 0;
    common.module = module;
    common.close = CameraHardware::close;

    /* camera_device fields. */
    ops = &mDeviceOps;
    priv = this;

    // instance V4L2CameraDevice object
    mV4L2CameraDevice = new V4L2CameraDevice(this, &mPreviewWindow, &mCallbackNotifier);
    if (mV4L2CameraDevice == NULL)
    {
        LOGE("Failed to create V4L2Camera instance");
        return ;
    }
    memset((void*)mFDOriention,0,sizeof(mFDOriention));
    memset((void*)mCallingProcessName, 0, sizeof(mCallingProcessName));
    memset(&mHalCameraInfo,0,sizeof(mHalCameraInfo));
    memset(&mFrameRectCrop, 0, sizeof(mFrameRectCrop));
    memset((void*)mFocusAreasStr, 0, sizeof(mFocusAreasStr));
    memset((void*)&mLastFocusAreas, 0, sizeof(mLastFocusAreas));
    memset(&mFrameFaceData, 0, sizeof(FrameFaceData));
    memset(&mFacePosition, 0, sizeof(FacePosition));

    // init command queue
    OSAL_QueueCreate(&mQueueCommand, CMD_QUEUE_MAX);
    memset((void*)mQueueElement, 0, sizeof(mQueueElement));

    // init command thread
    pthread_mutex_init(&mCommandMutex, NULL);
    pthread_cond_init(&mCommandCond, NULL);
    mCommandThread = new DoCommandThread(this);
    mCommandThread->startThread();

    // init auto focus thread
    pthread_mutex_init(&mAutoFocusMutex, NULL);
    pthread_cond_init(&mAutoFocusCond, NULL);
    mAutoFocusThread = new DoAutoFocusThread(this);
#ifdef __OPEN_FACEDECTION__
    // init face detection thread
    pthread_mutex_init(&mFaceDetectionMutex, NULL);
    pthread_mutex_init(&mFaceDetectionStateMutex, NULL);
    pthread_cond_init(&mFaceDetectionCond, NULL);
    mFaceDetectionThread = new DoFaceDetectionThread(this);
#endif

#ifdef __OPEN_APPERCEIVEPEOPLE__
    // init smart thread
    pthread_mutex_init(&mSmartMutex, NULL);
    pthread_cond_init(&mSmartCond, NULL);
    mSmartThread = new DoSmartThread(this);
#endif
}

CameraHardware::~CameraHardware()
{
    if (mCommandThread != NULL)
    {
        mCommandThread->stopThread();
        pthread_cond_signal(&mCommandCond);
        mCommandThread.clear();
        mCommandThread = 0;
    }

    pthread_mutex_destroy(&mCommandMutex);
    pthread_cond_destroy(&mCommandCond);
    OSAL_QueueTerminate(&mQueueCommand);

    if (mAutoFocusThread != NULL)
    {
        mAutoFocusThread.clear();
        mAutoFocusThread = 0;
    }

    pthread_mutex_destroy(&mAutoFocusMutex);
    pthread_cond_destroy(&mAutoFocusCond);

#ifdef __OPEN_FACEDECTION__

    if (mFaceDetectionThread != NULL)
    {
        mFaceDetectionThread.clear();
        mFaceDetectionThread = 0;
    }

    pthread_mutex_destroy(&mFaceDetectionMutex);
    pthread_mutex_destroy(&mFaceDetectionStateMutex);
    pthread_cond_destroy(&mFaceDetectionCond);
#endif

#ifdef __OPEN_APPERCEIVEPEOPLE__
    if (mSmartThread != NULL)
    {
        mSmartThread.clear();
        mSmartThread = 0;
    }

    pthread_mutex_destroy(&mSmartMutex);
    pthread_cond_destroy(&mSmartCond);


    if (mSmartDetection != NULL)
    {
        DestroyApperceivePeopleDev(mSmartDetection);
        mSmartDetection = NULL;
    }

    if (mSmartData != NULL)
    {
        free(mSmartData);
        mSmartData = NULL;
    }
#endif

    if (mV4L2CameraDevice != NULL)
    {
        delete mV4L2CameraDevice;
        mV4L2CameraDevice = NULL;
    }
}

bool CameraHardware::autoFocusThread()
{
    bool ret = true;
    int status = -1;
    FocusStatus new_status = FOCUS_STATUS_IDLE;

    usleep(30000);
/*
    const char * valstr = mParameters.get(CameraParameters::KEY_RECORDING_HINT);
    if(valstr != NULL
        && strcmp(valstr, CameraParameters::TRUE) == 0)
    {
        // return true;        // for cts
    }
*/
    bool timeout = false;
    int64_t lastTimeMs = systemTime() / 1000000;

    pthread_mutex_lock(&mAutoFocusMutex);
    if (mAutoFocusThread->getThreadStatus() == THREAD_STATE_EXIT)
    {
        LOGD("autoFocusThread exited");
        ret = false;        // exit thread
        pthread_mutex_unlock(&mAutoFocusMutex);
        goto FOCUS_THREAD_EXIT;
    }
    mAutoFocusThreadExit = false;
    pthread_mutex_unlock(&mAutoFocusMutex);

    if (mIsSingleFocus)
    {
        while(1)
        {
            // if hw af ok
            status = mV4L2CameraDevice->getAutoFocusStatus();
            if ( status == V4L2_AUTO_FOCUS_STATUS_REACHED)
            {
                LOGD("auto focus ok, use time: %lld(ms)", systemTime() / 1000000 - lastTimeMs);
                break;
            }
            else if (status == V4L2_AUTO_FOCUS_STATUS_BUSY
                        || status == V4L2_AUTO_FOCUS_STATUS_IDLE)
            {
                if ((systemTime() / 1000000 - lastTimeMs) > 2000)    // 2s timeout
                {
                    LOGW("auto focus time out, ret = %d", status);
                    timeout = true;
                    break;
                }
                //LOGV("wait auto focus ......");
                usleep(30000);
            }
            else if (status == V4L2_AUTO_FOCUS_STATUS_FAILED)
            {
                LOGW("auto focus failed");
                break;
            }
            else if (status < 0)
            {
                LOGE("auto focus interface error");
                break;
            }
        }

        mCallbackNotifier.autoFocusMsg(status == V4L2_AUTO_FOCUS_STATUS_REACHED);

        if (status == V4L2_AUTO_FOCUS_STATUS_REACHED)
        {
            mV4L2CameraDevice->set3ALock(V4L2_LOCK_FOCUS);
        }

        mIsSingleFocus = false;

        mFocusStatus = FOCUS_STATUS_DONE;
    }
    else
    {
        status = mV4L2CameraDevice->getAutoFocusStatus();

        if (status == V4L2_AUTO_FOCUS_STATUS_BUSY)
        {
            new_status = FOCUS_STATUS_BUSY;
        }
        else if (status == V4L2_AUTO_FOCUS_STATUS_REACHED)
        {
            new_status = FOCUS_STATUS_SUCCESS;
        }
        else if (status == V4L2_AUTO_FOCUS_STATUS_FAILED)
        {
            new_status = FOCUS_STATUS_FAIL;
        }
        else if (status == V4L2_AUTO_FOCUS_STATUS_IDLE)
        {
            // do nothing
            new_status = FOCUS_STATUS_IDLE;
        }
        else if ((unsigned int)status == 0xFF000000)
        {
            LOGV("getAutoFocusStatus, status = 0xFF000000");
            ret = false;        // exit thread
            goto FOCUS_THREAD_EXIT;
        }
        else
        {
            LOGW("unknow focus status: %d", status);
            ret = true;
            goto FOCUS_THREAD_EXIT;
        }

        // LOGD("status change, old: %d, new: %d", mFocusStatus, new_status);

        if (mFocusStatus == new_status)
        {
            ret = true;
            goto FOCUS_THREAD_EXIT;
        }

        if ((mFocusStatus == FOCUS_STATUS_IDLE
                || mFocusStatus & FOCUS_STATUS_DONE)
            && new_status == FOCUS_STATUS_BUSY)
        {
            // start focus
            LOGV("start focus, %d -> %d", mFocusStatus, new_status);
            // show focus animation
            mCallbackNotifier.autoFocusContinuousMsg(true);
        }
        else if (mFocusStatus == FOCUS_STATUS_BUSY
                && (new_status & FOCUS_STATUS_DONE))
        {
            // focus end
            LOGV("focus %s, %d -> %d", (new_status == FOCUS_STATUS_SUCCESS)?"ok":"fail", mFocusStatus, new_status);
            mCallbackNotifier.autoFocusContinuousMsg(false);
        }
        else if ((mFocusStatus & FOCUS_STATUS_DONE)
                && new_status == FOCUS_STATUS_IDLE)
        {
            // focus end
            LOGV("do nothing, %d -> %d", mFocusStatus, new_status);
            mV4L2CameraDevice->setAutoFocusStop();
            //ret = true;
        }
        else
        {
            LOGW("unknow status change, %d -> %d", mFocusStatus, new_status);
        }

        mFocusStatus = new_status;
    }

FOCUS_THREAD_EXIT:
    if (ret == false)
    {
        pthread_mutex_lock(&mAutoFocusMutex);
        mAutoFocusThreadExit = true;
        pthread_cond_signal(&mAutoFocusCond);
        pthread_mutex_unlock(&mAutoFocusMutex);
    }
    return ret;
}

bool CameraHardware::smartThread()
{
    bool result = true;
    int width, height;

    pthread_mutex_lock(&mSmartMutex);
    if (mSmartThread->getThreadStatus() == THREAD_STATE_EXIT)
    {
        LOGD("mSmartThread exited");
        result = false;        // exit thread
        pthread_mutex_unlock(&mSmartMutex);
        goto SMART_THREAD_EXIT;
    }
    mSmartThreadExit = false;
    pthread_mutex_unlock(&mSmartMutex);
    {
        LOGV("mSmartThread no msg, sleep...");
        //pthread_mutex_lock(&mSmartMutex);
        //pthread_cond_wait(&mSmartCond, &mSmartMutex);
        //pthread_mutex_unlock(&mSmartMutex);
    }

    if (mSmartDetectionEnable)
    {
        //memset(mSmartData, 0, 1920*1080);
        pthread_mutex_lock(&mSmartMutex);
        int ret = getCurrentFaceFrame(mSmartData, &width, &height);
        pthread_mutex_unlock(&mSmartMutex);

        if(ret != 0)
        {
            ALOGD("get current face frame failed!!!");
            //smartDetection(0);
            usleep(500*1000);
        }else {
            #if 1
            if (mSmartDiscardFrameNum >0)
            {
                mSmartDiscardFrameNum--;
                usleep(100*1000);
                return result;
            }
            #endif
            pthread_mutex_lock(&mSmartMutex);
            ALOGV("APPERCEIVEPEOPLE_OPS_CMD_START");
            if (mSmartDetection != NULL)
            {
                ret = mSmartDetection->ioctrl(mSmartDetection, APPERCEIVEPEOPLE_OPS_CMD_START, width, height, mSmartData, mSmartMode);
                if(ret < 0)
                {
                    //smartDetection(0);
                    ALOGD("smart detection failed!!!");
                }
            }
            else
            {
                //smartDetection(0);
                ALOGW("APPERCEIVEPEOPLE_OPS_CMD_START failed, mSmartDetection not opened.");
            }
            pthread_mutex_unlock(&mSmartMutex);
            usleep(500*1000);
        }
    }else {
        usleep(2000*1000);
    }

SMART_THREAD_EXIT:
    if (result == false)
    {
        pthread_mutex_lock(&mSmartMutex);
        mSmartThreadExit = true;
        pthread_cond_signal(&mSmartCond);
        pthread_mutex_unlock(&mSmartMutex);
    }
    return result;
}


bool CameraHardware::faceDetectionThread()
{
    bool ret = true;

    pthread_mutex_lock(&mFaceDetectionMutex);
    if (mFaceDetectionThread->getThreadStatus() == THREAD_STATE_EXIT)
    {
        LOGD("faceDetectionThread exited");
        ret = false;        // exit thread
        pthread_mutex_unlock(&mFaceDetectionMutex);
        goto FACE_DETECTION_THREAD_EXIT;
    }
    mFaceDetectionThreadExit = false;
    pthread_mutex_unlock(&mFaceDetectionMutex);

    {
        LOGD("faceDetectionThread no msg, sleep...");
        pthread_mutex_lock(&mFaceDetectionMutex);
        pthread_cond_wait(&mFaceDetectionCond, &mFaceDetectionMutex);
        pthread_mutex_unlock(&mFaceDetectionMutex);
        //return true;
    }

    pthread_mutex_lock(&mFaceDetectionMutex);

    if (mSmileDetectionEnable == true)
    {
        LOGV("SMILE_OPS_CMD_START");
        if (mSmileDetection != 0)
        {
            pthread_mutex_lock(&mFaceDetectionStateMutex);
            mSmileDetectionState = FACE_DETECTION_STARTED;
            pthread_mutex_unlock(&mFaceDetectionStateMutex);

            mSmileDetection->ioctrl(mSmileDetection, SMILE_OPS_CMD_START, 0, &mFrameFaceData);
        }
        else
        {
            LOGW("SMILE_OPS_CMD_START failed, mSmileDetection not opened.");
        }
    }

    pthread_mutex_unlock(&mFaceDetectionMutex);

#if __OPEN_BLINKDECTION__

    pthread_mutex_lock(&mFaceDetectionMutex);

    if (mBlinkDetectionEnable == true)
    {
        LOGV("EYE_BLINK_OPS_CMD_START");
        if (mBlinkDetection != 0)
        {
            pthread_mutex_lock(&mFaceDetectionStateMutex);
            mBlinkDetectionState = FACE_DETECTION_STARTED;
            pthread_mutex_unlock(&mFaceDetectionStateMutex);

            mBlinkDetection->ioctrl(mBlinkDetection, EYE_BLINK_OPS_CMD_START, 0, &mFrameFaceData);
        }
        else
        {
            LOGW("EYE_BLINK_OPS_CMD_START failed, mBlinkDetection not opened.");
        }
    }
    pthread_mutex_unlock(&mFaceDetectionMutex);

#endif

FACE_DETECTION_THREAD_EXIT:
    if (ret == false)
    {
        pthread_mutex_lock(&mFaceDetectionMutex);
        mFaceDetectionThreadExit = true;
        pthread_cond_signal(&mFaceDetectionCond);
        pthread_mutex_unlock(&mFaceDetectionMutex);
    }
    return ret;
}
bool CameraHardware::commandThread()
{
    pthread_mutex_lock(&mCommandMutex);
    if (mCommandThread->getThreadStatus() == THREAD_STATE_EXIT)
    {
        LOGD("commandThread exited");
        pthread_mutex_unlock(&mCommandMutex);
        return false;
    }
    pthread_mutex_unlock(&mCommandMutex);

    Queue_Element * queue = (Queue_Element *)OSAL_Dequeue(&mQueueCommand);
    if (queue == NULL)
    {
        pthread_mutex_lock(&mCommandMutex);
        LOGV("wait commond queue ......");
        pthread_cond_wait(&mCommandCond, &mCommandMutex);
        pthread_mutex_unlock(&mCommandMutex);
        return true;
    }

    V4L2CameraDevice* pV4L2Device = mV4L2CameraDevice;
    int cmd = queue->cmd;
    switch(cmd)
    {
        case CMD_QUEUE_SET_COLOR_EFFECT:
        {
            unsigned long new_image_effect = queue->data;
            LOGV("CMD_QUEUE_SET_COLOR_EFFECT: %ld", new_image_effect);

            if (pV4L2Device->setImageEffect(new_image_effect) < 0)
            {
                LOGE("ERR(%s):Fail on mV4L2Camera->setImageEffect(effect(%ld))", __FUNCTION__, new_image_effect);
            }
            break;
        }
        case CMD_QUEUE_SET_WHITE_BALANCE:
        {
            unsigned long new_white = queue->data;
            LOGV("CMD_QUEUE_SET_WHITE_BALANCE: %ld", new_white);

            if (pV4L2Device->setWhiteBalance(new_white) < 0)
            {
                LOGE("ERR(%s):Fail on mV4L2Camera->setWhiteBalance(white(%ld))", __FUNCTION__, new_white);
            }
            break;
        }
        case CMD_QUEUE_SET_EXPOSURE_COMPENSATION:
        {
            unsigned long new_exposure_compensation = queue->data;
            LOGV("CMD_QUEUE_SET_EXPOSURE_COMPENSATION: %ld", new_exposure_compensation);

            if (pV4L2Device->setExposureCompensation(new_exposure_compensation) < 0)
            {
                LOGE("ERR(%s):Fail on mV4L2Camera->setBrightness(brightness(%ld))", __FUNCTION__, new_exposure_compensation);
            }
            break;
        }
        case CMD_QUEUE_SET_FLASH_MODE:
        {
            unsigned long new_flash = queue->data;
            LOGD("CMD_QUEUE_SET_FLASH_MODE: %ld", new_flash);
            if (pV4L2Device->setFlashMode(new_flash) < 0)
            {
              LOGE("ERR(%s):Fail on mV4L2Camera->setFlashMode(flash(%ld))", __FUNCTION__, new_flash);
            }
            break;
        }
        case CMD_QUEUE_SET_FOCUS_MODE:
        {
            LOGV("CMD_QUEUE_SET_FOCUS_MODE");
            if(setAutoFocusRange() != OK)
            {
                LOGE("unknown focus mode");
               }
            break;
        }
        case CMD_QUEUE_SET_FOCUS_AREA:
        {
            char * new_focus_areas_str = (char *)queue->data;
            if (new_focus_areas_str != NULL)
            {
                LOGV("CMD_QUEUE_SET_FOCUS_AREA: %s", new_focus_areas_str);
                parse_focus_areas(new_focus_areas_str);
            }
            break;
        }
        case CMD_QUEUE_START_FACE_DETECTE:
        {
            int width = 0, height = 0;
            LOGV("CMD_QUEUE_START_FACE_DETECTE");
            if (mHalCameraInfo.fast_picture_mode)
            {
                pV4L2Device->getThumbSize(&width, &height);
            }
            else
            {
                const char * preview_capture_width_str = mParameters.get(KEY_PREVIEW_CAPTURE_SIZE_WIDTH);
                const char * preview_capture_height_str = mParameters.get(KEY_PREVIEW_CAPTURE_SIZE_HEIGHT);
                if (preview_capture_width_str != NULL
                    && preview_capture_height_str != NULL)
                {
                    width  = atoi(preview_capture_width_str);
                    height = atoi(preview_capture_height_str);
                }
            }

            if (mFaceDetection != 0)
            {
                int mRotation = mParameters.getInt(CameraParameters::KEY_ROTATION);
                if (0 <= mRotation)
                {
                    mPreviewRotation = mRotation;
                }
                mPreviewWidth = width;
                mPreviewHeight = height;
                LOGV("start facedetection size: %dx%d", width, height);
                mFaceDetection->ioctrl(mFaceDetection, FACE_OPS_CMD_START, width, height);
            }
            else
            {
                LOGW("CMD_QUEUE_START_FACE_DETECTE failed, mFaceDetection not opened.");
            }
            break;
        }
        case CMD_QUEUE_STOP_FACE_DETECTE:
        {
            LOGV("CMD_QUEUE_STOP_FACE_DETECTE");
            if (mFaceDetection != 0)
            {
                mFaceDetection->ioctrl(mFaceDetection, FACE_OPS_CMD_STOP, 0, 0);
            }
            else
            {
                LOGW("CMD_QUEUE_STOP_FACE_DETECTE failed, mFaceDetection not opened.");
            }
            break;
        }
        case CMD_QUEUE_START_SMILE_DETECTE:
        {
            LOGV("CMD_QUEUE_START_SMILE_DETECTE");
            if (mSmileDetection != 0)
            {
                mSmileDetection->ioctrl(mSmileDetection, SMILE_OPS_CMD_START, 0, &mFrameFaceData);
            }
            else
            {
                LOGW("CMD_QUEUE_START_SMILE_DETECTE failed, mSmileDetection not opened.");
            }
            break;
        }
        case CMD_QUEUE_STOP_SMILE_DETECTE:
        {
            LOGV("CMD_QUEUE_STOP_SMILE_DETECTE");
            if (mSmileDetection != 0)
            {
                mSmileDetection->ioctrl(mSmileDetection, SMILE_OPS_CMD_STOP, 0, 0);
            }
            else
            {
                LOGW("CMD_QUEUE_STOP_SMILE_DETECTE failed, mSmileDetection not opened.");
            }
            break;
        }
        case CMD_QUEUE_START_BLINK_DETECTE:
        {
            LOGV("CMD_QUEUE_START_BLINK_DETECTE");
            if (mBlinkDetection != 0)
            {
                mBlinkDetection->ioctrl(mBlinkDetection, EYE_BLINK_OPS_CMD_START, 0, &mFrameFaceData);
            }
            else
            {
                LOGW("CMD_QUEUE_START_BLINK_DETECTE failed, mBlinkDetection not opened.");
            }
            break;
        }
        case CMD_QUEUE_STOP_BLINK_DETECTE:
        {
            LOGV("CMD_QUEUE_STOP_BLINK_DETECTE");
            if (mBlinkDetection != 0)
            {
                mBlinkDetection->ioctrl(mBlinkDetection, EYE_BLINK_OPS_CMD_STOP, 0, 0);
            }
            else
            {
                LOGW("CMD_QUEUE_STOP_BLINK_DETECTE failed, mBlinkDetection not opened.");
            }
            break;
        }
        case CMD_QUEUE_TAKE_PICTURE:
        {
            LOGD("CMD_QUEUE_TAKE_PICTURE");
            doTakePicture();
            break;
        }
        case CMD_QUEUE_PICTURE_MSG:
        {
            LOGV("CMD_QUEUE_PICTURE_MSG");
            void * frame = (void *)queue->data;
            mCallbackNotifier.notifyPictureMsg(frame);
            if (strcmp(mCallingProcessName, "com.android.cts.stub") != 0
                && strcmp(mCallingProcessName, "com.android.cts.mediastress") != 0
                && mIsImageCaptureIntent == false)
            {
                doTakePictureEnd();
            }
            break;
        }
        case CMD_QUEUE_STOP_CONTINUOUSSNAP:
        {
            cancelPicture();
            doTakePictureEnd();
            break;
        }
        case CMD_QUEUE_SET_FOCUS_STATUS:
        {
            mCallbackNotifier.autoFocusMsg(true);
            break;
        }
        default:
            LOGW("unknown queue command: %d", cmd);
            break;
    }

    return true;
}

/****************************************************************************
 * Public API
 ***************************************************************************/

status_t CameraHardware::Initialize()
{
    F_LOG;

    getCallingProcessName(mCallingProcessName);
    mCallbackNotifier.setCallingProcess(mCallingProcessName);
#ifdef __OPEN_FACEDECTION__

    if (mFaceDetection == NULL)
    {
        // create FaceDetection object
        CreateFaceDetectionDev(&mFaceDetection);
        if (mFaceDetection == NULL)
        {
            LOGE("create FaceDetection failed");
            return UNKNOWN_ERROR;
        }
    }

    mFaceDetection->ioctrl(mFaceDetection, FACE_OPS_CMD_REGISTE_USER, (int)this, 0);
    mFaceDetection->setCallback(mFaceDetection, faceNotifyCb);

    mFrameData = (unsigned char*)malloc(4*1024*1024);
    mFrameFaceData.frameData = (unsigned char*)malloc(4*1024*1024);

#endif

#ifdef __OPEN_SMILEDECTION__

    if (mSmileDetection == NULL)
    {
        // create SmileDetection object
        CreateSmileDetectionDev(&mSmileDetection);
        if (mSmileDetection == NULL)
        {
            LOGE("create SmileDetection failed");
            return UNKNOWN_ERROR;
        }
        mSmileDetectionState = FACE_DETECTION_INITIALIZED;
    }

    mSmileDetection->ioctrl(mSmileDetection, SMILE_OPS_CMD_REGISTE_USER, (int)this, 0);
    //mSmileDetection->setCallback(mSmileDetection, smileNotifyCb);

#endif

#ifdef __OPEN_BLINKDECTION__
    if (mBlinkDetection == NULL)
    {
        // create mBlinkDetection object
        CreateEyeBlinkDetectionDev(&mBlinkDetection);
        if (mBlinkDetection == NULL)
        {
            LOGE("create mBlinkDetection failed");
            return UNKNOWN_ERROR;
        }
        mBlinkDetectionState = FACE_DETECTION_INITIALIZED;
    }

    mBlinkDetection->ioctrl(mBlinkDetection, EYE_BLINK_OPS_CMD_REGISTE_USER, (int)this, 0);
    mBlinkDetection->setCallback(mBlinkDetection, blinkNotifyCb);
#endif
    initDefaultParameters();

    return NO_ERROR;
}

void CameraHardware::initDefaultParameters()
{
    CameraParameters p = mParameters;
    String8 parameterString;
    char * value;

    // version
    p.set(KEY_CAMERA_HAL_VERSION, CAMERA_HAL_VERSION);

    // facing and orientation
    if (mHalCameraInfo.facing == CAMERA_FACING_BACK)
    {
        p.set(CameraHardware::FACING_KEY, CameraHardware::FACING_BACK);
        LOGV("%s: camera is facing %s", __FUNCTION__, CameraHardware::FACING_BACK);
    }
    else
    {
        p.set(CameraHardware::FACING_KEY, CameraHardware::FACING_FRONT);
        LOGV("%s: camera is facing %s", __FUNCTION__, CameraHardware::FACING_FRONT);
    }

    p.set(CameraHardware::ORIENTATION_KEY, mHalCameraInfo.orientation);

    // exif Make and Model
    char property[PROPERTY_VALUE_MAX];
    if (property_get("ro.product.manufacturer", property, NULL) > 0)
    {
        mCallbackNotifier.setExifMake(property);
    }
    if (property_get("ro.product.model", property, NULL) > 0)
    {
        mCallbackNotifier.setExifModel(property);
    }
    //mCallbackNotifier.setExifMake(mCameraConfig->getExifMake());
    //mCallbackNotifier.setExifModel(mCameraConfig->getExifModel());


        //add for android CTS by clx
    p.set(CameraParameters::KEY_SUPPORTED_ANTIBANDING, CameraParameters::ANTIBANDING_AUTO);
    p.set(CameraParameters::KEY_ANTIBANDING, CameraParameters::ANTIBANDING_AUTO);
    p.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED, "true");

    // for USB camera
    if (mV4L2CameraDevice->mCameraType == CAMERA_TYPE_UVC ||
            mV4L2CameraDevice->mCameraType == CAMERA_TYPE_TVIN_NTSC ||
            mV4L2CameraDevice->mCameraType == CAMERA_TYPE_TVIN_PAL ||
            mV4L2CameraDevice->mCameraType == CAMERA_TYPE_TVIN_YPBPR)
    {
        // preview, picture, video size
        char sizeStr[256];
        mV4L2CameraDevice->enumSize(sizeStr, 256);
        LOGV("enum size: %s", sizeStr);
        if (strlen(sizeStr) > 0)
        {
            p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, sizeStr);
            p.set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES, sizeStr);
            p.set(CameraParameters::KEY_SUPPORTED_VIDEO_SIZES, sizeStr);

        }
        else
        {
            p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, "640x480");
            p.set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES, "640x480");
            p.set(CameraParameters::KEY_SUPPORTED_VIDEO_SIZES, "640x480");
        }
        p.set(CameraParameters::KEY_PREVIEW_SIZE, "640x480");
        p.set(CameraParameters::KEY_PICTURE_SIZE, "640x480");
        p.set(CameraParameters::KEY_VIDEO_SIZE, "640x480");

        // add for android CTS
        p.set(CameraParameters::KEY_SUPPORTED_FOCUS_MODES, CameraParameters::FOCUS_MODE_FIXED);
        p.set(CameraParameters::KEY_FOCUS_MODE, CameraParameters::FOCUS_MODE_FIXED);
        p.set(CameraParameters::KEY_FOCUS_AREAS, "(0,0,0,0,0)");
        p.set(CameraParameters::KEY_FOCAL_LENGTH, "3.43");
        mCallbackNotifier.setFocalLenght(3.43);
        p.set(CameraParameters::KEY_FOCUS_DISTANCES, "0.10,1.20,Infinity");

        // fps
        p.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, "5000,30000");				 // add temp for CTS
        p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE, "(24000,24000),(5000,30000),(30000,30000),(5000,60000)");	// add temp for CTS
        p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES, "30");
        p.set(CameraParameters::KEY_PREVIEW_FRAME_RATE, "30");
        mV4L2CameraDevice->setFrameRate(30);

        // exposure compensation
        p.set(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION, "0");
        p.set(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION, "0");
        p.set(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP, "0");
        p.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, "0");

        goto COMMOM_PARAMS;
    }

    // fast picture mode
    // if (mHalCameraInfo.fast_picture_mode)
    {
        // capture size of picture-mode preview
        // get full size from the driver, to do
        mFullSizeWidth = 2592;
        mFullSizeHeight = 1936;

        // get full size from the driver
        mV4L2CameraDevice->getFullSize(&mFullSizeWidth, &mFullSizeHeight);
        LOGV("getFullSize: %dx%d", mFullSizeWidth, mFullSizeHeight);

        // any other differences ??

    }

    // preview size
    LOGV("init preview size");
    value = mCameraConfig->supportPreviewSizeValue();
    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, value);
    LOGV("supportPreviewSizeValue: [%s] %s", CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, value);

#ifdef USE_NEW_MODE
    // KEY_SUPPORTED_VIDEO_SIZES and KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO should be set
    // at the same time, or both NULL.
    // At present, we use preview and video the same size. Next version, maybe different.
    p.set(CameraParameters::KEY_SUPPORTED_VIDEO_SIZES, value);
    p.set(CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO, "1280x720");
#endif
    p.set(CameraParameters::KEY_SUPPORTED_VIDEO_SIZES, value);
    p.set(CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO, "640x480");

    value = mCameraConfig->defaultPreviewSizeValue();
    p.set(CameraParameters::KEY_PREVIEW_SIZE, value);
    p.set(CameraParameters::KEY_VIDEO_SIZE, value);
    p.set(CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO, value);

    // picture size
    LOGV("to init picture size");
    value = mCameraConfig->supportPictureSizeValue();
    p.set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES, value);
    LOGV("supportPreviewSizeValue: [%s] %s", CameraParameters::KEY_SUPPORTED_PICTURE_SIZES, value);

    value = mCameraConfig->defaultPictureSizeValue();
    p.set(CameraParameters::KEY_PICTURE_SIZE, value);

    // frame rate
    LOGV("to init frame rate");
    value = mCameraConfig->supportFrameRateValue();
    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES, value);
    LOGV("supportFrameRateValue: [%s] %s", CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES, value);

    p.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, "5000,30000");                // add temp for CTS
    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE, "(24000,24000),(5000,30000),(5000,60000)");    // add temp for CTS

    value = mCameraConfig->defaultFrameRateValue();
    p.set(CameraParameters::KEY_PREVIEW_FRAME_RATE, value);
    mV4L2CameraDevice->setFrameRate(atoi(value));

    // focus
    LOGV("to init focus");
    if (mCameraConfig->supportFocusMode())
    {
        value = mCameraConfig->supportFocusModeValue();
        p.set(CameraParameters::KEY_SUPPORTED_FOCUS_MODES, value);
        value = mCameraConfig->defaultFocusModeValue();
        p.set(CameraParameters::KEY_FOCUS_MODE, value);
        p.set(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS,"1");
    }
    else
    {
        // add for CTS
        p.set(CameraParameters::KEY_SUPPORTED_FOCUS_MODES, CameraParameters::FOCUS_MODE_FIXED);
        p.set(CameraParameters::KEY_FOCUS_MODE, CameraParameters::FOCUS_MODE_FIXED);
    }
    p.set(CameraParameters::KEY_FOCUS_AREAS, "(0,0,0,0,0)");
    //p.set(CameraParameters::KEY_FOCAL_LENGTH, "3.43");
    //mCallbackNotifier.setFocalLenght(3.43);
    p.set(CameraParameters::KEY_FOCUS_DISTANCES, "0.10,1.20,Infinity");


    // color effect
    LOGV("to init color effect");
    if (mCameraConfig->supportColorEffect())
    {
        value = mCameraConfig->supportColorEffectValue();
        p.set(CameraParameters::KEY_SUPPORTED_EFFECTS, value);
        value = mCameraConfig->defaultColorEffectValue();
        p.set(CameraParameters::KEY_EFFECT, value);
    }

    // flash mode
    LOGV("to init flash mode");
    if (mCameraConfig->supportFlashMode())
    {
        value = mCameraConfig->supportFlashModeValue();
        p.set(CameraParameters::KEY_SUPPORTED_FLASH_MODES, value);
        value = mCameraConfig->defaultFlashModeValue();
        p.set(CameraParameters::KEY_FLASH_MODE, value);
    }

    // scene mode
    LOGV("to init scene mode");
    if (mCameraConfig->supportSceneMode())
    {
        value = mCameraConfig->supportSceneModeValue();
        p.set(CameraParameters::KEY_SUPPORTED_SCENE_MODES, value);
        value = mCameraConfig->defaultSceneModeValue();
        p.set(CameraParameters::KEY_SCENE_MODE, value);
    }

    // white balance
    LOGV("to init white balance");
    if (mCameraConfig->supportWhiteBalance())
    {
        value = mCameraConfig->supportWhiteBalanceValue();
        p.set(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE, value);
        value = mCameraConfig->defaultWhiteBalanceValue();
        p.set(CameraParameters::KEY_WHITE_BALANCE, value);
    }

    // exposure compensation
    LOGV("to init exposure compensation");
    if (mCameraConfig->supportExposureCompensation())
    {
        value = mCameraConfig->minExposureCompensationValue();
        p.set(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION, value);

        value = mCameraConfig->maxExposureCompensationValue();
        p.set(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION, value);

        value = mCameraConfig->stepExposureCompensationValue();
        p.set(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP, value);

        value = mCameraConfig->defaultExposureCompensationValue();
        p.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, value);
    }
    else
    {
    //modify for CTS by clx
        p.set(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION, "-3");
        p.set(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION, "3");
        p.set(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP, "1");
        //p.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, "1");
    }

COMMOM_PARAMS:
    // zoom
    LOGV("to init zoom");
    p.set(CameraParameters::KEY_ZOOM_SUPPORTED, "true");
    p.set(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED, "false");
    p.set(CameraParameters::KEY_MAX_ZOOM, "100");

    int max_zoom = 100;
    char zoom_ratios[1024];
    memset(zoom_ratios, 0, 1024);
    for (int i = 0; i <= max_zoom; i++)
    {
        int i_ratio = 200 * i / max_zoom + 100;
        char str[8];
        sprintf(str, "%d,", i_ratio);
        strcat(zoom_ratios, str);
    }
    int len = strlen(zoom_ratios);
    zoom_ratios[len - 1] = 0;
    LOGV("zoom_ratios: %s", zoom_ratios);
    p.set(CameraParameters::KEY_ZOOM_RATIOS, zoom_ratios);
    p.set(CameraParameters::KEY_ZOOM, "0");

    mV4L2CameraDevice->setCrop(BASE_ZOOM, max_zoom);

    mZoomRatio = 100;

    // for some apps
    if (strcmp(mCallingProcessName, "com.android.facelock") == 0)
    {
        p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, "160x120");
        p.set(CameraParameters::KEY_PREVIEW_SIZE, "160x120");
    }

    // capture size
    const char *parm = p.get(CameraParameters::KEY_PREVIEW_SIZE);
    parse_pair(parm, &mCaptureWidth, &mCaptureHeight, 'x');
    LOGV("default preview size: %dx%d", mCaptureWidth, mCaptureHeight);
    // default callback size
    mCallbackNotifier.setCBSize(mCaptureWidth, mCaptureHeight);

    mV4L2CameraDevice->tryFmtSize(&mCaptureWidth, &mCaptureHeight);
    mVideoCaptureWidth = mCaptureWidth;
    mVideoCaptureHeight= mCaptureHeight;
    p.set(KEY_CAPTURE_SIZE_WIDTH, mCaptureWidth);
    p.set(KEY_CAPTURE_SIZE_HEIGHT, mCaptureHeight);
    p.set(KEY_PREVIEW_CAPTURE_SIZE_WIDTH, mCaptureWidth);
    p.set(KEY_PREVIEW_CAPTURE_SIZE_HEIGHT, mCaptureHeight);

    // preview formats, CTS must support at least 2 formats
    parameterString = CameraParameters::PIXEL_FORMAT_YUV420SP;            // NV21, default
    parameterString.append(",");
    parameterString.append(CameraParameters::PIXEL_FORMAT_YUV420P);        // YV12
    //parameterString.append(",");
    //parameterString.append(CameraParameters::PIXEL_FORMAT_RGBA8888);    // rgba8888

    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS, parameterString.string());

    p.set(CameraParameters::KEY_VIDEO_FRAME_FORMAT, CameraParameters::PIXEL_FORMAT_YUV420SP);
    p.set(CameraParameters::KEY_PREVIEW_FORMAT, CameraParameters::PIXEL_FORMAT_YUV420SP);

    p.set(CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS, CameraParameters::PIXEL_FORMAT_JPEG);

    p.set(CameraParameters::KEY_PICTURE_FORMAT, CameraParameters::PIXEL_FORMAT_JPEG);

    p.set(CameraParameters::KEY_JPEG_QUALITY, "95"); // maximum quality
    p.set(CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES, "320x240,0x0");
    p.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, "320");
    p.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, "240");
    p.set(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, "90");

    p.setPictureFormat(CameraParameters::PIXEL_FORMAT_JPEG);

    mCallbackNotifier.setJpegThumbnailSize(320, 240);

    // record hint
    p.set(CameraParameters::KEY_RECORDING_HINT, "false");

    // rotation
    p.set(CameraParameters::KEY_ROTATION, 0);

    // add for CTS
#if 0
    if (mHalCameraInfo.facing == CAMERA_FACING_BACK)
    {
        p.set(CameraParameters::KEY_HORIZONTAL_VIEW_ANGLE, "56.4");
        p.set(CameraParameters::KEY_VERTICAL_VIEW_ANGLE, "39.4");
    }
    else
    {
        p.set(CameraParameters::KEY_HORIZONTAL_VIEW_ANGLE, "54.0");
        p.set(CameraParameters::KEY_VERTICAL_VIEW_ANGLE, "39.4");
    }
#endif
    float HorizonalViewAngle = mCameraConfig->getHorizonalViewAngle();
    float VerticalViewAngle = mCameraConfig->getVerticalViewAngle();

    if(HorizonalViewAngle < 0.001 || VerticalViewAngle < 0.001)
    {
        if (mHalCameraInfo.facing == CAMERA_FACING_BACK)
        {
            HorizonalViewAngle = 56.4;
            VerticalViewAngle = 39.4;
        }
        else
        {
            HorizonalViewAngle = 54.0;
            VerticalViewAngle = 39.4;
        }
    }
    //LOGV("getHorizonalViewAngle: %f",HorizonalViewAngle);
    //LOGV("getVerticalViewAngle: %f ",VerticalViewAngle);
    p.setFloat(CameraParameters::KEY_HORIZONTAL_VIEW_ANGLE, HorizonalViewAngle);
    p.setFloat(CameraParameters::KEY_VERTICAL_VIEW_ANGLE, VerticalViewAngle);
#ifdef __OPEN_FACEDECTION__
    p.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW, 15);
    p.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_SW, 0);
#else
    p.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW, 0);
    p.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_SW, 0);
#endif

    // take picture in video mode
    p.set(CameraParameters::KEY_VIDEO_SNAPSHOT_SUPPORTED, "true");

    //init keys added for AWGallery
    p.set("continuous-picture-path", "");
    p.set("is_continuous_picture_fast", "false");
    p.set("snap-path", "");
    p.set("picture-mode", "normal");
    p.set("cancel_continuous_picture", "false");
    mParameters = p;

    mFirstSetParameters = true;

    mLastFocusAreas.x1 = -1000;
    mLastFocusAreas.y1 = -1000;
    mLastFocusAreas.x2 = -1000;
    mLastFocusAreas.y2 = -1000;

    LOGV("CameraHardware::initDefaultParameters ok");
}
int CameraHardware::setExifInfo(struct isp_exif_attribute exifinfo)
{
    float focal_lenght = (float)exifinfo.focal_length/100.0;
    mParameters.setFloat(CameraParameters::KEY_FOCAL_LENGTH,focal_lenght);
    //mParameters.dump();
    return 0;
}

void CameraHardware::onCameraDeviceError(int err)
{
    F_LOG;
    /* Errors are reported through the callback notifier */
    mCallbackNotifier.onCameraDeviceError(err);
}

/****************************************************************************
 * Camera API implementation.
 ***************************************************************************/

status_t CameraHardware::setCameraHardwareInfo(HALCameraInfo * halInfo)
{
    // check input params
    if (halInfo == NULL)
    {
        LOGE("ERROR camera info");
        return EINVAL;
    }

    memcpy((void*)&mHalCameraInfo, (void*)halInfo, sizeof(HALCameraInfo));

    return NO_ERROR;
}

bool CameraHardware::isCameraIdle()
{
    Mutex::Autolock locker(&mCameraIdleLock);
    return mIsCameraIdle;
}

status_t CameraHardware::connectCamera(hw_device_t** device)
{
    F_LOG;
    status_t res = EINVAL;

    {
        Mutex::Autolock locker(&mCameraIdleLock);
        mIsCameraIdle = false;
    }

    if (mV4L2CameraDevice != NULL)
    {
        res = mV4L2CameraDevice->connectDevice(&mHalCameraInfo);
        if (res == NO_ERROR)
        {
            *device = &common;

            if (mCameraConfig->supportFocusMode())
            {
                mV4L2CameraDevice->setAutoFocusInit();
                mV4L2CameraDevice->setExposureMode(V4L2_EXPOSURE_AUTO);

                // start focus thread
                mAutoFocusThread->startThread();
            }
        }
        else
        {
            Mutex::Autolock locker(&mCameraIdleLock);
            mIsCameraIdle = true;
        }
    }

    return -res;
}

status_t CameraHardware::closeCamera()
{
    F_LOG;

    return cleanupCamera();
}

status_t CameraHardware::setPreviewWindow(struct preview_stream_ops* window)
{
    F_LOG;
    /* Callback should return a negative errno. */
    /* return -mPreviewWindow.setPreviewWindow(window); */
    status_t res = EINVAL;
    res = mPreviewWindow.setPreviewWindow(window);
    if(mV4L2CameraDevice->mPreviewIsNull){
        stopPreview();
        usleep(50*1000);
        startPreview();
        LOGE("mPreviewIsNull restart preview, F:%s,L:%d",__FUNCTION__,__LINE__);
    }
    return -res;
}

void CameraHardware::setCallbacks(camera_notify_callback notify_cb,
                                  camera_data_callback data_cb,
                                  camera_data_timestamp_callback data_cb_timestamp,
                                  camera_request_memory get_memory,
                                  void* user)
{
    F_LOG;
    mCallbackNotifier.setCallbacks(notify_cb, data_cb, data_cb_timestamp,
                                    get_memory, user);
}

void CameraHardware::enableMsgType(int32_t msg_type)
{
    mCallbackNotifier.enableMessage(msg_type);
}

void CameraHardware::disableMsgType(int32_t msg_type)
{
    mCallbackNotifier.disableMessage(msg_type);
}

int CameraHardware::isMsgTypeEnabled(int32_t msg_type)
{
    return mCallbackNotifier.isMessageEnabled(msg_type);
}

void CameraHardware::enableSmartMsgType(int32_t msg_type)
{
    mCallbackNotifier.enableSmartMessage(msg_type);
}

void CameraHardware::disableSmartMsgType(int32_t msg_type)
{
    mCallbackNotifier.disableSmartMessage(msg_type);
}

int CameraHardware::isSmartMsgTypeEnabled(int32_t msg_type)
{
    return mCallbackNotifier.isSmartMessageEnabled(msg_type);
}
status_t CameraHardware::startPreview()
{
    F_LOG;
    Mutex::Autolock locker(&mObjectLock);
    /* Callback should return a negative errno. */
    mStartPreviewFlag = true;
    return -doStartPreview();
}

void CameraHardware::stopPreview()
{
    F_LOG;

    pthread_mutex_lock(&mCommandMutex);
    mQueueElement[CMD_QUEUE_STOP_FACE_DETECTE].cmd = CMD_QUEUE_STOP_FACE_DETECTE;
    OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_STOP_FACE_DETECTE]);
    pthread_cond_signal(&mCommandCond);
    pthread_mutex_unlock(&mCommandMutex);

    Mutex::Autolock locker(&mObjectLock);
    mStartPreviewFlag = false;
    doStopPreview();
}

int CameraHardware::isPreviewEnabled()
{
    F_LOG;
    return mPreviewWindow.isPreviewEnabled();
}

status_t CameraHardware::enablePreview()
{
    F_LOG;
    mPreviewWindow.startPreview();
    return NO_ERROR;
}

status_t CameraHardware::disablePreview()
{
    F_LOG;
    mPreviewWindow.stopPreview();
    return NO_ERROR;
}

status_t CameraHardware::storeMetaDataInBuffers(int enable)
{
    F_LOG;
#ifdef __CEDARX_FRAMEWORK_1__
    /* Callback should return a negative errno. */
    return -mCallbackNotifier.storeMetaDataInBuffers(enable);
#elif defined __CEDARX_FRAMEWORK_2__
    if(enable == false) {
        mUseHwEncoder = false;
        mV4L2CameraDevice->setHwEncoder(false);
    }else{
        mUseHwEncoder = true;
        mV4L2CameraDevice->setHwEncoder(true);
    }
    return OK;
#endif

}

status_t CameraHardware::startRecording()
{
    F_LOG;

    if (mStartPreviewFlag == false)
    {
        LOGE("It shouldn't be possible to start recording without enabling preview first.");
        return UNKNOWN_ERROR;
    }

    status_t enableVideoRecording;
    enableVideoRecording = mCallbackNotifier.enableVideoRecording();
    // video hint
    const char * valstr = mParameters.get(CameraParameters::KEY_RECORDING_HINT);
    if (valstr)
    {
        LOGV("KEY_RECORDING_HINT: %s -> true", valstr);
    }

    mParameters.set(KEY_SNAP_PATH, "");
    mCallbackNotifier.setSnapPath("");

    mParameters.set(CameraParameters::KEY_RECORDING_HINT, CameraParameters::TRUE);

    if (mCameraConfig->supportFocusMode())
    {
        parse_focus_areas("(0, 0, 0, 0, 0)", true);
        mV4L2CameraDevice->set3ALock(V4L2_LOCK_FOCUS);
    }

    if (mVideoCaptureWidth != mCaptureWidth
        || mVideoCaptureHeight != mCaptureHeight)
    {
        doStopPreview();
        doStartPreview();
    }

    /* Callback should return a negative errno. */
    return enableVideoRecording;
}

void CameraHardware::stopRecording()
{
    F_LOG;
    mCallbackNotifier.disableVideoRecording();
    mV4L2CameraDevice->setHwEncoder(false);

    if (mCameraConfig->supportFocusMode())
    {
        //mV4L2CameraDevice->set3ALock(~V4L2_LOCK_FOCUS);
        mV4L2CameraDevice->set3ALock((~V4L2_LOCK_EXPOSURE) & (~V4L2_LOCK_WHITE_BALANCE)& (~V4L2_LOCK_FOCUS));
    }
}

int CameraHardware::isRecordingEnabled()
{
    F_LOG;
    return mCallbackNotifier.isVideoRecordingEnabled();
}

void CameraHardware::releaseRecordingFrame(const void* opaque)
{
    F_LOG;
    if (mUseHwEncoder)
    {
        int lock_status;
        int try_lock_count = 0;
        VideoNativeHandleMetadata *metadata = (VideoNativeHandleMetadata*)(opaque);

CONTINUE_RELEASE:
        lock_status = mCallbackNotifier.releaseRecordingFrameLock();
        if(lock_status == 0 || try_lock_count > 20)
        {
            mV4L2CameraDevice->releasePreviewFrame(metadata->pHandle);
            mCallbackNotifier.releaseRecordingFrameCount();
            mCallbackNotifier.releaseRecordingFrameUnLock();
        }
        else
        {
            LOGW("releaseRecordingFrame try_lock_status error!Wait for a minute!");
            usleep(5000);
            try_lock_count++;
            goto CONTINUE_RELEASE;
        }

        if(mV4L2CameraDevice->mCameraType == CAMERA_TYPE_UVC
            || mV4L2CameraDevice->mCameraType == CAMERA_TYPE_CSI
            || mV4L2CameraDevice->mCameraType == CAMERA_TYPE_VFE)
        {
            native_handle_close(metadata->pHandle);
            native_handle_delete(metadata->pHandle);
        }
    }
}

/****************************************************************************
 * Focus
 ***************************************************************************/

status_t CameraHardware::setAutoFocus()
{
 
    const char *new_focus_mode_str = mParameters.get(CameraParameters::KEY_FOCUS_MODE);

    if (!mCameraConfig->supportFocusMode())
    {
        pthread_mutex_lock(&mCommandMutex);
        mQueueElement[CMD_QUEUE_SET_FOCUS_STATUS].cmd = CMD_QUEUE_SET_FOCUS_STATUS;
        OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_SET_FOCUS_STATUS]);
        pthread_cond_signal(&mCommandCond);
        pthread_mutex_unlock(&mCommandMutex);

        return OK;
    }

    pthread_mutex_lock(&mAutoFocusMutex);

    if (!strcmp(new_focus_mode_str, CameraParameters::FOCUS_MODE_INFINITY)
        || !strcmp(new_focus_mode_str, CameraParameters::FOCUS_MODE_FIXED))
    {
        // do nothing
    }
    else
    {
        mV4L2CameraDevice->set3ALock(~(V4L2_LOCK_FOCUS | V4L2_LOCK_EXPOSURE| V4L2_LOCK_WHITE_BALANCE));
        if((mLastFocusAreas.x1 == 0 && mLastFocusAreas.y1 == 0 && mLastFocusAreas.x2 == 0 && mLastFocusAreas.y2 == 0) || \
            mLastFocusAreas.x1 == -1000 && mLastFocusAreas.y1 == -1000 && mLastFocusAreas.x2 == -1000 && mLastFocusAreas.y2 == -1000){
                mV4L2CameraDevice->setAutoFocusRange(V4L2_AUTO_FOCUS_RANGE_AUTO);
        }
        else
        mV4L2CameraDevice->setAutoFocusStart();
    }

    mIsSingleFocus = true;
    pthread_mutex_unlock(&mAutoFocusMutex);

    return OK;
}

status_t CameraHardware::cancelAutoFocus()
{
    F_LOG;

    /* TODO: Future enhancements. */
    return NO_ERROR;
}

int CameraHardware::parse_focus_areas(const char *str, bool is_face)
{
    const char *ptr,*tmp;
    char p1[6] = {0}, p2[6] = {0};
    char p3[6] = {0}, p4[6] = {0}, p5[6] = {0};
    int  l,t,r,b;
    int  w,h;

    if (str == NULL
        || !mCameraConfig->supportFocusMode())
    {
        return 0;
    }

    // LOGV("parse_focus_areas : %s", str);

    tmp = strchr(str,'(');
    tmp++;
    ptr = strchr(tmp,',');
    memcpy(p1,tmp,ptr-tmp);

    tmp = ptr+1;
    ptr = strchr(tmp,',');
    memcpy(p2,tmp,ptr-tmp);

    tmp = ptr+1;
    ptr = strchr(tmp,',');
    memcpy(p3,tmp,ptr-tmp);

    tmp = ptr+1;
    ptr = strchr(tmp,',');
    memcpy(p4,tmp,ptr-tmp);

    tmp = ptr+1;
    ptr = strchr(tmp,')');
    memcpy(p5,tmp,ptr-tmp);

    l = atoi(p1);
    t = atoi(p2);
    r = atoi(p3);
    b = atoi(p4);

    w = l + (r-l)/2;
    h = t + (b-t)/2;

    struct v4l2_win_coordinate win;
    win.x1 = l;
    win.y1 = t;
    win.x2 = r;
    win.y2 = b;
    if (abs(mLastFocusAreas.x1 - win.x1) >= 40
        || abs(mLastFocusAreas.y1 - win.y1) >= 40
        || abs(mLastFocusAreas.x2 - win.x2) >= 40
        || abs(mLastFocusAreas.y2 - win.y2) >= 40)
    {
        if (!is_face && (mZoomRatio != 0))
        {
            win.x1 = win.x1 * 100 / mZoomRatio;
            win.y1 = win.y1 * 100 / mZoomRatio;
            win.x2 = win.x2 * 100 / mZoomRatio;
            win.y2 = win.y2 * 100 / mZoomRatio;
        }

        LOGV("mZoomRatio: %d, v4l2_win_coordinate: [%d,%d,%d,%d]",
            mZoomRatio, win.x1, win.y1, win.x2, win.y2);

        if ((l == 0) && (t == 0) && (r == 0) && (b == 0))
        {
            mV4L2CameraDevice->set3ALock(~(V4L2_LOCK_FOCUS | V4L2_LOCK_EXPOSURE | V4L2_LOCK_WHITE_BALANCE));
            setAutoFocusRange();
#ifdef USE_SUNXI_CAMERA_H
            mV4L2CameraDevice->setAutoFocusWind(0, (void*)&win);
            mV4L2CameraDevice->setExposureWind(0, (void*)&win);
#endif
        }
        else
        {
#ifdef USE_SUNXI_CAMERA_H
            mV4L2CameraDevice->setAutoFocusWind(1, (void*)&win);
            mV4L2CameraDevice->setExposureWind(1, (void*)&win);
#endif
        }
        mLastFocusAreas.x1 = win.x1;
        mLastFocusAreas.y1 = win.y1;
        mLastFocusAreas.x2 = win.x2;
        mLastFocusAreas.y2 = win.y2;
    }

    return 0;
}

int CameraHardware::setAutoFocusRange()
{
    F_LOG;

    v4l2_auto_focus_range af_range = V4L2_AUTO_FOCUS_RANGE_INFINITY;
    if (mCameraConfig->supportFocusMode())
    {
        // focus
        const char *new_focus_mode_str = mParameters.get(CameraParameters::KEY_FOCUS_MODE);
        if (!strcmp(new_focus_mode_str, CameraParameters::FOCUS_MODE_AUTO))
        {
            af_range = V4L2_AUTO_FOCUS_RANGE_AUTO;
        }
        else if (!strcmp(new_focus_mode_str, CameraParameters::FOCUS_MODE_INFINITY))
        {
            af_range = V4L2_AUTO_FOCUS_RANGE_INFINITY;
        }
        else if (!strcmp(new_focus_mode_str, CameraParameters::FOCUS_MODE_MACRO))
        {
            af_range = V4L2_AUTO_FOCUS_RANGE_MACRO;
        }
        else if (!strcmp(new_focus_mode_str, CameraParameters::FOCUS_MODE_FIXED))
        {
            af_range = V4L2_AUTO_FOCUS_RANGE_INFINITY;
        }
        else if (!strcmp(new_focus_mode_str, CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE)
                    || !strcmp(new_focus_mode_str, CameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO))
        {
            af_range = V4L2_AUTO_FOCUS_RANGE_AUTO;
        }
        else
        {
            return -EINVAL;
        }
    }
    else
    {
        af_range = V4L2_AUTO_FOCUS_RANGE_INFINITY;
    }

    mV4L2CameraDevice->setAutoFocusRange(af_range);

    return OK;
}

bool CameraHardware::checkFocusMode(const char * mode)
{
    const char * str = mParameters.get(CameraParameters::KEY_SUPPORTED_FOCUS_MODES);
    if (!strstr(str, mode))
    {
        return false;
    }
    return true;
}

bool CameraHardware::checkFocusArea(const char * area)
{
    if (area == NULL)
    {
        LOGE("checkFocusArea, area is null");
        return false;
    }

    int i = 0;
    int arr[5];        // [l, t, r, b, w]
    char temp[128];
    strcpy(temp, area);
    char *pval = temp;
    char *seps = (char *)"(,)";
    pval = strtok(pval, seps);
    while (pval != NULL)
    {
        if (i >= 5)
        {
            LOGE("checkFocusArea, i = %d", i);
            return false;
        }
        arr[i++] = atoi(pval);
        pval = strtok(NULL, seps);
    }

    LOGV("%s : (%d, %d, %d, %d, %d)", __FUNCTION__, arr[0], arr[1],arr[2],arr[3],arr[4]);

    if ((arr[0] == 0)
        && (arr[1] == 0)
        && (arr[2] == 0)
        && (arr[3] == 0)
        && (arr[4] == 0))
    {
        return true;
    }

    if (arr[4] < 1)
    {
        LOGE("checkFocusArea, arr[4] = %d", arr[4]);
        return false;
    }

    for(i = 0; i < 4; i++)
    {
        if ((arr[i] < -1000) || (arr[i] > 1000))
        {
            LOGE("checkFocusArea, i: %d arr[i] = %d", i, arr[i]);
            return false;
        }
    }

    if ((arr[0] >= arr[2])
        || (arr[1] >= arr[3]))
    {
        LOGE("checkFocusArea : (%d, %d, %d, %d, %d)", arr[0], arr[1],arr[2],arr[3],arr[4]);
        return false;
    }

    return true;
}

/****************************************************************************
 * take picture management
 ***************************************************************************/

status_t CameraHardware::doTakePicture()
{
    F_LOG;
    Mutex::Autolock locker(&mObjectLock);

    /* Get JPEG quality. */
    int jpeg_quality = mParameters.getInt(CameraParameters::KEY_JPEG_QUALITY);
    if (jpeg_quality <= 0) {
        jpeg_quality = 90;
    }

    /* Get JPEG rotate. */
    int jpeg_rotate = mParameters.getInt(CameraParameters::KEY_ROTATION);
    if (jpeg_rotate <= 0) {
        jpeg_rotate = 0;  /* Fall back to default. */
    }

    /* Get JPEG size. */
    int pic_width, pic_height;
    mParameters.getPictureSize(&pic_width, &pic_height);

    mCallbackNotifier.setJpegQuality(jpeg_quality);
    mCallbackNotifier.setJpegRotate(jpeg_rotate);
    mCallbackNotifier.setPictureSize(pic_width, pic_height);

    // mV4L2CameraDevice->setTakePictureCtrl();

    // if in recording mode
    const char * valstr = mParameters.get(CameraParameters::KEY_RECORDING_HINT);
    bool video_hint = (strcmp(valstr, CameraParameters::TRUE) == 0);
    if (video_hint)
    {
        if (strcmp(mCallingProcessName, "com.android.cts.stub"))
        {
            // not cts
            //modify for cts by clx
            //mCallbackNotifier.setPictureSize(mCaptureWidth, mCaptureHeight);
        }
        mV4L2CameraDevice->setTakePictureState(TAKE_PICTURE_RECORD);
        return OK;
    }

    // picture mode
    const char * cur_picture_mode = mParameters.get(KEY_PICTURE_MODE);
    if (cur_picture_mode != NULL)
    {
        // continuous picture
        if (!strcmp(cur_picture_mode, PICTURE_MODE_CONTINUOUS)
            || !strcmp(cur_picture_mode, PICTURE_MODE_CONTINUOUS_FAST))
        {
            // test continuous picture
            int number = 0;
            mV4L2CameraDevice->setTakePictureCtrl(V4L2_TAKE_PICTURE_FAST);
            if (!strcmp(cur_picture_mode, PICTURE_MODE_CONTINUOUS))
            {
                number = 40;
                mV4L2CameraDevice->setTakePictureState(TAKE_PICTURE_CONTINUOUS);
            }
            else if (!strcmp(cur_picture_mode, PICTURE_MODE_CONTINUOUS_FAST))
            {
                number = 10;
                mV4L2CameraDevice->setTakePictureState(TAKE_PICTURE_CONTINUOUS_FAST);
            }
            mCallbackNotifier.setPictureSize(pic_width, pic_height);
            mCallbackNotifier.setContinuousPictureCnt(number);
            mCallbackNotifier.startContinuousPicture();
            mV4L2CameraDevice->setContinuousPictureCnt(number);
            mV4L2CameraDevice->startContinuousPicture();

            return OK;
        }
        else if (!strcmp(cur_picture_mode, PICTURE_MODE_BLINK))
        {
            mV4L2CameraDevice->setTakePictureCtrl(V4L2_TAKE_PICTURE_FAST);
            mV4L2CameraDevice->setTakePictureState(TAKE_PICTURE_SMART);
            mV4L2CameraDevice->startSmartPicture();
            return OK;
        }
        else if (!strcmp(cur_picture_mode, PICTURE_MODE_SMILE))
        {
            mV4L2CameraDevice->setTakePictureCtrl(V4L2_TAKE_PICTURE_FAST);
            mV4L2CameraDevice->setTakePictureState(TAKE_PICTURE_SMART);
            mV4L2CameraDevice->startSmartPicture();
            return OK;
        }
        else if (!strcmp(cur_picture_mode, PICTURE_MODE_SCENE_MODE))
        {
            const char *now_scene_mode_str = mParameters.get(CameraParameters::KEY_SCENE_MODE);
            if (!strcmp(now_scene_mode_str, CameraParameters::SCENE_MODE_HDR)    || \
                !strcmp(now_scene_mode_str, CameraParameters::SCENE_MODE_NIGHT)){
                mV4L2CameraDevice->setTakePictureState(TAKE_PICTURE_SCENE_MODE);
                mV4L2CameraDevice->startSceneModePicture(0);
                mV4L2CameraDevice->setTakePictureCtrl(V4L2_TAKE_PICTURE_HDR);
                return OK;
            }
        }
    }

    if (mCameraConfig->supportFlashMode() && mCameraConfig->cameraFacing() == CAMERA_FACING_BACK){
        unsigned long new_flash = -1;
        const char * cur_flash_mode = mParameters.get(CameraParameters::KEY_FLASH_MODE);
        LOGD("CAMERA_FACING_BACK, get KEY_FLASH_MODE: %s", cur_flash_mode);
        if (!strcmp(cur_flash_mode, CameraParameters::FLASH_MODE_OFF))
            new_flash = V4L2_FLASH_LED_MODE_NONE;
        else if (!strcmp(cur_flash_mode, CameraParameters::FLASH_MODE_AUTO))
            new_flash = V4L2_FLASH_LED_MODE_AUTO;
        else if (!strcmp(cur_flash_mode, CameraParameters::FLASH_MODE_ON)){
            new_flash = V4L2_FLASH_LED_MODE_FLASH;
            //recording mode,Flash will keep in torch status
            if(video_hint) new_flash = V4L2_FLASH_LED_MODE_TORCH;
        }
        else if (!strcmp(cur_flash_mode, CameraParameters::FLASH_MODE_TORCH))
            new_flash = V4L2_FLASH_LED_MODE_TORCH;
        else if (!strcmp(cur_flash_mode, CameraParameters::FLASH_MODE_RED_EYE))
            new_flash = V4L2_FLASH_LED_MODE_RED_EYE;
        else{
            LOGE("ERR(%s):Invalid flash(%s)", __FUNCTION__, cur_flash_mode); //twilight, shade
        }

        if (mV4L2CameraDevice->setFlashMode(new_flash) < 0)
        {
          LOGE("ERR(%s):Fail on mV4L2Camera->setFlashMode(flash(%s))", __FUNCTION__, cur_flash_mode);
        }
        if (!strcmp(cur_flash_mode, CameraParameters::FLASH_MODE_ON) ||
            !strcmp(cur_flash_mode, CameraParameters::FLASH_MODE_AUTO)) {
              mV4L2CameraDevice->setTakePictureState(TAKE_PICTURE_FAST);
              mV4L2CameraDevice->setTakePictureCtrl(V4L2_TAKE_PICTURE_FLASH);
              return OK;
        } else {
          mV4L2CameraDevice->setTakePictureCtrl(V4L2_TAKE_PICTURE_NORM);
        }
    }else if (mCameraConfig->supportFlashMode() && mCameraConfig->cameraFacing() == CAMERA_FACING_FRONT){
        unsigned long new_flash = -1;
        const char * cur_flash_mode = mParameters.get(CameraParameters::KEY_FLASH_MODE);
        LOGD("CAMERA_FACING_FRONT, get KEY_FLASH_MODE: %s", cur_flash_mode);
        if (!strcmp(cur_flash_mode, CameraParameters::FLASH_MODE_OFF))
            new_flash = V4L2_FLASH_LED_MODE_NONE;
        else if (!strcmp(cur_flash_mode, CameraParameters::FLASH_MODE_AUTO))
            new_flash = V4L2_FLASH_LED_MODE_TORCH;
        else if (!strcmp(cur_flash_mode, CameraParameters::FLASH_MODE_ON)){
            new_flash = V4L2_FLASH_LED_MODE_TORCH;
            //recording mode,Flash will keep in torch status
            if(video_hint) new_flash = V4L2_FLASH_LED_MODE_TORCH;
        }
        else if (!strcmp(cur_flash_mode, CameraParameters::FLASH_MODE_TORCH))
            new_flash = V4L2_FLASH_LED_MODE_TORCH;
        else if (!strcmp(cur_flash_mode, CameraParameters::FLASH_MODE_RED_EYE))
            new_flash = V4L2_FLASH_LED_MODE_TORCH;
        else{
            LOGE("ERR(%s):Invalid flash(%s)", __FUNCTION__, cur_flash_mode); //twilight, shade
        }

        if (mV4L2CameraDevice->setFlashMode(new_flash) < 0)
        {
          LOGE("ERR(%s):Fail on mV4L2Camera->setFlashMode(flash(%s))", __FUNCTION__, cur_flash_mode);
        }
        if (!strcmp(cur_flash_mode, CameraParameters::FLASH_MODE_ON) ||
            !strcmp(cur_flash_mode, CameraParameters::FLASH_MODE_AUTO)) {
              mV4L2CameraDevice->setTakePictureState(TAKE_PICTURE_FAST);
              mV4L2CameraDevice->setTakePictureCtrl(V4L2_TAKE_PICTURE_FLASH);
              return OK;
        } else {
          mV4L2CameraDevice->setTakePictureCtrl(V4L2_TAKE_PICTURE_NORM);
        }
    }
    else
        mV4L2CameraDevice->setTakePictureCtrl(V4L2_TAKE_PICTURE_NORM);
    // normal picture mode

    // full-size capture
/*    bool fast_picture_mode = mHalCameraInfo.fast_picture_mode;
    if (fast_picture_mode \
        && mIsImageCaptureIntent == false \
        && strcmp(mCallingProcessName, "com.android.cts.hardware"))
    {
        mV4L2CameraDevice->setTakePictureState(TAKE_PICTURE_FAST);
        return OK;
    }
    // normal taking picture mode
*/
    int frame_width = pic_width;
    int frame_height = pic_height;

    bool fast_picture_mode = mHalCameraInfo.fast_picture_mode;
    if((0== strcmp(mCallingProcessName, "com.android.cts.verifier")) &&
        (mFullSizeHeight * mFullSizeWidth == 1600 * 1200) &&
        !fast_picture_mode)
    {
        frame_width = 800;
        frame_height = 600;
    }

    mV4L2CameraDevice->tryFmtSize(&frame_width, &frame_height);

    if (mCaptureWidth == frame_width
        && mCaptureHeight == frame_height
        && mIsImageCaptureIntent == false
        && strcmp(mCallingProcessName, "android.camera.cts"))
    {
        LOGV("cancel current capture size[%dx%d] is the same as picture size", frame_width, frame_height);
        //mV4L2CameraDevice->setTakePictureState(TAKE_PICTURE_FAST);
        //return OK;
    }

    // preview format and video format are the same
    const char* pix_fmt = mParameters.getPictureFormat();
    uint32_t org_fmt = V4L2_PIX_FMT_NV21;

    /*
     * Make sure preview is not running, and device is stopped before taking
     * picture.
     */
    const bool preview_on = mPreviewWindow.isPreviewEnabled();
    if (preview_on) {
        doStopPreview();
        if (mV4L2CameraDevice->mCameraType == CAMERA_TYPE_UVC) {
            mV4L2CameraDevice->disconnectDevice();
            mV4L2CameraDevice->connectDevice(&mHalCameraInfo);
        }
    }

    /* Camera device should have been stopped when the shutter message has been
     * enabled. */
    if (mV4L2CameraDevice->isStarted())
    {
        LOGW("%s: Camera device is started", __FUNCTION__);
        mV4L2CameraDevice->stopDeliveringFrames();
        mV4L2CameraDevice->stopDevice();
    }

    /*
     * Take the picture now.
     */
    mV4L2CameraDevice->setTakePictureState(TAKE_PICTURE_NORMAL);

    mCaptureWidth = frame_width;
    mCaptureHeight = frame_height;

    LOGD("Starting camera, Size:%dx%d , picture format:%s",
         mCaptureWidth, mCaptureHeight, pix_fmt);
    mV4L2CameraDevice->showformat(org_fmt, "take picture");
    status_t res = mV4L2CameraDevice->startDevice(mCaptureWidth, mCaptureHeight, org_fmt, video_hint);
    if (res != NO_ERROR)
    {
        if (preview_on) {
            doStartPreview();
        }
        return res;
    }

    res = mV4L2CameraDevice->startDeliveringFrames();
    if (res != NO_ERROR)
    {
        mV4L2CameraDevice->setTakePictureState(TAKE_PICTURE_NULL);
        if (preview_on) {
            doStartPreview();
        }
    }

    return OK;
}

status_t CameraHardware::doTakePictureEnd()
{
    F_LOG;

    Mutex::Autolock locker(&mObjectLock);

    if (mV4L2CameraDevice->isConnected()             // camera is connected or started
        && !mPreviewWindow.isPreviewEnabled()        // preview is not enable
        && !mHalCameraInfo.fast_picture_mode)
    {
        if (mV4L2CameraDevice->isStarted() &&
            (mV4L2CameraDevice->mCameraType == CAMERA_TYPE_UVC)) {
            mV4L2CameraDevice->stopDeliveringFrames();
            mV4L2CameraDevice->stopDevice();
            mV4L2CameraDevice->disconnectDevice();
            mV4L2CameraDevice->connectDevice(&mHalCameraInfo);
        }
        LOGV("doTakePictureEnd to doStartPreview");
        doStartPreview();
    }
    const char * valstr = mParameters.get(CameraParameters::KEY_RECORDING_HINT);

    if (valstr == NULL) {
        LOGE("get recording hint failed");
        mV4L2CameraDevice->setTakePictureCtrl(V4L2_TAKE_PICTURE_STOP);
        return OK;
    }

    bool video_hint = (strcmp(valstr, CameraParameters::TRUE) == 0);
    if(!video_hint)
        mV4L2CameraDevice->setTakePictureCtrl(V4L2_TAKE_PICTURE_STOP);

    if (mCameraConfig->supportFlashMode() && mCameraConfig->cameraFacing() == CAMERA_FACING_FRONT){
        unsigned long new_flash = V4L2_FLASH_LED_MODE_NONE;
        if (mV4L2CameraDevice->setFlashMode(new_flash) < 0)
        {
          LOGE("%s:Fail on mV4L2Camera->setFlashMode()", __FUNCTION__);
        }
    }
    return OK;
}

status_t CameraHardware::doStartSmart()
{
    F_LOG;
    Mutex::Autolock locker(&mObjectLock);
    status_t res;
    // preview size;
    int mWidth = 640;
    int mHeight = 480;

    V4L2CameraDevice* const camera_dev = mV4L2CameraDevice;

    if (camera_dev->isStarted())
    {
        camera_dev->stopDeliveringFrames();
        camera_dev->stopDevice();
    }

    // Make sure camera device is connected.
    if (!camera_dev->isConnected())
    {
        res = camera_dev->connectDevice(&mHalCameraInfo);
        if (res != NO_ERROR)
        {
            return res;
        }
        camera_dev->setAutoFocusInit();
    }

    // preview format and video format are the same
    uint32_t org_fmt = V4L2_PIX_FMT_NV21;        // android default
    const char* preview_format = mParameters.getPreviewFormat();
    if (preview_format != NULL)
    {
        if (strcmp(preview_format, CameraParameters::PIXEL_FORMAT_YUV420SP) == 0)
        {
#ifdef __SUN6I__
            org_fmt = V4L2_PIX_FMT_NV12;        // SGX support NV12
#else
            org_fmt = V4L2_PIX_FMT_NV21;        // MALI support NV21
#endif
        }
        else if (strcmp(preview_format, CameraParameters::PIXEL_FORMAT_YUV420P) == 0)
        {
            org_fmt = V4L2_PIX_FMT_YVU420;        // YV12
        }
        else
        {
            LOGE("unknown preview format");
        }
    }
    LOGD("preview_format is %s", preview_format);
    camera_dev->showformat(org_fmt, "smart preview");
    res = camera_dev->startDevice(mWidth, mHeight, org_fmt, false);
    if (res != NO_ERROR)
    {
        return res;
    }

    res = camera_dev->startDeliveringFrames();
    if (res != NO_ERROR)
    {
        camera_dev->stopDevice();
    }

    return res;
}


status_t CameraHardware::doStopSmart()
{
    F_LOG;
    Mutex::Autolock locker(&mObjectLock);

    status_t res = NO_ERROR;

    /* Stop the camera. */
    if (mV4L2CameraDevice->isStarted())
    {
        mV4L2CameraDevice->stopDeliveringFrames();
        res = mV4L2CameraDevice->stopDevice();
    }

    return NO_ERROR;
}
status_t CameraHardware::takePicture()
{
    F_LOG;

    if (mStartPreviewFlag == false)
    {
        LOGE("Image capture should fail when preview didn't get enabled first.");
        return UNKNOWN_ERROR;
    }

    // smart picture mode
    const char * cur_picture_mode = mParameters.get(KEY_PICTURE_MODE);
    if (cur_picture_mode != NULL)
    {
        // smile or blink picture
        if(!strcmp(cur_picture_mode, PICTURE_MODE_SMILE))
        {
            pthread_mutex_lock(&mFaceDetectionMutex);

            if (mSmileDetectionState != FACE_DETECTION_STATE_ERROR)
            {
                ALOGV("mSmileDetectionEnable is true!!!");
                mSmileDetectionEnable = true;
                //mBlinkDetectionEnable = false;
            }
            else
            {
                ALOGD("Be careful: to start smile detection failed!");
            }
            pthread_mutex_unlock(&mFaceDetectionMutex);

        }

        #if 0
        else if(!strcmp(cur_picture_mode, PICTURE_MODE_BLINK))
        {
            pthread_mutex_lock(&mFaceDetectionMutex);

            if (mSmileDetectionState != FACE_DETECTION_STATE_ERROR)
            {
                mSmileDetectionEnable = false;
                mBlinkDetectionEnable = true;
            }
            else
            {
                ALOGD("Be careful: to start smile detection failed!");
            }
            pthread_mutex_unlock(&mFaceDetectionMutex);
        }
        #endif

    }
    pthread_mutex_lock(&mCommandMutex);
    mQueueElement[CMD_QUEUE_TAKE_PICTURE].cmd = CMD_QUEUE_TAKE_PICTURE;
    OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_TAKE_PICTURE]);
    pthread_cond_signal(&mCommandCond);
    pthread_mutex_unlock(&mCommandMutex);

    return OK;
}

status_t CameraHardware::cancelPicture()
{
    F_LOG;
    mV4L2CameraDevice->setTakePictureState(TAKE_PICTURE_NULL);

    pthread_mutex_lock(&mFaceDetectionMutex);

    if (mSmileDetectionEnable == true)
    {
        mSmileDetectionEnable = false;

        LOGV("SMILE_OPS_CMD_STOP");
        if (mSmileDetection != 0)
        {
            mSmileDetection->ioctrl(mSmileDetection, SMILE_OPS_CMD_STOP, 0, 0);
        }
        else
        {
            LOGW("SMILE_OPS_CMD_STOP failed, mSmileDetection not opened.");
        }

        pthread_mutex_lock(&mFaceDetectionStateMutex);

        mSmileDetectionState = FACE_DETECTION_STOPPED;

        pthread_mutex_unlock(&mFaceDetectionStateMutex);
    }
    pthread_mutex_unlock(&mFaceDetectionMutex);

    #if 0

    pthread_mutex_lock(&mFaceDetectionMutex);

    if (mBlinkDetectionEnable == true)
    {
        mBlinkDetectionEnable = false;

        LOGV("EYE_BLINK_OPS_CMD_STOP");
        if (mBlinkDetection != 0)
        {
            mBlinkDetection->ioctrl(mBlinkDetection, EYE_BLINK_OPS_CMD_STOP, 0, 0);
        }
        else
        {
            LOGW("EYE_BLINK_OPS_CMD_STOP failed, mBlinkDetection not opened.");
        }

        pthread_mutex_lock(&mFaceDetectionStateMutex);

        mBlinkDetectionState = FACE_DETECTION_STOPPED;

        pthread_mutex_unlock(&mFaceDetectionStateMutex);
    }
    pthread_mutex_unlock(&mFaceDetectionMutex);

    #endif
    return NO_ERROR;
}

//
void CameraHardware::notifyPictureMsg(const void* frame)
{

    pthread_mutex_lock(&mCommandMutex);
    mQueueElement[CMD_QUEUE_PICTURE_MSG].cmd = CMD_QUEUE_PICTURE_MSG;
    mQueueElement[CMD_QUEUE_PICTURE_MSG].data = (unsigned long)frame;
    OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_PICTURE_MSG]);
    pthread_cond_signal(&mCommandCond);
    pthread_mutex_unlock(&mCommandMutex);

    pthread_mutex_lock(&mFaceDetectionMutex);
    if (mSmileDetectionEnable == true)
    {
        mSmileDetectionEnable = false;
        LOGV("SMILE_OPS_CMD_STOP");
        if (mSmileDetection != 0)
        {
            mSmileDetection->ioctrl(mSmileDetection, SMILE_OPS_CMD_STOP, 0, 0);
        }
        else
        {
            LOGW("SMILE_OPS_CMD_STOP failed, mSmileDetection not opened.");
        }

        pthread_mutex_lock(&mFaceDetectionStateMutex);

        mSmileDetectionState = FACE_DETECTION_STOPPED;

        pthread_mutex_unlock(&mFaceDetectionStateMutex);
    }
    pthread_mutex_unlock(&mFaceDetectionMutex);

    #if 0

    pthread_mutex_lock(&mFaceDetectionMutex);

    if (mBlinkDetectionEnable == true)
    {
        mBlinkDetectionEnable = false;

        LOGV("EYE_BLINK_OPS_CMD_STOP");
        if (mBlinkDetection != 0)
        {
            mBlinkDetection->ioctrl(mBlinkDetection, EYE_BLINK_OPS_CMD_STOP, 0, 0);
        }
        else
        {
            LOGW("EYE_BLINK_OPS_CMD_STOP failed, mBlinkDetection not opened.");
        }

        pthread_mutex_lock(&mFaceDetectionStateMutex);

        mBlinkDetectionState = FACE_DETECTION_STOPPED;

        pthread_mutex_unlock(&mFaceDetectionStateMutex);
    }
    pthread_mutex_unlock(&mFaceDetectionMutex);

    #endif

}

/****************************************************************************
 * set and get parameters
 ***************************************************************************/

void CameraHardware::setVideoCaptureSize(int video_w, int video_h)
{
    LOGD("setVideoCaptureSize next version to do ......");

    // video size is video_w x video_h, capture size may be different
    // now the same
    mVideoCaptureWidth = video_w;
    mVideoCaptureHeight= video_h;


    int videoCaptureWidth = mVideoCaptureWidth;
    int videoCaptureHeight =mVideoCaptureHeight;

    int ret = mV4L2CameraDevice->tryFmtSize(&videoCaptureWidth, &videoCaptureHeight);
    if(ret < 0)
    {
        LOGE("setVideoCaptureSize tryFmtSize failed");
        return;
    }

    float widthRate = (float)videoCaptureWidth / (float)mVideoCaptureWidth;
    float heightRate = (float)videoCaptureHeight / (float)mVideoCaptureHeight;

    if((widthRate > 4.0) && (heightRate > 4.0))
    {
        mV4L2CameraDevice->setThumbUsedForVideo(true);
        mV4L2CameraDevice->setVideoSize(video_w, video_h);
        mVideoCaptureWidth = videoCaptureWidth;
        mVideoCaptureHeight=videoCaptureHeight;
        mParameters.set(KEY_CAPTURE_SIZE_WIDTH, videoCaptureWidth / 2);
        mParameters.set(KEY_CAPTURE_SIZE_HEIGHT, videoCaptureHeight / 2);
    }
    else
    {
        mV4L2CameraDevice->setThumbUsedForVideo(false);
        mVideoCaptureWidth = videoCaptureWidth;
        mVideoCaptureHeight=videoCaptureHeight;
        mParameters.set(KEY_CAPTURE_SIZE_WIDTH, videoCaptureWidth);
        mParameters.set(KEY_CAPTURE_SIZE_HEIGHT, videoCaptureHeight);
    }
}

void CameraHardware::getHWOrientionInfo(void *OrientionInfo)
{
    struct APPERCEIVEPEOPLE_INFO *info = (struct APPERCEIVEPEOPLE_INFO*)OrientionInfo;
    if(mHalCameraInfo.orientation == 90 || \
        mHalCameraInfo.orientation == 270)
        info->scree_oriention = 2;
    else info->scree_oriention = 1;
    if(mHalCameraInfo.orientation == 180 || \
        mHalCameraInfo.orientation == 270)
        info->buffer_oriention = 1;
    else info->buffer_oriention = 0;
    if(mHalCameraInfo.facing = CAMERA_FACING_FRONT && info->scree_oriention == 2)
        info->buffer_oriention = (info->buffer_oriention + 1)%2;
}

void CameraHardware::makeFDOrientionArray()
{
    memset(mFDOriention,0,sizeof(mFDOriention));
    int rotation;

    //todo: test mHalCameraInfo.orientation 270 and 180
    if(mHalCameraInfo.orientation == 90 || mHalCameraInfo.orientation == 270){
        for(int i = 0;i < 4;i++){
            mFDOriention[i] = (360-i * 90 + mHalCameraInfo.orientation)%360;
            if(mHalCameraInfo.facing == CAMERA_FACING_FRONT && (i == 1 || i == 3)){
                mFDOriention[i] = (mFDOriention[i] + 180)%360;
            }
            LOGV("FDOriention array[%d]: %d",i,mFDOriention[i]);
        }

    }
    else{
        for(int i = 0;i < 4;i++){
            mFDOriention[i] = (i * 90 + mHalCameraInfo.orientation)%360;
            if(mHalCameraInfo.facing == CAMERA_FACING_FRONT && (i == 1 || i == 3)){
                mFDOriention[i] = (mFDOriention[i] + 180)%360;
            }
            LOGV("FDOriention array[%d]: %d",i,mFDOriention[i]);
        }
    }

}

void CameraHardware::getCurrentOriention(int * oriention, int compensation,bool reverse,int re_direction)
{
    char rota[100];
    property_get("sys.current.rotation",rota,"Unknow");
    mOriention = atoi(rota);
    //LOGV("FDOriention: %d",mFDOriention[mOriention]);
    *oriention = (mFDOriention[mOriention] + compensation)%360;
    if(reverse && (*oriention == re_direction || *oriention == (re_direction + 180)%360)) {
        *oriention = (*oriention + 180)%360;
    }
    //LOGV("FDOriention: %d",*oriention);
}
int CameraHardware::getPriviewSize(int* preview_width,int* preview_height,int capture_width,int capture_height)
{
    //in order to have a better preview image,awgallery run in a larger size,which
    //set in V4L2CameraDevice. but other application must run in a matching size
    if (strcmp(mCallingProcessName, "com.android.awgallery")){
        int width = *preview_width;
        int height = *preview_height;
        mParameters.getPreviewSize(preview_width, preview_height);
        if(*preview_width <= 640 && *preview_height <= 480){
            *preview_width = *preview_width * 2;
            *preview_height = *preview_height *2;
        }
        if((*preview_width > capture_width) || (*preview_height > capture_height)){
            *preview_width = width;
            *preview_height = height;
        }
        LOGD("raw sensor %s capture: %dx%d  preview: %dx%d",__FUNCTION__, \
            capture_width,capture_height,*preview_width,*preview_height);
    }
    return 0;
}

status_t CameraHardware::setParameters(const char* p)
{
    F_LOG;
    int ret = UNKNOWN_ERROR;

    PrintParamDiff(mParameters, p);

    CameraParameters params;
    String8 str8_param(p);
    params.unflatten(str8_param);

    V4L2CameraDevice* pV4L2Device = mV4L2CameraDevice;
    if (pV4L2Device == NULL)
    {
        LOGE("%s : getCameraDevice failed", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    // stop continuous picture
    const char * cur_picture_mode = mParameters.get(KEY_PICTURE_MODE);
    const char * stop_continuous_picture = params.get(KEY_CANCEL_CONTINUOUS_PICTURE);
    LOGV("%s : stop_continuous_picture : %s", __FUNCTION__, stop_continuous_picture);
    if (cur_picture_mode != NULL
        && stop_continuous_picture != NULL
        && !strcmp(cur_picture_mode, PICTURE_MODE_CONTINUOUS)
        && !strcmp(stop_continuous_picture, "true"))
    {
        mQueueElement[CMD_QUEUE_STOP_CONTINUOUSSNAP].cmd = CMD_QUEUE_STOP_CONTINUOUSSNAP;
        OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_STOP_CONTINUOUSSNAP]);
    }

    // picture mode
    const char * new_picture_mode = params.get(KEY_PICTURE_MODE);
    LOGV("%s : new_picture_mode : %s", __FUNCTION__, new_picture_mode);
    if (new_picture_mode != NULL)
    {
        if (!strcmp(new_picture_mode, PICTURE_MODE_NORMAL)
            || !strcmp(new_picture_mode, PICTURE_MODE_BLINK)
            || !strcmp(new_picture_mode, PICTURE_MODE_SMILE)
            || !strcmp(new_picture_mode, PICTURE_MODE_CONTINUOUS)
            || !strcmp(new_picture_mode, PICTURE_MODE_CONTINUOUS_FAST)
            || !strcmp(new_picture_mode, PICTURE_MODE_SCENE_MODE))
        {
            mParameters.set(KEY_PICTURE_MODE, new_picture_mode);
        }
        else
        {
            LOGW("%s : unknown picture mode: %s", __FUNCTION__, new_picture_mode);
        }

        // continuous picture path
        if (!strcmp(new_picture_mode, PICTURE_MODE_CONTINUOUS)
            || !strcmp(new_picture_mode, PICTURE_MODE_CONTINUOUS_FAST))
        {
            const char * new_path = params.get(KEY_CONTINUOUS_PICTURE_PATH);
            LOGV("%s : new_path : %s", __FUNCTION__, new_path);
            if (new_path != NULL)
            {
                mParameters.set(KEY_CONTINUOUS_PICTURE_PATH, new_path);
                mCallbackNotifier.setSaveFolderPath(new_path);
            }
            else
            {
                LOGW("%s : invalid path: %s in %s picture mode", __FUNCTION__, new_path, new_picture_mode);
                mParameters.set(KEY_PICTURE_MODE, PICTURE_MODE_NORMAL);
            }
        }
        else if(!strcmp(new_picture_mode, PICTURE_MODE_NORMAL)
                || !strcmp(new_picture_mode, PICTURE_MODE_BLINK)
                || !strcmp(new_picture_mode, PICTURE_MODE_SMILE)
                || !strcmp(new_picture_mode, PICTURE_MODE_SCENE_MODE))
        {
            const char * new_path = params.get(KEY_SNAP_PATH);
            LOGV("%s : snap new_path : %s", __FUNCTION__, new_path);
            if (new_path != NULL)
            {
                mParameters.set(KEY_SNAP_PATH, new_path);
                mCallbackNotifier.setSnapPath(new_path);
            }
        }

        #if 1

        if (mCameraConfig->supportSceneMode())
        {
            const char *now_scene_mode_str = mParameters.get(CameraParameters::KEY_SCENE_MODE);
            const char *new_scene_mode_str = params.get(CameraParameters::KEY_SCENE_MODE);
            LOGV("%s : new_scene_mode_str %s", __FUNCTION__, new_scene_mode_str);
            if ((new_scene_mode_str != NULL)
                && (mFirstSetParameters || strcmp(now_scene_mode_str, new_scene_mode_str)))
            {
                mParameters.set(CameraParameters::KEY_SCENE_MODE, new_scene_mode_str);
                const char *scene_mode_str = mParameters.get(CameraParameters::KEY_SCENE_MODE);

                ALOGD("!!! scene_mode_str %s", scene_mode_str);

                if (now_scene_mode_str != NULL)
                {
                    LOGV("%s : now_scene_mode_str : %s", __FUNCTION__, scene_mode_str);
                    mV4L2CameraDevice->closeSceneMode();//close old scene mode frist
                    if (!strcmp(scene_mode_str, CameraParameters::SCENE_MODE_AUTO)) {
                        //mParameters.set(KEY_PICTURE_MODE, PICTURE_MODE_SCENE_MODE);
                        mParameters.set(KEY_PICTURE_MODE, PICTURE_MODE_NORMAL);
                        const char * new_path = params.get(KEY_SNAP_PATH);
                        LOGV("%s : snap new_path : %s", __FUNCTION__, new_path);
                        if (new_path != NULL)
                        {
                            mParameters.set(KEY_SNAP_PATH, new_path);
                            mCallbackNotifier.setSnapPath(new_path);
                        }

                    } else if (!strcmp(scene_mode_str, CameraParameters::SCENE_MODE_HDR)){
                        if(0 == mV4L2CameraDevice->openSceneMode(scene_mode_str)) {
                            mParameters.set(KEY_PICTURE_MODE, PICTURE_MODE_SCENE_MODE);
                            const char * new_path = params.get(KEY_SNAP_PATH);
                            LOGV("%s : snap new_path : %s", __FUNCTION__, new_path);
                            if (new_path != NULL)
                            {
                                mParameters.set(KEY_SNAP_PATH, new_path);
                                mCallbackNotifier.setSnapPath(new_path);
                            }
                        }
                    } else if (!strcmp(scene_mode_str, CameraParameters::SCENE_MODE_NIGHT)){
                        if(0 == mV4L2CameraDevice->openSceneMode(scene_mode_str)) {
                            mParameters.set(KEY_PICTURE_MODE, PICTURE_MODE_SCENE_MODE);
                            const char * new_path = params.get(KEY_SNAP_PATH);
                            LOGV("%s : snap new_path : %s", __FUNCTION__, new_path);
                            if (new_path != NULL)
                            {
                                mParameters.set(KEY_SNAP_PATH, new_path);
                                mCallbackNotifier.setSnapPath(new_path);
                            }
                        }
                    }else {
                        mV4L2CameraDevice->closeSceneMode();
                        mParameters.set(KEY_PICTURE_MODE, PICTURE_MODE_NORMAL);
                        LOGE("ERR(%s):Invalid scene mode(%s)", __FUNCTION__, now_scene_mode_str);
                        ret = UNKNOWN_ERROR;
                    }
                }
            }

        }
        #endif

    }

    const char * new_continuous_picture_fast = params.get("is_continuous_picture_fast");
    if (new_continuous_picture_fast != NULL)
    {
        if(!strcmp(new_continuous_picture_fast, "true"))
        {
            mCallbackNotifier.setPictureMode(false);
        }
        else
        {
            mCallbackNotifier.setPictureMode(true);
        }
    }
    else
    {
        mCallbackNotifier.setPictureMode(true);
    }

    const char * is_imagecapture_intent = params.get(KEY_IS_IMAGECAPTURE_INTENT);
    if (is_imagecapture_intent != NULL)
    {
        if(!strcmp(is_imagecapture_intent, "true"))
        {
            LOGD("fuqiang mIsImageCaptureIntent = true");
            mIsImageCaptureIntent = true;
        }
        else
        {
            LOGD("fuqiang mIsImageCaptureIntent = false");
            mIsImageCaptureIntent = false;
        }
    }
    // preview format
    const char * new_preview_format = params.getPreviewFormat();
    LOGV("%s : new_preview_format : %s", __FUNCTION__, new_preview_format);
    if (new_preview_format != NULL
        && (strcmp(new_preview_format, CameraParameters::PIXEL_FORMAT_YUV420SP) == 0
        || strcmp(new_preview_format, CameraParameters::PIXEL_FORMAT_YUV420P) == 0))
    {
        mParameters.setPreviewFormat(new_preview_format);
    }
    else
    {
        LOGE("%s : Only yuv420sp or yuv420p preview is supported", __FUNCTION__);
        return -EINVAL;
    }

    // picture format
    const char * new_picture_format = params.getPictureFormat();
    LOGV("%s : new_picture_format : %s", __FUNCTION__, new_picture_format);
    if (new_picture_format == NULL
        || (strcmp(new_picture_format, CameraParameters::PIXEL_FORMAT_JPEG) != 0))
    {
        LOGE("%s : Only jpeg still pictures are supported", __FUNCTION__);
        return -EINVAL;
    }

    // picture size
    int new_picture_width  = 0;
    int new_picture_height = 0;
    params.getPictureSize(&new_picture_width, &new_picture_height);
    LOGV("%s : new_picture_width x new_picture_height = %dx%d", __FUNCTION__, new_picture_width, new_picture_height);
    if (0 < new_picture_width && 0 < new_picture_height)
    {
        mParameters.setPictureSize(new_picture_width, new_picture_height);
    }
    else
    {
        LOGE("%s : error picture size", __FUNCTION__);
        return -EINVAL;
    }

    // preview size
    int new_preview_width  = 0;
    int new_preview_height = 0;
    params.getPreviewSize(&new_preview_width, &new_preview_height);
    LOGV("%s : new_preview_width x new_preview_height = %dx%d",
         __FUNCTION__, new_preview_width, new_preview_height);
    if (0 < new_preview_width && 0 < new_preview_height)
    {
        bool is_vga = false;
        mParameters.setPreviewSize(new_preview_width, new_preview_height);

        mCallbackNotifier.setCBSize(new_preview_width, new_preview_height);
#if 0

        // do it in camera.cfg
        if (!mHalCameraInfo.fast_picture_mode                    // YUV sensor
            && new_preview_width == 640                            // preview with 640x480
            && new_preview_height == 480
            && mFullSizeWidth * mFullSizeHeight > 1600*1200)    // > 2M, maybe 5M sensor
        {
            // try to use 1280x960 for preview
            is_vga = true;
            new_preview_width = 1280;
            new_preview_height = 960;
        }

        if (strcmp(mCallingProcessName, "com.android.cts.verifier") == 0   //add for CTS Verifier by fuqiang
            && new_preview_width == 640
            && new_preview_height == 480)
        {
            // try to use 1280x960 for preview
            is_vga = true;
            new_preview_width = 1280;
            new_preview_height = 960;
        }
#endif

        // try size
        ret = pV4L2Device->tryFmtSize(&new_preview_width, &new_preview_height);
        if(ret < 0)
        {
            return ret;
        }

        if (is_vga
            && (new_preview_width * 3 != new_preview_height * 4))    // 800x600 is also ok
        {
            new_preview_width = 640;
            new_preview_height = 480;

            // try size(vga) again
            ret = pV4L2Device->tryFmtSize(&new_preview_width, &new_preview_height);
            if(ret < 0)
            {
                return ret;
            }
        }

        mParameters.set(KEY_PREVIEW_CAPTURE_SIZE_WIDTH, new_preview_width);
        mParameters.set(KEY_PREVIEW_CAPTURE_SIZE_HEIGHT, new_preview_height);
        int format = mV4L2CameraDevice->getCaptureFormat();
        mV4L2CameraDevice->showformat(format,"csi capture format is: ");
        mParameters.set(KEY_CAPTURE_FORMAT, format);
    }
    else
    {
        LOGE("%s : error preview size", __FUNCTION__);
        return -EINVAL;
    }

    // video size
    int new_video_width        = new_preview_width;//0;
    int new_video_height    = new_preview_height;//0;
    params.getVideoSize(&new_video_width, &new_video_height);
    LOGD("%s getparams: new_video_width x new_video_height = %dx%d",
         __FUNCTION__, new_video_width, new_video_height);
    if(new_preview_width > new_video_width){
        if((new_video_width > 0 && new_video_height> 0) &&
            (new_video_width < 4096 && new_video_height < 4096)){
            new_preview_width = new_video_width;
            new_preview_height = new_video_height;
        }else{
            new_video_width = new_preview_width;
            new_video_height = new_preview_height;
        }
    }
    LOGV("%s  set: new_video_width x new_video_height = %dx%d",
         __FUNCTION__, new_video_width, new_video_height);
    if (0 < new_video_width && 0 < new_video_height)
    {
        int video_width        = 0;
        int video_height    = 0;
        mParameters.getVideoSize(&video_width, &video_height);
        if (mFirstSetParameters
            || video_width != new_video_width
            || video_height != new_video_height)
        {
            mParameters.setVideoSize(new_video_width, new_video_height);
            if (new_video_width != mVideoCaptureWidth
                || new_video_height != mVideoCaptureHeight)
            {
                setVideoCaptureSize(new_video_width, new_video_height);
            }
        }
    }
    else
    {
        LOGE("%s : error video size", __FUNCTION__);
        return -EINVAL;
    }

    // video hint
    const char * valstr = params.get(CameraParameters::KEY_RECORDING_HINT);
    if (valstr)
    {
        LOGV("%s : KEY_RECORDING_HINT: %s", __FUNCTION__, valstr);
        mParameters.set(CameraParameters::KEY_RECORDING_HINT, valstr);
    }

    // frame rate
    int new_min_frame_rate, new_max_frame_rate;
    params.getPreviewFpsRange(&new_min_frame_rate, &new_max_frame_rate);
    int new_preview_frame_rate = params.getPreviewFrameRate();
    if (0 < new_preview_frame_rate && 0 < new_min_frame_rate
        && new_min_frame_rate <= new_max_frame_rate)
    {
        int preview_frame_rate = mParameters.getPreviewFrameRate();
        if (mFirstSetParameters
            || preview_frame_rate != new_preview_frame_rate)
        {
            mParameters.setPreviewFrameRate(new_preview_frame_rate);
            pV4L2Device->setFrameRate(new_preview_frame_rate);
        }
    }
    else
    {
        if (pV4L2Device->getCaptureFormat() == V4L2_PIX_FMT_YUYV)
        {
            LOGV("may be usb camera, don't care frame rate");
        }
        else
        {
            LOGE("%s : error preview frame rate", __FUNCTION__);
            return -EINVAL;
        }
    }

    const char * fps_range =  params.get(CameraParameters::KEY_PREVIEW_FPS_RANGE);
    mParameters.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, fps_range );

// add for cts by clx
    int new_jpeg_thumbnail_quality = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY);

    LOGV("%s : new_jpeg_thumbnail_quality %d", __FUNCTION__, new_jpeg_thumbnail_quality);
    if (new_jpeg_thumbnail_quality >=1 && new_jpeg_thumbnail_quality <= 100)
        {
            mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, new_jpeg_thumbnail_quality);
        }



    // JPEG image quality
    int new_jpeg_quality = params.getInt(CameraParameters::KEY_JPEG_QUALITY);
    LOGV("%s : new_jpeg_quality %d", __FUNCTION__, new_jpeg_quality);
    if (new_jpeg_quality >=1 && new_jpeg_quality <= 100)
    {
        mParameters.set(CameraParameters::KEY_JPEG_QUALITY, new_jpeg_quality);
    }
    else
    {
        if (pV4L2Device->getCaptureFormat() == V4L2_PIX_FMT_YUYV)
        {
            LOGV("may be usb camera, don't care picture quality");
            mParameters.set(CameraParameters::KEY_JPEG_QUALITY, 90);
        }
        else
        {
            LOGE("%s : error picture quality", __FUNCTION__);
            return -EINVAL;
        }
    }

    // rotation
    int new_rotation = params.getInt(CameraParameters::KEY_ROTATION);
    LOGV("%s : new_rotation %d", __FUNCTION__, new_rotation);
    if (0 <= new_rotation)
    {
        mOriention = new_rotation;
        mParameters.set(CameraParameters::KEY_ROTATION, new_rotation);
    }
    else
    {
        LOGE("%s : error rotate", __FUNCTION__);
        return -EINVAL;
    }

    // image effect
    if (mCameraConfig->supportColorEffect())
    {
        const char *now_image_effect_str = mParameters.get(CameraParameters::KEY_EFFECT);
        const char *new_image_effect_str = params.get(CameraParameters::KEY_EFFECT);
        LOGV("%s : new_image_effect_str %s", __FUNCTION__, new_image_effect_str);
        if ((new_image_effect_str != NULL)
            && (mFirstSetParameters || strcmp(now_image_effect_str, new_image_effect_str)))
        {
            unsigned long  new_image_effect = -1;

            if (!strcmp(new_image_effect_str, CameraParameters::EFFECT_NONE))
                new_image_effect = V4L2_COLORFX_NONE;
            else if (!strcmp(new_image_effect_str, CameraParameters::EFFECT_MONO))
                new_image_effect = V4L2_COLORFX_BW;
            else if (!strcmp(new_image_effect_str, CameraParameters::EFFECT_SEPIA))
                new_image_effect = V4L2_COLORFX_SEPIA;
            else if (!strcmp(new_image_effect_str, CameraParameters::EFFECT_AQUA))
                new_image_effect = V4L2_COLORFX_GRASS_GREEN;
            else if (!strcmp(new_image_effect_str, CameraParameters::EFFECT_NEGATIVE))
                new_image_effect = V4L2_COLORFX_NEGATIVE;
            else {
                //posterize, whiteboard, blackboard, solarize
                LOGE("ERR(%s):Invalid effect(%s)", __FUNCTION__, new_image_effect_str);
                ret = UNKNOWN_ERROR;
            }

            if (new_image_effect >= 0) {
                mParameters.set(CameraParameters::KEY_EFFECT, new_image_effect_str);
                mQueueElement[CMD_QUEUE_SET_COLOR_EFFECT].cmd = CMD_QUEUE_SET_COLOR_EFFECT;
                mQueueElement[CMD_QUEUE_SET_COLOR_EFFECT].data = new_image_effect;
                OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_SET_COLOR_EFFECT]);
            }
        }
    }
    // white balance
    if (mCameraConfig->supportWhiteBalance())
    {
        const char *now_white_str = mParameters.get(CameraParameters::KEY_WHITE_BALANCE);
        const char *new_white_str = params.get(CameraParameters::KEY_WHITE_BALANCE);
        LOGV("%s : new_white_str %s", __FUNCTION__, new_white_str);
        if ((new_white_str != NULL)
            && (mFirstSetParameters || strcmp(now_white_str, new_white_str)))
        {
            unsigned long new_white = -1;
            int no_auto_balance = 1;

            if (!strcmp(new_white_str, CameraParameters::WHITE_BALANCE_AUTO))
            {
                new_white = V4L2_WHITE_BALANCE_AUTO;
                no_auto_balance = 0;
            }
            else if (!strcmp(new_white_str,
                             CameraParameters::WHITE_BALANCE_DAYLIGHT))
                new_white = V4L2_WHITE_BALANCE_DAYLIGHT;
            else if (!strcmp(new_white_str,
                             CameraParameters::WHITE_BALANCE_CLOUDY_DAYLIGHT))
                new_white = V4L2_WHITE_BALANCE_CLOUDY;
            else if (!strcmp(new_white_str,
                             CameraParameters::WHITE_BALANCE_FLUORESCENT))
                new_white = V4L2_WHITE_BALANCE_FLUORESCENT_H;
            else if (!strcmp(new_white_str,
                             CameraParameters::WHITE_BALANCE_INCANDESCENT))
                new_white = V4L2_WHITE_BALANCE_INCANDESCENT;
            else if (!strcmp(new_white_str,
                             CameraParameters::WHITE_BALANCE_WARM_FLUORESCENT))
                new_white = V4L2_WHITE_BALANCE_FLUORESCENT;
            else{
                LOGE("ERR(%s):Invalid white balance(%s)", __FUNCTION__, new_white_str); //twilight, shade
                ret = UNKNOWN_ERROR;
            }

            mCallbackNotifier.setWhiteBalance(no_auto_balance);

            if (0 <= new_white)
            {
                mParameters.set(CameraParameters::KEY_WHITE_BALANCE, new_white_str);
                mQueueElement[CMD_QUEUE_SET_WHITE_BALANCE].cmd = CMD_QUEUE_SET_WHITE_BALANCE;
                mQueueElement[CMD_QUEUE_SET_WHITE_BALANCE].data = new_white;
                OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_SET_WHITE_BALANCE]);
            }
        }
    }

     //add for cts by clx
    const char *new_ae_lock_str = params.get(CameraParameters::KEY_AUTO_EXPOSURE_LOCK);
    const char *now_ae_lock_str = mParameters.get(CameraParameters::KEY_AUTO_EXPOSURE_LOCK);
        if ((new_ae_lock_str != NULL)
                    && (mFirstSetParameters || strcmp(now_ae_lock_str, new_ae_lock_str)))
            mParameters.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK, new_ae_lock_str);

    // exposure compensation
    if (mCameraConfig->supportExposureCompensation())
    {
        long now_exposure_compensation = mParameters.getInt(CameraParameters::KEY_EXPOSURE_COMPENSATION);
        long new_exposure_compensation = params.getInt(CameraParameters::KEY_EXPOSURE_COMPENSATION);
        long max_exposure_compensation = params.getInt(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION);
        long min_exposure_compensation = params.getInt(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION);
        LOGV("%s : new_exposure_compensation %d,min_exposure_compensation %d,max_exposure_compensation %ld",
            __FUNCTION__, new_exposure_compensation,min_exposure_compensation,max_exposure_compensation);
        if ((min_exposure_compensation <= new_exposure_compensation)
            && (max_exposure_compensation >= new_exposure_compensation))
        {
            if (mFirstSetParameters || (now_exposure_compensation != new_exposure_compensation))
            {
                mParameters.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, new_exposure_compensation);
                mQueueElement[CMD_QUEUE_SET_EXPOSURE_COMPENSATION].cmd = CMD_QUEUE_SET_EXPOSURE_COMPENSATION;
                mQueueElement[CMD_QUEUE_SET_EXPOSURE_COMPENSATION].data = new_exposure_compensation;
                OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_SET_EXPOSURE_COMPENSATION]);
            }
        }
        else
        {
            LOGE("ERR(%s):invalid exposure compensation: %d", __FUNCTION__, new_exposure_compensation);
            return -EINVAL;
        }
    }

    // flash mode
    if (mCameraConfig->supportFlashMode())
    {
        unsigned long new_flash = -1;
        const char *new_flash_mode_str = params.get(CameraParameters::KEY_FLASH_MODE);
        mParameters.set(CameraParameters::KEY_FLASH_MODE, new_flash_mode_str);

        const char * valstr = mParameters.get(CameraParameters::KEY_RECORDING_HINT);
        bool video_hint = (strcmp(valstr, CameraParameters::TRUE) == 0);
        LOGV("%s, flash_mode = %s", __FUNCTION__, new_flash_mode_str);

        if (!strcmp(new_flash_mode_str, CameraParameters::FLASH_MODE_OFF))
            new_flash = V4L2_FLASH_LED_MODE_NONE;
        else if (!strcmp(new_flash_mode_str, CameraParameters::FLASH_MODE_AUTO))
            new_flash = V4L2_FLASH_LED_MODE_AUTO;
        else if (!strcmp(new_flash_mode_str, CameraParameters::FLASH_MODE_ON)){
            new_flash = V4L2_FLASH_LED_MODE_FLASH;
            //recording mode,Flash will keep in torch status
            if(video_hint) new_flash = V4L2_FLASH_LED_MODE_TORCH;
        }
        else if (!strcmp(new_flash_mode_str, CameraParameters::FLASH_MODE_TORCH))
            new_flash = V4L2_FLASH_LED_MODE_TORCH;
        else if (!strcmp(new_flash_mode_str, CameraParameters::FLASH_MODE_RED_EYE))
            new_flash = V4L2_FLASH_LED_MODE_RED_EYE;
        else{
            LOGE("ERR(%s):Invalid flash(%s)", __FUNCTION__, new_flash_mode_str); //twilight, shade
            ret = UNKNOWN_ERROR;
        }

        if (0 <= new_flash)
        {
            mParameters.set(CameraParameters::KEY_FLASH_MODE, new_flash_mode_str);
            mQueueElement[CMD_QUEUE_SET_FLASH_MODE].cmd = CMD_QUEUE_SET_FLASH_MODE;
            mQueueElement[CMD_QUEUE_SET_FLASH_MODE].data = new_flash;
            OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_SET_FLASH_MODE]);
        }
    }

    // zoom
    int max_zoom = mParameters.getInt(CameraParameters::KEY_MAX_ZOOM);
    int new_zoom = params.getInt(CameraParameters::KEY_ZOOM);
    LOGV("%s : new_zoom: %d", __FUNCTION__, new_zoom);
    if (0 <= new_zoom && new_zoom <= max_zoom)
    {
        mParameters.set(CameraParameters::KEY_ZOOM, new_zoom);
        pV4L2Device->setCrop(new_zoom + BASE_ZOOM, max_zoom);
        mZoomRatio = (new_zoom + BASE_ZOOM) * 2 * 100 / max_zoom + 100;
    }
    else
    {
        LOGE("ERR(%s): invalid zoom value: %d", __FUNCTION__, new_zoom);
        return -EINVAL;
    }

    // focus
    if (mCameraConfig->supportFocusMode())
    {
        const char *now_focus_mode_str = mParameters.get(CameraParameters::KEY_FOCUS_MODE);
        const char *now_focus_areas_str = mParameters.get(CameraParameters::KEY_FOCUS_AREAS);
        const char *new_focus_mode_str = params.get(CameraParameters::KEY_FOCUS_MODE);
        const char *new_focus_areas_str = params.get(CameraParameters::KEY_FOCUS_AREAS);

        if (!checkFocusArea(new_focus_areas_str))
        {
            LOGE("ERR(%s): invalid focus areas", __FUNCTION__);
            return -EINVAL;
        }

        if (!checkFocusMode(new_focus_mode_str))
        {
            LOGE("ERR(%s): invalid focus mode", __FUNCTION__);
			return -EINVAL;
        }

        if (mFirstSetParameters || strcmp(now_focus_mode_str, new_focus_mode_str))
        {
            mParameters.set(CameraParameters::KEY_FOCUS_MODE, new_focus_mode_str);
            mQueueElement[CMD_QUEUE_SET_FOCUS_MODE].cmd = CMD_QUEUE_SET_FOCUS_MODE;
            OSAL_QueueSetElem(&mQueueCommand, &mQueueElement[CMD_QUEUE_SET_FOCUS_MODE]);
        }

        // to do, check running??
        if (/*pV4L2Device->getThreadRunning()
            &&*/ strcmp(now_focus_areas_str, new_focus_areas_str))
        {
            mParameters.set(CameraParameters::KEY_FOCUS_AREAS, new_focus_areas_str);

#if 0
            strcpy(mFocusAreasStr, new_focus_areas_str);
            mQueueElement[CMD_QUEUE_SET_FOCUS_AREA].cmd = CMD_QUEUE_SET_FOCUS_AREA;
            mQueueElement[CMD_QUEUE_SET_FOCUS_AREA].data = mFocusAreasStr;
            OSAL_QueueSetElem(&mQueueCommand, &mQueueElement[CMD_QUEUE_SET_FOCUS_AREA]);
#else
            parse_focus_areas(new_focus_areas_str);
#endif
        }
    }
    else
    {
        const char *new_focus_mode_str = params.get(CameraParameters::KEY_FOCUS_MODE);
        if (strcmp(new_focus_mode_str, CameraParameters::FOCUS_MODE_FIXED))
        {
            LOGE("ERR(%s): invalid focus mode: %s", __FUNCTION__, new_focus_mode_str);
            return -EINVAL;
        }
        mParameters.set(CameraParameters::KEY_FOCUS_MODE, CameraParameters::FOCUS_MODE_FIXED);
    }

    // gps latitude
    const char *new_gps_latitude_str = params.get(CameraParameters::KEY_GPS_LATITUDE);
    if (new_gps_latitude_str) {
        LOGV("%s, new gps latitude = %s", __FUNCTION__, new_gps_latitude_str);
        mCallbackNotifier.setGPSLatitude(atof(new_gps_latitude_str));
        LOGV("%s, new gps latitude = %lf", __FUNCTION__, atof(new_gps_latitude_str));
        mParameters.set(CameraParameters::KEY_GPS_LATITUDE, new_gps_latitude_str);
    } else {
        mCallbackNotifier.setGPSLatitude(0.0);
        mParameters.remove(CameraParameters::KEY_GPS_LATITUDE);
    }

    // gps longitude
    const char *new_gps_longitude_str = params.get(CameraParameters::KEY_GPS_LONGITUDE);
    if (new_gps_longitude_str) {
        mCallbackNotifier.setGPSLongitude(atof(new_gps_longitude_str));
        mParameters.set(CameraParameters::KEY_GPS_LONGITUDE, new_gps_longitude_str);
    } else {
        mCallbackNotifier.setGPSLongitude(0.0);
        mParameters.remove(CameraParameters::KEY_GPS_LONGITUDE);
    }

    // gps altitude
    const char *new_gps_altitude_str = params.get(CameraParameters::KEY_GPS_ALTITUDE);
    if (new_gps_altitude_str) {
        mCallbackNotifier.setGPSAltitude(atof(new_gps_altitude_str));
        mParameters.set(CameraParameters::KEY_GPS_ALTITUDE, new_gps_altitude_str);
    } else {
        mCallbackNotifier.setGPSAltitude(0);
        mParameters.remove(CameraParameters::KEY_GPS_ALTITUDE);
    }

    // gps timestamp
    const char *new_gps_timestamp_str = params.get(CameraParameters::KEY_GPS_TIMESTAMP);
    if (new_gps_timestamp_str) {
        mCallbackNotifier.setGPSTimestamp(atol(new_gps_timestamp_str));
        mParameters.set(CameraParameters::KEY_GPS_TIMESTAMP, new_gps_timestamp_str);
    } else {
        mCallbackNotifier.setGPSTimestamp(0);
        mParameters.remove(CameraParameters::KEY_GPS_TIMESTAMP);
    }

    // gps processing method
    const char *new_gps_processing_method_str = params.get(CameraParameters::KEY_GPS_PROCESSING_METHOD);
    LOGV("%s, new gps processing method = %s", __FUNCTION__, new_gps_processing_method_str);
    if (new_gps_processing_method_str) {
        mCallbackNotifier.setGPSMethod(new_gps_processing_method_str);
        mParameters.set(CameraParameters::KEY_GPS_PROCESSING_METHOD, new_gps_processing_method_str);
    } else {
        mCallbackNotifier.setGPSMethod("");
        mParameters.remove(CameraParameters::KEY_GPS_PROCESSING_METHOD);
    }

    // JPEG thumbnail size
    int new_jpeg_thumbnail_width = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH);
    int new_jpeg_thumbnail_height= params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT);
    LOGV("%s : new_jpeg_thumbnail_width: %d, new_jpeg_thumbnail_height: %d",
        __FUNCTION__, new_jpeg_thumbnail_width, new_jpeg_thumbnail_height);
    if (0 <= new_jpeg_thumbnail_width && 0 <= new_jpeg_thumbnail_height) {
        mCallbackNotifier.setJpegThumbnailSize(new_jpeg_thumbnail_width, new_jpeg_thumbnail_height);
        mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, new_jpeg_thumbnail_width);
        mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, new_jpeg_thumbnail_height);
    }
#ifdef USE_DEINTERLACE_HW
        int enable_yuv422 =  params.getInt("enable-yuv422");
        LOGD("%s : enable-yuv422 = %d", __FUNCTION__, enable_yuv422);
        if (enable_yuv422 >= 0) {
            mV4L2CameraDevice->mEnableYUV422 = enable_yuv422;
            LOGD("%s : mV4L2CameraDevice->mEnableYUV422 = %d", __FUNCTION__, mV4L2CameraDevice->mEnableYUV422);
        }
#endif


    mFirstSetParameters = false;
    pthread_cond_signal(&mCommandCond);

    return NO_ERROR;
}

/* A dumb variable indicating "no params" / error on the exit from
 * CameraHardware::getParameters(). */
static char lNoParam = '\0';
char* CameraHardware::getParameters()
{
    F_LOG;
    String8 params(mParameters.flatten());
    char* ret_str =
        reinterpret_cast<char*>(malloc(sizeof(char) * (params.length()+1)));
    memset(ret_str, 0, params.length()+1);
    if (ret_str != NULL) {
        strncpy(ret_str, params.string(), params.length()+1);
        return ret_str;
    } else {
        LOGE("%s: Unable to allocate string for %s", __FUNCTION__, params.string());
        /* Apparently, we can't return NULL fron this routine. */
        return &lNoParam;
    }
}

void CameraHardware::putParameters(char* params)
{
    F_LOG;
    /* This method simply frees parameters allocated in getParameters(). */
    if (params != NULL && params != &lNoParam) {
        free(params);
    }
}

status_t CameraHardware::setFd(int fd)
{
    mCallbackNotifier.setFd(fd);
    return NO_ERROR;
}
void CameraHardware::setNewCrop(Rect * rect)
{
    F_LOG;
    memcpy(&mFrameRectCrop, rect, sizeof(Rect));
}

status_t CameraHardware::sendCommand(int32_t cmd, int32_t arg1, int32_t arg2)
{
    LOGV("%s: cmd = %x, arg1 = %d, arg2 = %d", __FUNCTION__, cmd, arg1, arg2);

    /* TODO: Future enhancements. */

    switch (cmd)
    {
    //case CAMERA_CMD_SET_CEDARX_RECORDER:
    //    mUseHwEncoder = true;
    //    mV4L2CameraDevice->setHwEncoder(true);
    //    return OK;
    case CAMERA_CMD_START_FACE_DETECTION:
    {
        const char *face = mParameters.get(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW);
        if (face == NULL || (atoi(face) <= 0))
        {
            return -EINVAL;
        }
            pthread_mutex_lock(&mCommandMutex);
        mQueueElement[CMD_QUEUE_START_FACE_DETECTE].cmd = CMD_QUEUE_START_FACE_DETECTE;
        OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_START_FACE_DETECTE]);
        pthread_cond_signal(&mCommandCond);
            pthread_mutex_unlock(&mCommandMutex);

            // start face detection thread
            mFaceDetectionThread->startThread();
        return OK;
    }
        case CAMERA_CMD_STOP_FACE_DETECTION:
            // stop face detection thread
            pthread_mutex_lock(&mFaceDetectionMutex);
            if (mFaceDetectionThread->isThreadStarted())
            {
                mFaceDetectionThread->stopThread();
                pthread_cond_signal(&mFaceDetectionCond);
            }
            pthread_mutex_unlock(&mFaceDetectionMutex);
            pthread_mutex_lock(&mCommandMutex);
        mQueueElement[CMD_QUEUE_STOP_FACE_DETECTE].cmd = CMD_QUEUE_STOP_FACE_DETECTE;
        OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_STOP_FACE_DETECTE]);
        pthread_cond_signal(&mCommandCond);
            pthread_mutex_unlock(&mCommandMutex);
        return OK;
    case CAMERA_CMD_PING:
        return OK;
    case CAMERA_CMD_ENABLE_FOCUS_MOVE_MSG:
    {
        bool enable = static_cast<bool>(arg1);
        if (enable) {
            enableMsgType(CAMERA_MSG_FOCUS_MOVE);
        } else {
            disableMsgType(CAMERA_MSG_FOCUS_MOVE);
        }
        return OK;
    }
    case CAMERA_CMD_SET_DISPLAY_ORIENTATION:
        {
        LOGD("CAMERA_CMD_SET_DISPLAY_ORIENTATION, to do ...");
        return OK;
        }
        /*case CAMERA_CMD_START_SMART_DETECTION:
        {
            ALOGD("CAMERA_CMD_START_SMART_DETECTION");
            //enableSmartMsgType(CAMERA_SMART_MSG_STATUS);

            if (!mSmartDetectionEnable)
            {
                status_t result = doStartSmart();
                if (mSmartData == NULL)
                    mSmartData = (unsigned char*)malloc(1920*1080);
                if (mSmartDetection == NULL)
                {
                    //create ApperceivePeopleDev object
                    //CreateApperceivePeopleDev(&mSmartDetection);
                    if (mSmartDetection == NULL)
                    {
                        ALOGE("create ApperceivePeopleDev failed");
                    }
                    #if 1
                    mSmartMode = arg1;
                    if (mSmartMode == 0x03)
                    {
                        mSmartDiscardFrameNum = 5;
                    }
                    else
                    {
                        mSmartDiscardFrameNum = 0;
                    }
                    #endif
                    mSmartDetection->ioctrl(mSmartDetection, APPERCEIVEPEOPLE_OPS_CMD_REGISTE_USER, (unsigned long)this, 0, NULL, mSmartMode);
                    mSmartDetection->setCallback(mSmartDetection, ApperceiveNotifyCb);
                }
                mSmartDetectionEnable = true;
                // start smart thread
                ALOGD("mSmartThread start");
                mSmartThread->startThread();
                pthread_cond_signal(&mSmartCond);
            }

            return OK;
        }*/
        /*case CAMERA_CMD_STOP_SMART_DETECTION:

            ALOGD("CAMERA_CMD_STOP_SMART_DETECTION");
            //disableSmartMsgType(CAMERA_SMART_MSG_STATUS);
            if (mSmartDetectionEnable)
            {
                pthread_mutex_lock(&mSmartMutex);
                if (mSmartThread->isThreadStarted())
                {
                    mSmartThread->stopThread();
                    pthread_cond_signal(&mSmartCond);
                }
                pthread_mutex_unlock(&mSmartMutex);
                mSmartDiscardFrameNum = 0;

                doStopSmart();
                mSmartDetectionEnable = false;

                if (mSmartDetection != NULL)
                {
                    //DestroyApperceivePeopleDev(mSmartDetection);
                    mSmartDetection = NULL;
                }

                if (mSmartData != NULL)
                {
                    free(mSmartData);
                    mSmartData = NULL;
                }
            }

            return OK;*/
        default:
            break;
    }

    return -EINVAL;
}

void CameraHardware::releaseCamera()
{
    LOGV("%s", __FUNCTION__);

    cleanupCamera();
}

status_t CameraHardware::dumpCamera(int fd)
{
    LOGV("%s", __FUNCTION__);

    /* TODO: Future enhancements. */
    return OK;
}

/****************************************************************************
 * Facedetection management
 ***************************************************************************/

int CameraHardware::getCurrentFaceFrame(void* frame, int* width, int* height)
{
    return mV4L2CameraDevice->getCurrentFaceFrame(frame, width, height);
}

int CameraHardware::faceDetection(camera_frame_metadata_t *face)
{
    int number_of_faces = face->number_of_faces;
    if (number_of_faces == 0)
    {
        parse_focus_areas("(0, 0, 0, 0, 0)", true);
    }
    else
    {
        if (mZoomRatio != 0)
        {
            for(int i =0; i < number_of_faces; i++)
            {
                face->faces[i].rect[0] = (face->faces[i].rect[0] * mZoomRatio) / 100;
                face->faces[i].rect[1] = (face->faces[i].rect[1] * mZoomRatio) / 100;
                face->faces[i].rect[2] = (face->faces[i].rect[2] * mZoomRatio) / 100;
                face->faces[i].rect[3] = (face->faces[i].rect[3] * mZoomRatio) / 100;
            }
        }
    }

    if ((mFrameFaceData.faceNum > 0) &&
        (mSmileDetectionEnable || mBlinkDetectionEnable))
    {
        pthread_cond_signal(&mFaceDetectionCond);
    }
    else
    {
        pthread_mutex_lock(&mFaceDetectionStateMutex);

        mSmileDetectionState = FACE_DETECTION_IDLE;
        mBlinkDetectionState = FACE_DETECTION_IDLE;

        mSmilePictureResult = false;
        mBlinkPictureResult = false;

        pthread_mutex_unlock(&mFaceDetectionStateMutex);
    }
    return mCallbackNotifier.faceDetectionMsg(face);
}
/*int CameraHardware::smileDetection(camera_face_smile_status_t *smile)
{
        int number_of_smiles = smile->number_of_smiles;
        if (number_of_smiles <= 0)
        {
            smile->number_of_smiles = 0;
            //smile->smiles = NULL;
        }

        pthread_mutex_lock(&mFaceDetectionStateMutex);
        {
            int i;
            ALOGD("smile->number_of_smiles %d", smile->number_of_smiles);

            mSmilePictureResult = false;
            for(i=0; i<smile->number_of_smiles; i++)
            {
                if ((int)(smile->smiles[i]) == 1)
                {
                    mSmilePictureResult = true;
                    ALOGD("mSmilePictureResult %d", mSmilePictureResult);
                    break;
                }
            }
        }

        mSmileDetectionState = FACE_DETECTION_PREPARED;
        pthread_mutex_unlock(&mFaceDetectionStateMutex);

    //return mCallbackNotifier.smileDetectionMsg(smile);
    return NO_ERROR;
}*/

/*int CameraHardware::blinkDetection(camera_face_blink_status_t *blink)
{
    int number_of_blinks = blink->number_of_blinks;
    if (number_of_blinks <= 0)
    {
        blink->number_of_blinks = 0;
        //blink->blinks = NULL;
    }

    pthread_mutex_lock(&mFaceDetectionStateMutex);
    {
        int i;
        ALOGD("blink->number_of_blinks %d", blink->number_of_blinks);

        mBlinkPictureResult = false;
        for(i=0; i<blink->number_of_blinks; i++)
        {
            if (blink->blinks[i] == 1)
            {
                mBlinkPictureResult = true;
                ALOGD("mBlinkPictureResult %d", mBlinkPictureResult);
                break;
            }
        }
    }
    mBlinkDetectionState = FACE_DETECTION_PREPARED;

    pthread_mutex_unlock(&mFaceDetectionStateMutex);

    //return mCallbackNotifier.blinkDetectionMsg(blink);
    return NO_ERROR;
}*/

/*int CameraHardware::smartDetection(int type)
{
    ALOGD("smartDetection type %d", type);
    //pthread_mutex_lock(&mSmartMutex);
    //pthread_cond_signal(&mSmartCond);
    //pthread_mutex_unlock(&mSmartMutex);

    return mCallbackNotifier.smartDetectionMsg(type);
}*/

/****************************************************************************
 * Preview management.
 ***************************************************************************/

status_t CameraHardware::doStartPreview()
{
    F_LOG;

    V4L2CameraDevice* const camera_dev = mV4L2CameraDevice;

    if (camera_dev->isStarted()
        && mPreviewWindow.isPreviewEnabled())
    {
        LOGD("CameraHardware::doStartPreview has been already started");
        return NO_ERROR;
    }

    if (camera_dev->isStarted())
    {
        camera_dev->stopDeliveringFrames();
        camera_dev->stopDevice();
    }

    status_t res = mPreviewWindow.startPreview();
    if (res != NO_ERROR)
    {
        return res;
    }

    // Make sure camera device is connected.
    if (!camera_dev->isConnected())
    {
        res = camera_dev->connectDevice(&mHalCameraInfo);
        if (res != NO_ERROR)
        {
            mPreviewWindow.stopPreview();
            return res;
        }

        camera_dev->setAutoFocusInit();
    }

    //make facedetection oriention arrary
    makeFDOrientionArray();
    const char * valstr = mParameters.get(CameraParameters::KEY_RECORDING_HINT);
    bool video_hint = (strcmp(valstr, CameraParameters::TRUE) == 0);
    if(strcmp(mCallingProcessName, "com.android.cts.verifier") == 0) //Add for CTS
        video_hint = 1;

    // preview size
    int preview_width = 0, preview_height = 0;
    const char * preview_capture_width_str = mParameters.get(KEY_PREVIEW_CAPTURE_SIZE_WIDTH);
    const char * preview_capture_height_str = mParameters.get(KEY_PREVIEW_CAPTURE_SIZE_HEIGHT);
    if (preview_capture_width_str != NULL
        && preview_capture_height_str != NULL)
    {
        preview_width  = atoi(preview_capture_width_str);
        preview_height = atoi(preview_capture_height_str);
    }

    // video size
    int video_width = 0, video_height = 0;
    mParameters.getVideoSize(&video_width, &video_height);
    // capture size
    if (video_hint)
    {
        if ((isRecordingEnabled())||(strcmp(mCallingProcessName, "android.camera.cts") == 0))
        {
            mCaptureWidth = video_width;
            mCaptureHeight= video_height;
        }else
        {
            mCaptureWidth = preview_width;
            mCaptureHeight= preview_height;
        }
    }
    else
    {
        if (mHalCameraInfo.fast_picture_mode)
        {
            mCaptureWidth = mFullSizeWidth;
            mCaptureHeight= mFullSizeHeight;
        }
        else
        {
            mCaptureWidth = preview_width;
            mCaptureHeight= preview_height;
        }
    }

    if(strcmp(mCallingProcessName, "com.android.cts.verifier") == 0)  //add for CTS Verifier by fuqiang
    {
        mCaptureWidth = preview_width;
        mCaptureHeight= preview_height;
    }

    // preview format and video format are the same
    uint32_t org_fmt = V4L2_PIX_FMT_NV21;        // android default
    const char* preview_format = mParameters.getPreviewFormat();
    if (preview_format != NULL)
    {
        if (strcmp(preview_format, CameraParameters::PIXEL_FORMAT_YUV420SP) == 0)
        {
#ifdef __SUN6I__
            org_fmt = V4L2_PIX_FMT_NV12;        // SGX support NV12
#else
            org_fmt = V4L2_PIX_FMT_NV21;        // MALI support NV21
#endif
        }
        else if (strcmp(preview_format, CameraParameters::PIXEL_FORMAT_YUV420P) == 0)
        {
            org_fmt = V4L2_PIX_FMT_YVU420;        // YV12
        }
        else
        {
            LOGE("unknown preview format");
        }
    }

    LOGD("Starting camera, Size:%dx%d , picture format:%s",
         mCaptureWidth, mCaptureHeight, preview_format);
    camera_dev->showformat(org_fmt, "preview");
    res = camera_dev->startDevice(mCaptureWidth, mCaptureHeight, org_fmt, video_hint);
    if (res != NO_ERROR)
    {
        mPreviewWindow.stopPreview();
        return res;
    }
    if (mCameraConfig->supportFocusMode())
    setAutoFocusRange();
    res = camera_dev->startDeliveringFrames();
    if (res != NO_ERROR)
    {
        camera_dev->stopDevice();
        mPreviewWindow.stopPreview();
    }

    return res;
}

status_t CameraHardware::doStopPreview()
{
    F_LOG;

    status_t res = NO_ERROR;
    if (mPreviewWindow.isPreviewEnabled())
    {
        /* Stop the camera. */
        if (mV4L2CameraDevice->isStarted())
        {
            mV4L2CameraDevice->stopDeliveringFrames();
            mV4L2CameraDevice->stopPreviewThread();
            res = mV4L2CameraDevice->stopDevice();
        }

        if (res == NO_ERROR)
        {
            /* Disable preview as well. */
            mPreviewWindow.stopPreview();
        }
    }

    return NO_ERROR;
}

/****************************************************************************
 * Private API.
 ***************************************************************************/

status_t CameraHardware::cleanupCamera()
{
    F_LOG;

    status_t res = NO_ERROR;
    //add for cts by clx
      mParameters.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK, false);

    mParameters.set(KEY_SNAP_PATH, "");
    mCallbackNotifier.setSnapPath("");

    mParameters.set(KEY_PICTURE_MODE, "normal");
    // reset preview format to yuv420sp
    mParameters.set(CameraParameters::KEY_PREVIEW_FORMAT, CameraParameters::PIXEL_FORMAT_YUV420SP);

    mV4L2CameraDevice->setHwEncoder(false);

    mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, 320);
    mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, 240);

    mParameters.set(CameraParameters::KEY_ZOOM, 0);

    mVideoCaptureWidth = 0;
    mVideoCaptureHeight = 0;
    mUseHwEncoder = false;
    // stop smart detection
    //disableSmartMsgType(CAMERA_SMART_MSG_STATUS);
    if (mSmartDetectionEnable)
    {
        pthread_mutex_lock(&mSmartMutex);
        if (mSmartThread->isThreadStarted())
        {
            mSmartThread->stopThread();
            pthread_cond_signal(&mSmartCond);
        }
        pthread_mutex_unlock(&mSmartMutex);
        mSmartDiscardFrameNum = 0;

        doStopSmart();
        mSmartDetectionEnable = false;

        if (mSmartDetection != NULL)
        {
//            DestroyApperceivePeopleDev(mSmartDetection);
            mSmartDetection = NULL;
        }

        if (mSmartData != NULL)
        {
            free(mSmartData);
            mSmartData = NULL;
        }
    }

    // stop focus thread
    pthread_mutex_lock(&mAutoFocusMutex);
    if (mAutoFocusThread->isThreadStarted())
    {
        mAutoFocusThread->stopThread();
        pthread_cond_signal(&mAutoFocusCond);
    }
    pthread_mutex_unlock(&mAutoFocusMutex);

    if (mCameraConfig->supportFocusMode())
    {
        // wait for auto focus thread exit
        pthread_mutex_lock(&mAutoFocusMutex);
        if (!mAutoFocusThreadExit)
        {
            LOGW("wait for auto focus thread exit");
            pthread_cond_wait(&mAutoFocusCond, &mAutoFocusMutex);
        }
        pthread_mutex_unlock(&mAutoFocusMutex);
    }
    usleep(100000);    // tmp for CTS

    /* If preview is running - stop it. */
    res = doStopPreview();
    if (res != NO_ERROR) {
        return -res;
    }

    /* Stop and disconnect the camera device. */
    V4L2CameraDevice* const camera_dev = mV4L2CameraDevice;
    //stop flash
    if (mCameraConfig->supportFlashMode())
        camera_dev->setFlashMode(V4L2_FLASH_LED_MODE_NONE);
    if (camera_dev != NULL)
    {
        if (camera_dev->isStarted())
        {
            camera_dev->stopDeliveringFrames();
            res = camera_dev->stopDevice();
            if (res != NO_ERROR) {
                return -res;
            }
        }
        if (camera_dev->isConnected())
        {
            res = camera_dev->disconnectDevice();
            if (res != NO_ERROR) {
                return -res;
            }
        }
    }

    mCallbackNotifier.cleanupCBNotifier();

    {
        Mutex::Autolock locker(&mCameraIdleLock);
        mIsCameraIdle = true;
    }
/*
    if (mBlinkDetection != NULL)
    {
        DestroyEyeBlinkDetectionDev(mBlinkDetection);
        mBlinkDetection = NULL;
    }*/
/*
    if (mSmileDetection != NULL)
    {
        DestroySmileDetectionDev(mSmileDetection);
        mSmileDetection = NULL;
    }
*/
    if (mFaceDetection != NULL)
    {
        //DestroyFaceDetectionDev(mFaceDetection);
        mFaceDetection = NULL;
    }

    if (mFrameData != NULL)
    {
        free(mFrameData);
        mFrameData = NULL;
    }

    if (mFrameFaceData.frameData != NULL)
    {
        free(mFrameFaceData.frameData);
        mFrameFaceData.frameData = NULL;
    }
    return NO_ERROR;
}

/****************************************************************************
 * Camera API callbacks as defined by camera_device_ops structure.
 *
 * Callbacks here simply dispatch the calls to an appropriate method inside
 * CameraHardware instance, defined by the 'dev' parameter.
 ***************************************************************************/

int CameraHardware::set_preview_window(struct camera_device* dev,
                                       struct preview_stream_ops* window)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        LOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->setPreviewWindow(window);
}

void CameraHardware::set_callbacks(
        struct camera_device* dev,
        camera_notify_callback notify_cb,
        camera_data_callback data_cb,
        camera_data_timestamp_callback data_cb_timestamp,
        camera_request_memory get_memory,
        void* user)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        LOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return;
    }
    ec->setCallbacks(notify_cb, data_cb, data_cb_timestamp, get_memory, user);
}

void CameraHardware::enable_msg_type(struct camera_device* dev, int32_t msg_type)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        LOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return;
    }
    ec->enableMsgType(msg_type);
}

void CameraHardware::disable_msg_type(struct camera_device* dev, int32_t msg_type)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        LOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return;
    }
    ec->disableMsgType(msg_type);
}

int CameraHardware::msg_type_enabled(struct camera_device* dev, int32_t msg_type)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        LOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->isMsgTypeEnabled(msg_type);
}

int CameraHardware::start_preview(struct camera_device* dev)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        LOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->startPreview();
}

void CameraHardware::stop_preview(struct camera_device* dev)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        LOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return;
    }
    ec->stopPreview();
}

int CameraHardware::preview_enabled(struct camera_device* dev)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        LOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->isPreviewEnabled();
}

/*int CameraHardware::enable_preview(struct camera_device* dev)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        LOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->enablePreview();
}*/

/*int CameraHardware::disable_preview(struct camera_device* dev)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        LOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->disablePreview();
}*/

int CameraHardware::store_meta_data_in_buffers(struct camera_device* dev,
                                               int enable)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        LOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->storeMetaDataInBuffers(enable);
}

int CameraHardware::start_recording(struct camera_device* dev)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        LOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->startRecording();
}

void CameraHardware::stop_recording(struct camera_device* dev)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        LOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return;
    }
    ec->stopRecording();
}

int CameraHardware::recording_enabled(struct camera_device* dev)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        LOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->isRecordingEnabled();
}

void CameraHardware::release_recording_frame(struct camera_device* dev,
                                             const void* opaque)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        LOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return;
    }
    ec->releaseRecordingFrame(opaque);
}

int CameraHardware::auto_focus(struct camera_device* dev)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        LOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->setAutoFocus();
}

int CameraHardware::cancel_auto_focus(struct camera_device* dev)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        LOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->cancelAutoFocus();
}

int CameraHardware::take_picture(struct camera_device* dev)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        LOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->takePicture();
}

int CameraHardware::cancel_picture(struct camera_device* dev)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        LOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->cancelPicture();
}

int CameraHardware::set_parameters(struct camera_device* dev, const char* parms)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        LOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }

    int64_t lasttime = systemTime();
    int ret = ec->setParameters(parms);
    LOGV("setParameters use time: %lld(ms)", (systemTime() - lasttime)/1000000);

    return ret;
}

/*int CameraHardware::set_fd(struct camera_device* dev, int fd)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        LOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }

    int ret = ec->setFd(fd);
    return ret;
}*/
char* CameraHardware::get_parameters(struct camera_device* dev)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        LOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return NULL;
    }
    return ec->getParameters();
}

void CameraHardware::put_parameters(struct camera_device* dev, char* params)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        LOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return;
    }
    ec->putParameters(params);
}

int CameraHardware::send_command(struct camera_device* dev,
                                 int32_t cmd,
                                 int32_t arg1,
                                 int32_t arg2)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        LOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->sendCommand(cmd, arg1, arg2);
}

void CameraHardware::release(struct camera_device* dev)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        LOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return;
    }
    ec->releaseCamera();
}

int CameraHardware::dump(struct camera_device* dev, int fd)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        LOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->dumpCamera(fd);
}

int CameraHardware::close(struct hw_device_t* device)
{
    CameraHardware* ec =
        reinterpret_cast<CameraHardware*>(reinterpret_cast<struct camera_device*>(device)->priv);
    if (ec == NULL) {
        LOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->closeCamera();
}

HALCameraInfo* CameraHardware::get_halinfo()
{
    return &mHalCameraInfo;
}
// -------------------------------------------------------------------------
// extended interfaces here <***** star *****>
// -------------------------------------------------------------------------

/****************************************************************************
 * Static initializer for the camera callback API
 ****************************************************************************/

camera_device_ops_t CameraHardware::mDeviceOps = {
    CameraHardware::set_preview_window,
    CameraHardware::set_callbacks,
    CameraHardware::enable_msg_type,
    CameraHardware::disable_msg_type,
    CameraHardware::msg_type_enabled,
    CameraHardware::start_preview,
    CameraHardware::stop_preview,
    CameraHardware::preview_enabled,
    //CameraHardware::enable_preview,
    //CameraHardware::disable_preview,
    CameraHardware::store_meta_data_in_buffers,
    CameraHardware::start_recording,
    CameraHardware::stop_recording,
    CameraHardware::recording_enabled,
    CameraHardware::release_recording_frame,
    CameraHardware::auto_focus,
    CameraHardware::cancel_auto_focus,
    CameraHardware::take_picture,
    CameraHardware::cancel_picture,
    CameraHardware::set_parameters,
    CameraHardware::get_parameters,
    CameraHardware::put_parameters,
    CameraHardware::send_command,
    CameraHardware::release,
    CameraHardware::dump,
    //CameraHardware::set_fd
};

/****************************************************************************
 * Common keys
 ***************************************************************************/

const char CameraHardware::FACING_KEY[]         = "prop-facing";
const char CameraHardware::ORIENTATION_KEY[]    = "prop-orientation";
const char CameraHardware::RECORDING_HINT_KEY[] = "recording-hint";

/****************************************************************************
 * Common string values
 ***************************************************************************/

const char CameraHardware::FACING_BACK[]      = "back";
const char CameraHardware::FACING_FRONT[]     = "front";

/****************************************************************************
 * Helper routines
 ***************************************************************************/

static char* AddValue(const char* param, const char* val)
{
    const size_t len1 = strlen(param);
    const size_t len2 = strlen(val);
    char* ret = reinterpret_cast<char*>(malloc(len1 + len2 + 2));
    LOGE_IF(ret == NULL, "%s: Memory failure", __FUNCTION__);
    if (ret != NULL) {
        memcpy(ret, param, len1);
        ret[len1] = ',';
        memcpy(ret + len1 + 1, val, len2);
        ret[len1 + len2 + 1] = '\0';
    }
    return ret;
}

/****************************************************************************
 * Parameter debugging helpers
 ***************************************************************************/

#if DEBUG_PARAM
static void PrintParamDiff(const CameraParameters& current,
                            const char* new_par)
{
    char tmp[2048];
    const char* wrk = new_par;

    /* Divided with ';' */
    const char* next = strchr(wrk, ';');
    while (next != NULL) {
        snprintf(tmp, sizeof(tmp), "%.*s", next-wrk, wrk);
        /* in the form key=value */
        char* val = strchr(tmp, '=');
        if (val != NULL) {
            *val = '\0'; val++;
            const char* in_current = current.get(tmp);
            if (in_current != NULL) {
                if (strcmp(in_current, val)) {
                    LOGD("=== Value changed: %s: %s -> %s", tmp, in_current, val);
                }
            } else {
                LOGD("+++ New parameter: %s=%s", tmp, val);
            }
        } else {
            LOGW("No value separator in %s", tmp);
        }
        wrk = next + 1;
        next = strchr(wrk, ';');
    }
}
#endif  /* DEBUG_PARAM */

}; /* namespace android */
