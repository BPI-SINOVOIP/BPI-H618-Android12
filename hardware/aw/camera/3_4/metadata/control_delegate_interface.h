
#ifndef V4L2_CAMERA_HAL_METADATA_CONTROL_DELEGATE_INTERFACE_H_
#define V4L2_CAMERA_HAL_METADATA_CONTROL_DELEGATE_INTERFACE_H_

#include "state_delegate_interface.h"

namespace v4l2_camera_hal {

// A ControlDelegate extends StateDelegate with a setter method.
template <typename T>
class ControlDelegateInterface : public StateDelegateInterface<T> {
 public:
  virtual ~ControlDelegateInterface(){};

  // ControlDelegates are allowed to be unreliable, so SetValue is best-effort;
  // GetValue immediately after may not match (SetValue may, for example,
  // automatically replace invalid values with valid ones,
  // or have a delay before setting the requested value).
  // Returns 0 on success, error code on failure.
  virtual int SetValue(const T& value) = 0;
  // Children must also override GetValue from StateDelegateInterface.
};

}  // namespace v4l2_camera_hal

#endif  // V4L2_CAMERA_HAL_METADATA_CONTROL_DELEGATE_INTERFACE_H_
