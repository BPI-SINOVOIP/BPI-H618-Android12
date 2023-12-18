/*
 * Copyright (C) 2014 The Android Open Source Project
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libboot.h"
#include "sunxi_boot_api.h"
#include "private_uboot.h"
#include "private_toc.h"
static int read_display_parameter(void *buffer, void *param)
{
	struct spare_parameter_head_t *para_head = NULL;
	para_head = (struct spare_parameter_head_t *)buffer;

	if (!buffer || !param)
		return -1;

	if(strncmp((const char *)(para_head->para_head.magic), PARAMETER_MAGIC, sizeof(PARAMETER_MAGIC)))
	{
		fprintf(stderr, "parameter magic err\n");
		return -1;
	}

	/* modify parameter value below */
	struct display_params *save = &para_head->para_data.display_param;
	struct user_display_param *out = param;

	strncpy(out->resolution, save->resolution, sizeof(out->resolution));
	strncpy(out->margin, save->margin, sizeof(out->margin));
	strncpy(out->vendorid, save->vendorid, sizeof(out->vendorid));

	out->format = save->format;
	out->depth  = save->depth;
	out->eotf   = save->eotf;
	out->color_space = save->color_space;

	para_head->para_head.crc = generate_check_sum(para_head, para_head->para_head.length, para_head->para_head.crc);
	return 0;
}


static int sync_display_parameter(void *buffer, void *param)
{
	struct spare_parameter_head_t *para_head = NULL;
	para_head = (struct spare_parameter_head_t *)buffer;

	if (!buffer || !param)
		return -1;

	if(strncmp((const char *)(para_head->para_head.magic), PARAMETER_MAGIC, sizeof(PARAMETER_MAGIC)))
	{
		fprintf(stderr, "parameter magic err\n");
		return -1;
	}

	/* modify parameter value below */
	struct display_params *save = &para_head->para_data.display_param;
	struct user_display_param *in = param;
	strncpy(save->resolution, in->resolution, sizeof(save->resolution));
	strncpy(save->margin, in->margin, sizeof(save->margin));
	strncpy(save->vendorid, in->vendorid, sizeof(save->vendorid));

	save->format = in->format;
	save->depth  = in->depth;
	save->eotf   = in->eotf;
	save->color_space = in->color_space;

	para_head->para_head.crc = generate_check_sum(para_head, para_head->para_head.length, para_head->para_head.crc);
	return 0;
}

int libboot_read_display_param(struct user_display_param *out)
{
	return libboot_read_item(read_display_parameter, out, ITEM_PARAMETER_NAME);
}

int libboot_sync_display_param(struct user_display_param *in)
{
	return libboot_update_item(sync_display_parameter, in, ITEM_PARAMETER_NAME);
}
