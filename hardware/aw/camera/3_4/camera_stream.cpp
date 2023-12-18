

#undef NDEBUG
#if DBG_STREAM_MANAGER

#endif
#define LOG_TAG "CameraHALv3_CameraStream"

#include <android/log.h>

#include "stream_manager.h"
#include "camera_stream.h"
#include "metadata/metadata_common.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#include <cstdlib>
#include <string.h>

#include <sunxi_camera_v2.h>
#include "CameraMetadata.h"
#include <hardware/camera3.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))


namespace v4l2_camera_hal {

CameraStream* CameraStream::NewCameraStream(std::shared_ptr<V4L2Stream> stream,
                                                  int isBlob, int isMirror) {
  HAL_LOG_ENTER();

  STREAM_SERIAL ss_tmp = stream->getStreamSerial();
  HAL_LOGD("Device getStreamSerial %d, isBlob:%d, isMirror:%d.", ss_tmp, isBlob, isMirror);

  if(isMirror) {
    if(ss_tmp == MAIN_STREAM) {
      return new CameraMainMirrorStream(stream, isBlob);
    } else if (ss_tmp == SUB_0_STREAM){
      return new CameraSubMirrorStream(stream, isBlob);
    } else {
      return nullptr;
    }
  } else {
    if(ss_tmp == MAIN_STREAM) {
      return new CameraMainStream(stream, isBlob);
    } else if (ss_tmp == SUB_0_STREAM){
      return new CameraSubStream(stream, isBlob);
    } else {
      return nullptr;
    }
  }
}
CameraStream::CameraStream(std::shared_ptr<V4L2Stream> stream, int isBlob):
                                    stream_(stream),
                                    isBlobFlag(isBlob){
  HAL_LOG_ENTER();
  //connection_.reset(new V4L2Stream::Connection(stream_));
  manager_ = nullptr;
  mMetadata = nullptr;
  mStreamOn = false;
  initialized = false;
  mWidth = 0;
  mHeight = 0;
  mFormat = 0;
  mUsage = 0;

}

int CameraStream::configurateManager(StreamManager* manager){
  HAL_LOG_ENTER();
  int res = 0;
  manager_ = manager;
  return res;
}

CameraStream::~CameraStream(){
  HAL_LOG_ENTER();
}

CameraMainStream::CameraMainStream(std::shared_ptr<V4L2Stream> stream, int isBlob):
    CameraStream(stream, isBlob){
  HAL_LOG_ENTER();
}
CameraMainStream::~CameraMainStream(){
  HAL_LOG_ENTER();
  std::unique_lock<std::mutex> lock(main_yuv_buffer_queue_lock_);
}

int CameraMainStream::start(){
  HAL_LOG_ENTER();

  int res = 0;
  if(initialized) {
    // Stream on first.
    if(!mStreamOn) {
      res = stream_->StreamOn();
      if (res) {
        HAL_LOGE("Device failed to turn on stream.");
        return res;
      }
      mStreamOn = true;
      //manager_->start(stream_->getStreamSerial());
    }
  }
  return res;
}
int CameraMainStream::stop(){
  HAL_LOG_ENTER();

  int res = 0;
  if(initialized) {
    // Stream on first.
    if(!mStreamOn) {
      HAL_LOGE("Device failed to turn off stream, turn on first.");
    } else {
      res = stream_->StreamOff();
      if (res) {
        HAL_LOGE("Device failed to turn off stream.");
        return res;
      }
      mStreamOn = false;
    }
  }
  return res;
}

int CameraMainStream::flush(){
  HAL_LOG_ENTER();

  int res = 0;
  if(initialized) {
    // Stream on first.
    if(!mStreamOn) {
      HAL_LOGE("Device failed to flush stream, turn on first.");
    } else {
      res = stream_->flush();
      if (res) {
        HAL_LOGE("Device failed to flush stream.");
        return res;
      }
    }
  }
  return res;
}

int CameraMainStream::initialize(uint32_t width, uint32_t height, int format, uint32_t usage) {
  HAL_LOG_ENTER();

  int res = -1;
  HAL_LOGD("CameraMainStream:%d x %d, format:%d, usage:%d.", width, height, format, usage);

  res = stream_->SetParm(V4L2_MODE_VIDEO);
  if (res) {
    HAL_LOGE("Failed to SetParm.");
  }

  uint32_t max_buffers = MAX_BUFFER_NUM;
  if(format == HAL_PIXEL_FORMAT_BLOB) {
    format = HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED;
  }
  StreamFormat stream_format(format, width, height);
  res = stream_->SetFormat(stream_format, &max_buffers);
  if (res) {
    HAL_LOGE("Failed to set stream to correct format for stream: %d.", res);
    //return -ENODEV;
  }

  res = stream_->PrepareBuffer();
  if (res) {
    HAL_LOGE("Failed to prepare buffer.");
    return -ENODEV;
  }
  initialized = true;
  // Stream on first.
  if(!mStreamOn) {
    res = stream_->StreamOn();
    if (res) {
      HAL_LOGE("Device failed to turn on stream.");
      return res;
    }
    mStreamOn = true;
    //manager_->start(stream_->getStreamSerial());
  }

  return res;
}
int CameraMainStream::setFormat(uint32_t width, uint32_t height, int format, uint32_t usage) {
  HAL_LOG_ENTER();

  int res = -1;
  HAL_LOGD("CameraMainStream:%d x %d, format:%d, usage:%d.", width, height, format, usage);

  mWidth = width;
  mHeight = height;
  mFormat = format;
  mUsage = usage;
  res = 0;
  return res;
}

int CameraMainStream::waitBuffer(buffer_handle_t * buffer, uint32_t frameNumber) {

  int res = -1;
  //HAL_LOGD("Failed to prepare buffer.");
  while(main_frameNumber_buffers_map_queue_.empty()){
    std::unique_lock<std::mutex> lock(main_yuv_buffer_queue_lock_);
    HAL_LOGD("Wait the main_yuv_buffer_queue_lock_ lock.");
    main_yuv_buffer_availabl_queue_.wait(lock);
    HAL_LOGD("The lock has been notified.");
    res = 0;
  }

  return res;
}

int CameraMainStream::getBuffer(buffer_handle_t ** buffer, uint32_t* frameNumber) {

  int res = -ENOMEM;
  HAL_LOGD("main_frameNumber_buffers_map_queue_ has %d buffer(s).", main_frameNumber_buffers_map_queue_.size());
  HAL_LOGD("main_blob_frameNumber_buffers_map_queue_ has %d buffer(s).", main_blob_frameNumber_buffers_map_queue_.size());
  if(main_frameNumber_buffers_map_queue_.size() && !isBlobFlag){
    std::unique_lock<std::mutex> lock(main_yuv_buffer_queue_lock_);
    frame_bufferHandle_map_t tmp = main_frameNumber_buffers_map_queue_.front();;
    *frameNumber = tmp.frameNum;
    *buffer = tmp.bufferHandleptr;
    main_frameNumber_buffers_map_queue_.pop();
    HAL_LOGD("main_frameNumber_buffers_map_queue_ buffer_handle_t*:%p, FrameNumber:%d poped.", *buffer, *frameNumber);
    res = 0;
  } else if(main_blob_frameNumber_buffers_map_queue_.size()) {
    std::unique_lock<std::mutex> lock(main_blob_buffer_queue_lock_);
    frame_bufferHandle_map_t tmp = main_blob_frameNumber_buffers_map_queue_.front();;
    *frameNumber = tmp.frameNum;
    *buffer = tmp.bufferHandleptr;
    main_blob_frameNumber_buffers_map_queue_.pop();
    HAL_LOGD("main_blob_frameNumber_buffers_map_queue_ buffer_handle_t*:%p, FrameNumber:%d poped.", *buffer, *frameNumber);
    res = 0;
  }

  return res;
}
#if 0
int CameraMainStream::encodebuffer(void * dst_addr, void * src_addr, unsigned long mJpegBufferSizes) {

  int res = -ENOMEM;
  //HAL_LOGD("Failed to prepare buffer.");
  JPEG_ENC_t jpeg_enc;
  memset(&jpeg_enc, 0, sizeof(jpeg_enc));

  std::array<uint8_t, 33> mGpsMethod = {0};
  res = SingleTagValue(mMetadata,
                          ANDROID_JPEG_GPS_PROCESSING_METHOD,
                          &mGpsMethod);
  HAL_LOGD("jpeg info:mGpsMethod:%s.", (unsigned char *)(&(mGpsMethod[0])));

  if (0 != mGpsMethod[0])
  {
      std::array<double, 3> gpsCoordinates = {0,0,0};
      res = SingleTagValue(mMetadata,
                              ANDROID_JPEG_GPS_COORDINATES,
                              &gpsCoordinates);
      HAL_LOGD("jpeg info:latitude:%f, longitude:%f, altitude:%f.", gpsCoordinates[0], gpsCoordinates[1], gpsCoordinates[2]);
      int64 mGpsTimestamp = 0;
      res = SingleTagValue(mMetadata,
                              ANDROID_JPEG_GPS_TIMESTAMP,
                              &mGpsTimestamp);
      HAL_LOGD("jpeg info:mGpsTimestamp:%d.", mGpsTimestamp);

      jpeg_enc.enable_gps            = 1;
      jpeg_enc.gps_latitude        = gpsCoordinates[0];
      jpeg_enc.gps_longitude        = gpsCoordinates[1];
      jpeg_enc.gps_altitude        = gpsCoordinates[2];
      jpeg_enc.gps_timestamp        = mGpsTimestamp;
      for(int i = 0; i< 33; i++) {
        jpeg_enc.gps_processing_method[i] = mGpsMethod[i];
      }
      //strcpy(jpeg_enc.gps_processing_method, mGpsMethod);
      //memset(mGpsMethod, 0, sizeof(mGpsMethod));
  }
  else
  {
      jpeg_enc.enable_gps            = 0;
  }

  int32 mOrientation = 0;
  res = SingleTagValue(mMetadata,
                          ANDROID_JPEG_ORIENTATION,
                          &mOrientation);
  HAL_LOGD("jpeg info:mOrientation:%d.", mOrientation);

  byte mQuality = 90;
  res = SingleTagValue(mMetadata,
                          ANDROID_JPEG_QUALITY,
                          &mQuality);
  HAL_LOGD("jpeg info:mQuality:%d.", mQuality);
  jpeg_enc.quality = mQuality;

  byte mThumbnailQuality = 90;
  res = SingleTagValue(mMetadata,
                          ANDROID_JPEG_THUMBNAIL_QUALITY,
                          &mThumbnailQuality);
  HAL_LOGD("jpeg info:mThumbnailQuality:%d.", mQuality);

  std::array<int32, 2> mThumbnailSize = {320,240};
  res = SingleTagValue(mMetadata,
                          ANDROID_JPEG_THUMBNAIL_SIZE,
                          &mThumbnailSize);
  HAL_LOGD("jpeg info:mThumbnailSize:%dx%d.", mThumbnailSize[0], mThumbnailSize[1]);

  jpeg_enc.pic_w            = mWidth;
  jpeg_enc.pic_h            = mHeight;

  jpeg_enc.thumbWidth        = mThumbnailSize[0];
  jpeg_enc.thumbHeight    = mThumbnailSize[1];

  res = stream_->EncodeBuffer(dst_addr, src_addr, mJpegBufferSizes, jpeg_enc);
  if (res) {
    HAL_LOGE("Device EncodeBuffer failed.");
  }
  return res;

}
#endif
int CameraMainStream::encodebuffer(void * dst_addr, void *src_addr, unsigned long mJpegBufferSizes) {

  int res = -ENOMEM;
  //HAL_LOGD("Failed to prepare buffer.");
  JPEG_ENC_t jpeg_enc;
  memset(&jpeg_enc, 0, sizeof(jpeg_enc));

  std::array<uint8_t, 33> mGpsMethod = {0};
  res = SingleTagValue(mMetadata,
                          ANDROID_JPEG_GPS_PROCESSING_METHOD,
                          &mGpsMethod);
  HAL_LOGD("jpeg info:mGpsMethod:%s.", (unsigned char *)(&(mGpsMethod[0])));

  if (0 != mGpsMethod[0])
  {
      std::array<double, 3> gpsCoordinates = {0,0,0};
      res = SingleTagValue(mMetadata,
                              ANDROID_JPEG_GPS_COORDINATES,
                              &gpsCoordinates);
      HAL_LOGD("jpeg info:latitude:%f, longitude:%f, altitude:%f.", gpsCoordinates[0], gpsCoordinates[1], gpsCoordinates[2]);
      int64 mGpsTimestamp = 0;
      res = SingleTagValue(mMetadata,
                              ANDROID_JPEG_GPS_TIMESTAMP,
                              &mGpsTimestamp);
      HAL_LOGD("jpeg info:mGpsTimestamp:%d.", mGpsTimestamp);

      jpeg_enc.enable_gps            = 1;
      jpeg_enc.gps_latitude        = gpsCoordinates[0];
      jpeg_enc.gps_longitude        = gpsCoordinates[1];
      jpeg_enc.gps_altitude        = gpsCoordinates[2];
      jpeg_enc.gps_timestamp        = mGpsTimestamp;
      for(int i = 0; i< 33; i++) {
        jpeg_enc.gps_processing_method[i] = mGpsMethod[i];
      }
      //strcpy(jpeg_enc.gps_processing_method, mGpsMethod);
      //memset(mGpsMethod, 0, sizeof(mGpsMethod));
  }
  else
  {
      jpeg_enc.enable_gps            = 0;
  }

  int32 mOrientation = 0;
  res = SingleTagValue(mMetadata,
                          ANDROID_JPEG_ORIENTATION,
                          &mOrientation);
  HAL_LOGD("jpeg info:mOrientation:%d.", mOrientation);
  jpeg_enc.rotate = mOrientation;

  byte mQuality = 90;
  res = SingleTagValue(mMetadata,
                          ANDROID_JPEG_QUALITY,
                          &mQuality);
  HAL_LOGD("jpeg info:mQuality:%d.", mQuality);
  jpeg_enc.quality = mQuality;

  byte mThumbnailQuality = 90;
  res = SingleTagValue(mMetadata,
                          ANDROID_JPEG_THUMBNAIL_QUALITY,
                          &mThumbnailQuality);
  HAL_LOGD("jpeg info:mThumbnailQuality:%d.", mQuality);

  std::array<int32, 2> mThumbnailSize = {320,240};
  res = SingleTagValue(mMetadata,
                          ANDROID_JPEG_THUMBNAIL_SIZE,
                          &mThumbnailSize);
  HAL_LOGD("jpeg info:mThumbnailSize:%dx%d.", mThumbnailSize[0], mThumbnailSize[1]);

  jpeg_enc.pic_w            = mWidth;
  jpeg_enc.pic_h            = mHeight;

  jpeg_enc.thumbWidth        = mThumbnailSize[0];
  jpeg_enc.thumbHeight    = mThumbnailSize[1];

  res = stream_->EncodeBuffer(dst_addr, src_addr, mJpegBufferSizes, jpeg_enc);
  if (res) {
    HAL_LOGE("Device EncodeBuffer failed.");
  }
  return res;

}

int CameraMainStream::copybuffer(void * dst_addr, void * src_addr) {

  int res = -ENOMEM;
  //HAL_LOGD("Failed to prepare buffer.");
  res = stream_->CopyBuffer(dst_addr, src_addr, mUsage);
  if (res) {
    HAL_LOGE("Device CopyBuffer failed.");
  }
  return res;
}

int CameraMainStream::request(buffer_handle_t * buffer, uint32_t frameNumber, CameraMetadata* metadata, StreamManager* mManager) {
  HAL_LOG_ENTER();

  int res = 0;
  //HAL_LOGD("Failed to prepare buffer.");
  mMetadata = metadata;


  if(!isBlobFlag){
    std::lock_guard<std::mutex> guard(main_yuv_buffer_queue_lock_);
    frame_bufferHandle_map_t tmp;
    tmp.bufferHandleptr = buffer;
    tmp.frameNum = frameNumber;
    main_frameNumber_buffers_map_queue_.push(tmp);
    HAL_LOGD("main_frameNumber_buffers_map_queue_ buffer_handle_t*:%p, FrameNumber:%d pushed.", buffer, frameNumber);
  } else {
    std::lock_guard<std::mutex> guard(main_blob_buffer_queue_lock_);
    frame_bufferHandle_map_t tmp;
    tmp.bufferHandleptr = buffer;
    tmp.frameNum = frameNumber;
    main_blob_frameNumber_buffers_map_queue_.push(tmp);
    HAL_LOGD("main_blob_frameNumber_buffers_map_queue_ buffer_handle_t*:%p, FrameNumber:%d pushed.", buffer, frameNumber);
  }


  if(!mStreamOn) {
    res = stream_->StreamOn();
    if (res) {
      HAL_LOGE("Device failed to turn on stream.");
      return res;
    }
    mStreamOn = true;
    manager_->start(stream_->getStreamSerial());
  }
  //main_yuv_buffer_availabl_queue_.notify_one();

  return res;
}

int CameraMainStream::enqueueBuffer() {
  HAL_LOG_ENTER();
  int res = 0;
  if(initialized) {
    res = stream_->EnqueueBuffer();
    if (res) {
      HAL_LOGE("Device failed to turn on stream.");
      return res;
    }
  }

  return res;
}

int CameraMainStream::dequeueBuffer(void ** src_addr,struct timeval * ts) {
  HAL_LOG_ENTER();
  int res = 0;

  if(initialized) {
    HAL_LOGV("dequeueBuffer before WaitCameraReady.");
    res = stream_->WaitCameraReady();
    if (res != 0) {
        HAL_LOGE("wait v4l2 buffer time out in dequeueBuffer!");
        return res;
    }
    HAL_LOGV("dequeueBuffer after WaitCameraReady.");

    res = stream_->DequeueBuffer(src_addr,ts);
    if (res) {
      HAL_LOGE("Device failed to DequeueBuffer.");
      return res;
    }
  }

  return res;
}

int CameraMainStream::setIspFlash(int CameraFlashMode)
{
    HAL_LOG_ENTER();
    int res = 0;
    HAL_LOGV("####CameraMainStream setIspFlash CameraFlashMode:%d",CameraFlashMode);
    if(CameraFlashMode == CAM_FLASH_MODE_ON)
    {
        HAL_LOGV("####CAM_FLASH_MODE_ON");
        stream_->SetTakePictureCtrl(V4L2_TAKE_PICTURE_FLASH);
        stream_->SetFlashMode(V4L2_FLASH_LED_MODE_TORCH);
    }
    else if(CameraFlashMode == CAM_FLASH_MODE_AUTO)
    {
        HAL_LOGV("####CAM_FLASH_MODE_AUTO");
        stream_->SetTakePictureCtrl(V4L2_TAKE_PICTURE_FLASH);
        stream_->SetFlashMode(V4L2_FLASH_LED_MODE_AUTO);
    }
    else if(CameraFlashMode == CAM_FLASH_MODE_TORCH)
    {
        HAL_LOGV("####CAM_FLASH_MODE_TORCH");
        stream_->SetFlashMode(V4L2_FLASH_LED_MODE_TORCH);
    }
    else if(CameraFlashMode == CAM_FLASH_MODE_VIDEO_OFF)
    {
        HAL_LOGV("####CAM_FLASH_MODE_VIDEO_OFF");
        stream_->SetFlashMode(V4L2_FLASH_LED_MODE_NONE);
    }
    else if(CameraFlashMode == CAM_FLASH_MODE_OFF)
    {
        HAL_LOGV("####CAM_FLASH_MODE_OFF");
        stream_->SetTakePictureCtrl(V4L2_TAKE_PICTURE_STOP);
        stream_->SetFlashMode(V4L2_FLASH_LED_MODE_NONE);
    }

    return res;
}

int CameraMainStream::getFocusStatus()
{
    HAL_LOG_ENTER();
    int res = -1;

    res = stream_->GetAutoFocusStatus();

    HAL_LOGV("####CameraMainStream getFocusStatus res:%d",res);

    return res;
}

int CameraMainStream::setFocusInit()
{
    HAL_LOG_ENTER();
    int res = -1;

    res = stream_->SetAutoFocusInit();

    HAL_LOGV("####CameraMainStream SetAutoFocusInit res:%d",res);

    return res;
}

int CameraMainStream::setIspFocus(int cameraFocusMode)
{
    HAL_LOG_ENTER();
    int res = -1;

    HAL_LOGV("####CameraMainStream setIspFocus res:%d cameraFocusMode:%d",res,
        cameraFocusMode);

    if(cameraFocusMode == CAM_FOCUS_MODE_START_UP)
    {
        //res = stream_->SetAutoFocusRange(V4L2_AUTO_FOCUS_RANGE_AUTO);
        //res = stream_->SetAutoFocusStart();
    }
    else if(cameraFocusMode == CAM_FOCUS_MODE_CONTINUOUS_PICTURE)
    {
        //res = stream_->SetAutoFocusStart();
    }
    else if(cameraFocusMode == CAM_FOCUS_MODE_STOP)
    {
        res = stream_->SetAutoFocusStop();
    }
    else if(cameraFocusMode == CAM_FOCUS_MODE_START)
    {
        res = stream_->SetAutoFocusStart();
    }

    return res;
}
int CameraMainStream::setFpsRanage(int min_fps)
{
        int res = -1;
        res = stream_->SetFpsRanage(min_fps);
        return res;
}

int CameraMainStream::setTestPattern(int test_pattern)
{
        int res = -1;
        res = stream_->SetTestPattern(test_pattern);
        return res;
}

int CameraMainStream::setIspCrop(cam_crop_rect_t camCropRect)
{
    HAL_LOG_ENTER();
    int res = -1;

    res = stream_->SetCropRect(camCropRect);

    HAL_LOGV("####CameraMainStream setIspCrop left:%d top:%d width:%d height:%d",
        camCropRect.left,camCropRect.top,camCropRect.width,camCropRect.height);

    return res;
}

int CameraMainStream::setIspFocusRegions(cam_rect_t cam_regions)
{
    HAL_LOG_ENTER();
    int res = -1;

    res = stream_->SetAutoFocusRegions(cam_regions);

    HAL_LOGV("####CameraMainStream SetAutoFocusRegions x_min:%d y_min:%d x_max:%d y_max:%d",
        cam_regions.x_min,cam_regions.y_min,cam_regions.x_max,cam_regions.y_max);

    return res;
}

CameraSubStream::CameraSubStream(std::shared_ptr<V4L2Stream> stream, int isBlob):
    CameraStream(stream, isBlob){
  HAL_LOG_ENTER();
}

CameraSubStream::~CameraSubStream() {
    HAL_LOG_ENTER();
    std::unique_lock<std::mutex> lock(sub_yuv_buffer_queue_lock_);
}

int CameraSubStream::start(){
  HAL_LOG_ENTER();

  int res = 0;
  if(initialized) {
    // Stream on first.
    if(!mStreamOn) {
      res = stream_->StreamOn();
      if (res) {
        HAL_LOGE("Device failed to turn on stream.");
        return res;
      }
      mStreamOn = true;
      //manager_->start(stream_->getStreamSerial());
    }
  }

  return res;
}
int CameraSubStream::stop(){
  HAL_LOG_ENTER();

  int res = 0;
  if(initialized) {
    // Stream on first.
    if(!mStreamOn) {
      HAL_LOGE("Device failed to turn off stream, turn on first.");
    } else {
      res = stream_->StreamOff();
      if (res) {
        HAL_LOGE("Device failed to turn off stream.");
        return res;
      }
      mStreamOn = false;
    }
  }
  return res;
}
int CameraSubStream::flush(){
  HAL_LOG_ENTER();

  int res = 0;
  if(initialized) {
    // Stream on first.
    if(!mStreamOn) {
      HAL_LOGE("Device failed to flush stream, turn on first.");
    } else {
      res = stream_->flush();
      if (res) {
        HAL_LOGE("Device failed to flush stream.");
        return res;
      }
    }
  }
  return res;
}

int CameraSubStream::initialize(uint32_t width, uint32_t height, int format, uint32_t usage) {
  HAL_LOG_ENTER();

  int res = -1;
  HAL_LOGD("CameraSubStream:%d x %d, format:%d, usage:%d.", width, height, format, usage);

  res = stream_->SetParm(V4L2_MODE_VIDEO);
  if (res) {
    HAL_LOGE("Failed to SetParm.");
  }

  uint32_t max_buffers = MAX_BUFFER_NUM;
  if(format == HAL_PIXEL_FORMAT_BLOB) {
    format = HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED;
  }
  StreamFormat stream_format(format, width, height);
  res = stream_->SetFormat(stream_format, &max_buffers);
  if (res) {
    HAL_LOGE("Failed to set stream to correct format for stream: %d.", res);
    //return -ENODEV;
  }

  res = stream_->PrepareBuffer();
  if (res) {
    HAL_LOGE("Failed to prepare buffer.");
    return -ENODEV;
  }
  initialized = true;
#if 1
  // Stream on first.
  if(!mStreamOn) {
    res = stream_->StreamOn();
    if (res) {
      HAL_LOGE("Device failed to turn on stream.");
      return res;
    }
    mStreamOn = true;
    //manager_->start(stream_->getStreamSerial());
  }
#endif
  return res;
}
int CameraSubStream::setFormat(uint32_t width, uint32_t height, int format, uint32_t usage) {
  HAL_LOG_ENTER();

  int res = -1;
  HAL_LOGD("CameraSubStream:%d x %d, format:%d, usage:%d.", width, height, format, usage);

  mWidth = width;
  mHeight = height;
  mFormat = format;
  mUsage = usage;
  res = 0;
  return res;
}

int CameraSubStream::waitBuffer(buffer_handle_t * buffer, uint32_t frameNumber) {

  int res = -1;
  //HAL_LOGD("Failed to prepare buffer.");
  while(sub_frameNumber_buffers_map_queue_.empty()){
    std::unique_lock<std::mutex> lock(sub_yuv_buffer_queue_lock_);
    HAL_LOGD("Wait the main_yuv_buffer_queue_lock_ lock.");
    sub_yuv_buffer_availabl_queue_.wait(lock);
    HAL_LOGD("The lock has been notified.");
    res = 0;
  }

  return res;
}

int CameraSubStream::getBuffer(buffer_handle_t ** buffer, uint32_t* frameNumber) {

  int res = -ENOMEM;
  //HAL_LOGD("Failed to prepare buffer.");
  HAL_LOGD("sub_frameNumber_buffers_map_queue_ has %d buffer(s).", sub_frameNumber_buffers_map_queue_.size());
  HAL_LOGD("sub_blob_frameNumber_buffers_map_queue_ has %d buffer(s).", sub_blob_frameNumber_buffers_map_queue_.size());

  if(sub_frameNumber_buffers_map_queue_.size() && !isBlobFlag){
    std::unique_lock<std::mutex> lock(sub_yuv_buffer_queue_lock_);
    frame_bufferHandle_map_t tmp = sub_frameNumber_buffers_map_queue_.front();;
    *frameNumber = tmp.frameNum;
    *buffer = tmp.bufferHandleptr;
    sub_frameNumber_buffers_map_queue_.pop();
    HAL_LOGD("sub_frameNumber_buffers_map_queue_ buffer_handle_t*:%p, FrameNumber:%d poped.", *buffer, *frameNumber);
    res = 0;
  } else if(sub_blob_frameNumber_buffers_map_queue_.size()) {
    std::unique_lock<std::mutex> lock(sub_blob_buffer_queue_lock_);
    frame_bufferHandle_map_t tmp = sub_blob_frameNumber_buffers_map_queue_.front();;
    *frameNumber = tmp.frameNum;
    *buffer = tmp.bufferHandleptr;
    sub_blob_frameNumber_buffers_map_queue_.pop();
    HAL_LOGD("sub_blob_frameNumber_buffers_map_queue_ buffer_handle_t*:%p, FrameNumber:%d poped.", *buffer, *frameNumber);
    res = 0;
  }
  return res;
}

int CameraSubStream::encodebuffer(void * dst_addr, void * src_addr, unsigned long mJpegBufferSizes) {
  HAL_LOG_ENTER();

  int res = -ENOMEM;
  JPEG_ENC_t jpeg_enc;
  memset(&jpeg_enc, 0, sizeof(jpeg_enc));

  std::array<uint8_t, 33> mGpsMethod = {0};
  res = SingleTagValue(mMetadata,
                          ANDROID_JPEG_GPS_PROCESSING_METHOD,
                          &mGpsMethod);
  HAL_LOGD("jpeg info:mGpsMethod:%s.", (unsigned char *)(&(mGpsMethod[0])));

  if (0 != mGpsMethod[0])
  {
    std::array<double, 3> gpsCoordinates = {0,0,0};
    res = SingleTagValue(mMetadata,
                            ANDROID_JPEG_GPS_COORDINATES,
                            &gpsCoordinates);
    HAL_LOGD("jpeg info:latitude:%f, longitude:%f, altitude:%f.", gpsCoordinates[0], gpsCoordinates[1], gpsCoordinates[2]);
    int64 mGpsTimestamp = 0;
    res = SingleTagValue(mMetadata,
                            ANDROID_JPEG_GPS_TIMESTAMP,
                            &mGpsTimestamp);
    HAL_LOGD("jpeg info:mGpsTimestamp:%d.", mGpsTimestamp);

    jpeg_enc.enable_gps            = 1;
    jpeg_enc.gps_latitude        = gpsCoordinates[0];
    jpeg_enc.gps_longitude        = gpsCoordinates[1];
    jpeg_enc.gps_altitude        = gpsCoordinates[2];
    jpeg_enc.gps_timestamp        = mGpsTimestamp;
    for(int i = 0; i< 33; i++) {
      jpeg_enc.gps_processing_method[i] = mGpsMethod[i];
    }
  }
  else
  {
      jpeg_enc.enable_gps            = 0;
  }

  int32 mOrientation = 0;
  res = SingleTagValue(mMetadata,
                          ANDROID_JPEG_ORIENTATION,
                          &mOrientation);
  HAL_LOGD("jpeg info:mOrientation:%d.", mOrientation);
  jpeg_enc.rotate = mOrientation;

  byte mQuality = 90;
  res = SingleTagValue(mMetadata,
                          ANDROID_JPEG_QUALITY,
                          &mQuality);
  HAL_LOGD("jpeg info:mQuality:%d.", mQuality);
  jpeg_enc.quality = mQuality;

  byte mThumbnailQuality = 90;
  res = SingleTagValue(mMetadata,
                          ANDROID_JPEG_THUMBNAIL_QUALITY,
                          &mThumbnailQuality);
  HAL_LOGD("jpeg info:mThumbnailQuality:%d.", mQuality);

  std::array<int32, 2> mThumbnailSize = {320,240};
  res = SingleTagValue(mMetadata,
                          ANDROID_JPEG_THUMBNAIL_SIZE,
                          &mThumbnailSize);
  HAL_LOGD("jpeg info:mThumbnailSize:%dx%d.", mThumbnailSize[0], mThumbnailSize[1]);

  jpeg_enc.pic_w            = mWidth;
  jpeg_enc.pic_h            = mHeight;

  jpeg_enc.thumbWidth        = mThumbnailSize[0];
  jpeg_enc.thumbHeight    = mThumbnailSize[1];

  res = stream_->EncodeBuffer(dst_addr, src_addr, mJpegBufferSizes, jpeg_enc);

  if (res) {
    HAL_LOGE("Device EncodeBuffer failed.");
  }
  return res;

}

int CameraSubStream::copybuffer(void * dst_addr, void * src_addr) {

  int res = -ENOMEM;
  //HAL_LOGD("Failed to prepare buffer.");
  res = stream_->CopyBuffer(dst_addr, src_addr, mUsage);
  if (res) {
    HAL_LOGE("Device CopyBuffer.");
  }
  return res;
}

int CameraSubStream::request(buffer_handle_t * buffer, uint32_t frameNumber, CameraMetadata* metadata, StreamManager* mManager) {
  HAL_LOG_ENTER();

  int res = 0;
  //HAL_LOGD("Failed to prepare buffer.");
  mMetadata = metadata;

  if(!isBlobFlag){
    std::lock_guard<std::mutex> guard(sub_yuv_buffer_queue_lock_);
    frame_bufferHandle_map_t tmp;
    tmp.bufferHandleptr = buffer;
    tmp.frameNum = frameNumber;
    sub_frameNumber_buffers_map_queue_.push(tmp);
    HAL_LOGD("sub_frameNumber_buffers_map_queue_ buffer_handle_t*:%p, FrameNumber:%d pushed.", buffer, frameNumber);
  } else {
    std::lock_guard<std::mutex> guard(sub_blob_buffer_queue_lock_);
    frame_bufferHandle_map_t tmp;
    tmp.bufferHandleptr = buffer;
    tmp.frameNum = frameNumber;
    sub_blob_frameNumber_buffers_map_queue_.push(tmp);
    HAL_LOGD("sub_blob_frameNumber_buffers_map_queue_ buffer_handle_t*:%p, FrameNumber:%d pushed.", buffer, frameNumber);
  }



  return res;
}

int CameraSubStream::enqueueBuffer() {
  HAL_LOG_ENTER();
  int res = 0;
  if(initialized) {
    res = stream_->EnqueueBuffer();
    if (res) {
      HAL_LOGE("Device failed to turn on stream.");
      return res;
    }
  }

  return res;
}

int CameraSubStream::dequeueBuffer(void ** src_addr,struct timeval * ts) {
  HAL_LOG_ENTER();
  int res = 0;
  if(initialized) {
    HAL_LOGV("dequeueBuffer before WaitCameraReady.");
    res = stream_->WaitCameraReady();
    if (res != 0) {
        HAL_LOGW("wait v4l2 buffer time out in dequeueBuffer!");
        return res;
    }
    HAL_LOGV("dequeueBuffer after WaitCameraReady.");

    res = stream_->DequeueBuffer(src_addr,ts);
    if (res) {
      HAL_LOGE("Device failed to turn on stream.");
      return res;
    }
  }

  return res;
}

int CameraSubStream::setIspFlash(int CameraFlashMode)
{
    HAL_LOG_ENTER();
    int res = 0;

    HAL_LOGV("####CameraSubStream setIspFlash CameraFlashMode:%d",CameraFlashMode);
    if(CameraFlashMode == CAM_FLASH_MODE_ON)
    {
        HAL_LOGV("####CAM_FLASH_MODE_ON");
        stream_->SetTakePictureCtrl(V4L2_TAKE_PICTURE_FLASH);
        stream_->SetFlashMode(V4L2_FLASH_LED_MODE_TORCH);
    }
    else if(CameraFlashMode == CAM_FLASH_MODE_AUTO)
    {
        HAL_LOGV("####CAM_FLASH_MODE_AUTO");
        stream_->SetTakePictureCtrl(V4L2_TAKE_PICTURE_FLASH);
        stream_->SetFlashMode(V4L2_FLASH_LED_MODE_AUTO);
    }
    else if(CameraFlashMode == CAM_FLASH_MODE_TORCH)
    {
        HAL_LOGV("####CAM_FLASH_MODE_TORCH");
        stream_->SetFlashMode(V4L2_FLASH_LED_MODE_TORCH);
    }
    else if(CameraFlashMode == CAM_FLASH_MODE_VIDEO_OFF)
    {
        HAL_LOGV("####CAM_FLASH_MODE_VIDEO_OFF");
        stream_->SetFlashMode(V4L2_FLASH_LED_MODE_NONE);
    }
    else if(CameraFlashMode == CAM_FLASH_MODE_OFF)
    {
        HAL_LOGV("####CAM_FLASH_MODE_OFF");
        stream_->SetTakePictureCtrl(V4L2_TAKE_PICTURE_STOP);
        stream_->SetFlashMode(V4L2_FLASH_LED_MODE_NONE);
    }

    return res;
}

int CameraSubStream::getFocusStatus()
{
    HAL_LOG_ENTER();
    int res = -1;

    res = stream_->GetAutoFocusStatus();

    HAL_LOGV("####CameraSubStream getFocusStatus res:%d",res);

    return res;
}

int CameraSubStream::setFocusInit()
{
    HAL_LOG_ENTER();
    int res = -1;

    res = stream_->SetAutoFocusInit();

    HAL_LOGV("####CameraSubStream SetAutoFocusInit res:%d",res);

    return res;
}

int CameraSubStream::setIspFocus(int cameraFocusMode)
{
    HAL_LOG_ENTER();
    int res = -1;

    HAL_LOGV("####CameraSubStream setIspFocus res:%d cameraFocusMode:%d",res,
        cameraFocusMode);

    if(cameraFocusMode == CAM_FOCUS_MODE_START_UP)
    {
        //res = stream_->Set3ALock(~(V4L2_LOCK_FOCUS | V4L2_LOCK_EXPOSURE| V4L2_LOCK_WHITE_BALANCE));
        //res = stream_->SetAutoFocusRange(V4L2_AUTO_FOCUS_RANGE_AUTO);
        //res = stream_->SetAutoFocusStart();
    }
    else if(cameraFocusMode == CAM_FOCUS_MODE_CONTINUOUS_PICTURE)
    {
        //res = stream_->SetAutoFocusStart();
    }
    else if(cameraFocusMode == CAM_FOCUS_MODE_STOP)
    {
        res = stream_->SetAutoFocusStop();
    }
    else if(cameraFocusMode == CAM_FOCUS_MODE_START)
    {
        res = stream_->SetAutoFocusStart();
    }

    return res;
}

int CameraSubStream::setFpsRanage(int min_fps)
{
        int res = -1;
        res = stream_->SetFpsRanage(min_fps);
        return res;
}

int CameraSubStream::setTestPattern(int test_pattern)
{
        int res = -1;
        res = stream_->SetTestPattern(test_pattern);
        return res;
}

int CameraSubStream::setIspCrop(cam_crop_rect_t camCropRect)
{
    HAL_LOG_ENTER();
    int res = -1;

    HAL_LOGV("####CameraSubStream setIspCrop left:%d top:%d width:%d height:%d",
        camCropRect.left,camCropRect.top,camCropRect.width,camCropRect.height);

    res = stream_->SetCropRect(camCropRect);

    return res;
}

int CameraSubStream::setIspFocusRegions(cam_rect_t cam_regions)
{
    HAL_LOG_ENTER();
    int res = -1;

    res = stream_->SetAutoFocusRegions(cam_regions);

    HAL_LOGV("####CameraSubStream SetAutoFocusRegions x_min:%d y_min:%d x_max:%d y_max:%d",
        cam_regions.x_min,cam_regions.y_min,cam_regions.x_max,cam_regions.y_max);

    return res;
}

CameraMainMirrorStream::CameraMainMirrorStream(std::shared_ptr<V4L2Stream> stream, int isBlob):
    CameraStream(stream, isBlob){
  HAL_LOG_ENTER();
}
CameraMainMirrorStream::~CameraMainMirrorStream(){
  HAL_LOG_ENTER();
  std::unique_lock<std::mutex> lock(main_yuv_buffer_queue_lock_);
}

int CameraMainMirrorStream::start(){
  HAL_LOG_ENTER();
  int res = 0;
  return res;
}
int CameraMainMirrorStream::stop(){
  HAL_LOG_ENTER();
  int res = 0;
  return res;
}

int CameraMainMirrorStream::flush(){
  HAL_LOG_ENTER();
  int res = 0;
  return res;
}

int CameraMainMirrorStream::initialize(uint32_t width, uint32_t height, int format, uint32_t usage) {
  HAL_LOG_ENTER();
  int res = 0;
  HAL_LOGD("CameraMainMirrorStream:%d x %d, format:%d, usage:%d.", width, height, format, usage);
  initialized = true;
  return res;
}
int CameraMainMirrorStream::setFormat(uint32_t width, uint32_t height, int format, uint32_t usage) {
  HAL_LOG_ENTER();
  int res = -1;
  HAL_LOGD("CameraMainMirrorStream:%d x %d, format:%d, usage:%d.", width, height, format, usage);

  mWidth = width;
  mHeight = height;
  mFormat = format;
  mUsage = usage;
  res = 0;
  return res;
}

int CameraMainMirrorStream::waitBuffer(buffer_handle_t * buffer, uint32_t frameNumber) {
  int res = 0;
  return res;
}

int CameraMainMirrorStream::getBuffer(buffer_handle_t ** buffer, uint32_t* frameNumber) {

  int res = -ENOMEM;
  HAL_LOGD("main_frameNumber_buffers_map_queue_ has %d buffer(s).", main_frameNumber_buffers_map_queue_.size());
  HAL_LOGD("main_blob_frameNumber_buffers_map_queue_ has %d buffer(s).", main_blob_frameNumber_buffers_map_queue_.size());
  if(main_frameNumber_buffers_map_queue_.size() && !isBlobFlag){
    std::unique_lock<std::mutex> lock(main_yuv_buffer_queue_lock_);
    frame_bufferHandle_map_t tmp = main_frameNumber_buffers_map_queue_.front();;
    *frameNumber = tmp.frameNum;
    *buffer = tmp.bufferHandleptr;
    main_frameNumber_buffers_map_queue_.pop();
    HAL_LOGD("main_frameNumber_buffers_map_queue_ buffer_handle_t*:%p, FrameNumber:%d poped.", *buffer, *frameNumber);
    res = 0;
  } else if(main_blob_frameNumber_buffers_map_queue_.size()) {
    std::unique_lock<std::mutex> lock(main_blob_buffer_queue_lock_);
    frame_bufferHandle_map_t tmp = main_blob_frameNumber_buffers_map_queue_.front();;
    *frameNumber = tmp.frameNum;
    *buffer = tmp.bufferHandleptr;
    main_blob_frameNumber_buffers_map_queue_.pop();
    HAL_LOGD("main_blob_frameNumber_buffers_map_queue_ buffer_handle_t*:%p, FrameNumber:%d poped.", *buffer, *frameNumber);
    res = 0;
  }

  return res;
}
int CameraMainMirrorStream::encodebuffer(void * dst_addr, void * src_addr, unsigned long mJpegBufferSizes) {

  int res = -ENOMEM;
  //HAL_LOGD("Failed to prepare buffer.");
  JPEG_ENC_t jpeg_enc;
  memset(&jpeg_enc, 0, sizeof(jpeg_enc));

  std::array<uint8_t, 33> mGpsMethod = {0};
  res = SingleTagValue(mMetadata,
                          ANDROID_JPEG_GPS_PROCESSING_METHOD,
                          &mGpsMethod);
  HAL_LOGD("jpeg info:mGpsMethod:%s.", (unsigned char *)(&(mGpsMethod[0])));

  if (0 != mGpsMethod[0])
  {
      std::array<double, 3> gpsCoordinates = {0,0,0};
      res = SingleTagValue(mMetadata,
                              ANDROID_JPEG_GPS_COORDINATES,
                              &gpsCoordinates);
      HAL_LOGD("jpeg info:latitude:%f, longitude:%f, altitude:%f.", gpsCoordinates[0], gpsCoordinates[1], gpsCoordinates[2]);
      int64 mGpsTimestamp = 0;
      res = SingleTagValue(mMetadata,
                              ANDROID_JPEG_GPS_TIMESTAMP,
                              &mGpsTimestamp);
      HAL_LOGD("jpeg info:mGpsTimestamp:%d.", mGpsTimestamp);

      jpeg_enc.enable_gps            = 1;
      jpeg_enc.gps_latitude        = gpsCoordinates[0];
      jpeg_enc.gps_longitude        = gpsCoordinates[1];
      jpeg_enc.gps_altitude        = gpsCoordinates[2];
      jpeg_enc.gps_timestamp        = mGpsTimestamp;
      for(int i = 0; i< 33; i++) {
        jpeg_enc.gps_processing_method[i] = mGpsMethod[i];
      }
      //strcpy(jpeg_enc.gps_processing_method, mGpsMethod);
      //memset(mGpsMethod, 0, sizeof(mGpsMethod));
  }
  else
  {
      jpeg_enc.enable_gps            = 0;
  }

  int32 mOrientation = 0;
  res = SingleTagValue(mMetadata,
                          ANDROID_JPEG_ORIENTATION,
                          &mOrientation);
  HAL_LOGD("jpeg info:mOrientation:%d.", mOrientation);
  jpeg_enc.rotate = mOrientation;

  byte mQuality = 90;
  res = SingleTagValue(mMetadata,
                          ANDROID_JPEG_QUALITY,
                          &mQuality);
  HAL_LOGD("jpeg info:mQuality:%d.", mQuality);
  jpeg_enc.quality = mQuality;

  byte mThumbnailQuality = 90;
  res = SingleTagValue(mMetadata,
                          ANDROID_JPEG_THUMBNAIL_QUALITY,
                          &mThumbnailQuality);
  HAL_LOGD("jpeg info:mThumbnailQuality:%d.", mQuality);

  std::array<int32, 2> mThumbnailSize = {320,240};
  res = SingleTagValue(mMetadata,
                          ANDROID_JPEG_THUMBNAIL_SIZE,
                          &mThumbnailSize);
  HAL_LOGD("jpeg info:mThumbnailSize:%dx%d.", mThumbnailSize[0], mThumbnailSize[1]);

  jpeg_enc.pic_w            = mWidth;
  jpeg_enc.pic_h            = mHeight;

  jpeg_enc.thumbWidth        = mThumbnailSize[0];
  jpeg_enc.thumbHeight    = mThumbnailSize[1];

  res = stream_->EncodeBuffer(dst_addr, src_addr, mJpegBufferSizes, jpeg_enc);
  if (res) {
    HAL_LOGE("Device EncodeBuffer failed.");
  }
  return res;

}

int CameraMainMirrorStream::copybuffer(void * dst_addr, void * src_addr) {

  int res = -ENOMEM;
  //HAL_LOGD("Failed to prepare buffer.");
  res = stream_->CopyBuffer(dst_addr, src_addr, mUsage);
  if (res) {
    HAL_LOGE("Device CopyBuffer failed.");
  }
  return res;
}

int CameraMainMirrorStream::request(buffer_handle_t * buffer, uint32_t frameNumber, CameraMetadata* metadata, StreamManager* mManager) {
  HAL_LOG_ENTER();

  int res = 0;
  //HAL_LOGD("Failed to prepare buffer.");
  mMetadata = metadata;
  //TODO:GET frameNumber.
  if(!isBlobFlag){
    std::lock_guard<std::mutex> guard(main_yuv_buffer_queue_lock_);
    frame_bufferHandle_map_t tmp;
    tmp.bufferHandleptr = buffer;
    tmp.frameNum = frameNumber;
    main_frameNumber_buffers_map_queue_.push(tmp);
    HAL_LOGD("main_frameNumber_buffers_map_queue_ buffer_handle_t*:%p, FrameNumber:%d pushed.", buffer, frameNumber);
  } else {
    std::lock_guard<std::mutex> guard(main_blob_buffer_queue_lock_);
    frame_bufferHandle_map_t tmp;
    tmp.bufferHandleptr = buffer;
    tmp.frameNum = frameNumber;
    main_blob_frameNumber_buffers_map_queue_.push(tmp);
    HAL_LOGD("main_blob_frameNumber_buffers_map_queue_ buffer_handle_t*:%p, FrameNumber:%d pushed.", buffer, frameNumber);
  }



  return res;
}

int CameraMainMirrorStream::enqueueBuffer() {
  HAL_LOG_ENTER();
  int res = 0;
  return res;
}

int CameraMainMirrorStream::dequeueBuffer(void ** src_addr,struct timeval * ts) {
  HAL_LOG_ENTER();
  int res = 0;
  return res;
}

int CameraMainMirrorStream::setIspFlash(int CameraFlashMode)
{
    HAL_LOG_ENTER();
    int res = 0;

    HAL_LOGV("####CameraMainMirrorStream setIspFlash CameraFlashMode:%d",CameraFlashMode);

    return res;
}

int CameraMainMirrorStream::getFocusStatus()
{
    HAL_LOG_ENTER();
    int res = -1;

    res = stream_->GetAutoFocusStatus();

    HAL_LOGV("####CameraMainMirrorStream getFocusStatus res:%d",res);

    return res;
}

int CameraMainMirrorStream::setFocusInit()
{
    HAL_LOG_ENTER();
    int res = -1;

    res = stream_->SetAutoFocusInit();

    HAL_LOGV("####CameraMainMirrorStream SetAutoFocusInit res:%d",res);

    return res;
}

int CameraMainMirrorStream::setIspFocus(int cameraFocusMode)
{
    HAL_LOG_ENTER();
    int res = -1;

    HAL_LOGV("####CameraMainMirrorStream setIspFocus res:%d cameraFocusMode:%d",res,
        cameraFocusMode);

    return res;
}

int CameraMainMirrorStream::setFpsRanage(int min_fps)
{
        int res = -1;
        res = stream_->SetFpsRanage(min_fps);
        return res;
}

int CameraMainMirrorStream::setTestPattern(int test_pattern)
{
        int res = -1;
        res = stream_->SetTestPattern(test_pattern);
        return res;
}

int CameraMainMirrorStream::setIspCrop(cam_crop_rect_t camCropRect)
{
    HAL_LOG_ENTER();
    int res = -1;

    res = stream_->SetCropRect(camCropRect);

    HAL_LOGV("####CameraMainMirrorStream setIspCrop left:%d top:%d width:%d height:%d",
        camCropRect.left,camCropRect.top,camCropRect.width,camCropRect.height);

    return res;
}

int CameraMainMirrorStream::setIspFocusRegions(cam_rect_t cam_regions)
{
    HAL_LOG_ENTER();
    int res = -1;

    HAL_LOGV("####CameraMainMirrorStream SetAutoFocusRegions x_min:%d y_min:%d x_max:%d y_max:%d",
        cam_regions.x_min,cam_regions.y_min,cam_regions.x_max,cam_regions.y_max);

    return res;
}

CameraSubMirrorStream::CameraSubMirrorStream(std::shared_ptr<V4L2Stream> stream, int isBlob):
    CameraStream(stream, isBlob){
  mScaleFlag = false;
  memset(&crop_data, 0, sizeof(aw_crop_data_t));
  AWCropInit(&crop_data);
  HAL_LOG_ENTER();
}
CameraSubMirrorStream::~CameraSubMirrorStream(){
  HAL_LOG_ENTER();
  std::unique_lock<std::mutex> lock(sub_yuv_buffer_queue_lock_);
  AWCropExit(&crop_data);
}

int CameraSubMirrorStream::start(){
  HAL_LOG_ENTER();
  int res = 0;
  return res;
}
int CameraSubMirrorStream::stop(){
  HAL_LOG_ENTER();
  int res = 0;
  return res;
}

int CameraSubMirrorStream::flush(){
  HAL_LOG_ENTER();
  int res = 0;
  return res;
}
int CameraSubMirrorStream::setScaleFlag() {
  HAL_LOG_ENTER();
  mScaleFlag = true;
  return 0;
}

int CameraSubMirrorStream::initialize(uint32_t width, uint32_t height, int format, uint32_t usage) {
  HAL_LOG_ENTER();
  int res = 0;
  HAL_LOGD("CameraSubMirrorStream:%d x %d, format:%d, usage:%d.", width, height, format, usage);
  initialized = true;
  return res;
}
int CameraSubMirrorStream::setFormat(uint32_t width, uint32_t height, int format, uint32_t usage) {
  HAL_LOG_ENTER();
  int res = -1;
  HAL_LOGD("CameraSubMirrorStream:%d x %d, format:%d, usage:%d.", width, height, format, usage);

  mWidth = width;
  mHeight = height;
  mFormat = format;
  mUsage = usage;
  res = 0;
  return res;
}

int CameraSubMirrorStream::waitBuffer(buffer_handle_t * buffer, uint32_t frameNumber) {
  int res = 0;
  return res;
}

int CameraSubMirrorStream::getBuffer(buffer_handle_t ** buffer, uint32_t* frameNumber) {

  int res = -ENOMEM;
  HAL_LOGD("CameraSubMirrorStream sub_frameNumber_buffers_map_queue_ has %d buffer(s).", sub_frameNumber_buffers_map_queue_.size());
  HAL_LOGD("CameraSubMirrorStream sub_blob_frameNumber_buffers_map_queue_ has %d buffer(s).", sub_blob_frameNumber_buffers_map_queue_.size());
  if(sub_frameNumber_buffers_map_queue_.size() && !isBlobFlag){
    std::unique_lock<std::mutex> lock(sub_yuv_buffer_queue_lock_);
    frame_bufferHandle_map_t tmp = sub_frameNumber_buffers_map_queue_.front();;
    *frameNumber = tmp.frameNum;
    *buffer = tmp.bufferHandleptr;
    sub_frameNumber_buffers_map_queue_.pop();
    HAL_LOGD("CameraSubMirrorStream sub_frameNumber_buffers_map_queue_ buffer_handle_t*:%p, FrameNumber:%d poped.", *buffer, *frameNumber);
    res = 0;
  } else if(sub_blob_frameNumber_buffers_map_queue_.size()) {
    std::unique_lock<std::mutex> lock(sub_blob_buffer_queue_lock_);
    frame_bufferHandle_map_t tmp = sub_blob_frameNumber_buffers_map_queue_.front();;
    *frameNumber = tmp.frameNum;
    *buffer = tmp.bufferHandleptr;
    sub_blob_frameNumber_buffers_map_queue_.pop();
    HAL_LOGD("CameraSubMirrorStream sub_blob_frameNumber_buffers_map_queue_ buffer_handle_t*:%p, FrameNumber:%d poped.", *buffer, *frameNumber);
    res = 0;
  }

  return res;
}
int CameraSubMirrorStream::encodebuffer(void * dst_addr, void * src_addr, unsigned long mJpegBufferSizes) {

  int res = -ENOMEM;
  //HAL_LOGD("Failed to prepare buffer.");
  JPEG_ENC_t jpeg_enc;
  memset(&jpeg_enc, 0, sizeof(jpeg_enc));

  std::array<uint8_t, 33> mGpsMethod = {0};
  res = SingleTagValue(mMetadata,
                          ANDROID_JPEG_GPS_PROCESSING_METHOD,
                          &mGpsMethod);
  HAL_LOGD("jpeg info:mGpsMethod:%s.", (unsigned char *)(&(mGpsMethod[0])));

  if (0 != mGpsMethod[0])
  {
      std::array<double, 3> gpsCoordinates = {0,0,0};
      res = SingleTagValue(mMetadata,
                              ANDROID_JPEG_GPS_COORDINATES,
                              &gpsCoordinates);
      HAL_LOGD("jpeg info:latitude:%f, longitude:%f, altitude:%f.", gpsCoordinates[0], gpsCoordinates[1], gpsCoordinates[2]);
      int64 mGpsTimestamp = 0;
      res = SingleTagValue(mMetadata,
                              ANDROID_JPEG_GPS_TIMESTAMP,
                              &mGpsTimestamp);
      HAL_LOGD("jpeg info:mGpsTimestamp:%d.", mGpsTimestamp);

      jpeg_enc.enable_gps            = 1;
      jpeg_enc.gps_latitude        = gpsCoordinates[0];
      jpeg_enc.gps_longitude        = gpsCoordinates[1];
      jpeg_enc.gps_altitude        = gpsCoordinates[2];
      jpeg_enc.gps_timestamp        = mGpsTimestamp;
      for(int i = 0; i< 33; i++) {
        jpeg_enc.gps_processing_method[i] = mGpsMethod[i];
      }
      //strcpy(jpeg_enc.gps_processing_method, mGpsMethod);
      //memset(mGpsMethod, 0, sizeof(mGpsMethod));
  }
  else
  {
      jpeg_enc.enable_gps            = 0;
  }

  int32 mOrientation = 0;
  res = SingleTagValue(mMetadata,
                          ANDROID_JPEG_ORIENTATION,
                          &mOrientation);
  HAL_LOGD("jpeg info:mOrientation:%d.", mOrientation);
  jpeg_enc.rotate = mOrientation;

  byte mQuality = 90;
  res = SingleTagValue(mMetadata,
                          ANDROID_JPEG_QUALITY,
                          &mQuality);
  HAL_LOGD("jpeg info:mQuality:%d.", mQuality);
  jpeg_enc.quality = mQuality;

  byte mThumbnailQuality = 90;
  res = SingleTagValue(mMetadata,
                          ANDROID_JPEG_THUMBNAIL_QUALITY,
                          &mThumbnailQuality);
  HAL_LOGD("jpeg info:mThumbnailQuality:%d.", mQuality);

  std::array<int32, 2> mThumbnailSize = {320,240};
  res = SingleTagValue(mMetadata,
                          ANDROID_JPEG_THUMBNAIL_SIZE,
                          &mThumbnailSize);
  HAL_LOGD("jpeg info:mThumbnailSize:%dx%d.", mThumbnailSize[0], mThumbnailSize[1]);


  jpeg_enc.src_w            = mWidth;
  jpeg_enc.src_h            = mHeight;
  jpeg_enc.pic_w            = mWidth;
  jpeg_enc.pic_h            = mHeight;


  jpeg_enc.thumbWidth        = mThumbnailSize[0];
  jpeg_enc.thumbHeight    = mThumbnailSize[1];

  res = stream_->EncodeBuffer(dst_addr, src_addr, mJpegBufferSizes, jpeg_enc);
  if (res) {
    HAL_LOGE("Device EncodeBuffer failed.");
  }
  return res;

}
#if 0
int CameraSubMirrorStream::yuv420spDownScale(void* psrc, void* pdst, int src_w, int src_h, int dst_w, int dst_h)
{
  HAL_LOG_ENTER();

  char * psrc_y = (char *)psrc;
  char * pdst_y = (char *)pdst;
  char * psrc_uv = (char *)psrc + src_w * src_h;
  char * pdst_uv = (char *)pdst + dst_w * dst_h;

  int scale = 1;
  int scale_w = src_w / dst_w;
  int scale_h = src_h / dst_h;
  int h, w;

  if (dst_w > src_w
      || dst_h > src_h)
  {
    HAL_LOGE("error size, %dx%d -> %dx%d\n", src_w, src_h, dst_w, dst_h);
    return -1;
  }

  if (scale_w == scale_h)
  {
    scale = scale_w;
  }

  HAL_LOGV("scale = %d\n", scale);

  if (scale == 1)
  {
    if ((src_w == dst_w)
        && (src_h == dst_h))
    {
      memcpy((char*)pdst, (char*)psrc, dst_w * dst_h * 3/2);
    }
    else
    {
      // crop
      for (h = 0; h < dst_h; h++)
      {
        memcpy((char*)pdst + h * dst_w, (char*)psrc + h * src_w, dst_w);
      }
      for (h = 0; h < dst_h / 2; h++)
      {
        memcpy((char*)pdst_uv + h * dst_w, (char*)psrc_uv + h * src_w, dst_w);
      }
    }

    return 0;
  }

  for (h = 0; h < dst_h; h++)
  {
    for (w = 0; w < dst_w; w++)
    {
      *(pdst_y + h * dst_w + w) = *(psrc_y + h * scale * src_w + w * scale);
    }
  }
  for (h = 0; h < dst_h / 2; h++)
  {
    for (w = 0; w < dst_w; w+=2)
    {
      *((short*)(pdst_uv + h * dst_w + w)) = *((short*)(psrc_uv + h * scale * src_w + w * scale));
    }
  }

  return 0;
}
#else
int CameraSubMirrorStream::yuv420spDownScale(void* psrc, void* pdst, int src_w, int src_h, int dst_w, int dst_h){
  VencRect sCropInfo;
  VencIspBufferInfo pInBuffer,pOutBuffer;
  memset(&pInBuffer, 0, sizeof(pInBuffer));
  memset(&pOutBuffer, 0, sizeof(pOutBuffer));
  int mCopySize = dst_w*dst_h*3/2;
  int isAlign = IS_USAGE_VIDEO(mUsage) ? true:false;
  bool isCrop = false;
  if (dst_w != src_w || dst_h != src_h) {
     sCropInfo.nLeft    = 0;
     sCropInfo.nTop     = 0;
     sCropInfo.nWidth   = src_w;
     sCropInfo.nHeight  = src_h;
     isCrop = true;
 }
 pInBuffer.nWidth         =  src_w;
 pInBuffer.nHeight         = src_h;
 pInBuffer.colorFormat     = VENC_PIXEL_YUV420SP;
 pInBuffer.pAddrVirY     = (unsigned char*)psrc;

 pOutBuffer.nWidth         = dst_w;
 pOutBuffer.nHeight         = dst_h;
 pOutBuffer.pAddrVirY     = (unsigned char*)pdst;
 if (isCrop) {
     AWCropYuv(&crop_data, &pInBuffer, &sCropInfo , &pOutBuffer, isAlign);
     goto COPY_END;
 }
 if (isAlign) {
   memcpy(pdst, psrc, dst_w*dst_h);
   memcpy((char *)pdst + ALIGN_16B(dst_w)*ALIGN_16B(dst_h),
               (char *)psrc + dst_w*dst_h,
               (dst_w)*(dst_h)/2);
 } else {
   memcpy(pdst, psrc, mCopySize);
 }
 COPY_END:
    return 0;
}

#endif
int CameraSubMirrorStream::copybuffer(void * dst_addr, void * src_addr) {

  int res = -ENOMEM;
  //HAL_LOGD("Failed to prepare buffer.");
  if(mScaleFlag) {
    res = yuv420spDownScale(src_addr, dst_addr, stream_->GetDeviceWidth(), stream_->GetDeviceHeight(), mWidth, mHeight);
  } else {
    res = stream_->CopyBuffer(dst_addr, src_addr, mUsage);
  }

  if (res) {
    HAL_LOGE("Device CopyBuffer failed.");
  }
  return res;
}

int CameraSubMirrorStream::request(buffer_handle_t * buffer, uint32_t frameNumber, CameraMetadata* metadata, StreamManager* mManager) {
  HAL_LOG_ENTER();

  int res = 0;
  //HAL_LOGD("Failed to prepare buffer.");
  mMetadata = metadata;


  if(!isBlobFlag){
    std::lock_guard<std::mutex> guard(sub_yuv_buffer_queue_lock_);
    frame_bufferHandle_map_t tmp;
    tmp.bufferHandleptr = buffer;
    tmp.frameNum = frameNumber;
    sub_frameNumber_buffers_map_queue_.push(tmp);
    HAL_LOGD("CameraSubMirrorStream sub_frameNumber_buffers_map_queue_ buffer_handle_t*:%p, FrameNumber:%d pushed.", buffer, frameNumber);
  } else {
    std::lock_guard<std::mutex> guard(sub_blob_buffer_queue_lock_);
    frame_bufferHandle_map_t tmp;
    tmp.bufferHandleptr = buffer;
    tmp.frameNum = frameNumber;
    sub_blob_frameNumber_buffers_map_queue_.push(tmp);
    HAL_LOGD("CameraSubMirrorStream sub_blob_frameNumber_buffers_map_queue_ buffer_handle_t*:%p, FrameNumber:%d pushed.", buffer, frameNumber);
  }


  return res;
}

int CameraSubMirrorStream::enqueueBuffer() {
  HAL_LOG_ENTER();
  int res = 0;
  return res;
}

int CameraSubMirrorStream::dequeueBuffer(void ** src_addr,struct timeval * ts) {
  HAL_LOG_ENTER();
  int res = 0;
  return res;
}

int CameraSubMirrorStream::setIspFlash(int CameraFlashMode)
{
    HAL_LOG_ENTER();
    int res = 0;

    HAL_LOGV("####CameraSubMirrorStream setIspFlash CameraFlashMode:%d",CameraFlashMode);

    return res;
}

int CameraSubMirrorStream::getFocusStatus()
{
    HAL_LOG_ENTER();
    int res = -1;

    res = stream_->GetAutoFocusStatus();

    HAL_LOGV("####CameraSubMirrorStream getFocusStatus res:%d",res);

    return res;
}

int CameraSubMirrorStream::setFocusInit()
{
    HAL_LOG_ENTER();
    int res = -1;

    res = stream_->SetAutoFocusInit();

    HAL_LOGV("####CameraSubMirrorStream SetAutoFocusInit res:%d",res);

    return res;
}

int CameraSubMirrorStream::setIspFocus(int cameraFocusMode)
{
    HAL_LOG_ENTER();
    int res = -1;

    HAL_LOGV("####CameraSubMirrorStream setIspFocus res:%d cameraFocusMode:%d",res,
        cameraFocusMode);

    return res;
}

int CameraSubMirrorStream::setFpsRanage(int min_fps)
{
        int res = -1;
        res = stream_->SetFpsRanage(min_fps);
        return res;
}

int CameraSubMirrorStream::setTestPattern(int test_pattern)
{
        int res = -1;
        res = stream_->SetTestPattern(test_pattern);
        return res;
}

int CameraSubMirrorStream::setIspCrop(cam_crop_rect_t camCropRect)
{
    HAL_LOG_ENTER();
    int res = -1;

    res = stream_->SetCropRect(camCropRect);

    HAL_LOGV("####CameraSubMirrorStream setIspCrop left:%d top:%d width:%d height:%d",
        camCropRect.left,camCropRect.top,camCropRect.width,camCropRect.height);

    return res;
}

int CameraSubMirrorStream::setIspFocusRegions(cam_rect_t cam_regions)
{
    HAL_LOG_ENTER();
    int res = -1;

    HAL_LOGV("####CameraSubMirrorStream SetAutoFocusRegions x_min:%d y_min:%d x_max:%d y_max:%d",
        cam_regions.x_min,cam_regions.y_min,cam_regions.x_max,cam_regions.y_max);

    return res;
}

}
