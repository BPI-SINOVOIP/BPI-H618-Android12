/* Copyright 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <syslog.h>

#include "cras_board_config.h"
#include "iniparser_wrapper.h"

static const int32_t DEFAULT_OUTPUT_BUFFER_SIZE = 512;
static const int32_t AEC_SUPPORTED_DEFAULT = 0;
static const int32_t AEC_GROUP_ID_DEFAULT = -1;
static const int32_t BLUETOOTH_WBS_ENABLED_INI_DEFAULT = 1;
static const int32_t BLUETOOTH_DEPRIORITIZE_WBS_MIC_INI_DEFAULT = 0;
static const int32_t HOTWORD_PAUSE_AT_SUSPEND_DEFAULT = 0;

#define CONFIG_NAME "board.ini"
#define DEFAULT_OUTPUT_BUF_SIZE_INI_KEY "output:default_output_buffer_size"
#define AEC_SUPPORTED_INI_KEY "processing:aec_supported"
#define AEC_GROUP_ID_INI_KEY "processing:group_id"
#define BLUETOOTH_WBS_ENABLED_INI_KEY "bluetooth:wbs_enabled"
#define BLUETOOTH_DEPRIORITIZE_WBS_MIC_INI_KEY "bluetooth:deprioritize_wbs_mic"
#define UCM_IGNORE_SUFFIX_KEY "ucm:ignore_suffix"
#define HOTWORD_PAUSE_AT_SUSPEND "hotword:pause_at_suspend"

void cras_board_config_get(const char *config_path,
			   struct cras_board_config *board_config)
{
	char ini_name[MAX_INI_NAME_LENGTH + 1];
	char ini_key[MAX_INI_KEY_LENGTH + 1];
	const char *ptr;
	dictionary *ini;

	board_config->default_output_buffer_size = DEFAULT_OUTPUT_BUFFER_SIZE;
	board_config->aec_supported = AEC_SUPPORTED_DEFAULT;
	board_config->aec_group_id = AEC_GROUP_ID_DEFAULT;
	board_config->ucm_ignore_suffix = NULL;
	board_config->bt_wbs_enabled = BLUETOOTH_WBS_ENABLED_INI_DEFAULT;
	board_config->deprioritize_bt_wbs_mic =
		BLUETOOTH_DEPRIORITIZE_WBS_MIC_INI_DEFAULT;
	if (config_path == NULL)
		return;

	snprintf(ini_name, MAX_INI_NAME_LENGTH, "%s/%s", config_path,
		 CONFIG_NAME);
	ini_name[MAX_INI_NAME_LENGTH] = '\0';
	ini = iniparser_load_wrapper(ini_name);
	if (ini == NULL) {
		syslog(LOG_DEBUG, "No ini file %s", ini_name);
		return;
	}

	snprintf(ini_key, MAX_INI_KEY_LENGTH, DEFAULT_OUTPUT_BUF_SIZE_INI_KEY);
	ini_key[MAX_INI_KEY_LENGTH] = 0;
	board_config->default_output_buffer_size =
		iniparser_getint(ini, ini_key, DEFAULT_OUTPUT_BUFFER_SIZE);

	snprintf(ini_key, MAX_INI_KEY_LENGTH, AEC_SUPPORTED_INI_KEY);
	ini_key[MAX_INI_KEY_LENGTH] = 0;
	board_config->aec_supported =
		iniparser_getint(ini, ini_key, AEC_SUPPORTED_DEFAULT);

	snprintf(ini_key, MAX_INI_KEY_LENGTH, AEC_GROUP_ID_INI_KEY);
	ini_key[MAX_INI_KEY_LENGTH] = 0;
	board_config->aec_group_id =
		iniparser_getint(ini, ini_key, AEC_GROUP_ID_DEFAULT);

	snprintf(ini_key, MAX_INI_KEY_LENGTH, BLUETOOTH_WBS_ENABLED_INI_KEY);
	ini_key[MAX_INI_KEY_LENGTH] = 0;
	board_config->bt_wbs_enabled = iniparser_getint(
		ini, ini_key, BLUETOOTH_WBS_ENABLED_INI_DEFAULT);

	snprintf(ini_key, MAX_INI_KEY_LENGTH,
		 BLUETOOTH_DEPRIORITIZE_WBS_MIC_INI_KEY);
	ini_key[MAX_INI_KEY_LENGTH] = 0;
	board_config->deprioritize_bt_wbs_mic = iniparser_getint(
		ini, ini_key, BLUETOOTH_DEPRIORITIZE_WBS_MIC_INI_DEFAULT);

	snprintf(ini_key, MAX_INI_KEY_LENGTH, UCM_IGNORE_SUFFIX_KEY);
	ini_key[MAX_INI_KEY_LENGTH] = 0;
	ptr = iniparser_getstring(ini, ini_key, "");
	if (ptr) {
		board_config->ucm_ignore_suffix = strdup(ptr);
		if (!board_config->ucm_ignore_suffix)
			syslog(LOG_ERR, "Failed to call strdup: %d", errno);
	}

	snprintf(ini_key, MAX_INI_KEY_LENGTH, HOTWORD_PAUSE_AT_SUSPEND);
	ini_key[MAX_INI_KEY_LENGTH] = 0;
	board_config->hotword_pause_at_suspend = iniparser_getint(
		ini, ini_key, HOTWORD_PAUSE_AT_SUSPEND_DEFAULT);

	iniparser_freedict(ini);
	syslog(LOG_DEBUG, "Loaded ini file %s", ini_name);
}
