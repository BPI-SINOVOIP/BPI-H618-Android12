
#ifndef V4L2_CAMERA_HAL_METADATA_H_
#define V4L2_CAMERA_HAL_METADATA_H_

#include <set>

#include "CameraMetadata.h"
#include <hardware/camera3.h>

#include "../common.h"
#include "metadata_common.h"

namespace v4l2_camera_hal {

using ::android::hardware::camera::common::V1_0::helper::CameraMetadata;

class Metadata {
 public:
  Metadata(PartialMetadataSet components);
  virtual ~Metadata();

  int FillStaticMetadata(CameraMetadata* metadata,int DeviceId,int FocusSupported,
      int CropRegionsSupported);
  bool IsValidRequest(const CameraMetadata& metadata);
  int GetRequestTemplate(int template_type,
                         CameraMetadata* template_metadata);
  int SetRequestSettings(const CameraMetadata& metadata);
  int FillResultMetadata(CameraMetadata* metadata);

 private:
  // The overall metadata is broken down into several distinct pieces.
  // Note: it is undefined behavior if multiple components share tags.
  PartialMetadataSet components_;

  DISALLOW_COPY_AND_ASSIGN(Metadata);
};

}  // namespace v4l2_camera_hal

#endif  // V4L2_CAMERA_HAL_V4L2_METADATA_H_
