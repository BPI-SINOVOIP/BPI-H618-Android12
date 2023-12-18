/*
 * \file        flashlight.c
 * \brief
 *
 * \version     1.0.0
 * \date        2023年01月17日
 * \author      Liu Chsnesheng <liuchensheng@allwinnertech.com>
 *
 * Copyright (c) 2023 Allwinner Technology. All Rights Reserved.
 *
 */

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

#if defined (_CONFIG_LINUX_5_4) || (_CONFIG_LINUX_5_10) || (_CONFIG_LINUX_5_15)
#include "sun50iw10p_display.h"
#else
#include "sunxi_display2.h"
#endif

#include "dragonboard_inc.h"
#include "videodev2_vin.h"

static char sensor_name[20];
static int sensor_fd;
static int flash_test_cnt;
static pthread_t flash_tid;

static int subdev_open(int *sub_fd, char *str)
{
	char subdev[20] = {'\0'};
	char node[50] = {'\0'};
	char data[20] = {'\0'};
	int i, fs = -1;

	for (i = 0; i < 255; i++) {
		sprintf(node, "/sys/class/video4linux/v4l-subdev%d/name", i);
		fs = open(node, O_RDONLY/* required */| O_NONBLOCK, 0);
		if (fs < 0) {
			db_error("open %s falied\n", node);
			continue;
		}
		/*data_length = lseek(fd, 0, SEEK_END);*/
		lseek(fs, 0L, SEEK_SET);
		read(fs, data, 20);
		close(fs);
		if (!strncmp(str, data, strlen(str))) {
			sprintf(subdev, "/dev/v4l-subdev%d", i);
			db_msg("find %s is %s\n", str, subdev);
			*sub_fd = open (subdev, O_RDWR | O_NONBLOCK, 0);
			if (*sub_fd < 0) {
				db_error("open %s falied\n", str);
				return -1;
			}
			db_debug("open %s fd = %d\n", str, *sub_fd);
			return 0;
		}
	}
	db_error("can not find %s!\n", str);
	return -1;
}

static void *flashlight_open(void *args)
{
	int dev_no;
	struct flash_para flash;

	if (-1 == subdev_open(&sensor_fd, sensor_name))
		return -1;

	flash.mode = V4L2_FLASH_LED_MODE_FLASH;
	if (-1 == ioctl(sensor_fd, VIDIOC_VIN_FLASH_EN, &flash)) {
		printf("VIDIOC_VIN_FLASH_EN failed\n");
		return -1;
	}

	db_msg("flashlight: flash open success!!\n");
}

static void *flashlight_close(void *args)
{
	struct flash_para flash;

	flash.mode = V4L2_FLASH_LED_MODE_NONE;
	if (-1 == ioctl(sensor_fd, VIDIOC_VIN_FLASH_EN, &flash)) {
		printf("VIDIOC_VIN_FLASH_EN failed\n");
		return -1;
	}

	db_msg("flashlight: flash close success!!\n");
}

int flashlight_test(void)
{
	void *retval;

	if (script_fetch("flashlight", "sensor_name", (int *)sensor_name, sizeof(sensor_name) / 4)) {
		db_error("flashlight: sensor_name not found\n");
		return -1;
	}

	if (flash_test_cnt == 0){
		flash_test_cnt = 1;
		db_debug("flashlight: create flash open thread\n");
		if (pthread_create(&flash_tid, NULL, flashlight_open, NULL)) {
			db_error("flashlight: can't create flash open thread(%s)\n", strerror(errno));
			return -1;
		}
	} else {
		flash_test_cnt = 0;
		db_debug("flashlight: create flash close thread\n");
		if (pthread_create(&flash_tid, NULL, flashlight_close, NULL)) {
			db_error("flashlight: can't create flash close thread(%s)\n", strerror(errno));
			return -1;
		}
	}

	if (pthread_join(flash_tid, &retval)) {
		db_error("flashlight: can't join with flash thread\n");
	}
	db_msg("flashlight: flash thread exit code #%d\n", (int)retval);

	return 0;
}
