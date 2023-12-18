
#include "format_metadata_factory.h"

#include "metadata/array_vector.h"
#include "metadata/partial_metadata_factory.h"
#include "metadata/property.h"

namespace v4l2_camera_hal {

static int GetHalFormats(const std::shared_ptr<V4L2Stream>& device,
                         std::set<int32_t>* result_formats) {
  if (!result_formats) {
    HAL_LOGE("Null result formats pointer passed");
    return -EINVAL;
  }

  std::set<uint32_t> v4l2_formats;
  int res = device->GetFormats(&v4l2_formats);
  if (res) {
    HAL_LOGE("Failed to get device formats.");
    return res;
  }
  for (auto v4l2_format : v4l2_formats) {
    int32_t hal_format = StreamFormat::V4L2ToHalPixelFormat(v4l2_format);
    if (hal_format < 0) {
      // Unrecognized/unused format. Skip it.
      continue;
    }
    result_formats->insert(hal_format);
  }

  // In addition to well-defined formats, there may be an
  // "Implementation Defined" format chosen by the HAL (in this
  // case what that means is managed by the StreamFormat class).

  // Get the V4L2 format for IMPLEMENTATION_DEFINED.
  int v4l2_format = StreamFormat::HalToV4L2PixelFormat(
      HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED);
  // If it's available, add IMPLEMENTATION_DEFINED to the result set.
  if (v4l2_format && v4l2_formats.count(v4l2_format) > 0) {
    result_formats->insert(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED);
  }

  return 0;
}

int AddFormatComponents(
    std::shared_ptr<V4L2Stream> device,
    std::insert_iterator<PartialMetadataSet> insertion_point,
    std::shared_ptr<CCameraConfig> pCameraCfg) {
  HAL_LOG_ENTER();

  // Get all supported formats.
  std::set<int32_t> hal_formats;
  int res = GetHalFormats(device, &hal_formats);
  if (res) {
    return res;
  }

  // Requirements check: need to support YCbCr_420_888, JPEG,
  // and "Implementation Defined".
  // There are the difference from google HAL v3, since allwinner need the 
  // HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED(NV21), but HAL_PIXEL_FORMAT_YCbCr_420_888.
  if (hal_formats.find(HAL_PIXEL_FORMAT_YCrCb_420_SP) == hal_formats.end()) {
    HAL_LOGE("YCrCb_420_SP(NV21) not supported by device.");
    //return -ENODEV;
  } else if (hal_formats.find(HAL_PIXEL_FORMAT_BLOB) == hal_formats.end()) {
    HAL_LOGE("JPEG not supported by device.");
    //TODO: Support JPEG format in Camera HAL 3
    //return -ENODEV;
  } else if (hal_formats.find(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED) ==
             hal_formats.end()) {
    HAL_LOGE("HAL implementation defined format not supported by device.");
    return -ENODEV;
  }

  // Find sizes and frame/stall durations for all formats.
  // We also want to find the smallest max frame duration amongst all formats,
  // And the largest min frame duration amongst YUV (i.e. largest max frame rate
  // supported by all YUV sizes).
  // Stream configs are {format, width, height, direction} (input or output).
  ArrayVector<int32_t, 4> stream_configs;
  // Frame durations are {format, width, height, duration} (duration in ns).
  ArrayVector<int64_t, 4> min_frame_durations;
  // Stall durations are {format, width, height, duration} (duration in ns).
  ArrayVector<int64_t, 4> stall_durations;
  int64_t min_max_frame_duration = std::numeric_limits<int64_t>::max();
  int64_t max_min_frame_duration_yuv = std::numeric_limits<int64_t>::min();
  for (auto hal_format : hal_formats) {
    // Get the corresponding V4L2 format.
    uint32_t v4l2_format = StreamFormat::HalToV4L2PixelFormat(hal_format);
    if (v4l2_format == 0) {
      // Unrecognized/unused format. Should never happen since hal_formats
      // came from translating a bunch of V4L2 formats above.
      HAL_LOGE("Couldn't find V4L2 format for HAL format %d", hal_format);
      return -ENODEV;
    }

    // Get the available sizes for this format, sort by decresing.
    std::set<std::array<int32_t, 2>, std::greater<std::array<int32_t, 2>>> frame_sizes;
    //device->GetDeviceId() == DEVICE_FACING_BACK
    HAL_LOGD("Camera Facing :%d.",device->GetDeviceId());
    res = device->GetFormatFrameSizes(v4l2_format, &frame_sizes);
    if (res) {
      HAL_LOGE("Failed to get all frame sizes for format %d", v4l2_format);
      return res;
    }

    for (const auto& frame_size : frame_sizes) {
      //TODOzjw:acheive HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED in csi driver
      //now we directly changed the HAL_PIXEL_FORMAT_YCbCr_420_888 to HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED.
      //Why do we support HAL_PIXEL_FORMAT_YCbCr_420_888 will encount wrong in framework?
      //hal_format = HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED;

      // Note the format and size combination in stream configs.
      stream_configs.push_back(
          {{hal_format,
            frame_size[0],
            frame_size[1],
            ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT}});
      HAL_LOGV("Push info in stream_configs,hal_format:%d,frame_size[0]:%d,frame_size[1]:%d",hal_format,frame_size[0],frame_size[1]);

      // Find the duration range for this format and size.
      std::array<int64_t, 2> duration_range;
      res = device->GetFormatFrameDurationRange(
          v4l2_format, frame_size, &duration_range);
      if (res) {
        HAL_LOGE(
            "Failed to get frame duration range for format %d, "
            "size %u x %u",
            v4l2_format,
            frame_size[0],
            frame_size[1]);
        return res;
      }
      //int64_t size_min_frame_duration = duration_range[0];
      //int64_t size_max_frame_duration = duration_range[1];
      int64_t size_min_frame_duration = duration_range[0];
      int64_t size_max_frame_duration = duration_range[1];

      min_frame_durations.push_back({{hal_format,
                                      frame_size[0],
                                      frame_size[1],
                                      size_min_frame_duration}});

      // Note the stall duration for this format and size.
      // Usually 0 for non-jpeg, non-zero for JPEG.
      // Randomly choosing absurd 1 sec for JPEG. Unsure what this breaks.
      int64_t stall_duration = 0;
      if (hal_format == HAL_PIXEL_FORMAT_BLOB) {
        stall_duration = 1000000000;
      }
      stall_durations.push_back(
          {{hal_format, frame_size[0], frame_size[1], stall_duration}});

      // Update our search for general min & max frame durations.
      // In theory max frame duration (min frame rate) should be consistent
      // between all formats, but we check and only advertise the smallest
      // available max duration just in case.
      if (size_max_frame_duration < min_max_frame_duration) {
        min_max_frame_duration = size_max_frame_duration;
      }
      // We only care about the largest min frame duration
      // (smallest max frame rate) for YUV sizes.
      if (hal_format == HAL_PIXEL_FORMAT_YCrCb_420_SP &&
          size_min_frame_duration > max_min_frame_duration_yuv) {
        max_min_frame_duration_yuv = size_min_frame_duration;
      }
    }
  }

  // Convert from frame durations measured in ns.
  // Min fps supported by all formats.
  //int32_t min_fps = 1000000000 / min_max_frame_duration;
  int32_t min_fps = 10;
  if (min_fps > 15) {
    HAL_LOGE("Minimum FPS %d is larger than HAL max allowable value of 15",
             min_fps);
    return -ENODEV;
  }
  // Max fps supported by all YUV formats.
  //int32_t max_yuv_fps = 1000000000 / max_min_frame_duration_yuv;
  int32_t max_yuv_fps = 30;
  // ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES should be at minimum
  // {mi, ma}, {ma, ma} where mi and ma are min and max frame rates for
  // YUV_420_888. Min should be at most 15.Max should be at least 24.
  std::vector<std::array<int32_t, 2>> fps_ranges;
  //fps_ranges.push_back({{min_fps, max_yuv_fps}});
  // 15, 24
  HAL_LOGD("zjw,min_fps:%d,max_yuv_fps:%d", min_fps, max_yuv_fps);

  int32_t support_frame_rate = 0;
  if (pCameraCfg != NULL)
  {
      support_frame_rate = pCameraCfg->getSupportFrameRate();
      fps_ranges.push_back({{15, support_frame_rate}});
      fps_ranges.push_back({{support_frame_rate, support_frame_rate}});
  }

  /*if(device->GetDeviceId() == DEVICE_FACING_FRONT) {
    fps_ranges.push_back({{10, 24}});
    fps_ranges.push_back({{24, 24}});
  } else {
    fps_ranges.push_back({{10, 24}});
    fps_ranges.push_back({{24, 24}});
  }*/

  std::array<int32_t, 2> video_fps_range;
  int32_t video_fps = support_frame_rate;
  video_fps_range = {{video_fps, video_fps}};


  #if 0
  HAL_LOGE("video_fps_range[0]:%d,video_fps_range[1]:%d.",
             video_fps_range[0],video_fps_range[1]);
  fps_ranges.push_back(video_fps_range);
  
  HAL_LOGE("fps_ranges[0]:%d,fps_ranges[1]:%d.",
             fps_ranges[0][0],fps_ranges[0][1]);
  #endif
  // Construct the metadata components.
  insertion_point = std::make_unique<Property<ArrayVector<int32_t, 4>>>(
      ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS,
      std::move(stream_configs));
  insertion_point = std::make_unique<Property<ArrayVector<int64_t, 4>>>(
      ANDROID_SCALER_AVAILABLE_MIN_FRAME_DURATIONS,
      std::move(min_frame_durations));
  insertion_point = std::make_unique<Property<ArrayVector<int64_t, 4>>>(
      ANDROID_SCALER_AVAILABLE_STALL_DURATIONS, std::move(stall_durations));
  insertion_point = std::make_unique<Property<int64_t>>(
      ANDROID_SENSOR_INFO_MAX_FRAME_DURATION, min_max_frame_duration);
  // TODO(b/31019725): This should probably not be a NoEffect control.
  insertion_point = NoEffectMenuControl<std::array<int32_t, 2>>(
      ANDROID_CONTROL_AE_TARGET_FPS_RANGE,
      ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES,
      fps_ranges,
      {{CAMERA3_TEMPLATE_VIDEO_RECORD, video_fps_range},
       {OTHER_TEMPLATES, fps_ranges[0]}});

  return 0;
}

}  // namespace v4l2_camera_hal
