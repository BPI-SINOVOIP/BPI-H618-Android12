
#ifndef V4L2_CAMERA_HAL_FUNCTION_THREAD_H_
#define V4L2_CAMERA_HAL_FUNCTION_THREAD_H_

#include <functional>

#include <utils/Thread.h>

#include "common.h"

namespace v4l2_camera_hal {

class FunctionThread : public android::Thread {
 public:
  FunctionThread(std::function<bool()> function) : function_(function){};

 private:
  bool threadLoop() override {
    bool result = function_();
    return result;
  };

  std::function<bool()> function_;
};

}  // namespace v4l2_camera_hal

#endif  // V4L2_CAMERA_HAL_FUNCTION_THREAD_H_
