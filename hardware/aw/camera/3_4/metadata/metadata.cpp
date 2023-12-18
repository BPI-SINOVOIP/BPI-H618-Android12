
#include "metadata.h"

#include <hardware/camera3.h>

#include "../common.h"
#include "metadata_common.h"
#include "CameraMetadata.h"
#include "partial_metadata_factory.h"

namespace v4l2_camera_hal {

Metadata::Metadata(PartialMetadataSet components)
    : components_(std::move(components)) {
  HAL_LOG_ENTER();
}

Metadata::~Metadata() {
  HAL_LOG_ENTER();
}

int Metadata::FillStaticMetadata(CameraMetadata* metadata,int DeviceId,int FocusSupported,
    int CropRegionsSupported) {
  HAL_LOG_ENTER();
  if (!metadata) {
    HAL_LOGE("Can't fill null metadata.");
    return -EINVAL;
  }

  std::vector<int32_t> static_tags;
  std::vector<int32_t> control_tags;
  std::vector<int32_t> dynamic_tags;
  int res = 0;

  for (auto& component : components_) {
    // Prevent components from potentially overriding others.
    CameraMetadata additional_metadata;
    // Populate the fields.
    res = component->PopulateStaticFields(&additional_metadata);
    if (res) {
      HAL_LOGE("Failed to get all static properties.");
      return res;
    }
    // Add it to the overall result.
    if (!additional_metadata.isEmpty()) {
      res = metadata->append(additional_metadata);
      if (res != android::OK) {
        HAL_LOGE("Failed to append all static properties.");
        return res;
      }
    }

    // Note what tags the component adds.
    std::vector<int32_t> tags = component->StaticTags();
    //从这里来看，tags已经将内容放到了static_tags里面。详细用法不懂，后续再跟进掌握。
    std::move(tags.begin(),
              tags.end(),
              std::inserter(static_tags, static_tags.end()));
    tags = component->ControlTags();
    std::move(tags.begin(),
              tags.end(),
              std::inserter(control_tags, control_tags.end()));
    tags = component->DynamicTags();
    std::move(tags.begin(),
              tags.end(),
              std::inserter(dynamic_tags, dynamic_tags.end()));
  }

  int32_t tagsjpeg[3] = {ANDROID_JPEG_ORIENTATION, ANDROID_JPEG_QUALITY, ANDROID_JPEG_THUMBNAIL_QUALITY};
  std::vector<int32_t> tags_jpeg(tagsjpeg, tagsjpeg +3);

  std::move(tags_jpeg.begin(),
            tags_jpeg.end(),
            std::inserter(control_tags, control_tags.end()));
  std::move(tags_jpeg.begin(),
            tags_jpeg.end(),
            std::inserter(dynamic_tags, dynamic_tags.end()));

  if(DeviceId == 0 && FocusSupported == 1)
  {
      HAL_LOGV("####push_back ANDROID_CONTROL_AF_REGIONS DeviceId:%d",DeviceId);
      control_tags.push_back(ANDROID_CONTROL_AF_REGIONS);
      dynamic_tags.push_back(ANDROID_CONTROL_AF_REGIONS);
  }
  else
  {
      HAL_LOGV("####!!!!not push_back ANDROID_CONTROL_AF_REGIONS DeviceId:%d",DeviceId);
  }

  if(/*DeviceId == 0 && */CropRegionsSupported == 1)
  {
      HAL_LOGV("####push_back ANDROID_SCALER_CROP_REGION DeviceId:%d",DeviceId);
      control_tags.push_back(ANDROID_SCALER_CROP_REGION);
      dynamic_tags.push_back(ANDROID_SCALER_CROP_REGION);
  }
  else
  {
      HAL_LOGE("####!!!!not push_back ANDROID_SCALER_CROP_REGION DeviceId:%d",DeviceId);
  }

  // Populate the meta fields.
  static_tags.push_back(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS);
  res = UpdateMetadata(
      metadata, ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS, control_tags);

  if (res != android::OK) {
    HAL_LOGE("Failed to add request keys meta key.");
    return -ENODEV;
  }
  static_tags.push_back(ANDROID_REQUEST_AVAILABLE_RESULT_KEYS);
  res = UpdateMetadata(
      metadata, ANDROID_REQUEST_AVAILABLE_RESULT_KEYS, dynamic_tags);
  if (res != android::OK) {
    HAL_LOGE("Failed to add result keys meta key.");
    return -ENODEV;
  }
  static_tags.push_back(ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS);
  res = UpdateMetadata(
      metadata, ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS, static_tags);
  if (res != android::OK) {
    HAL_LOGE("Failed to add characteristics keys meta key.");
    return -ENODEV;
  }

  // TODO(b/31018853): cache result.
  return 0;
}

bool Metadata::IsValidRequest(const CameraMetadata& metadata) {
  HAL_LOG_ENTER();

  // Empty means "use previous settings", which are inherently valid.
  if (metadata.isEmpty())
    return true;

  for (auto& component : components_) {
    // Check that all components support the values requested of them.
    bool valid_request = component->SupportsRequestValues(metadata);
    if (!valid_request) {
      //TODO:Calvin, fix this bug.
      continue;
      // Exit early if possible.
      //return false;
    }
  }

  return true;
}

int Metadata::GetRequestTemplate(int template_type,
                                 CameraMetadata* template_metadata) {
  HAL_LOG_ENTER();
  if (!template_metadata) {
    HAL_LOGE("Can't fill null template.");
    return -EINVAL;
  }

  // Templates are numbered 1 through COUNT-1 for some reason.
  if (template_type < 1 || template_type >= CAMERA3_TEMPLATE_COUNT) {
    HAL_LOGE("Unrecognized template type %d.", template_type);
    return -EINVAL;
  }

  for (auto& component : components_) {
    // Prevent components from potentially overriding others.
    CameraMetadata additional_metadata;
    int res =
        component->PopulateTemplateRequest(template_type, &additional_metadata);
    if (res) {
      HAL_LOGE("Failed to get all default request fields.");
      return res;
    }
    // Add it to the overall result.
    if (!additional_metadata.isEmpty()) {
      res = template_metadata->append(additional_metadata);
      if (res != android::OK) {
        HAL_LOGE("Failed to append all default request fields.");
        return res;
      }
    }
  }

  // TODO(b/31018853): cache result.
  return 0;
}

int Metadata::SetRequestSettings(const CameraMetadata& metadata) {
  HAL_LOG_ENTER();

  // Empty means "use previous settings".
  if (metadata.isEmpty())
    return 0;

  for (auto& component : components_) {
    int res = component->SetRequestValues(metadata);
    if (res) {
      HAL_LOGE("Failed to set all requested settings.");
      return res;
    }
  }

  return 0;
}

int Metadata::FillResultMetadata(CameraMetadata* metadata) {
  HAL_LOG_ENTER();
  if (!metadata) {
    HAL_LOGE("Can't fill null metadata.");
    return -EINVAL;
  }

  for (auto& component : components_) {
    // Prevent components from potentially overriding others.
    CameraMetadata additional_metadata;
    int res = component->PopulateDynamicFields(&additional_metadata);
    if (res) {
      HAL_LOGE("Failed to get all dynamic result fields.");
      return res;
    }
    // Add it to the overall result.
    if (!additional_metadata.isEmpty()) {
      res = metadata->append(additional_metadata);
      if (res != android::OK) {
        HAL_LOGE("Failed to append all dynamic result fields.");
        return res;
      }
    }
  }

  return 0;
}

}  // namespace v4l2_camera_hal
