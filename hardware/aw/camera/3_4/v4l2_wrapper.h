
#ifndef V4L2_CAMERA_HAL_V4L2_WRAPPER_H_
#define V4L2_CAMERA_HAL_V4L2_WRAPPER_H_

#include <array>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>
#include <queue>

#include <android-base/unique_fd.h>

#include "common.h"
#include "stream_format.h"

#include "v4l2_stream.h"

//#include "videodev2_34.h"
#ifdef USE_ISP
#include "AWIspApi.h"
#endif

#include "vencoder.h"
#include "camera_config.h"
//#include "MetadataBufferType.h"

namespace v4l2_camera_hal {
class V4L2Stream;
class ConnectionStream;



class V4L2Wrapper {
  //friend class V4L2Stream;
  //friend class V4L2Stream::Connection;


  public:
  // Use this method to create V4L2Wrapper objects. Functionally equivalent
  // to "new V4L2Wrapper", except that it may return nullptr in case of failure.
  static V4L2Wrapper* NewV4L2Wrapper(const int id, std::shared_ptr<CCameraConfig> pCameraCfg);
  std::shared_ptr<V4L2Stream> getStream(STREAM_SERIAL ss);

  virtual ~V4L2Wrapper();

  // Helper class to ensure all opened connections are closed.
  class Connection {
   public:
    Connection(std::shared_ptr<V4L2Wrapper> device, STREAM_SERIAL ss)
        : device_(std::move(device)), ss_(std::move(ss)), connect_result_(device_->Connect(ss)) {}
    ~Connection() {
      if (connect_result_ == 0) {
        device_->Disconnect(ss_);
      }
    }
    // Check whether the connection succeeded or not.
    inline int status() const { return connect_result_; }

   private:
    std::shared_ptr<V4L2Wrapper> device_;
    const int connect_result_;
    STREAM_SERIAL ss_;
  };

  // Tools.
 int GetDeviceId(){ return device_id_;};


 private:
  std::shared_ptr<CCameraConfig> mCameraConfig;
  //CCameraConfig *    mCameraConfig;
  // Constructor is private to allow failing on bad input.
  // Use NewV4L2Wrapper instead.
  V4L2Wrapper(const int id, std::shared_ptr<CCameraConfig> pCameraCfg);

  // Connect or disconnect to the device. Access by creating/destroying
  // a V4L2Wrapper::Connection object.
  int Connect(STREAM_SERIAL ss);
  void Disconnect(STREAM_SERIAL ss);

  std::unordered_map<std::string, android::base::unique_fd> map_stream_fd_;
  std::unordered_map<STREAM_SERIAL, std::shared_ptr<V4L2Stream>> map_stream_obj_;
  std::shared_ptr<V4L2Stream> array_stream_obj[MAX_STREAM];
  std::shared_ptr<ConnectionStream> stream_connection[MAX_STREAM];

  inline bool connected(STREAM_SERIAL ss) {
    if(array_stream_obj[ss] == nullptr) {
      HAL_LOGD("Do not find stream obj.");
      return false;
    }
    return true;
  }

  // The camera device path. For example, /dev/video0.
  const std::string device_path_;
  // The opened device fd.
  android::base::unique_fd device_fd_[MAX_STREAM];

  // The open camera facing.
  const int device_id_;
  // The underlying gralloc module.
  std::unique_ptr<V4L2Gralloc> gralloc_;
  // Whether or not the device supports the extended control query.
  bool extended_query_supported_;
  // The format this device is set up for.
  std::unique_ptr<StreamFormat> format_;
  //
  bool has_StreamOn;
  //
  bool isTakePicure;
  enum{
    BUFFER_UNINIT,
    BUFFER_PREPARE,
    BUFFER_QUEUE,
    BUFFER_DEQUEUE
    } buffer_state_;
  // Map indecies to buffer status. True if the index is in-flight.
  // |buffers_.size()| will always be the maximum number of buffers this device
  // can handle in its current format.
  std::vector<bool> buffers_pstream_;

  // Map indecies to buffer status. True if the index is in-flight.
  // |buffers_.size()| will always be the maximum number of buffers this device
  // can handle in its current format.
  std::vector<bool> buffers_;

  // Lock protecting use of the buffer tracker.
  std::mutex buffer_queue_lock_;
  std::queue<int>buffers_num_;
  std::condition_variable buffer_availabl_queue_;


  // Buffer manager.
  std::mutex cmd_queue_lock_;
  int buffer_cnt_inflight_;
  // Lock protecting use of the device.
  std::mutex device_lock_;
  // Lock protecting connecting/disconnecting the device.
  std::mutex connection_lock_;
  // Reference count connections.
  int connection_count_[MAX_STREAM];

  // Debug tools for save buffers.
  void * buffers_addr[MAX_BUFFER_NUM];
  int buffers_fd[MAX_BUFFER_NUM];

#ifdef USE_ISP
  android::AWIspApi                          *mAWIspApi;
  int                               mIspId;
#endif

  friend class Connection;
  friend class V4L2WrapperMock;
#ifdef USE_ISP
  friend class android::AWIspApi;
#endif

  DISALLOW_COPY_AND_ASSIGN(V4L2Wrapper);
};

}  // namespace v4l2_camera_hal

#endif  // V4L2_CAMERA_HAL_V4L2_WRAPPER_H_
