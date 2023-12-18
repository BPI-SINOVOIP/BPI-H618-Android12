
#ifndef V4L2_CAMERA_HAL_METADATA_BOOTTIME_STATE_DELEGATE_H_
#define V4L2_CAMERA_HAL_METADATA_BOOTTIME_STATE_DELEGATE_H_

#include "../common.h"
#include "state_delegate_interface.h"

namespace v4l2_camera_hal {

// A StateDelegate is simply a dynamic value that can be queried.
// The value may change between queries.
class BoottimeStateDelegate : public StateDelegateInterface<int64_t> {
 public:
  BoottimeStateDelegate(){};
  ~BoottimeStateDelegate(){};

  int GetValue(int64_t* value) override;
};

}  // namespace v4l2_camera_hal

#endif  // V4L2_CAMERA_HAL_METADATA_BOOTTIME_STATE_DELEGATE_H_
