
//#include "CameraDebug.h"
#include <cutils/properties.h>
#include "common.h"
#if DBG_CAMERA_CONFIG
#define LOG_NDEBUG 0
#endif
#define LOG_TAG "CCameraConfig"
#include <utils/Log.h>
#include <string>

#include "camera_config.h"

#define READ_KEY_VALUE(key, val)                        \
    val = (char*)::malloc(KEY_LENGTH);                  \
    if (val == 0){                                      \
        HAL_LOGE("malloc %s failed", val);              \
    }                                                   \
    memset(val, 0, KEY_LENGTH);                         \
    if (readKey(key, val)){                             \
        HAL_LOGV("read key: %s = %s", key, val);            \
    }

#define INIT_PARAMETER(KEY, key)                                        \
    memcpy(mUsed##key, "0\0", 2);                                       \
    mSupport##key##Value = 0;                                           \
    mDefault##key##Value = 0;                                           \
    if (readKey(kUSED_##KEY, mUsed##key))                               \
    {                                                                   \
        if (usedKey(mUsed##key))                                        \
        {                                                               \
            READ_KEY_VALUE(kSUPPORT_##KEY, mSupport##key##Value)        \
            READ_KEY_VALUE(kDEFAULT_##KEY, mDefault##key##Value)        \
        }                                                               \
        else                                                            \
        {                                                               \
            HAL_LOGV("\"%s\" not support", kUSED_##KEY);                \
        }                                                               \
     }

#define CHECK_FREE_POINTER(key)          \
    if(mSupport##key##Value != 0){       \
        ::free(mSupport##key##Value);    \
        mSupport##key##Value = 0;        \
    }                                    \
    if(mDefault##key##Value != 0){       \
        ::free(mDefault##key##Value);    \
        mDefault##key##Value = 0;        \
    }


#define _DUMP_PARAMETERS(key, value)     \
    if (value != 0){                     \
        HAL_LOGV("%s = %s", key, value); \
    }else{                               \
        HAL_LOGV("%s not support", key); \
    }

#define DUMP_PARAMETERS(kused, key)                                \
    _DUMP_PARAMETERS(kUSED_##kused, mUsed##key)                    \
    _DUMP_PARAMETERS(kSUPPORT_##kused, mSupport##key##Value)       \
    _DUMP_PARAMETERS(kDEFAULT_##kused, mDefault##key##Value)

#define MEMBER_FUNCTION(fun)                        \
    bool CCameraConfig::support##fun(){             \
        return usedKey(mUsed##fun);                 \
    };                                              \
    char *CCameraConfig::support##fun##Value(){     \
        return mSupport##fun##Value;                \
    };                                              \
    char *CCameraConfig::default##fun##Value(){     \
        return mDefault##fun##Value;                \
    };

MEMBER_FUNCTION(PreviewSize)
MEMBER_FUNCTION(PictureSize)
MEMBER_FUNCTION(InterpolationSize)
MEMBER_FUNCTION(FlashMode)
MEMBER_FUNCTION(ColorEffect)
MEMBER_FUNCTION(FrameRate)
MEMBER_FUNCTION(FocusMode)
MEMBER_FUNCTION(SceneMode)
MEMBER_FUNCTION(WhiteBalance)

/*
 * Initialize static members of CCameraConfig.
 */
//FILE* CCameraConfig::mhKeyFile = ::fopen(CAMERA_KEY_CONFIG_PATH, "rb");
FILE* CCameraConfig::mhKeyFile = getCameraCfg();
int CCameraConfig::mNumberOfCamera = 0;
int CCameraConfig::mCameraMultiplexing = 0;
char CCameraConfig::mCameraMake[64] = {0};
char CCameraConfig::mCameraModel[64] = {0};

CCameraConfig::CCameraConfig(int id)
    :mConstructOk(false)
    ,mCurCameraId(id)
    ,mCameraFacing(0)
    ,mOrientation(0)
    ,mSupportFrameRate(0)
    ,mDeviceID(0)
    ,mFastPictureMode(false)
{
    // get camera facing
    char cameraFacing[2];
    if(readKey(kCAMERA_FACING, cameraFacing))
    {
        mCameraFacing = atoi(cameraFacing);
        HAL_LOGV("camera facing %s", (mCameraFacing == 0) ? "back" : "front");
    }

    // get camera device driver
    memset(mCameraDevice, 0, sizeof(mCameraDevice));
    if(readKey(kCAMERA_DEVICE, mCameraDevice))
    {
        HAL_LOGV("camera device %s", mCameraDevice);
    }

    // get device id
    char deviceID[2];
    if(readKey(kDEVICE_ID, deviceID))
    {
        mDeviceID = atoi(deviceID);
        HAL_LOGV("camera device id %d", mDeviceID);
    }

    // get camera orientation
    char str[4];
    if(readKey(kCAMERA_ORIENTATION, str))
    {
        mOrientation = atoi(str);
        int32_t rotation = property_get_int32("ro.primary_display.user_rotation", 0);
        if (rotation > 0) {
            if (mCameraFacing) {
                mOrientation -= rotation;
            } else {
                mOrientation += rotation;
            }
            mOrientation = (mOrientation + 360) % 360;
        }
        HAL_LOGV("camera orientation %d", mOrientation);
    }

    // get camera support frame rate
    char str1[4];
    if(readKey(kSUPPORT_FRAME_RATE, str1))
    {
        mSupportFrameRate = atoi(str1);
    }

    //init other parameters
    initParameters();

    mConstructOk = true;
}

CCameraConfig::~CCameraConfig()
{
    CHECK_FREE_POINTER(PreviewSize)
    CHECK_FREE_POINTER(PictureSize)
    CHECK_FREE_POINTER(InterpolationSize)
    CHECK_FREE_POINTER(FlashMode)
    CHECK_FREE_POINTER(ColorEffect)
    CHECK_FREE_POINTER(FrameRate)
    CHECK_FREE_POINTER(FocusMode)
    CHECK_FREE_POINTER(SceneMode)
    CHECK_FREE_POINTER(WhiteBalance)

    if (mhKeyFile != NULL)
    {
        ::fclose(mhKeyFile);
        mhKeyFile = NULL;
    }
}

bool CCameraConfig::usedKey(char *value)
{
    return strcmp(value, "1") ? false : true;
}

void CCameraConfig::initParameters()
{
    if (!mConstructOk)
    {
        return;
    }

    if (mhKeyFile == NULL)
    {
        HAL_LOGW("invalid camera config file hadle");
        return ;
    }

    // fast picture mode
    char str[10];
    if(readKey(kFAST_PICTURE_MODE, str))
    {
        mFastPictureMode = (atoi(str) == 1) ? true : false;
        HAL_LOGV("%s support fast picture mode", mFastPictureMode ? "" : "do not");
    }
    else if(readKey(kUSE_BUILTIN_ISP, str))
    {
        mFastPictureMode = (atoi(str) == 1) ? true : false;
    }

    INIT_PARAMETER(PREVIEW_SIZE, PreviewSize)
    INIT_PARAMETER(PICTURE_SIZE, PictureSize)
    INIT_PARAMETER(INTERPOLATION_SIZE, InterpolationSize)
    INIT_PARAMETER(FLASH_MODE, FlashMode)
    INIT_PARAMETER(COLOR_EFFECT, ColorEffect)
    INIT_PARAMETER(FRAME_RATE, FrameRate)
    INIT_PARAMETER(FOCUS_MODE, FocusMode)
    INIT_PARAMETER(SCENE_MODE, SceneMode)
    INIT_PARAMETER(WHITE_BALANCE, WhiteBalance)

    // exposure compensation
    memcpy(mUsedExposureCompensation, "0\0", 2);
    memset(mMaxExposureCompensation, 0, 4);
    memset(mMinExposureCompensation, 0, 4);
    memset(mStepExposureCompensation, 0, 4);
    memset(mDefaultExposureCompensation, 0, 4);
    if (readKey(kUSED_EXPOSURE_COMPENSATION, mUsedExposureCompensation))
    {
        if (usedKey(mUsedExposureCompensation))
        {
            readKey(kMIN_EXPOSURE_COMPENSATION, mMinExposureCompensation);
            readKey(kMAX_EXPOSURE_COMPENSATION, mMaxExposureCompensation);
            readKey(kSTEP_EXPOSURE_COMPENSATION, mStepExposureCompensation);
            readKey(kDEFAULT_EXPOSURE_COMPENSATION, mDefaultExposureCompensation);
        }
        else
        {
            HAL_LOGV("\"%s\" not support", kUSED_EXPOSURE_COMPENSATION);
        }
     }

    // zoom
    memcpy(mUsedZoom, "0\0", 2);
    memset(mZoomSupported, 0, 8);
    memset(mSmoothZoomSupported, 0, 4);
    memset(mZoomRatios, 0, KEY_LENGTH);
    memset(mMaxZoom, 0, 4);
    memset(mDefaultZoom, 0, 4);
    if (readKey(kUSED_ZOOM, mUsedZoom))
    {
        if (usedKey(mUsedZoom))
        {
            readKey(kZOOM_SUPPORTED, mZoomSupported);
            readKey(kSMOOTH_ZOOM_SUPPORTED, mSmoothZoomSupported);
            readKey(kZOOM_RATIOS, mZoomRatios);
            readKey(kMAX_ZOOM, mMaxZoom);
            readKey(kDEFAULT_ZOOM, mDefaultZoom);
        }
        else
        {
            HAL_LOGV("\"%s\" not support", kUSED_ZOOM);
        }
     }
    readKey(KHORIZONTAL_VIEW_ANGLE, str);
    mHorizonalViewAngle = atof(str);
    readKey(KVERTICAL_VIEW_ANGLE, str);
    mVerticalViewAngle = atof(str);
}

void CCameraConfig::dumpParameters()
{
    if (!mConstructOk)
    {
        return;
    }

    if (mhKeyFile == NULL)
    {
        HAL_LOGW("invalid camera config file hadle");
        return ;
    }

    HAL_LOGV("/*------------------------------------------------------*/");
    HAL_LOGV("camrea id: %d", mCurCameraId);
    HAL_LOGV("camera facing %s", (mCameraFacing == 0) ? "back" : "front");
    HAL_LOGV("%s support fast picture mode", mFastPictureMode ? "" : "do not");
    HAL_LOGV("camera orientation %d", mOrientation);
    HAL_LOGV("camera support frame rate %d", mSupportFrameRate);
    HAL_LOGV("camera device %s", mCameraDevice);
    DUMP_PARAMETERS(PREVIEW_SIZE, PreviewSize)
    DUMP_PARAMETERS(PICTURE_SIZE, PictureSize)
    DUMP_PARAMETERS(INTERPOLATION_SIZE, InterpolationSize)
    DUMP_PARAMETERS(FLASH_MODE, FlashMode)
    DUMP_PARAMETERS(COLOR_EFFECT, ColorEffect)
    DUMP_PARAMETERS(FRAME_RATE, FrameRate)
    DUMP_PARAMETERS(FOCUS_MODE, FocusMode)
    DUMP_PARAMETERS(SCENE_MODE, SceneMode)
    DUMP_PARAMETERS(WHITE_BALANCE, WhiteBalance)

    _DUMP_PARAMETERS(kUSED_EXPOSURE_COMPENSATION, mUsedExposureCompensation)
    _DUMP_PARAMETERS(kMIN_EXPOSURE_COMPENSATION, mMinExposureCompensation)
    _DUMP_PARAMETERS(kMAX_EXPOSURE_COMPENSATION, mMaxExposureCompensation)
    _DUMP_PARAMETERS(kSTEP_EXPOSURE_COMPENSATION, mStepExposureCompensation)
    _DUMP_PARAMETERS(kDEFAULT_EXPOSURE_COMPENSATION, mDefaultExposureCompensation)

    _DUMP_PARAMETERS(kUSED_ZOOM, mUsedZoom)
    _DUMP_PARAMETERS(kZOOM_SUPPORTED, mZoomSupported)
    _DUMP_PARAMETERS(kSMOOTH_ZOOM_SUPPORTED, mSmoothZoomSupported)
    _DUMP_PARAMETERS(kZOOM_RATIOS, mZoomRatios)
    _DUMP_PARAMETERS(kMAX_ZOOM, mMaxZoom)
    _DUMP_PARAMETERS(kDEFAULT_ZOOM, mDefaultZoom)
    HAL_LOGV("/*------------------------------------------------------*/");
}

void CCameraConfig::getValue(char *line, char *value)
{
    char * ptemp = line;
    while(*ptemp)
    {
        if (*ptemp++ == '=')
        {
            break;
        }
    }

    char *pval = ptemp;
    char *seps = " \n\r\t";
    int offset = 0;
    pval = strtok(pval, seps);
    while (pval != NULL)
    {
        strncpy(value + offset, pval, strlen(pval));
        offset += strlen(pval);
        pval = strtok(NULL, seps);
    }
    *(value + offset) = 0;
}

bool CCameraConfig::readKey(char *key, char *value)
{
    return readCommonKey(key, value, mCurCameraId);
}

bool CCameraConfig::readCommonKey(char *key, char * value, int cameraId)
{
    bool bRet = false;
    bool bFlagBegin = false;
    char strId[2];
    char str[KEY_LENGTH];

    if (key == 0 || value == 0)
    {
        HAL_LOGE("error input para");
        return false;
    }

    if (mhKeyFile == NULL)
    {
        HAL_LOGE("error key file handle");
        return false;
    }

    fseek(mhKeyFile, 0L, SEEK_SET);

    memset(str, 0, KEY_LENGTH);
    while (fgets(str, KEY_LENGTH , mhKeyFile))
    {
        if (!strcmp(key, kNUMBER_OF_CAMERA)
            || !strcmp(key, kCAMERA_EXIF_MAKE)
            || !strcmp(key, kCAMERA_EXIF_MODEL)
            || !strcmp(key, kCAMERA_MULTIPLEXING))
        {
            bFlagBegin = true;
        }

        if (!bFlagBegin)
        {
            if (!strncmp(str, "camera_id", strlen("camera_id")))
            {
                getValue(str, strId);
                if (atoi(strId) == cameraId)
                {
                    bFlagBegin = true;
                }
            }
            continue;
        }

        if (!strncmp(str, "camera_id", strlen("camera_id")))
        {
            break;
        }

        if (!strncmp(key, str, strlen(key)))
        {
            getValue(str, value);

            bRet = true;
            break;
        }
        memset(str, 0, KEY_LENGTH);
    }
    return bRet;
}

