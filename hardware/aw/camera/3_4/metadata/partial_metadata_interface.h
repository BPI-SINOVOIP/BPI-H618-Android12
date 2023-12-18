
#ifndef V4L2_CAMERA_HAL_METADATA_PARTIAL_METADATA_INTERFACE_H_
#define V4L2_CAMERA_HAL_METADATA_PARTIAL_METADATA_INTERFACE_H_

#include <array>
#include <vector>

#include "../common.h"
#include "array_vector.h"
#include "CameraMetadata.h"

namespace v4l2_camera_hal {

using ::android::hardware::camera::common::V1_0::helper::CameraMetadata;

// A subset of metadata.
class PartialMetadataInterface {
 public:
  virtual ~PartialMetadataInterface(){};

  // The metadata tags this partial metadata is responsible for.
  // See system/media/camera/docs/docs.html for descriptions of each tag.
  virtual std::vector<int32_t> StaticTags() const = 0;
  virtual std::vector<int32_t> ControlTags() const = 0;
  virtual std::vector<int32_t> DynamicTags() const = 0;

  // Add all the static properties this partial metadata
  // is responsible for to |metadata|.
  virtual int PopulateStaticFields(CameraMetadata* metadata) const = 0;
  // Add all the dynamic states this partial metadata
  // is responsible for to |metadata|.
  virtual int PopulateDynamicFields(
      CameraMetadata* metadata) const = 0;
  // Add default request values for a given template type for all the controls
  // this partial metadata owns.
  virtual int PopulateTemplateRequest(
      int template_type, CameraMetadata* metadata) const = 0;
  // Check if the requested control values from |metadata| (for controls
  // this partial metadata owns) are supported. Empty/null values for owned
  // control tags indicate no change, and are thus inherently supported.
  // If |metadata| is empty all controls are implicitly supported.
  virtual bool SupportsRequestValues(
      const CameraMetadata& metadata) const = 0;
  // Set all the controls this partial metadata
  // is responsible for from |metadata|. Empty/null values for owned control
  // tags indicate no change. If |metadata| is empty no controls should
  // be changed.
  virtual int SetRequestValues(const CameraMetadata& metadata) = 0;
};

}  // namespace v4l2_camera_hal

#endif  // V4L2_CAMERA_HAL_METADATA_PARTIAL_METADATA_INTERFACE_H_
