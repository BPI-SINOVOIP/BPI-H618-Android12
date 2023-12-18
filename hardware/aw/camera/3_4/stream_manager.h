
// Loosely based on hardware/libhardware/modules/camera/ExampleCamera.h

#ifndef V4L2_CAMERA_HAL_STREAM_MANAGER_H_
#define V4L2_CAMERA_HAL_STREAM_MANAGER_H_

#include <array>
#include <condition_variable>
#include <map>
#include <queue>
#include <string>
#include <functional>


#include <utils/StrongPointer.h>
#include <utils/Thread.h>
#include <utils/RefBase.h>

#include "CameraMetadata.h"
#include "camera.h"
#include "common.h"
#include "metadata/metadata.h"
#include "v4l2_wrapper.h"
#include "v4l2_camera.h"

namespace v4l2_camera_hal {
class CameraStream;
class V4L2Camera;

typedef struct{
       short refCnt;
       int frameNum;
    }FrameNumRef;

class StreamManager : public android::RefBase  {
  friend class CameraStream;
  friend class V4L2Camera;

public:
  static std::shared_ptr<StreamManager> NewStreamManager(std::shared_ptr<V4L2Wrapper> device, std::shared_ptr<V4L2Camera> camera);
  CameraStream* createStream(STREAM_SERIAL ss,
                                uint32_t width, uint32_t height, int format, uint32_t usage,int isBlob);
  int configurateManager(STREAM_SERIAL ss);
  int start(STREAM_SERIAL ss);
  int stop(STREAM_SERIAL ss);
  int resultCallback(uint32_t frameNumber,struct timeval ts);
  int markFrameNumber(uint32_t frameNumber);
  int request(uint32_t frameNumber);

  ~StreamManager();
private:
  StreamManager* instance;
  int mDrop_main_buffers;
  int mDrop_sub_buffers;

protected:
   class StreamYUVMEQ : public android::Thread {
   protected:
     StreamManager*    mStreamManager;
     bool                mRequestExit;

   public:
     StreamYUVMEQ(StreamManager* dev) :
       Thread(false),
       mStreamManager(dev),
       mRequestExit(false){
     }
   
     void startThread() {
       run("StreamYUVmainEnqueue", android::PRIORITY_URGENT_DISPLAY);
     }
     void stopThread() {
       mRequestExit = true;
     }
   
     virtual bool threadLoop() {
       if (mRequestExit) {
          return false;
       }
       return mStreamManager->sYUVmainEnqueue();
     }
   };

  class StreamYUVMDQ : public android::Thread {
  protected:
    StreamManager*    mStreamManager;
    bool                mRequestExit;
    const std::string mThreadName;

  public:
    StreamYUVMDQ(StreamManager* dev) :
      Thread(false),
      mStreamManager(dev),
      mRequestExit(false){
    }

    void startThread() {
      run("StreamYUVmainDequeue", android::PRIORITY_URGENT_DISPLAY);
    }
    void stopThread() {
      mRequestExit = true;
    }

    virtual bool threadLoop() {
      if (mRequestExit) {
         return false;
      }
      return mStreamManager->sYUVmainDequeue();
    }
  };
 class StreamYUVSEQ : public android::Thread {
 protected:
   StreamManager*    mStreamManager;
   bool                mRequestExit;

 public:
   StreamYUVSEQ(StreamManager* dev) :
     Thread(false),
     mStreamManager(dev),
     mRequestExit(false){
   }
 
   void startThread() {
     run("StreamYUVsubEnqueue", android::PRIORITY_URGENT_DISPLAY);
   }
   void stopThread() {
     mRequestExit = true;
   }
 
   virtual bool threadLoop() {
     if (mRequestExit) {
        return false;
     }
     return mStreamManager->sYUVsubEnqueue();
   }
 };

class StreamYUVSDQ : public android::Thread {
protected:
  StreamManager*    mStreamManager;
  bool                mRequestExit;
  const std::string mThreadName;
  std::mutex stream_lock_;

public:
  StreamYUVSDQ(StreamManager* dev) :
    Thread(false),
    mStreamManager(dev),
    mRequestExit(false){
  }

  void startThread() {
    run("StreamYUVsubDequeue", android::PRIORITY_URGENT_DISPLAY);
  }
  void stopThread() {
    mRequestExit = true;
    std::unique_lock<std::mutex> lock(stream_lock_);
  }

  virtual bool threadLoop() {
    std::unique_lock<std::mutex> lock(stream_lock_);
    if (mRequestExit) {
       return false;
    }
    return mStreamManager->sYUVsubDequeue();
  }
};

private:
  StreamManager(std::shared_ptr<V4L2Wrapper> device,
                     std::unique_ptr<V4L2Gralloc> gralloc,
                     std::shared_ptr<V4L2Camera> camera);

  bool sYUVmainEnqueue();

  bool sYUVmainDequeue();

  bool sYUVsubEnqueue();

  bool sYUVsubDequeue();

  std::shared_ptr<V4L2Wrapper> device_;
  std::unique_ptr<V4L2Gralloc> gralloc_;
  std::shared_ptr<V4L2Camera> camera_;
  std::unique_ptr<V4L2Wrapper::Connection> mConnection[MAX_STREAM];
  std::shared_ptr<V4L2Stream> mStream[MAX_STREAM];
  std::shared_ptr<CameraStream> mCameraStream[MAX_STREAM];
  
  int64_t gtimemain;

  int64_t gtimesub;



  // Map frameNumber: refcnt about the buffer.
  std::unordered_map<uint32_t, int> mMapFrameNumRef;
  std::mutex frameNumber_lock_;
  std::condition_variable frameNumber_condition_;

  /* Defines possible states of the stream thread.
   */
  enum StreamStatus {
      /* Do not open steam. */
      STREAM_STATE_NULL,
      /* Do not open steam. */
      STREAM_STATE_PAUSED,
      /* Start steam. */
      STREAM_STATE_STARTED,
      /* exit thread*/
      STREAM_STATE_EXIT,
  };

  StreamStatus                    mYUVMEThreadState;
  android::sp<StreamYUVMEQ>       msYUVmainEnqueue;
  std::mutex                      msYUVmainEnqueueMutex;
  std::condition_variable         msYUVmainEnqueueCond;

  StreamStatus                    mYUVMDThreadState;
  android::sp<StreamYUVMDQ>       msYUVmainDequeue;
  std::mutex                      msYUVmainDequeueMutex;
  std::condition_variable         msYUVmainDequeueCond;

  StreamStatus                    mYUVSEThreadState;
  android::sp<StreamYUVSEQ>       msYUVsubEnqueue;
  std::mutex                      msYUVsubEnqueueMutex;
  std::condition_variable         msYUVsubEnqueueCond;

  StreamStatus                    mYUVSDThreadState;
  android::sp<StreamYUVSDQ>       msYUVsubDequeue;
  std::mutex                      msYUVsubDequeueMutex;
  std::condition_variable         msYUVsubDequeueCond;

  DISALLOW_COPY_AND_ASSIGN(StreamManager);
};

}  // namespace v4l2_camera_hal

#endif  // V4L2_CAMERA_HAL_V4L2_CAMERA_H_

