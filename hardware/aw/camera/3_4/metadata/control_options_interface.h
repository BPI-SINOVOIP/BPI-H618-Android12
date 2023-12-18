
#ifndef V4L2_CAMERA_HAL_METADATA_CONTROL_OPTIONS_INTERFACE_H_
#define V4L2_CAMERA_HAL_METADATA_CONTROL_OPTIONS_INTERFACE_H_

#include <vector>

namespace v4l2_camera_hal {

// A ControlOptions defines acceptable values for a control.
template <typename T>
class ControlOptionsInterface {
 public:
  virtual ~ControlOptionsInterface(){};

  // Get a metadata-acceptable representation of the options.
  // For enums this will be a list of values, for ranges this
  // will be min and max, etc.
  virtual std::vector<T> MetadataRepresentation() = 0;
  // Get whether or not a given value is acceptable.
  virtual bool IsSupported(const T& option);
  // Get a default option for a given template type, from the available options.
  // Because a default must be available, any ControlOptions should have at
  // least one supported value.
  virtual int DefaultValueForTemplate(int template_type, T* default_value);
};

}  // namespace v4l2_camera_hal

#endif  // V4L2_CAMERA_HAL_METADATA_CONTROL_OPTIONS_INTERFACE_H_
