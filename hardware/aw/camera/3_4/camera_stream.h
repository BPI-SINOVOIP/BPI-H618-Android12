
// Loosely based on hardware/libhardware/modules/camera/ExampleCamera.h

#ifndef V4L2_CAMERA_HAL_CAMERA_STREAM_H_
#define V4L2_CAMERA_HAL_CAMERA_STREAM_H_

#include <array>
#include <condition_variable>
#include <map>
#include <queue>
#include <string>


#include <utils/StrongPointer.h>
#include <utils/Thread.h>

#include "CameraMetadata.h"
#include "camera.h"
#include "common.h"
#include "metadata/metadata.h"
#include "v4l2_wrapper.h"

// For take picture.
#include "type_camera.h"
#include "vencoder.h"

namespace v4l2_camera_hal {
class V4L2Camera;
class V4L2Stream;
class StreamManager;

typedef enum {
    CAM_FLASH_MODE_OFF,
    CAM_FLASH_MODE_AUTO,
    CAM_FLASH_MODE_ON,
    CAM_FLASH_MODE_TORCH,
    CAM_FLASH_MODE_SINGLE,
    CAM_FLASH_MODE_VIDEO_OFF,
    CAM_FLASH_MODE_MAX
  } cam_flash_mode_t;

typedef struct  {
    int32_t x_min;
    int32_t y_min;
    int32_t x_max;
    int32_t y_max;
  } cam_rect_t;

  typedef struct  {
    cam_rect_t rect;
    int32_t weight;
  } cam_area_t;

  typedef struct {
      int32_t left;
      int32_t top;
      int32_t width;
      int32_t height;
  } cam_crop_rect_t;

  typedef enum {
    CAM_FOCUS_MODE_OFF,
    CAM_FOCUS_MODE_AUTO,
    CAM_FOCUS_MODE_MACRO,
    CAM_FOCUS_MODE_CONTINUOUS_VIDEO,
    CAM_FOCUS_MODE_CONTINUOUS_PICTURE,
    CAM_FOCUS_MODE_EDOF,
    CAM_FOCUS_MODE_START,
    CAM_FOCUS_MODE_STOP,
    CAM_FOCUS_MODE_START_UP,
    CAM_FOCUS_MODE_MAX
  } cam_focus_mode_t;

  typedef enum {
    CAM_FOCUS_TRIGGER_IDLE,
    CAM_FOCUS_TRIGGER_START,
    CAM_FOCUS_TRIGGER_CANCLE,
    CAM_FOCUS_TRIGGER_MAX
  } cam_focus_trigger_t;

class CameraStream {
  friend class StreamManager;
  friend class V4L2Stream;
public:
  static CameraStream* NewCameraStream(std::shared_ptr<V4L2Stream> stream, int isBlob, int isMirror);
  CameraStream(std::shared_ptr<V4L2Stream> stream, int isBlob);
  int configurateManager(StreamManager* manager);

  virtual ~CameraStream();
  virtual int start() = 0;
  virtual int stop() = 0;
  virtual int flush() = 0;
  virtual int initialize(uint32_t width, uint32_t height, int format, uint32_t usage) = 0;
  virtual int setFormat(uint32_t width, uint32_t height, int format, uint32_t usage) = 0;
  virtual int request(buffer_handle_t * buffer, uint32_t frameNumber, CameraMetadata* metadata, StreamManager* mManager) = 0;
  virtual int getBuffer(buffer_handle_t ** buffer, uint32_t* frameNumber) = 0;
  virtual int enqueueBuffer() = 0;
  virtual int dequeueBuffer(void ** src_addr,struct timeval * ts) = 0;
  virtual int encodebuffer(void * dst_addr, void * src_addr, unsigned long mJpegBufferSizes) = 0;
  virtual int copybuffer(void * dst_addr, void * src_addr) = 0;
  virtual int setIspFlash(int CameraFlashMode) = 0;
  virtual int getFocusStatus() = 0;
  virtual int setFocusInit() = 0;
  virtual int setIspFocus(int cameraFocusMode) = 0;
  virtual int setFpsRanage(int max_fps) = 0;
  virtual int setTestPattern(int test_pattern) = 0;
  virtual int setIspCrop(cam_crop_rect_t camCropRect) = 0;
  virtual int setIspFocusRegions(cam_rect_t cam_regions) = 0;

protected:
  int isBlobFlag;
  bool initialized;
  std::shared_ptr<V4L2Stream> stream_;
  StreamManager* manager_;
  //std::unique_ptr<V4L2Stream::Connection> connection_;
  std::condition_variable requests_available_;
  bool mStreamOn;
  CameraMetadata*  mMetadata;

  uint32_t mWidth;
  uint32_t mHeight;
  int mFormat;
  uint32_t mUsage;

  typedef struct frame_bufferHandle_map_t{
      uint32_t    frameNum;
      buffer_handle_t*     bufferHandleptr;
  }frame_bufferHandle_map_t;


private:




  DISALLOW_COPY_AND_ASSIGN(CameraStream);
};

class CameraMainStream : public CameraStream {
public:
  CameraMainStream(std::shared_ptr<V4L2Stream> stream, int isBlob);

  ~CameraMainStream();
  int start();
  int stop();
  int flush();
  int initialize(uint32_t width, uint32_t height, int format, uint32_t usage);
  int setFormat(uint32_t width, uint32_t height, int format, uint32_t usage);
  int getBuffer(buffer_handle_t ** buffer, uint32_t* frameNumber);
  int waitBuffer(buffer_handle_t * buffer, uint32_t frameNumber);
  int request(buffer_handle_t * buffer, uint32_t frameNumber, CameraMetadata* metadata, StreamManager* mManager);
  int dequeueBuffer(void ** src_addr,struct timeval * ts);
  int enqueueBuffer();
  int encodebuffer(void * dst_addr, void * src_addr, unsigned long mJpegBufferSizes);
  int copybuffer(void * dst_addr, void * src_addr);
  int setIspFlash(int CameraFlashMode);
  int getFocusStatus();
  int setFocusInit();
  int setIspFocus(int cameraFocusMode);
  int setFpsRanage(int max_fps);
  int setTestPattern(int test_pattern);
  int setIspCrop(cam_crop_rect_t camCropRect);
  int setIspFocusRegions(cam_rect_t cam_regions);

protected:


private:

  std::mutex main_yuv_buffer_queue_lock_;
  std::queue<frame_bufferHandle_map_t> main_frameNumber_buffers_map_queue_;
  std::condition_variable main_yuv_buffer_availabl_queue_;

  std::mutex main_blob_buffer_queue_lock_;
  std::queue<frame_bufferHandle_map_t> main_blob_frameNumber_buffers_map_queue_;
  std::condition_variable main_blob_buffer_availabl_queue_;


  DISALLOW_COPY_AND_ASSIGN(CameraMainStream);
};

class CameraSubStream : public CameraStream {
public:
  CameraSubStream(std::shared_ptr<V4L2Stream> stream, int isBlob);
  int start();
  int stop();
  int flush();
  int initialize(uint32_t width, uint32_t height, int format, uint32_t usage);
  int setFormat(uint32_t width, uint32_t height, int format, uint32_t usage);
  int getBuffer(buffer_handle_t ** buffer, uint32_t* frameNumber);
  int waitBuffer(buffer_handle_t * buffer, uint32_t frameNumber);
  int request(buffer_handle_t * buffer, uint32_t frameNumber, CameraMetadata* metadata, StreamManager* mManager);
  int dequeueBuffer(void ** src_addr,struct timeval * ts);
  int enqueueBuffer();
  int encodebuffer(void * dst_addr, void * src_addr, unsigned long mJpegBufferSizes);
  int copybuffer(void * dst_addr, void * src_addr);
  int setIspFlash(int CameraFlashMode);
  int getFocusStatus();
  int setFocusInit();
  int setIspFocus(int cameraFocusMode);
  int setFpsRanage(int max_fps);
  int setTestPattern(int test_pattern);
  int setIspCrop(cam_crop_rect_t camCropRect);
  int setIspFocusRegions(cam_rect_t cam_regions);

  ~CameraSubStream();

protected:

private:
  std::mutex sub_yuv_buffer_queue_lock_;
  std::queue<frame_bufferHandle_map_t> sub_frameNumber_buffers_map_queue_;
  std::condition_variable sub_yuv_buffer_availabl_queue_;

  std::mutex sub_blob_buffer_queue_lock_;
  std::queue<frame_bufferHandle_map_t> sub_blob_frameNumber_buffers_map_queue_;
  std::condition_variable sub_blob_buffer_availabl_queue_;

  DISALLOW_COPY_AND_ASSIGN(CameraSubStream);
};

class CameraMainMirrorStream : public CameraStream {
public:
  CameraMainMirrorStream(std::shared_ptr<V4L2Stream> stream, int isBlob);

  ~CameraMainMirrorStream();
  int start();
  int stop();
  int flush();
  int initialize(uint32_t width, uint32_t height, int format, uint32_t usage);
  int setFormat(uint32_t width, uint32_t height, int format, uint32_t usage);
  int getBuffer(buffer_handle_t ** buffer, uint32_t* frameNumber);
  int waitBuffer(buffer_handle_t * buffer, uint32_t frameNumber);
  int request(buffer_handle_t * buffer, uint32_t frameNumber, CameraMetadata* metadata, StreamManager* mManager);
  int dequeueBuffer(void ** src_addr,struct timeval * ts);
  int enqueueBuffer();
  int encodebuffer(void * dst_addr, void * src_addr, unsigned long mJpegBufferSizes);
  int copybuffer(void * dst_addr, void * src_addr);
  int setIspFlash(int CameraFlashMode);
  int getFocusStatus();
  int setFocusInit();
  int setIspFocus(int cameraFocusMode);
  int setFpsRanage(int max_fps);
  int setTestPattern(int test_pattern);
  int setIspCrop(cam_crop_rect_t camCropRect);
  int setIspFocusRegions(cam_rect_t cam_regions);

protected:


private:

  std::mutex main_yuv_buffer_queue_lock_;
  std::queue<frame_bufferHandle_map_t> main_frameNumber_buffers_map_queue_;
  std::condition_variable main_yuv_buffer_availabl_queue_;

  std::mutex main_blob_buffer_queue_lock_;
  std::queue<frame_bufferHandle_map_t> main_blob_frameNumber_buffers_map_queue_;
  std::condition_variable main_blob_buffer_availabl_queue_;


  DISALLOW_COPY_AND_ASSIGN(CameraMainMirrorStream);
};

class CameraSubMirrorStream : public CameraStream {
public:
  CameraSubMirrorStream(std::shared_ptr<V4L2Stream> stream, int isBlob);
  int start();
  int stop();
  int flush();
  int initialize(uint32_t width, uint32_t height, int format, uint32_t usage);
  int setScaleFlag();
  int setFormat(uint32_t width, uint32_t height, int format, uint32_t usage);
  int getBuffer(buffer_handle_t ** buffer, uint32_t* frameNumber);
  int waitBuffer(buffer_handle_t * buffer, uint32_t frameNumber);
  int request(buffer_handle_t * buffer, uint32_t frameNumber, CameraMetadata* metadata, StreamManager* mManager);
  int dequeueBuffer(void ** src_addr,struct timeval * ts);
  int enqueueBuffer();
  int encodebuffer(void * dst_addr, void * src_addr, unsigned long mJpegBufferSizes);
  int copybuffer(void * dst_addr, void * src_addr);
  int setIspFlash(int CameraFlashMode);
  int getFocusStatus();
  int setFocusInit();
  int setIspFocus(int cameraFocusMode);
  int setFpsRanage(int max_fps);
  int setTestPattern(int test_pattern);
  int setIspCrop(cam_crop_rect_t camCropRect);
  int setIspFocusRegions(cam_rect_t cam_regions);

  ~CameraSubMirrorStream();

protected:

private:
  int yuv420spDownScale(void* psrc, void* pdst, int src_w, int src_h, int dst_w, int dst_h);

  bool mScaleFlag;
  std::mutex sub_yuv_buffer_queue_lock_;
  std::queue<frame_bufferHandle_map_t> sub_frameNumber_buffers_map_queue_;
  std::condition_variable sub_yuv_buffer_availabl_queue_;

  std::mutex sub_blob_buffer_queue_lock_;
  std::queue<frame_bufferHandle_map_t> sub_blob_frameNumber_buffers_map_queue_;
  std::condition_variable sub_blob_buffer_availabl_queue_;
  aw_crop_data_t crop_data;

  DISALLOW_COPY_AND_ASSIGN(CameraSubMirrorStream);
};

}  // namespace v4l2_camera_hal

#endif  // V4L2_CAMERA_HAL_V4L2_CAMERA_H_

