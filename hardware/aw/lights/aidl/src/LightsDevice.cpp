
#define LOG_TAG "lights_device"
#define LOGE ALOGE

#include <cutils/log.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <hardware/sunxi_display2.h>

#include <aidl/android/hardware/light/LightType.h>

#include "LightsDevice.h"

namespace aidl {
namespace android {
namespace hardware {
namespace light {

static int rgb_to_brightness(struct light_state_t const *state)
{
    int color = state->color & 0x00ffffff;
    return ((77 * ((color >> 16) & 0x00ff))
        + (150 * ((color >> 8) & 0x00ff)) +
        (29 * (color & 0x00ff))) >> 8;
}

static int set_light_backlight(struct DeviceInfo *device,
                   struct light_state_t const *state)
{
    int brightness = rgb_to_brightness(state);
    int err = 0;
#ifdef USE_EINK_BACKLIGHT
    char buffer[20];
    pthread_mutex_lock(&device->g_lock);
    int bytes = sprintf(buffer, "%d\n", brightness);
    err = write(device->fd, &buffer, bytes);
    if (err == bytes)
        err = 0;
    else
        ALOGE("set backlight fail!:%s", strerror(errno));
#else
    unsigned long args[3];
    int i = 0;
    pthread_mutex_lock(&device->g_lock);
    for (i = 0; i < 2; i++) {
        args[0] = i;
        if (ioctl(device->fd, DISP_GET_OUTPUT_TYPE, args) ==
            DISP_OUTPUT_TYPE_LCD) {
            args[1] = brightness;
            args[2] = 0;
            err = ioctl(device->fd, DISP_LCD_SET_BRIGHTNESS, args);
            if (err < 0) {
                ALOGE("failed set brightness. %s.", strerror(errno));
            }

            break;
        }
    }

#endif

    pthread_mutex_unlock(&device->g_lock);
    return err;
}

/** Close the lights device */
static int close_lights(struct DeviceInfo *dev)
{
    if(dev->fd != 0)
    {
        close(dev->fd);
        ALOGD("close disp device for type=%d", dev->type);
    }
    if (dev)
        free(dev);
    return 0;
}


/** Open a new instance of a lights device using type */
int LightsDevice::open_lights (int type, struct DeviceInfo **device) {
    int (*set_light) (struct DeviceInfo *device,
            struct light_state_t const *state);
    int fd = 0;

    switch ((LightType)type) {
        case LightType::BACKLIGHT:
            set_light = set_light_backlight;
#ifdef USE_EINK_BACKLIGHT
            fd = open("/sys/class/backlight/eink/brightness", O_RDWR);
#else
            fd = open("/dev/disp", O_RDONLY);
#endif
            if (fd < 0) {
                ALOGE("failed open disp device. %s.", strerror(errno));
            } else {
                ALOGD("success open disp device type=%d.", type);
            }
            break;
        default:
            return -1;
    }

    struct DeviceInfo *dev = (struct DeviceInfo *)malloc(sizeof(struct DeviceInfo));
    memset(dev, 0, sizeof(*dev));
    dev->type = type;
    dev->fd = fd;
    dev->close = close_lights;
    dev->set_light = set_light;
    pthread_mutex_init(&dev->g_lock, NULL);

    *device = (struct DeviceInfo *)dev;

    return 0;
}

}  // namespace light
}  // namespace hardware
}  // namespace android
}  // namespace aidl
