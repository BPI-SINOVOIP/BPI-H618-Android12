/*
 * src/view/tp_track_eink/tp_track_eink.c
 *
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <linux/fb.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <asm-generic/ioctl.h>

#include "dragonboard.h"
#include "script.h"
#include "df_view.h"


//static unsigned int layer_id;
static int screen_width;
static int screen_height;
//static int fb;
static unsigned char *buffer = NULL;


static int draw_type;
static int tp_draw_color_idx;
static unsigned int tp_draw_color;

static int pre_x;
static int pre_y;
static int fd;

int tp_track_init(void)
{
	int ret = 0;
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;

	fd = open("/dev/fb1", O_RDWR);
	if (fd < 0) {
		db_msg("open fb1 fail:%s\n", strerror(errno));
		goto OUT1;
	}

	ret = ioctl(fd, FBIOGET_VSCREENINFO, &vinfo);
	if (ret < 0) {
		db_msg("get var info fail:%s\n", strerror(errno));
		goto OUT1;
	}
	ret = ioctl(fd, FBIOGET_FSCREENINFO, &finfo);
	if (ret < 0) {
		db_msg("get fix info fail:%s\n", strerror(errno));
		goto OUT1;
	}

	buffer = (char *)mmap(0, finfo.smem_len, PROT_READ | PROT_WRITE,
					MAP_SHARED, fd, 0);
	if (buffer == MAP_FAILED) {
		db_msg("map fail:%s %u\n", strerror(errno), finfo.smem_len);
		goto OUT1;
	}
	memset(buffer, 0x0, finfo.smem_len);

	/* Universal implement */
	screen_width  = vinfo.xres;
	screen_height = vinfo.yres;

	pre_x = -1;
	pre_y = -1;

	 /* get draw type */
	if (script_fetch("tp", "draw_type", &draw_type, 1) ||
		(draw_type != 0 && draw_type != 1)) {
		draw_type = 0;
	}

	if (script_fetch("df_view", "tp_draw_color", &tp_draw_color_idx, 1) ||
		tp_draw_color < COLOR_WHITE_IDX ||
		tp_draw_color > COLOR_BLACK_IDX) {
		tp_draw_color_idx = COLOR_WHITE_IDX;
	}

	tp_draw_color = 0xff << 24 |
			color_table[tp_draw_color_idx].r << 16 |
			color_table[tp_draw_color_idx].g << 8  |
			color_table[tp_draw_color_idx].b;

	db_msg("tp draw color: 0x%x scn:[%u x %u] buf len:%u\n", tp_draw_color, screen_width, screen_height, finfo.smem_len);

	return 0;

OUT1:
	close(fd);
	return ret;
}

void tp_track_draw_pixel(int x, int y, unsigned int color)
{
    unsigned char *pixel;

    pixel = (buffer + screen_width * y * 4 + x * 4);
    memcpy(pixel, &color, 4);
}

void tp_track_draw_line(int x1, int y1, int x2, int y2, unsigned int color)
{
    int dx = x2 - x1;
    int dy = y2 - y1;
    int ux = ((dx > 0) << 1) - 1; // x的增量方向，取或-1
    int uy = ((dy > 0) << 1) - 1; // y的增量方向，取或-1
    int x = x1, y = y1, eps;      // eps为累加误差

    eps = 0;
    dx = abs(dx);
    dy = abs(dy);
    if (dx > dy)
    {
        for (x = x1; x != x2; x += ux)
        {
            tp_track_draw_pixel(x, y, color);
            eps += dy;
            if ((eps << 1) >= dx)
            {
                y += uy; eps -= dx;
            }
        }
    }
    else
    {
        for (y = y1; y != y2; y += uy)
        {
            tp_track_draw_pixel(x, y, color);
            eps += dx;
            if ((eps << 1) >= dy)
            {
                x += ux; eps -= dy;
            }
        }
    }
}

int tp_track_draw(int x, int y, int press)
{
    if (x < 0 || x > screen_width || y < 0 || y > screen_height)
        return -1;

    if (draw_type) {
        tp_track_draw_pixel(x, y, tp_draw_color);
    }
    else {
        if (press == -1) {
            if (pre_x != -1 && pre_y != -1) {
                tp_track_draw_line(pre_x, pre_y, x, y, tp_draw_color);
            }
            pre_x = pre_y = -1;
        }
        else if (press == 0) {
            pre_x = x;
            pre_y = y;
        }
        else if (press == 1) {
            if (pre_x != -1 && pre_y != -1) {
                tp_track_draw_line(pre_x, pre_y, x, y, tp_draw_color);
            }
            pre_x = x;
            pre_y = y;
        }
    }

    return 0;
}

void tp_track_start(int x, int y)
{
    pre_x = x;
    pre_y = y;
}

void tp_track_clear(void)
{
    memset(buffer, 0, screen_width * screen_height * 4);
}
