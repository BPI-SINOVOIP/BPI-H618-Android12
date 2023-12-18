
#include "stream_format.h"

#include <linux/videodev2.h>

#include <system/graphics.h>

#include "common.h"

namespace v4l2_camera_hal {

StreamFormat::StreamFormat(int format, uint32_t width, uint32_t height)
    // TODO(b/30000211): multiplanar support.
    : type_(V4L2_CAPTURE_TYPE),
      memory_(V4L2_MEMORY_DMABUF),
      v4l2_pixel_format_(StreamFormat::HalToV4L2PixelFormat(format)),
      width_(width),
      height_(height),
      //TODOzjw: Be compatibility with GPU, DE, VE, CSI, ISP.
      //Now we set the bytes_per_line_ as width because the drivers return this one.
      bytes_per_line_(width),
      min_buffer_size_(0) {}

StreamFormat::StreamFormat(const v4l2_format& format)
    //TODOzjw:be compatibility with multiplanar and single planar.
    : type_(format.type),
      memory_(V4L2_MEMORY_DMABUF),
      nplanes_(format.fmt.pix_mp.num_planes),
      // TODO(b/30000211): multiplanar support.
      v4l2_pixel_format_(format.fmt.pix_mp.pixelformat),
      width_(format.fmt.pix_mp.width),
      height_(format.fmt.pix_mp.height),
      bytes_per_line_(format.fmt.pix_mp.width),
      min_buffer_size_(0)
      //bytes_per_line_(format.fmt.pix_mp.bytesperline),
      //min_buffer_size_(format.fmt.pix_mp.sizeimage)
      {}

void StreamFormat::FillFormatRequest(v4l2_format* format) const {
  memset(format, 0, sizeof(*format));
  format->type = type_;
  format->fmt.pix_mp.pixelformat = v4l2_pixel_format_;
  format->fmt.pix_mp.width = width_;
  format->fmt.pix_mp.height = height_;
  // Bytes per line and min buffer size are outputs set by the driver,
  // not part of the request.
}

FormatCategory StreamFormat::Category() const {
  switch (v4l2_pixel_format_) {
    case V4L2_PIX_FMT_JPEG:
      return kFormatCategoryStalling;
    case V4L2_PIX_FMT_YUV420:  // Fall through.
    case V4L2_PIX_FMT_BGR32:
      return kFormatCategoryNonStalling;
    default:
      // Note: currently no supported RAW formats.
      return kFormatCategoryUnknown;
  }
}

bool StreamFormat::operator==(const StreamFormat& other) const {
  // Used to check that a requested format was actually set, so
  // don't compare bytes per line or min buffer size.
  return (type_ == other.type_ &&
          v4l2_pixel_format_ == other.v4l2_pixel_format_ &&
          width_ == other.width_ && height_ == other.height_);
}

bool StreamFormat::operator!=(const StreamFormat& other) const {
  return !(*this == other);
}

int StreamFormat::V4L2ToHalPixelFormat(uint32_t v4l2_pixel_format) {
  // Translate V4L2 format to HAL format.
  int hal_pixel_format = -1;
  switch (v4l2_pixel_format) {
    case V4L2_PIX_FMT_JPEG:
      hal_pixel_format = HAL_PIXEL_FORMAT_BLOB;
      break;
    case V4L2_PIX_FMT_NV21:
      //hal_pixel_format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
      hal_pixel_format = HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED;
      HAL_LOGD("Get v4l2 pixel format %u from csi driver, we set the format as HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED for GPU.", v4l2_pixel_format);
      break;
    case V4L2_PIX_FMT_NV12:
      hal_pixel_format = HAL_PIXEL_FORMAT_YCbCr_420_888;
      HAL_LOGD("Get NV12 from csi driver, transfor to HAL_PIXEL_FORMAT_YCbCr_420_888.");
      break;
    case V4L2_PIX_FMT_YUV420:
      hal_pixel_format = HAL_PIXEL_FORMAT_YCbCr_420_888;
      break;
    case V4L2_PIX_FMT_BGR32:
      hal_pixel_format = HAL_PIXEL_FORMAT_RGBA_8888;
      break;
    default:
      // Unrecognized format.
      HAL_LOGV("Unrecognized v4l2 pixel format %u", v4l2_pixel_format);
      break;
  }
  return hal_pixel_format;
}

uint32_t StreamFormat::HalToV4L2PixelFormat(int hal_pixel_format) {
  // Translate HAL format to V4L2 format.
  uint32_t v4l2_pixel_format = 0;
  switch (hal_pixel_format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
      // Should be RGB32, but RPi doesn't support that.
      // For now we accept that the colors will be off.
      v4l2_pixel_format = V4L2_PIX_FMT_BGR32;
      break;
      // Set the default format between CSI and GPU-gralloc with HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED.
      // In CSI we see the HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED as V4L2_PIX_FMT_NV21.
      // In GPU they see the HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED as HAL_PIXEL_FORMAT_YCrCb_420_SP.
      // PS: In CSI V4L2_PIX_FMT_YVU420 equals YV12.
      //    In CSI V4L2_PIX_FMT_YUV420 equals YV21.
      //keep the HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED for MPLANE.
    case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:  // fall-through.
      v4l2_pixel_format = V4L2_PIX_FMT_NV21;
      break;
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
      v4l2_pixel_format = V4L2_PIX_FMT_NV21;
      break;
    // TODOzjw: Why the framewroks received V4L2_PIX_FMT_NV12 and see them as HAL_PIXEL_FORMAT_YCbCr_420_888?
    case HAL_PIXEL_FORMAT_YCbCr_420_888:
      v4l2_pixel_format = V4L2_PIX_FMT_NV21;
      break;
    case HAL_PIXEL_FORMAT_BLOB:
      v4l2_pixel_format = V4L2_PIX_FMT_JPEG;
      break;
    default:
      // Unrecognized format.
      HAL_LOGV("Unrecognized HAL pixel format %d", hal_pixel_format);
      break;
  }
  HAL_LOGD("HAL pixel format %d.", hal_pixel_format);
  return v4l2_pixel_format;
}

}  // namespace v4l2_camera_hal
