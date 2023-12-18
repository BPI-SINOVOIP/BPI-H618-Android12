
#ifndef __HAL_CAMERA_MANAGER_H__
#define __HAL_CAMERA_MANAGER_H__

#include "CameraDebug.h"
#include <fcntl.h>
#include <cutils/list.h>
#include <utils/Mutex.h>
#include <type_camera.h>
#include "CameraHardware2.h"

#ifdef CAMERA_MANAGER_ENABLE

#define NB_COMPOSE_BUFFER 5//10

namespace android {

typedef struct BufManager_t
{
    int         write_id;
    int         read_id;
    int         buf_used;
    pthread_mutex_t                 mutex;
    pthread_cond_t                  condition;
    V4L2BUF_t   buf[NB_COMPOSE_BUFFER];
}BufManager;

enum CaptureState {
        CAPTURE_STATE_PAUSE,
        CAPTURE_STATE_READY,
        CAPTURE_STATE_STARTED,
        CAPTURE_STATE_EXIT,
        CAPTURE_STATE_IDLE,
};

class CameraManager {
public:
    CameraManager();
    ~CameraManager();
    bool composeThread();
    bool commandThread();
    bool previewThread();
    status_t setCameraHardware(int index,CameraHardware *hardware);
    int setFrameSize(int index,int width,int height);
    int setCaptureState(int index,int state);
    int queueCameraBuf(int index,V4L2BUF_t *buffer);
    void startPreview();
    void stopPreview();
    void releaseCamera();
    int composeBufInit();
    int composeBufDeinit();
    inline void setOviewEnable(bool enable)
    {
        mIsOview= enable;
    }

    inline bool isOviewEnable(void)
    {
        return mIsOview;
    }

protected:
    typedef enum CMD_CM_QUEUE_t{
        CMD_CM_START_PREVIEW    = 0,
        CMD_CM_STOP_PREVIEW,
        CMD_CM_RELEASE_CAMERA,
        CMD_CM_QUEUE_MAX
    }CMD_CM_QUEUE;

    OSAL_QUEUE                      mQueueCMCommand;

    typedef struct Queue_CM_Element_t {
        CMD_CM_QUEUE cmd;
        int data;
    }Queue_CM_Element;

    Queue_CM_Element                   mQueueCMElement[CMD_CM_QUEUE_MAX];

    class DoCommandThread : public Thread {
        CameraManager* mCameraManager;
        bool                mRequestExit;
    public:
       DoCommandThread(CameraManager* dev) :
                Thread(false),
                mCameraManager(dev),
                mRequestExit(false) {
            }
            void startThread() {
                run("CMDoCommandThread", PRIORITY_URGENT_DISPLAY);
            }
            void stopThread() {
                mRequestExit = true;
            }
            virtual bool threadLoop() {
                if (mRequestExit) {
                    return false;
                }
                return mCameraManager->commandThread();
            }
    };
    sp<DoCommandThread>             mCommandThread;

    pthread_mutex_t                 mCommandMutex;
    pthread_cond_t                  mCommandCond;

        class ComposeThread : public Thread {
            CameraManager*  mCameraManager;
            bool                mRequestExit;
        public:
            ComposeThread(CameraManager* dev) :
                Thread(false),
                mCameraManager(dev),
                mRequestExit(false) {
            }
            void startThread() {
                run("ComposeThread", PRIORITY_URGENT_DISPLAY);
            }
            void stopThread() {
                mRequestExit = true;
            }
            virtual bool threadLoop() {
                if (mRequestExit) {
                    return false;
                }
                return mCameraManager->composeThread();
            }
        };

        class PreviewThread : public Thread {
            CameraManager*  mCameraManager;
            bool                mRequestExit;
        public:
            PreviewThread(CameraManager* dev) :
                Thread(false),
                mCameraManager(dev),
                mRequestExit(false) {
            }
            void startThread() {
                run("PreviewThread", PRIORITY_URGENT_DISPLAY);
            }
            void stopThread() {
                mRequestExit = true;
            }
            virtual bool threadLoop() {
                if (mRequestExit) {
                    return false;
                }
                return mCameraManager->previewThread();
            }
        };
    int g2d_scale(unsigned long src, int src_width, int src_height, unsigned long dst,int dst_x,int dst_y, int dst_width, int dst_height);
    int g2d_clip(void* psrc, int src_w, int src_h, int src_x, int src_y, int width, int height, void* pdst, int dst_w, int dst_h, int dst_x, int dst_y);
    int g2d_compose(void* psrc1, void* psrc2, void* psrc3, void* psrc4, int src_w, int src_h, void* pdst);
	int g2d_compose_two(void* psrc1, void* psrc2, int src_w, int src_h, void* pdst);
#ifdef ENABLE_SCALE
	int g2d_scale_compose(unsigned long psrc1, unsigned long psrc2, unsigned long psrc3,
	unsigned long psrc4, int src_w, int src_h, unsigned long pdst);
#endif

public:
    int mStartCameraID;
    int mCameraTotalNum;
    int mFrameWidth;
    int mFrameHeight;
    int mComposeFrameWidth;
    int mComposeFrameHeight;
    bool mTakePicState;
    Mutex mLock;
    void releaseByIndex(int index);

private:
    CaptureState    mCaptureState;

    bool mIsOview;
    bool mCameraState[MAX_NUM_OF_CAMERAS];
    int mReleaseIndex[NB_BUFFER][MAX_NUM_OF_CAMERAS];
    BufManager mComposeBuf;
    BufManager mCameraBuf[MAX_NUM_OF_CAMERAS];
    int mAbandonFrameCnt;
    CameraHardware *                mCameraHardware[MAX_NUM_OF_CAMERAS];
    pthread_mutex_t                 mPreviewMutex;
    pthread_cond_t                  mPreviewCond;
    pthread_mutex_t                 mComposeMutex;
    pthread_cond_t                  mComposeCond;
    sp<ComposeThread>             mComposeThread;
    sp<PreviewThread>             mPreviewThread;
    int mG2DHandle;

    int ionOpen();
    int ionClose();
    V4L2BUF_t * getAvailableWriteBuf();
    bool canCompose();
    void composeBuffer2in1(unsigned char *outBuffer,unsigned char *inBuffer0,unsigned char *inBuffer1);
    void composeBuffer4in1(unsigned char *outBuffer,unsigned char *inBuffer0,unsigned char *inBuffer1,
    unsigned char *inBuffer2,unsigned char *inBuffer3);
    int queueComposeBuf();
    bool isSameSize();
    void releaseAllCameraBuff();
    struct ScCamMemOpsS* mMemOpsSCM;
};

}; /* namespace android */

#endif //CAMERA_MANAGER_ENABLE

#endif  /* __HAL_CAMERA_MANAGER_H__ */


