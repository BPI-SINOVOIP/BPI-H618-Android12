
#include "CameraDebug.h"
#if DBG_CAMERA_FACTORY
#define LOG_NDEBUG 0
#endif
#define LOG_TAG "HALCameraFactory"
#include <cutils/log.h>
#include <android/log.h>

#include <binder/IPCThreadState.h>
#include <cutils/properties.h>

#include "HALCameraFactory.h"
#include "CCameraConfig.h"

extern camera_module_t HAL_MODULE_INFO_SYM;

/* A global instance of HALCameraFactory is statically instantiated and
 * initialized when camera emulation HAL is loaded.
 */
android::HALCameraFactory  gEmulatedCameraFactory;

namespace android {

#define GET_CALLING_PID    (IPCThreadState::self()->getCallingPid())

#if 0
void getCallingProcessName(char *name)
{
    char proc_node[128];

    if (name == 0)
    {
        LOGE("error in params");
        return;
    }

    memset(proc_node, 0, sizeof(proc_node));
    sprintf(proc_node, "/proc/%d/cmdline", GET_CALLING_PID);
    int fp = ::open(proc_node, O_RDONLY);
    if (fp > 0)
    {
        memset(name, 0, 128);
        ::read(fp, name, 128);
        ::close(fp);
        fp = 0;
        LOGD("Calling process is: %s", name);
    }
    else
    {
        LOGE("Obtain calling process failed");
    }
}
#endif

void getCallingProcessName(char *name)
{
    char value[1024];
    if(name == 0)
     {
         ALOGE("error in params");
         return;
     }
    int getpersist;
    getpersist = property_get("persist.sys.cts", value, NULL);
    ALOGD("Initialize getpersist = %d value=%s", getpersist, value);
    strcpy(name, value);
}

HALCameraFactory::HALCameraFactory()
        : mHardwareCameras(NULL),
          mAttachedCamerasNum(0),
          mRemovableCamerasNum(0),
#ifdef CAMERA_MANAGER_ENABLE
          mCameraManager(NULL),
#endif
          mConstructedOK(false)
{
    F_LOG;

    LOGD("camera hal version: %s", CAMERA_HAL_VERSION);

    memset(&mHalCameraInfo, 0, MAX_NUM_OF_CAMERAS * sizeof(HALCameraInfo));

    /* Make sure that array is allocated. */
    if (mHardwareCameras == NULL) {
        mHardwareCameras = new CameraHardware*[MAX_NUM_OF_CAMERAS];
        if (mHardwareCameras == NULL) {
            LOGE("%s: Unable to allocate V4L2Camera array for %d entries",
                 __FUNCTION__, MAX_NUM_OF_CAMERAS);
            return;
        }
        memset(mHardwareCameras, 0, MAX_NUM_OF_CAMERAS * sizeof(CameraHardware*));
    }

    /* Create the cameras */
    for (int id = 0; id < MAX_NUM_OF_CAMERAS; id++)
    {
        // camera config information
        mCameraConfig[id] = new CCameraConfig(0);
        if(mCameraConfig[id] == 0)
        {
            LOGW("create CCameraConfig failed");
        }
        else
        {
            mCameraConfig[id]->initParameters();
            mCameraConfig[id]->dumpParameters();
        }

        mHardwareCameras[id] = new CameraHardware(&HAL_MODULE_INFO_SYM.common, mCameraConfig[id]);
        if (mHardwareCameras[id] == NULL)
        {
            mHardwareCameras--;
            LOGE("%s: Unable to instantiate fake camera class", __FUNCTION__);
            return;
        }
    }

    // check camera cfg
    if (mCameraConfig[0] != NULL)
    {
        mAttachedCamerasNum = mCameraConfig[0]->numberOfCamera();

        if ((mAttachedCamerasNum == 2)
            && (mCameraConfig[1] == NULL))
        {
            return;
        }
    }
#ifdef CAMERA_MANAGER_ENABLE
    mCameraManager = new CameraManager();
#endif
    mConstructedOK = true;
}

HALCameraFactory::~HALCameraFactory()
{
    F_LOG;
    if (mHardwareCameras != NULL)
    {
        for (int n = 0; n < MAX_NUM_OF_CAMERAS; n++)
        {
            if (mHardwareCameras[n] != NULL)
            {
                delete mHardwareCameras[n];
                mHardwareCameras[n] = NULL;
            }
        }
        delete[] mHardwareCameras;
        mHardwareCameras = NULL;
    }

    for (int n = 0; n < MAX_NUM_OF_CAMERAS; n++)
    {
        if (mCameraConfig[n] != NULL)
        {
            delete mCameraConfig[n];
            mCameraConfig[n] = NULL;
        }
    }
#ifdef CAMERA_MANAGER_ENABLE
    if (mCameraManager != NULL)
    {
        delete mCameraManager;
        mCameraManager = NULL;
    }
#endif
}

/****************************************************************************
 * Camera HAL API handlers.
 *
 * Each handler simply verifies existence of an appropriate CameraHardware
 * instance, and dispatches the call to that instance.
 *
 ***************************************************************************/

int HALCameraFactory::getCameraHardwareNum()
{
    int ret = 0;
    char dev_node[16];
    int orientation = 0;

    if (mCameraConfig[0] != NULL)
    {
        orientation = mCameraConfig[0]->getCameraOrientation();
    }

    mRemovableCamerasNum = 0;

    // there are two attached cameras.
    for (int i = 0; i < MAX_NUM_OF_CAMERAS; i++) {
        sprintf(dev_node, "/dev/video%d", i);
        strcpy(mHalCameraInfo[mRemovableCamerasNum].device_name, dev_node);
        mHalCameraInfo[mRemovableCamerasNum].device_id = i;
        mRemovableCamerasNum++;

        mHalCameraInfo[i].facing        = CAMERA_FACING_BACK;
        mHalCameraInfo[i].orientation    = orientation;
        mHalCameraInfo[i].fast_picture_mode    = false;
        mHalCameraInfo[i].is_uvc        = false;
    }


    return MAX_NUM_OF_CAMERAS;
}

int HALCameraFactory::getCameraInfo(int camera_id, struct camera_info* info)
{
    //LOGV("%s: id = %d", __FUNCTION__, camera_id);

    int total_num_of_cameras = MAX_NUM_OF_CAMERAS;//mAttachedCamerasNum + mRemovableCamerasNum;
    char calling_process[256];

    if (!isConstructedOK()) {
        LOGE("%s: HALCameraFactory has failed to initialize", __FUNCTION__);
        return -EINVAL;
    }

    if (camera_id < 0 || camera_id >= total_num_of_cameras) {
        LOGE("%s: Camera id %d is out of bounds (%d)",
             __FUNCTION__, camera_id, total_num_of_cameras);
        return -EINVAL;
    }

    info->orientation    = mHalCameraInfo[camera_id].orientation;
    info->facing        = mHalCameraInfo[camera_id].facing;
    info->static_camera_characteristics = NULL;

    // single camera
    if (total_num_of_cameras == 1)
    {
        getCallingProcessName(calling_process);
        if ((strcmp(calling_process, "com.tencent.mobileqq") == 0)
            || (strcmp(calling_process, "com.tencent.mobileqq:video") == 0))
        {
            // cts, mobile qq need facing-back camera
            info->facing = CAMERA_FACING_BACK;
        }
        else if ((strcmp(calling_process, "com.google.android.talk") == 0)
            || (strcmp(calling_process, "com.android.facelock") == 0))
        {
            // gtalk, facelock need facing-front camera
            info->facing = CAMERA_FACING_FRONT;
        }
    }

    char property[PROPERTY_VALUE_MAX];
    if (property_get("ro.sf.hwrotation", property, NULL) > 0)
    {
        //displayOrientation
        switch (atoi(property))
        {
        case 270:
            if(info->facing == CAMERA_FACING_BACK)
            {
                info->orientation = (info->orientation + 90) % 360;
            }
            else if(info->facing == CAMERA_FACING_FRONT)
            {
                info->orientation = (info->orientation + 270) % 360;
            }
            break;
        }
    }

    return NO_ERROR;
}

#ifdef CAMERA_MANAGER_ENABLE
int HALCameraFactory::cameraDeviceOpen360(int startId, int cameraNum, int width, int height,
                                          int is360View, hw_device_t** device)
{
    LOGD("F:%s, L:%d, startId:%d, Num:%d, w:%d, h:%d", __FUNCTION__, __LINE__,
         startId, cameraNum, width, height);

    *device = NULL;
    int i;

    //mCameraManager = new CameraManager();

    mCameraManager->mStartCameraID = startId;
    mCameraManager->mCameraTotalNum = cameraNum;
    mCameraManager->mFrameWidth = width;
    mCameraManager->mFrameHeight = height;
    if (cameraNum == 2) {
        mCameraManager->mComposeFrameWidth = width * 2;
        mCameraManager->mComposeFrameHeight = height;
    } else {
        mCameraManager->mComposeFrameWidth = width * 2;
        mCameraManager->mComposeFrameHeight = height * 2;
    }
    int startCameraID = mCameraManager->mStartCameraID;
    //int cameraNum = mCameraManager->mCameraTotalNum;

    for(int i=startCameraID;i< MAX_NUM_OF_CAMERAS && i<startCameraID+cameraNum;i++)
    {
        mHardwareCameras[i]->setCameraManager(mCameraManager);
        mCameraManager->setCameraHardware(i,mHardwareCameras[i]);
    }

    LOGD("%s: id = %d", __FUNCTION__, startId);
    if (!isConstructedOK()) {
        LOGE("%s: HALCameraFactory has failed to initialize", __FUNCTION__);
        return -EINVAL;
    }

    if (startId < 0 || startId > MAX_NUM_OF_CAMERAS) {
        LOGE("%s: Camera id %d is out of bounds (%d)",
             __FUNCTION__, startId, MAX_NUM_OF_CAMERAS);
        return -EINVAL;
    }

    if((startId >= startCameraID) && (startId < startCameraID+cameraNum))
    {
        for(i=startCameraID;i< MAX_NUM_OF_CAMERAS && i<startCameraID+cameraNum;i++)
        {
            if(!mHardwareCameras[i]->isCameraIdle())
            {
                LOGE("start 360,but camera[%d] is busy,wait for a moment",i);
                return -1;
            }
        }
    }

    if(is360View > 0)
    {
        mCameraManager->setOviewEnable(true);
    }
    else
    {
        mCameraManager->setOviewEnable(false);
    }

    for(i= (startId + cameraNum -1);i >= startId;i--)
    {
        LOGD("CameraDebug,F:%s,L:%d,i:%d",__FUNCTION__,__LINE__,i);
        mHardwareCameras[i]->setCameraHardwareInfo(&mHalCameraInfo[i]);
        if (mHardwareCameras[i]->connectCamera(device) != NO_ERROR)
        {
            LOGE("%s: Unable to connect camera", __FUNCTION__);
            return -EINVAL;
        }

        if (mHardwareCameras[i]->Initialize() != NO_ERROR)
        {
            LOGE("%s: Unable to Initialize camera", __FUNCTION__);
            return -EINVAL;
        }
    }

    return NO_ERROR;
}
#endif
int HALCameraFactory::cameraDeviceOpen(int camera_id, hw_device_t** device)
{
    LOGV("%s: id = %d", __FUNCTION__, camera_id);

    *device = NULL;

    if (!isConstructedOK()) {
        LOGE("%s: HALCameraFactory has failed to initialize", __FUNCTION__);
        return -EINVAL;
    }

   // if (camera_id < 0 || camera_id >= (mAttachedCamerasNum + mRemovableCamerasNum)) {
    if (camera_id < 0 || camera_id >= (MAX_NUM_OF_CAMERAS)) {
        LOGE("%s: Camera id %d is out of bounds (%d)",
             __FUNCTION__, camera_id, mAttachedCamerasNum + mRemovableCamerasNum);
        return -EINVAL;
    }

#if 0
    if (!mHardwareCameras[0]->isCameraIdle()
        || !mHardwareCameras[1]->isCameraIdle())
    {
        LOGW("camera device is busy, wait a moment");
        usleep(500000);
    }
#endif
    mHardwareCameras[camera_id]->setCameraHardwareInfo(&mHalCameraInfo[camera_id]);

    if (mHardwareCameras[camera_id]->connectCamera(device) != NO_ERROR)
    {
        LOGE("%s: Unable to connect camera", __FUNCTION__);
        return -EUSERS;
    }

    if (mHardwareCameras[camera_id]->Initialize() != NO_ERROR)
    {
        LOGE("%s: Unable to Initialize camera", __FUNCTION__);
        return -EINVAL;
    }

    return NO_ERROR;
}

/****************************************************************************
 * Camera HAL API callbacks.
 ***************************************************************************/

int HALCameraFactory::get_number_of_cameras(void)
{
    ALOGD("get_number_of_cameras:%d",gEmulatedCameraFactory.getCameraHardwareNum());
    return gEmulatedCameraFactory.getCameraHardwareNum();
}

int HALCameraFactory::get_camera_info(int camera_id,
                                           struct camera_info* info)
{
    return gEmulatedCameraFactory.getCameraInfo(camera_id, info);
}

int HALCameraFactory::device_open(const hw_module_t* module,
                                       const char* name,
                                       hw_device_t** device)
{
    /*
     * Simply verify the parameters, and dispatch the call inside the
     * HALCameraFactory instance.
     */
	LOGD("device_open");
    if (module != &HAL_MODULE_INFO_SYM.common) {
        LOGE("%s: Invalid module %p expected %p",
             __FUNCTION__, module, &HAL_MODULE_INFO_SYM.common);
        return -EINVAL;
    }
    if (name == NULL) {
        LOGE("%s: NULL name is not expected here", __FUNCTION__);
        return -EINVAL;
    }

    return gEmulatedCameraFactory.cameraDeviceOpen(atoi(name), device);
}


int HALCameraFactory::device_open_aw(const hw_module_t* module,
                                       int startId, int cameraNum, int width, int height, int is360View,
                                       hw_device_t** device)
{
    /*
     * Simply verify the parameters, and dispatch the call inside the
     * HALCameraFactory instance.
     */
    ALOGD("F:%s,L:%d,id:%d,num:%d,w:%d,h:%d",__FUNCTION__,__LINE__,startId, cameraNum, width, height);
    if (module != &HAL_MODULE_INFO_SYM.common) {
        LOGE("%s: Invalid module %p expected %p",
             __FUNCTION__, module, &HAL_MODULE_INFO_SYM.common);
        return -EINVAL;
    }

    if (cameraNum == 2 || cameraNum == 4) {
        LOGD("F:%s,L:%d,goto open 360", __FUNCTION__, __LINE__);
        return gEmulatedCameraFactory.cameraDeviceOpen360(startId, cameraNum, width, height, is360View, device);
    } else {
        LOGD("F:%s,L:%d,goto open one camera id:%d", __FUNCTION__, __LINE__, startId);
        return gEmulatedCameraFactory.cameraDeviceOpen(startId, device);
    }
}

/********************************************************************************
 * Initializer for the static member structure.
 *******************************************************************************/

/* Entry point for camera HAL API. */
struct hw_module_methods_t HALCameraFactory::mCameraModuleMethods = {
    open: HALCameraFactory::device_open
};

}; /* namespace android */

camera_module_t HAL_MODULE_INFO_SYM = {
    common: {
         tag:                   HARDWARE_MODULE_TAG,
         module_api_version:    CAMERA_DEVICE_API_VERSION_1_0,
         hal_api_version:         HARDWARE_HAL_API_VERSION,
         id:                    CAMERA_HARDWARE_MODULE_ID,
         name:                  "V4L2Camera Module",
         author:                "XSJ",
         methods:               &android::HALCameraFactory::mCameraModuleMethods,
         dso:                   NULL,
         reserved:              {0},
    },
    get_number_of_cameras:      android::HALCameraFactory::get_number_of_cameras,
    get_camera_info:            android::HALCameraFactory::get_camera_info,
    open_aw:                    android::HALCameraFactory::device_open_aw,
};

