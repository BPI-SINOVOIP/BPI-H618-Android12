/*
 * multi ir daemon for android -
 * Copyright (C) 2015-2018 AllwinnerTech
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/inotify.h>
#include <sys/poll.h>
#include <utils/Log.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/types.h>
#include <errno.h>

#include "multiir.h"
#include "virtual_input.h"

#define	uinput_debug(fmt, arg...) 				\
do { 											\
	if (print_flags & PRINT_DEBUG)				\
		fprintf(stdout, "[D] " fmt"\n", ##arg);	\
} while(0)

#define	uinput_warning(fmt, arg...) 			\
do { 											\
	if (print_flags & PRINT_WARN)				\
		fprintf(stdout, "[W] " fmt"\n", ##arg);	\
} while(0)

#define	uinput_error(fmt, arg...) 				\
do { 											\
	if (print_flags & PRINT_ERROR)				\
		fprintf(stderr, "[E] " fmt"\n", ##arg);	\
} while(0)

enum {
    PRINT_ERROR	 = 1U << 0,
    PRINT_WARN	 = 1U << 1,
    PRINT_DEBUG	 = 1U << 2,
};

#define UINPUT_NAME		"/dev/uinput"
static int g_uinputfd = -1;
int g_mousefd = -1;
static int print_flags = 0;

static struct uinput_user_dev virtual_inputdev;
static struct uinput_user_dev virtual_mousedev;


int nfds = 0;

static int is_devname_match(const char *devpath, const char *tname)
{
	int fd;
	char name[80];

	if (!devpath || !tname)
		return 0;

	fd = open(devpath, O_RDWR);
	if (fd<0) {
		uinput_error("could not open '%s', %s",
			devpath, strerror(errno));
		return 0;
	}
	name[sizeof(name)-1] = '\0';
	if(ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
		uinput_error("could not get device name for '%s', %s",
			devpath, strerror(errno));
		name[0] = '\0';
		goto notmatch;
	}
	if (!strcmp(name, tname)) {
		uinput_debug("find match device '%s'", devpath);
		close(fd);
		return 1;
	}

notmatch:
	close(fd);
	uinput_debug("device not match or ioctl error, %s(%s)", devpath, name);
	return 0;
}

int try_to_find_device(const char *devname, char *path)
{
	const char *dirname = "/dev/input";
    char devpath[PATH_MAX];
    char *p;
    DIR *dir;
    struct dirent *de;

	if (!devname || !path)
		return -1;

	path[0] = '\0';
    dir = opendir(dirname);
    if(dir == NULL)
        return -1;
    strcpy(devpath, dirname);
    p = devpath + strlen(devpath);
    *p++ = '/';
    while((de = readdir(dir))) {
        if(de->d_name[0] == '.' &&
           (de->d_name[1] == '\0' ||
            (de->d_name[1] == '.' && de->d_name[2] == '\0')))
            continue;

		strcpy(p, de->d_name);
		if (is_devname_match(devpath, devname)) {
			strcpy(path, devpath);
			break;
		}
    }
    closedir(dir);
    return 0;
}

struct inputdev_info *open_device(const char *devpath, struct pollfd *ufds)
{
	struct inputdev_info *info;

	if (!devpath)
		goto errout;

	info = (inputdev_info *)calloc(sizeof(*info), sizeof(char));
	if (!info) {
		uinput_error("could not malloc, %s", strerror(errno));
		goto errout;
	}

	info->fd = open(devpath, O_RDWR);
	if (info->fd < 0) {
		uinput_error("could not open device '%s', %s",
			devpath, strerror(errno));
		free(info);
		goto errout;
	}

	strcpy(info->devpath, devpath);
	if (ioctl(info->fd, EVIOCGVERSION, &info->version)) {
		uinput_warning("could not get driver version for '%s', %s",
			devpath, strerror(errno));
	}
	if (ioctl(info->fd, EVIOCGID, &info->id)) {
		uinput_warning("could not get driver id for '%s', %s",
			devpath, strerror(errno));
    }

	if (ioctl(info->fd, EVIOCGNAME(sizeof(info->name)-1), info->name) < 1) {
		uinput_warning("could not get device name for %s, %s",
			devpath, strerror(errno));
	}
	if (ioctl(info->fd, EVIOCGPHYS(sizeof(info->location)-1), info->location) < 1) {
		uinput_warning("could not get location for %s, %s",
			devpath, strerror(errno));
	}
	if (ioctl(info->fd, EVIOCGUNIQ(sizeof(info->idstr) - 1), info->idstr) < 1) {
		uinput_warning("could not get idstring for %s, %s",
			devpath, strerror(errno));
    }

	if (print_flags & PRINT_DEBUG) {
		fprintf(stdout, "    devpath:  %s\n", info->devpath);
		fprintf(stdout, "    bus:      %04x\n"
					 	"    vendor    %04x\n"
						"    product   %04x\n"
						"    version   %04x\n",
						info->id.bustype, info->id.vendor,
						info->id.product, info->id.version);
		fprintf(stdout, "    name:     \"%s\"\n", info->name);
		fprintf(stdout, "    location: \"%s\"\n"
						"    id:       \"%s\"\n", info->location, info->idstr);
		fprintf(stdout, "    version:  %d.%d.%d\n",
						info->version >> 16,
						(info->version >> 8) & 0xff,
						info->version & 0xff);
	}

	ufds[INPUT_FD_INDEX].fd = info->fd;
	ufds[INPUT_FD_INDEX].events = POLLIN;
	nfds++;
	uinput_debug("add device '%s' to poll", devpath);
	return info;
errout:
	uinput_error("could open device '%s'!", devpath);
	return NULL;
}

static void close_device(struct inputdev_info * sourcedev, const char *devpath, struct pollfd *ufds)
{
	if (!sourcedev)
		return;

	if (!strcmp(sourcedev->devpath, devpath)) {
		close(sourcedev->fd);
		free(sourcedev);
		sourcedev = NULL;

		nfds--;
		ufds[nfds].fd = -1;
		ufds[nfds].events = 0;
		uinput_debug("close device '%s'", devpath);
	} else {
		uinput_debug("device '%s' not open, ignore", devpath);
	}
}

int notify_process(struct inputdev_info * sourcedev, char *device_name, const char *dirname, int nfd, struct pollfd *ufds)
{
	int res;
	char devpath[PATH_MAX];
	char *p;
	char event_buf[512];
	int event_size;
	int event_pos = 0;
	struct inotify_event *event;

	res = read(nfd, event_buf, sizeof(event_buf));
	if (res < (int)sizeof(*event)) {
		if (errno == EINTR)
			return 0;
		uinput_error("could not get event, %s", strerror(errno));
		return 1;
	}
	uinput_debug("got %d bytes of event information", res);

	strcpy(devpath, dirname);
	p = devpath + strlen(devpath);
	*p++ = '/';

	while(res >= (int)sizeof(*event)) {
		event = (struct inotify_event *)(event_buf + event_pos);
		uinput_debug("%d: %08x \"%s\"",
			event->wd, event->mask, event->len ? event->name : "");

		if (event->len) {
			strcpy(p, event->name);
			if (event->mask & IN_CREATE) {
				if (!sourcedev && is_devname_match(devpath, device_name)) {
					uinput_debug("find target device, path=%s", devpath);
					sourcedev = open_device(devpath, ufds);
				}
			} else {
				close_device(sourcedev, devpath, ufds);
			}
		}

		event_size = sizeof(*event) + event->len;
		res -= event_size;
		event_pos += event_size;
	}

	return 0;
}

int input_event_process(int nfd, struct input_event *event)
{
	int res;

	res = read(nfd, event, sizeof(struct input_event));
	if (res < (int)sizeof(struct input_event)) {
		uinput_error("could not get event");
		return -1;
	}

//	ALOGD("%04x %04x %08x", event->type, event->code, event->value);
	return 0;
}

int create_virtual_mouse_dev(char *mouse_dev) {
    int uinputfd = -1;

    if (g_mousefd >= 0) {
        ALOGD("multiir had enter mouse mode");
        return 0;
    }

    ALOGD("multi_ir open uinput dev");

    uinputfd = open(UINPUT_NAME, O_WRONLY | O_NDELAY);
    if (uinputfd <= 0) {
        ALOGE("could not open '%s', %s\n", UINPUT_NAME, strerror(errno));
        return uinputfd;
    }

    memset(&virtual_mousedev, 0, sizeof(virtual_mousedev));
    strncpy(virtual_mousedev.name, mouse_dev, UINPUT_MAX_NAME_SIZE);
    virtual_mousedev.id.version = 0x0111;
    virtual_mousedev.id.bustype = BUS_USB;
    virtual_mousedev.id.vendor = 0x093a;
    virtual_mousedev.id.product = 0x2521;

    /* setup mouse coordinate event */
    if (ioctl(uinputfd, UI_SET_EVBIT,  EV_REL) < 0) ALOGE("set evbit rel fail : %s", strerror(errno));
    if (ioctl(uinputfd, UI_SET_EVBIT,  EV_KEY) < 0) ALOGE("set evbit key fail : %s", strerror(errno));
    if (ioctl(uinputfd, UI_SET_RELBIT, REL_X) < 0) ALOGE("set relbit x fail : %s", strerror(errno));
    if (ioctl(uinputfd, UI_SET_RELBIT, REL_Y) < 0) ALOGE("set relbit y fail : %s", strerror(errno));

    /* setup mouse button event */
    if (ioctl(uinputfd, UI_SET_KEYBIT, BTN_MOUSE) < 0) ALOGE("set keybit mouse fail : %s", strerror(errno));

    /* create input device into input subsystem */
    write(uinputfd, &virtual_mousedev, sizeof(virtual_mousedev));
    if (ioctl(uinputfd, UI_DEV_CREATE)) {
        ALOGE("could not create uinput device, %s\n", strerror(errno));
        close(uinputfd);
        uinputfd = -1;
        return uinputfd;
    }

    g_mousefd = uinputfd;
    setMouseMode(1);
    return 0;
}

int destory_virtual_mouse_dev(void) {
    if (g_mousefd == -1) {
        ALOGD("multiir had exit mouse mode");
        return 0;
    }

    if (ioctl(g_mousefd, UI_DEV_DESTROY)) {
        ALOGD("could not destory virtual mouse: %s", strerror(errno));
        close(g_mousefd);
        g_mousefd = -1;
        return -1;
    }
    close(g_mousefd);
    g_mousefd = -1;
    setMouseMode(0);
    return 0;
}

int setup_virtual_input_dev(char *inputdev_name)
{
    int i;
    int uinputfd = -1;

    ALOGD("multi_ir open uinput dev");

    uinputfd = open(UINPUT_NAME, O_WRONLY | O_NDELAY);
    if (uinputfd <= 0) {
        ALOGE("could not open '%s', %s\n", UINPUT_NAME, strerror(errno));
        return uinputfd;
    }

    memset(&virtual_inputdev, 0, sizeof(virtual_inputdev));
    strncpy(virtual_inputdev.name, inputdev_name, UINPUT_MAX_NAME_SIZE);
    virtual_inputdev.id.version = 4;
    virtual_inputdev.id.bustype = BUS_HOST;
    ioctl(uinputfd, UI_SET_EVBIT, EV_KEY);
   	ioctl(uinputfd, UI_SET_EVBIT, EV_MSC);
   	ioctl(uinputfd, UI_SET_EVBIT, EV_REP);

    /* register a cursor device */
    ioctl(uinputfd, UI_SET_KEYBIT, BTN_MOUSE);
    ioctl(uinputfd, UI_SET_RELBIT, REL_X);
    ioctl(uinputfd, UI_SET_RELBIT, REL_Y);
    ioctl(uinputfd, UI_SET_EVBIT,  EV_REL);
#if 0
	/* setup mouse coordinate event */
    ioctl(uinputfd, UI_SET_EVBIT,  EV_REL);
    ioctl(uinputfd, UI_SET_RELBIT, REL_X);
    ioctl(uinputfd, UI_SET_RELBIT, REL_Y);

    /* setup mouse button event */
    ioctl(uinputfd, UI_SET_KEYBIT, BTN_MOUSE);
    ioctl(uinputfd, UI_SET_KEYBIT, BTN_TOUCH);
    ioctl(uinputfd, UI_SET_KEYBIT, BTN_LEFT);
    ioctl(uinputfd, UI_SET_KEYBIT, BTN_RIGHT);
    ioctl(uinputfd, UI_SET_KEYBIT, BTN_MIDDLE);
#endif

    for (i = KEYCODE_MIN; i < KEYCODE_CNT; i++)
        ioctl(uinputfd, UI_SET_KEYBIT, i);

    /* create input device into input subsystem */
    write(uinputfd, &virtual_inputdev, sizeof(virtual_inputdev));
    if (ioctl(uinputfd, UI_DEV_CREATE)) {
        ALOGE("could not create uinput device, %s\n", strerror(errno));
        close(uinputfd);
        uinputfd = -1;
        return uinputfd;
    }

    g_uinputfd = uinputfd;
    return uinputfd;
}

void report_key_event(int uinputfd, struct input_event *event, int sync)
{
    struct input_event temp_event;

    write(uinputfd, event, sizeof(struct input_event));

    if (sync) {
        memset(&temp_event, 0, sizeof(struct input_event));
        temp_event.type = EV_SYN;
        temp_event.code = SYN_REPORT;
        temp_event.value = 0;
        write(uinputfd, &temp_event, sizeof(struct input_event));
    }
}

#define POWER_KEY	(0x74)
void report_standby_request(void)
{
	int uinputfd = g_uinputfd;
	struct input_event event;

	if (uinputfd<0)
		return;

	memset(&event, 0, sizeof(event));
	gettimeofday(&event.time, NULL);
	event.type = EV_KEY;
	event.code = POWER_KEY;
	event.value = 1;
	write(uinputfd, &event, sizeof(event));

	event.type = EV_SYN;
	event.code = SYN_REPORT;
	event.value = 0;
	write(uinputfd, &event, sizeof(event));

	memset(&event, 0, sizeof(event));
	gettimeofday(&event.time, NULL);
	event.type = EV_KEY;
	event.code = POWER_KEY;
	event.value = 0;
	write(uinputfd, &event, sizeof(event));

	event.type = EV_SYN;
	event.code = SYN_REPORT;
	event.value = 0;
	write(uinputfd, &event, sizeof(event));
}
