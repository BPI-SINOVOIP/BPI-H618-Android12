#include "SceneModeFactory.h"

#define LOG_TAG "SceneModeFactory"

namespace android {

ISceneMode* SceneModeFactory::CreateSceneMode(int SceneMode)
{
    ISceneMode* mode = NULL;
    mSceneMode = SceneMode;
    switch(mSceneMode)
    {
        case SCENE_FACTORY_MODE_HDR:
            mode = HDRSceneModeGetInstance();
            if(mode == NULL)
                ALOGE("Create HDR Scene Mode failed!");
            break;
        case SCENE_FACTORY_MODE_NIGHT:
            mode = NightSceneModeGetInstance();
            if(mode == NULL)
                ALOGE("Create Night Scene Mode failed!");
            break;
        default:
            break;
    }
    return mode;
}

void SceneModeFactory::DestorySceneMode(ISceneMode* mode)
{
    //destory all mode
    ALOGD("DestorySceneMode %d",mSceneMode);
    if(mode == NULL){
        mSceneMode = SCENE_FACTORY_MODE_AUTO;
        if(mHDRSceneMode != NULL){
            mHDRSceneMode->ReleaseSceneMode();
            delete(mHDRSceneMode);
            mHDRSceneMode = NULL;
        }
        //Night mode
        if(mNightSceneMode != NULL){
            mNightSceneMode->ReleaseSceneMode();
            delete(mNightSceneMode);
            mNightSceneMode = NULL;
        }
        return;
    }
    //just destory the HDR mode
    switch(mode->GetCurrentSceneMode())
    {
        case SCENE_FACTORY_MODE_HDR:
            if(mHDRSceneMode != NULL){
                delete(mHDRSceneMode);
                mHDRSceneMode = NULL;
            }
            break;
        case SCENE_FACTORY_MODE_NIGHT:
            if(mNightSceneMode != NULL){
                delete(mNightSceneMode);
                mNightSceneMode = NULL;
            }
            break;
        default:
            break;
    }
    mSceneMode = SCENE_FACTORY_MODE_AUTO;
    return;
}

HDRSceneMode* SceneModeFactory::HDRSceneModeGetInstance()
{
    if(mHDRSceneMode == NULL)
        mHDRSceneMode = new HDRSceneMode();
    return     mHDRSceneMode;
}

NightSceneMode* SceneModeFactory::NightSceneModeGetInstance()
{
    if(mNightSceneMode == NULL)
        mNightSceneMode = new NightSceneMode();
    return     mNightSceneMode;
}

};