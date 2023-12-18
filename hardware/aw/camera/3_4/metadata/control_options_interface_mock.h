
// Mock for control options interfaces.

#ifndef V4L2_CAMERA_HAL_METADATA_CONTROL_OPTIONS_INTERFACE_MOCK_H_
#define V4L2_CAMERA_HAL_METADATA_CONTROL_OPTIONS_INTERFACE_MOCK_H_

#include <gmock/gmock.h>

#include "control_options_interface.h"

namespace v4l2_camera_hal {

template <typename T>
class ControlOptionsInterfaceMock : public ControlOptionsInterface<T> {
 public:
  ControlOptionsInterfaceMock(){};
  MOCK_METHOD0_T(MetadataRepresentation, std::vector<T>());
  MOCK_METHOD1_T(IsSupported, bool(const T&));
  MOCK_METHOD2_T(DefaultValueForTemplate, int(int, T*));
};

}  // namespace v4l2_camera_hal

#endif  // V4L2_CAMERA_HAL_METADATA_CONTROL_OPTIONS_INTERFACE_MOCK_H_
