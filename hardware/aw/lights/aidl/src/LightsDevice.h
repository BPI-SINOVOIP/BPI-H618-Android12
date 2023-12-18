#pragma once

#include <hardware/lights.h>
#include <pthread.h>

namespace aidl {
namespace android {
namespace hardware {
namespace light {

struct DeviceInfo {
    int (*close) (struct DeviceInfo *device);
    int (*set_light) (struct DeviceInfo *device,
            struct light_state_t const *state);

    pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
    int fd = 0;
    int type = 0;
};

class LightsDevice {
public:
    static int open_lights (int type, struct DeviceInfo **device);
};

}  // namespace light
}  // namespace hardware
}  // namespace android
}  // namespace aidl
