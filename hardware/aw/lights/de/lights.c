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
#include <cutils/properties.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <time.h>

#include <hardware/lights.h>
#include <hardware/hardware.h>
#include <hardware/drv_display.h>

#define LED_BRIGHTNESS_OFF 0
#define LED_BRIGHTNESS_MAX 255

#define ALPHA_MASK 0xff000000
#define COLOR_MASK 0x00ffffff

#define NSEC_PER_MSEC 1000000ULL
#define NSEC_PER_SEC 1000000000ULL
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static unsigned int leds_exist = 0x00000000;

const char *const LED_DIR = "/sys/class/leds";
const char *const LED_BRIGHTNESS_FILE = "brightness";
const char *const LED_DELAY_ON_FILE = "delay_on";
const char *const LED_DELAY_OFF_FILE = "delay_off";
const char *const LED_TRIGGER_FILE = "trigger";
const char* const led_name[3] = {"red_led","green_led","blue_led"};

enum LED_NAME {
        RED,
        GREEN,
        BLUE,
};

enum LED_STATE {
        OFF,
        ON,
        BLINK,
};

struct sunxi_led_info {
        unsigned int color;
        unsigned int delay_on;
        unsigned int delay_off;
        enum LED_STATE state[3];
};

#define LIGHT_PATH_NOTIFICATION  "/sys/class/leds/sunxi_respiration_lamp/ctrl"
int fd =0, notify_fd = 0;

static int rgb_to_brightness(struct light_state_t const *state)
{
	int color = state->color & 0x00ffffff;
	return ((77 * ((color >> 16) & 0x00ff))
		+ (150 * ((color >> 8) & 0x00ff)) +
		(29 * (color & 0x00ff))) >> 8;
}

static int set_light_backlight(struct light_device_t *dev,
			       struct light_state_t const *state)
{
    struct light_context_t      *ctx;

    int err = 0;

    int brightness = rgb_to_brightness(state);
	
    pthread_mutex_lock(&g_lock);	
    unsigned long  args[3];

    args[0]  = 0;
    args[1]  = brightness;
    args[2]  = 0;
    err = ioctl(fd,DISP_CMD_LCD_SET_BRIGHTNESS,args);

    pthread_mutex_unlock(&g_lock);
    return err;
}

static void time_add(struct timespec *time, int sec, int nsec)
{
        time->tv_nsec += nsec;
        time->tv_sec += time->tv_nsec / NSEC_PER_SEC;
        time->tv_nsec %= NSEC_PER_SEC;
        time->tv_sec += sec;
}

static bool time_after(struct timespec *t)
{
        struct timespec now;

        clock_gettime(CLOCK_MONOTONIC, &now);
        return now.tv_sec > t->tv_sec || (now.tv_sec == t->tv_sec && now.tv_nsec > t->tv_nsec);
}

static int led_sysfs_write(char *buf, const char *path, char *format, ...)
{
        int fd;
        int err;
        int len;
        va_list args;
        struct timespec timeout;
        int ret;
        
        //ALOGE("path=%s\n", path);

        clock_gettime(CLOCK_MONOTONIC, &timeout);
        time_add(&timeout, 0, 100 * NSEC_PER_MSEC);

        do {
                fd = open(path, O_WRONLY);
                err = -errno;
                if (fd < 0) {
                        if (errno != EINTR && errno != EACCES && time_after(&timeout)) {
                                ALOGE("failed to open %s!", path);
                                return err;
                }
                sched_yield();
                }
        } while (fd < 0);

        va_start(args, format);
        len = vsprintf(buf, format, args);
        va_end(args);
        //ALOGE("%s %d buf=%s\n", __FUNCTION__,__LINE__,buf);
        if (len < 0)
                return len;

        err = write(fd, buf, len);
        if (err == -1)
                return -errno;

        err = close(fd);
        if (err == -1)
                return -errno;

        return 0;
}

static int write_leds(struct sunxi_led_info *leds)
{
        char buf[20];
        int err = 0;
        enum LED_NAME i = RED;
        char path_name[PATH_MAX];

        pthread_mutex_lock(&g_lock);
        for(i = RED;i <= BLUE;i++){
                if(((leds_exist >> (16-8*i)) & 0x000000ff) == 0)
                        continue;
                switch(leds->state[i]) {
                        case OFF:
                                sprintf(path_name, "%s/%s/%s", LED_DIR,led_name[i],LED_BRIGHTNESS_FILE);
                                err = led_sysfs_write(buf,path_name, "%d",LED_BRIGHTNESS_OFF);
                                if (err)
                                        goto err_write_fail;
                                err = sprintf(path_name, "%s/%s/%s", LED_DIR,led_name[i],LED_TRIGGER_FILE);
                                err = led_sysfs_write(buf, path_name, "%s", "none");
                                if (err)
                                        goto err_write_fail;
                                break;
                        case BLINK:
                                err = sprintf(path_name, "%s/%s/%s", LED_DIR,led_name[i],LED_TRIGGER_FILE);
                                err = led_sysfs_write(buf, path_name, "%s", "timer");
                                if (err)
                                        goto err_write_fail;
                                property_set("sys.lights_leds", "1");
                                usleep(100000);
                                sprintf(path_name, "%s/%s/%s", LED_DIR,led_name[i],LED_DELAY_ON_FILE);
                                err = led_sysfs_write(buf, path_name, "%u", leds->delay_on);
                                if (err)
                                        goto err_write_fail;
                                sprintf(path_name, "%s/%s/%s", LED_DIR,led_name[i],LED_DELAY_OFF_FILE);
                                err = led_sysfs_write(buf, path_name, "%u", leds->delay_off);
                                if (err)
                                        goto err_write_fail;
                        case ON:
                                sprintf(path_name, "%s/%s/%s", LED_DIR,led_name[i],LED_BRIGHTNESS_FILE);
                                err = led_sysfs_write(buf, path_name, "%d",LED_BRIGHTNESS_MAX);
                                if (err)
                                        goto err_write_fail;
                        default:
                                break;
                }
        }

err_write_fail:
        pthread_mutex_unlock(&g_lock);
        return err;
}

static int set_light_leds(struct light_state_t const *state, int type)
{
        struct sunxi_led_info leds;
        unsigned int color;
        unsigned int ledcolor[3];
        enum LED_NAME i = RED;

        memset(&leds, 0, sizeof(leds));
        if (state->flashOnMS < 0 || state->flashOffMS < 0)
        	return -EINVAL;

        leds.delay_off = state->flashOffMS;
        leds.delay_on = state->flashOnMS;

        if (!state->color) {
        	leds.state[RED] = OFF;
        	leds.state[GREEN] = OFF;
        	leds.state[BLUE] = OFF;
        }

        color = state->color & COLOR_MASK;
        
        ledcolor[RED] = (color >> 16) & 0x000000ff;
        ledcolor[GREEN] = (color >> 8) & 0x000000ff;
        ledcolor[BLUE] = color & 0x000000ff;

        for(i = RED;i <= BLUE;i++){
                if (ledcolor[i] == 0) {
                	leds.state[i] = OFF;
                }
                else{
                        if (state->flashMode != LIGHT_FLASH_NONE && leds.delay_on && leds.delay_off)
                                leds.state[i] = BLINK;
                        else
                                leds.state[i] = ON;
                }
                ALOGE("%s set leds state=%d!flashMode=%d color=0x%x offMS=%d onMS=%d\n", __FUNCTION__,leds.state[i],state->flashMode,state->color,state->flashOffMS,state->flashOnMS);
        }
        return write_leds(&leds);
}

static int set_light_leds_battery(struct light_device_t *dev,
			struct light_state_t const *state)
{
        return set_light_leds(state, 0);
}

static int set_light_leds_notifications(struct light_device_t *dev,
			struct light_state_t const *state)
{
	int i = 0, fd = 0;
	unsigned long  args[3];
	int err = 0;

	pthread_mutex_lock(&g_lock);

	args[0] = state->flashOnMS;
	args[1] = state->flashOffMS;
	args[2] = state->color;

	LOGE("state->flashOnMS = %d\n", state->flashOnMS);
	LOGE("state->flashOffMS = %d\n", state->flashOffMS);
	LOGE("state->color = %d\n", state->color);

	if(write(notify_fd, args, 12) < 0)
	{
		LOGE("set_light_notification --> write LED args failed.\n");
	}

	pthread_mutex_unlock(&g_lock);
	return err;
}

static int set_light_leds_attention(struct light_device_t *dev,
			struct light_state_t const *state)
{
//        return set_light_leds(state, 1);
	  return 0;
}

static int porbe_light_leds(void)
{
        char path[PATH_MAX];
        int fd = 123,err;
        enum LED_NAME i = RED;

        if(leds_exist & 0xff000000)
                return leds_exist;
        else
                leds_exist |= 0xff000000;

        property_set("sys.lights_leds", "1");
        ALOGE("%s leds_exist=%u\n", __FUNCTION__,leds_exist);
        for(i = RED;i <= BLUE;i++){
                sprintf(path, "%s/%s/%s", LED_DIR,led_name[i],LED_BRIGHTNESS_FILE);
                ALOGE("%s path=%s\n", __FUNCTION__,path);
                fd = open(path,O_WRONLY);
                err = -errno;
                if(fd != -1){
                        leds_exist |= 0xff<<(16-8*i);
                        close(fd);
                        fd = -1;
                }
                else{
                        leds_exist |= 0x0<<(16-8*i);
                }
        }
        ALOGE("%s leds_exist=%u\n", __FUNCTION__,leds_exist);
        return leds_exist;
}

/** Close the lights device */
static int close_lights(struct light_device_t *dev)
{
	if(fd != 0)
	{	
		close(fd);
	}
        if (notify_fd != 0)
        {
		close(notify_fd);
        }
	if (dev)
		free(dev);
	return 0;
}

/** Open a new instance of a lights device using name */
static int open_lights(const struct hw_module_t *module, char const *name,
		       struct hw_device_t **device)
{
	pthread_t lighting_poll_thread;

	int (*set_light) (struct light_device_t *dev,
			  struct light_state_t const *state);

	if (0 == strcmp(LIGHT_ID_BACKLIGHT, name))
	{
		set_light = set_light_backlight;
		fd =  open("/dev/disp", O_RDONLY);
		if(fd < 0)
			ALOGE("#################1##################");
	}
        else if (0 == strcmp(LIGHT_ID_NOTIFICATIONS, name))
        {
                set_light = set_light_leds_notifications;
                notify_fd =  open(LIGHT_PATH_NOTIFICATION, O_RDWR);
                if(fd < 0)
                        ALOGE("#####Open Notification LED Fail#####");
        }
	else{
    	if(porbe_light_leds() & 0x00ffffff){
			if (strcmp(LIGHT_ID_NOTIFICATIONS, name) == 0)
				set_light = set_light_leds_notifications;
			else if (strcmp(LIGHT_ID_ATTENTION, name) == 0)
				set_light = set_light_leds_attention;
			else if (strcmp(LIGHT_ID_BATTERY, name) == 0)
				set_light = set_light_leds_battery;
			else
				return -EINVAL;	
		}
		else
			return -EINVAL;
    }

	pthread_mutex_init(&g_lock, NULL);
	struct light_device_t *dev = malloc(sizeof(struct light_device_t));
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
