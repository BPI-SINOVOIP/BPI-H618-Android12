/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cras_device_blocklist.h"
#include "iniparser_wrapper.h"
#include "utlist.h"

struct cras_device_blocklist {
	dictionary *ini;
};

/*
 * Exported Interface
 */

struct cras_device_blocklist *
cras_device_blocklist_create(const char *config_path)
{
	struct cras_device_blocklist *blocklist;
	char ini_name[MAX_INI_NAME_LENGTH + 1];

	blocklist = calloc(1, sizeof(*blocklist));
	if (!blocklist)
		return NULL;

	snprintf(ini_name, MAX_INI_NAME_LENGTH, "%s/%s", config_path,
		 "device_blocklist");
	ini_name[MAX_INI_NAME_LENGTH] = '\0';
	blocklist->ini = iniparser_load_wrapper(ini_name);

	return blocklist;
}

void cras_device_blocklist_destroy(struct cras_device_blocklist *blocklist)
{
	if (blocklist && blocklist->ini)
		iniparser_freedict(blocklist->ini);
	free(blocklist);
}

int cras_device_blocklist_check(struct cras_device_blocklist *blocklist,
				unsigned vendor_id, unsigned product_id,
				unsigned desc_checksum, unsigned device_index)
{
	char ini_key[MAX_INI_KEY_LENGTH + 1];

	if (!blocklist)
		return 0;

	snprintf(ini_key, MAX_INI_KEY_LENGTH, "USB_Outputs:%04x_%04x_%08x_%u",
		 vendor_id, product_id, desc_checksum, device_index);
	ini_key[MAX_INI_KEY_LENGTH] = 0;
	return iniparser_getboolean(blocklist->ini, ini_key, 0);
}
