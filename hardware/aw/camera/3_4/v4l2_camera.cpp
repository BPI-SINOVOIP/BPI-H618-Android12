


#if DBG_V4L2_CAMERA

#endif
#define LOG_TAG "CameraHALv3_V4L2Camera"
#undef NDEBUG

#include <utils/Log.h>


#include "v4l2_camera.h"

#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#include <cstdlib>

#include "CameraMetadata.h"
#include <hardware/camera3.h>


#include "common.h"
#include "function_thread.h"
#include "metadata/metadata_common.h"
#include "stream_format.h"
#include "v4l2_metadata_factory.h"
#include "camera_config.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))

namespace v4l2_camera_hal {

using ::android::hardware::camera::common::V1_0::helper::CameraMetadata;

// Helper function for managing metadata.
static std::vector<int32_t> getMetadataKeys(const camera_metadata_t* metadata) {
  std::vector<int32_t> keys;
  size_t num_entries = get_camera_metadata_entry_count(metadata);
  for (size_t i = 0; i < num_entries; ++i) {
    camera_metadata_ro_entry_t entry;
    get_camera_metadata_ro_entry(metadata, i, &entry);
    keys.push_back(entry.tag);
  }
  return keys;
}

V4L2Camera* V4L2Camera::NewV4L2Camera(int id, const std::string path, std::shared_ptr<CCameraConfig> pCameraCfg) {
  HAL_LOG_ENTER();

  // Select one stream for fill metadata.The path has been pick the stream for query format.
  std::shared_ptr<V4L2Wrapper> v4l2_wrapper(V4L2Wrapper::NewV4L2Wrapper(id, pCameraCfg));
  if (!v4l2_wrapper) {
    HAL_LOGE("Failed to initialize V4L2 wrapper.");
    return nullptr;
  }
  HAL_LOGD("v4l2_wrapper.count %d", v4l2_wrapper.use_count());

  std::unique_ptr<Metadata> metadata;
  // TODOzjw: fix the Metadata frameworks, the google Metadata manager frameworks is
  // too too too complex for us to debug and using.Damn it.
  // Using for format query.
  int res = GetV4L2Metadata(v4l2_wrapper, &metadata, pCameraCfg);
  if (res) {
    HAL_LOGE("Failed to initialize V4L2 metadata: %d", res);
    return nullptr;
  }

  HAL_LOGD("v4l2_wrapper.count %d", v4l2_wrapper.use_count());

  return new V4L2Camera(id, std::move(v4l2_wrapper), std::move(metadata), std::move(pCameraCfg));

  //return new V4L2Camera(id, std::move(metadata));
}

V4L2Camera::V4L2Camera(int id,
                       std::shared_ptr<V4L2Wrapper> v4l2_wrapper,
                       std::unique_ptr<Metadata> metadata, std::shared_ptr<CCameraConfig> pCameraCfg)
    : default_camera_hal::Camera(id),
      device_(std::move(v4l2_wrapper)),
      device_id_(id),
      mgSituationMode(INIT),
      metadata_(std::move(metadata)),
      max_input_streams_(0),
      max_output_streams_({{0, 0, 0}}),
      FlashStatusFlag(0),
      MarkFrameNum(0),
      curFlashStateE(CAM_FLASH_STATE_MAX),
      curFlashModeE(CAM_FLASH_MODE_MAX),
      camera_fd(-1),
      in_flight_buffer_count(0),
      mCameraConfig(std::move(pCameraCfg)),
      buffers_in_flight_flag_(false)
{

  HAL_LOG_ENTER();
  instance = std::shared_ptr<V4L2Camera>(this);
#if 0
  connection[MAIN_STREAM].reset(new V4L2Wrapper::Connection(device_, MAIN_STREAM));
  if (connection[MAIN_STREAM]->status()) {
    HAL_LOGE("Failed to connect to device: %d.", connection[MAIN_STREAM]->status());
    //return connection[MAIN_STREAM].status();
  }
  mStreamm = device_->getStream(MAIN_STREAM);
  if (mStreamm == nullptr) {
    HAL_LOGE("Failed to get Stream %d, we should connect first.", MAIN_STREAM);
    //return 1;
  }
  connection[SUB_0_STREAM].reset(new V4L2Wrapper::Connection(device_, SUB_0_STREAM));
  if (connection[SUB_0_STREAM]->status()) {
    HAL_LOGE("Failed to connect to device: %d.", connection[SUB_0_STREAM]->status());
    //return connection[SUB_0_STREAM]->status();
  }
  mStreams = device_->getStream(SUB_0_STREAM);
  if (mStreams == nullptr) {
    HAL_LOGE("Failed to get Stream %d, we should connect first.", SUB_0_STREAM);
    //return 1;
  }
#endif

  HAL_LOGD("v4l2_wrapper.count %d", v4l2_wrapper.use_count());

  memset(&mBeffer_status, 0, sizeof(mBeffer_status));
  memset(&mStream_status, 0, sizeof(mStream_status));
  for(int i = 0;i < MAX_STREAM;i++)
  {
      mCameraStream[i] = nullptr;
  }

  camFocusImpl.focus_frame_num = -1;
  camFocusImpl.focused_times = -1;
  camFocusImpl.update_status = -1;
  camFocusImpl.status_flag = -1;
  camFocusImpl.focus_status = -1;
  camFocusImpl.update_status_times = -1;
  camFocusImpl.camera_focus_mode = CAM_FOCUS_MODE_MAX;
  camFocusImpl.camera_focus_trigger = CAM_FOCUS_TRIGGER_MAX;
  memset(&camAreaRoi,0,sizeof(cam_area_t));
  memset(&camCropRect,0,sizeof(cam_crop_rect_t));

  mStream_status.pstream = OPEN_STREAM;

}

V4L2Camera::~V4L2Camera() {
  HAL_LOG_ENTER();
  #if 0
  connection[MAIN_STREAM].reset();
  connection[SUB_0_STREAM].reset();
  #endif
}

int V4L2Camera::connect() {
  HAL_LOG_ENTER();

  return 0;
}

void V4L2Camera::disconnect() {
  HAL_LOG_ENTER();
  HAL_LOGD("before disconnect.");
  //std::lock_guard<std::mutex> guard(stream_lock_);
  // stop stream.
  #if 1
  if(mStreamManager_ != nullptr) {
    for(int ss = 0; ss < MAX_STREAM; ss++) {
      mStreamManager_->stop((STREAM_SERIAL)ss);
    }
  }
  #endif
  HAL_LOGD("after disconnect.");
  in_flight_buffer_count = 0;

  mStreamManager_.reset();

  flushRequests(-ENODEV);
  mBeffer_status.pbuffers = INIT_BUFFER;
  mBeffer_status.vbuffers = INIT_BUFFER;
  if(mCameraConfig->supportFlashMode())
  {
      std::lock_guard<std::mutex> guard(camera_fd_lock_);
      if (camera_fd != -1)
      {
          int ret_close = 0;
          ret_close = ::close(camera_fd);
          HAL_LOGD("mCameraFd:%d, ret_close: %d, errno:%s",camera_fd,ret_close,strerror(errno));
          camera_fd = -1;
      }
  }
  // TODO(b/29158098): Inform service of any flashes that are available again
  // because this camera is no longer in use.
}

int V4L2Camera::flushBuffers() {
  HAL_LOG_ENTER();
  int res = 0;
  return res;
}

int V4L2Camera::flushRequests(int err) {
  HAL_LOG_ENTER();
    //Calvin: encount wrong in picture mode.

  // This is not strictly necessary, but prevents a buildup of aborted
  // requests in the in_flight_ map. These should be cleared
  // whenever the stream is turned off.
  HAL_LOGD("We hold %d request(s) in Camera HAL,let us send them to frameworks.",in_flight_.size());
  for(int i = (MAX_BUFFER_NUM -1); i >= 0; i--) {
    //std::lock_guard<std::mutex> guard(in_flight_lock_); // confict in picture mode.
    auto brequest = in_flight_.find(i);
    if(brequest != in_flight_.end()) {
      HAL_LOGD("Send No.%d request to frameworks request queue.",i);
      completeRequest(brequest->second, err);
      in_flight_.erase(i);
    } else {
      HAL_LOGD("No.%d request request had been called back.",i);
      in_flight_.erase(i);
    }
  }

  HAL_LOG_EXIT();

  return 0;
}

int V4L2Camera::initStaticInfo(CameraMetadata* out) {
  HAL_LOG_ENTER();

  int focus_supported = 0;
  int crop_regions_supported = 0;
  if(mCameraConfig->supportFocusMode())
  {
      focus_supported = 1;
  }
  if(mCameraConfig->supportZoom())
  {
      crop_regions_supported = 1;
  }
  int res = metadata_->FillStaticMetadata(out,device_id_,focus_supported,
      crop_regions_supported);
  if (res) {
    HAL_LOGE("Failed to get static metadata.");
    return res;
  }

  if(mCameraConfig->supportFlashMode() && device_id_ == 0)
  {
      HAL_LOGV("####initStaticInfo");
      std::vector<uint8_t> avail_ae_modes;
      avail_ae_modes.push_back(ANDROID_CONTROL_AE_MODE_ON);
      avail_ae_modes.push_back(ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH);
      avail_ae_modes.push_back(ANDROID_CONTROL_AE_MODE_ON_ALWAYS_FLASH);
      out->update(ANDROID_CONTROL_AE_AVAILABLE_MODES,
                  avail_ae_modes.data(),
                  avail_ae_modes.size());
  }

  if(mCameraConfig->supportFocusMode())
  {
      std::vector<uint8_t> avail_af_modes;
      HAL_LOGV("####initStaticInfo avail_af_modes");
      avail_af_modes.push_back(ANDROID_CONTROL_AF_MODE_OFF);
      avail_af_modes.push_back(ANDROID_CONTROL_AF_MODE_AUTO);
      //avail_af_modes.push_back(ANDROID_CONTROL_AF_MODE_MACRO);
      //avail_af_modes.push_back(ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO);
      //avail_af_modes.push_back(ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE);
      out->update(ANDROID_CONTROL_AF_AVAILABLE_MODES,
                  avail_af_modes.data(),
                  avail_af_modes.size());
  }

  // Extract max streams for use in verifying stream configs.
  res = SingleTagValue(
      *out, ANDROID_REQUEST_MAX_NUM_INPUT_STREAMS, &max_input_streams_);
  if (res) {
    HAL_LOGE("Failed to get max num input streams from static metadata.");
    return res;
  }
  res = SingleTagValue(
      *out, ANDROID_REQUEST_MAX_NUM_OUTPUT_STREAMS, &max_output_streams_);
  if (res) {
    HAL_LOGE("Failed to get max num output streams from static metadata.");
    return res;
  }

  return 0;
}

int V4L2Camera::initTemplate(int type, CameraMetadata* out) {
  HAL_LOG_ENTER();

  return metadata_->GetRequestTemplate(type, out);
}

void V4L2Camera::initDeviceInfo(camera_info_t* info) {
  HAL_LOG_ENTER();

  // TODO(b/31044975): move this into device interface.
  // For now, just constants.
  info->resource_cost = 100;
  info->conflicting_devices = nullptr;
  info->conflicting_devices_length = 0;
}

int V4L2Camera::initDevice() {
  HAL_LOG_ENTER();
  return 0;
}

int V4L2Camera::closeFlashTorch()
{
    std::lock_guard<std::mutex> guard(camera_fd_lock_);
    if (camera_fd != -1)
    {
        int ret_close = 0;
        ret_close = ::close(camera_fd);
        HAL_LOGD("mCameraFd:%d, ret_close: %d, errno:%s",camera_fd,ret_close,strerror(errno));
        camera_fd = -1;
        return 1;
    }
    else
    {
        return 0;
    }
}

int V4L2Camera::setFlashTorchMode(bool enabled) {
    int retVal = 0;
    //int cameraFd;
    struct v4l2_input inp;
    struct v4l2_control ctrl;

    if(camera_fd == -1)
    {
        camera_fd = open("/dev/video0", O_RDWR | O_NONBLOCK, 0);
        if (camera_fd == -1)
        {
            HAL_LOGE("ERROR opening /dev/video0: %s", strerror(errno));
            return -1;
        }
        HAL_LOGD("cameraFd : %d opened",camera_fd);
        inp.index = 0;
        if (-1 == ioctl (camera_fd, VIDIOC_S_INPUT, &inp))
        {
            HAL_LOGE("VIDIOC_S_INPUT error!");
            ::close(camera_fd);
            return -1;
        }
    }

    if(enabled)
    {
        ctrl.id = V4L2_CID_FLASH_LED_MODE;
        ctrl.value = V4L2_FLASH_LED_MODE_TORCH;
        retVal = ioctl(camera_fd, VIDIOC_S_CTRL, &ctrl);
        if (retVal < 0)
        {
            HAL_LOGE("setFlashMode failed, %s", strerror(errno));
        }
        else
        {
            HAL_LOGE("setFlashMode ok V4L2_FLASH_LED_MODE_TORCH");
        }
    }
    else
    {
        ctrl.id = V4L2_CID_FLASH_LED_MODE;
        ctrl.value = V4L2_FLASH_LED_MODE_NONE;
        retVal = ioctl(camera_fd, VIDIOC_S_CTRL, &ctrl);
        if (retVal < 0)
        {
            HAL_LOGE("setFlashMode failed, %s", strerror(errno));
        }
        else
        {
            HAL_LOGE("setFlashMode ok V4L2_FLASH_LED_MODE_NONE");
        }
    }

    return retVal;
}

int V4L2Camera::updateStatus(SituationMode mSituationMode) {

  mgSituationMode = mSituationMode;
  switch (mSituationMode) {
    case BEFORE_PREVIEW:
      break;
    case BEFORE_VIDEO:
      break;
    case BEFORE_PICTURE:
      mStream_status.pstream = CLOSE_STREAM;
      mStream_status.tstream = OPEN_STREAM;
      mBeffer_status.tbuffers = INIT_BUFFER;
      break;
    case AFTER_PICTURE:
      mStream_status.pstream = OPEN_STREAM;
      mBeffer_status.pbuffers = INIT_BUFFER;
      mBeffer_status.tbuffers = INIT_BUFFER;
      break;
    default:
      break;
    }
  return -1;
}


V4L2Camera::SituationMode V4L2Camera::getStatus() {
  return mgSituationMode;
}

V4L2Camera::RequestMode V4L2Camera::analysisRequest(
    std::shared_ptr<default_camera_hal::CaptureRequest> request) {

  if(request == nullptr) {
    if((mVideoflag||isVideoByTemplate) && mBeffer_status.vbuffers == PREPARE_BUFFER) {
      return RUN_VIDEO_MODE;
    } else if(mBeffer_status.pbuffers == PREPARE_BUFFER){
      return RUN_PREVIEW_MODE;
    }
  } else {
    // If the request include the tstream, return immediatly.
    for(int i = 0; i <request->output_buffers.size(); i++) {
      if((analysisStreamModes(request->output_buffers[i].stream) == TSTREAM) && (request->tbuffer_has_been_used == false)) {
        return PICTURE_MODE;
      }
    }

    // If the request include the vstream and in video mode, return immediatly.
    for(int i = 0; i <request->output_buffers.size(); i++) {
      if((analysisStreamModes(request->output_buffers[i].stream) == VSTREAM) && (mVideoflag||isVideoByTemplate) ) {
        return VIDEO_MODE;
      }
    }

    // Deal the preview stream in the final.
    for(int i = 0; i <request->output_buffers.size(); i++) {
      if((analysisStreamModes(request->output_buffers[i].stream) == PSTREAM) ) {
        return PREVIEW_MODE;
      }
    }
  }

  return ERROR_MODE;
}
int V4L2Camera::analysisStreamNum(
    std::shared_ptr<default_camera_hal::CaptureRequest> request, RequestMode mRequestMode) {


  int revalue = -1;
  for(int i = 0; i <request->output_buffers.size(); i++) {
    HAL_LOGD("Get format %d!", request->output_buffers[i].stream->format);
    if(request->output_buffers[i].stream->format == HAL_PIXEL_FORMAT_BLOB) {
      if(mRequestMode == VIDEO_SNAP_SHOT_MODE) {
        revalue = i;
      }
    }

    if(request->output_buffers[i].stream->format == HAL_PIXEL_FORMAT_YCBCR_420_888) {
      if(mRequestMode == PICTURE_MODE) {
        revalue = i;
      }
    }

    if(request->output_buffers[i].stream->format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED) {
      if((mRequestMode == PREVIEW_MODE) && (analysisStreamModes(request->output_buffers[i].stream) == PSTREAM)) {
        revalue = i;
      }
      if((mRequestMode == VIDEO_MODE) && (analysisStreamModes(request->output_buffers[i].stream) == VSTREAM)) {
        revalue = i;
      }
    }
  }
  HAL_LOGD("No.%d is picked.", revalue);

  return revalue;
}

V4L2Camera::StreamIedex V4L2Camera::analysisStreamModes(
    camera3_stream_t * stream) {

  HAL_LOGD("Get usage %d, format %d!", stream->usage, stream->format);

  StreamIedex mStreamIedex = ESTREAM;
  switch (stream->format) {
    case HAL_PIXEL_FORMAT_YCBCR_420_888: {
        HAL_LOGD("Detected the picture stream.");
        mStreamIedex = TSTREAM;
        break;
    }
    case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED: {
        if((stream->usage & GRALLOC_USAGE_HW_TEXTURE)||(stream->usage & (GRALLOC_USAGE_HW_COMPOSER|GRALLOC_USAGE_HW_TEXTURE))) {
          mStreamIedex = PSTREAM;
        } else if((stream->usage & (GRALLOC_USAGE_HW_VIDEO_ENCODER|GRALLOC_USAGE_HW_2D))
                //||((stream->usage & GRALLOC_USAGE_SW_READ_OFTEN)&&(isVideoByTemplate))
                  ) {
          mStreamIedex = VSTREAM;
        }
        break;
    }
    // Be compatible with Camera api 1.
    case HAL_PIXEL_FORMAT_BLOB: {
        #if 1
        if((stream->usage & GRALLOC_USAGE_SW_READ_OFTEN)&&(isVideoByTemplate)) {
             mStreamIedex = VSTREAM;
        }
        #endif
        if ((stream->usage & GRALLOC_USAGE_SW_READ_OFTEN)&& (stream->usage & GRALLOC_USAGE_SW_WRITE_OFTEN)) {
          mStreamIedex = VHSTREAM;
          //isVideoSnap  = true;
          HAL_LOGD("Set stream isVideoSnap flag to ture!");
        }
        break;
    }
    default: {
      HAL_LOGE("Do not find any stream!");
      break;
    }
  }

  return mStreamIedex;
}

int V4L2Camera::max(int a, int b, int c) {

  int ret = -1;
  if(a >= b) {
    if(a >= c){
      ret = 0;
    } else {
      ret = 2;
    }
  } else {
    if(b >= c){
      ret = 1;
    } else {
      ret = 2;
    }
  }
  HAL_LOGV("a:%d,b:%d,c:%d, ret index: %d!", a, b, c, ret);
  return ret;
}

int V4L2Camera::min(int a, int b, int c) {

  int ret = -1;
  if(a >= b) {
    if(b >= c){
      ret = 2;
    } else {
      ret = 1;
    }
  } else {
    if(a >= c){
      ret = 2;
    } else {
      ret = 0;
    }
  }
  HAL_LOGV("a:%d,b:%d,c:%d, ret index: %d!", a, b, c, ret);
  return ret;
}

int V4L2Camera::maxWidth(camera3_stream_t ** streams, int num) {
  int ret = -1;
  if(num == 0) {
    HAL_LOGE("Prepare num err!");
  } else if (num == 1) {
    ret = 0;
  } else if (num == 2) {
    if(streams[0]->width >streams[1]->width) {
      ret = 0;
    } else if (streams[0]->width = streams[1]->width) {
      ret = 0;
    } else {
      ret = 1;
    }
  } else if (num == 3) {
    ret = max(streams[0]->width, streams[1]->width, streams[2]->width);
  }
  return ret;
}
int V4L2Camera::maxHeight(camera3_stream_t ** streams, int num) {
  int ret = -1;
  if(num == 0) {
    HAL_LOGE("Prepare num err!");
  } else if (num == 1) {
    ret = 0;
  } else if (num == 2) {
    if(streams[0]->height >streams[1]->height) {
      ret = 0;
    } else if (streams[0]->height = streams[1]->height) {
      ret = 0;
    } else {
      ret = 1;
    }
  } else if (num == 3) {
    ret = max(streams[0]->height, streams[1]->height, streams[2]->height);
  }
  return ret;
}
int V4L2Camera::fillStreamInfo(camera3_stream_t * stream) {
  int res = 0;
  // Doesn't matter what was requested, we always use dataspace V0_JFIF.
  // Note: according to camera3.h, this isn't allowed, but the camera
  // framework team claims it's underdocumented; the implementation lets the
  // HAL overwrite it. If this is changed, change the validation above.
  stream->data_space = HAL_DATASPACE_V0_JFIF;

  // Usage: currently using sw graphics.
  switch (stream->stream_type) {

    case CAMERA3_STREAM_INPUT:
      stream->usage |= GRALLOC_USAGE_SW_READ_OFTEN;
      break;
    case CAMERA3_STREAM_OUTPUT:
      // Set the usage to GRALLOC_USAGE_SW_WRITE_OFTEN for buffers with cache alloc by gralloc.
      stream->usage |= GRALLOC_USAGE_SW_WRITE_OFTEN;
      break;
    case CAMERA3_STREAM_BIDIRECTIONAL:
      stream->usage |=
          (GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN);
      break;
    default:
      // nothing to do.
      break;
  }
  // Add camera usage.
  stream->usage |= GRALLOC_USAGE_HW_CAMERA_WRITE;

  switch (stream->format) {
    case HAL_PIXEL_FORMAT_BLOB: {
        stream->max_buffers = PICTURE_BUFFER_NUM;
        break;
    }
    case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED: {
        stream->max_buffers = MAX_BUFFER_NUM;
        break;
    }
    case HAL_PIXEL_FORMAT_YCBCR_420_888: {
        stream->max_buffers = PICTURE_BUFFER_NUM;
        break;
    }
    default: {
      HAL_LOGE("Do not find any format!");
      break;
    }
  }
  return res;
}
int V4L2Camera::findStreamModes(STREAM_SERIAL stream_serial,
    camera3_stream_configuration_t* stream_config, int *isBlob) {

  int mStreamIndex = -1;
  int tmpMaxWI = -1;
  int tmpMaxHI = -1;
  int tmpMax = -1;

  switch (stream_serial) {
    case MAIN_STREAM: {
        if(stream_config->num_streams == 1) {
          mStreamIndex = 0;
        } else if(stream_config->num_streams == 2) {
          if((stream_config->streams[0]->width * stream_config->streams[0]->height)
            >= (stream_config->streams[1]->width * stream_config->streams[1]->height)) {
            mStreamIndex = 0;
          } else {
            mStreamIndex = 1;
          }
        } else if(stream_config->num_streams == 3) {
          mStreamIndex = max((stream_config->streams[0]->width * stream_config->streams[0]->height),
          (stream_config->streams[1]->width * stream_config->streams[1]->height),
          (stream_config->streams[2]->width * stream_config->streams[2]->height));
        } else {
          HAL_LOGE("Get stream_config->num_streams:%d failed!", stream_config->num_streams);
          break;
        }
        if(stream_config->streams[mStreamIndex]->format == HAL_PIXEL_FORMAT_BLOB) {
          *isBlob = 1;
        } else {
          *isBlob = 0;
        }
        break;
    }
    case SUB_0_STREAM: {
        if(stream_config->num_streams <= 1) {
          HAL_LOGE("Get stream_config->num_streams:%d failed!", stream_config->num_streams);
        } else if(stream_config->num_streams == 2) {
          int maintmp = findStreamModes(MAIN_STREAM, stream_config, isBlob);
          if(maintmp == 1) {
            mStreamIndex = 0;
          } else {
            mStreamIndex = 1;
          }
        } else if(stream_config->num_streams == 3) {
          int maxtmp = max((stream_config->streams[0]->width * stream_config->streams[0]->height),
          (stream_config->streams[1]->width * stream_config->streams[1]->height),
          (stream_config->streams[2]->width * stream_config->streams[2]->height));
          int mintmp = min((stream_config->streams[0]->width * stream_config->streams[0]->height),
          (stream_config->streams[1]->width * stream_config->streams[1]->height),
          (stream_config->streams[2]->width * stream_config->streams[2]->height));

          for(int i = 0;i <stream_config->num_streams; i++) {
            if((i != maxtmp) && (i != mintmp) ) {
              mStreamIndex = i;
            }
          }
        } else {
          HAL_LOGE("Get stream_config->num_streams:%d failed!", stream_config->num_streams);
          break;
        }
        if(stream_config->streams[mStreamIndex]->format == HAL_PIXEL_FORMAT_BLOB) {
          *isBlob = 1;
        } else {
          *isBlob = 0;
        }
        break;
    }
    default: {
      HAL_LOGE("Do not find any stream!");
      break;
    }
  }

  return mStreamIndex;
}

int V4L2Camera::ProbeFlashMode(CameraFlashModeE *camera_flash_mode,
        CameraMetadata* camera_meta_data,
        int *set_flash_flag,int frame_number)
{
    int result = 0;
    uint8_t fwk_aeMode = 0;
    uint8_t capture_intent = 0;
    uint8_t fwk_flashMode = 0;
    //vCameraFlashState v_camera_flash_state;
    if(camera_meta_data->exists(ANDROID_CONTROL_AE_MODE))
    {
        SingleTagValue(camera_meta_data, ANDROID_CONTROL_AE_MODE, &fwk_aeMode);
    }
    if (camera_meta_data->exists(ANDROID_CONTROL_CAPTURE_INTENT))
    {
        SingleTagValue(camera_meta_data, ANDROID_CONTROL_CAPTURE_INTENT, &capture_intent);
    }
    if(camera_meta_data->exists(ANDROID_FLASH_MODE))
    {
        SingleTagValue(camera_meta_data, ANDROID_FLASH_MODE, &fwk_flashMode);
    }

    std::lock_guard<std::mutex> guard(flash_status_lock_);
    if(capture_intent == ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE &&
        fwk_aeMode == ANDROID_CONTROL_AE_MODE_ON_ALWAYS_FLASH)
    {
        if(!FlashStatusFlag)
        {
            MarkFrameNum = frame_number;
            FlashStatusFlag = 1;
            *camera_flash_mode = CAM_FLASH_MODE_ON;
            *set_flash_flag = 1;
        }
    }

    if(capture_intent == ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE &&
        fwk_aeMode == ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH)
    {
        if(!FlashStatusFlag)
        {
            MarkFrameNum = frame_number;
            FlashStatusFlag = 1;
            *camera_flash_mode = CAM_FLASH_MODE_AUTO;
            *set_flash_flag = 1;
        }
    }

    if(capture_intent == ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE &&
        fwk_aeMode == ANDROID_CONTROL_AE_MODE_ON &&
        fwk_flashMode == ANDROID_FLASH_MODE_TORCH)
    {
            if(curFlashModeE != CAM_FLASH_MODE_TORCH)
            {
                MarkFrameNum = 0;
                curFlashModeE == CAM_FLASH_MODE_TORCH;
                *camera_flash_mode = CAM_FLASH_MODE_TORCH;
                *set_flash_flag = 1;
            }
    }

    if(capture_intent == ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD &&
        fwk_flashMode == ANDROID_FLASH_MODE_TORCH)
    {
        HAL_LOGV("########CAM_FLASH_MODE_TORCH");
        MarkFrameNum = 0;
        *camera_flash_mode = CAM_FLASH_MODE_TORCH;
        *set_flash_flag = 1;
    }

    if(capture_intent == ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD &&
            fwk_flashMode == ANDROID_FLASH_MODE_OFF && frame_number > 10)
    {
        HAL_LOGV("########CAM_FLASH_MODE_VIDEO_OFF");
        MarkFrameNum = 0;
        *camera_flash_mode = CAM_FLASH_MODE_VIDEO_OFF;
        *set_flash_flag = 1;
    }

    if(MarkFrameNum != 0 && frame_number - MarkFrameNum == 20)
    {
        *camera_flash_mode = CAM_FLASH_MODE_OFF;
        *set_flash_flag = 1;
    }
    if(MarkFrameNum != 0 && frame_number - MarkFrameNum > 33)
    {
        FlashStatusFlag = 0;
        MarkFrameNum = 0;
    }
    if(capture_intent == ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW &&
        fwk_aeMode == ANDROID_CONTROL_AE_MODE_ON_ALWAYS_FLASH &&
        fwk_flashMode == ANDROID_FLASH_MODE_OFF)
    {
        HAL_LOGV("####ON_ALWAYS_FLASH MODE_OFF frame_number:%d",frame_number);
        /*v_camera_flash_state.flash_frame_num = frame_number;
          v_camera_flash_state.camera_flash_mode_e = CAM_FLASH_MODE_ON;
          vecCameraFlashState.push_back(v_camera_flash_state);*/
        curFlashStateE = CAM_FLASH_STATE_READY;
    }
    if(capture_intent == ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW &&
        fwk_aeMode == ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH &&
        fwk_flashMode == ANDROID_FLASH_MODE_OFF)
    {
        HAL_LOGV("####ON_AUTO_FLASH MODE_OFF frame_number:%d",frame_number);
        /*v_camera_flash_state.flash_frame_num = frame_number;
          v_camera_flash_state.camera_flash_mode_e = CAM_FLASH_MODE_AUTO;
          vecCameraFlashState.push_back(v_camera_flash_state);*/
        curFlashStateE = CAM_FLASH_STATE_READY;
        curFlashModeE = CAM_FLASH_MODE_AUTO;
    }
    if(capture_intent == ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW &&
        fwk_aeMode == ANDROID_CONTROL_AE_MODE_ON &&
        fwk_flashMode == ANDROID_FLASH_MODE_OFF)
    {
        HAL_LOGV("####AE_MODE_ON FLASH_MODE_OFF frame_number:%d",frame_number);
        /*v_camera_flash_state.flash_frame_num = frame_number;
           v_camera_flash_state.camera_flash_mode_e = CAM_FLASH_MODE_ON;
           vecCameraFlashState.push_back(v_camera_flash_state);*/
        if(curFlashModeE == CAM_FLASH_MODE_TORCH)
        {
            *camera_flash_mode = CAM_FLASH_MODE_VIDEO_OFF;
            *set_flash_flag = 1;
        }
        curFlashStateE = CAM_FLASH_STATE_READY;
    }
    if(capture_intent == ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW &&
        fwk_aeMode == ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH_REDEYE &&
        fwk_flashMode == ANDROID_FLASH_MODE_OFF)
    {
        HAL_LOGV("####REDEYE MODE_OFF frame_number:%d",frame_number);
        /*v_camera_flash_state.flash_frame_num = frame_number;
        v_camera_flash_state.camera_flash_mode_e = CAM_FLASH_MODE_ON;
        vecCameraFlashState.push_back(v_camera_flash_state);*/
        curFlashStateE = CAM_FLASH_STATE_READY;
    }
    if(capture_intent == ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW &&
        fwk_aeMode == ANDROID_CONTROL_AE_MODE_ON &&
        fwk_flashMode == ANDROID_FLASH_MODE_SINGLE)
    {
        HAL_LOGV("####FIRED FLASH_MODE_SINGLE frame_number:%d",frame_number);
        /*v_camera_flash_state.flash_frame_num = frame_number;
          v_camera_flash_state.camera_flash_mode_e = CAM_FLASH_MODE_ON;
          vecCameraFlashState.push_back(v_camera_flash_state);*/
        curFlashStateE = CAM_FLASH_STATE_FIRED;
    }
    if(capture_intent == ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW &&
        fwk_flashMode == ANDROID_FLASH_MODE_TORCH &&
        fwk_aeMode == ANDROID_CONTROL_AE_MODE_ON)
    {
        HAL_LOGV("####TORCH AE_MODE_ON frame_number:%d",frame_number);
        /*v_camera_flash_state.flash_frame_num = frame_number;
          v_camera_flash_state.camera_flash_mode_e = CAM_FLASH_MODE_TORCH;
          vecCameraFlashState.push_back(v_camera_flash_state);*/
        MarkFrameNum = 0;
        *camera_flash_mode = CAM_FLASH_MODE_TORCH;
        *set_flash_flag = 1;
        curFlashModeE = CAM_FLASH_MODE_TORCH;
        curFlashStateE = CAM_FLASH_STATE_FIRED;
    }
    return result;
}
void V4L2Camera::updateFlashState( CameraMetadata* camera_meta_data, int frameNumber, int curFlashStateE)
{
    int ret;
    if(mCameraConfig->supportFlashMode())
     {
         uint8_t fwk_flashState;
         uint8_t fwk_aeMode;
         if(curFlashStateE == CAM_FLASH_STATE_READY)
         {
             fwk_flashState = ANDROID_FLASH_STATE_READY;
             ret = camera_meta_data->update(ANDROID_FLASH_STATE, &fwk_flashState, 1);
             if (ret) {
                  HAL_LOGE("Failed to update metadata tag 0x%x", ANDROID_FLASH_STATE);
             }
         }
         else if(curFlashStateE == CAM_FLASH_STATE_FIRED)
         {
             fwk_flashState = ANDROID_FLASH_STATE_FIRED;
             ret = camera_meta_data->update(ANDROID_FLASH_STATE, &fwk_flashState, 1);
             if (ret) {
                 HAL_LOGE("Failed to update metadata tag 0x%x", ANDROID_FLASH_STATE);
             }
         }
         if(curFlashModeE == CAM_FLASH_MODE_AUTO)
         {
             HAL_LOGV("####frameNumber:%d",frameNumber);
             fwk_aeMode = ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH;
             ret = camera_meta_data->update(ANDROID_CONTROL_AE_MODE, &fwk_aeMode, 1);
             if (ret) {
                 HAL_LOGE("Failed to update metadata tag 0x%x", ANDROID_CONTROL_AE_MODE);
             }
         }
         if (isDebugEnable(DEBUG_FLASH))
            HAL_LOGI("####frameNumber:%d fwk_flashState:%d curFlashModeE:%d",frameNumber, fwk_flashState, curFlashStateE);
     }
}

void V4L2Camera::updateAfState(CameraMetadata *camera_meta_data, int frameNumber)
{
    int ret;
    int i;
    CameraStream *focusStream;
    uint8_t fwk_afStatus;
    uint8_t fwk_afTrigger;
    uint8_t fwk_afMode;


    if(mCameraConfig->supportFocusMode()) {
        for(i = 0;i < MAX_STREAM;i++) {
                if(mCameraStream[i] != nullptr)
                {
                    focusStream = mCameraStream[i];
                    break;
                }
                if (i == MAX_STREAM) {
                    HAL_LOGE("No active stream here, something muse be err");
                    return;
                }
         }
        int32_t focus_status = focusStream->getFocusStatus();
        if (isDebugEnable(DEBUG_AF)) {
            HAL_LOGI("AF frameNumber:%d  focus_status:%d focused_times:%d", frameNumber, focus_status,camFocusImpl.focused_times);
        }
        if(focus_status == 2 && camFocusImpl.focused_times < 2)
        {
            fwk_afStatus = ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED;
            ret = camera_meta_data->update(ANDROID_CONTROL_AF_STATE, &fwk_afStatus, 1);
            if (ret) {
                HAL_LOGE("Failed to update metadata tag 0x%x", ANDROID_CONTROL_AF_STATE);
            }
            camFocusImpl.focused_times++;
        }

        if(camera_meta_data->exists(ANDROID_CONTROL_AF_TRIGGER))
        {
            SingleTagValue(camera_meta_data,  ANDROID_CONTROL_AF_TRIGGER,  &fwk_afTrigger);
        }
        if(camera_meta_data->exists(ANDROID_CONTROL_AF_MODE))
        {
            SingleTagValue(camera_meta_data,  ANDROID_CONTROL_AF_MODE,  &fwk_afMode);
        }
        if (isDebugEnable(DEBUG_AF)) {
            HAL_LOGI("AF frameNumber:%d afStatus:0x%x afTrigger:0x%x afMode:0x%x",frameNumber, fwk_afStatus, fwk_afTrigger, fwk_afMode);
        }
        if (camFocusImpl.focus_status == CAM_FOCUS_MODE_START ) {
                fwk_afStatus = ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED;
                camera_meta_data->update(ANDROID_CONTROL_AF_STATE, &fwk_afStatus, 1);
        } else if(camFocusImpl.focus_status == CAM_FOCUS_MODE_STOP) {
                fwk_afStatus = ANDROID_CONTROL_AF_STATE_INACTIVE;
                camera_meta_data->update(ANDROID_CONTROL_AF_STATE, &fwk_afStatus, 1);
        }

        if(focus_status == 2 &&
            fwk_afTrigger == ANDROID_CONTROL_AF_TRIGGER_START &&
            fwk_afMode == ANDROID_CONTROL_AF_MODE_AUTO)
        {
            fwk_afStatus = ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED;
            ret = camera_meta_data->update(ANDROID_CONTROL_AF_STATE, &fwk_afStatus, 1);
            if (ret) {
                HAL_LOGE("Failed to update metadata tag 0x%x", ANDROID_CONTROL_AF_STATE);
            }
            camFocusImpl.status_flag = 1;
        }
        if(focus_status == 4 &&
            fwk_afTrigger == ANDROID_CONTROL_AF_TRIGGER_START &&
            fwk_afMode == ANDROID_CONTROL_AF_MODE_AUTO)
        {
            fwk_afStatus = ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED;
            ret = camera_meta_data->update(ANDROID_CONTROL_AF_STATE, &fwk_afStatus, 1);
            if (ret) {
                HAL_LOGE("Failed to update metadata tag 0x%x", ANDROID_CONTROL_AF_STATE);
            }
            camFocusImpl.status_flag = 1;
        }
        if(focus_status == 2 &&
            camFocusImpl.update_status == 1 &&
            camFocusImpl.status_flag == 1)
        {
            fwk_afStatus = ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED;
            ret = camera_meta_data->update(ANDROID_CONTROL_AF_STATE, &fwk_afStatus, 1);
            if (ret) {
                HAL_LOGE("Failed to update metadata tag 0x%x", ANDROID_CONTROL_AF_STATE);
            }
            camFocusImpl.update_status_times++;
        }
        if(focus_status == 4 &&
            camFocusImpl.update_status == 1 &&
            camFocusImpl.status_flag == 1)
        {
            fwk_afStatus = ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED;
            ret = camera_meta_data->update(ANDROID_CONTROL_AF_STATE, &fwk_afStatus, 1);
            if (ret) {
                HAL_LOGE("Failed to update metadata tag 0x%x", ANDROID_CONTROL_AF_STATE);
            }
            camFocusImpl.update_status_times++;
        }
        if(focus_status == 1)
        {
            fwk_afStatus = ANDROID_CONTROL_AF_STATE_ACTIVE_SCAN;
            ret = camera_meta_data->update(ANDROID_CONTROL_AF_STATE_ACTIVE_SCAN, &fwk_afStatus, 1);
            if (ret) {
                HAL_LOGE("Failed to update metadata tag 0x%x", ANDROID_CONTROL_AF_STATE);
            }
        }
        if (isDebugEnable(DEBUG_AF)) {
            HAL_LOGI("AF frameNumber:%d afStatus%x update_status_times%d status_flag:%d update_status:%d",
                frameNumber, fwk_afStatus, camFocusImpl.update_status_times, camFocusImpl.status_flag, camFocusImpl.update_status);
        }
    }
}
void V4L2Camera::convertFromRegions(cam_area_t &roi,
        const CameraMetadata &frame_settings, uint32_t tag)
{
    int32_t x_min = frame_settings.find(tag).data.i32[0];
    int32_t y_min = frame_settings.find(tag).data.i32[1];
    int32_t x_max = frame_settings.find(tag).data.i32[2];
    int32_t y_max = frame_settings.find(tag).data.i32[3];
    roi.weight = frame_settings.find(tag).data.i32[4];
    roi.rect.x_min = x_min;
    roi.rect.y_min = y_min;
    roi.rect.x_max = x_max;
    roi.rect.y_max = y_max;
}
void V4L2Camera::updateActiveArray(CameraMetadata *camera_meta_data, uint32_t tag){
    std::array<int32_t, 5>  ActiveRegions= {0, 0, 0, 0, 0};
    SingleTagValue(camera_meta_data, tag, &ActiveRegions);
    int32_t x_min  = ActiveRegions[0];
    int32_t y_min  = ActiveRegions[1];
    int32_t x_max  = ActiveRegions[2];
    int32_t y_max  = ActiveRegions[3];
    int32_t weight = ActiveRegions[4];
    if((camCropRect.left + camCropRect.width) > x_min
        && (camCropRect.top + camCropRect.height) > y_min
        && camCropRect.left < x_max && camCropRect.top < y_max)  {

        x_min = std::max(camCropRect.left, x_min);
        y_min = std::max(camCropRect.top, y_min);
        x_max = std::min(x_max, camCropRect.left + camCropRect.width);
        y_max = std::min(y_max, camCropRect.top  + camCropRect.height);
    } else {
        x_min = 0;
        y_min = 0;
        x_max = 0;
        y_max = 0;
    }
    int32_t NewActiveRegions[5] = {x_min, y_min, x_max, y_max, weight};
    camera_meta_data->update(tag, NewActiveRegions, 5);
}

int V4L2Camera::ProcessFocusMode(CameraMetadata *camera_meta_data,int frame_number)
{
    int result = 0;
    uint8_t fwk_afMode = 0;
    uint8_t fwk_afTrigger = 0;
    uint8_t fwk_afStatus = 0;
    CameraStream *focusStream;
    //std::shared_ptr<CameraStream>focusStream;
    int i = 0;

    std::lock_guard<std::mutex> guard(auto_focus_lock_);
    if(camera_meta_data->exists(ANDROID_CONTROL_AF_MODE))
    {
        for(i = 0;i < MAX_STREAM;i++)
        {
            if(mCameraStream[i] != nullptr)
            {
                focusStream = mCameraStream[i];
                break;
            }
        }
        if(i >= MAX_STREAM)
        {
            HAL_LOGE("can not find camera stream!");
            return -1;
        }
        camFocusImpl.focus_frame_num = frame_number;
        SingleTagValue(camera_meta_data, ANDROID_CONTROL_AF_MODE, &fwk_afMode);
        if (fwk_afMode == ANDROID_CONTROL_AF_MODE_OFF) {
            HAL_LOGV("####ANDROID_CONTROL_AF_MODE_OFF frameNumber:%d",frame_number);
            camFocusImpl.camera_focus_mode = CAM_FOCUS_MODE_OFF;
        } else if (fwk_afMode == ANDROID_CONTROL_AF_MODE_AUTO) {
            HAL_LOGV("####ANDROID_CONTROL_AF_MODE_AUTO frameNumber:%d",frame_number);
            camFocusImpl.camera_focus_mode = CAM_FOCUS_MODE_AUTO;
        } else if(fwk_afMode == ANDROID_CONTROL_AF_MODE_MACRO) {
            HAL_LOGV("####ANDROID_CONTROL_AF_MODE_MACRO frameNumber:%d",frame_number);
            camFocusImpl.camera_focus_mode = CAM_FOCUS_MODE_MACRO;
        } else if(fwk_afMode == ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO) {
            HAL_LOGV("####ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO frameNumber:%d",frame_number);
            camFocusImpl.camera_focus_mode = CAM_FOCUS_MODE_CONTINUOUS_VIDEO;
        } else if(fwk_afMode == ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE) {
            HAL_LOGV("####ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE frameNumber:%d",frame_number);
            camFocusImpl.camera_focus_mode = CAM_FOCUS_MODE_CONTINUOUS_PICTURE;
        } else {
            HAL_LOGE("####error ANDROID_CONTROL_AF_MODE frameNumber:%d",frame_number);
        }
    }

    if (camera_meta_data->exists(ANDROID_CONTROL_AF_TRIGGER))
    {
            SingleTagValue(camera_meta_data, ANDROID_CONTROL_AF_TRIGGER, &fwk_afTrigger);
            if (fwk_afTrigger == ANDROID_CONTROL_AF_TRIGGER_IDLE) {
                HAL_LOGV("####ANDROID_CONTROL_AF_TRIGGER_IDLE frameNumber:%d",frame_number);
                camFocusImpl.camera_focus_trigger = CAM_FOCUS_TRIGGER_IDLE;
            } else if (fwk_afTrigger == ANDROID_CONTROL_AF_TRIGGER_START) {
                HAL_LOGV("####ANDROID_CONTROL_AF_TRIGGER_START frameNumber:%d",frame_number);
                camFocusImpl.camera_focus_trigger = CAM_FOCUS_TRIGGER_START;
            } else if(fwk_afTrigger == ANDROID_CONTROL_AF_TRIGGER_CANCEL) {
                HAL_LOGV("####ANDROID_CONTROL_AF_TRIGGER_CANCEL frameNumber:%d",frame_number);
                camFocusImpl.camera_focus_trigger = CAM_FOCUS_TRIGGER_CANCLE;
            }else {
                HAL_LOGE("####error ANDROID_CONTROL_AF_TRIGGER frameNumber:%d",frame_number);
            }
    }
    if (camera_meta_data->exists(ANDROID_SCALER_CROP_REGION)){
        updateActiveArray(camera_meta_data, ANDROID_CONTROL_AF_REGIONS);
    }
    if (camera_meta_data->exists(ANDROID_CONTROL_AF_REGIONS)) {
        cam_area_t roi;
        int equal_flag = 0;
        memset(&roi,0,sizeof(cam_area_t));
        std::array<int32_t, 5> AfRegions = {0, 0, 0, 0, 0};
        SingleTagValue(camera_meta_data,
                              ANDROID_CONTROL_AF_REGIONS,
                              &AfRegions);
        roi.rect.x_min = AfRegions[0];
        roi.rect.y_min = AfRegions[1];
        roi.rect.x_max = AfRegions[2];
        roi.rect.y_max = AfRegions[3];
        roi.weight     = AfRegions[4];
        if (isDebugEnable(DEBUG_AF)) {
            HAL_LOGI("AF frameNumber:%d,roi.weight:%d,roi.rect.x_min:%d roi.rect.y_min:%d roi.rect.x_max:%d roi.rect.y_max:%d",
                frame_number,roi.weight,roi.rect.x_min,roi.rect.y_min,roi.rect.x_max,roi.rect.y_max);
        }
        if(roi.weight != 0)
        {
            cam_rect_t cam_regions;
            cam_regions.x_min = roi.rect.x_min;
            cam_regions.y_min = roi.rect.y_min;
            cam_regions.x_max = roi.rect.x_max;
            cam_regions.y_max = roi.rect.y_max;
            camAreaRoi.rect.x_min = roi.rect.x_min;
            camAreaRoi.rect.y_min = roi.rect.y_min;
            camAreaRoi.rect.x_max = roi.rect.x_max;
            camAreaRoi.rect.y_max = roi.rect.y_max;
            camAreaRoi.weight = roi.weight;
            focusStream->setIspFocusRegions(cam_regions);
        }
    }

    if(fwk_afMode == ANDROID_CONTROL_AF_MODE_AUTO &&
        fwk_afTrigger == ANDROID_CONTROL_AF_TRIGGER_IDLE)
    {
        camFocusImpl.update_status = 1;
    }

    if(fwk_afMode == ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE &&
        frame_number == 0)
    {
        focusStream->setIspFocus(CAM_FOCUS_MODE_START_UP);
        camFocusImpl.focus_status = CAM_FOCUS_MODE_START_UP;
    }
    else if(fwk_afMode == ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE &&
        frame_number != 0 &&
        camFocusImpl.focus_status != CAM_FOCUS_MODE_CONTINUOUS_PICTURE)
    {
        focusStream->setIspFocus(CAM_FOCUS_MODE_CONTINUOUS_PICTURE);
        camFocusImpl.focus_status = CAM_FOCUS_MODE_CONTINUOUS_PICTURE;
    }
    else if(fwk_afTrigger == ANDROID_CONTROL_AF_TRIGGER_CANCEL)
    {
        focusStream->setIspFocus(CAM_FOCUS_MODE_STOP);
        camFocusImpl.focus_status = CAM_FOCUS_MODE_STOP;
        camFocusImpl.camera_focus_trigger = CAM_FOCUS_TRIGGER_CANCLE;
    }
    else if(fwk_afTrigger == ANDROID_CONTROL_AF_TRIGGER_START &&
        fwk_afMode != ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE &&
        camFocusImpl.focus_status != CAM_FOCUS_MODE_START)
    {
        focusStream->setIspFocus(CAM_FOCUS_MODE_START);
        camFocusImpl.focus_status = CAM_FOCUS_MODE_START;
        camFocusImpl.focused_times = -1;
    }
    else if(fwk_afMode == ANDROID_CONTROL_AF_MODE_AUTO &&
        fwk_afTrigger == ANDROID_CONTROL_AF_TRIGGER_IDLE &&
        camFocusImpl.focus_status == CAM_FOCUS_MODE_STOP)
    {
        camFocusImpl.focus_status = CAM_FOCUS_MODE_STOP;
        camFocusImpl.focused_times = -1;
    }
    else if(fwk_afMode == ANDROID_CONTROL_AF_MODE_AUTO &&
        fwk_afTrigger == ANDROID_CONTROL_AF_TRIGGER_IDLE &&
        camFocusImpl.focus_status == CAM_FOCUS_MODE_START)
    {
        camFocusImpl.focus_status = CAM_FOCUS_MODE_START;
        camFocusImpl.focused_times = -1;
    }
    if (isDebugEnable(DEBUG_AF)) {
        HAL_LOGI("fwk_afMode%d fwk_afTrigger:%d focus:%d",
            fwk_afMode, fwk_afTrigger, camFocusImpl.focus_status);
    }


    return result;
}
int V4L2Camera::ProcessTestPattern(CameraMetadata *camera_meta_data, CameraStream *camera_stream)
{
  int res;
  if(camera_meta_data->exists(ANDROID_SENSOR_TEST_PATTERN_MODE)) {
     uint8_t test_pattern_mode = ANDROID_SENSOR_TEST_PATTERN_MODE_OFF;
     res = SingleTagValue(
       camera_meta_data, ANDROID_SENSOR_TEST_PATTERN_MODE, &test_pattern_mode);
     if (res) {
         HAL_LOGE("Failed to set ANDROID_SENSOR.");
     } else {
         int32_t test_pattern_data[4] = {0, 0, 0, 0};
         if (test_pattern_mode == ANDROID_SENSOR_TEST_PATTERN_MODE_BLACK) {
             res = camera_meta_data->update(ANDROID_SENSOR_TEST_PATTERN_DATA, test_pattern_data, 4);
         }
         camera_stream->setTestPattern(test_pattern_mode);
     }
  }
  return 0;
}
int V4L2Camera::ProcessFpsRanage(CameraMetadata *camera_meta_data, CameraStream *camera_stream)
{
    std::array<int32_t, 2> FpsRange = {15, 30};
    if (camera_meta_data->exists(ANDROID_CONTROL_AE_TARGET_FPS_RANGE)){
        SingleTagValue(camera_meta_data, ANDROID_CONTROL_AE_TARGET_FPS_RANGE, &FpsRange);
        int32_t min_fps = (int32_t) FpsRange[0];
        int32_t max_fps = (int32_t) FpsRange[1];
        camera_stream->setFpsRanage(min_fps);
        HAL_LOGE("min_fps:%d max_fps:%d", min_fps, max_fps);
    }
    return 0;
}
int V4L2Camera::ProcessCropRegion(CameraMetadata *camera_meta_data,CameraStream *camera_stream)
{
    int result = 0;
    std::array<int32_t, 4> CropRegion = {0, 0, 0, 0};
    if (camera_meta_data->exists(ANDROID_SCALER_CROP_REGION))
    {
        cam_crop_rect_t CameraCropRect;
        memset(&CameraCropRect,0,sizeof(cam_crop_rect_t));
        SingleTagValue(camera_meta_data, ANDROID_SCALER_CROP_REGION, &CropRegion);
        camCropRect.left   = CropRegion[0];
        camCropRect.top    = CropRegion[1];
        camCropRect.width  = CropRegion[2];
        camCropRect.height = CropRegion[3];
        if (isDebugEnable(DEBUG_ZOOM)) {
            HAL_LOGI("####ANDROID_SCALER_CROP_REGION left:%d top:%d width:%d height:%d",
                camCropRect.left,camCropRect.top,camCropRect.width,camCropRect.height);
        }
        CameraCropRect.left = camCropRect.left;
        CameraCropRect.top = camCropRect.top;
        CameraCropRect.width = camCropRect.width;
        CameraCropRect.height = camCropRect.height;

        result = camera_stream->setIspCrop(CameraCropRect);
        if (result) {
            HAL_LOGE("Fail to setIspCrop on mCameraStream!");
            return result;
        }

    }

    return result;
}

int V4L2Camera::enqueueRequest(
    std::shared_ptr<default_camera_hal::CaptureRequest> request) {
  HAL_LOG_ENTER();
  int res;

  // Be compatible with Camera api 1.
  mRunStreamNum = request->output_buffers.size();
  uint32_t frameNumber;
  {
    //std::lock_guard<std::mutex> guard(frameNumber_request_lock_);
    std::unique_lock<std::mutex> lock(frameNumber_request_lock_);
    frameNumber = request->frame_number;
    in_flight_buffer_count++;
    if(in_flight_buffer_count > MAX_BUFFER_NUM)
    {
        HAL_LOGW("####in_flight_buffer_count:%d > %d,wait!",
            in_flight_buffer_count,MAX_BUFFER_NUM);
        tbuffers_in_flight_.wait(lock);
    }
    mMapFrameNumRequest.emplace(frameNumber, request);
    rfequest_queue_stream_.push(request);
    mMetadata = &request->settings;
  }
  //mMetadata.reset(new CameraMetadata(request->settings));


  for(int i = 0; i <request->output_buffers.size(); i++) {
    res = mStreamManager_.get()->markFrameNumber(frameNumber);
    if (res) {
        HAL_LOGE("mark FrameNumber failed.%d", i);
        return -ENODEV;
    }
  }
  for(int i = 0; i <request->output_buffers.size(); i++) {
    std::lock_guard<std::mutex> guard(request_queue_stream_lock_);
    const camera3_stream_buffer_t& output = request->output_buffers[i];
    mCameraStream[i] = (CameraStream *)output.stream->priv;
    HAL_LOGV("####i:%d request %d x %d, format:%d. frameNumber:%d", i,output.stream->width, output.stream->height, output.stream->format,
        frameNumber);
    int setFlashFlag = 0;
    CameraFlashModeE CameraFlashMode = CAM_FLASH_MODE_MAX;
    if(mCameraConfig->supportFlashMode())
    {
        ProbeFlashMode(&CameraFlashMode, mMetadata, &setFlashFlag, frameNumber);
    }
    if (mCameraConfig->supportFrameRate())
    {
        ProcessFpsRanage(mMetadata, mCameraStream[i]);
    }
    ProcessTestPattern(mMetadata, mCameraStream[i]);
    res = mCameraStream[i]->request(output.buffer, frameNumber, mMetadata, mStreamManager_.get());
    if (res) {
        HAL_LOGE("Fail to request on mCameraStream");
    }

    if(mCameraConfig->supportZoom())
    {
        ProcessCropRegion(mMetadata,mCameraStream[i]);
    }

    if(setFlashFlag)
    {
        HAL_LOGV("####setIspFlash(CameraFlashMode:%d)",CameraFlashMode);
        res = mCameraStream[i]->setIspFlash(CameraFlashMode);
        if (res) {
            HAL_LOGE("Fail to setIspFlash on mCameraStream");
        }
    }
  }

  if(mCameraConfig->supportFocusMode())
  {
      ProcessFocusMode(mMetadata, frameNumber);
  }


  {
      std::unique_lock<std::mutex> lock(frameNumber_request_lock_);
      res = metadata_->SetRequestSettings(*mMetadata);
        if (res) {
          HAL_LOGE("Failed to set settings.");
        }

        res = metadata_->FillResultMetadata(mMetadata);
        if (res) {
          HAL_LOGE("Failed to fill result metadata.");
        }
        updateFlashState(mMetadata, frameNumber,  curFlashStateE);
        updateAfState(mMetadata, frameNumber);
  }
  //requests_available_stream_.notify_one();

  return 0;
}

std::shared_ptr<default_camera_hal::CaptureRequest>
V4L2Camera::dequeueRequest() {
  HAL_LOG_ENTER();

  // Wait.
  // 1. No request in stream.
  // 2. And exchange to record, record exchange to preview.
  //    a.exchange to record: In preview mode, so pbuffers must be PREPARE_BUFFER, until timeout that mean
  // in_flight_.size() less than 2 buffers.
  //    b.exchange to preview: In record mode, so vbuffers must be PREPARE_BUFFER, until timeout that mean
  // in_flight_.size() less than 2 buffers.
  // Jump out while wait.
  // 1. There are request in stream.
  // 2. Or exchange to record, record exchange to preview.
  //    a.exchange to record: In preview mode, so pbuffers must be PREPARE_BUFFER, until timeout that mean
  // in_flight_.size() less than 2 buffers.
  //    b.exchange to preview: In record mode, so vbuffers must be PREPARE_BUFFER, until timeout that mean
  // in_flight_.size() less than 2 buffers.
#if 0
  if(!(((mBeffer_status.pbuffers == PREPARE_BUFFER)||(mBeffer_status.vbuffers == PREPARE_BUFFER))
              &&(in_flight_.size() > 0)
              //&& isVideoByTemplate
              )) {
    while ( request_queue_pstream_.empty()
            && request_queue_tstream_.empty()
            && request_queue_vstream_.empty()
            ) {
      HAL_LOGD("Wait the requests_available_stream_ lock,in_flight_ buffer(s) size:%d.",in_flight_.size());
      requests_available_stream_.wait(lock);
      HAL_LOGD("The lock has been notified.");
    }
  }

  while ( request_queue_pstream_.empty()
          && request_queue_tstream_.empty()
          && request_queue_vstream_.empty()
          ) {
    HAL_LOGD("Wait the requests_available_stream_ lock,in_flight_ buffer(s) size:%d.",in_flight_.size());
    requests_available_stream_.wait(lock);
    HAL_LOGD("The lock has been notified.");
  }
#endif

  HAL_LOGD("The request queue will be pop,there are leave %d in prequest, %d in trequest, %d in vrequest,\
  %d in vhrequest now!",
        request_queue_pstream_.size(),
        request_queue_tstream_.size(),
        request_queue_vstream_.size(),
        request_queue_vhstream_.size());

  std::shared_ptr<default_camera_hal::CaptureRequest> request = nullptr;

  while(1) {
    //{
      std::unique_lock<std::mutex> lock(request_queue_stream_lock_);
      // Only pick one stream.
      if(!request_queue_tstream_.empty()) {
        request = request_queue_tstream_.front();
        request_queue_tstream_.pop();
        break;

      } else if(!request_queue_vstream_.empty() && mVideoflag) {
        request = request_queue_vstream_.front();
        request_queue_vstream_.pop();
        break;

      } else if(!request_queue_pstream_.empty() && ((mVideoflag == false)||mRunStreamNum == 1)) {
        // Think about the stream has only one cache for using, add detect stream number for preview in video mode.
        request = request_queue_pstream_.front();
        request_queue_pstream_.pop();
        break;
      }
    //}
    HAL_LOGD("Wait the requests_available_stream_ lock,in_flight_ buffer(s) size:%d.",in_flight_.size());
    requests_available_stream_.wait(lock);
    HAL_LOGD("The lock has been notified.");

  }

  HAL_LOGD("Stream queue has been pop,there are leave %d in prequest, %d in trequest, %d in vrequest,\
  %d in vhrequest now!",
        request_queue_pstream_.size(),
        request_queue_tstream_.size(),
        request_queue_vstream_.size(),
        request_queue_vhstream_.size());

  return request;
}
void V4L2Camera::ReadyForBuffer() {
  // Wait until something is available before looping again.
  std::unique_lock<std::mutex> lock(in_flight_lock_);
  while(!(((mBeffer_status.pbuffers == PREPARE_BUFFER)||(mBeffer_status.vbuffers == PREPARE_BUFFER))
              &&(in_flight_.size() > 0)
              //&& isVideoByTemplate
              )) {

    HAL_LOGD("Wait the buffers_in_flight_ lock,in_flight_ buffer(s) size:%d.",in_flight_.size());
    buffers_in_flight_.wait(lock);
    HAL_LOGD("The lock has been notified.");
  }
}

bool V4L2Camera::validateDataspacesAndRotations(
    const camera3_stream_configuration_t* stream_config) {
  HAL_LOG_ENTER();

  for (uint32_t i = 0; i < stream_config->num_streams; ++i) {
    if (stream_config->streams[i]->rotation != CAMERA3_STREAM_ROTATION_0) {
      HAL_LOGV("Rotation %d for stream %d not supported",
               stream_config->streams[i]->rotation,
               i);
      return false;
    }
    // Accept all dataspaces, as it will just be overwritten below anyways.
  }
  return true;
}

int V4L2Camera::sResultCallback(uint32_t frameNumber,struct timeval ts) {
  //std::lock_guard<std::mutex> guard(frameNumber_request_lock_);
  std::unique_lock<std::mutex> lock(frameNumber_request_lock_);
  int res = 0;
  int ret = 0;
  std::shared_ptr<default_camera_hal::CaptureRequest> request = rfequest_queue_stream_.front();

  auto map_entry = mMapFrameNumRequest.find(frameNumber);
  if (map_entry == mMapFrameNumRequest.end()) {
    HAL_LOGE("No matching request for frameNumber:%d, something wrong!", frameNumber);
    return -ENOMEM;
  } else {
    if(request->frame_number != frameNumber) {
      HAL_LOGE("No the successfully front request frameNumber:%d for result frameNumber:%d, something wrong!", request->frame_number, frameNumber);
      wfequest_queue_stream_.push(map_entry->second);
    } else {
      // Fix cts case: android.hardware.camera2.cts.StillCaptureTest#testJpegExif
      // We update the mThumbnailSize and sort for avoid framework sort the metadata
      // so mThumbnailSize changed.
      std::array<int32_t, 2> mThumbnailSize = {-1, -1};
      ret = SingleTagValue(&map_entry->second->settings,
                              ANDROID_JPEG_THUMBNAIL_SIZE,
                              &mThumbnailSize);
      (&map_entry->second->settings)->sort();
      HAL_LOGD("Before completeRequest mThumbnailSize info:mThumbnailSize:%dx%d.", mThumbnailSize[0], mThumbnailSize[1]);
      if((mThumbnailSize[0] == 0)&&(mThumbnailSize[1] == 0)) {
        int mThumbnailSizet[2] = {0, 0};
        ret = (&map_entry->second->settings)->update(ANDROID_JPEG_THUMBNAIL_SIZE, mThumbnailSizet, 2);
        if (ret) {
          HAL_LOGE("Failed to update metadata tag 0x%x", ANDROID_JPEG_THUMBNAIL_SIZE);
        }
      }
      int64_t timestamp = 0;
      timestamp = ts.tv_sec * 1000000000ULL + ts.tv_usec*1000;
      ret = (&map_entry->second->settings)->update(ANDROID_SENSOR_TIMESTAMP, &timestamp, 1);
      if (ret) {
          HAL_LOGE("Failed to update metadata tag 0x%x", ANDROID_SENSOR_TIMESTAMP);
      }

      if (isDebugEnable(DEBUG_TIMESTAMP)) {
        HAL_LOGI("timestamp:%lld frameNumber:%d", timestamp, frameNumber);
      }
      in_flight_buffer_count--;
      tbuffers_in_flight_.notify_one();

      completeRequest(map_entry->second, res);
      rfequest_queue_stream_.pop();
      while(!wfequest_queue_stream_.empty()) {
          if (rfequest_queue_stream_.front()->frame_number == wfequest_queue_stream_.front()->frame_number){
              completeRequest(wfequest_queue_stream_.front(), res);
              wfequest_queue_stream_.pop();
              rfequest_queue_stream_.pop();
              in_flight_buffer_count--;
              tbuffers_in_flight_.notify_one();

          } else {
              HAL_LOGE("the front frameNumber in rfequest_queue(%d) != the wfequest_queue(%d), so keep",
                   rfequest_queue_stream_.front()->frame_number,wfequest_queue_stream_.front()->frame_number);
              break;
          }
      }
    }
    mMapFrameNumRequest.erase(frameNumber);
  }
  return res;
}

int V4L2Camera::setupStreams(camera3_stream_configuration_t* stream_config) {
  HAL_LOG_ENTER();
  int res = 0;
  HAL_LOGE("bill setupStreams begin");
  // We should flush before change the situation.
  //flush();
  // stream_config should have been validated; assume at least 1 stream.
  if(stream_config->num_streams < 1) {
    HAL_LOGE("Validate the stream numbers, at least 1 stream.");
    return -EINVAL;
  }
  // stream_config should have been validated; do not over MAX_STREAM_NUM.
  if(stream_config->num_streams > MAX_STREAM_NUM) {
    HAL_LOGE("Validate the stream numbers, over the max stream number %d we support.", MAX_STREAM_NUM);
    return -EINVAL;
  }

  //std::lock_guard<std::mutex> guard(stream_lock_);
  // stop stream.
  if(mStreamManager_ != nullptr) {
    HAL_LOGD("Stop stream.");
    for(int ss = 0; ss < MAX_STREAM; ss++) {
      mStreamManager_->stop((STREAM_SERIAL)ss);
    }
  }

  while(!wfequest_queue_stream_.empty()) {
    wfequest_queue_stream_.pop();
  }
  while(!rfequest_queue_stream_.empty()) {
    rfequest_queue_stream_.pop();
  }
  mMapFrameNumRequest.clear();
  mStreamManager_.reset();
  mStreamManager_ = StreamManager::NewStreamManager(device_, instance);

  if(mCameraConfig->supportFlashMode())
  {
      std::lock_guard<std::mutex> guard(camera_fd_lock_);
      if (camera_fd != -1)
      {
          int ret_close = 0;
          ret_close = ::close(camera_fd);
          HAL_LOGD("mCameraFd:%d, ret_close: %d, errno:%s",camera_fd,ret_close,strerror(errno));
          camera_fd = -1;
      }
  }

  int numStreamsSet = 0;
  int retIndex = -1;
  int mainIndex = -1;
  int subIndex = -1;
  int thirdIndex = -1;

  for(int j = 0; j <MAX_STREAM; j++) {
    mStreamTracker[j] = false;
  }
  for(int j = 0; j <MAX_STREAM; j++) {
    mSourceStreamTracker[j] = false;
  }

  // Analysis and create stream.
  for (uint32_t i = 0; i < stream_config->num_streams; ++i) {

    int isBlob = 0;
    retIndex = findStreamModes(MAIN_STREAM, stream_config, &isBlob);
    if((retIndex >= 0) && (!mStreamTracker[MAIN_STREAM +isBlob])) {
      HAL_LOGD("Detect the main stream %d is format %d, width %d, height %d, usage %d, stream_type %d, data_space %d, num_streams:%d.",
            retIndex,
            stream_config->streams[retIndex]->format,
            stream_config->streams[retIndex]->width,
            stream_config->streams[retIndex]->height,
            stream_config->streams[retIndex]->usage,
            stream_config->streams[retIndex]->stream_type,
            stream_config->streams[retIndex]->data_space,
            stream_config->num_streams);
      mainIndex = retIndex;

      stream_config->streams[retIndex]->priv =
        reinterpret_cast<void *> (mStreamManager_->createStream(MAIN_STREAM,
        stream_config->streams[retIndex]->width,
        stream_config->streams[retIndex]->height,
        stream_config->streams[retIndex]->format,
        stream_config->streams[retIndex]->usage,
        isBlob));
      if(nullptr == stream_config->streams[retIndex]->priv) {
        HAL_LOGE("Failed create main stream!");
        return -EINVAL;
      }
      mSourceStreamTracker[retIndex] = true;
      mStreamTracker[MAIN_STREAM +isBlob] = true;
      numStreamsSet++;
      continue;
    }
    isBlob = 0;
    retIndex = findStreamModes(SUB_0_STREAM, stream_config, &isBlob);
    if((retIndex >= 0) && (!mStreamTracker[SUB_0_STREAM +isBlob])) {
      HAL_LOGD("Detect the sub stream %d is format %d, width %d, height %d, usage %d, stream_type %d, data_space %d, num_streams:%d.",
            retIndex,
            stream_config->streams[retIndex]->format,
            stream_config->streams[retIndex]->width,
            stream_config->streams[retIndex]->height,
            stream_config->streams[retIndex]->usage,
            stream_config->streams[retIndex]->stream_type,
            stream_config->streams[retIndex]->data_space,
            stream_config->num_streams);

      subIndex = retIndex;

      stream_config->streams[retIndex]->priv =
        reinterpret_cast<void *> (mStreamManager_->createStream(SUB_0_STREAM,
        stream_config->streams[retIndex]->width,
        stream_config->streams[retIndex]->height,
        stream_config->streams[retIndex]->format,
        stream_config->streams[retIndex]->usage,
        isBlob));
      if(nullptr == stream_config->streams[retIndex]->priv) {
        HAL_LOGE("Failed create sub stream!");
        return -EINVAL;
      }
      mSourceStreamTracker[retIndex] = true;
      mStreamTracker[SUB_0_STREAM +isBlob] = true;
      numStreamsSet++;
      continue;
    }

    // For third stream
    isBlob = 0;
    //  detect source stream
    for(int k = 0; k <stream_config->num_streams; k++) {
      if(mSourceStreamTracker[k] == false) {
        thirdIndex = k;
        if(stream_config->streams[k]->format == HAL_PIXEL_FORMAT_BLOB) {
          isBlob = 1;
        }
      }
    }

    // We can not deal the blob stream when input source/output source large than 5.
    if((numStreamsSet != stream_config->num_streams)
      && (((stream_config->streams[subIndex]->width/stream_config->streams[thirdIndex]->width) < 5)
          &&((stream_config->streams[subIndex]->height/stream_config->streams[thirdIndex]->height) < 5))
      ){
      // For third blob stream
      for(int j = SUB_0_STREAM_BLOB; j >0; ) {
        HAL_LOGD("In j:%d circle!", j);
        if((!mStreamTracker[j]) && isBlob){
          HAL_LOGD("Find j+isBlob:%d false!", j);
          HAL_LOGD("Detect the third blob stream %d link to %d stream is format %d, width %d, height %d, num_streams:%d.",
                thirdIndex,
                j,
                stream_config->streams[thirdIndex]->format,
                stream_config->streams[thirdIndex]->width,
                stream_config->streams[thirdIndex]->height,
                stream_config->num_streams);

          stream_config->streams[thirdIndex]->priv =
            reinterpret_cast<void *> (mStreamManager_->createStream((STREAM_SERIAL)(j -1),
            stream_config->streams[thirdIndex]->width,
            stream_config->streams[thirdIndex]->height,
            stream_config->streams[thirdIndex]->format,
            stream_config->streams[thirdIndex]->usage,
            isBlob));

          if(nullptr == stream_config->streams[thirdIndex]->priv) {
            HAL_LOGE("Failed create third stream!");
            return -EINVAL;
          }
          mSourceStreamTracker[thirdIndex] = true;
          mStreamTracker[j] = true;
          numStreamsSet++;
          break;
        }

        if(numStreamsSet == stream_config->num_streams) {
          break;
        }
        j = j -2;
      }
    }

    //  find mirror stream
    if(numStreamsSet != stream_config->num_streams) {
      if (!mStreamTracker[SUB_0_MIRROR_STREAM]) {
        HAL_LOGD("Find SUB_0_MIRROR_STREAM:%d!", SUB_0_MIRROR_STREAM+isBlob);
        HAL_LOGD("Detect the third mirror stream %d link to %d stream is format %d, width %d, height %d, num_streams:%d.",
              thirdIndex,
              SUB_0_MIRROR_STREAM +isBlob,
              stream_config->streams[thirdIndex]->format,
              stream_config->streams[thirdIndex]->width,
              stream_config->streams[thirdIndex]->height,
              stream_config->num_streams);

        stream_config->streams[thirdIndex]->priv =
          reinterpret_cast<void *> (mStreamManager_->createStream((STREAM_SERIAL)SUB_0_MIRROR_STREAM,
          stream_config->streams[thirdIndex]->width,
          stream_config->streams[thirdIndex]->height,
          stream_config->streams[thirdIndex]->format,
          stream_config->streams[thirdIndex]->usage,
          isBlob));
        if((stream_config->streams[subIndex]->width != stream_config->streams[thirdIndex]->width) ||
            (stream_config->streams[subIndex]->height != stream_config->streams[thirdIndex]->height) ||
            mCameraConfig->supportInterpolationSize()) {
          res = ((CameraSubMirrorStream *)(stream_config->streams[thirdIndex]->priv))->setScaleFlag();
          if(res) {
            HAL_LOGE("Failed setScaleFlag!");
          }
        }
        if(nullptr == stream_config->streams[thirdIndex]->priv) {
          HAL_LOGE("Failed create third stream!");
          return -EINVAL;
        }
        mSourceStreamTracker[thirdIndex] = true;
        mStreamTracker[SUB_0_MIRROR_STREAM +isBlob] = true;
        numStreamsSet++;

      }
    }
#if 0
    if(numStreamsSet != stream_config->num_streams) {
      // For third stream
      if (!mStreamTracker[SUB_0_MIRROR_STREAM] &&
        (stream_config->streams[subIndex]->width == stream_config->streams[thirdIndex]->width) &&
        (stream_config->streams[subIndex]->height == stream_config->streams[thirdIndex]->height)) {
        HAL_LOGD("Find MAIN_MIRROR_STREAM:%d!", SUB_0_MIRROR_STREAM+isBlob);
        HAL_LOGD("Detect the third mirror stream %d link to %d stream is format %d, width %d, height %d, num_streams:%d.",
              thirdIndex,
              SUB_0_MIRROR_STREAM +isBlob,
              stream_config->streams[thirdIndex]->format,
              stream_config->streams[thirdIndex]->width,
              stream_config->streams[thirdIndex]->height,
              stream_config->num_streams);

        stream_config->streams[thirdIndex]->priv =
          reinterpret_cast<void *> (mStreamManager_->createStream((STREAM_SERIAL)SUB_0_MIRROR_STREAM,
          stream_config->streams[thirdIndex]->width,
          stream_config->streams[thirdIndex]->height,
          stream_config->streams[thirdIndex]->format,
          stream_config->streams[thirdIndex]->usage,
          isBlob));

        if(nullptr == stream_config->streams[thirdIndex]->priv) {
          HAL_LOGE("Failed create third stream!");
          return -EINVAL;
        }
        mSourceStreamTracker[thirdIndex] = true;
        mStreamTracker[SUB_0_MIRROR_STREAM +isBlob] = true;
        numStreamsSet++;

      }else if(!mStreamTracker[MAIN_MIRROR_STREAM] &&
        (stream_config->streams[mainIndex]->width == stream_config->streams[thirdIndex]->width) &&
        (stream_config->streams[mainIndex]->height == stream_config->streams[thirdIndex]->height)){
        HAL_LOGD("Find MAIN_MIRROR_STREAM:%d!", MAIN_MIRROR_STREAM+isBlob);
        HAL_LOGD("Detect the third mirror stream %d link to %d stream is format %d, width %d, height %d, num_streams:%d.",
              thirdIndex,
              MAIN_MIRROR_STREAM +isBlob,
              stream_config->streams[thirdIndex]->format,
              stream_config->streams[thirdIndex]->width,
              stream_config->streams[thirdIndex]->height,
              stream_config->num_streams);

        stream_config->streams[thirdIndex]->priv =
          reinterpret_cast<void *> (mStreamManager_->createStream((STREAM_SERIAL)MAIN_MIRROR_STREAM,
          stream_config->streams[thirdIndex]->width,
          stream_config->streams[thirdIndex]->height,
          stream_config->streams[thirdIndex]->format,
          stream_config->streams[thirdIndex]->usage,
          isBlob));

        if(nullptr == stream_config->streams[thirdIndex]->priv) {
          HAL_LOGE("Failed create third stream!");
          return -EINVAL;
        }
        mSourceStreamTracker[thirdIndex] = true;
        mStreamTracker[MAIN_MIRROR_STREAM +isBlob] = true;
        numStreamsSet++;
      }
    }
#endif

  }

  HAL_LOGD("Configurate stream Manager.");
  if(mStreamManager_ != nullptr) {
    for(int ss = 0; ss < MAX_STREAM; ss++) {
      res = mStreamManager_->configurateManager((STREAM_SERIAL)ss);
      if(res) {
        HAL_LOGE("Configurate stream Manager failed.");
      }
    }
  }

  camFocusImpl.focus_frame_num = -1;
  camFocusImpl.focused_times = -1;
  camFocusImpl.update_status = -1;
  camFocusImpl.status_flag = -1;
  camFocusImpl.focus_status = -1;
  camFocusImpl.update_status_times = -1;
  camFocusImpl.camera_focus_mode = CAM_FOCUS_MODE_MAX;
  camFocusImpl.camera_focus_trigger = CAM_FOCUS_TRIGGER_MAX;
  memset(&camAreaRoi,0,sizeof(cam_area_t));
  memset(&camCropRect,0,sizeof(cam_crop_rect_t));

  HAL_LOGD("Start stream.");
  // start stream.
  if(mStreamManager_ != nullptr) {
    for(int ss = 0; ss < MAX_STREAM; ss++) {
      mStreamManager_->start((STREAM_SERIAL)ss);
    }
  }

  // Fill some stream info: stream->usage, stream->max_buffers, stream->data_space.
  for (uint32_t i = 0; i < stream_config->num_streams; ++i) {
    fillStreamInfo(stream_config->streams[i]);
  }
  for (uint32_t i = 0; i < stream_config->num_streams; ++i) {
    HAL_LOGI("stream %d is format %d, width %d, height %d, usage %d, stream_type %d, data_space %d, num_streams:%d.",
          i,
          stream_config->streams[i]->format,
          stream_config->streams[i]->width,
          stream_config->streams[i]->height,
          stream_config->streams[i]->usage,
          stream_config->streams[i]->stream_type,
          stream_config->streams[i]->data_space,
          stream_config->num_streams);
  }

  HAL_LOGE("bill setupStreams finished");
  return 0;
}

bool V4L2Camera::isValidRequestSettings(
    const CameraMetadata& settings) {
  if (!metadata_->IsValidRequest(settings)) {
    HAL_LOGE("Invalid request settings.");
    return false;
  }
  return true;
}

}  // namespace v4l2_camera_hal
