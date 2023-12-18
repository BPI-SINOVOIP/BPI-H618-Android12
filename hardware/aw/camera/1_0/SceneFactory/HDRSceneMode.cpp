#include "HDRSceneMode.h"
#define LOG_TAG "HDRSceneMode"
namespace android {

HDRSceneMode::HDRSceneMode()
{
    mWidth = 0;
    mHeight = 0;
    mSceneNotifyCb = NULL;
    mUser = NULL;
    mSceneMode = SCENE_FACTORY_MODE_HDR;
    mHDRState -1;
    mHDRFinished = 0;
    mGainBright = 0.0;
    mGainDark = 0.0;
    memset(&mHDRBuffer,0,sizeof(mHDRBuffer));
    mHDRState = HDRSTEP0;
}

HDRSceneMode::~HDRSceneMode()
{

}

int HDRSceneMode::RequestHDRBuffer()
{
    int ret = 0;
    int framesize = mWidth * mHeight * 3 >>1;
    // used for HDR image mode
    if (mHDRBuffer.Image0Yuv == NULL)
        mHDRBuffer.Image0Yuv = (void *)malloc(framesize);
    if (mHDRBuffer.Image1Yuv == NULL)
        mHDRBuffer.Image1Yuv = (void *)malloc(framesize);
    if (mHDRBuffer.Image2Yuv == NULL)
        mHDRBuffer.Image2Yuv = (void *)malloc(framesize);
    if (mHDRBuffer.Image3Yuv == NULL)
        mHDRBuffer.Image3Yuv = (void *)malloc(framesize);
    if (mHDRBuffer.Image4Yuv == NULL)
        mHDRBuffer.Image4Yuv = (void *)malloc(framesize);


    //malloc failed, release all buffers
    if((mHDRBuffer.Image0Yuv == NULL) || \
         (mHDRBuffer.Image1Yuv == NULL) || \
         (mHDRBuffer.Image2Yuv == NULL) || \
         (mHDRBuffer.Image3Yuv == NULL) || \
         (mHDRBuffer.Image4Yuv == NULL)) {

        ReleaseHDRBuffer();
        ALOGE("Request HDR buffer failed!!!");
        ret = -1;
    }
    return ret;

}
void HDRSceneMode::ReleaseHDRBuffer()
{
    //release all buffers
    if (mHDRBuffer.Image0Yuv != NULL){
        free(mHDRBuffer.Image0Yuv);
        mHDRBuffer.Image0Yuv = NULL;
    }
    if (mHDRBuffer.Image1Yuv != NULL){
        free(mHDRBuffer.Image1Yuv);
        mHDRBuffer.Image1Yuv = NULL;
    }
    if (mHDRBuffer.Image2Yuv != NULL){
        free(mHDRBuffer.Image2Yuv);
        mHDRBuffer.Image2Yuv = NULL;
    }
    if (mHDRBuffer.Image3Yuv != NULL){
        free(mHDRBuffer.Image3Yuv);
        mHDRBuffer.Image3Yuv = NULL;
    }
    if (mHDRBuffer.Image4Yuv != NULL){
        free(mHDRBuffer.Image4Yuv);
        mHDRBuffer.Image4Yuv = NULL;
    }
}
void HDRSceneMode::SetCallBack(SceneNotifyCb Scenenotifycb,void* user)
{
    mSceneNotifyCb = Scenenotifycb;
    mUser = user;
}

int HDRSceneMode::InitSceneMode(int width,int height)
{
    int ret;
    mWidth    =    width;
    mHeight    =    height;
    ALOGD("HDR InitSceneModeframe size: %d x %d",mWidth,mHeight);

    ret = RequestHDRBuffer();
    if(ret == -1){
        ALOGE("Request HDR Buffer failed");
        return -1;
    }
    return 0;
}

int HDRSceneMode::StartScenePicture()
{
    int ret;
    const double LOG_2 = log(2);
    int value_dark, value_bright;
    struct isp_stat_buf    AeBuf;
    struct isp_stat_buf    HistBuf;

    //check callback handle
    if(mSceneNotifyCb == NULL) return -1;

    //request isp buffer
    HistBuf.buf = malloc(0x200);
    AeBuf.buf = malloc(0xc00);
    if(AeBuf.buf == NULL || HistBuf.buf == NULL){
        if(AeBuf.buf != NULL) free(AeBuf.buf);
        if(HistBuf.buf != NULL) free(HistBuf.buf);
        return -1;
    }

    //get AE State
    memset(AeBuf.buf,0,0xc00);
    mSceneNotifyCb(SCENE_NOTIFY_CMD_GET_AE_STATE,(void*)&AeBuf,&ret,mUser);

    //get Hist state
    memset(HistBuf.buf,0,0x200);
    mSceneNotifyCb(SCENE_NOTIFY_CMD_GET_HIST_STATE,(void*)&HistBuf,&ret,mUser);

    //get exposuregain
    GetExposureGain(mWidth, mHeight, (uint32_t *)AeBuf.buf, (uint32_t *)HistBuf.buf, &mGainDark, &mGainBright);

    //get dark and bright value
    value_dark = - int(25.0*log(1.0/mGainDark)/LOG_2+0.5);
    value_bright = int(25.0*log(mGainBright)/LOG_2+0.5);

    ALOGD("gain_dark = %lf, gain_bright = %lf", mGainDark, mGainBright);
    ALOGD("value_dark = %d, value_bright = %d", value_dark, value_bright);

    mHDRSetting.hdr_en = 1;
    mHDRSetting.total_frames = 2;
    mHDRSetting.values[0] = value_dark;
    mHDRSetting.values[1] = value_bright;
    mHDRSetting.values[2] = 0;
    mHDRSetting.values[3] = 0;
    mHDRSetting.values[4] = 0;
    free(AeBuf.buf);
    free(HistBuf.buf);

    // set GAIN and EXP
    mSceneNotifyCb(SCENE_NOTIFY_CMD_SET_HDR_SETTING,(void*)&mHDRSetting,&ret,mUser);
    //ALOGD("setHDRMode retrun = %d\n",ret);
    mSceneNotifyCb(SCENE_NOTIFY_CMD_SET_3A_LOCK,(void*)8,&ret,mUser);
    //ALOGD("set3ALock retrun = %d\n",ret);
    mHDRState = HDRSTEP1;
    return ret;
}

int HDRSceneMode::StopScenePicture()
{
    mHDRState = HDRSTEP0;
    return 0;
}

int HDRSceneMode::GetCurrentFrameData(void* vaddr)
{
    int ret;
    int FrameCnt;
    mSceneNotifyCb(SCENE_NOTIFY_CMD_GET_HDR_FRAME_COUNT,(void*)&FrameCnt,&ret,mUser);
    if(FrameCnt > 3) return SCENE_CAPTURE_FAIL;
    if(FrameCnt == 1)
    {
        if(mHDRState == HDRSTEP1)
        {
            ALOGD("step 1");
            //save dark picture
            int framesize = mWidth * mHeight * 3 >> 1;
            memcpy(mHDRBuffer.Image0Yuv, vaddr, framesize);
            mHDRState = HDRSTEP2;
        }
    }

    if(FrameCnt == 2)
    {
        if(mHDRState == HDRSTEP2)
        {
            ALOGD("step 2");
            //save light picture
            int frame_size = mWidth * mHeight * 3 >> 1;
            memcpy(mHDRBuffer.Image1Yuv,vaddr, frame_size);
            mHDRFinished = 1;
            mHDRState = HDRSTEP3;
        }
    }

    if((FrameCnt <= 2) && (mHDRState != HDRSTEP3)) //???
        return SCENE_CAPTURE_UNKNOW;

    if(FrameCnt > 2 && mHDRState == HDRSTEP3)
    {
        struct isp_hdr_setting_t hdr_setting;
        hdr_setting.hdr_en = 0;
        hdr_setting.total_frames = 5;
        hdr_setting.values[0] = -50;
        hdr_setting.values[1] = 0;
        hdr_setting.values[2] = 0;
        hdr_setting.values[3] = 0;
        hdr_setting.values[4] = 0;
        mSceneNotifyCb(SCENE_NOTIFY_CMD_SET_HDR_SETTING,(void*)&hdr_setting,&ret,mUser);
        mSceneNotifyCb(SCENE_NOTIFY_CMD_SET_3A_LOCK,(void*)0,&ret,mUser);
        return SCENE_CAPTURE_DONE;
    }

    return SCENE_CAPTURE_UNKNOW;
}

int HDRSceneMode::PostScenePicture(void* vaddr)
{
    if(mHDRState == HDRSTEP3){
        captureHDR(mHDRBuffer.Image0Yuv,
                   mHDRBuffer.Image1Yuv,
                   mHDRBuffer.Image2Yuv,
                   mHDRBuffer.Image3Yuv,
                   &mHDRFinished,
                   mWidth,
                   mHeight,
                   mGainBright,
                   mGainDark);
    }
    if(mHDRFinished){
        ALOGD("hdr done");
        memcpy(vaddr,mHDRBuffer.Image1Yuv,mWidth*mHeight*3 >> 1);
        return SCENE_COMPUTE_DONE;
    }
    ALOGD("hdr fail");
    return SCENE_COMPUTE_FAIL;
}
int HDRSceneMode::GetScenePictureState()
{
    return 0;
}
int HDRSceneMode::GetCurrentSceneMode()
{
    return mSceneMode;
}
void HDRSceneMode::ReleaseSceneMode()
{
    ALOGD("Release HDR SenceMode");
    ReleaseHDRBuffer();
}


};