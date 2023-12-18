#ifndef __CAMERA_CONFIG_H__
#define __CAMERA_CONFIG_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/media.h>
#include <errno.h>
#define MEDIA_PATH "/dev/media0"
#define CAMERA_KEY_CONFIG_PATH    "/vendor/etc/camera.cfg"

#define KEY_LENGTH    256

#define kCAMERA_EXIF_MAKE                    "key_camera_exif_make"
#define kCAMERA_EXIF_MODEL                    "key_camera_exif_model"

#define kNUMBER_OF_CAMERA                    "number_of_camera"
#define kCAMERA_MULTIPLEXING                  "use_camera_multiplexing"
#define kCAMERA_FACING                        "camera_facing"
#define kCAMERA_ORIENTATION                    "camera_orientation"
#define kCAMERA_DEVICE                        "camera_device"
#define kDEVICE_ID                            "device_id"
#define kFAST_PICTURE_MODE                    "fast_picture_mode"
#define kUSE_BUILTIN_ISP                    "use_builtin_isp"

#define kUSED_PREVIEW_SIZE                    "used_preview_size"
#define kSUPPORT_PREVIEW_SIZE                "key_support_preview_size"
#define kDEFAULT_PREVIEW_SIZE                "key_default_preview_size"

#define kUSED_PICTURE_SIZE                   "used_picture_size"
#define kSUPPORT_PICTURE_SIZE                "key_support_picture_size"
#define kDEFAULT_PICTURE_SIZE                "key_default_picture_size"

#define kUSED_INTERPOLATION_SIZE              "used_interpolation_size"
#define kSUPPORT_INTERPOLATION_SIZE           "key_support_src_interpolation_size"
#define kDEFAULT_INTERPOLATION_SIZE           "key_default_dst_interpolation_size"

#define kUSED_FLASH_MODE                    "used_flash_mode"
#define kSUPPORT_FLASH_MODE                    "key_support_flash_mode"
#define kDEFAULT_FLASH_MODE                    "key_default_flash_mode"

#define kUSED_COLOR_EFFECT                    "used_color_effect"
#define kSUPPORT_COLOR_EFFECT                "key_support_color_effect"
#define kDEFAULT_COLOR_EFFECT                "key_default_color_effect"

#define kUSED_FRAME_RATE                    "used_frame_rate"
#define kSUPPORT_FRAME_RATE                    "key_support_frame_rate"
#define kDEFAULT_FRAME_RATE                    "key_default_frame_rate"

#define kUSED_FOCUS_MODE                    "used_focus_mode"
#define kSUPPORT_FOCUS_MODE                    "key_support_focus_mode"
#define kDEFAULT_FOCUS_MODE                    "key_default_focus_mode"

#define kUSED_SCENE_MODE                    "used_scene_mode"
#define kSUPPORT_SCENE_MODE                    "key_support_scene_mode"
#define kDEFAULT_SCENE_MODE                    "key_default_scene_mode"

#define kUSED_WHITE_BALANCE                    "used_white_balance"
#define kSUPPORT_WHITE_BALANCE                "key_support_white_balance"
#define kDEFAULT_WHITE_BALANCE                "key_default_white_balance"

#define kUSED_EXPOSURE_COMPENSATION            "used_exposure_compensation"
#define kMIN_EXPOSURE_COMPENSATION            "key_min_exposure_compensation"
#define kMAX_EXPOSURE_COMPENSATION            "key_max_exposure_compensation"
#define kSTEP_EXPOSURE_COMPENSATION            "key_step_exposure_compensation"
#define kDEFAULT_EXPOSURE_COMPENSATION        "key_default_exposure_compensation"

#define kUSED_ZOOM                            "used_zoom"
#define kZOOM_SUPPORTED                       "key_zoom_supported"
#define kSMOOTH_ZOOM_SUPPORTED                "key_smooth_zoom_supported"
#define kZOOM_RATIOS                          "key_zoom_ratios"
#define kMAX_ZOOM                             "key_max_zoom"
#define kDEFAULT_ZOOM                         "key_default_zoom"
#define KHORIZONTAL_VIEW_ANGLE "key_horizonal_view_angle"
#define KVERTICAL_VIEW_ANGLE "key_vertical_view_angle"

#define MEMBER_DEF(mem)                \
    char mUsed##mem[2];                \
    char * mSupport##mem##Value;    \
    char * mDefault##mem##Value;
static FILE *getCameraCfg(){
    FILE *f = NULL;
    int ret = 0;
    char CfgPath[100]="/vendor/etc/camera";
    int fd = open(MEDIA_PATH, O_RDONLY);
    if (fd == -1) {
        HAL_LOGE("Could not openg media controller device: %s!", strerror(errno));
        goto error;
    }
    struct media_entity_desc entity;
    do {
       // Go through the list of media controller entities
       entity.id |= MEDIA_ENT_ID_FLAG_NEXT;
       if (ioctl(fd, MEDIA_IOC_ENUM_ENTITIES, &entity) < 0) {
           if (errno == EINVAL) {
               // Ending up here when no more entities left.
               // Will simply 'break' if everything was ok
                   HAL_LOGI("no sensor or get sensor done!");
                   ret = -1;
           } else {
               HAL_LOGE("ERROR in browsing media controller entities: %s!", strerror(errno));
               ret = -1;
           }
           break;
       } else {
           if (entity.type == MEDIA_ENT_T_V4L2_SUBDEV_SENSOR) {
               // A driver has been found!
               // The driver is using sensor name when registering
               // to media controller (we will truncate that to
               // first space, if any)
               strcat(CfgPath, "_");
               strcat(CfgPath, entity.name);
            }
        }
    } while (!ret);
    strcat(CfgPath, ".cfg");

    f = ::fopen(CfgPath, "rb");
    close(fd);
    error:
    if (f == NULL) {
           HAL_LOGE("open %s failed use default cfg", CfgPath);
           f = ::fopen(CAMERA_KEY_CONFIG_PATH, "rb");
           if (f == NULL) {
                HAL_LOGE("open %s failed use default cfg", CAMERA_KEY_CONFIG_PATH);
           }
    } else {
           HAL_LOGI("open %s success", CfgPath);
    }
    return f;
}
class CCameraConfig
{
public:
    CCameraConfig(int id);
    ~CCameraConfig();

    void initParameters();
    void dumpParameters();

    static int numberOfCamera()
    {
        if(mhKeyFile == NULL){
            mhKeyFile = ::fopen(CAMERA_KEY_CONFIG_PATH, "rb");
        }
        if(mNumberOfCamera == 0)
        {
            char numberOfCamera[2];
            bool retNumOfCamera = readCommonKey(kNUMBER_OF_CAMERA, numberOfCamera);
            std::string numberOfCameraStr = std::string(numberOfCamera);
            mNumberOfCamera = std::stoi(numberOfCameraStr);
        }
        return mNumberOfCamera;
    }

    static int supportCameraMulplex()
    {
        char cameraMultiplexing[2];
        if (readCommonKey(kCAMERA_MULTIPLEXING, cameraMultiplexing))
        {
            mCameraMultiplexing = atoi(cameraMultiplexing);
        }
        return mCameraMultiplexing;
    }
    int cameraFacing()
    {
        return mCameraFacing;
    }

    int getCameraOrientation()
    {
        return mOrientation;
    }

	int getSupportFrameRate()
	{
	    return mSupportFrameRate;
	}

    char * cameraDevice()
    {
        return mCameraDevice;
    }

    int getDeviceID()
    {
        return mDeviceID;
    }

    // support fast picture mode or not
    bool supportFastPictureMode()
    {
        return mFastPictureMode;
    }

    // exif
    static char * getExifMake()
    {
        if(mhKeyFile == NULL){
            mhKeyFile = ::fopen(CAMERA_KEY_CONFIG_PATH, "rb");
        }
        readCommonKey(kCAMERA_EXIF_MAKE,  mCameraMake);
        return mCameraMake;
    }

    static char * getExifModel()
    {
        if(mhKeyFile == NULL){
            mhKeyFile = ::fopen(CAMERA_KEY_CONFIG_PATH, "rb");
        }
        readCommonKey(kCAMERA_EXIF_MODEL, mCameraModel);
        return mCameraModel;
    }
    float getHorizonalViewAngle()
    {
        return mHorizonalViewAngle;
    }
    float getVerticalViewAngle()
    {
        return mVerticalViewAngle;
    }

    bool supportPreviewSize();
    char * supportPreviewSizeValue();
    char * defaultPreviewSizeValue();

    bool supportPictureSize();
    char * supportPictureSizeValue();
    char * defaultPictureSizeValue();

    bool supportInterpolationSize();
    char * supportInterpolationSizeValue();
    char * defaultInterpolationSizeValue();

    bool supportFlashMode();
    char * supportFlashModeValue();
    char * defaultFlashModeValue();

    bool supportColorEffect();
    char * supportColorEffectValue();
    char * defaultColorEffectValue();

    bool supportFrameRate();
    char * supportFrameRateValue();
    char * defaultFrameRateValue();

    bool supportFocusMode();
    char * supportFocusModeValue();
    char * defaultFocusModeValue();

    bool supportSceneMode();
    char * supportSceneModeValue();
    char * defaultSceneModeValue();

    bool supportWhiteBalance();
    char * supportWhiteBalanceValue();
    char * defaultWhiteBalanceValue();

    // exposure compensation
    bool supportExposureCompensation()
    {
        return usedKey(mUsedExposureCompensation);
    }

    char * minExposureCompensationValue()
    {
        return mMinExposureCompensation;
    }

    char * maxExposureCompensationValue()
    {
        return mMaxExposureCompensation;
    }

    char * stepExposureCompensationValue()
    {
        return mStepExposureCompensation;
    }

    char * defaultExposureCompensationValue()
    {
        return mDefaultExposureCompensation;
    }

    // zoom
    bool supportZoom()
    {
        return usedKey(mUsedZoom);
    }

    char * zoomSupportedValue()
    {
        return mZoomSupported;
    }

    char * smoothZoomSupportedValue()
    {
        return mSmoothZoomSupported;
    }

    char * zoomRatiosValue()
    {
        return mZoomRatios;
    }

    char * maxZoomValue()
    {
        return mMaxZoom;
    }

    char * defaultZoomValue()
    {
        return mDefaultZoom;
    }

    static FILE * mhKeyFile;
    static int mNumberOfCamera;
    static char mCameraMake[64];
    static char mCameraModel[64];
    static int mCameraMultiplexing;
    static bool readCommonKey(char *key, char* value, int cameraId = -1);
private:
    bool readKey(char *key, char *value);
    bool usedKey(char *value);

    static void getValue(char *line, char *value);

    bool mConstructOk;

    int mCurCameraId;
    int mCameraFacing;
    int mOrientation;
    int mSupportFrameRate;
    char mCameraDevice[64];
    int mDeviceID;
    bool mFastPictureMode;

    MEMBER_DEF(PreviewSize)
    MEMBER_DEF(PictureSize)
    MEMBER_DEF(InterpolationSize)
    MEMBER_DEF(FlashMode)
    MEMBER_DEF(ColorEffect)
    MEMBER_DEF(FrameRate)
    MEMBER_DEF(FocusMode)
    MEMBER_DEF(SceneMode)
    MEMBER_DEF(WhiteBalance)
    // MEMBER_DEF(ExposureCompensation)

    char mUsedExposureCompensation[2];
    char mMinExposureCompensation[4];
    char mMaxExposureCompensation[4];
    char mStepExposureCompensation[4];
    char mDefaultExposureCompensation[4];

    char mUsedZoom[2];
    char mZoomSupported[8];
    char mSmoothZoomSupported[8];
    char mZoomRatios[KEY_LENGTH];
    char mMaxZoom[4];
    char mDefaultZoom[4];
    float mHorizonalViewAngle;
    float mVerticalViewAngle;
};

#endif // __CAMERA_CONFIG_H__
