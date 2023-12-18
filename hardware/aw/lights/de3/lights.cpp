/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "lights"
#define LOGE ALOGE

#include <cutils/log.h>

#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <sys/types.h>

#include <hardware/lights.h>
#include <hardware/hardware.h>
#include <hardware/sunxi_display2.h>

#include <cutils/list.h>
#include <stdlib.h>
#include <sys/cdefs.h>
#include <cutils/properties.h>
#include <pthread.h>
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;


int fd = 0;

static int rgb_to_brightness(struct light_state_t const *state)
{
    int color = state->color & 0x00ffffff;
    return ((77 * ((color >> 16) & 0x00ff))
        + (150 * ((color >> 8) & 0x00ff)) +
        (29 * (color & 0x00ff))) >> 8;
}

static int set_light_backlight(struct light_device_t *,
                   struct light_state_t const *state)
{
    int brightness = rgb_to_brightness(state);
    int i = 0;
    unsigned long  args[3];
    int err = 0;

    pthread_mutex_lock(&g_lock);

    for(i = 0; i < 2; i++)
    {
        args[0] = i;
        if(ioctl(fd,DISP_GET_OUTPUT_TYPE,args) == DISP_OUTPUT_TYPE_LCD)
        {
            args[1]  = brightness;
            args[2]  = 0;
            err = ioctl(fd,DISP_LCD_SET_BRIGHTNESS,args);
            if (err < 0) {
                ALOGE("failed set brightness. %s.", strerror(errno));
            }

            break;
        }
    }

    pthread_mutex_unlock(&g_lock);
    return err;
}

/** Close the lights device */
static int close_lights(struct light_device_t *dev)
{
    if(fd != 0)
    {
        close(fd);
    }
    if (dev)
        free(dev);
    return 0;
}

/** Open a new instance of a lights device using name */
static int open_lights(const struct hw_module_t *module, char const *name,
               struct hw_device_t **device)
{
    int (*set_light) (struct light_device_t *dev,
              struct light_state_t const *state);

    if (0 == strcmp(LIGHT_ID_BACKLIGHT, name))
    {
        set_light = set_light_backlight;
        fd =  open("/dev/disp", O_RDONLY);
        if (fd < 0) {
            ALOGE("failed open disp device. %s.", strerror(errno));
        }
    }
    else
        return -EINVAL;

    pthread_mutex_init(&g_lock, NULL);
    struct light_device_t *dev = (struct light_device_t *)malloc(sizeof(struct light_device_t));
    memset(dev, 0, sizeof(*dev));

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = (struct hw_module_t *)module;
    dev->common.close = (int (*)(struct hw_device_t *))close_lights;
    dev->set_light = set_light;

    *device = (struct hw_device_t *)dev;

    return 0;
}

static struct hw_module_methods_t lights_methods =
{
    .open =  open_lights,
};

/*
 * The backlight Module
 */
struct hw_module_t HAL_MODULE_INFO_SYM =
{
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = LIGHTS_HARDWARE_MODULE_ID,
    .name = "SoftWinner lights Module",
    .author = "SOFTWINNER",
    .methods = &lights_methods,
};
