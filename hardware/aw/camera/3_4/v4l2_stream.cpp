


#if DBG_V4L2_STREAM

#endif
#define LOG_TAG "CameraHALv3_V4L2Stream"
#undef NDEBUG

#include <android/log.h>


#include <algorithm>
#include <array>
#include <limits>
#include <mutex>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <sstream>

#include <fcntl.h>
#include <string.h>

//#include <linux/videodev2.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
//#include <stdio.h>

#include <utils/Timers.h>
#include <cutils/properties.h>



#include <android-base/unique_fd.h>

#include "common.h"
#include "v4l2_stream.h"// Helpers of logging (showing function name and line number).
#include "camera_config.h"
#include "memory/ion_alloc.h"
// For making difference between main stream and sub stream.
#define HAL_LOGD(fmt, args...) if(LOG_LEVEL >= 3) \
    ALOGD("%s:%d:%d: " fmt, __func__, __LINE__, device_ss_, ##args)
#define HAL_LOGV(fmt, args...) if(LOG_LEVEL >= 4) \
    ALOGV("%s:%d:%d: " fmt, __func__, __LINE__, device_ss_,  ##args)

#include "stream_format.h"
#include "v4l2_gralloc.h"
#include "linux/videodev2.h"
#include "type_camera.h"
#include GPU_PUBLIC_INCLUDE


extern "C" int AWJpecEnc(JpegEncInfo* pJpegInfo, EXIFInfo* pExifInfo, void* pOutBuffer, int* pOutBufferSize);

#define ISP_3A_PARAM_SIZE  81412
#define ISP_DEBUG_MSG_SIZE 20796
#define ISP_DEBUG_MAGIC_STR  "ISPDEBUG"

namespace v4l2_camera_hal {


static int mFlashMode = V4L2_FLASH_LED_MODE_NONE;
static bool mFlashOk = false;
const int32_t kStandardSizes[][2] = {{1920, 1080}, {640, 480}};

V4L2Stream* V4L2Stream::NewV4L2Stream(const int id, const std::string device_path, std::shared_ptr<CCameraConfig> pCameraCfg) {
  return new V4L2Stream(id, device_path, pCameraCfg);
}
V4L2Stream::V4L2Stream(const int id,
                              const std::string device_path, std::shared_ptr<CCameraConfig> pCameraCfg)
    : device_id_(id),
      device_path_(std::move(device_path)),
      mCameraConfig(pCameraCfg),
      has_StreamOn(false),
      mTestPattern(0),
      buffer_state_(BUFFER_UNINIT),
      isTakePicure(false),
      mflush_buffers(false),
#ifdef USE_ISP
      mAWIspApi(NULL),
      mIspId(-1),
#endif
      mInterpolation_src_width(0),
      mInterpolation_src_height(0),
      mInterpolation_dst_width(0),
      mInterpolation_dst_height(0),
      mStream_max_width(0),
      mStream_max_height(0),
      disconnect(false),
      connection_count_(0),
      device_fd_(-1) {
  int pipefd[2];
  int ret = -1;
  HAL_LOG_ENTER();
  if(device_path_.compare(MAIN_STREAM_PATH) == 0) {
    device_ss_ = MAIN_STREAM;
  } else if(device_path_.compare(SUB_0_STREAM_PATH) == 0) {
    device_ss_ = SUB_0_STREAM;
  } else if(device_path_.compare(MAIN_FRONT_STREAM_PATH) == 0) {
    device_ss_ = MAIN_STREAM;
  } else if(device_path_.compare(SUB_0_FRONT_STREAM_PATH) == 0) {
    device_ss_ = SUB_0_STREAM;
  }

  memset(&jpeg_crop_rect,0,sizeof(cam_crop_rect_t));
  memset(&crop_data, 0, sizeof(aw_crop_data_t));
  char * value;
  if (mCameraConfig->supportInterpolationSize()) {
       int interpolation_dst_width = 0;
       int interpolation_dst_height = 0;
       int interpolation_src_width = 0;
       int interpolation_src_height = 0;
       value = mCameraConfig->supportInterpolationSizeValue();
       parse_pair(value, &interpolation_src_width, &interpolation_src_height, 'x');
       value = mCameraConfig->defaultInterpolationSizeValue();
       parse_pair(value, &interpolation_dst_width, &interpolation_dst_height, 'x');
       mInterpolation_dst_height = interpolation_dst_height;
       mInterpolation_dst_width  = interpolation_dst_width;
       mInterpolation_src_height = interpolation_src_height;
       mInterpolation_src_width  = interpolation_src_width;
       mStream_max_width         = interpolation_src_width;
       mStream_max_height        = interpolation_src_height;
  } else {
     value = mCameraConfig->defaultPictureSizeValue();
     sscanf(value, "%dx%d", &mStream_max_width,&mStream_max_height);
  }
  jpeg_crop_rect.left   = 0;
  jpeg_crop_rect.top    = 0;
  jpeg_crop_rect.width  = mStream_max_width;
  jpeg_crop_rect.height = mStream_max_height;
  ret = pipe(pipefd);
  if (ret == -1) {
      ALOGD("V4L2Stream create pipe failed");
  } else {
      read_fd_ = pipefd[0];
      write_fd_ = pipefd[1];
  }
  pEvents = (epoll_event *)calloc(2, sizeof(epoll_event));
}

V4L2Stream::~V4L2Stream() {
  HAL_LOG_ENTER();
  std::unique_lock<std::mutex> lock(buffer_queue_lock_);
  HAL_LOGD("~V4L2Stream %s, device_ss_:%d.", device_path_.c_str(), device_ss_);
  close(read_fd_);
  close(write_fd_);
}

int V4L2Stream::Connect() {
  HAL_LOG_ENTER();
  std::lock_guard<std::mutex> lock(connection_lock_);

  if (connected()) {
    HAL_LOGV("Camera stream %s is already connected.", device_path_.c_str());
    ++connection_count_;
    return 0;
  }
  HAL_LOGD("Camera stream will link to %s.", device_path_.c_str());
  int try_num = 5;
  int fd = -1;
  while(try_num--) {
    HAL_LOGD("try to link %s, the %d time.", device_path_.c_str(), 5 -try_num);
    // Open in nonblocking mode (DQBUF may return EAGAIN).
    fd = TEMP_FAILURE_RETRY(open(device_path_.c_str(), O_RDWR | O_NONBLOCK, 0));
    if (fd < 0) {
      HAL_LOGE("failed to open %s (%s)", device_path_.c_str(), strerror(errno));
      //return -ENODEV;
      usleep(200*1000);
      continue;
    }
    break;
  }
  if (fd < 0) {
    HAL_LOGE("failed to open %s (%s)", device_path_.c_str(), strerror(errno));
    return -ENODEV;
  }

  //device_fd_.reset(fd);
  device_fd_ = fd;
  ++connection_count_;
  ion_alloc_open();
  HAL_LOGV("Detect camera stream %s, stream serial:%d.", device_path_.c_str(), device_ss_);

  AWCropInit(&crop_data);
  // TODOzjw: setting ccameraconfig for open front&back sensor .
  struct v4l2_input inp;
  inp.index = getSupportCameraId(device_id_);
   if (TEMP_FAILURE_RETRY(ioctl(fd, VIDIOC_S_INPUT, &inp)) != 0) {
     HAL_LOGE(
         "VIDIOC_S_INPUT on %d error: %s.", inp.index, strerror(errno));
   }
#ifdef USE_ISP
   mAWIspApi = new android::AWIspApi();
#endif

  return 0;
}

void V4L2Stream::Disconnect() {
  HAL_LOG_ENTER();
  std::lock_guard<std::mutex> lock(connection_lock_);
  if (connection_count_ == 0) {
    // Not connected.
    HAL_LOGE("Camera device %s is not connected, cannot disconnect.",
             device_path_.c_str());
    return;
  }

  --connection_count_;
  if (connection_count_ > 0) {
    HAL_LOGV("Disconnected from camera device %s. %d connections remain.",
             device_path_.c_str());
    return;
  }
  std::unique_lock<std::mutex> lock_1(disconnection_lock_);
  AWCropExit(&crop_data);
  //wake up epoll
  disconnect = true;
  write(write_fd_, "w", 2);
  int res = TEMP_FAILURE_RETRY(close(device_fd_));
  HAL_LOGV("Close device path:%s, fd:%d, res: %s", device_path_.c_str(), device_fd_, strerror(res));
  if (res) {
    HAL_LOGW("Disconnected from camera device %s. fd:%d encount err(%s).",device_path_.c_str(), device_fd_, strerror(res));
  }
  // Delay for open after close success encount open device busy.
  //TODO: optimize this, keep node open until close the camera hal.
  //usleep(200*1000);

#ifdef USE_ISP
  if (mAWIspApi != NULL) {
      delete mAWIspApi;
      mAWIspApi = NULL;
  }
#endif
  for (size_t i = 0; i < buffers_.size(); ++i) {
    buffers_[i] = false;
  }
  // munmap buffer.
  for (int i = 0; i < buffers_.size(); i++)
  {
    switch (format_->memory()) {
        case V4L2_MEMORY_USERPTR:
            aw_ion_free(stream_buffers.start[i]);
            break;
        case V4L2_MEMORY_DMABUF:
            aw_ion_free(stream_buffers.start[i]);
            break;
        default:
            HAL_LOGE("not support memory type: %d", format_->memory());
            break;
    }
  }
  aw_ion_free(mPatternBlock.start);
  ion_alloc_close();

  //device_fd_.reset(-1);  // Includes close().
  device_fd_ = -1;
  format_.reset();
  buffers_.clear();
  // Closing the device releases all queued buffers back to the user.
}

// Helper function. Should be used instead of ioctl throughout this class.
template <typename T>
int V4L2Stream::IoctlLocked(int request, T data) {
  // Potentially called so many times logging entry is a bad idea.
  std::lock_guard<std::mutex> lock(device_lock_);

  if (!connected()) {
    HAL_LOGE("Stream %s not connected.", device_path_.c_str());
    return -ENODEV;
  }
  HAL_LOGV("Stream fd:%d..", device_fd_);
  return TEMP_FAILURE_RETRY(ioctl(device_fd_, request, data));
}

int V4L2Stream::StreamOn() {
  HAL_LOG_ENTER();

  if (!format_) {
    HAL_LOGE("Stream format must be set before turning on stream.");
    return -EINVAL;
  }

  if (has_StreamOn) {
    HAL_LOGV("Stream had been turned on.");
    return 0;
  }
#if DELAY_BETWEEN_ON_OFF
  mTimeStampsFstreamon = systemTime() / 1000000;
#endif
  int mDevice_id = getSupportCameraId(device_id_);
  HAL_LOGD("id:%d mDevice_id:%d\n", device_id_, mDevice_id);
  if (device_id_ == 0 && device_id_ != mDevice_id) {
      struct v4l2_control ctrl;
      ctrl.id = V4L2_CID_VFLIP;
      ctrl.value = 1;
      if (TEMP_FAILURE_RETRY(ioctl(device_fd_, VIDIOC_S_CTRL, &ctrl)) != 0) {
              HAL_LOGE( "VIDIOC_S_CTRL  error: %s. value:%d", strerror(errno), ctrl.value);
      }
  }

  int32_t type = format_->type();
  if (IoctlLocked(VIDIOC_STREAMON, &type) < 0) {
    HAL_LOGE("STREAMON fails: %s", strerror(errno));
    return -ENODEV;
  }else {
    buffer_state_ = BUFFER_UNINIT;
    has_StreamOn = true;
  }
#if DELAY_BETWEEN_ON_OFF
  HAL_LOGV("Stream turned on.");
  usleep(100*1000);
  HAL_LOGV("Stream after turned on sleep for stream on prepare.");
#endif


#ifdef USE_ISP
  mIspId = 0;
  if (getSingleCameraId() < 0){
      mIspId = mAWIspApi->awIspGetIspId(mDevice_id);
  }
  if (mIspId >= 0) {
      mAWIspApi->awIspStart(mIspId);
      HAL_LOGD("ISP turned on.");
  } else {
    HAL_LOGE("ISP turned on failed!");
  }
#endif
   return 0;
}

int V4L2Stream::StreamOff() {
  HAL_LOG_ENTER();

  std::lock_guard<std::mutex> lock(stream_lock_);
  if (!format_) {
    // Can't have turned on the stream without format being set,
    // so nothing to turn off here.
    return 0;
  }
#if DELAY_BETWEEN_ON_OFF
  // TODO: Remove it.
  // Delay between vin stream on and off time that less than DELAY_BETWEEN_STREAM
  // for resource release completely.
  unsigned long  mDeltaStream = systemTime() / 1000000 - mTimeStampsFstreamon;
  HAL_LOGD("mDeltaStream:%ld, mTimeStampsFstreamon:%ld, systemTime() / 1000000:%ld.", mDeltaStream, mTimeStampsFstreamon, systemTime() / 1000000);
  if(mDeltaStream < DELAY_BETWEEN_STREAM) {
    HAL_LOGD("mDeltaStream:%ld.", mDeltaStream);
    usleep((DELAY_BETWEEN_STREAM -mDeltaStream)*1000);
  }
#endif
  int32_t type = format_->type();
  int res = IoctlLocked(VIDIOC_STREAMOFF, &type);
  if (res) {
    HAL_LOGW("Stream turned off failed, err(%s).",strerror(res));
    //return res;
  }
  if (res < 0) {
    HAL_LOGE("STREAMOFF fails: %s", strerror(errno));
    //return -ENODEV;
  }
  HAL_LOGD("After stream %d, ind:%d turned off.", device_id_, device_fd_);
#ifdef USE_ISP
  mAWIspApi->awIspStop(mIspId);
  HAL_LOGV("Stream %d, ind:%d awIspStop.", device_id_, device_fd_);
#endif

  // Calling STREAMOFF releases all queued buffers back to the user.
  //int gralloc_res = gralloc_->unlockAllBuffers();
  // No buffers in flight.
  has_StreamOn = false;
  HAL_LOGV("Stream %d, ind:%d turned off.", device_id_, device_fd_);
  return 0;
}

int V4L2Stream::flush() {
  HAL_LOG_ENTER();
  mflush_buffers = true;
  buffer_availabl_queue_.notify_one();
  HAL_LOGV("Stream %d, ss:%d, ind:%d flush.", device_id_, device_ss_, device_fd_);
  return 0;
}

int V4L2Stream::QueryControl(uint32_t control_id,
                              v4l2_query_ext_ctrl* result) {
  int res;

  memset(result, 0, sizeof(*result));

  if (extended_query_supported_) {
    result->id = control_id;
    res = IoctlLocked(VIDIOC_QUERY_EXT_CTRL, result);
    // Assuming the operation was supported (not ENOTTY), no more to do.
    if (errno != ENOTTY) {
      if (res) {
        HAL_LOGE("QUERY_EXT_CTRL fails: %s", strerror(errno));
        return -ENODEV;
      }
      return 0;
    }
  }

  // Extended control querying not supported, fall back to basic control query.
  v4l2_queryctrl query;
  query.id = control_id;
  if (IoctlLocked(VIDIOC_QUERYCTRL, &query)) {
    HAL_LOGE("QUERYCTRL fails: %s", strerror(errno));
    return -ENODEV;
  }

  // Convert the basic result to the extended result.
  result->id = query.id;
  result->type = query.type;
  memcpy(result->name, query.name, sizeof(query.name));
  result->minimum = query.minimum;
  if (query.type == V4L2_CTRL_TYPE_BITMASK) {
    // According to the V4L2 documentation, when type is BITMASK,
    // max and default should be interpreted as __u32. Practically,
    // this means the conversion from 32 bit to 64 will pad with 0s not 1s.
    result->maximum = static_cast<uint32_t>(query.maximum);
    result->default_value = static_cast<uint32_t>(query.default_value);
  } else {
    result->maximum = query.maximum;
    result->default_value = query.default_value;
  }
  result->step = static_cast<uint32_t>(query.step);
  result->flags = query.flags;
  result->elems = 1;
  switch (result->type) {
    case V4L2_CTRL_TYPE_INTEGER64:
      result->elem_size = sizeof(int64_t);
      break;
    case V4L2_CTRL_TYPE_STRING:
      result->elem_size = result->maximum + 1;
      break;
    default:
      result->elem_size = sizeof(int32_t);
      break;
  }

  return 0;
}

int V4L2Stream::GetControl(uint32_t control_id, int32_t* value) {
  // For extended controls (any control class other than "user"),
  // G_EXT_CTRL must be used instead of G_CTRL.
  if (V4L2_CTRL_ID2CLASS(control_id) != V4L2_CTRL_CLASS_USER) {
    v4l2_ext_control control;
    v4l2_ext_controls controls;
    memset(&control, 0, sizeof(control));
    memset(&controls, 0, sizeof(controls));

    control.id = control_id;
    controls.ctrl_class = V4L2_CTRL_ID2CLASS(control_id);
    controls.count = 1;
    controls.controls = &control;

    if (IoctlLocked(VIDIOC_G_EXT_CTRLS, &controls) < 0) {
      HAL_LOGE("G_EXT_CTRLS fails: %s", strerror(errno));
      return -ENODEV;
    }
    *value = control.value;
  } else {
    v4l2_control control{control_id, 0};
    if (IoctlLocked(VIDIOC_G_CTRL, &control) < 0) {
      HAL_LOGE("G_CTRL fails: %s", strerror(errno));
      return -ENODEV;
    }
    *value = control.value;
  }
  return 0;
}

int V4L2Stream::SetTakePictureCtrl(enum v4l2_take_picture value)
{
    struct v4l2_control ctrl;
    int ret = -1;
    //if (mHalCameraInfo.fast_picture_mode){
    if (1){
      HAL_LOGV("####setTakePictureCtrl value = %d",value);
      ctrl.id = V4L2_CID_TAKE_PICTURE;
      ctrl.value = value;
      ret = IoctlLocked(VIDIOC_S_CTRL, &ctrl);
      if (ret < 0)
      {
          HAL_LOGV("####setTakePictureCtrl failed, %s", strerror(errno));
      }
      else
      {
          HAL_LOGV("####setTakePictureCtrl ok");
      }

      return ret;
    }
    return 0;
}

int V4L2Stream::SetFlashMode(uint32_t mode)
{
    int ret = -1;
    struct v4l2_control ctrl;

    if(mode == V4L2_FLASH_LED_MODE_AUTO)
    {
        ctrl.id = V4L2_CID_FLASH_LED_MODE_V1;
        ctrl.value = V4L2_FLASH_MODE_AUTO;
    }
    else
    {
        ctrl.id = V4L2_CID_FLASH_LED_MODE;
        ctrl.value = mode;
    }

    HAL_LOGV("setFlashMode mode = %d id = %d",mode,ctrl.id);

    ret = IoctlLocked(VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
    {
        HAL_LOGV("####setFlashMode failed, %s", strerror(errno));
    }
    else
    {
        HAL_LOGV("####setFlashMode ok, %s",strerror(errno));
    }
    if (mFlashMode != mode) {
        mFlashMode = mode;
        mFlashOk = false;
    }
    return ret;
}

int V4L2Stream::SetAutoFocusInit()
{
    int ret = -1;
    struct v4l2_control ctrl;

    ctrl.id = V4L2_CID_AUTO_FOCUS_INIT;
    ctrl.value = 0;

    ret = IoctlLocked(VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
    {
        HAL_LOGE("####SetAutoFocusInit failed, %s", strerror(errno));
    }
    else
    {
        HAL_LOGV("####SetAutoFocusInit ok, %s",strerror(errno));
    }

    return ret;
}

int V4L2Stream::SetAutoFocusRange(int af_range)
{
    int ret = -1;
    struct v4l2_control ctrl;

    ctrl.id = V4L2_CID_FOCUS_AUTO;
    ctrl.value = 1;

    ret = IoctlLocked(VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
    {
        HAL_LOGE("####SetAutoFocusRange id V4L2_CID_FOCUS_AUTO failed, %s", strerror(errno));
    }
    else
    {
        HAL_LOGV("####SetAutoFocusRange id V4L2_CID_FOCUS_AUTO ok, %s",strerror(errno));
    }

    ctrl.id = V4L2_CID_AUTO_FOCUS_RANGE;
    ctrl.value = af_range;

    ret = IoctlLocked(VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
    {
        HAL_LOGE("####SetAutoFocusRange id V4L2_CID_AUTO_FOCUS_RANGE failed, %s", strerror(errno));
    }
    else
    {
        HAL_LOGV("####SetAutoFocusRange id V4L2_CID_AUTO_FOCUS_RANGE ok, %s",strerror(errno));
    }

    return ret;
}

int V4L2Stream::SetAutoFocusStart()
{
    int ret = -1;
    struct v4l2_control ctrl;

    ctrl.id = V4L2_CID_AUTO_FOCUS_START;

    ret = IoctlLocked(VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
    {
        HAL_LOGE("####SetAutoFocusStart failed, %s", strerror(errno));
    }
    else
    {
        HAL_LOGV("####SetAutoFocusStart ok, %s",strerror(errno));
    }

    return ret;
}

int V4L2Stream::SetAutoFocusStop()
{
    int ret = -1;
    struct v4l2_control ctrl;

    ctrl.id = V4L2_CID_AUTO_FOCUS_STOP;

    ret = IoctlLocked(VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
    {
        HAL_LOGE("####SetAutoFocusStop failed, %s", strerror(errno));
    }
    else
    {
        HAL_LOGV("####SetAutoFocusStop ok, %s",strerror(errno));
    }

    return ret;
}

int V4L2Stream::Set3ALock(int lock)
{
    int ret = -1;
    struct v4l2_control ctrl;

    ctrl.id = V4L2_CID_3A_LOCK;
    ctrl.value = lock;

    ret = IoctlLocked(VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
    {
        HAL_LOGE("####Set3ALock failed, %s", strerror(errno));
    }
    else
    {
        HAL_LOGV("####Set3ALock ok, %s",strerror(errno));
    }

    return ret;
}

int V4L2Stream::GetAutoFocusStatus()
{
    int ret = -1;
    struct v4l2_control ctrl;

    if(device_fd_ < 0)
    {
        return 0xFF000000;
    }
#ifdef USE_ISP
    ret = mAWIspApi->awGetFocusStatus();
    HAL_LOGD("####GetAutoFocusStatus ret:%d device_fd_:%d",ret,device_fd_);
    return ret;
#else
	return 0;
#endif
}

int V4L2Stream::SetAutoFocusRegions(cam_rect_t cam_regions)
{
    int ret = -1;
#ifdef USE_ISP
    ret = mAWIspApi->awSetFocusRegions(cam_regions.x_min, cam_regions.y_min,
            cam_regions.x_max,cam_regions.y_max);
    HAL_LOGV("####SetAutoFocusRegions ret:%d device_fd_:%d,x1:%d y1:%d x2:%d y2:%d",
        ret,device_fd_,cam_regions.x_min,cam_regions.y_min,
        cam_regions.x_max,cam_regions.y_max);

    return ret;
#else
	return 0;
#endif
}
int V4L2Stream::SetFpsRanage(int min_fps)
{
    int ret = -1;
    ret = mAWIspApi->awSetFpsRanage(mIspId, min_fps);
    return ret;
}

int V4L2Stream::SetTestPattern(int test_pattern)
{
    int ret = -1;
    mTestPattern = test_pattern;
    HAL_LOGI("TestPattern:%d", test_pattern);
    return 0;
}
int V4L2Stream::SetCropRect(cam_crop_rect_t cam_crop_rect)
{
    int ret = -1;
    struct v4l2_selection s;
    float ratio_w;
    float ratio_h;

    s.target = V4L2_SEL_TGT_CROP;
    s.r.left   = cam_crop_rect.left;
    s.r.top    = cam_crop_rect.top;
    s.r.width  = cam_crop_rect.width;
    s.r.height = cam_crop_rect.height;
    if (mCameraConfig->supportInterpolationSize()) {
        s.r.left    = s.r.left   * mInterpolation_src_width  / mInterpolation_dst_width;
        s.r.width   = s.r.width  * mInterpolation_src_width  / mInterpolation_dst_width;
        s.r.top     = s.r.top    * mInterpolation_src_height / mInterpolation_dst_height;
        s.r.height  = s.r.height * mInterpolation_src_height / mInterpolation_dst_height;
    }

    jpeg_crop_rect.left     = s.r.left;
    jpeg_crop_rect.top      = s.r.top;
    jpeg_crop_rect.width    = s.r.width;
    jpeg_crop_rect.height   = s.r.height;

    ret = IoctlLocked(VIDIOC_S_SELECTION, &s);
    if (isDebugEnable(DEBUG_ZOOM)) {
        HAL_LOGI("####SetCropRect ret:%d device_fd_:%d jpeg_crop_rect(%d,%d,%d,%d) cam_crop(%d,%d,%d,%d),format:(%d,%d)",
            ret,device_fd_,s.r.left,s.r.top,s.r.width,s.r.height,
            cam_crop_rect.left, cam_crop_rect.top, cam_crop_rect.width, cam_crop_rect.height,
            format_->width(), format_->height());
    }
    if (ret < 0)
    {
        HAL_LOGE("####SetCropRect failed, %s", strerror(errno));
    }
    else
    {
        HAL_LOGV("####SetCropRect ok, %s",strerror(errno));
    }

    return ret;
}

int V4L2Stream::SetControl(uint32_t control_id,
                            int32_t desired,
                            int32_t* result) {
  int32_t result_value = 0;

  // TODO(b/29334616): When async, this may need to check if the stream
  // is on, and if so, lock it off while setting format. Need to look
  // into if V4L2 supports adjusting controls while the stream is on.

  // For extended controls (any control class other than "user"),
  // S_EXT_CTRL must be used instead of S_CTRL.
  if (V4L2_CTRL_ID2CLASS(control_id) != V4L2_CTRL_CLASS_USER) {
    v4l2_ext_control control;
    v4l2_ext_controls controls;
    memset(&control, 0, sizeof(control));
    memset(&controls, 0, sizeof(controls));

    control.id = control_id;
    control.value = desired;
    controls.ctrl_class = V4L2_CTRL_ID2CLASS(control_id);
    controls.count = 1;
    controls.controls = &control;

    if (IoctlLocked(VIDIOC_S_EXT_CTRLS, &controls) < 0) {
      HAL_LOGE("S_EXT_CTRLS fails: %s", strerror(errno));
      return -ENODEV;
    }
    result_value = control.value;
  } else {
    v4l2_control control{control_id, desired};
    if (IoctlLocked(VIDIOC_S_CTRL, &control) < 0) {
      HAL_LOGE("S_CTRL fails: %s", strerror(errno));
      return -ENODEV;
    }
    result_value = control.value;
  }

  // If the caller wants to know the result, pass it back.
  if (result != nullptr) {
    *result = result_value;
  }
  return 0;
}

int V4L2Stream::SetParm(int mCapturemode) {
  HAL_LOG_ENTER();

  struct v4l2_streamparm params;
  memset(&params, 0, sizeof(params));

  params.parm.capture.timeperframe.numerator = 1;
  params.parm.capture.timeperframe.denominator = 30;
  params.parm.capture.reserved[0] = 0;
  params.type = V4L2_CAPTURE_TYPE;
  params.parm.capture.capturemode = mCapturemode;

  if (IoctlLocked(VIDIOC_S_PARM, &params) < 0) {
    HAL_LOGE("S_PARM fails: %s", strerror(errno));
    return -ENODEV;
  }

  return 0;
}

int V4L2Stream::GetFormats(std::set<uint32_t>* v4l2_formats) {
  HAL_LOG_ENTER();

  #if 0
  v4l2_fmtdesc format_query;
  memset(&format_query, 0, sizeof(format_query));

  // TODO(b/30000211): multiplanar support.
  format_query.type = V4L2_CAPTURE_TYPE;
  while (IoctlLocked(VIDIOC_ENUM_FMT, &format_query) >= 0) {
    v4l2_formats->insert(format_query.pixelformat);
    ++format_query.index;
    HAL_LOGV(
      "ENUM_FMT at index %d: ,pixelformat:%d.", format_query.index, format_query.pixelformat);
  }
  if (errno != EINVAL) {
    HAL_LOGE(
        "ENUM_FMT fails at index %d: %s", format_query.index, strerror(errno));
    return -ENODEV;
  }
  #endif

  int format_temp = V4L2_PIX_FMT_NV12;
  v4l2_formats->insert(format_temp);

  format_temp = V4L2_PIX_FMT_YUV420;
  v4l2_formats->insert(format_temp);

  format_temp = V4L2_PIX_FMT_NV21;
  v4l2_formats->insert(format_temp);

  // Add the jpeg format for take picture.
  format_temp = V4L2_PIX_FMT_JPEG;
  v4l2_formats->insert(format_temp);

  return 0;
}

int V4L2Stream::GetFormatFrameSizes(uint32_t v4l2_format,
                                     std::set<std::array<int32_t, 2>, std::greater<std::array<int32_t, 2>>>* sizes) {
  v4l2_frmsizeenum size_query;
  memset(&size_query, 0, sizeof(size_query));

  // Add the jpeg format for take picture.
  if(v4l2_format == V4L2_PIX_FMT_JPEG) {
    v4l2_format = V4L2_PIX_FMT_DEFAULT;
  }

  size_query.pixel_format = v4l2_format;

#if 0
  if (IoctlLocked(VIDIOC_ENUM_FRAMESIZES, &size_query) < 0) {
    HAL_LOGE("ENUM_FRAMESIZES failed: %s", strerror(errno));
    return -ENODEV;
  }
  if (size_query.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
    // Discrete: enumerate all sizes using VIDIOC_ENUM_FRAMESIZES.
    // Assuming that a driver with discrete frame sizes has a reasonable number
    // of them.
    do {
      sizes->insert({{{static_cast<int32_t>(size_query.discrete.width),
                       static_cast<int32_t>(size_query.discrete.height)}}});
      ++size_query.index;
      HAL_LOGV("size_query.discrete.width x size_query.discrete.height %u x %u for size_query.index:%d",
         size_query.discrete.width,size_query.discrete.height,
         size_query.index);
    } while (IoctlLocked(VIDIOC_ENUM_FRAMESIZES, &size_query) >= 0);
    if (errno != EINVAL) {
      HAL_LOGW("ENUM_FRAMESIZES fails at index %d: %s,maybe we encount the ends of framesizes.",
               size_query.index,
               strerror(errno));
      //return -ENODEV;
    }
  } else {
    HAL_LOGV("size_query.stepwise.min_width x size_query.stepwise.min_height %u x %u.",
     size_query.stepwise.min_width,size_query.stepwise.min_height);
    HAL_LOGV("size_query.stepwise.max_width x size_query.stepwise.max_height %u x %u.",
     size_query.stepwise.max_width,size_query.stepwise.max_height);
    // Continuous/Step-wise: based on the stepwise struct returned by the query.
    // Fully listing all possible sizes, with large enough range/small enough
    // step size, may produce far too many potential sizes. Instead, find the
    // closest to a set of standard sizes.
    for (const auto size : kStandardSizes) {
      // Find the closest size, rounding up.
      uint32_t desired_width = size[0];
      uint32_t desired_height = size[1];
      if (desired_width < size_query.stepwise.min_width ||
          desired_height < size_query.stepwise.min_height) {
        HAL_LOGV("Standard size %u x %u is too small than %u x %u for format %d",
                 desired_width,desired_height,
                 size_query.stepwise.min_width,size_query.stepwise.min_height,
                 v4l2_format);
        continue;
      } else if (desired_width > size_query.stepwise.max_width &&
                 desired_height > size_query.stepwise.max_height) {
        HAL_LOGV("Standard size %u x %u is too big for format %d",
                 desired_width,desired_height,
                 size_query.stepwise.max_width,size_query.stepwise.max_height,
                 v4l2_format);
        continue;
      }

      // Round up.
      uint32_t width_steps = (desired_width - size_query.stepwise.min_width +
                              size_query.stepwise.step_width - 1) /
                             size_query.stepwise.step_width;
      uint32_t height_steps = (desired_height - size_query.stepwise.min_height +
                               size_query.stepwise.step_height - 1) /
                              size_query.stepwise.step_height;
      sizes->insert(
          {{{static_cast<int32_t>(size_query.stepwise.min_width +
                                  width_steps * size_query.stepwise.step_width),
             static_cast<int32_t>(size_query.stepwise.min_height +
                                  height_steps *
                                      size_query.stepwise.step_height)}}});
    }
  }
#endif

  char * value;
  value = mCameraConfig->supportPictureSizeValue();

  std::string st1 = value;
  int size_width = 0;
  int size_height = 0;
  std::string tmp;
  std::vector<std::string> data;
  std::stringstream input(st1);

  while(getline(input, tmp, ','))
  {
      data.push_back(tmp);
  }
  for(auto s : data)
  {
      sscanf(s.c_str(), "%dx%d", &size_width,&size_height);
      sizes->insert({{{size_width,size_height}}});
  }


  return 0;
}

// Converts a v4l2_fract with units of seconds to an int64_t with units of ns.
inline int64_t FractToNs(const v4l2_fract& fract) {
  return (1000000000LL * fract.numerator) / fract.denominator;
}

int V4L2Stream::GetFormatFrameDurationRange(
    uint32_t v4l2_format,
    const std::array<int32_t, 2>& size,
    std::array<int64_t, 2>* duration_range) {
  // Potentially called so many times logging entry is a bad idea.

  v4l2_frmivalenum duration_query;
  memset(&duration_query, 0, sizeof(duration_query));

  // Add the jpeg format for take picture.
  if(v4l2_format == V4L2_PIX_FMT_JPEG) {
    v4l2_format = V4L2_PIX_FMT_DEFAULT;
  }

  duration_query.pixel_format = v4l2_format;
  duration_query.width = size[0];
  duration_query.height = size[1];
  //TODOzjw: fix the driver interface VIDIOC_ENUM_FRAMEINTERVALS
  if (IoctlLocked(VIDIOC_ENUM_FRAMEINTERVALS, &duration_query) < 0) {
    HAL_LOGW("ENUM_FRAMEINTERVALS failed: %s", strerror(errno));
    //return -ENODEV;
  }

  int64_t min = std::numeric_limits<int64_t>::max();
  int64_t max = std::numeric_limits<int64_t>::min();
  #if 0
  if (duration_query.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
    // Discrete: enumerate all durations using VIDIOC_ENUM_FRAMEINTERVALS.
    do {
      min = std::min(min, FractToNs(duration_query.discrete));
      max = std::max(max, FractToNs(duration_query.discrete));
      ++duration_query.index;
    } while (IoctlLocked(VIDIOC_ENUM_FRAMEINTERVALS, &duration_query) >= 0);
    if (errno != EINVAL) {
      HAL_LOGE("ENUM_FRAMEINTERVALS fails at index %d: %s",
               duration_query.index,
               strerror(errno));
      return -ENODEV;
    }
  } else {
    // Continuous/Step-wise: simply convert the given min and max.
    min = FractToNs(duration_query.stepwise.min);
    max = FractToNs(duration_query.stepwise.max);
  }
  #endif
  min = 33300000;
  max = 100000000;
  (*duration_range)[0] = min;
  (*duration_range)[1] = max;
  return 0;
}

int V4L2Stream::parse_pair(const char *str, int *first, int *second, char delim)
{
    // Find the first integer.
    char *end;
    int w = (int)strtol(str, &end, 10);
    // If a delimeter does not immediately follow, give up.
    if (*end != delim) {
        HAL_LOGE("Cannot find delimeter (%c) in str=%s", delim, str);
        return -1;
    }

    // Find the second integer, immediately after the delimeter.
    int h = (int)strtol(end+1, &end, 10);

    *first = w;
    *second = h;

    return 0;
}

int V4L2Stream::SetFormat(const StreamFormat& desired_format,
                           uint32_t* result_max_buffers) {
  HAL_LOG_ENTER();

  if (format_ && desired_format == *format_) {
    HAL_LOGV("The desired format is as same as the format set last.");
    //*result_max_buffers = buffers_.size();
    //return 0;
  }

  // Not in the correct format, set the new one.

  if (format_) {
    // If we had an old format, first request 0 buffers to inform the device
    // we're no longer using any previously "allocated" buffers from the old
    // format. This seems like it shouldn't be necessary for USERPTR memory,
    // and/or should happen from turning the stream off, but the driver
    // complained. May be a driver issue, or may be intended behavior.
    int res = RequestBuffers(0);
    if (res) {
      return res;
    }
  }

  // Set the camera to the new format.
  v4l2_format new_format;
  desired_format.FillFormatRequest(&new_format);
  int setFormatFlag = 0;
  int cur_width = 0;
  int cur_height = 0;
  if(mCameraConfig->supportInterpolationSize())
  {
      if(new_format.fmt.pix_mp.width > mStream_max_width &&
          new_format.fmt.pix_mp.height > mStream_max_height)
      {
          cur_width = new_format.fmt.pix_mp.width;
          cur_height = new_format.fmt.pix_mp.height;
          new_format.fmt.pix_mp.width = mInterpolation_src_width;
          new_format.fmt.pix_mp.height = mInterpolation_src_height;
          setFormatFlag = 1;
      }
  }
  // TODO(b/29334616): When async, this will need to check if the stream
  // is on, and if so, lock it off while setting format.
  if (IoctlLocked(VIDIOC_S_FMT, &new_format) < 0) {
    HAL_LOGE("S_FMT failed: %s", strerror(errno));
    return -ENODEV;
  }

  if (IoctlLocked(VIDIOC_G_FMT, &new_format) < 0) {
    HAL_LOGE("G_FMT failed: %s", strerror(errno));
    return -ENODEV;
  }

  // Check that the driver actually set to the requested values.
  if (desired_format != new_format) {
    HAL_LOGE("Device doesn't support desired stream configuration.");
    //return -EINVAL;
  }
  if (isDebugEnable(DEBUG_INSERT)) {
      HAL_LOGI("format:(%dx%d) newformat(%dx%d)FormatFlag:%d",
             cur_width, cur_height, new_format.fmt.pix_mp.width, new_format.fmt.pix_mp.height,
             setFormatFlag);
  }
  if(setFormatFlag && cur_width != 0 && cur_height != 0){
      new_format.fmt.pix_mp.width = cur_width;
      new_format.fmt.pix_mp.height = cur_height;
  }
  // Keep track of our new format.
  format_.reset(new StreamFormat(new_format));
  // Format changed, request new buffers.
  int res = RequestBuffers(*result_max_buffers);
  if (res) {
    HAL_LOGE("Requesting buffers for new format failed.");
    return res;
  }
  *result_max_buffers = buffers_.size();
  HAL_LOGD("*result_max_buffers:%d.",*result_max_buffers);
  return 0;
}

int V4L2Stream::RequestBuffers(uint32_t num_requested) {
  v4l2_requestbuffers req_buffers;
  memset(&req_buffers, 0, sizeof(req_buffers));
  req_buffers.type = format_->type();
  req_buffers.memory = format_->memory();
  req_buffers.count = num_requested;

  int res = IoctlLocked(VIDIOC_REQBUFS, &req_buffers);
  // Calling REQBUFS releases all queued buffers back to the user.
  //int gralloc_res = gralloc_->unlockAllBuffers();
  if (res < 0) {
    HAL_LOGE("REQBUFS failed: %s", strerror(errno));
    return -ENODEV;
  }


  // V4L2 will set req_buffers.count to a number of buffers it can handle.
  if (num_requested > 0 && req_buffers.count < 1) {
    HAL_LOGE("REQBUFS claims it can't handle any buffers.");
    return -ENODEV;
  }

  {
    std::lock_guard<std::mutex> guard(cmd_queue_lock_);
    buffer_cnt_inflight_ = 0;
  }

  // refresh buffers_num_ queue.
  while(!buffers_num_.empty()) {
    buffers_num_.pop();
  }

  if(buffers_num_.empty()) {
    for (int i = 0; i < req_buffers.count; ++i) {
      buffers_num_.push(i);
      HAL_LOGD("buffers_num_ push:%d, size:%d.", i, buffers_num_.size());
    }
  }

  buffers_.resize(req_buffers.count, false);

  HAL_LOGD("num_requested:%d,req_buffers.count:%d.",num_requested,req_buffers.count);

  return 0;
}
int V4L2Stream::queueBuffer(v4l2_buffer* pdevice_buffer) {
  int res;
  std::lock_guard<std::mutex> guard(cmd_queue_lock_);
  res = IoctlLocked(VIDIOC_QBUF, pdevice_buffer);
  if(res >= 0) {
    buffer_cnt_inflight_++;
    HAL_LOGD("After queue buffer csi driver has %d buffer(s) now.",buffer_cnt_inflight_);
  }
  return res;
}
int V4L2Stream::dequeueBuffer(v4l2_buffer* pdevice_buffer) {
  int res;
  std::lock_guard<std::mutex> guard(cmd_queue_lock_);

  res = IoctlLocked(VIDIOC_DQBUF, pdevice_buffer);
  if(res >= 0) {
    buffer_cnt_inflight_--;
    if (isDebugEnable(DEBUG_BUFF))
        HAL_LOGI("After dequeue buffer csi driver has %d buffer(s) now.",buffer_cnt_inflight_);
  }
  return res;
}

int V4L2Stream::PrepareBuffer() {
  if (!format_) {
    HAL_LOGE("Stream format must be set before enqueuing buffers.");
    return -ENODEV;
  }

  int ret = 0;
  for (int i = buffers_.size() - 1; i>=0; i--)
  {
    struct v4l2_buffer device_buffer;
    std::lock_guard<std::mutex> guard(buffer_queue_lock_);
    // Set up a v4l2 buffer struct.
    memset(&device_buffer, 0, sizeof(device_buffer));
    device_buffer.type = format_->type();
    device_buffer.index = i;
    device_buffer.memory = format_->memory();
    device_buffer.length = format_->nplanes();
    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    //TODOzjw:support mutiplanar.
    memset(planes, 0, VIDEO_MAX_PLANES*sizeof(struct v4l2_plane));
    if(V4L2_CAPTURE_TYPE == device_buffer.type) {
      device_buffer.m.planes =planes;
      if (NULL == device_buffer.m.planes) {
          HAL_LOGE("device_buffer.m.planes calloc failed!\n");
      }
    }

    // Use QUERYBUF to ensure our buffer/device is in good shape,
    // and fill out remaining fields.
    if (IoctlLocked(VIDIOC_QUERYBUF, &device_buffer) < 0) {
      HAL_LOGE("QUERYBUF fails: %s", strerror(errno));
      return -ENODEV;
    }
    switch (format_->memory()) {
        case V4L2_MEMORY_USERPTR:
            HAL_LOGI("Use V4L2_MEMORY_USERPTR");
            stream_buffers.length[i] = device_buffer.m.planes[0].length;
            stream_buffers.start[i] = (void *)aw_ion_alloc(stream_buffers.length[i]);
            break;
        case V4L2_MEMORY_DMABUF:
            HAL_LOGI("Use V4L2_MEMORY_DMABUF");
            stream_buffers.length[i] = device_buffer.m.planes[0].length;
            stream_buffers.start[i] = (void *)aw_ion_alloc(stream_buffers.length[i]);
            stream_buffers.fd[i] = ion_vir2fd(stream_buffers.start[i]);
            break;
        default:
            HAL_LOGE("not support memory type: %d", format_->memory());
            break;
    }
    if (!i) {
        mPatternBlock.length = device_buffer.m.planes[0].length;
        mPatternBlock.start  = (void *)aw_ion_alloc(mPatternBlock.length);
        memset(mPatternBlock.start, 0x00,  mPatternBlock.length*2/3);
        memset((char *)mPatternBlock.start + mPatternBlock.length*2/3, 0x80,  mPatternBlock.length/3);
    }

  }
  for (int i = 0; i < buffers_.size(); i++) {
     struct v4l2_buffer device_buffer;
    std::lock_guard<std::mutex> guard(buffer_queue_lock_);
    int index = buffers_num_.front();
    buffers_num_.pop();
    // Set up a v4l2 buffer struct.
    memset(&device_buffer, 0, sizeof(device_buffer));
    device_buffer.type = format_->type();
    device_buffer.index = index;
    device_buffer.memory = format_->memory();
    device_buffer.length = format_->nplanes();
    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    //TODOzjw:support mutiplanar.
    memset(planes, 0, VIDEO_MAX_PLANES*sizeof(struct v4l2_plane));
    if(V4L2_CAPTURE_TYPE == device_buffer.type) {
      device_buffer.m.planes =planes;
      if (NULL == device_buffer.m.planes) {
          HAL_LOGE("device_buffer.m.planes calloc failed!\n");
      }
    }
    switch (format_->memory()) {
        case V4L2_MEMORY_USERPTR:
            device_buffer.m.planes[0].m.userptr = (unsigned long)stream_buffers.start[index];
            device_buffer.m.planes[0].length = stream_buffers.length[index];
            break;
        case V4L2_MEMORY_DMABUF:
            device_buffer.m.planes[0].m.userptr = (unsigned long)stream_buffers.start[index];
            device_buffer.m.planes[0].length = stream_buffers.length[index];
            device_buffer.m.planes[0].m.fd   = stream_buffers.fd[index];
            break;
        defalut:
            HAL_LOGE("not support memory type: %d", format_->memory());
            break;
    }
    if (isDebugEnable(DEBUG_BUFF))
         HAL_LOGI("userptr:%p lenght:%d", device_buffer.m.planes[0].m.userptr, device_buffer.m.planes[0].length);
    if (queueBuffer(&device_buffer) < 0) {
      HAL_LOGE("QBUF fails: %s", strerror(errno));
      return -ENODEV;
    }

  }
  HAL_LOGD("Buffers had been prepared!");
  return 0;

}

int V4L2Stream::EnqueueBuffer() {
  std::unique_lock<std::mutex> connect_lock(connection_lock_);
  if (!format_) {
    HAL_LOGE("Stream format must be set before enqueuing buffers.");
    return -ENODEV;
  }

  // Find a free buffer index. Could use some sort of persistent hinting
  // here to improve expected efficiency, but buffers_.size() is expected
  // to be low enough (<10 experimentally) that it's not worth it.
  int index = -1;
  {
    std::unique_lock<std::mutex> lock(buffer_queue_lock_);
    while(buffers_num_.empty()) {
      HAL_LOGD("buffers_num_ is empty now, wait for the queue to be filled.");
      if(mflush_buffers) {
        mflush_buffers = false;
        return 0;
      }
      buffer_availabl_queue_.wait(lock);
      if(mflush_buffers) {
        mflush_buffers = false;
        return 0;
      }
    }
    index = buffers_num_.front();
    buffers_num_.pop();
    if(isDebugEnable(DEBUG_BUFF)) {
        HAL_LOGI("buffers_num_ pop:%d, size:%d. id:%d ss:%d", index, buffers_num_.size(),device_id_, device_ss_);
    }
  }
  if (index < 0) {
    // Note: The HAL should be tracking the number of buffers in flight
    // for each stream, and should never overflow the device.
    HAL_LOGE("Cannot enqueue buffer: stream is already full.");
    return -ENODEV;
  }

  // Set up a v4l2 buffer struct.
  v4l2_buffer device_buffer;
  memset(&device_buffer, 0, sizeof(device_buffer));
  device_buffer.type = format_->type();
  device_buffer.index = index;
  device_buffer.memory = format_->memory();
  device_buffer.length = format_->nplanes();
  struct v4l2_plane planes[VIDEO_MAX_PLANES];
  memset(planes, 0, VIDEO_MAX_PLANES*sizeof(struct v4l2_plane));
  if(V4L2_CAPTURE_TYPE == device_buffer.type) {
    device_buffer.m.planes = planes;
    if (NULL == device_buffer.m.planes) {
        HAL_LOGE("device_buffer.m.planes calloc failed!\n");
    }
  }

  switch (device_buffer.memory) {
      case V4L2_MEMORY_USERPTR:
        device_buffer.m.planes[0].m.userptr = (unsigned long)stream_buffers.start[index];
        device_buffer.m.planes[0].length = stream_buffers.length[index];
        break;
      case V4L2_MEMORY_DMABUF:
        device_buffer.m.planes[0].m.userptr = (unsigned long)stream_buffers.start[index];
        device_buffer.m.planes[0].length    = stream_buffers.length[index];
        device_buffer.m.planes[0].m.fd      = stream_buffers.fd[index];
        break;
      default:
        HAL_LOGE("not support memory type: %d", format_->memory());
        break;
  }
  if (queueBuffer(&device_buffer) < 0) {
    HAL_LOGD("QBUF fails: %s id:%d device_ss:%d", strerror(errno), device_id_, device_ss_);
    return -ENODEV;
  }

  // Mark the buffer as in flight.
  std::lock_guard<std::mutex> guard(buffer_queue_lock_);
  buffers_[index] = true;

  return 0;
}

int V4L2Stream::DequeueBuffer(void ** src_addr_,struct timeval * ts) {
  std::unique_lock<std::mutex> disconnect_lock(disconnection_lock_);
  if (!format_) {
    HAL_LOGE(
        "Format not set, so stream can't be on, "
        "so no buffers available for dequeueing");
    return -EAGAIN;
  }
  static int drop_number = 0;
  int64_t beginTime = systemTime() / 1000000;
  v4l2_buffer buffer;
  memset(&buffer, 0, sizeof(buffer));
  buffer.type = format_->type();
  buffer.memory = format_->memory();
  buffer.length = format_->nplanes();
  struct v4l2_plane planes[VIDEO_MAX_PLANES];
  memset(planes, 0, VIDEO_MAX_PLANES*sizeof(struct v4l2_plane));
  if(V4L2_CAPTURE_TYPE == buffer.type) {
    buffer.m.planes = planes;
    if (NULL == buffer.m.planes) {
        HAL_LOGE("device_buffer.m.planes calloc failed!\n");
    }
  }

  int res = dequeueBuffer(&buffer);
  if (res) {
    if (errno == EAGAIN) {
      // Expected failure.
      return -EAGAIN;
    } else {
      // Unexpected failure.
      HAL_LOGE("DQBUF fails: %s", strerror(errno));
      return -ENODEV;
    }
  }

  *ts =  buffer.timestamp;
  if (mTestPattern) {
    *src_addr_ = mPatternBlock.start;
  } else {
    switch (format_->memory()) {
        case V4L2_MEMORY_USERPTR:
            *src_addr_ = stream_buffers.start[buffer.index];
            break;
        case V4L2_MEMORY_DMABUF:
            *src_addr_ = stream_buffers.start[buffer.index];
            break;
        default:
            HAL_LOGE("not support memory type: %d", format_->memory());
            break;
    }
  }
  int copy_size = format_->width()*format_->height()*3/2;

  static int capture_count = 0;
  char path[128] = {0};
  if (isDebugEnable(DEBUG_RAW_PIC)){
        if (capture_count == 0) {
            sprintf(path, "/data/vendor/camera/pic_%dx%d_%d.yuv",
                   format_->width(),
                   format_->height(),
                   device_ss_);
            HAL_LOGI("save picture:%d*%d path:%s", format_->width(), format_->height(), path);
            saveBuffers(path,*src_addr_,copy_size,true);
            capture_count += 1;
        }
  } else if (isDebugEnable(DEBUG_RAW_VIDEO)){
       sprintf(path, "/data/vendor/camera/video_%dx%d_%d.yuv",
              format_->width(),
              format_->height(),
              device_ss_);
       saveBuffers(path,*src_addr_,copy_size,false);
  }
  // Mark the buffer as no longer in flight.
  {
    std::lock_guard<std::mutex> guard(buffer_queue_lock_);
    buffers_[buffer.index] = false;
    //*index = buffers_num_.front();
    buffers_num_.push(buffer.index);
    if (isDebugEnable(DEBUG_BUFF)) {
        HAL_LOGI("buffers_num_ push:%d, size:%d. id:%d ss:%d", buffer.index, buffers_num_.size(), device_id_, device_ss_);
    }
    buffer_availabl_queue_.notify_one();
    HAL_LOGV("buffer.index:%d has been freed by csi driver, and buffer_availabl_queue_ was notified!\n",buffer.index);
  }

  if (mFlashMode != V4L2_FLASH_LED_MODE_NONE && drop_number <= 5 && !mFlashOk) {
        drop_number += 1;
        *src_addr_ = nullptr;
        if (drop_number == 5) {
            mFlashOk = true;
        }
        return -EAGAIN;
  } else {
    drop_number = 0;
  }
  int64_t endTime = systemTime() / 1000000;
  if (isDebugEnable(DEBUG_PERF)) {
    HAL_LOGI("device_ss_:%d cost:%lldms timestamp:%lld",
        device_ss_, endTime - beginTime, ts->tv_sec * 1000000000ULL + ts->tv_usec*1000 );
  }
  return 0;
}

int V4L2Stream::CopyBuffer(void * dst_addr, void * src_addr, uint32_t mUsage) {
  std::lock_guard<std::mutex> guard(cmd_queue_lock_);
  int64_t beginTime = systemTime() / 1000000;
  if (!format_) {
    HAL_LOGE("Stream format must be set before enqueuing buffers.");
    return -ENODEV;
  }
  VencRect sCropInfo;
  VencIspBufferInfo pInBuffer,pOutBuffer;
  memset(&pInBuffer, 0, sizeof(pInBuffer));
  memset(&pOutBuffer, 0, sizeof(pOutBuffer));
  int mCopySize = ALIGN_16B(format_->width())*ALIGN_16B(format_->height())*3/2;
  HAL_LOGD("dst_addr:%p,src_addr:%p,mCopySize:%d.", dst_addr, src_addr, mCopySize);
  //memcpy(dst_addr, src_addr, mCopySize);
  int width = format_->width();
  int height = format_->height();
  int isAlign = IS_USAGE_VIDEO(mUsage)? true:false;
  static int capture_count = 0;
  char path[128] = {0};
  static int copySize = 0;
  bool isCrop = false;
  if (width >= mStream_max_width || height >= mStream_max_height) {
        if (jpeg_crop_rect.width < mStream_max_width || jpeg_crop_rect.height < mStream_max_height) {
            sCropInfo.nWidth   = jpeg_crop_rect.width;
            sCropInfo.nHeight  = jpeg_crop_rect.height;
            sCropInfo.nLeft    = jpeg_crop_rect.left;
            sCropInfo.nTop     = jpeg_crop_rect.top;
            isCrop             = true;
        } else {
            sCropInfo.nLeft    = 0;
            sCropInfo.nTop     = 0;
            sCropInfo.nWidth   = mStream_max_width;
            sCropInfo.nHeight  = mStream_max_height;
            if (width > mStream_max_width || height > mStream_max_height) {
                isCrop = true;
            } else {
                isCrop = false;
            }
        }
        pInBuffer.nWidth         =  mStream_max_width;
        pInBuffer.nHeight         = mStream_max_height;
        pInBuffer.colorFormat     = VENC_PIXEL_YUV420SP;
        pInBuffer.pAddrVirY     = (unsigned char*)src_addr;

        pOutBuffer.nWidth         = width;
        pOutBuffer.nHeight         = height;
        pOutBuffer.pAddrVirY     = (unsigned char*)dst_addr;
        if (isCrop) {
            AWCropYuv(&crop_data, &pInBuffer, &sCropInfo , &pOutBuffer, isAlign);
            goto COPY_END;
        }
  }
  if (isAlign) {
    memcpy(dst_addr, src_addr, width*height);
    memcpy((char *)dst_addr + ALIGN_16B(width)*ALIGN_16B(height),
                (char *)src_addr + width*height,
                (width)*(height)/2);
    copySize = mCopySize;
  } else {
    memcpy(dst_addr, src_addr, width*height*3/2);
    copySize = width*height*3/2;
  }
  if (isDebugEnable(DEBUG_RAW_PIC)){
        if (capture_count == 0) {
            sprintf(path, "/data/vendor/camera/copy_%dx%d_%d_%d_%d.yuv",
                   format_->width(),
                   format_->height(),
                   device_ss_,isAlign, copySize);
            HAL_LOGI("save picture:%d*%d path:%s", format_->width(), format_->height(), path);
            saveBuffers(path,dst_addr,copySize,true);
            capture_count += 1;
        }
  }
  COPY_END:
  if (isDebugEnable(DEBUG_PERF)) {
     int64_t endTime = systemTime() / 1000000;
     HAL_LOGI("device_ss_:%d cost:%lldms format_size:%dx%d crop_size:%ldx%ld max_size:%dx%d mUsage:%x",
               device_ss_, endTime - beginTime, format_->width(), format_->height(),
               sCropInfo.nWidth, sCropInfo.nHeight,  mStream_max_width, mStream_max_height, mUsage);
  }

  return 0;
}

int V4L2Stream::EncodeBuffer(void * dst_addr, void * src_addr, unsigned long mJpegBufferSizes, JPEG_ENC_t jpeg_enc){
  isTakePicure = true;

  unsigned long jpeg_buf = (unsigned long)dst_addr;
  int bufSize = 0;

  int64_t beginTime = systemTime() / 1000000;
  // Get buffer size.
  HAL_LOGD("jpeg info:lock_buffer vaddr:%p, buffer size:%d.", jpeg_buf, mJpegBufferSizes);

#if 1
  if(format_->width() > mStream_max_width || format_->height() > mStream_max_height){
         jpeg_enc.src_w = mStream_max_width;
         jpeg_enc.src_h = mStream_max_height;
  }
  if(jpeg_enc.src_w == 0) {
    jpeg_enc.src_w            = format_->width();
  }
  if(jpeg_enc.src_h == 0) {
    jpeg_enc.src_h            = format_->height();
  }
  //sunxilong@aw add: fix CTS StillCaptureTest#testJpegExif 2020/9/14 begin
  if(jpeg_enc.rotate == 270 || jpeg_enc.rotate == 90) {
    std::swap(jpeg_enc.pic_w, jpeg_enc.pic_h);
  }
  //sunxilong@aw add: fix CTS StillCaptureTest#testJpegExif 2020/9/14 end
  jpeg_enc.colorFormat    = JPEG_COLOR_YUV420_NV21;
  //jpeg_enc.quality        = 90;
  //jpeg_enc.rotate            = 0;

  char                              mDateTime[64];
  time_t t;
  struct tm *tm_t;
  time(&t);
  tm_t = localtime(&t);
  sprintf(mDateTime, "%4d:%02d:%02d %02d:%02d:%02d",
      tm_t->tm_year+1900, tm_t->tm_mon+1, tm_t->tm_mday,
      tm_t->tm_hour, tm_t->tm_min, tm_t->tm_sec);

  char property[PROPERTY_VALUE_MAX];
  if (property_get("ro.product.manufacturer", property, "") > 0)
  {
      strcpy(jpeg_enc.CameraMake, property);
  }
  if (property_get("ro.product.model", property, "") > 0)
  {
      strcpy(jpeg_enc.CameraModel, property);
  }

  strcpy(jpeg_enc.DateTime, mDateTime);
  HAL_LOGD("jpeg info:%s.", mDateTime);

  //jpeg_enc.thumbWidth        = 320;
  //jpeg_enc.thumbHeight    = 240;
  jpeg_enc.whitebalance   = 0;
  jpeg_enc.focal_length    = 3.04;

#if 0
  if ((src_crop.width != jpeg_enc.src_w)
      || (src_crop.height != jpeg_enc.src_h))
  {
      jpeg_enc.enable_crop        = 1;
      jpeg_enc.crop_x                = src_crop.left;
      jpeg_enc.crop_y                = src_crop.top;
      jpeg_enc.crop_w                = src_crop.width;
      jpeg_enc.crop_h                = src_crop.height;
  }
  else
  {
      jpeg_enc.enable_crop        = 0;
  }
#endif
  HAL_LOGD("src: %dx%d, pic: %dx%d, quality: %d, rotate: %d, Gps method: %s,\
      thumbW: %d, thumbH: %d, thubmFactor: %d, crop: [%d, %d, %d, %d]",
      jpeg_enc.src_w, jpeg_enc.src_h,
      jpeg_enc.pic_w, jpeg_enc.pic_h,
      jpeg_enc.quality, jpeg_enc.rotate,
      jpeg_enc.gps_processing_method,
      jpeg_enc.thumbWidth,
      jpeg_enc.thumbHeight,
      jpeg_enc.scale_factor,
      jpeg_enc.crop_x,
      jpeg_enc.crop_y,
      jpeg_enc.crop_w,
      jpeg_enc.crop_h);


  JpegEncInfo sjpegInfo;
  EXIFInfo   exifInfo;

  memset(&sjpegInfo, 0, sizeof(JpegEncInfo));
  memset(&exifInfo, 0, sizeof(EXIFInfo));

  sjpegInfo.sBaseInfo.nStride = jpeg_enc.src_w;
  sjpegInfo.sBaseInfo.nInputWidth = jpeg_enc.src_w;
  sjpegInfo.sBaseInfo.nInputHeight = jpeg_enc.src_h;
  sjpegInfo.sBaseInfo.nDstWidth = jpeg_enc.pic_w;
  sjpegInfo.sBaseInfo.nDstHeight = jpeg_enc.pic_h;
  //sjpegInfo.sBaseInfo.memops = (ScMemOpsS*)memOpsS;
  //sjpegInfo.pAddrPhyY = (unsigned char*)src_addr_phy;
  //sjpegInfo.pAddrPhyC = (unsigned char*)src_addr_phy + ALIGN_16B(src_width) * src_height;
  sjpegInfo.sBaseInfo.eInputFormat = VENC_PIXEL_YVU420SP;
  if(mCameraConfig->supportInterpolationSize())
  {
      jpeg_enc.quality = 100;
      sjpegInfo.quality = 100;
  }
  else
  {
      sjpegInfo.quality        = jpeg_enc.quality;
  }

  exifInfo.Orientation    = jpeg_enc.rotate;

  if(jpeg_enc.crop_h != 0) {
    sjpegInfo.nShareBufFd = jpeg_enc.crop_h;
    jpeg_enc.crop_h = 0;
    sjpegInfo.bNoUseAddrPhy = 0;
  } else {
    sjpegInfo.nShareBufFd = jpeg_enc.crop_h;
    jpeg_enc.crop_h = 0;
    sjpegInfo.bNoUseAddrPhy = 1;
  }
  if (isDebugEnable(DEBUG_RAW_PIC)) {
     HAL_LOGI("jpeg:(%d,%d,%d,%d) format:(%d,%d),mStream:(%d,%d)",
         jpeg_crop_rect.left,jpeg_crop_rect.top,
         jpeg_crop_rect.width,jpeg_crop_rect.height,
         format_->width(), format_->height(), mStream_max_width, mStream_max_height);
  }
  if(format_->width() < mStream_max_width && format_->height() < mStream_max_height)
  {
      sjpegInfo.bEnableCorp = 0;
  }
  else
  {
      sjpegInfo.bEnableCorp = !((format_->width() == mStream_max_width && format_->height() == mStream_max_height)
                                && (jpeg_crop_rect.left == 0 && jpeg_crop_rect.top == 0));
      if (sjpegInfo.bEnableCorp) {
          sjpegInfo.sCropInfo.nLeft = jpeg_crop_rect.left;
          sjpegInfo.sCropInfo.nTop = jpeg_crop_rect.top;
          sjpegInfo.sCropInfo.nWidth = jpeg_crop_rect.width;
          sjpegInfo.sCropInfo.nHeight = jpeg_crop_rect.height;
      }
  }
  sjpegInfo.pAddrPhyY = (unsigned char *)src_addr;
  sjpegInfo.pAddrPhyC = (unsigned char *)((unsigned long)src_addr + jpeg_enc.src_w *jpeg_enc.src_h);
  sjpegInfo.pAddrVirY = (unsigned char *)src_addr;
  sjpegInfo.pAddrVirC = (unsigned char *)((unsigned long)src_addr + jpeg_enc.src_w *jpeg_enc.src_h);


  exifInfo.ThumbWidth = jpeg_enc.thumbWidth;
  exifInfo.ThumbHeight = jpeg_enc.thumbHeight;
  if (isDebugEnable(DEBUG_RAW_PIC)){
     HAL_LOGI("src: %dx%dx%d, pic: %dx%d, quality: %d, rotate: %d,\
         thumbW: %d, thumbH: %d,EnableCorp: %d,crop: [%d, %d, %d, %d],share_fd:%d",
         sjpegInfo.sBaseInfo.nInputWidth, sjpegInfo.sBaseInfo.nInputHeight, sjpegInfo.sBaseInfo.nStride,
         sjpegInfo.sBaseInfo.nDstWidth, sjpegInfo.sBaseInfo.nDstHeight,
         sjpegInfo.quality, exifInfo.Orientation,
         exifInfo.ThumbWidth,
         exifInfo.ThumbHeight,
         sjpegInfo.bEnableCorp,
         sjpegInfo.sCropInfo.nLeft,
         sjpegInfo.sCropInfo.nTop,
         sjpegInfo.sCropInfo.nWidth,
         sjpegInfo.sCropInfo.nHeight,sjpegInfo.nShareBufFd);
  }
  strcpy((char*)exifInfo.CameraMake,    jpeg_enc.CameraMake);
  strcpy((char*)exifInfo.CameraModel,    jpeg_enc.CameraModel);
  strcpy((char*)exifInfo.DateTime, jpeg_enc.DateTime);

  struct timeval tv;
  int res = gettimeofday(&tv, NULL);
  char       subSecTime1[8];
  char       subSecTime2[8];
  char       subSecTime3[8];
  sprintf(subSecTime1, "%06ld", tv.tv_usec);
  sprintf(subSecTime2, "%06ld", tv.tv_usec);
  sprintf(subSecTime3, "%06ld", tv.tv_usec);
  strcpy((char*)exifInfo.subSecTime,     subSecTime1);
  strcpy((char*)exifInfo.subSecTimeOrig, subSecTime2);
  strcpy((char*)exifInfo.subSecTimeDig,  subSecTime3);

  if (0 != strlen(jpeg_enc.gps_processing_method)){
      strcpy((char*)exifInfo.gpsProcessingMethod,jpeg_enc.gps_processing_method);
      exifInfo.enableGpsInfo = 1;
      exifInfo.gps_latitude = jpeg_enc.gps_latitude;
      exifInfo.gps_longitude = jpeg_enc.gps_longitude;
      exifInfo.gps_altitude = jpeg_enc.gps_altitude;
      exifInfo.gps_timestamp = jpeg_enc.gps_timestamp;
  }
  else
      exifInfo.enableGpsInfo = 0;

  // TODO: fix parameter for sensor
  exifInfo.ExposureTime.num = 25;
  exifInfo.ExposureTime.den = 100;

  exifInfo.FNumber.num = 200; //eg:FNum=2.2, aperture = 220, --> num = 220,den = 100
  exifInfo.FNumber.den = 100;
  exifInfo.ISOSpeed = 400;

  exifInfo.ExposureBiasValue.num= 25;
  exifInfo.ExposureBiasValue.den= 100;

  exifInfo.MeteringMode = 0;
  exifInfo.FlashUsed = 0;

  exifInfo.FocalLength.num = 304;
  exifInfo.FocalLength.den = 100;

  exifInfo.DigitalZoomRatio.num = 0;
  exifInfo.DigitalZoomRatio.den = 0;

  exifInfo.WhiteBalance = 0;
  exifInfo.ExposureMode = 0;

  //saveBuffers("/data/camera/yuv_a.bin",src_addr,mJpegBufferSizes,true);
  int ret = AWJpecEnc(&sjpegInfo, &exifInfo, (void *)jpeg_buf, &bufSize);
  //int64_t lasttime = systemTime();
  if (ret < 0)
  {
      HAL_LOGE("JpegEnc failed");
      return false;
  }
  //LOGV("hw enc time: %lld(ms), size: %d", (systemTime() - lasttime)/1000000, bufSize);

  camera3_jpeg_3a_blob_t jpeg_3a_header;
  jpeg_3a_header.jpeg_3a_header_id = CAMERA3_JPEG_3A_PARAM_BLOB_ID;
  jpeg_3a_header.jpeg_3a_size = sizeof(camera3_jpeg_3a_blob_t) + ISP_3A_PARAM_SIZE;
  ALOGD("bill 3a jpeg_3a_size = %d sizeof struct = %d", jpeg_3a_header.jpeg_3a_size, sizeof(camera3_jpeg_3a_blob_t));
  strncpy(jpeg_3a_header.magic_str, ISP_DEBUG_MAGIC_STR, 8);

  camera3_jpeg_isp_msg_blob_t jpeg_isp_msg_header;
  jpeg_isp_msg_header.jpeg_isp_msg_header_id = CAMERA3_JPEG_ISP_MSG_BLOB_ID;
  jpeg_isp_msg_header.jpeg_isp_msg_size = sizeof(camera3_jpeg_isp_msg_blob_t) + ISP_DEBUG_MSG_SIZE;
  ALOGD("bill isp jpeg_isp_size = %d sizeof struct = %d", jpeg_isp_msg_header.jpeg_isp_msg_size, sizeof(camera3_jpeg_isp_msg_blob_t));
  strncpy(jpeg_isp_msg_header.magic_str, ISP_DEBUG_MAGIC_STR, 8);

  camera3_jpeg_blob_t jpegHeader;
  jpegHeader.jpeg_blob_id = CAMERA3_JPEG_BLOB_ID;
  jpegHeader.jpeg_size = bufSize + jpeg_3a_header.jpeg_3a_size + jpeg_isp_msg_header.jpeg_isp_msg_size;

  unsigned long jpeg_eof_offset =
          (unsigned long)(mJpegBufferSizes - (unsigned long)sizeof(jpegHeader));
  char *jpeg_eof = reinterpret_cast<char *>(jpeg_buf +jpeg_eof_offset);
  memcpy(jpeg_eof, &jpegHeader, sizeof(jpegHeader));

  char *jpeg_isp_3a_params = reinterpret_cast<char *>(jpeg_buf + bufSize);
  memcpy(jpeg_isp_3a_params, &jpeg_3a_header, sizeof(jpeg_3a_header));
  mAWIspApi->awIspGet3AParameters((void*)(jpeg_isp_3a_params + sizeof(jpeg_3a_header)));

  char *jpeg_isp_debug_msg = reinterpret_cast<char *>(jpeg_buf + bufSize +
          jpeg_3a_header.jpeg_3a_size);
  memcpy(jpeg_isp_debug_msg, &jpeg_isp_msg_header, sizeof(jpeg_isp_msg_header));
  mAWIspApi->awIspGetDebugMessage((void*)(jpeg_isp_debug_msg + sizeof(jpeg_isp_msg_header)));
#endif

  int64_t endTime = systemTime() / 1000000;
  if (isDebugEnable(DEBUG_PERF)) {
    HAL_LOGI("device_ss_:%d cost:%lldms", device_ss_, endTime - beginTime);
  }

  return 0;
}

int V4L2Stream::WaitCameraReady()
{
  std::lock_guard<std::mutex> lock(stream_lock_);
  if (!format_ && !has_StreamOn) {
    HAL_LOGV(
        "Format not set, so stream can't be on, "
        "so no buffers available for Ready");
    return -EAGAIN;
  }

  int ret = -1;
  int epollftd = epoll_create(2);
  if (epollftd == -1) {
      HAL_LOGE("create epoll failed");
      return -1;
  }

  epoll_event event;
  event.events = EPOLLIN | EPOLLET;
  event.data.fd = device_fd_;
  ret = epoll_ctl(epollftd, EPOLL_CTL_ADD, device_fd_, &event);

  if (ret == -1) {
      HAL_LOGE("add device fd failed");
      close(epollftd);
      return -1;
  }

  epoll_event wakeup;
  wakeup.events = EPOLLIN | EPOLLET;
  wakeup.data.fd = read_fd_;
  epoll_ctl(epollftd, EPOLL_CTL_ADD, read_fd_, &event);

  if (pEvents == NULL) {
      pEvents = (epoll_event *)calloc(2, sizeof(epoll_event));
      if (pEvents == NULL) {
          close(epollftd);
          return -1;
      }
  }
  int nEventNum = epoll_wait(epollftd, pEvents, 2, 1000);
  ret = -1;
  for (int i = 0; i < nEventNum; i++) {
      if (pEvents[i].data.fd == device_fd_) {
          if (pEvents[i].events & EPOLLIN) {
              ret = 0;
              break;
          }
      } else if (pEvents[i].data.fd == read_fd_ && disconnect) {
          char data[2];
          read(read_fd_, data, 2);
          ret = -1;
          break;
      }
  }

  close(epollftd);
  return ret;
}

}  // namespace v4l2_camera_hal
