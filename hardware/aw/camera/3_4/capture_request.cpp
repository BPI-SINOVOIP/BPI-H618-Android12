
#include "capture_request.h"

#include <set>
#include <utils/Timers.h>

namespace default_camera_hal {

CaptureRequest::CaptureRequest() : CaptureRequest(nullptr) {}

CaptureRequest::CaptureRequest(const camera3_capture_request_t* request) {
  if (!request) {
    return;
  }

  frame_number = request->frame_number;

  // CameraMetadata makes copies of camera_metadata_t through the
  // assignment operator (the constructor taking a camera_metadata_t*
  // takes ownership instead).
  settings = request->settings;

  // camera3_stream_buffer_t can be default copy constructed,
  // as its pointer values are handles, not ownerships.

  // Copy the input buffer.
  if (request->input_buffer) {
    input_buffer =
        std::make_unique<camera3_stream_buffer_t>(*request->input_buffer);
  }

  // Safely copy all the output buffers.
  uint32_t num_output_buffers = request->num_output_buffers;
  if (num_output_buffers < 0 || !request->output_buffers) {
    num_output_buffers = 0;
  }

  tbuffer_has_been_used = false;
  #if DEBUG_PERFORMANCE
  timeRequest = systemTime() / 1000000;
  #endif
  output_buffers.insert(output_buffers.end(),
                        request->output_buffers,
                        request->output_buffers + num_output_buffers);
}

}  // namespace default_camera_hal
