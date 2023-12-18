#if DBG_V4L2_GRALLOC
#undef NDEBUG
#endif

#define LOG_TAG "CameraHALv3_V4L2Gralloc"

#include "v4l2_gralloc.h"

#include <linux/videodev2.h>

#include <cstdlib>

#include <hardware/camera3.h>
#include <hardware/gralloc.h>
#include <system/graphics.h>
#include <ui/Rect.h>
#include <ui/GraphicBufferMapper.h>

#include "common.h"
#include "stream_format.h"
#include GPU_PUBLIC_INCLUDE


namespace v4l2_camera_hal {

// Copy |height| lines from |src| to |dest|,
// where |src| and |dest| may have different line lengths.
void copyWithPadding(uint8_t* dest,
                     const uint8_t* src,
                     size_t dest_stride,
                     size_t src_stride,
                     size_t height) {
  size_t copy_stride = dest_stride;
  if (copy_stride > src_stride) {
    // Adding padding, not reducing. 0 out the extra memory.
    memset(dest, 0, src_stride * height);
    copy_stride = src_stride;
  }
  uint8_t* dest_line_start = dest;
  const uint8_t* src_line_start = src;
  for (size_t row = 0; row < height;
       ++row, dest_line_start += dest_stride, src_line_start += src_stride) {
    memcpy(dest_line_start, src_line_start, copy_stride);
  }
}

V4L2Gralloc* V4L2Gralloc::NewV4L2Gralloc() {
  // Initialize and check the gralloc module.
  const hw_module_t* module = nullptr;
  int res = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
  if (res || !module) {
    HAL_LOGE("Couldn't get gralloc module.");
    return nullptr;
  }
  const gralloc_module_t* gralloc =
      reinterpret_cast<const gralloc_module_t*>(module);

  // This class only supports Gralloc v0, not Gralloc V1.
  /*if (gralloc->common.module_api_version > GRALLOC_MODULE_API_VERSION_0_3) {
    HAL_LOGE(
        "Invalid gralloc version %x. Only 0.3 (%x) "
        "and below are supported by this HAL.",
        gralloc->common.module_api_version,
        GRALLOC_MODULE_API_VERSION_0_3);
    return nullptr;
  }*/

  return new V4L2Gralloc(gralloc);
}

// Private. As checked by above factory, module will be non-null
// and a supported version.
V4L2Gralloc::V4L2Gralloc(const gralloc_module_t* module) : mModule(module) {
  for(int i = 0; i< MAX_BUFFER_NUM; i++) {
    aBufferData[i] = (BufferData *)malloc(sizeof(BufferData));
    memset(aBufferData[i],0,sizeof(BufferData));
  }
}

V4L2Gralloc::~V4L2Gralloc() {
  // Unlock buffers that are still locked.
  unlockAllBuffers();
  for(int i = 0; i< MAX_BUFFER_NUM; i++) {
    free(aBufferData[i]);
  }
}
int V4L2Gralloc::get_share_fd(const buffer_handle_t* buffer,
                      int * sharefd, unsigned long * mJpegBufferSizes) {
  // Lock the camera buffer (varies depending on if the buffer is YUV or not).
  private_handle_t *hnd = (private_handle_t *)(*buffer);
  HAL_LOGD("buffer_handle_t buffer : %p, hnd->height:%d, hnd->width:%d, hnd->usage:0x%x, hnd->share_fd:%d.", *buffer, hnd->height, hnd->width, 
  hnd->usage, hnd->share_fd);
  *mJpegBufferSizes = hnd->width*hnd->height;
  *sharefd = hnd->share_fd;
  return 0;
}

int V4L2Gralloc::lock_handle(const buffer_handle_t* buffer,
                      void ** dst_addr) {
    int ret = -1;
    void * virFromGralloc;
    private_handle_t *hnd = (private_handle_t *)(*buffer);
    HAL_LOGD("buffer_handle_t buffer : %p, hnd->height:%d, hnd->width:%d, hnd->usage:0x%x.", *buffer, hnd->height, hnd->width, hnd->usage);

    /* let the graphics framework to lock the buffer, and get the virtual address. */
    const android::Rect rect(hnd->width, hnd->height);
    android::GraphicBufferMapper& grbuffer_mapper(android::GraphicBufferMapper::get());
    ret = grbuffer_mapper.lock(*buffer, hnd->usage, rect, &virFromGralloc);
    HAL_LOGV("%s: virFromGralloc = %p ",__FUNCTION__, virFromGralloc);
    if (ret != 1) {
        HAL_LOGD("%s: grbuffer_mapper.lock : %d -> %s",
            __FUNCTION__, ret, strerror(ret));
        //return false;
    }
    *dst_addr = virFromGralloc;
    HAL_LOGD("lock get buffer vaddr is %p",*dst_addr);

  // Lock the camera buffer (varies depending on if the buffer is YUV or not).
  /*private_handle_t *hnd = (private_handle_t *)(*buffer);
  HAL_LOGD("buffer_handle_t buffer : %p, hnd->height:%d, hnd->width:%d, hnd->usage:0x%x.", *buffer, hnd->height, hnd->width, hnd->usage);

  void* data;
  int ret = mModule->lock(mModule,
                            *buffer,
                            hnd->usage,
                            0,
                            0,
                            hnd->width,
                            hnd->height,
                            &data);
  if (ret) {
    HAL_LOGE("Failed to lock buffer: %d", ret);
    return ret;
  }
  HAL_LOGD("*camera_buffer->buffer:0x%x, stream->width:%d, stream->height:%d, lock buffer:%p.",
  *buffer, hnd->width, hnd->height, data);
  // If so, great, point to the beginning.
  if (!data) {
    ALOGE("Gralloc lock returned null ptr");
    return -ENODEV;
  }
  *dst_addr = data;
  HAL_LOGD("lock get buffer vaddr is %p",*dst_addr);*/
  return 0;
}

int V4L2Gralloc::lock_handle(const buffer_handle_t* buffer,
                      void ** dst_addr, unsigned long * mJpegBufferSizes) {
  // Lock the camera buffer (varies depending on if the buffer is YUV or not).
  private_handle_t *hnd = (private_handle_t *)(*buffer);
  HAL_LOGD("buffer_handle_t buffer : %p, hnd->height:%d, hnd->width:%d, hnd->usage:0x%x, hnd->share_fd:%d.",
    *buffer,
    hnd->height,
    hnd->width,
    hnd->usage,
    hnd->share_fd);
  *mJpegBufferSizes = hnd->width*hnd->height;
  int ret = -1;
  void * virFromGralloc;

  /* let the graphics framework to lock the buffer, and get the virtual address. */
  const android::Rect rect(hnd->width, hnd->height);
  android::GraphicBufferMapper& grbuffer_mapper(android::GraphicBufferMapper::get());
  ret = grbuffer_mapper.lock(*buffer, hnd->usage, rect, &virFromGralloc);
  HAL_LOGV("%s: virFromGralloc = %p ",__FUNCTION__, virFromGralloc);
  if (ret != 1) {
      HAL_LOGD("%s: grbuffer_mapper.lock: %d -> %s",
          __FUNCTION__, ret, strerror(ret));
      //return false;
  }
  *dst_addr = virFromGralloc;
  HAL_LOGD("lock get buffer vaddr is %p",*dst_addr);

  /*void* data;
  int ret = mModule->lock(mModule,
                            *buffer,
                            hnd->usage,
                            0,
                            0,
                            hnd->width,
                            hnd->height,
                            &data);
  if (ret) {
    HAL_LOGE("Failed to lock buffer: %d", ret);
    return ret;
  }
  HAL_LOGD("*camera_buffer->buffer:0x%x, stream->width:%d, stream->height:%d, lock buffer:%p.",
  *buffer, hnd->width, hnd->height, data);
  // If so, great, point to the beginning.
  if (!data) {
    ALOGE("Gralloc lock returned null ptr");
    return -ENODEV;
  }
  *dst_addr = data;
  HAL_LOGD("lock get buffer vaddr is %p",*dst_addr);*/
  return 0;
}

int V4L2Gralloc::unlock_handle(const buffer_handle_t* buffer) {
  int ret = -1;
  private_handle_t *hnd = (private_handle_t *)(*buffer);
  HAL_LOGV("buffer : %p,The id is %lld.", *buffer, hnd->aw_buf_id);
  /* let the graphics framework to lock the buffer, and get the virtual address. */
  const android::Rect rect(hnd->width, hnd->height);
  android::GraphicBufferMapper& grbuffer_mapper(android::GraphicBufferMapper::get());
  ret = grbuffer_mapper.unlock(*buffer);
  if (ret != 1) {
      HAL_LOGD("%s: grbuffer_mapper.unlock: %d -> %s",
        __FUNCTION__, ret, strerror(ret));
     //return false;
  }
  // Unlock.
  /*int res = mModule->unlock(mModule, *buffer);
  if (res) {
    HAL_LOGE("Failed to unlock buffer at %p", *buffer);
    return -ENODEV;
  }*/
  return 0;
}

int V4L2Gralloc::lock_buffer(const camera3_stream_buffer_t* camera_buffer,
                      unsigned long* device_buffer) {

  // Lock the camera buffer (varies depending on if the buffer is YUV or not).
  buffer_handle_t buffer = *camera_buffer->buffer;
  camera3_stream_t* stream = camera_buffer->stream;
  int ret = 0;
  private_handle_t *hnd = (private_handle_t *)buffer;

  HAL_LOGV("buffer_handle_t buffer : %p, hnd->height:%d, hnd->width:%d.", buffer, hnd->height, hnd->width);

  void* data;
  ret = mModule->lock(mModule,
                            buffer,
                            stream->usage,
                            0,
                            0,
                            stream->width,
                            stream->height,
                            &data);
  if (ret) {
    HAL_LOGE("Failed to lock buffer: %d", ret);
    return ret;
  }
  HAL_LOGV("*camera_buffer->buffer:0x%x, stream->width:%d, stream->height:%d, lock buffer:%p.",
  *camera_buffer->buffer, stream->width, stream->height, data);
  // If so, great, point to the beginning.
  if (!data) {
    ALOGE("Gralloc lock returned null ptr");
    return -ENODEV;
  }
  *device_buffer = reinterpret_cast<unsigned long>(data);
  HAL_LOGD("lock get buffer vaddr is %p",*device_buffer);
  return 0;
}

int V4L2Gralloc::unlock_buffer(const camera3_stream_buffer_t* camera_buffer) {
  buffer_handle_t buffer = *camera_buffer->buffer;

  private_handle_t *hnd = (private_handle_t *)buffer;
  HAL_LOGV("buffer : %p,The id is %lld.", buffer, hnd->aw_buf_id);

  // Unlock.
  int res = mModule->unlock(mModule, buffer);
  if (res) {
    HAL_LOGE("Failed to unlock buffer at %p", buffer);
    return -ENODEV;
  }

  return 0;
}

int V4L2Gralloc::lock_pic(const camera3_stream_buffer_t* camera_buffer,
                      unsigned long* device_buffer) {

  // Lock the camera buffer (varies depending on if the buffer is YUV or not).
  buffer_handle_t buffer = *camera_buffer->buffer;
  void* data;
  camera3_stream_t* stream = camera_buffer->stream;
  int ret = 0;
  HAL_LOGV("buffer_handle_t buffer : %p", buffer);

  void* nv21_data;
  ret = mModule->lock(mModule,
                            buffer,
                            stream->usage,
                            0,
                            0,
                            stream->width,
                            stream->height,
                            &nv21_data);
  if (ret) {
    HAL_LOGE("Failed to lock buffer: %d", ret);
    return ret;
  }
  HAL_LOGV("*camera_buffer->buffer:0x%x, stream->width:%d, stream->height:%d.",
  *camera_buffer->buffer, stream->width, stream->height);
  // If so, great, point to the beginning.
  data = nv21_data;
  HAL_LOGD("yuv_data:0x%x.", nv21_data);


  if (!data) {
    ALOGE("Gralloc lock returned null ptr");
    return -ENODEV;
  }

  // Set up the device buffer.
  static_assert(sizeof(unsigned long) >= sizeof(void*),
                "void* must be able to fit in the v4l2_buffer m.userptr "
                "field (unsigned long) for this code to work");

  *device_buffer = reinterpret_cast<unsigned long>(data);

  HAL_LOGD("lock get buffer vaddr is %p",*device_buffer);

  return 0;
}
int V4L2Gralloc::lock_index(const camera3_stream_buffer_t* camera_buffer,
                      unsigned long* device_buffer, uint32_t result_index) {

  // Lock the camera buffer (varies depending on if the buffer is YUV or not).
  buffer_handle_t buffer = *camera_buffer->buffer;
  void* data;
  camera3_stream_t* stream = camera_buffer->stream;
  int ret = 0;
  HAL_LOGV("buffer_handle_t buffer : %p", buffer);
  aBufferDataVideo[result_index] = buffer;

  void* nv21_data;
  ret = mModule->lock(mModule,
                            buffer,
                            stream->usage,
                            0,
                            0,
                            stream->width,
                            stream->height,
                            &nv21_data);
  if (ret) {
    HAL_LOGE("Failed to lock buffer: %d", ret);
    return ret;
  }
  HAL_LOGV("*camera_buffer->buffer:0x%x, stream->width:%d, stream->height:%d.",
  *camera_buffer->buffer, stream->width, stream->height);
  // If so, great, point to the beginning.
  data = nv21_data;

  if (!data) {
    ALOGE("Gralloc lock returned null ptr");
    return -ENODEV;
  }

  *device_buffer = reinterpret_cast<unsigned long>(data);
  HAL_LOGD("lock get buffer vaddr is %p",*device_buffer);

  return 0;
}

int V4L2Gralloc::unlock_index(uint32_t result_index) {
  const buffer_handle_t buffer = aBufferDataVideo[result_index];
  private_handle_t *hnd = (private_handle_t *)buffer;
  HAL_LOGV("buffer_handle_t[%d] buffer : %p,The id is %lld.", result_index, buffer, hnd->aw_buf_id);

  // Unlock.
  int res = mModule->unlock(mModule, buffer);
  if (res) {
    HAL_LOGE("Failed to unlock buffer at %p", buffer);
    return -ENODEV;
  }

  return 0;
}

int V4L2Gralloc::lock(const camera3_stream_buffer_t* camera_buffer,
                      uint32_t bytes_per_line,
                      v4l2_buffer* device_buffer) {
  // Lock the camera buffer (varies depending on if the buffer is YUV or not).
  std::shared_ptr<BufferData> buffer_data(
      new BufferData{camera_buffer, bytes_per_line});
  aBufferData[device_buffer->index];
  memcpy(aBufferData[device_buffer->index],buffer_data.get(),sizeof(BufferData));
  
  HAL_LOGD("aBufferData[%d]:%p, camera_buffer:%p.", device_buffer->index, 
  aBufferData[device_buffer->index],aBufferData[device_buffer->index]->camera_buffer);

  buffer_handle_t buffer = *camera_buffer->buffer;
  void* data;
  camera3_stream_t* stream = camera_buffer->stream;
  private_handle_t *hnd = (private_handle_t *)buffer;
  int ret = 0;
  HAL_LOGV("buffer_handle_t[%d] buffer : %p,The camera_buffer is %p,id is %lld.", device_buffer->index, buffer, camera_buffer, hnd->aw_buf_id);

  //TODOzjw: I do not know what the format gpu used.
  //I do not know what the chroma_step&uv_data.cr meant.
  switch (StreamFormat::HalToV4L2PixelFormat(stream->format)) {
    // TODOzjw: Care about difference between NV21/NV12/YV12/YV21.
    case V4L2_PIX_FMT_NV21:
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_YUV420:

      void* nv21_data;
      ret = mModule->lock(mModule,
                                buffer,
                                stream->usage,
                                0,
                                0,
                                stream->width,
                                stream->height,
                                &nv21_data);
      if (ret) {
        HAL_LOGE("Failed to lock buffer: %d", ret);
        return ret;
      }
      HAL_LOGV("*camera_buffer->buffer:0x%x, stream->width:%d, stream->height:%d, bytes_per_line:%d.",
      *camera_buffer->buffer, stream->width, stream->height, bytes_per_line);
      // If so, great, point to the beginning.
      data = nv21_data;
      HAL_LOGD("yuv_data:0x%x, bytes_per_line:%d.", nv21_data, bytes_per_line);
    break;
    // TODO(b/30119452): support more YCbCr formats.
      case V4L2_PIX_FMT_YVU420:
      android_ycbcr yuv_data;
      ret = mModule->lock_ycbcr(mModule,
                                buffer,
                                stream->usage,
                                0,
                                0,
                                stream->width,
                                stream->height,
                                &yuv_data);
      if (ret) {
        HAL_LOGE("Failed to lock ycbcr buffer: %d", ret);
        return ret;
      }
      HAL_LOGV("*camera_buffer->buffer:0x%x, stream->width:%d, stream->height:%d, bytes_per_line:%d.",
      *camera_buffer->buffer, stream->width, stream->height, bytes_per_line);
      HAL_LOGV("yuv_data.ystride:%d, yuv_data.cstride:%d, yuv_data.chroma_step:%d, yuv_data.y:0x%x, yuv_data.cb:0x%x, \
        yuv_data.cr:0x%x.",
        yuv_data.ystride, yuv_data.cstride, yuv_data.chroma_step,
        yuv_data.y, yuv_data.cb, yuv_data.cr);

      // Check if gralloc format matches v4l2 format
      // (same padding, not interleaved, contiguous).
      // In there yuv_data.chroma_step is difference.So ,what the yuv_data.chroma_step meant for buffer?
#if 0 //by zjw
      if (yuv_data.ystride == bytes_per_line &&
          yuv_data.cstride == bytes_per_line / 2 && yuv_data.chroma_step == 1 &&
          (reinterpret_cast<uint8_t*>(yuv_data.cb) ==
           reinterpret_cast<uint8_t*>(yuv_data.y) +
               (stream->height * yuv_data.ystride)) &&
          (reinterpret_cast<uint8_t*>(yuv_data.cr) ==
           reinterpret_cast<uint8_t*>(yuv_data.cb) +
               (stream->height / 2 * yuv_data.cstride))) {

        if (yuv_data.ystride == bytes_per_line  &&
            (reinterpret_cast<uint8_t*>(yuv_data.cb) ==
            reinterpret_cast<uint8_t*>(yuv_data.y) + (stream->height * yuv_data.ystride))) {
#endif
        if (1) {
        // If so, great, point to the beginning.
        data = yuv_data.y;
        HAL_LOGD("yuv_data.y:0x%x, bytes_per_line:%d.", yuv_data.y, bytes_per_line);
      } else {
        // If not, allocate a contiguous buffer of appropriate size
        // (to be transformed back upon unlock).
        data = new uint8_t[device_buffer->length];
        // Make a dynamically-allocated copy of yuv_data,
        // since it will be needed at transform time.
        //buffer_data->transform_dest.reset(new android_ycbcr(yuv_data));
      }
      break;
    case V4L2_PIX_FMT_JPEG:
      // Jpeg buffers are just contiguous blobs; lock length * 1.
      ret = mModule->lock(mModule,
                          buffer,
                          stream->usage,
                          0,
                          0,
                          device_buffer->length,
                          1,
                          &data);
      if (ret) {
        HAL_LOGE("Failed to lock jpeg buffer: %d", ret);
        return ret;
      }
      break;
    case V4L2_PIX_FMT_BGR32:  // Fall-through.
    case V4L2_PIX_FMT_RGB32:
      // RGB formats have nice agreed upon representation. Unless using android
      // flex formats.
      ret = mModule->lock(mModule,
                          buffer,
                          stream->usage,
                          0,
                          0,
                          stream->width,
                          stream->height,
                          &data);
      if (ret) {
        HAL_LOGE("Failed to lock RGB buffer: %d", ret);
        return ret;
      }
      break;
    default:
      return -EINVAL;
  }

  if (!data) {
    ALOGE("Gralloc lock returned null ptr");
    return -ENODEV;
  }

  // Set up the device buffer.
  static_assert(sizeof(unsigned long) >= sizeof(void*),
                "void* must be able to fit in the v4l2_buffer m.userptr "
                "field (unsigned long) for this code to work");

  if(V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == device_buffer->type) {
    // Note the mapping of index:buffer info for when unlock is called.
    //mBufferMap.emplace(device_buffer->index, buffer_data.get());

    device_buffer->m.planes[0].m.userptr = reinterpret_cast<unsigned long>(data);
    // Find and pop the matching entry in the map.
    //auto map_entry = mBufferMap.find(device_buffer->index);
    //if (map_entry == mBufferMap.end()) {
    //  HAL_LOGE("lock No matching buffer for data at %p", data);
    //}

  } else if(V4L2_BUF_TYPE_VIDEO_CAPTURE == device_buffer->type){
    // Note the mapping of index:buffer info for when unlock is called.
    //mBufferMap.emplace(device_buffer->index, buffer_data.get());

    device_buffer->m.userptr = reinterpret_cast<unsigned long>(data);
    // Find and pop the matching entry in the map.
    //auto map_entry = mBufferMap.find(device_buffer->index);
    //if (map_entry == mBufferMap.end()) {
    //  HAL_LOGE("lock No matching buffer for data at %p", data);
    //}
  }
  device_buffer->m.planes[0].m.userptr = reinterpret_cast<unsigned long>(data);

  HAL_LOGD("lock get index:%d buffer vaddr is %p", device_buffer->index, data);


  return 0;
}

int V4L2Gralloc::unlock(const v4l2_buffer* device_buffer) {
  // TODO(b/30000211): support multi-planar data (video_capture_mplane).
  if (device_buffer->type != V4L2_CAPTURE_TYPE) {
    return -EINVAL;
  }

  HAL_LOGV("unlock get index:%d buffer ", device_buffer->index);

  // Find and pop the matching entry in the map.
  //auto map_entry = mBufferMap.find(device_buffer->index);
  //if (map_entry == mBufferMap.end()) {
  //  HAL_LOGE("No matching buffer for data at %p", device_buffer->index);
    //return -EINVAL;
  //}
  HAL_LOGD("aBufferData[%d]:%p, camera_buffer:%p.", device_buffer->index, 
  aBufferData[device_buffer->index],aBufferData[device_buffer->index]->camera_buffer);

  //std::unique_ptr<const BufferData> buffer_data(map_entry->second);

  const camera3_stream_buffer_t* camera_buffer = aBufferData[device_buffer->index]->camera_buffer;

  //const camera3_stream_buffer_t* camera_buffer = buffer_data->camera_buffer;

  const buffer_handle_t buffer = *camera_buffer->buffer;

  //mBufferMap.erase(map_entry);

  private_handle_t *hnd = (private_handle_t *)buffer;
  HAL_LOGV("buffer_handle_t[%d] buffer : %p,The camera_buffer is %p,id is %lld, vaddr is %p.", device_buffer->index, buffer, camera_buffer, 
  hnd->aw_buf_id, hnd->base);
#if 0

  // Check for transform.
  if (buffer_data->transform_dest) {
    HAL_LOGV("Transforming V4L2 YUV to gralloc YUV.");
    // In this case data was allocated by this class, put it in a unique_ptr
    // to ensure it gets cleaned up no matter which way this function exits.
    //TODOzjw: get vaddr for this function.
    //void * data = NULL;
    //std::unique_ptr<uint8_t[]> data_cleanup(reinterpret_cast<uint8_t*>(data));

    uint32_t bytes_per_line = buffer_data->v4l2_bytes_per_line;
    android_ycbcr* yuv_data = buffer_data->transform_dest.get();

    // Should only occur in error situations.
    if (device_buffer->bytesused == 0) {
      return -EINVAL;
    }

    //TODOzjw: fellow the google deal.
    // Transform V4L2 to Gralloc, copying each plane to the correct place,
    // adjusting padding, and interleaving if necessary.

    uint32_t height = camera_buffer->stream->height;
    // Y data first.
    size_t y_len = bytes_per_line * height;
    if (yuv_data->ystride == bytes_per_line) {
      // Data should match exactly.
      memcpy(yuv_data->y, data, y_len);
    } else {
      HAL_LOGV("Changing padding on Y plane from %u to %u.",
               bytes_per_line,
               yuv_data->ystride);
      // Wrong padding from V4L2.
      copyWithPadding(reinterpret_cast<uint8_t*>(yuv_data->y),
                      reinterpret_cast<uint8_t*>(data),
                      yuv_data->ystride,
                      bytes_per_line,
                      height);
    }
    // C data.
    // TODO(b/30119452): These calculations assume YCbCr_420_888.
    size_t c_len = y_len / 4;
    uint32_t c_bytes_per_line = bytes_per_line / 2;
    // V4L2 is packed, meaning the data is stored as contiguous {y, cb, cr}.
    uint8_t* cb_device = reinterpret_cast<uint8_t*>(data) + y_len;
    uint8_t* cr_device = cb_device + c_len;
    size_t step = yuv_data->chroma_step;
    if (step == 1) {
      // Still planar.
      if (yuv_data->cstride == c_bytes_per_line) {
        // Data should match exactly.
        memcpy(yuv_data->cb, cb_device, c_len);
        memcpy(yuv_data->cr, cr_device, c_len);
      } else {
        HAL_LOGV("Changing padding on C plane from %u to %u.",
                 c_bytes_per_line,
                 yuv_data->cstride);
        // Wrong padding from V4L2.
        copyWithPadding(reinterpret_cast<uint8_t*>(yuv_data->cb),
                        cb_device,
                        yuv_data->cstride,
                        c_bytes_per_line,
                        height / 2);
        copyWithPadding(reinterpret_cast<uint8_t*>(yuv_data->cr),
                        cr_device,
                        yuv_data->cstride,
                        c_bytes_per_line,
                        height / 2);
      }
    } else {
      // Desire semiplanar (cb and cr interleaved).
      HAL_LOGV("Interleaving cb and cr. Padding going from %u to %u.",
               c_bytes_per_line,
               yuv_data->cstride);
      uint32_t c_height = height / 2;
      uint32_t c_width = camera_buffer->stream->width / 2;
      // Zero out destination
      uint8_t* cb_gralloc = reinterpret_cast<uint8_t*>(yuv_data->cb);
      uint8_t* cr_gralloc = reinterpret_cast<uint8_t*>(yuv_data->cr);
      memset(cb_gralloc, 0, c_width * c_height * step);

      // Interleaving means we need to copy the cb and cr bytes one by one.
      for (size_t line = 0; line < c_height; ++line,
                  cb_gralloc += yuv_data->cstride,
                  cr_gralloc += yuv_data->cstride,
                  cb_device += c_bytes_per_line,
                  cr_device += c_bytes_per_line) {
        for (size_t i = 0; i < c_width; ++i) {
          *(cb_gralloc + (i * step)) = *(cb_device + i);
          *(cr_gralloc + (i * step)) = *(cr_device + i);
        }
      }
    }

  }
#endif

  //HAL_LOGV("send buffer handle %p to unlock.", buffer);

  // Unlock.
  int res = mModule->unlock(mModule, buffer);
  if (res) {
    HAL_LOGE("Failed to unlock buffer at %p", buffer);
    return -ENODEV;
  }
  HAL_LOGD("The camera_buffer is %p,camera_buffer->buffer is %p", camera_buffer, camera_buffer->buffer);

  return 0;
}

int V4L2Gralloc::unlockAllBuffers() {
  HAL_LOG_ENTER();

  bool failed = false;
  //for (auto entry : mBufferMap) {
#if 0

    std::unique_ptr<const BufferData> buffer_data(entry.second);


    HAL_LOGV("The camera_buffer is %p.buffer_data->camera_buffer->buffer:%p",buffer_data->camera_buffer,buffer_data->camera_buffer->buffer);
    // TODOzjw: Find the ptr to handle release contrel. 
    if(buffer_data->camera_buffer->buffer == NULL) {
      HAL_LOGV("buffer_data->camera_buffer->buffer == NULL.");
      continue;
    }
    //const camera3_stream_buffer_t* camera_buffer = aBufferData[entry.first]->camera_buffer;

    const camera3_stream_buffer_t* camera_buffer = buffer_data->camera_buffer;
    HAL_LOGV("buffer_handle_t[%d] buffer : %p,The camera_buffer is %p.", entry.first, *camera_buffer->buffer, camera_buffer);

    const buffer_handle_t buffer = *camera_buffer->buffer;
    private_handle_t *hnd = (private_handle_t *)buffer;

    HAL_LOGV("buffer_handle_t[%d] buffer : %p,The camera_buffer is %p,id is %lld.", entry.first, buffer, camera_buffer, hnd->aw_buf_id);

    HAL_LOGD("Send index(%d) handle:%p to unlock.", entry.first,*camera_buffer->buffer);

    //int res = mModule->unlock(mModule, buffer);
   // if (res) {
    //  failed = true;
   // }
#endif
    // When there is a transform to be made, the buffer returned by lock()
    // is dynamically allocated (to hold the pre-transform data).
    //if (entry.second->transform_dest) {
      //TODOzjw: I changed the void* data to index.
      //delete[] reinterpret_cast<uint8_t*>(entry.first);
    //}
    // The BufferData entry is always dynamically allocated in lock().
   // delete entry.second;
 // }
  //mBufferMap.clear();
  //TODOzjw: fix unlock buffer
#if 0
  for(int i = 0; i< MAX_BUFFER_NUM; i++) {
    if(aBufferData[i]->camera_buffer == NULL) {
      HAL_LOGE("Index in %d point to NULL",i);
      break;
    }
    const camera3_stream_buffer_t* camera_buffer = aBufferData[i]->camera_buffer;
    const buffer_handle_t buffer = *camera_buffer->buffer;
    //private_handle_t *hnd = (private_handle_t *)buffer;

    int res = mModule->unlock(mModule, buffer);
    if (res) {
      failed = true;
    }
  }
#endif
  // If any unlock failed, return error.
  if (failed) {
    return -ENODEV;
  }

  return 0;
}

}  // namespace default_camera_hal
