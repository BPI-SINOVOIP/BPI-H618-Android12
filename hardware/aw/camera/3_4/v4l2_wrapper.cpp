#undef NDEBUG

#if DBG_V4L2_WRAPPER

#endif

#define LOG_TAG "CameraHALv3_V4L2Wrapper"


#include <utils/Log.h>

#include <algorithm>
#include <array>
#include <limits>
#include <mutex>
#include <vector>

#include <fcntl.h>
#include <string.h>

//#include <linux/videodev2.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <android-base/unique_fd.h>


#include "stream_format.h"

#include "v4l2_stream.h"
#include "linux/videodev2.h"
#include "type_camera.h"
#include "v4l2_wrapper.h"

#include GPU_PUBLIC_INCLUDE
#include "camera_config.h"


extern "C" int AWJpecEnc(JpegEncInfo* pJpegInfo, EXIFInfo* pExifInfo, void* pOutBuffer, int* pOutBufferSize);

namespace v4l2_camera_hal {
const int32_t kStandardSizes[][2] = {{1920, 1080}, {640, 480}};

V4L2Wrapper* V4L2Wrapper::NewV4L2Wrapper(const int id, std::shared_ptr<CCameraConfig> pCameraCfg) {
  HAL_LOG_ENTER();
  #if 0
  std::unique_ptr<V4L2Stream> stream(V4L2Stream::NewV4L2Stream(id, device_path));
  if (!stream) {
    HAL_LOGE("Failed to initialize stream helper.");
    return nullptr;
  }
  #endif
  return new V4L2Wrapper(id,pCameraCfg);
}
std::shared_ptr<V4L2Stream> V4L2Wrapper::getStream(STREAM_SERIAL ss) {
  HAL_LOG_ENTER();

  std::lock_guard<std::mutex> lock(connection_lock_);
  STREAM_SERIAL tmpss;
  switch (ss) {
    case MAIN_STREAM:
    case MAIN_STREAM_BLOB:
    case MAIN_MIRROR_STREAM:
    case MAIN_MIRROR_STREAM_BLOB:
      tmpss = MAIN_STREAM;
    break;
    case SUB_0_STREAM:
    case SUB_0_STREAM_BLOB:
    case SUB_0_MIRROR_STREAM:
    case SUB_0_MIRROR_STREAM_BLOB:
      tmpss = SUB_0_STREAM;
    break;
    default: 
      HAL_LOGE("Failed to set stream device_path.");
      break;
  }

  if(array_stream_obj[tmpss] == nullptr) {
    HAL_LOGE("Failed to find stream obj, you should connect device first.");
    return nullptr;
  }
  HAL_LOGD("getStream %d successfully.", tmpss);

  return array_stream_obj[tmpss];
}

V4L2Wrapper::V4L2Wrapper(const int id, std::shared_ptr<CCameraConfig> pCameraCfg)
    : device_id_(id),
      mCameraConfig(pCameraCfg),
      has_StreamOn(false),
      buffer_state_(BUFFER_UNINIT),
#ifdef USE_ISP
      mAWIspApi(NULL),
      mIspId(-1),
#endif
      isTakePicure(false){
  HAL_LOG_ENTER();

  for(int ss = 0; ss < MAX_STREAM; ss++) {
    connection_count_[ss] = 0;
  }

}

V4L2Wrapper::~V4L2Wrapper() {}

int V4L2Wrapper::Connect(STREAM_SERIAL ss) {
  HAL_LOG_ENTER();
  std::lock_guard<std::mutex> lock(connection_lock_);
  std::string device_path;
  STREAM_SERIAL tmpss;
  int Support_Id = getSupportCameraId(device_id_);
  HAL_LOGD("device_id:%d Support_Id:%d", device_id_, Support_Id);
  switch (ss) {
    case MAIN_STREAM:
    case MAIN_STREAM_BLOB:
    case MAIN_MIRROR_STREAM:
    case MAIN_MIRROR_STREAM_BLOB:
    {
        if(Support_Id == 0)
        {
            device_path = MAIN_STREAM_PATH;
        }
        tmpss = MAIN_STREAM;
    }
    break;
    case SUB_0_STREAM:
    case SUB_0_STREAM_BLOB:
    case SUB_0_MIRROR_STREAM:
    case SUB_0_MIRROR_STREAM_BLOB:
    {
        if(Support_Id == 0)
        {
            device_path = SUB_0_STREAM_PATH;
        }
        tmpss = SUB_0_STREAM;
    }
    break;
    default: 
      HAL_LOGE("Failed to set stream device_path.");
      break;
  }

  if (connected(tmpss)) {
    HAL_LOGV("Camera stream %s is already connected.", device_path_.c_str());
    ++connection_count_[tmpss];
    return 0;
  }

  if(Support_Id == 1 && tmpss == MAIN_STREAM)
  {
      device_path = MAIN_FRONT_STREAM_PATH;
  }
  if(Support_Id == 1 && tmpss == SUB_0_STREAM)
  {
      device_path = SUB_0_FRONT_STREAM_PATH;
  }

  std::shared_ptr<V4L2Stream> stream(V4L2Stream::NewV4L2Stream(device_id_, device_path, mCameraConfig));
  if (!stream) {
    HAL_LOGE("Failed to initialize stream helper.");
    return -1;
  }

  stream_connection[tmpss].reset(new ConnectionStream(stream));
  if (stream_connection[tmpss]->status()) {
    HAL_LOGE("Failed to connect to device: %d.", stream_connection[tmpss]->status());
    return stream_connection[tmpss]->status();
  }

  array_stream_obj[tmpss] = stream;
  ++connection_count_[tmpss];

  return 0;
}

void V4L2Wrapper::Disconnect(STREAM_SERIAL ss) {
  HAL_LOG_ENTER();
  std::lock_guard<std::mutex> lock(connection_lock_);
  std::string device_path;
  STREAM_SERIAL tmpss;

  switch (ss) {
    case MAIN_STREAM:
    case MAIN_STREAM_BLOB:
    case MAIN_MIRROR_STREAM:
    case MAIN_MIRROR_STREAM_BLOB:
    {
        if(device_id_ == 0)
        {
            device_path = MAIN_STREAM_PATH;
        }
        tmpss = MAIN_STREAM;
    }
    break;
    case SUB_0_STREAM:
    case SUB_0_STREAM_BLOB:
    case SUB_0_MIRROR_STREAM:
    case SUB_0_MIRROR_STREAM_BLOB:
    {
        if(device_id_ == 0)
        {
            device_path = SUB_0_STREAM_PATH;
        }
        tmpss = SUB_0_STREAM;
    }
    break;
    default: 
      HAL_LOGE("Failed to set stream device_path.");
      break;
  }

  if (connection_count_[tmpss] == 0) {
    // Not connected.
    HAL_LOGE("Camera device %s is not connected, cannot disconnect.",
             device_path_.c_str());
    return;
  }

  if(device_id_ == 1 && tmpss == MAIN_STREAM)
  {
      device_path = MAIN_FRONT_STREAM_PATH;
  }
  if(device_id_ == 1 && tmpss == SUB_0_STREAM)
  {
      device_path = SUB_0_FRONT_STREAM_PATH;
  }

  --connection_count_[tmpss];
  if (connection_count_[tmpss] > 0) {
    HAL_LOGV("Disconnected from camera device %s connections remain.",
             device_path_.c_str());
    return;
  }
  stream_connection[tmpss].reset();

  //map_stream_obj_.erase(ss);
  array_stream_obj[tmpss].reset();


  device_fd_[MAIN_STREAM].reset(-1);  // Includes close().
  format_.reset();
  buffers_.clear();
}

}  // namespace v4l2_camera_hal
