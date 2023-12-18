#include "NightSceneMode.h"
#define LOG_TAG "NightSceneMode"
namespace android {

NightSceneMode::NightSceneMode()
{
    mWidth = 0;
    mHeight = 0;
    mSceneNotifyCb = NULL;
    mUser = NULL;
    mNightFinished = 0;
    mGainBright = 0.0;
    mGainDark = 0.0;
    mSceneMode = SCENE_FACTORY_MODE_NIGHT;
    memset(&mNightBuffer,0,sizeof(mNightBuffer));
    mNightState = NightSTEP0;
}

NightSceneMode::~NightSceneMode()
{

}

int NightSceneMode::RequestNightBuffer()
{
    int ret = 0;
    int framesize = mWidth * mHeight * 3 >>1;
    // used for Night Scene mode image
    if (mNightBuffer.Image0Yuv == NULL)
        mNightBuffer.Image0Yuv = (void *)malloc(framesize);
    if (mNightBuffer.Image1Yuv == NULL)
        mNightBuffer.Image1Yuv = (void *)malloc(framesize);
    if (mNightBuffer.Image2Yuv == NULL)
        mNightBuffer.Image2Yuv = (void *)malloc(framesize);
    if (mNightBuffer.Image3Yuv == NULL)
        mNightBuffer.Image3Yuv = (void *)malloc(framesize);
    if (mNightBuffer.Image4Yuv == NULL)
        mNightBuffer.Image4Yuv = (void *)malloc(framesize);

    ALOGD("Request Night Buffer Successes!");
    //malloc failed, release all buffers
    if ((mNightBuffer.Image0Yuv == NULL) ||  \
         (mNightBuffer.Image1Yuv == NULL) || \
         (mNightBuffer.Image2Yuv == NULL) || \
         (mNightBuffer.Image3Yuv == NULL) || \
         (mNightBuffer.Image4Yuv == NULL)) {

        ReleaseNightBuffer();
        ALOGE("Request Night buffer failed!!!");
        ret = -1;
    }
    return ret;
}
void NightSceneMode::ReleaseNightBuffer()
{
    //release all buffers
    if (mNightBuffer.Image0Yuv != NULL){
        free(mNightBuffer.Image0Yuv);
        mNightBuffer.Image0Yuv = NULL;
    }
    if (mNightBuffer.Image1Yuv != NULL){
        free(mNightBuffer.Image1Yuv);
        mNightBuffer.Image1Yuv = NULL;
    }
    if (mNightBuffer.Image2Yuv != NULL){
        free(mNightBuffer.Image2Yuv);
        mNightBuffer.Image2Yuv = NULL;
    }
    if (mNightBuffer.Image3Yuv != NULL){
        free(mNightBuffer.Image3Yuv);
        mNightBuffer.Image3Yuv = NULL;
    }
    if (mNightBuffer.Image4Yuv != NULL){
        free(mNightBuffer.Image4Yuv);
        mNightBuffer.Image4Yuv = NULL;
    }
}
void NightSceneMode::SetCallBack(SceneNotifyCb scenenotifycb,void* user)
{
    mSceneNotifyCb = scenenotifycb;
    mUser = user;
}

int NightSceneMode::InitSceneMode(int width,int height)
{
    int ret;
    mWidth    =    width;
    mHeight    =    height;
    ALOGD("Night InitSceneModeframe size: %d x %d",mWidth,mHeight);

    ret = RequestNightBuffer();
    if(ret == -1){
        ALOGE("Request Night Buffer failed");
        return -1;
    }
    return 0;
}

int NightSceneMode::StartScenePicture()
{
    mNightState = NightSTEP1;
    return 0;
}

int NightSceneMode::StopScenePicture()
{
    mNightState = NightSTEP0;
    return 0;
}

int NightSceneMode::GetCurrentFrameData(void* vaddr)
{
    int ret;
    if(mNightState == NightSTEP1)
    {
        ALOGD("step 1");
        //save 1st picture
        int framesize = mWidth * mHeight * 3 >> 1;
        memcpy(mNightBuffer.Image0Yuv, vaddr, framesize);
        mNightState = NightSTEP2;
    }
    else if(mNightState == NightSTEP2)
    {
        ALOGD("step 2");
        //save 2nd picture
        int frame_size = mWidth * mHeight * 3 >> 1;
        memcpy(mNightBuffer.Image1Yuv, vaddr, frame_size);
        mNightState = NightSTEP3;
    }
    else if(mNightState == NightSTEP3)
    {
        ALOGD("step 3");
        //save 3rd picture
        int frame_size = mWidth * mHeight * 3 >> 1;
        memcpy(mNightBuffer.Image2Yuv, vaddr, frame_size);
        mNightState = NightSTEP4;
    }
    else if(mNightState == NightSTEP4)
    {
        ALOGD("step 4");
        //save 4th picture
        int frame_size = mWidth * mHeight * 3 >> 1;
        memcpy(mNightBuffer.Image3Yuv, vaddr, frame_size);
        mNightFinished = 1;
        mNightState = NightSTEP5;
    }

    if(mNightState <= 4) //???
        return SCENE_CAPTURE_UNKNOW;

    if(mNightState == NightSTEP5)
    {
        mSceneNotifyCb(SCENE_NOTIFY_CMD_SET_3A_LOCK,(void*)0,&ret,mUser);
        return SCENE_CAPTURE_DONE;
    }
    return SCENE_CAPTURE_UNKNOW;
}

int NightSceneMode::PostScenePicture(void* vaddr)
{
    captureDenoise(mNightBuffer.Image0Yuv,
                      mNightBuffer.Image1Yuv,
                      mNightBuffer.Image2Yuv,
                      mNightBuffer.Image3Yuv,
                      mNightBuffer.Image4Yuv,
                    &mNightFinished,
                      mWidth,
                      mHeight);
    if(mNightFinished){
        ALOGD("Night Denoise done");
        memcpy(vaddr,mNightBuffer.Image4Yuv,mWidth*mHeight*3 >> 1);
        return SCENE_COMPUTE_DONE;
    }
    ALOGD("Night Denoise fail");
    return SCENE_COMPUTE_FAIL;
}

int NightSceneMode::GetScenePictureState()
{
    return 0;
}

int NightSceneMode::GetCurrentSceneMode()
{
    return mSceneMode;
}

void NightSceneMode::ReleaseSceneMode()
{
    ReleaseNightBuffer();
}

};