#include "CameraDebug.h"

#if DBG_CAMERA_MANAGER
#define LOG_NDEBUG 0
#endif
#define LOG_TAG "CameraManager"
#include <cutils/log.h>
#include "CameraManager.h"
#include <sys/time.h>

#ifdef CAMERA_MANAGER_ENABLE
#include <g2d_driver.h>
#include "G2dApi.h"
#define SAVE_FRAME 0
#if SAVE_FRAME
static int count = 0;
#endif
#define ABANDONFRAMECT  15
#define MAX_WIDTH 1280
#define MAX_HEIGHT 720
namespace android {

CameraManager::CameraManager()
    :mStartCameraID(CAMERA_ID_START)
    ,mCameraTotalNum(NB_CAMERA)
    ,mFrameWidth(MAX_WIDTH)
    ,mFrameHeight(MAX_HEIGHT)
    ,mComposeFrameWidth(MAX_WIDTH * 2)
    ,mComposeFrameHeight(MAX_HEIGHT * 2)
    ,mTakePicState(false)
    ,mCaptureState(CAPTURE_STATE_PAUSE)
    ,mIsOview(false)
    ,mAbandonFrameCnt(ABANDONFRAMECT)
    ,mG2DHandle(-1)
    ,mMemOpsSCM(NULL)
{
    memset(&mComposeBuf,0x0,sizeof(BufManager));
    memset(&mCameraBuf,0x0,sizeof(BufManager)*MAX_NUM_OF_CAMERAS);
    memset(mCameraState,0,sizeof(bool)*MAX_NUM_OF_CAMERAS);
    memset(mReleaseIndex, 0, sizeof(mReleaseIndex));
    for(int i=0;i<MAX_NUM_OF_CAMERAS;i++)
    {
        mCameraHardware[i] = NULL;
    }

    composeBufInit();

    //init mutex and condition
    pthread_mutex_init(&mPreviewMutex,NULL);
    pthread_cond_init(&mPreviewCond,NULL);

    pthread_mutex_init(&mComposeMutex,NULL);
    pthread_cond_init(&mComposeCond,NULL);

    // init command queue
    OSAL_QueueCreate(&mQueueCMCommand, CMD_CM_QUEUE_MAX);
    memset((void*)mQueueCMElement, 0, sizeof(mQueueCMElement));

    // init command thread
    pthread_mutex_init(&mCommandMutex, NULL);
    pthread_cond_init(&mCommandCond, NULL);
    mCommandThread = new DoCommandThread(this);
    mCommandThread->startThread();

    // init Compose thread
    mComposeThread = new ComposeThread(this);
    mComposeThread->startThread();
    mPreviewThread = new PreviewThread(this);
    mPreviewThread->startThread();
    F_LOGD;

}

int CameraManager::ionOpen()
{
    mMemOpsSCM = MemCamAdapterGetOpsS();

    int ret = mMemOpsSCM->open_cam();
    if (ret < 0)
    {
        ALOGE("ion_alloc_open failed");
        return -1;
    }
    F_LOGD;
    return 0;
}

int CameraManager::ionClose()
{
    if (mMemOpsSCM != NULL)
    {
        mMemOpsSCM->close_cam();
        mMemOpsSCM == NULL;
        F_LOGD;
    }
    return 0;
}

int CameraManager::composeBufInit()
{
   ionOpen();
   for(int i = 0;i< NB_COMPOSE_BUFFER;i++)
   {
        if (mCameraTotalNum == 2) {
            int size = mFrameWidth*mFrameHeight*3;
            mComposeBuf.buf[i].addrVirY = (unsigned long)mMemOpsSCM->palloc_cam(size,&mComposeBuf.buf[i].nShareBufFd);
            mComposeBuf.buf[i].addrPhyY = (unsigned long)mMemOpsSCM->cpu_get_phyaddr_cam((void*)mComposeBuf.buf[i].addrVirY);
            memset((void*)mComposeBuf.buf[i].addrVirY, 0x10, mFrameWidth*mFrameHeight *2);
            memset((void*)(mComposeBuf.buf[i].addrVirY + mFrameWidth*mFrameHeight *2),
                        0x80, mFrameWidth*mFrameHeight);
        }
        if (mCameraTotalNum == 4) {
#if ENABLE_SCALE
            int size = SCALE_WIDTH*SCALE_HEIGHT*3/2;
            mComposeBuf.buf[i].addrVirY = (unsigned long)mMemOpsSCM->palloc_cam(size,&mComposeBuf.buf[i].nShareBufFd);
            mComposeBuf.buf[i].addrPhyY = (unsigned long)mMemOpsSCM->cpu_get_phyaddr_cam((void*)mComposeBuf.buf[i].addrVirY);
            memset((void*)mComposeBuf.buf[i].addrVirY, 0x10, SCALE_WIDTH*SCALE_HEIGHT);
            memset((void*)(mComposeBuf.buf[i].addrVirY + SCALE_WIDTH*SCALE_HEIGHT),
                        0x80, SCALE_WIDTH*SCALE_HEIGHT/2);
#else
            int size = mFrameWidth*mFrameHeight*4*3/2;
            mComposeBuf.buf[i].addrVirY = (unsigned long)mMemOpsSCM->palloc_cam(size,&mComposeBuf.buf[i].nShareBufFd);
            mComposeBuf.buf[i].addrPhyY = (unsigned long)mMemOpsSCM->cpu_get_phyaddr_cam((void*)mComposeBuf.buf[i].addrVirY);
            memset((void*)mComposeBuf.buf[i].addrVirY, 0x10, mFrameWidth*mFrameHeight *4);
            memset((void*)(mComposeBuf.buf[i].addrVirY + mFrameWidth*mFrameHeight *4),
                        0x80, mFrameWidth*mFrameHeight*2);
#endif
        }
   }
   mG2DHandle = open("/dev/g2d", O_RDWR, 0);
   if (mG2DHandle < 0)
   {
        LOGE("open /dev/g2d failed");
        return -1;
   }

   F_LOGD;
   return 0;
}

int CameraManager::composeBufDeinit()
{
    for(int i = 0;i< NB_COMPOSE_BUFFER;i++)
    {
        mMemOpsSCM->pfree_cam((void*)mComposeBuf.buf[i].addrVirY);
        mComposeBuf.buf[i].addrPhyY = 0;
    }

    if(mG2DHandle != 0)
    {
        close(mG2DHandle);
        mG2DHandle = 0;
    }

    ionClose();
    F_LOGD;
    return 0;
}

V4L2BUF_t * CameraManager::getAvailableWriteBuf()
{
    V4L2BUF_t *ret = NULL;

    ALOGV("Used %d compose buffers in max %d", mComposeBuf.buf_used, NB_COMPOSE_BUFFER);
    pthread_mutex_lock(&mPreviewMutex);
    if (mComposeBuf.buf_used < NB_COMPOSE_BUFFER -1)
    {
        ret = &mComposeBuf.buf[mComposeBuf.write_id];
    }
    pthread_mutex_unlock(&mPreviewMutex);
    F_LOGD;
    return ret;
}

bool CameraManager::canCompose()
{
    int i,j;
    bool canCompose = true;

    pthread_mutex_lock(&mComposeMutex);

    for (i = 0; i < mCameraTotalNum; i++)
    {
        if (mCameraBuf[i].buf_used <= 0)
        {
            ALOGV("mCameraBuf[camera %d].buf_used==0", i + mStartCameraID);
            canCompose = false;
            break;
        }
    }

    for (i = 0; i < mCameraTotalNum; i++)
    {
        if (mCameraBuf[i].buf_used > NB_COMPOSE_BUFFER -2)
        {
            V4L2BUF_t buffer;
            memcpy(&buffer, &mCameraBuf[i].buf[mCameraBuf[i].read_id], sizeof(V4L2BUF_t));
            mCameraBuf[i].read_id++;
            mCameraBuf[i].buf_used--;
            if (mCameraBuf[i].read_id >= NB_COMPOSE_BUFFER)
            {
                mCameraBuf[i].read_id = 0;
            }
            pthread_mutex_unlock(&mComposeMutex);
            ALOGV("too many buffers so release camrea[%d].index=%d refcnt=%d", i + mStartCameraID, buffer.index, buffer.refCnt);
            mCameraHardware[i+mStartCameraID]->releasePreviewFrame(buffer.index);
            pthread_mutex_lock(&mComposeMutex);
        }
    }

    pthread_mutex_unlock(&mComposeMutex);
    F_LOGD;
    return canCompose;
}

void CameraManager::releaseAllCameraBuff()
{
    int i;
    bool canCompose = true;
    pthread_mutex_lock(&mComposeMutex);
    for(i=0;i<mCameraTotalNum;i++)
    {
        while(mCameraBuf[i].buf_used > 0)
        {
            V4L2BUF_t buffer;// = (mCameraBuf[i].buf[mCameraBuf[i].read_id]);
            memcpy(&buffer,&mCameraBuf[i].buf[mCameraBuf[i].read_id],sizeof(V4L2BUF_t));
            mCameraBuf[i].read_id++;
            mCameraBuf[i].buf_used--;
            if(mCameraBuf[i].read_id >= NB_COMPOSE_BUFFER)
            {
                mCameraBuf[i].read_id = 0;
            }
            pthread_mutex_unlock(&mComposeMutex);
            mCameraHardware[i+mStartCameraID]->releasePreviewFrame(buffer.index);
            pthread_mutex_lock(&mComposeMutex);
            ALOGV("FUNC:%s, CamID=%d, mCameraBuf[i].buf_used =%d", __FUNCTION__, i, mCameraBuf[i].buf_used);
        }
    }
    pthread_mutex_unlock(&mComposeMutex);
    F_LOGD;
}

void CameraManager::releaseByIndex(int index)
{
    if (isOviewEnable())
    {
        for (int i = 0; i < mCameraTotalNum; i++)
        {
            mCameraHardware[mStartCameraID + i]->mV4L2CameraDevice->releasePreviewFrame(mReleaseIndex[index][i]);
            ALOGV("[camera %d].release buffer index=%d", i + mStartCameraID, mReleaseIndex[index][i]);
        }
    }
}

int CameraManager::setFrameSize(int index,int width,int height)
{
    pthread_mutex_lock(&mComposeMutex);
    mFrameWidth = width;
    mFrameHeight = height;
    LOGD("V4L2-Debug,F:%s,L:%d,mFrameWidth:%d,mFrameHeight:%d,id:%d",__FUNCTION__,__LINE__,mFrameWidth,mFrameHeight,index);
    pthread_mutex_unlock(&mComposeMutex);
    F_LOGD;
    return 0;
}

bool CameraManager::isSameSize()
{
    int width;
    int height;
    int i;

    width = mCameraHardware[mStartCameraID]->getFrameWidth();
    height = mCameraHardware[mStartCameraID]->getFrameHeight();
    for(i=1;i<mCameraTotalNum;i++)
    {
        if((width != mCameraHardware[i+ mStartCameraID]->getFrameWidth())
            ||(height != mCameraHardware[i+mStartCameraID]->getFrameHeight()))
            return false;
    }
    F_LOGD;

    return true;
}

int CameraManager::queueCameraBuf(int index,V4L2BUF_t *buffer)
{
    int i;

    pthread_mutex_lock(&mComposeMutex);

    if ((mCaptureState == CAPTURE_STATE_EXIT) || (mCaptureState == CAPTURE_STATE_PAUSE))
    {
        pthread_mutex_unlock(&mComposeMutex);
        ALOGE("Camera[%d], invalid capture state:%d, F:%s,L:%d", index, mCaptureState, __FUNCTION__, __LINE__);
        return -1;
    }

    ALOGV("Camera[%d], Capture state:%d, F:%s,L:%d", index, mCaptureState, __FUNCTION__, __LINE__);

    if ((index < mStartCameraID) || (index >= mStartCameraID + mCameraTotalNum))
    {
        pthread_mutex_unlock(&mComposeMutex);
        ALOGE("Camera[%d] out of range [%d,%d)", index, mStartCameraID, mStartCameraID + mCameraTotalNum);
        return -1;
    }

    index = index - mStartCameraID;

    if (mCaptureState == CAPTURE_STATE_READY)
    {
        mCameraState[index] = true;
        for (i = 0; i < mCameraTotalNum; i++)
        {
            if (mCameraState[i] == false)
            {
                pthread_mutex_unlock(&mComposeMutex);
                ALOGI("Camera[%d] is not ready yet!", i + mStartCameraID);
                return -1;
            }
        }

        if (!isSameSize())
        {
            mCaptureState = CAPTURE_STATE_PAUSE;
            pthread_mutex_unlock(&mComposeMutex);
            ALOGE("Camera size is not the same,So it's can not be composed");
            return -1;
        }

        ALOGV("OK,It's will do camera buffer compose");
        mCaptureState = CAPTURE_STATE_STARTED;
    }

    if (mCameraBuf[index].buf_used < NB_COMPOSE_BUFFER)
    {
        buffer->refMutex.lock();
        buffer->refCnt++;
        buffer->refMutex.unlock();
        ALOGV("Camera[%d], buffer index:%d, refcnt:%d", index + mStartCameraID, buffer->index, buffer->refCnt);
        memcpy(&mCameraBuf[index].buf[mCameraBuf[index].write_id], buffer, sizeof(V4L2BUF_t));
        mCameraBuf[index].write_id++;
        if(mCameraBuf[index].write_id >= NB_COMPOSE_BUFFER)
        {
            mCameraBuf[index].write_id = 0;
        }
        mCameraBuf[index].buf_used++;
        ALOGV("F:%s,L:%d,Camera[%d],buf_used:%d,buf_index=%d",__FUNCTION__,__LINE__,index + mStartCameraID,mCameraBuf[index].buf_used,buffer->index);
    }
    else
    {
        ALOGE("Camera[%d].buf_used >= NB_COMPOSE_BUFFER",index + mStartCameraID);
    }

    pthread_cond_signal(&mComposeCond);
    pthread_mutex_unlock(&mComposeMutex);
    F_LOGD;
    return 0;
}

//#if 0
void CameraManager::composeBuffer2in1(unsigned char *outBuffer,unsigned char *inBuffer0,unsigned char *inBuffer1)
{
    //origin V4L2CameraDevice composebuffer need 60 ms
    //int buf1 = buf[0].
    //copy y
    int i=0;
    int n=0;
    unsigned char *pbuf;
    unsigned char *pbuf0;
    unsigned char *pbuf1;
    int size_y = mFrameWidth * mFrameHeight;
    int size_uv = mFrameWidth * mFrameHeight/2;

    if((inBuffer0 == 0) || (inBuffer1 == 0))
    {
        memset((void*)outBuffer, 0x10, size_y);
        memset((void*)(outBuffer + size_y), 0x80, size_uv);
        return;
    }

    memcpy(outBuffer,inBuffer0,size_y);
    memcpy(outBuffer+size_y,inBuffer1,size_y);

    //copy uv
    pbuf = outBuffer+size_y*2;
    pbuf0 = inBuffer0+size_y;
    pbuf1 = inBuffer1+size_y;

    memcpy(pbuf,pbuf0,size_uv);
    memcpy(pbuf+size_uv,pbuf1,size_uv);

    F_LOGD;

}

//#else
void CameraManager::composeBuffer4in1(unsigned char *outBuffer,unsigned char *inBuffer0,unsigned char *inBuffer1,
    unsigned char *inBuffer2,unsigned char *inBuffer3)
{
    //origin V4L2CameraDevice composebuffer need 60 ms
    //int buf1 = buf[0].
    //copy y
    int i=0;
    int n=0;
    unsigned char *pbuf;
    unsigned char *pbuf0;
    unsigned char *pbuf1;
    unsigned char *pbuf2;
    unsigned char *pbuf3;
    if((inBuffer0 == 0) || (inBuffer1 == 0) || (inBuffer2 == 0) || (inBuffer3 == 0))
    {
        memset((void*)outBuffer, 0x10, mFrameWidth*2 * mFrameHeight*2);
            memset((void*)(outBuffer + mFrameWidth*2 * mFrameHeight*2),
                    0x80, mFrameWidth * mFrameHeight*2);
            return;
    }
    for (i= 0;i < mFrameHeight;i++)
    {
        memcpy(outBuffer+n*mFrameWidth,inBuffer0+i*mFrameWidth,mFrameWidth);
        memcpy(outBuffer+(n+1)*mFrameWidth,inBuffer1+i*mFrameWidth,mFrameWidth);
        n += 2;
    }
    pbuf = outBuffer+mFrameHeight*mFrameWidth*2;
    n = 0;
    for (i= 0;i < mFrameHeight;i++)
    {
        memcpy(pbuf+n*mFrameWidth,inBuffer2+i*mFrameWidth,mFrameWidth);
        memcpy(pbuf+(n+1)*mFrameWidth,inBuffer3+i*mFrameWidth,mFrameWidth);
        n += 2;
    }
    //copy uv
    pbuf = outBuffer+mFrameWidth*mFrameHeight*4;
    pbuf0 = inBuffer0+mFrameWidth*mFrameHeight;
    pbuf1 = inBuffer1+mFrameWidth*mFrameHeight;
    pbuf2 = inBuffer2+mFrameWidth*mFrameHeight;
    pbuf3 = inBuffer3+mFrameWidth*mFrameHeight;
    n=0;
    for(i=0;i<mFrameHeight/2;i++)
    {
        memcpy(pbuf+n*mFrameWidth,pbuf0+i*mFrameWidth,mFrameWidth);
        memcpy(pbuf+(n+1)*mFrameWidth,pbuf1+i*mFrameWidth,mFrameWidth);
        n += 2;
    }
    pbuf = outBuffer+mFrameWidth*mFrameHeight*5;
    n = 0;
    for(i=0;i<mFrameHeight/2;i++)
    {
        memcpy(pbuf+n*mFrameWidth,pbuf2+i*mFrameWidth,mFrameWidth);
        memcpy(pbuf+(n+1)*mFrameWidth,pbuf3+i*mFrameWidth,mFrameWidth);
        n += 2;
    }
    F_LOGD;

}
//#endif

//queue compose buffer and send condition signal to another wait thread
int CameraManager::queueComposeBuf()
{
    pthread_mutex_lock(&mPreviewMutex);
    mComposeBuf.write_id++;
    if (mComposeBuf.write_id >= NB_COMPOSE_BUFFER)
    {
        mComposeBuf.write_id = 0;
    }
    mComposeBuf.buf_used++;
    F_LOGD;
    pthread_cond_signal(&mPreviewCond);
    pthread_mutex_unlock(&mPreviewMutex);
    return 0;
}

bool CameraManager::previewThread()
{
    V4L2BUF_t *buf = NULL;
    F_LOGD;

    if (mComposeBuf.buf_used <= 0)
    {
        ALOGV("Has no enough compose buffer, sleep in F:%s,L:%d", __FUNCTION__, __LINE__);
        pthread_mutex_lock(&mPreviewMutex);
        pthread_cond_wait(&mPreviewCond, &mPreviewMutex);
        pthread_mutex_unlock(&mPreviewMutex);
    }

    pthread_mutex_lock(&mPreviewMutex);
    buf = &mComposeBuf.buf[mComposeBuf.read_id];
    mComposeBuf.read_id++;
    if(mComposeBuf.read_id >= NB_COMPOSE_BUFFER)
    {
        mComposeBuf.read_id = 0;
    }
    mComposeBuf.buf_used--;
    pthread_mutex_unlock(&mPreviewMutex);

    ALOGV("%d frames into one,mCaptureState=%d", mCameraTotalNum, mCaptureState);

    if (mCameraTotalNum == 2)
    {
        buf->width = mFrameWidth;
        buf->height = mFrameHeight * 2;
        buf->crop_rect.left     = 0;
        buf->crop_rect.top      = 0;
        buf->crop_rect.width    = mFrameWidth - 1;
        buf->crop_rect.height   = mFrameHeight * 2 - 1;
    }
    else if (mCameraTotalNum == 4)
    {
#if ENABLE_SCALE
        buf->width = SCALE_WIDTH;
        buf->height = SCALE_HEIGHT;
        buf->crop_rect.left     = 0;
        buf->crop_rect.top      = 0;
        buf->crop_rect.width    = SCALE_WIDTH - 1;
        buf->crop_rect.height   = SCALE_HEIGHT - 1;
#else
        buf->width = mFrameWidth * 2;
        buf->height = mFrameHeight * 2;
        buf->crop_rect.left     = 0;
        buf->crop_rect.top      = 0;
        buf->crop_rect.width    = mFrameWidth * 2 -1;
        buf->crop_rect.height   = mFrameHeight * 2 - 1;
#endif
    }

    if (buf->timeStamp <= 0)
    {
        buf->timeStamp = (int64_t)systemTime();
    }

    buf->format = V4L2_PIX_FMT_NV21;
    buf->refCnt = 1;
    buf->isThumbAvailable = 0;

    if (mAbandonFrameCnt > 0)
    {
        mAbandonFrameCnt--;
        if (mCameraTotalNum == 2)
        {
            memset((void*)buf->addrVirY, 0x10, mFrameWidth * mFrameHeight * 2);
            memset((void*)(buf->addrVirY + mFrameWidth * mFrameHeight * 2), 0x80, mFrameWidth * mFrameHeight);
        }
        else if (mCameraTotalNum == 4)
        {
#if ENABLE_SCALE
            memset((void*)buf->addrVirY, 0x10, SCALE_WIDTH * SCALE_HEIGHT);
            memset((void*)(buf->addrVirY + SCALE_WIDTH * SCALE_HEIGHT), 0x80, SCALE_WIDTH * SCALE_HEIGHT/2);
#else
            memset((void*)buf->addrVirY, 0x10, mFrameWidth * 2 * mFrameHeight * 2);
            memset((void*)(buf->addrVirY + mFrameWidth * 2 * mFrameHeight *2), 0x80, mFrameWidth * mFrameHeight * 2);
#endif
        }
    }

    if (mCaptureState == CAPTURE_STATE_STARTED)
    {
        if (mTakePicState)
        {
            mTakePicState = false;
            mCameraHardware[mStartCameraID]->mV4L2CameraDevice->takePicInputBuffer(buf);
        }

        V4L2BUF_t *tmpBuf[1];
        tmpBuf[0] = buf;
        mCameraHardware[mStartCameraID]->mCallbackNotifier.onNextFrameAvailable((const void **)(&tmpBuf), 1, NOT_NEED_RELEASE_INDEX);
        F_LOGD;
    }

    return true;
}

bool CameraManager::composeThread()
{
    int i;
    V4L2BUF_t *writeBuffer = NULL;
    V4L2BUF_t inbuffer[mCameraTotalNum];

    memset(&inbuffer, 0x0, sizeof(V4L2BUF_t) * mCameraTotalNum);

    if (mCaptureState == CAPTURE_STATE_EXIT)
    {
        ALOGV("Capture state:%d, sleep in F:%s,L:%d", mCaptureState, __FUNCTION__, __LINE__);
        releaseAllCameraBuff();
        pthread_mutex_lock(&mComposeMutex);
        pthread_cond_wait(&mComposeCond, &mComposeMutex);
        pthread_mutex_unlock(&mComposeMutex);
        return true;
    }

    if (mCaptureState == CAPTURE_STATE_PAUSE)
    {
        ALOGV("Capture state:%d, sleep in F:%s,L:%d", mCaptureState, __FUNCTION__, __LINE__);
        pthread_mutex_lock(&mComposeMutex);
        pthread_cond_wait(&mComposeCond, &mComposeMutex);
        pthread_mutex_unlock(&mComposeMutex);
        return true;
    }

    bool bufReady = canCompose();
    if (!bufReady)
    {
        ALOGV("Has no enough camera buffer, sleep in F:%s,L:%d", __FUNCTION__, __LINE__);
        pthread_mutex_lock(&mComposeMutex);
        struct timespec timeout;
        timeout.tv_sec=time(0);
        timeout.tv_nsec = 100000000; // 100ms timeout
        int ret = pthread_cond_timedwait(&mComposeCond, &mComposeMutex, &timeout);
        pthread_mutex_unlock(&mComposeMutex);
        return true;
    }

    pthread_mutex_lock(&mComposeMutex);
    for (i = 0; i < mCameraTotalNum; i++)
    {
        memcpy(&inbuffer[i], &mCameraBuf[i].buf[mCameraBuf[i].read_id], sizeof(V4L2BUF_t));
        mCameraBuf[i].read_id++;
        mCameraBuf[i].buf_used--;
        if (mCameraBuf[i].read_id >= NB_COMPOSE_BUFFER)
        {
            mCameraBuf[i].read_id = 0;
        }
    }
    pthread_mutex_unlock(&mComposeMutex);

    if (isOviewEnable())
    {
        V4L2BUF_t *tmpBuf[mCameraTotalNum];
        int sIndex = inbuffer[0].index;

        for (i = 0; i < mCameraTotalNum; i++)
        {
            tmpBuf[i] = &inbuffer[i];
            mReleaseIndex[sIndex][i] = inbuffer[i].index;
            ALOGV("[camera %d].need release index=%d", i + mStartCameraID, inbuffer[i].index);
        }

        mCameraHardware[mStartCameraID]->mCallbackNotifier.onNextFrameAvailable((const void **)(&tmpBuf), mCameraTotalNum, NEED_RELEASE_INDEX);

        ALOGV("Upload %d frames success", mCameraTotalNum);
    }
    else
    {
        writeBuffer = getAvailableWriteBuf();
        if (writeBuffer != NULL)
        {
            writeBuffer->timeStamp = inbuffer[0].timeStamp;

            if (mCameraTotalNum == 2)
            {
                ALOGV("Compse 2 in 1");
                if (mG2DHandle > 0)
                {
                    mMemOpsSCM->flush_cache_cam((void*)writeBuffer->addrVirY, mFrameWidth * mFrameHeight*3);
                    g2d_compose_two((unsigned char *)(inbuffer[0].addrPhyY),
                                (unsigned char *)(inbuffer[1].addrPhyY),
                                mFrameWidth, mFrameHeight, (unsigned char *)writeBuffer->addrPhyY);
                }
                else
                {
                    composeBuffer2in1((unsigned char *)writeBuffer->addrVirY, (unsigned char *)(inbuffer[0].addrVirY),
                                (unsigned char *)(inbuffer[1].addrVirY));
                }

                //mCameraHardware[mStartCameraID]->addWaterMark((unsigned char *)writeBuffer->addrVirY, mFrameWidth, mFrameHeight*2);
#if SAVE_FRAME
                count++;
                if (count <= 10)
                {
                    char buffer[128] = {0};
                    sprintf(buffer,"/data/camera/nv21_%d", count);
                    saveframe(buffer, (void *)writeBuffer->addrVirY, mFrameWidth*mFrameHeight*3, 1);
                }
#endif
            }
            else if (mCameraTotalNum == 4)
            {
                ALOGV("Compse 4 in 1");
                if (mG2DHandle > 0)
                {
#if ENABLE_SCALE
                    mMemOpsSCM->flush_cache_cam((void*)writeBuffer->addrVirY, SCALE_WIDTH*SCALE_HEIGHT*3/2);
                    g2d_scale_compose(inbuffer[0].addrPhyY,inbuffer[1].addrPhyY,inbuffer[2].addrPhyY,inbuffer[3].addrPhyY,
                                mFrameWidth, mFrameHeight, writeBuffer->addrPhyY);
#else
                    mMemOpsSCM->flush_cache_cam((void*)writeBuffer->addrVirY, mFrameWidth * mFrameHeight*3/2*4);
                    g2d_compose((unsigned char *)(inbuffer[0].addrPhyY),
                                (unsigned char *)(inbuffer[1].addrPhyY),
                                (unsigned char *)(inbuffer[2].addrPhyY),
                                (unsigned char *)(inbuffer[3].addrPhyY),
                                mFrameWidth, mFrameHeight, (unsigned char *)writeBuffer->addrPhyY);
#endif
                }
                else
                {
                    composeBuffer4in1((unsigned char *)writeBuffer->addrVirY,
                                      (unsigned char *)(inbuffer[0].addrVirY),
                                      (unsigned char *)(inbuffer[1].addrVirY),
                                      (unsigned char *)(inbuffer[2].addrVirY),
                                      (unsigned char *)(inbuffer[3].addrVirY));
                }
#if ENABLE_SCALE
                //mCameraHardware[mStartCameraID]->addWaterMark((unsigned char *)writeBuffer->addrVirY, SCALE_WIDTH,SCALE_HEIGHT);
#else
                //mCameraHardware[mStartCameraID]->addWaterMark((unsigned char *)writeBuffer->addrVirY, mFrameWidth*2, mFrameHeight*2);
#endif

#if SAVE_FRAME
                count++;
                if (count <= 10)
                {
                    char buffer[128]= {0};
                    int len;
#if ENABLE_SCALE
                    sprintf(buffer,"/data/camera/nv21_%dx%d_%d", SCALE_WIDTH, SCALE_HEIGHT, count);
                    len = SCALE_WIDTH * SCALE_HEIGHT;
#else
                    sprintf(buffer,"/data/camera/nv21_%dx%d_%d", mFrameWidth * 2, mFrameHeight * 2, count);
                    len = mFrameWidth * 2 * mFrameHeight * 2;
#endif
                    saveframe(buffer, (void *)writeBuffer->addrVirY, len, 1);
                }
#endif
            }

            queueComposeBuf();
        }
        else
        {
            ALOGE("preview thread get compose data too slow!!");
            usleep(5000);
        }
    }

    for (i = 0; i < mCameraTotalNum; i++)
    {
        if (inbuffer[i].addrVirY != 0)
        {
            ALOGV("release [camera %d].index=%d", i + mStartCameraID, inbuffer[i].index);
            mCameraHardware[i + mStartCameraID]->releasePreviewFrame(inbuffer[i].index);
        }
    }

    return true;
}

bool CameraManager::commandThread()
{
    int i;
    pthread_mutex_lock(&mCommandMutex);
    Queue_CM_Element * queue = (Queue_CM_Element *)OSAL_Dequeue(&mQueueCMCommand);
    if (queue == NULL)
    {
        ALOGV("Sleep in F:%s,L:%d", __FUNCTION__, __LINE__);
        pthread_cond_wait(&mCommandCond, &mCommandMutex);
        pthread_mutex_unlock(&mCommandMutex);
        return true;
    }
    pthread_mutex_unlock(&mCommandMutex);
    switch(queue->cmd)
    {
        case CMD_CM_START_PREVIEW:
        {
            LOGD("CameraManDubug,F:%s,L:%d",__FUNCTION__,__LINE__);
            for(i= mStartCameraID+ 1;i<mCameraTotalNum + mStartCameraID;i++)
            {
                ALOGD("commandThread mCameraHardware[%d]->startPreview_h(),mCameraHardware[%d]=0x%x",i,i,mCameraHardware[i]);
                LOGD("CameraManDubug,F:%s,L:%d",__FUNCTION__,__LINE__);
                mCameraHardware[i]->startPreview_h();
                ALOGD("commandThread mCameraHardware[%d]->startPreview_h() end",i);
            }
            LOGD("CameraManDubug,F:%s,L:%d",__FUNCTION__,__LINE__);

            pthread_mutex_lock(&mComposeMutex);
            for(i=0;i<mCameraTotalNum;i++)
            {
                mCameraBuf[i].buf_used = 0;
            }

            pthread_mutex_unlock(&mComposeMutex);
            F_LOGD;
            break;
        }
        case CMD_CM_STOP_PREVIEW:
        {
            int i = 0;
            for(i= mStartCameraID+ 1;i<mCameraTotalNum + mStartCameraID;i++)
            {
                mCameraHardware[i]->stopPreview_h();
            }
            F_LOGD;
            break;
        }
        case CMD_CM_RELEASE_CAMERA:
        {
            int i = 0;
            for(i= mStartCameraID+ 1;i<mCameraTotalNum + mStartCameraID;i++)
            {
                F_LOG;
                mCameraHardware[i]->releaseCamera_h();
                F_LOG;
            }
            F_LOGD;
            break;
        }
        default:
            ALOGW("unknown queue command: %d", queue->cmd);
            break;
    }
    return true;
}

void CameraManager::startPreview()
{
#if SAVE_FRAME
    count = 0;
#endif
    memset(mCameraState,0,sizeof(bool)*MAX_NUM_OF_CAMERAS);
    mAbandonFrameCnt = ABANDONFRAMECT;
    pthread_mutex_lock(&mCommandMutex);
    mQueueCMElement[CMD_CM_START_PREVIEW].cmd = CMD_CM_START_PREVIEW;
    OSAL_Queue(&mQueueCMCommand, &mQueueCMElement[CMD_CM_START_PREVIEW]);
    pthread_cond_signal(&mCommandCond);
    pthread_mutex_unlock(&mCommandMutex);
    pthread_mutex_lock(&mComposeMutex);
    mCaptureState = CAPTURE_STATE_READY;
    pthread_mutex_unlock(&mComposeMutex);
    LOGD("CameraManDubug,F:%s,L:%d",__FUNCTION__,__LINE__);
    F_LOGD;
}

void CameraManager::stopPreview()
{
    pthread_mutex_lock(&mComposeMutex);
    mCaptureState = CAPTURE_STATE_PAUSE;
    pthread_mutex_unlock(&mComposeMutex);
    Mutex::Autolock autoLock(mLock);
    pthread_mutex_lock(&mCommandMutex);
    mQueueCMElement[CMD_CM_STOP_PREVIEW].cmd = CMD_CM_STOP_PREVIEW;
    OSAL_Queue(&mQueueCMCommand, &mQueueCMElement[CMD_CM_STOP_PREVIEW]);
    pthread_cond_signal(&mCommandCond);
    pthread_mutex_unlock(&mCommandMutex);
    F_LOGD;

}

void CameraManager::releaseCamera()
{
    F_LOGD;
    pthread_mutex_lock(&mComposeMutex);
    mCaptureState = CAPTURE_STATE_EXIT;
    pthread_mutex_unlock(&mComposeMutex);
    pthread_mutex_lock(&mCommandMutex);
    mQueueCMElement[CMD_CM_RELEASE_CAMERA].cmd = CMD_CM_RELEASE_CAMERA;
    OSAL_Queue(&mQueueCMCommand, &mQueueCMElement[CMD_CM_RELEASE_CAMERA]);
    pthread_cond_signal(&mCommandCond);
    pthread_mutex_unlock(&mCommandMutex);
    //composeBufDeinit();
    if(isOviewEnable())
        setOviewEnable(false);
}

status_t CameraManager::setCameraHardware(int index,CameraHardware *hardware)
{
    if(index < mStartCameraID)
        return EINVAL;
    if (hardware == NULL)
    {
        LOGE("ERROR hardware info");
        return EINVAL;
    }
    mCameraHardware[index] = hardware;
    return NO_ERROR;
}

CameraManager::~CameraManager()
{
    ALOGV("~CameraManager");
    if (mComposeThread != NULL)
    {
        mComposeThread->stopThread();
        pthread_cond_signal(&mPreviewCond);
        mComposeThread.clear();
        mComposeThread = 0;
    }
    if (mCommandThread != NULL)
    {
        mCommandThread->stopThread();
        pthread_cond_signal(&mCommandCond);
        mCommandThread.clear();
        mCommandThread = 0;
    }
    if(mPreviewThread != NULL)
    {
        mPreviewThread->stopThread();
        mPreviewThread.clear();
        mPreviewThread = 0;
    }
    composeBufDeinit();
    pthread_mutex_destroy(&mCommandMutex);
    pthread_cond_destroy(&mCommandCond);

    pthread_mutex_destroy(&mPreviewMutex);
    pthread_cond_destroy(&mPreviewCond);

    pthread_mutex_destroy(&mComposeMutex);
    pthread_cond_destroy(&mComposeCond);
    OSAL_QueueTerminate(&mQueueCMCommand);
}

int CameraManager::g2d_scale(unsigned long src, int src_width, int src_height, unsigned long dst, int dst_x,int dst_y,int dst_width, int dst_height)
{
    g2d_stretchblt scale;

    scale.flag = G2D_BLT_NONE;//G2D_BLT_NONE;//G2D_BLT_PIXEL_ALPHA|G2D_BLT_ROTATE90;
    scale.src_image.addr[0] = src;
    scale.src_image.addr[1] = src + src_width*src_height;
    scale.src_image.w = src_width;
    scale.src_image.h = src_height;
    scale.src_image.format = G2D_FMT_PYUV420UVC;
    scale.src_image.pixel_seq = G2D_SEQ_NORMAL;
    scale.src_rect.x = 0;
    scale.src_rect.y = 0;
    scale.src_rect.w = src_width;
    scale.src_rect.h = src_height;

    scale.dst_image.addr[0] = dst;
    scale.dst_image.addr[1] = dst + dst_width*dst_height;
    //scale.dst_image.addr[2] = (int)dst + dst_width * dst_height * 5 / 4;
    scale.dst_image.w = dst_width;
    scale.dst_image.h = dst_height;
    scale.dst_image.format =G2D_FMT_PYUV420UVC;// G2D_FMT_PYUV420UVC;
    scale.dst_image.pixel_seq = G2D_SEQ_NORMAL;
    scale.dst_rect.x = dst_x;
    scale.dst_rect.y = dst_y;
    scale.dst_rect.w = dst_width/2;
    scale.dst_rect.h = dst_height/2;
    scale.color = 0xff;
    scale.alpha = 0xff;

    if(ioctl(mG2DHandle, G2D_CMD_STRETCHBLT, &scale)<0){
        LOGE("g2d_scale: failed to call G2D_CMD_STRETCHBLT!!!\n");
        return -1;
    }
    return 0;
}

int CameraManager::g2d_clip(void* psrc, int src_w, int src_h, int src_x, int src_y, int width, int height, void* pdst, int dst_w, int dst_h, int dst_x, int dst_y)
{
    g2d_blt     blit_para;
    int         err;

    blit_para.src_image.addr[0]      = (int)psrc;
    blit_para.src_image.addr[1]      = (int)psrc + src_w * src_h;
    blit_para.src_image.w            = src_w;
    blit_para.src_image.h            = src_h;
    blit_para.src_image.format       = G2D_FMT_PYUV420UVC;
    blit_para.src_image.pixel_seq    = G2D_SEQ_NORMAL;
    blit_para.src_rect.x             = src_x;
    blit_para.src_rect.y             = src_y;
    blit_para.src_rect.w             = width;
    blit_para.src_rect.h             = height;

    blit_para.dst_image.addr[0]      = (int)pdst;
    blit_para.dst_image.addr[1]      = (int)pdst + dst_w * dst_h;
    blit_para.dst_image.w            = dst_w;
    blit_para.dst_image.h            = dst_h;
    blit_para.dst_image.format       = G2D_FMT_PYUV420UVC;
    blit_para.dst_image.pixel_seq    = G2D_SEQ_NORMAL;
    blit_para.dst_x                  = dst_x;
    blit_para.dst_y                  = dst_y;
    blit_para.color                  = 0xff;
    blit_para.alpha                  = 0xff;

    blit_para.flag = G2D_BLT_NONE;

    err = ioctl(mG2DHandle, G2D_CMD_BITBLT, (unsigned long)&blit_para);
    if(err < 0)
    {
        LOGE("g2d_clip: failed to call G2D_CMD_BITBLT!!!\n");
        return -1;
    }

    return 0;
}

int CameraManager::g2d_compose_two(void* psrc1, void* psrc2, int src_w, int src_h, void* pdst)
{
    int dst_w, dst_h;
    int ret;

	//example todo
	//src_w =2560
	//src_h = 720;
	//dst_w= 2560
	//dst_h = 1440

    dst_w = src_w;
    dst_h = src_h * 2;

    ret  = g2d_clip(psrc1, src_w, src_h, 0, 0, src_w/2, src_h, pdst, dst_w, dst_h, 0, 0);
    ret |= g2d_clip(psrc1, src_w, src_h, src_w/2, 0, src_w/2, src_h, pdst, dst_w, dst_h, src_w/2, 0);
    ret |= g2d_clip(psrc2, src_w, src_h, 0, 0, src_w/2, src_h, pdst, dst_w, dst_h, 0, src_h);
    ret |= g2d_clip(psrc2, src_w, src_h, src_w/2, 0, src_w/2, src_h, pdst, dst_w, dst_h, src_w/2, src_h);


    if (ret != 0) {
        LOGE("g2d_compose: failed to call g2dYuvspClip!!!\n");
        return -1;
    }
    F_LOGD;

    return 0;
}

int CameraManager::g2d_compose(void* psrc1, void* psrc2, void* psrc3, void* psrc4, int src_w, int src_h, void* pdst)
{
    int dst_w, dst_h;
    int ret;

    dst_w = src_w * 2;
    dst_h = src_h * 2;

    ret  = g2d_clip(psrc1, src_w, src_h, 0, 0, src_w, src_h, pdst, dst_w, dst_h, 0, 0);
    ret |= g2d_clip(psrc2, src_w, src_h, 0, 0, src_w, src_h, pdst, dst_w, dst_h, src_w, 0);
    ret |= g2d_clip(psrc3, src_w, src_h, 0, 0, src_w, src_h, pdst, dst_w, dst_h, 0, src_h);
    ret |= g2d_clip(psrc4, src_w, src_h, 0, 0, src_w, src_h, pdst, dst_w, dst_h, src_w, src_h);

    if (ret != 0) {
        LOGE("g2d_compose: failed to call g2dYuvspClip!!!\n");
        return -1;
    }
    F_LOGD;

    return 0;
}

#if ENABLE_SCALE
int CameraManager::g2d_scale_compose(unsigned long psrc1, unsigned long psrc2, unsigned long psrc3,
	unsigned long psrc4, int src_w, int src_h, unsigned long pdst)
{
    int dst_w, dst_h;
    int ret;

    dst_w = src_w * 2;
    dst_h = src_h * 2;

	ret |= g2d_scale(psrc1, src_w, src_h, pdst, 0,0,SCALE_WIDTH, SCALE_HEIGHT);
	ret |= g2d_scale(psrc2, src_w, src_h, pdst, SCALE_WIDTH/2,0,SCALE_WIDTH, SCALE_HEIGHT);
	ret |= g2d_scale(psrc3, src_w, src_h, pdst, 0,SCALE_HEIGHT/2,SCALE_WIDTH, SCALE_HEIGHT);
	ret |= g2d_scale(psrc4, src_w, src_h, pdst, SCALE_WIDTH/2,SCALE_HEIGHT/2,SCALE_WIDTH, SCALE_HEIGHT);

    if (ret != 0) {
        LOGE("g2d_scal_compose: failed to call g2dYuvspClip!!!\n");
        return -1;
    }
    F_LOGD;

    return 0;
}
#endif

}

#endif //CAMERA_MANAGER_ENABLE
