
#ifndef V4L2_CAMERA_HAL_METADATA_DEFAULT_OPTION_DELEGATE_H_
#define V4L2_CAMERA_HAL_METADATA_DEFAULT_OPTION_DELEGATE_H_

#include <errno.h>

#include <map>

#include <hardware/camera3.h>

namespace v4l2_camera_hal {

// A constant that can be used to identify an overall default.
static constexpr int OTHER_TEMPLATES = CAMERA3_TEMPLATE_COUNT;

// DefaultingOptionDelegate provides an interface to get default options from.
template <typename T>
class DefaultOptionDelegate {
 public:
  // |defaults| maps template types to default values
  DefaultOptionDelegate(std::map<int, T> defaults)
      : defaults_(std::move(defaults)){};
  virtual ~DefaultOptionDelegate(){};

  // Get a default value for a template type. Returns false if no default
  // provided.
  virtual bool DefaultValueForTemplate(int template_type, T* default_value) {
    if (defaults_.count(template_type) > 0) {
      // Best option is template-specific.
      *default_value = defaults_[template_type];
      return true;
    } else if (defaults_.count(OTHER_TEMPLATES)) {
      // Fall back to a general default.
      *default_value = defaults_[OTHER_TEMPLATES];
      return true;
    }

    return false;
  };

 private:
  std::map<int, T> defaults_;
};

}  // namespace v4l2_camera_hal

#endif  // V4L2_CAMERA_HAL_METADATA_DEFAULT_OPTION_DELEGATE_H_
