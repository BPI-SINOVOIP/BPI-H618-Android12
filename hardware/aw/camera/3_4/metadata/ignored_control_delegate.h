
#ifndef V4L2_CAMERA_HAL_METADATA_IGNORED_CONTROL_DELEGATE_H_
#define V4L2_CAMERA_HAL_METADATA_IGNORED_CONTROL_DELEGATE_H_

#include "control_delegate_interface.h"

namespace v4l2_camera_hal {

// An IgnoredControlDelegate, as the name implies,
// has a fixed value and ignores all requests to set it.
template <typename T>
class IgnoredControlDelegate : public ControlDelegateInterface<T> {
 public:
  IgnoredControlDelegate(T value) : value_(value){};

  int GetValue(T* value) override {
    *value = value_;
    return 0;
  };
  int SetValue(const T& value) override { return 0; };

 private:
  const T value_;
};

}  // namespace v4l2_camera_hal

#endif  // V4L2_CAMERA_HAL_METADATA_IGNORED_CONTROL_DELEGATE_H_
