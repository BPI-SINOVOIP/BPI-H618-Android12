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

#ifndef  __virtual_input_h
#define  __virtual_input_h

#include <linux/input.h>

struct inputdev_info {
	char devpath[PATH_MAX];
	char name[80];
	char location[80];
	char idstr[80];

	struct input_id id;
	int version;

	int fd;
	int idx;
};

int setup_virtual_input_dev(char *inputdev_name);
int destory_virtual_mouse_dev(void);
int create_virtual_mouse_dev(char *mouse_dev);
int try_to_find_device(const char *devname, char *path);
struct inputdev_info *open_device(const char *devpath, struct pollfd *ufds);
int notify_process(struct inputdev_info * sourcedev, char *device_name, const char *dirname, int nfd, struct pollfd *ufds);
int input_event_process(int nfd, struct input_event *event);
void report_key_event(int uinputfd, struct input_event *event, int sync);
int get_default_pointerspeed(void);
int get_default_stepdistance(void);
void set_pointerspeed(int ms);
void set_stepdistance(int px);
void reset(void);
#endif
