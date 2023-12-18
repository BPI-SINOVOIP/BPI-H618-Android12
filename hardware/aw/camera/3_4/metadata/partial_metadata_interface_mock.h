
// Mock for partial metadata interfaces.

#ifndef V4L2_CAMERA_HAL_PARTIAL_METADATA_INTERFACE_MOCK_H_
#define V4L2_CAMERA_HAL_PARTIAL_METADATA_INTERFACE_MOCK_H_

#include <gmock/gmock.h>

#include "partial_metadata_interface.h"

namespace v4l2_camera_hal {

using ::android::hardware::camera::common::V1_0::helper::CameraMetadata;

class PartialMetadataInterfaceMock : public PartialMetadataInterface {
 public:
  PartialMetadataInterfaceMock() : PartialMetadataInterface(){};
  MOCK_CONST_METHOD0(StaticTags, std::vector<int32_t>());
  MOCK_CONST_METHOD0(ControlTags, std::vector<int32_t>());
  MOCK_CONST_METHOD0(DynamicTags, std::vector<int32_t>());
  MOCK_CONST_METHOD1(PopulateStaticFields, int(CameraMetadata*));
  MOCK_CONST_METHOD1(PopulateDynamicFields, int(CameraMetadata*));
  MOCK_CONST_METHOD2(PopulateTemplateRequest,
                     int(int, CameraMetadata*));
  MOCK_CONST_METHOD1(SupportsRequestValues,
                     bool(const CameraMetadata&));
  MOCK_METHOD1(SetRequestValues, int(const CameraMetadata&));
};

}  // namespace v4l2_camera_hal

#endif  // V4L2_CAMERA_HAL_PARTIAL_METADATA_INTERFACE_MOCK_H_
