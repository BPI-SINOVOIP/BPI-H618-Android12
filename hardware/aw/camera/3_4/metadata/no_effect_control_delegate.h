
#ifndef V4L2_CAMERA_HAL_METADATA_NO_EFFECT_CONTROL_DELEGATE_H_
#define V4L2_CAMERA_HAL_METADATA_NO_EFFECT_CONTROL_DELEGATE_H_

#include "control_delegate_interface.h"

namespace v4l2_camera_hal {

// A NoEffectControlDelegate, as the name implies, has no effect.
// The value can be gotten and set, but it does nothing.
template <typename T>
class NoEffectControlDelegate : public ControlDelegateInterface<T> {
 public:
  NoEffectControlDelegate(T default_value) : value_(default_value){};

  int GetValue(T* value) override {
    *value = value_;
    return 0;
  };
  int SetValue(const T& value) override {
    value_ = value;
    return 0;
  };

 private:
  T value_;
};

}  // namespace v4l2_camera_hal

#endif  // V4L2_CAMERA_HAL_METADATA_NO_EFFECT_CONTROL_DELEGATE_H_
