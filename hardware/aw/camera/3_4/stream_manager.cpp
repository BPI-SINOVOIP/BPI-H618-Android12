#if DBG_STREAM_MANAGER

#endif

#define LOG_TAG "CameraHALv3_StreamManager"
#undef NDEBUG
#include <utils/Log.h>

#include "stream_manager.h"

#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#include <cstdlib>

#include "CameraMetadata.h"
#include <hardware/camera3.h>
#include "common.h"




#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))


namespace v4l2_camera_hal {

std::shared_ptr<StreamManager> StreamManager::NewStreamManager(std::shared_ptr<V4L2Wrapper> device, std::shared_ptr<V4L2Camera> camera) {
  HAL_LOG_ENTER();

  std::unique_ptr<V4L2Gralloc> gralloc(V4L2Gralloc::NewV4L2Gralloc());

  if (!gralloc) {
    HAL_LOGE("Failed to initialize gralloc helper.");
  }
  HAL_LOGD("device.use_count:%d.", device.use_count());
  HAL_LOGD("camera.use_count:%d.", camera.use_count());
  //instance.reset(new StreamManager(std::move(device), std::move(gralloc), std::move(camera)));

  return std::shared_ptr<StreamManager>(new StreamManager(std::move(device), std::move(gralloc), std::move(camera)));
}

StreamManager::StreamManager(std::shared_ptr<V4L2Wrapper> device,
                                  std::unique_ptr<V4L2Gralloc> gralloc,
                                  std::shared_ptr<V4L2Camera> camera):
                                  gralloc_(std::move(gralloc)),
                                  device_(std::move(device)),
                                  camera_(std::move(camera)){
  HAL_LOG_ENTER();

  mDrop_main_buffers = 0;
  mDrop_sub_buffers = 0;

  //instance = std::make_shared<StreamManager>(std::shared_ptr<StreamManager>(this));

  HAL_LOGD("device.use_count:%d.", device_.use_count());
  HAL_LOGD("camera.use_count:%d.", camera_.use_count());

}

StreamManager::~StreamManager(){
  HAL_LOG_ENTER();
  HAL_LOGD("~StreamManager");

  for(int ss = 0; ss < MAX_STREAM; ss++) {
    HAL_LOGD("before reset null mStream[%d].use_count:%d.", ss, mStream[ss].use_count());
    HAL_LOGD("before reset null mCameraStream[%d].use_count:%d.", ss, mCameraStream[ss].use_count());
  }
  #if 0
  for(int ss = 0; ss < MAX_STREAM; ss++) {
    if(mStream[ss] != nullptr) {
      int res = mStream[ss]->StreamOff();
      if (res) {
        HAL_LOGE("Device failed to turn off stream.");
      }
    }
  }
  #endif
#if 1
  for(int ss = 0; ss < MAX_STREAM; ss++) {
    mConnection[ss].reset();
  }
  HAL_LOGD("mConnection");

  for(int ss = 0; ss < MAX_STREAM; ss++) {
    mStream[ss].reset();
  }
  HAL_LOGD("mStream");

  for(int ss = 0; ss < MAX_STREAM; ss++) {
    mCameraStream[ss].reset();
  }
#endif
  for(int ss = 0; ss < MAX_STREAM; ss++) {
    HAL_LOGD("mStream[%d].use_count:%d.", ss, mStream[ss].use_count());
    HAL_LOGD("mCameraStream[%d].use_count:%d.", ss, mCameraStream[ss].use_count());
  }

  HAL_LOGD("device.use_count:%d.", device_.use_count());
  HAL_LOGD("camera.use_count:%d.", camera_.use_count());

}

CameraStream* StreamManager::createStream(STREAM_SERIAL ss,
                                  uint32_t width, uint32_t height, int format, uint32_t usage, int isBlob) {
  HAL_LOG_ENTER();

  mMapFrameNumRef.clear();
  #if 0
  int i = 0;
  for(i = 0;i < MAX_FRAME_NUM;i++) {
    mMapFrameNumRef[i].refCnt = -1;
    mMapFrameNumRef[i].frameNum = -1;
  }
  #endif
  if (mConnection[ss +isBlob] != nullptr) {
    HAL_LOGD("Camera stream %d is already connected.", ss +isBlob);
    return nullptr;
  }

  mConnection[ss +isBlob].reset(new V4L2Wrapper::Connection(device_, ss));
  if (mConnection[ss +isBlob]->status()) {
    HAL_LOGE("Failed to connect to device: %d.", mConnection[ss +isBlob]->status());
    return nullptr;
  }

  mStream[ss +isBlob] = device_->getStream(ss);
  if (mStream[ss +isBlob] == nullptr) {
    HAL_LOGE("Failed to get Stream, we should connect first.");
    return nullptr;
  }

  int isThirdMirrorStream = 0;
  if(ss >= MAIN_MIRROR_STREAM) {
    isThirdMirrorStream = 1;
  }
  mCameraStream[ss +isBlob].reset(CameraStream::NewCameraStream(mStream[ss +isBlob],  isBlob, isThirdMirrorStream));
  if (mCameraStream[ss +isBlob] == nullptr) {
    HAL_LOGD("Failed to get mCameraStream ojb %d .", ss +isBlob);
    mConnection[ss +isBlob].reset();
    return nullptr;
  }

  int res = mCameraStream[ss +isBlob]->setFormat(width, height, format, usage);
  if (res) {
    HAL_LOGW("Failed to setFormat, ojb %d .", ss +isBlob);
  }

  if(ss < SUB_0_STREAM_BLOB) {
    if(isBlob) {
      if(mCameraStream[ss] == nullptr) {
        if (mCameraStream[ss +isBlob]->initialize(width, height, format, usage)) {
          HAL_LOGD("mCameraStream %d initialize failed.", ss);
          mConnection[ss +isBlob].reset();
          return nullptr;
        }
      } else {
        HAL_LOGD("mCameraStream %d has link to stream, do not need initialize.", ss +isBlob);
      }
    } else {
      if(mCameraStream[ss +1] == nullptr) {
        if (mCameraStream[ss +isBlob]->initialize(width, height, format, usage)) {
          HAL_LOGD("mCameraStream %d initialize failed.", ss);
          mConnection[ss +isBlob].reset();
          return nullptr;
        }
      } else {
        HAL_LOGD("mCameraStream %d has link to stream, do not need initialize.", ss +isBlob);
      }
    }
  }

  HAL_LOGD("mCameraStream %d created, blob flag:%d.", ss, isBlob);

  return mCameraStream[ss +isBlob].get();
}

int StreamManager::configurateManager(STREAM_SERIAL ss) {
  int res  = 0;

  if(instance == nullptr) {
    instance = this;
  }

  if(mCameraStream[ss] != nullptr) {
    res = mCameraStream[ss]->configurateManager(instance);
  }

  return res;
}

int StreamManager::start(STREAM_SERIAL ss) {
  HAL_LOGD("Stream %d to be start.", ss);
  std::lock_guard<std::mutex> guard(frameNumber_lock_);
  if(mCameraStream[ss] != nullptr) {
    mCameraStream[ss]->start();

    switch (ss) {
      case MAIN_STREAM:
      case MAIN_STREAM_BLOB:
        if(msYUVmainEnqueue == nullptr) {
          // init YUV main stream Enqueue thread
          msYUVmainEnqueue = new StreamYUVMEQ(this);
          mYUVMEThreadState = STREAM_STATE_NULL;
          msYUVmainEnqueue->startThread();
          HAL_LOGD("msYUVmainEnqueue was created.");
        }

        if(msYUVmainDequeue == nullptr) {
          // init YUV main stream Dequeue thread
          msYUVmainDequeue = new StreamYUVMDQ(this);
          mYUVMDThreadState = STREAM_STATE_NULL;
          msYUVmainDequeue->startThread();
          HAL_LOGD("msYUVmainDequeue was created.");
        }
        {
            std::unique_lock<std::mutex> lock(msYUVmainDequeueMutex);
            // singal to start main thread, start dequeue before enqueue buffer.
            if(mYUVMDThreadState == STREAM_STATE_NULL) {
                mYUVMDThreadState = STREAM_STATE_STARTED;
                msYUVmainDequeueCond.notify_one();
            }
        }
        {
            std::unique_lock<std::mutex> lock(msYUVmainEnqueueMutex);
            if(mYUVMEThreadState == STREAM_STATE_NULL) {
                  mYUVMEThreadState = STREAM_STATE_STARTED;
                  msYUVmainEnqueueCond.notify_one();
            }
        }
        break;
      case SUB_0_STREAM:
      case SUB_0_STREAM_BLOB:
        // init YUV sub stream Enqueue thread
        if(msYUVsubEnqueue == nullptr) {
          msYUVsubEnqueue = new StreamYUVSEQ(this);
          mYUVSEThreadState = STREAM_STATE_NULL;
          msYUVsubEnqueue->startThread();
          HAL_LOGD("msYUVsubEnqueue was created.");
        }

        // init YUV sub stream Dequeue thread
        if(msYUVsubDequeue == nullptr) {
          msYUVsubDequeue = new StreamYUVSDQ(this);
          mYUVSDThreadState = STREAM_STATE_NULL;
          msYUVsubDequeue->startThread();
          HAL_LOGD("msYUVsubDequeue was created.");
        }
        {
            std::unique_lock<std::mutex> lock(msYUVsubDequeueMutex);
            // singal to start main thread, start dequeue before enqueue buffer.
            if(mYUVSDThreadState == STREAM_STATE_NULL) {
                  mYUVSDThreadState = STREAM_STATE_STARTED;
                  msYUVsubDequeueCond.notify_one();
            }
        }
        {
            std::unique_lock<std::mutex> lock(msYUVsubEnqueueMutex);
            if(mYUVSEThreadState == STREAM_STATE_NULL) {
                mYUVSEThreadState = STREAM_STATE_STARTED;
                msYUVsubEnqueueCond.notify_one();
            }
        }
        break;
      default:
        break;
      }

  } else {
    HAL_LOGD("mCameraStream %d has not live.", ss);
  }

  return 0;
}
int StreamManager::stop(STREAM_SERIAL ss) {
  HAL_LOG_ENTER();
  std::lock_guard<std::mutex> guard(frameNumber_lock_);
  if(mCameraStream[ss] != nullptr) {

    switch (ss) {
      case MAIN_STREAM:
      case MAIN_STREAM_BLOB:
        // stop main thread
        if (msYUVmainEnqueue != NULL)
        {
            //std::unique_lock<std::mutex> lock(msYUVmainEnqueueMutex);
            msYUVmainEnqueue->stopThread();
            mCameraStream[ss]->flush();
            msYUVmainEnqueue.clear();
            msYUVmainEnqueue = 0;
            HAL_LOGD("msYUVmainEnqueue %d stoped.", ss);
        }
        if (msYUVmainDequeue != NULL)
        {
            //std::unique_lock<std::mutex> lock(msYUVmainDequeueMutex);
            msYUVmainDequeue->stopThread();
            mCameraStream[ss]->flush();
            msYUVmainDequeue.clear();
            msYUVmainDequeue = 0;
            HAL_LOGD("msYUVmainDequeue %d stoped.", ss);
        }
        mYUVMDThreadState = STREAM_STATE_NULL;
        mYUVMEThreadState = STREAM_STATE_NULL;

        break;
      case SUB_0_STREAM:
      case SUB_0_STREAM_BLOB:
        // singal to stop sub thread

        if (msYUVsubEnqueue != NULL)
        {
            //std::unique_lock<std::mutex> lock(msYUVsubEnqueueMutex);
            msYUVsubEnqueue->stopThread();
            mCameraStream[ss]->flush();
            msYUVsubEnqueue.clear();
            msYUVsubEnqueue = 0;
            HAL_LOGD("msYUVsubEnqueue %d stoped.", ss);
        }
        if (msYUVsubDequeue != NULL)
        {
            //std::unique_lock<std::mutex> lock(msYUVsubDequeueMutex);
            msYUVsubDequeue->stopThread();
            mCameraStream[ss]->flush();
            msYUVsubDequeue.clear();
            msYUVsubDequeue = 0;
            HAL_LOGD("msYUVsubDequeue %d stoped.", ss);
        }
        mYUVSDThreadState = STREAM_STATE_NULL;
        mYUVSEThreadState = STREAM_STATE_NULL;
        break;
      default:
        break;
    }


    mCameraStream[ss]->stop();

  }
#if 0
  if(mCameraStream[ss] != nullptr) {
    mCameraStream[ss]->flush();
  }
#endif
  if(mCameraStream[ss] != nullptr) {
    //mCameraStream[ss]->stop();
  } else {
    HAL_LOGD("mCameraStream %d has not live.", ss);
  }

  HAL_LOGD("mCameraStream %d stoped.", ss);
  return 0;
}

int StreamManager::request(uint32_t frameNumber) {
  HAL_LOG_ENTER();
  return 0;
}

int StreamManager::markFrameNumber(uint32_t frameNumber) {
  HAL_LOG_ENTER();

  std::lock_guard<std::mutex> guard(frameNumber_lock_);
  int res = 0;
  auto map_entry = mMapFrameNumRef.find(frameNumber);
  if (map_entry == mMapFrameNumRef.end()) {
    HAL_LOGD("No matching refcnt for frameNumber:%d, initialize!", frameNumber);
    mMapFrameNumRef.emplace(frameNumber,1);
    //return -ENODEV;
  } else {
    int refcnt = map_entry->second;
    if(refcnt < 1) {
      HAL_LOGE("Refcnt:%d for frameNumber:%d erased!", refcnt, frameNumber);
      mMapFrameNumRef.erase(frameNumber);
      return -ENODEV;
    }
    mMapFrameNumRef.erase(frameNumber);
    refcnt++;
    HAL_LOGD("Refcnt:%d for frameNumber:%d emplaced!", refcnt, frameNumber);
    mMapFrameNumRef.emplace(frameNumber,refcnt);
  }
  return res;
}

int StreamManager::resultCallback(uint32_t frameNumber,struct timeval ts) {
  HAL_LOG_ENTER();

  std::lock_guard<std::mutex> guard(frameNumber_lock_);
  int res = 0;
  auto map_entry = mMapFrameNumRef.find(frameNumber);
  if (map_entry == mMapFrameNumRef.end()) {
    HAL_LOGE("No matching refcnt for frameNumber:%d, something wrong!", frameNumber);
    return -ENOMEM;
  } else {
    int refcnt = map_entry->second;
    refcnt = refcnt -1;
    HAL_LOGD("Encount call back frameNumber:%d, refcnt:%d!", frameNumber, refcnt);
    if(refcnt == 0) {
      HAL_LOGD("Call back frameNumber:%d!", frameNumber);
      mMapFrameNumRef.erase(frameNumber);
      camera_->sResultCallback(frameNumber,ts);
      return res;
    }
    mMapFrameNumRef.erase(frameNumber);
    mMapFrameNumRef.emplace(frameNumber,refcnt);
  }
  return res;
}

bool StreamManager::sYUVmainEnqueue() {
  HAL_LOG_ENTER();

  {
    std::unique_lock<std::mutex> lock(msYUVmainEnqueueMutex);
    while(STREAM_STATE_STARTED != mYUVMEThreadState) {
      msYUVmainEnqueueCond.wait(lock);
    }
  }

  int res = -1;
  if(mCameraStream[MAIN_STREAM] != nullptr) {
    res = mCameraStream[MAIN_STREAM]->enqueueBuffer();
    if (res) {
    HAL_LOGE("Device EnqueueBuffer failed.");
    }
  }
  if(mCameraStream[MAIN_STREAM_BLOB] != nullptr) {
    res = mCameraStream[MAIN_STREAM_BLOB]->enqueueBuffer();
    if (res) {
    HAL_LOGE("Device EnqueueBuffer failed.");
    }
  }

  return true;
}

bool StreamManager::sYUVmainDequeue() {
  HAL_LOG_ENTER();

  {
    std::unique_lock<std::mutex> lock(msYUVmainDequeueMutex);
    while(STREAM_STATE_STARTED != mYUVMDThreadState) {
      msYUVmainDequeueCond.wait(lock);
    }
  }

  int res = -1;
  void * src_addr = nullptr;
  struct timeval stream_timestamp;
  if(mCameraStream[MAIN_STREAM] != nullptr) {
    res = mCameraStream[MAIN_STREAM]->dequeueBuffer(&src_addr,&stream_timestamp);
    if (res) {
      HAL_LOGE("Device main stream dequeueBuffer failed, src_addr:%p.", src_addr);
      if(src_addr == nullptr) {
        return true;
      }
    }
  }
  if(mCameraStream[MAIN_STREAM_BLOB] != nullptr) {
    res = mCameraStream[MAIN_STREAM_BLOB]->dequeueBuffer(&src_addr,&stream_timestamp);
    if (res) {
      HAL_LOGE("Device main blob stream dequeueBuffer failed, src_addr:%p.", src_addr);
      if(src_addr == nullptr) {
        return true;
      }
    }
  }

  if(mDrop_main_buffers <= DROP_BUFFERS_NUM) {
    mDrop_main_buffers++;
    HAL_LOGD("mDrop_main_buffers:%d, DequeueBuffer %p.", mDrop_main_buffers, src_addr);
    return true;
  }

  HAL_LOGD("Device DequeueBuffer %p.", src_addr);
  if(gtimemain >0) {
    int64_t currentTime = systemTime() / 1000000;
    int64_t deltaTime = currentTime - gtimemain;
    gtimemain = currentTime;
    HAL_LOGD("Device deltaTime %lld.", deltaTime);
  } else {
    gtimemain = systemTime() / 1000000;
  }

  void * dst_addr = nullptr;
  buffer_handle_t * buffer = nullptr;
  uint32_t frameNumber = 0;

  if(mCameraStream[MAIN_STREAM] != nullptr) {
    res = mCameraStream[MAIN_STREAM]->getBuffer(&buffer, &frameNumber);
    if(!res) {
      gralloc_->lock_handle(buffer, &dst_addr);
      if(mCameraStream[MAIN_STREAM]->copybuffer(dst_addr, src_addr)) {
        gralloc_->unlock_handle(buffer);
        HAL_LOGE("Device mian stream copybuffer failed.");
      } else {
        gralloc_->unlock_handle(buffer);
        //TODO: avoid deadlock there.
        resultCallback(frameNumber,stream_timestamp);
      }
    }
  }
  if(mCameraStream[MAIN_STREAM_BLOB] != nullptr){
    res = mCameraStream[MAIN_STREAM_BLOB]->getBuffer(&buffer, &frameNumber);
    if(!res) {
      unsigned long  mJpegBufferSizes = 0;
      int sharefd = 0;
      gralloc_->lock_handle(buffer, &dst_addr, &mJpegBufferSizes);
      if(mCameraStream[MAIN_STREAM_BLOB]->encodebuffer(dst_addr, src_addr, mJpegBufferSizes)) {
        gralloc_->unlock_handle(buffer);
        HAL_LOGE("Device main blob stream copybuffer failed.");
      } else {
        gralloc_->unlock_handle(buffer);
        //TODO: avoid deadlock there.
        resultCallback(frameNumber,stream_timestamp);
      }
    }
  }
  if(mCameraStream[MAIN_MIRROR_STREAM] != nullptr) {
    res = mCameraStream[MAIN_MIRROR_STREAM]->getBuffer(&buffer, &frameNumber);
    if(!res) {
      gralloc_->lock_handle(buffer, &dst_addr);
      if(mCameraStream[MAIN_MIRROR_STREAM]->copybuffer(dst_addr, src_addr)) {
        gralloc_->unlock_handle(buffer);
        HAL_LOGE("Device main mirror tream opybuffer failed.");
      } else {
        gralloc_->unlock_handle(buffer);
        //TODO: avoid deadlock there.
        resultCallback(frameNumber,stream_timestamp);
      }
    }
  }
  if(mCameraStream[MAIN_MIRROR_STREAM_BLOB] != nullptr){
    res = mCameraStream[MAIN_MIRROR_STREAM_BLOB]->getBuffer(&buffer, &frameNumber);
    if(!res) {
      unsigned long  mJpegBufferSizes = 0;
      gralloc_->lock_handle(buffer, &dst_addr, &mJpegBufferSizes);
      if(mCameraStream[MAIN_MIRROR_STREAM_BLOB]->encodebuffer(dst_addr, src_addr, mJpegBufferSizes)) {
        gralloc_->unlock_handle(buffer);
        HAL_LOGE("Device main mirror blob stream copybuffer failed.");
      } else {
        gralloc_->unlock_handle(buffer);
        //TODO: avoid deadlock there.
        resultCallback(frameNumber,stream_timestamp);
      }
    }
  }

  return true;
}

bool StreamManager::sYUVsubEnqueue() {
  HAL_LOG_ENTER();

  {
    std::unique_lock<std::mutex> lock(msYUVsubEnqueueMutex);
    while(STREAM_STATE_STARTED != mYUVSEThreadState) {
        msYUVsubEnqueueCond.wait(lock);
    }
  }


  int res = -1;
  if(mCameraStream[SUB_0_STREAM] != nullptr) {
    res = mCameraStream[SUB_0_STREAM]->enqueueBuffer();
    if (res) {
      HAL_LOGE("Device EnqueueBuffer failed.");
    }
  }
  if(mCameraStream[SUB_0_STREAM_BLOB] != nullptr) {
    res = mCameraStream[SUB_0_STREAM_BLOB]->enqueueBuffer();
    if (res) {
      HAL_LOGE("Device EnqueueBuffer failed.");
    }
  }

  return true;
}

bool StreamManager::sYUVsubDequeue() {
  HAL_LOG_ENTER();

  {
    std::unique_lock<std::mutex> lock(msYUVsubDequeueMutex);
    while(STREAM_STATE_STARTED != mYUVSDThreadState) {
      msYUVsubDequeueCond.wait(lock);
    }
  }

  int res = -1;
  void * src_addr = nullptr;
  struct timeval stream_timestamp;
  if(mCameraStream[SUB_0_STREAM] != nullptr) {
    res = mCameraStream[SUB_0_STREAM]->dequeueBuffer(&src_addr,&stream_timestamp);
    if (res) {
      HAL_LOGE("Device sub stream dequeueBuffer failed, src_addr:%p.", src_addr);
      if(src_addr == nullptr) {
        return true;
      }
    }
  }
  if(mCameraStream[SUB_0_STREAM_BLOB] != nullptr) {
    res = mCameraStream[SUB_0_STREAM_BLOB]->dequeueBuffer(&src_addr,&stream_timestamp);
    if (res) {
      HAL_LOGE("Device sub blob stream dequeueBuffer failed, src_addr:%p.", src_addr);
      if(src_addr == nullptr) {
        return true;
      }
    }
  }
  if(mDrop_sub_buffers <= DROP_BUFFERS_NUM) {
    mDrop_sub_buffers++;
    HAL_LOGD("mDrop_sub_buffers:%d, DequeueBuffer %p.", mDrop_sub_buffers, src_addr);
    return true;
  }

  HAL_LOGD("Device DequeueBuffer %p.", src_addr);

  if(gtimesub >0) {
    int64_t currentTime = systemTime() / 1000000;
    int64_t deltaTime = currentTime - gtimesub;
    HAL_LOGD("Device deltaTime %lld.", deltaTime);
    gtimesub = currentTime;
  } else {
    gtimesub = systemTime() / 1000000;
  }

  void * dst_addr = nullptr;
  buffer_handle_t * buffer = nullptr;
  uint32_t frameNumber = 0;
  if(mCameraStream[SUB_0_STREAM] != nullptr) {
    res = mCameraStream[SUB_0_STREAM]->getBuffer(&buffer, &frameNumber);
    if(!res) {
      gralloc_->lock_handle(buffer, &dst_addr);
      if(mCameraStream[SUB_0_STREAM]->copybuffer(dst_addr, src_addr)) {
        gralloc_->unlock_handle(buffer);
        HAL_LOGE("Device sub stream copybuffer failed.");
      } else {
        gralloc_->unlock_handle(buffer);
        //TODO: avoid deadlock there.
        resultCallback(frameNumber,stream_timestamp);

      }
    }
  }
  if(mCameraStream[SUB_0_STREAM_BLOB] != nullptr) {
    if(mCameraStream[SUB_0_STREAM_BLOB] != nullptr)
    res = mCameraStream[SUB_0_STREAM_BLOB]->getBuffer(&buffer, &frameNumber);
    if(!res) {
      unsigned long  mJpegBufferSizes = 0;
      gralloc_->lock_handle(buffer, &dst_addr, &mJpegBufferSizes);
      if(mCameraStream[SUB_0_STREAM_BLOB]->encodebuffer(dst_addr, src_addr, mJpegBufferSizes)) {
        gralloc_->unlock_handle(buffer);
        HAL_LOGE("Device copybuffer failed.");
      } else {
        gralloc_->unlock_handle(buffer);
        //TODO: avoid deadlock there.
        resultCallback(frameNumber,stream_timestamp);
      }
    }
  }
  if(mCameraStream[SUB_0_MIRROR_STREAM] != nullptr) {
    res = mCameraStream[SUB_0_MIRROR_STREAM]->getBuffer(&buffer, &frameNumber);
    if(!res) {
      gralloc_->lock_handle(buffer, &dst_addr);
      if(mCameraStream[SUB_0_MIRROR_STREAM]->copybuffer(dst_addr, src_addr)) {
        gralloc_->unlock_handle(buffer);
        HAL_LOGE("Device sub mirror stream copybuffer failed.");
      } else {
        gralloc_->unlock_handle(buffer);
        //TODO: avoid deadlock there.
        resultCallback(frameNumber,stream_timestamp);
      }
    }
  }
  if(mCameraStream[SUB_0_MIRROR_STREAM_BLOB] != nullptr) {
    if(mCameraStream[SUB_0_MIRROR_STREAM_BLOB] != nullptr)
    res = mCameraStream[SUB_0_MIRROR_STREAM_BLOB]->getBuffer(&buffer, &frameNumber);
    if(!res) {
      unsigned long  mJpegBufferSizes = 0;
      gralloc_->lock_handle(buffer, &dst_addr, &mJpegBufferSizes);
      if(mCameraStream[SUB_0_MIRROR_STREAM_BLOB]->encodebuffer(dst_addr, src_addr, mJpegBufferSizes)) {
        gralloc_->unlock_handle(buffer);
        HAL_LOGE("Device sub mirror blob stream copybuffer failed.");
      } else {
        gralloc_->unlock_handle(buffer);
        //TODO: avoid deadlock there.
        resultCallback(frameNumber,stream_timestamp);
      }
    }
  }
  return true;
}

}
