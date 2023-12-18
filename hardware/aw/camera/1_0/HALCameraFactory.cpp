
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
    char value[128];
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
          mConstructedOK(false)
{
    F_LOG;

    LOGD("camera hal version: %s", CAMERA_HAL_VERSION);

    memset(&mHalCameraInfo,0,sizeof(mHalCameraInfo));

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
        mCameraConfig[id] = new CCameraConfig(id);
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
        mAttachedCamerasNum = mCameraConfig[0]->numberOfCsiOrMipiCamera();
        mRemovableCamerasNum = mCameraConfig[0]->numberOfUsbCamera();

        if ((mAttachedCamerasNum == 2)
            && (mCameraConfig[1] == NULL))
        {
            return;
        }
    }

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
    int num_of_cameras_id = 0;
    int removable_camera_num = 0;

    if (mCameraConfig[0] != NULL)
    {
        orientation = mCameraConfig[0]->getCameraOrientation();
        mAttachedCamerasNum = mCameraConfig[0]->numberOfCsiOrMipiCamera();
        mRemovableCamerasNum = mCameraConfig[0]->numberOfUsbCamera();
    }

    for(int i = 0; i < mAttachedCamerasNum; i++)
    {
        strcpy(mHalCameraInfo[i].device_name, mCameraConfig[i]->cameraDevice());
        mHalCameraInfo[i].device_id        = mCameraConfig[i]->getDeviceID();
        mHalCameraInfo[i].facing        = mCameraConfig[i]->cameraFacing();
        mHalCameraInfo[i].orientation    = mCameraConfig[i]->getCameraOrientation();
        mHalCameraInfo[i].fast_picture_mode    = mCameraConfig[i]->supportFastPictureMode();
        mHalCameraInfo[i].is_uvc        = false;
#ifdef __T8__
        mHalCameraInfo[i].is_vfe        = true;
#else
        mHalCameraInfo[i].is_vfe        = false;
#endif
    }

    for(int j = 0; j < mRemovableCamerasNum; j++)
    {
       num_of_cameras_id = mAttachedCamerasNum + j;
       sprintf(dev_node, "/dev/video%d", num_of_cameras_id);
       ret = access(dev_node, F_OK);
       if(ret == 0)
       {
           removable_camera_num++;
           strcpy(mHalCameraInfo[num_of_cameras_id].device_name, dev_node);
           LOGV("HalCameraInfo[%d].device_name:%s",num_of_cameras_id,mHalCameraInfo[num_of_cameras_id].device_name);
           mHalCameraInfo[num_of_cameras_id].device_id     =
               mCameraConfig[num_of_cameras_id]->getDeviceID();
           mHalCameraInfo[num_of_cameras_id].facing        = mCameraConfig[num_of_cameras_id]->cameraFacing();
           mHalCameraInfo[num_of_cameras_id].orientation    = orientation;
           mHalCameraInfo[num_of_cameras_id].fast_picture_mode    = false;
           mHalCameraInfo[num_of_cameras_id].is_uvc        = true;
           mHalCameraInfo[num_of_cameras_id].is_vfe        = false;
       }
       else
       {
           LOGE("error number of removable cameras: %d", j);
           if(j < mRemovableCamerasNum)
           {
               LOGW("continue to probe!");
           }
           else
           {
               LOGV("mAttachedCamerasNum + removable_camera_num:%d",
                   mAttachedCamerasNum + removable_camera_num);
               return mAttachedCamerasNum + removable_camera_num;
           }
       }
    }

    if(removable_camera_num < mAttachedCamerasNum + mRemovableCamerasNum)
    {
        return mAttachedCamerasNum + removable_camera_num;
    }

    // total numbers include attached and removable cameras
    return mAttachedCamerasNum + mRemovableCamerasNum;
}

int HALCameraFactory::getCameraInfo(int camera_id, struct camera_info* info)
{
    //LOGV("%s: id = %d", __FUNCTION__, camera_id);

    int total_num_of_cameras = mAttachedCamerasNum + mRemovableCamerasNum;
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

	getCallingProcessName(calling_process);
	if((strcmp(calling_process, "com.tencent.mobileqq:video") == 0))
	{
		LOGE("hxl ======================== com.tencent.mobileqq:video");
		//info->orientation =0;
	}
	
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

int HALCameraFactory::cameraDeviceOpen(int camera_id, hw_device_t** device)
{
    LOGV("%s: id = %d", __FUNCTION__, camera_id);

    *device = NULL;

    if (!isConstructedOK()) {
        LOGE("%s: HALCameraFactory has failed to initialize", __FUNCTION__);
        return -EINVAL;
    }

    if (camera_id < 0 || camera_id >= (mAttachedCamerasNum + mRemovableCamerasNum)) {
        LOGE("%s: Camera id %d is out of bounds (%d)",
             __FUNCTION__, camera_id, mAttachedCamerasNum + mRemovableCamerasNum);
        return -EINVAL;
    }

    if (!mHardwareCameras[0]->isCameraIdle()
        || !mHardwareCameras[1]->isCameraIdle())
    {
        LOGW("camera device is busy, wait a moment");
        usleep(500000);
    }

    mHardwareCameras[camera_id]->setCameraHardwareInfo(&mHalCameraInfo[camera_id]);

    if(!mHalCameraInfo[camera_id].is_uvc)
    {
        int id = 0;
        for (id = 0; id < MAX_NUM_OF_CAMERAS; id++) {
            if (mHardwareCameras[id]->mV4L2CameraDevice->isConnected()) {
                LOGE("%s: user has connected camera", __FUNCTION__);
                return -EUSERS;
            }
        }
    }

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

/********************************************************************************
 * Initializer for the static member structure.
 *******************************************************************************/

/* Entry point for camera HAL API. */
struct hw_module_methods_t HALCameraFactory::mCameraModuleMethods = {
    .open = HALCameraFactory::device_open
};

}; /* namespace android */

camera_module_t HAL_MODULE_INFO_SYM = {
    .common ={
         .tag                =                   HARDWARE_MODULE_TAG,
         .module_api_version =    CAMERA_DEVICE_API_VERSION_1_0,
         .hal_api_version    =         HARDWARE_HAL_API_VERSION,
         .id      =                    CAMERA_HARDWARE_MODULE_ID,
         .name    =                  "V4L2Camera Module",
         .author  =                "XSJ",
         .methods =               &android::HALCameraFactory::mCameraModuleMethods,
         .dso     =               NULL,
         .reserved =              {0},
    },
    .get_number_of_cameras =      android::HALCameraFactory::get_number_of_cameras,
    .get_camera_info       =            android::HALCameraFactory::get_camera_info,
};

