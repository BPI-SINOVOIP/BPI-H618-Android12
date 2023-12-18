
#include <errno.h>
#include <string.h>
#include <time.h>

#include "boottime_state_delegate.h"

namespace v4l2_camera_hal {

int BoottimeStateDelegate::GetValue(int64_t* value) {
  struct timespec ts;

  int res = clock_gettime(CLOCK_BOOTTIME, &ts);
  if (res) {
    HAL_LOGE("Failed to get BOOTTIME for state delegate: %d (%s)",
             errno,
             strerror(errno));
    return -errno;
  }
  *value = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
  HAL_LOGD("BoottimeStateDelegate::GetValue:%lld.", *value);

  return 0;
}

}  // namespace v4l2_camera_hal
