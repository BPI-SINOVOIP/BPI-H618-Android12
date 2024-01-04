/*
 * Sunxi display params save/read utils function
 * Base on libboot
 *
 * Copyright (C) 2015-2018 AllwinnerTech, Inc.
 *
 * Contacts:
 * Zeng.Yajian <zengyajian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include "libboot.h"
#include <utils/Log.h>

typedef int (*param_update_method_t)(struct user_display_param *param, char *value);

static int update_resolution(struct user_display_param *param, char *value)
{
	if (strcmp(param->resolution, value)) {
		strncpy(param->resolution, value, sizeof(param->resolution));
		return 1;
	}
	return 0;
}

static int update_vendorid(struct user_display_param *param, char *value)
{
	if (strcmp(param->vendorid, value)) {
		strncpy(param->vendorid, value, sizeof(param->vendorid));
		return 1;
	}
	return 0;
}

static int update_video_format(struct user_display_param *param, char *value)
{
	int format = strtol(value, NULL, 16);
	if (param->format != format) {
		param->format = format;
		return 1;
	}
	return 0;
}

static int update_color_depth(struct user_display_param *param, char *value)
{
	int depth = strtol(value, NULL, 16);
	if (param->depth != depth) {
		param->depth = depth;
		return 1;
	}
	return 0;
}

static int update_eotf(struct user_display_param *param, char *value)
{
	int eotf = strtol(value, NULL, 16);
	if (param->eotf != eotf) {
		param->eotf = eotf;
		return 1;
	}
	return 0;
}

static int update_color_space(struct user_display_param *param, char *value)
{
	int space = strtol(value, NULL, 16);
	if (param->color_space != space) {
		param->color_space = space;
		return 1;
	}
	return 0;
}

/*
 * input format: "type,mode - format,depth,space,eotf"
 */

static int update_item(int *raw, unsigned int mask, int offset, int value)
{
	int tmp = *raw;
	if (value == -1)
		return 0;
	if (((tmp & mask) >> offset) == (uint32)value)
		return 0;
	*raw = (tmp & (~mask)) | (value << offset);
	return 1;
}

#define ARRAYLENGTH 32
static int update_rsl_item(char *raw, int type, int mode)
{
	char valueString[ARRAYLENGTH] = {0};
	char *pValue, *pt, *ptEnd;
	int index = 0;
	int i = 0;
	int len = 0;
	int format = ((type & 0xFF) << 8) | (mode & 0xFF);
	int values[3] = {0, 0, 0};

	if (raw == NULL) {
		return 0;
	}
	switch(type) {
		case 4:/*HDMI*/
			index = 1;
			break;
		case 2:/*CVBS*/
			index = 0;
			break;
		case 8:/*VGA*/
			index = 2;
			break;
		default:
			return 0;
	}
	len =  strlen(raw);
	if (len <= 0) {
		values[index] = format;
		sprintf(raw, "%x\n%x\n%x\n", values[0], values[1], values[2]);
		ALOGD("after set raw=%x, len=%d", raw, len);
		return 1;
	}

	strncpy(valueString, raw, len);
	pValue = valueString;
	pt = valueString;
	ptEnd = valueString + len;
	for(;(i < 3) && (pt != ptEnd); pt++) {
		if('\n' == *pt) {
			*pt = '\0';
			values[i] = (int)strtoul(pValue, NULL, 16);
			ALOGD("libboot:pValue=%s, values[%d]=0x%x", pValue, i, values[i]);
			pValue = pt + 1;
			i++;
		}
	}
	ALOGD("libboot:format=%x, values[%d]=0x%x", format, index, values[index]);
	if (values[index] != format) {
		values[index] = format;
		sprintf(raw, "%x\n%x\n%x\n", values[0], values[1], values[2]);
		ALOGD("after set raw=%x", raw);
		return 1;
	}
	return 0;
}

static int update_config(struct user_display_param *param, char *value)
{
	int type   = 0;
	int mode   = 0;
	int format = 0;
	int depth  = 0;
	int space  = 0;
	int eotf   = 0;
	int count  = 0;
	int update = 0;
	int offset = 0;
	unsigned int mask   = 0xffff;

	count = sscanf(value, "%d,%d - %d,%d,%d,%d",
					&type, &mode, &format, &depth, &space, &eotf);
	if ((count != 6) || (type != 2 && type != 4))
		return 0;

	if (type == 2) { offset = 16; mask <<= 16; } /* CVBS */
	else if (type == 4) { offset =  0; mask <<= 0;  } /* HDMI */
	else return 0;

	update += update_item(&param->format, mask, offset, format);
	update += update_item(&param->depth, mask, offset, depth);
	update += update_item(&param->eotf, mask, offset, eotf);
	update += update_item(&param->color_space, mask, offset, space);
	update += update_rsl_item(param->resolution, type, mode);
	return update;
}

struct param_update_entry {
	const char *name;
	param_update_method_t function;
};

const struct param_update_entry method_maps[] = {
	{"disp_rsl"    , update_resolution  },
	{"tv_vdid"     , update_vendorid    },
	{"vido_format" , update_video_format},
	{"color_depth" , update_color_depth },
	{"color_space" , update_color_space },
	{"output_eotf" , update_eotf        },
	{"disp_config" , update_config      },
};

int libboot_sync_display_param_keyvalue(const char *key, char *value)
{
	if (!key || ! value)
		return -1;

	int dirty = 0;
	struct user_display_param param;
	memset(&param, 0, sizeof(param));
	if (libboot_read_display_param(&param) != 0) {
		fprintf(stderr, "libboot read display params failed\n");
		return -1;
	}

	for (uint32 i = 0; i < sizeof(method_maps) / sizeof(method_maps[0]); i++) {
		if (!strcmp(key, method_maps[i].name)) {
			param_update_method_t func = method_maps[i].function;
			dirty = func(&param, value);
			break;
		}
	}

	if (dirty)
		libboot_sync_display_param(&param);
	return 0;
}
