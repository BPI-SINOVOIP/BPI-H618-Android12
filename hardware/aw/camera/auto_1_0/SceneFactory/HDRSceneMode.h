#ifndef __HDR_SCENE_MODE__
#define __HDR_SCENE_MODE__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cutils/log.h>
#include <math.h>

#include "CameraPlatform.h"
#include "ISceneMode.h"
#include "hdr.h"

#ifdef USE_SUNXI_CAMERA_H
#include <sunxi_camera.h>
#endif
#ifdef USE_CSI_VIN_DRIVER
#include <sunxi_camera_v2.h>
#endif

namespace android {

enum HDRState{
    HDRSTEP0 = 0,    //used for init step, do nothing
    HDRSTEP1 = 1,
    HDRSTEP2 = 2,
    HDRSTEP3 = 3,
    HDRSTEP4 = 4,
};
struct HDRBuffer{
    void*    Image0Yuv;
    void*    Image1Yuv;
    void*    Image2Yuv;
    void*    Image3Yuv;
    void*    Image4Yuv;
};

class HDRSceneMode : public ISceneMode {

public:
    HDRSceneMode();
    ~HDRSceneMode();

/************************************************************
* Public API for baseclass ISceneMode virtual function
* it must be declared here and realized here
*************************************************************/
public:
    int        InitSceneMode(int width,int height);
    void    SetCallBack(SceneNotifyCb Scenenotifycb,void* user);
    int        StartScenePicture();
    int        StopScenePicture();
    int        GetCurrentFrameData(void* vaddr);
    int        PostScenePicture(void* vaddr);
    void    ReleaseSceneMode();
    int        GetScenePictureState();
    int        GetCurrentSceneMode();

protected:
    enum    HDRState mHDRState;
    int     mHDRFinished;

    struct HDRBuffer mHDRBuffer;
    struct isp_hdr_setting_t mHDRSetting;
    double     mGainBright;
    double     mGainDark;

private:
    int      RequestHDRBuffer();
    void     ReleaseHDRBuffer();
};

};
#endif
