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

#ifndef __LIBBOOT_H__
#define __LIBBOOT_H__

#include "type_def.h"
#include "spare_head.h"

#define BOOT0			0
#define UBOOT			1

struct user_display_param {
	/*
	 * resolution store like that:
	 *  (type << 8 | mode)
	 */
	char resolution[32];
	char margin[32];

	/*
	 * vender id of the last hdmi output device
	 */
	char vendorid[64];

	/*
	 * Add more fields for HDR support
	 */
	int format;       /* RGB / YUV444 / YUV422 / YUV420     */
	int depth;        /* Color depth: 8 / 10 / 12 / 16      */
	int eotf;         /* Electro-Optical Transfer Functions */
	int color_space;  /* BT.601 / BT.709 / BT.2020          */
};

struct user_mmc_para {
	/*
	 *----- normal
	 *	offset 0~127: boot0 struct _boot_sdcard_info_t;
	 *	offset 128~255: struct tune_sdly, timing
	 *	parameter for speed mode and frequency
	 *----- secure
	 *	offset 128 ~ (224=384-160):
	 *	struct tune_sdly, timing
	 *	rameter for speed mode and
	 *	frequency
	 *	sizeof(priv_info)  is about 60 bytes.
	 *
	 * */
	char storage_data[512];
};

int get_libboot_version(void);

int libboot_read_display_param(struct user_display_param *out);
int libboot_sync_display_param(struct user_display_param *in);
int libboot_sync_display_param_keyvalue(const char *key, char *value);

int libboot_read_mmc_param(void *out);
int libboot_burn_mmc_para(void *in, void *param);

int libboot_recovery_read_uboot_item(BufferExtractCookie *out, void *item_name);
int libboot_recovery_burn_boot(BufferExtractCookie *in, void *item_name);
int libboot_recovery_set_file_dir(BufferExtractCookie *file_dir);

u32 randto1k(u32 num);
int get_cookie_of_file(const char *path, BufferExtractCookie *cookie);
int read_file_to_cookie(const char *path, BufferExtractCookie *cookie);
#endif
