#ifndef __SCENE_MODE_FACTORY_H__
#define __SCENE_MODE_FACTORY_H__
#include <cutils/log.h>
#include "ISceneMode.h"
#include "HDRSceneMode.h"
#include "NightSceneMode.h"

namespace android {

/* ScenceMode factory,all the SenceMode created and destory in here*/
class SceneModeFactory {
public:
    SceneModeFactory():    \
        mSceneMode(SCENE_FACTORY_MODE_AUTO),mHDRSceneMode(NULL),mNightSceneMode(NULL){};
    ~SceneModeFactory(){};

private:
    int mSceneMode;
    HDRSceneMode* mHDRSceneMode;    //now we support HDR mode
    NightSceneMode* mNightSceneMode; // now we support Night mode

private:
    HDRSceneMode* HDRSceneModeGetInstance();
    NightSceneMode* NightSceneModeGetInstance();
public:
    ISceneMode* CreateSceneMode(int mode);
    void DestorySceneMode(ISceneMode* mode);
};


};

#endif