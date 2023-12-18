
#ifndef V4L2_CAMERA_HAL_V4L2_GRALLOC_H_
#define V4L2_CAMERA_HAL_V4L2_GRALLOC_H_

#include <linux/videodev2.h>

#include <unordered_map>
#include <errno.h>
#include <hardware/camera3.h>
#include <hardware/gralloc.h>
#include <system/graphics.h>

#include "common.h"

namespace v4l2_camera_hal {

// Generously allow up to 6MB (the largest JPEG on the RPi camera is about 5MB).
static constexpr size_t V4L2_MAX_JPEG_SIZE = 6000000;

// V4L2Gralloc is a wrapper around relevant parts of a gralloc module,
// with some assistive transformations.
class V4L2Gralloc {
 public:
  // Use this method to create V4L2Gralloc objects. Functionally equivalent
  // to "new V4L2Gralloc", except that it may return nullptr in case of failure.
  static V4L2Gralloc* NewV4L2Gralloc();
  virtual ~V4L2Gralloc();
  // Lock buffer for picture.
  int lock_pic(const camera3_stream_buffer_t* camera_buffer,
                        unsigned long* device_buffer);
  int lock_buffer(const camera3_stream_buffer_t* camera_buffer,
                        unsigned long* device_buffer);
  int unlock_buffer(const camera3_stream_buffer_t* camera_buffer);

  // Lock a camera buffer. Uses device buffer length, sets user pointer.
  int lock(const camera3_stream_buffer_t* camera_buffer,
           uint32_t bytes_per_line,
           v4l2_buffer* device_buffer);
  // Unlock a buffer that was locked by this helper (equality determined
  // based on buffer user pointer, not the specific object).
  int unlock(const v4l2_buffer* device_buffer);
  // Lock a camera buffer. Get vir addr, record index addr.
  int lock_index(const camera3_stream_buffer_t* camera_buffer,
                      unsigned long* device_buffer, uint32_t result_index);
  // Unlock buffer by index addr.
  int unlock_index(uint32_t result_index);
  int lock_handle(const buffer_handle_t* buffer,
                        void ** dst_addr);
  int lock_handle(const buffer_handle_t* buffer,
                        void ** dst_addr, unsigned long * mJpegBufferSizes);
  int get_share_fd(const buffer_handle_t* buffer,
                        int * sharefd, unsigned long * mJpegBufferSizes);

  int unlock_handle(const buffer_handle_t* buffer);

  // Release all held locks.
  int unlockAllBuffers();

 private:
  // Constructor is private to allow failing on bad input.
  // Use NewV4L2Gralloc instead.
  V4L2Gralloc(const gralloc_module_t* module);

  const gralloc_module_t* mModule;

  struct BufferData {
    const camera3_stream_buffer_t* camera_buffer;
    // Below fields only used when a ycbcr format transform is necessary.
    //std::unique_ptr<android_ycbcr> transform_dest;  // nullptr if no transform.
    uint32_t v4l2_bytes_per_line;
  };
  BufferData* aBufferData[MAX_BUFFER_NUM];
  buffer_handle_t aBufferDataVideo[MAX_BUFFER_NUM];

  // Map buffer index : BufferData about that buffer.
  std::unordered_map<int, const BufferData*> mBufferMap;
};

}  // namespace default_camera_hal

#endif  // V4L2_CAMERA_HAL_V4L2_GRALLOC_H_
