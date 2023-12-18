
// Loosely based on hardware/libhardware/modules/camera/ExampleCamera.h

#ifndef V4L2_CAMERA_HAL_V4L2_CAMERA_H_
#define V4L2_CAMERA_HAL_V4L2_CAMERA_H_

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

#include "camera_stream.h"
#include "stream_manager.h"
#include "camera_config.h"

namespace v4l2_camera_hal {
class StreamManager;
class CameraStream;

// V4L2Camera is a specific V4L2-supported camera device. The Camera object
// contains all logic common between all cameras (e.g. front and back cameras),
// while a specific camera device (e.g. V4L2Camera) holds all specific
// metadata and logic about that device.
class V4L2Camera : public default_camera_hal::Camera {
 friend class CameraStream;
 friend class StreamManager;

 public:
  // Use this method to create V4L2Camera objects. Functionally equivalent
  // to "new V4L2Camera", except that it may return nullptr in case of failure.
  static V4L2Camera* NewV4L2Camera(int id, const std::string path, std::shared_ptr<CCameraConfig> pCameraCfg);
  int setFlashTorchMode(bool enabled);
  int closeFlashTorch();
  ~V4L2Camera();

 private:
  std::shared_ptr<V4L2Camera> instance;
  // Constructor private to allow failing on bad input.
  // Use NewV4L2Camera instead.
  //CCameraConfig *             mCameraConfig;
  std::shared_ptr<CCameraConfig> mCameraConfig;
  //V4L2Camera(int id,
  //           std::shared_ptr<V4L2Wrapper> v4l2_wrapper,
  //           std::unique_ptr<Metadata> metadata, CCameraConfig* pCameraCfg);
  V4L2Camera(int id,
             std::shared_ptr<V4L2Wrapper> v4l2_wrapper,
             std::unique_ptr<Metadata> metadata, std::shared_ptr<CCameraConfig> pCameraCfg);

  // default_camera_hal::Camera virtual methods.
  // Connect to the device: open dev nodes, etc.
  int connect() override;
  // Disconnect from the device: close dev nodes, etc.
  void disconnect() override;
  // Initialize static camera characteristics for individual device.
  int initStaticInfo(CameraMetadata* out) override;
  // Initialize a template of the given type.
  int initTemplate(int type, CameraMetadata* out) override;
  // Initialize device info: resource cost and conflicting devices
  // (/conflicting devices length).
  void initDeviceInfo(camera_info_t* info) override;
  // Extra initialization of device when opened.
  int initDevice() override;
  // Verify stream configuration dataspaces and rotation values
  bool validateDataspacesAndRotations(
      const camera3_stream_configuration_t* stream_config) override;
  // Set up the streams, including seting usage & max_buffers
  int setupStreams(camera3_stream_configuration_t* stream_config) override;
  // Verify settings are valid for a capture or reprocessing.
  bool isValidRequestSettings(const CameraMetadata& settings) override;
  // Enqueue a request to receive data from the camera.
  int enqueueRequest(
      std::shared_ptr<default_camera_hal::CaptureRequest> request) override;
  // Flush in flight buffers.
  int flushBuffers() override;

  int flushRequests(int err);

  // Async request processing helpers.
  // Dequeue a request from the waiting queue.
  // Blocks until a request is available.
  std::shared_ptr<default_camera_hal::CaptureRequest> dequeueRequest();

  // Thread functions. Return true to loop, false to exit.
  // Pass buffers for enqueued requests to the device.
  bool enqueuePreviewBuffers();
  // Retreive buffers from the device.
  bool dequeuePreviewBuffers();

  bool enqueuePictureBuffers();

  bool dequeuePictureBuffers();

  bool slowThread();
  bool enqueueThread();
  bool dequeueThread();
  void ReadyForBuffer();

  bool previewThread();

  bool pictureThread();

  bool mainThread();

  bool subThread();

  int sResultCallback(uint32_t frameNumber,struct timeval);

  // Tools for detect main stream.
  int max(int a, int b, int c);
  int min(int a, int b, int c);
  int maxHeight(camera3_stream_t ** streams,int num); 
  int maxWidth(camera3_stream_t ** streams, int num);
  int fillStreamInfo(camera3_stream_t * stream);
  int findStreamModes(STREAM_SERIAL stream_serial, camera3_stream_configuration_t* stream_config);
  int findStreamModes(STREAM_SERIAL stream_serial, camera3_stream_configuration_t* stream_config, int *isBlob);
  void updateActiveArray(CameraMetadata *camera_meta_data, uint32_t tag);
  // V4L2 helper.
  std::shared_ptr<V4L2Wrapper> device_;
  std::shared_ptr<V4L2Wrapper> device_main_channel_;
  std::shared_ptr<V4L2Wrapper> device_sub_channel_;
  std::unique_ptr<V4L2Wrapper::Connection> connection_;
  int device_id_;
  std::unique_ptr<Metadata> metadata_;
  CameraMetadata*  mMetadata;
  //std::shared_ptr<CameraMetadata>  mMetadata;

  std::mutex in_flight_lock_;
  // Maps buffer index : request.
  std::unordered_map<uint32_t, std::shared_ptr<default_camera_hal::CaptureRequest>>
      in_flight_;
  // Threads require holding an Android strong pointer.
  android::sp<android::Thread> buffer_preview_enqueuer_;
  android::sp<android::Thread> buffer_preview_dequeuer_;
  // Threads require holding an Android strong pointer.
  android::sp<android::Thread> buffer_picture_enqueuer_;
  android::sp<android::Thread> buffer_picture_dequeuer_;
    // Threads require holding an Android strong pointer.
  android::sp<android::Thread> buffer_preview_thread_;
  android::sp<android::Thread> buffer_picture_thread_;

  android::sp<android::Thread> main_stream_thread_;
  android::sp<android::Thread> sub_stream_thread_;

  android::sp<android::Thread> one_slow_thread_;
  android::sp<android::Thread> multi_enqueue_thread_;
  android::sp<android::Thread> multi_dequeue_thread_;

  int camera_fd;
  std::mutex camera_fd_lock_;

  int in_flight_buffer_count;
  std::condition_variable buffers_in_flight_;
  std::condition_variable tbuffers_in_flight_;
  bool buffers_in_flight_flag_;

  int32_t max_input_streams_;
  std::array<int, 3> max_output_streams_;  // {raw, non-stalling, stalling}.
  // Map stream format : camera3_stream_t* about that stream.
  std::unordered_map<int, const camera3_stream_t*> mStreamlistMap;
  std::shared_ptr<V4L2Stream> mStreamm;
  std::shared_ptr<V4L2Stream> mStreams;
  std::shared_ptr<V4L2Wrapper> mWrapper;
  std::shared_ptr<V4L2Wrapper::Connection> connection[MAX_STREAM];

  enum SituationMode {
    INIT = 0,
    BEFORE_PREVIEW,
    AFTER_PREVIEW,
    BEFORE_PICTURE,
    AFTER_PICTURE,
    BEFORE_VIDEO,
    AFTER_VIDEO
  };

  typedef enum {
    CAM_FLASH_MODE_OFF,
    CAM_FLASH_MODE_AUTO,
    CAM_FLASH_MODE_ON,
    CAM_FLASH_MODE_TORCH,
    CAM_FLASH_MODE_SINGLE,
    CAM_FLASH_MODE_VIDEO_OFF,
    CAM_FLASH_MODE_MAX
  } CameraFlashModeE;

  typedef enum {
    CAM_FLASH_STATE_UNAVAILABLE,
    CAM_FLASH_STATE_CHARGINT,
    CAM_FLASH_STATE_READY,
    CAM_FLASH_STATE_FIRED,
    CAM_FLASH_STATE_PARTIAL,
    CAM_FLASH_STATE_MAX
  } CameraFlashStateE;

  typedef struct  {
    int32_t focus_frame_num;
    int32_t focused_times;
    int32_t focus_status;
    int32_t update_status;
    int32_t status_flag;
    int32_t update_status_times;
    cam_focus_mode_t camera_focus_mode;
    cam_focus_trigger_t camera_focus_trigger;
  } cam_focus_impl;

  enum RequestMode {
    PREVIEW_MODE = 0,
    RUN_PREVIEW_MODE,
    PICTURE_MODE,
    VIDEO_MODE,
    VIDEO_SNAP_SHOT_MODE,
    RUN_VIDEO_MODE,
    ERROR_MODE
  };
  enum BuffersStatus {
    INIT_BUFFER = 0,
    PREPARE_BUFFER
  };

  struct {
    BuffersStatus pbuffers;
    BuffersStatus tbuffers;
    BuffersStatus vbuffers;
  }mBeffer_status;

  /*struct vCameraFlashState{
    CameraFlashModeE camera_flash_mode_e;
    uint32_t flash_frame_num;
  };*/

  enum StreamStatus {
    CLOSE_STREAM = 0,
    OPEN_STREAM
  };

  struct {
    StreamStatus pstream;
    StreamStatus tstream;
    StreamStatus vstream;
  }mStream_status;

  enum StreamIedex {
    PSTREAM = 0,
    TSTREAM,
    VSTREAM,
    VHSTREAM,
    ESTREAM
  };

  typedef RequestMode typeRequestMode;

  int FlashStatusFlag;
  int MarkFrameNum;
  std::mutex flash_status_lock_;
  std::mutex auto_focus_lock_;
  int ProbeFlashMode(CameraFlashModeE *camera_flash_mode, CameraMetadata* camera_meta_data,
        int *set_flash_flag,int frame_number);
  void updateFlashState(CameraMetadata *camera_meta_data, int frameNumber, int curFlashStateE);
  int ProcessFocusMode(CameraMetadata *camera_meta_data,int frame_number);
  void updateAfState(CameraMetadata *camera_meta_data, int frame_number);
  int ProcessFpsRanage(CameraMetadata *camera_meta_data, CameraStream *camera_stream);
  int ProcessTestPattern(CameraMetadata *camera_meta_data, CameraStream *camera_stream);
  int ProcessCropRegion(CameraMetadata *camera_meta_data,CameraStream *camera_stream);
  void convertFromRegions(cam_area_t &roi,
        const CameraMetadata &frame_settings, uint32_t tag);

  int analysisStreamNum(
      std::shared_ptr<default_camera_hal::CaptureRequest> request, RequestMode mRequestMode);


  RequestMode analysisRequest(
      std::shared_ptr<default_camera_hal::CaptureRequest> request);
  StreamIedex analysisStreamModes(
      camera3_stream_t * stream);

  int updateStatus(SituationMode mSituationMode);
  SituationMode getStatus();


  SituationMode mgSituationMode;
  bool mPicOnly;
  bool mVideoOnly;
  bool mVideoflag;
  int mTimeOutCount;
  int mRunStreamNum;
  bool isVideoSnap;



  // For steam on & stream off.
  //std::mutex stream_lock_;
  std::shared_ptr<std::mutex> stream_lock_;

  // Map stream format : camera3_stream_t* about that stream format.
  std::unordered_map<int, const StreamFormat> mStreamFormatMap;

  // Map frameNumber: refcnt about the buffer.
  std::mutex frameNumber_request_lock_;
  std::unordered_map<uint32_t, std::shared_ptr<default_camera_hal::CaptureRequest>> mMapFrameNumRequest;
  //std::vector<vCameraFlashState> vecCameraFlashState;
  CameraFlashStateE curFlashStateE;
  CameraFlashModeE curFlashModeE;
  CameraStream *mCameraStream[MAX_STREAM];
  //std::shared_ptr<CameraStream> mCameraStream[MAX_STREAM];

  cam_focus_impl camFocusImpl;
  cam_area_t camAreaRoi;
  cam_crop_rect_t camCropRect;
  std::queue<std::shared_ptr<default_camera_hal::CaptureRequest>>
      rfequest_queue_stream_;
  std::queue<std::shared_ptr<default_camera_hal::CaptureRequest>>
      wfequest_queue_stream_;


  // For preview steam.
  std::mutex request_queue_stream_lock_;
  std::queue<std::shared_ptr<default_camera_hal::CaptureRequest>>
      request_queue_stream_;
  std::condition_variable requests_available_stream_;
  bool stream_buffers_hasprepare_;

  // For preview steam.
  //std::mutex request_queue_pstream_lock_;
  //std::unique_ptr<std::mutex> request_queue_pstream_lock_;
  std::queue<std::shared_ptr<default_camera_hal::CaptureRequest>>
      request_queue_pstream_;
  std::condition_variable requests_available_pstream_;
  bool pstream_buffers_hasprepare_;

  // For picture steam.
  //std::mutex request_queue_tstream_lock_;
  //std::unique_ptr<std::mutex> request_queue_tstream_lock_;
  std::queue<std::shared_ptr<default_camera_hal::CaptureRequest>>
      request_queue_tstream_;
  std::condition_variable requests_available_tstream_;
  bool tstream_buffers_hasprepare_;

  // For video steam.
  //std::mutex request_queue_vstream_lock_;
  //std::unique_ptr<std::mutex> request_queue_vstream_lock_;
  std::queue<std::shared_ptr<default_camera_hal::CaptureRequest>>
      request_queue_vstream_;
  //std::mutex request_queue_vhstream_lock_;
  //std::unique_ptr<std::mutex>request_queue_vhstream_lock_;
  std::queue<std::shared_ptr<default_camera_hal::CaptureRequest>>
      request_queue_vhstream_;

  std::condition_variable requests_available_vstream_;
  bool vstream_buffers_hasprepare_;

  std::shared_ptr<default_camera_hal::CaptureRequest> trequest;

  // For destructor twice.
  std::shared_ptr<StreamManager> mStreamManager_;
  //StreamManager* mStreamManager_ptr_;
  //std::unique_ptr<StreamManager> mStreamManager_;
  //std::unique_ptr<StreamManager> mStreamManager_ptr_;

  bool mStreamTracker[MAX_STREAM];
  bool mSourceStreamTracker[MAX_STREAM];

  //V4L2Stream::Connection wrapper_connection[MAX_STREAM];

  DISALLOW_COPY_AND_ASSIGN(V4L2Camera);
};

}  // namespace v4l2_camera_hal

#endif  // V4L2_CAMERA_HAL_V4L2_CAMERA_H_
