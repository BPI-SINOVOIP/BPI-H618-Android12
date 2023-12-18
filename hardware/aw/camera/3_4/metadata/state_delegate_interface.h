
#ifndef V4L2_CAMERA_HAL_METADATA_STATE_DELEGATE_INTERFACE_H_
#define V4L2_CAMERA_HAL_METADATA_STATE_DELEGATE_INTERFACE_H_

namespace v4l2_camera_hal {

// A StateDelegate is simply a dynamic value that can be queried.
// The value may change between queries.
template <typename T>
class StateDelegateInterface {
 public:
  virtual ~StateDelegateInterface(){};
  // Returns 0 on success, error code on failure.
  virtual int GetValue(T* value) = 0;
};

}  // namespace v4l2_camera_hal

#endif  // V4L2_CAMERA_HAL_METADATA_STATE_DELEGATE_INTERFACE_H_
