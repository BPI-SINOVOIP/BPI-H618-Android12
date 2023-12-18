
// Mock for state delegate interfaces.

#ifndef V4L2_CAMERA_HAL_METADATA_STATE_DELEGATE_INTERFACE_MOCK_H_
#define V4L2_CAMERA_HAL_METADATA_STATE_DELEGATE_INTERFACE_MOCK_H_

#include <gmock/gmock.h>

#include "state_delegate_interface.h"

namespace v4l2_camera_hal {

template <typename T>
class StateDelegateInterfaceMock : public StateDelegateInterface<T> {
 public:
  StateDelegateInterfaceMock(){};
  MOCK_METHOD1_T(GetValue, int(T*));
};

}  // namespace v4l2_camera_hal

#endif  // V4L2_CAMERA_HAL_METADATA_CONTROL_DELEGATE_INTERFACE_MOCK_H_
