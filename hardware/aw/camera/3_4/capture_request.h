
#ifndef DEFAULT_CAMERA_HAL_CAPTURE_REQUEST_H_
#define DEFAULT_CAMERA_HAL_CAPTURE_REQUEST_H_

#include <memory>
#include <vector>

#include "CameraMetadata.h"
#include <hardware/camera3.h>

#include "common.h"

namespace default_camera_hal {


using ::android::hardware::camera::common::V1_0::helper::CameraMetadata;

// A simple wrapper for camera3_capture_request_t,
// with a constructor that makes a deep copy from the original struct.
struct CaptureRequest {
  uint32_t frame_number;
  CameraMetadata settings;
  std::unique_ptr<camera3_stream_buffer_t> input_buffer;
  std::vector<camera3_stream_buffer_t> output_buffers;
  bool tbuffer_has_been_used;
#if DEBUG_PERFORMANCE
  int64_t timeRequest;
#endif

  CaptureRequest();
  // Create a deep copy of |request|.
  CaptureRequest(const camera3_capture_request_t* request);
};

}  // namespace default_camera_hal

#endif  // DEFAULT_CAMERA_HAL_CAPTURE_REQUEST_H_
