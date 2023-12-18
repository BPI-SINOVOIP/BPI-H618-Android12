#ifndef __NIGHT_SCENE_MODE__
#define __NIGHT_SCENE_MODE__

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

enum NightState{
    NightSTEP0 = 0,    //used for init step, do nothing
    NightSTEP1 = 1,
    NightSTEP2 = 2,
    NightSTEP3 = 3,
    NightSTEP4 = 4,
    NightSTEP5 = 5,
};
struct NightBuffer{
    void*    Image0Yuv;
    void*    Image1Yuv;
    void*    Image2Yuv;
    void*    Image3Yuv;
    void*    Image4Yuv;
};

class NightSceneMode : public ISceneMode {

public:
    NightSceneMode();
    ~NightSceneMode();

/************************************************************
* Public API for baseclass ISceneMode virtual function
* it must be declared here and realized here
*************************************************************/
public:
    int        InitSceneMode(int width,int height);
    void    SetCallBack(SceneNotifyCb scenenotifycb,void* user);
    int        StartScenePicture();
    int        StopScenePicture();
    int        GetCurrentFrameData(void* vaddr);
    int        PostScenePicture(void* vaddr);
    void    ReleaseSceneMode();
    int        GetScenePictureState();
    int        GetCurrentSceneMode();

protected:
    enum     NightState mNightState;
    int     mNightFinished;

    struct     NightBuffer mNightBuffer;
    //struct isp_hdr_setting_t mNightSetting;
    double     mGainBright;
    double     mGainDark;

private:
    int      RequestNightBuffer();
    void     ReleaseNightBuffer();
};

};
#endif
