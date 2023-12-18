
// Mock for default option delegates.

#ifndef V4L2_CAMERA_HAL_METADATA_DEFAULT_OPTION_DELEGATE_MOCK_H_
#define V4L2_CAMERA_HAL_METADATA_DEFAULT_OPTION_DELEGATE_MOCK_H_

#include <gmock/gmock.h>

#include "default_option_delegate.h"

namespace v4l2_camera_hal {

template <typename T>
class DefaultOptionDelegateMock : public DefaultOptionDelegate<T> {
 public:
  DefaultOptionDelegateMock() : DefaultOptionDelegate<T>({}){};
  MOCK_METHOD2_T(DefaultValueForTemplate, bool(int, T*));
};

}  // namespace v4l2_camera_hal

#endif  // V4L2_CAMERA_HAL_METADATA_DEFAULT_OPTION_DELEGATE_MOCK_H_
