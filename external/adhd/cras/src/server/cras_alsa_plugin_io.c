/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <alsa/asoundlib.h>
#include <alsa/use-case.h>
#include <stdio.h>
#include <sys/select.h>
#include <syslog.h>

#include "cras_alsa_io.h"
#include "cras_alsa_jack.h"
#include "cras_alsa_mixer.h"
#include "cras_alsa_ucm.h"
#include "cras_iodev.h"
#include "cras_system_state.h"
#include "iniparser_wrapper.h"
#include "utlist.h"

#define PLUGINS_INI "plugins.ini"
#define PLUGIN_KEY_CTL "ctl"
#define PLUGIN_KEY_DIR "dir"
#define PLUGIN_KEY_PCM "pcm"
#define PLUGIN_KEY_CARD "card"

#define NULL_USB_VID 0x00
#define NULL_USB_PID 0x00
#define NULL_USB_SERIAL_NUMBER "serial-number-not-used"

struct hctl_poll_fd {
	int fd;
	struct hctl_poll_fd *prev, *next;
};

struct alsa_plugin {
	snd_hctl_t *hctl;
	struct cras_alsa_mixer *mixer;
	struct hctl_poll_fd *hctl_poll_fds;
	struct cras_use_case_mgr *ucm;
	struct cras_iodev *iodev;
	struct alsa_plugin *next, *prev;
};

static struct alsa_plugin *plugins;

static char ini_name[MAX_INI_NAME_LENGTH + 1];
static char key_name[MAX_INI_NAME_LENGTH + 1];
static dictionary *plugins_ini = NULL;

static void hctl_event_pending(void *arg, int revents)
{
	struct alsa_plugin *plugin;

	plugin = (struct alsa_plugin *)arg;
	if (plugin->hctl == NULL)
		return;

	/* handle_events will trigger the callback registered with each control
	 * that has changed. */
	snd_hctl_handle_events(plugin->hctl);
}

/* hctl poll descritpor */
static void collect_poll_descriptors(struct alsa_plugin *plugin)
{
	struct hctl_poll_fd *registered_fd;
	struct pollfd *pollfds;
	int i, n, rc;

	n = snd_hctl_poll_descriptors_count(plugin->hctl);
	if (n == 0) {
		syslog(LOG_DEBUG, "No hctl descritpor to poll");
		return;
	}

	pollfds = malloc(n * sizeof(*pollfds));
	if (pollfds == NULL)
		return;

	n = snd_hctl_poll_descriptors(plugin->hctl, pollfds, n);
	for (i = 0; i < n; i++) {
		registered_fd = calloc(1, sizeof(*registered_fd));
		if (registered_fd == NULL) {
			free(pollfds);
			return;
		}
		registered_fd->fd = pollfds[i].fd;
		DL_APPEND(plugin->hctl_poll_fds, registered_fd);
		rc = cras_system_add_select_fd(
			registered_fd->fd, hctl_event_pending, plugin, POLLIN);
		if (rc < 0) {
			DL_DELETE(plugin->hctl_poll_fds, registered_fd);
			free(pollfds);
			return;
		}
	}
	free(pollfds);
}

static void cleanup_poll_descriptors(struct alsa_plugin *plugin)
{
	struct hctl_poll_fd *poll_fd;
	DL_FOREACH (plugin->hctl_poll_fds, poll_fd) {
		cras_system_rm_select_fd(poll_fd->fd);
		DL_DELETE(plugin->hctl_poll_fds, poll_fd);
		free(poll_fd);
	}
}

static void destroy_plugin(struct alsa_plugin *plugin);

void alsa_plugin_io_create(enum CRAS_STREAM_DIRECTION direction,
			   const char *pcm_name, const char *ctl_name,
			   const char *card_name)
{
	struct alsa_plugin *plugin;
	struct ucm_section *section;
	struct ucm_section *ucm_sections;
	int rc;

	plugin = (struct alsa_plugin *)calloc(1, sizeof(*plugin));
	if (!plugin) {
		syslog(LOG_ERR, "No memory to create alsa plugin");
		return;
	}

	rc = snd_hctl_open(&plugin->hctl, ctl_name, SND_CTL_NONBLOCK);
	if (rc < 0) {
		syslog(LOG_ERR, "open hctl fail for plugin %s", ctl_name);
		goto cleanup;
	}

	rc = snd_hctl_nonblock(plugin->hctl, 1);
	if (rc < 0) {
		syslog(LOG_ERR, "Failed to nonblock hctl for %s", ctl_name);
		goto cleanup;
	}
	rc = snd_hctl_load(plugin->hctl);
	if (rc < 0) {
		syslog(LOG_ERR, "Failed to load hctl for %s", ctl_name);
		goto cleanup;
	}
	collect_poll_descriptors(plugin);

	plugin->mixer = cras_alsa_mixer_create(ctl_name);

	plugin->ucm = ucm_create(card_name);

	DL_APPEND(plugins, plugin);

	ucm_sections = ucm_get_sections(plugin->ucm);
	DL_FOREACH (ucm_sections, section) {
		rc = cras_alsa_mixer_add_controls_in_section(plugin->mixer,
							     section);
		if (rc)
			syslog(LOG_ERR,
			       "Failed adding control to plugin,"
			       "section %s mixer_name %s",
			       section->name, section->mixer_name);
	}
	plugin->iodev = alsa_iodev_create(0, card_name, 0, pcm_name, "", "",
					  ALSA_CARD_TYPE_USB, 1, /* is first */
					  plugin->mixer, NULL, plugin->ucm,
					  plugin->hctl, direction, NULL_USB_VID,
					  NULL_USB_PID, NULL_USB_SERIAL_NUMBER);

	DL_FOREACH (ucm_sections, section) {
		if (section->dir != plugin->iodev->direction)
			continue;
		section->dev_idx = 0;
		alsa_iodev_ucm_add_nodes_and_jacks(plugin->iodev, section);
	}

	alsa_iodev_ucm_complete_init(plugin->iodev);

	return;
cleanup:
	if (plugin)
		destroy_plugin(plugin);
}

static void destroy_plugin(struct alsa_plugin *plugin)
{
	cleanup_poll_descriptors(plugin);
	if (plugin->hctl)
		snd_hctl_close(plugin->hctl);
	if (plugin->iodev)
		alsa_iodev_destroy(plugin->iodev);
	if (plugin->mixer)
		cras_alsa_mixer_destroy(plugin->mixer);

	free(plugin);
}

void alsa_pluigin_io_destroy_all()
{
	struct alsa_plugin *plugin;

	DL_FOREACH (plugins, plugin)
		destroy_plugin(plugin);
}

void cras_alsa_plugin_io_init(const char *device_config_dir)
{
	int nsec, i;
	enum CRAS_STREAM_DIRECTION direction;
	const char *sec_name;
	const char *tmp, *pcm_name, *ctl_name, *card_name;

	snprintf(ini_name, MAX_INI_NAME_LENGTH, "%s/%s", device_config_dir,
		 PLUGINS_INI);
	ini_name[MAX_INI_NAME_LENGTH] = '\0';

	plugins_ini = iniparser_load_wrapper(ini_name);
	if (!plugins_ini)
		return;

	nsec = iniparser_getnsec(plugins_ini);
	for (i = 0; i < nsec; i++) {
		sec_name = iniparser_getsecname(plugins_ini, i);

		/* Parse dir=output or dir=input */
		snprintf(key_name, MAX_INI_NAME_LENGTH, "%s:%s", sec_name,
			 PLUGIN_KEY_DIR);
		tmp = iniparser_getstring(plugins_ini, key_name, NULL);
		if (strcmp(tmp, "output") == 0)
			direction = CRAS_STREAM_OUTPUT;
		else if (strcmp(tmp, "input") == 0)
			direction = CRAS_STREAM_INPUT;
		else
			continue;

		/* pcm=<plugin-pcm-name> this name will be used with
		 * snd_pcm_open. */
		snprintf(key_name, MAX_INI_NAME_LENGTH, "%s:%s", sec_name,
			 PLUGIN_KEY_PCM);
		pcm_name = iniparser_getstring(plugins_ini, key_name, NULL);
		if (!pcm_name)
			continue;

		/* ctl=<plugin-ctl-name> this name will be used with
		 * snd_hctl_open. */
		snprintf(key_name, MAX_INI_NAME_LENGTH, "%s:%s", sec_name,
			 PLUGIN_KEY_CTL);
		ctl_name = iniparser_getstring(plugins_ini, key_name, NULL);
		if (!ctl_name)
			continue;

		/* card=<card-name> this name will be used with
		 * snd_use_case_mgr_open. */
		snprintf(key_name, MAX_INI_NAME_LENGTH, "%s:%s", sec_name,
			 PLUGIN_KEY_CARD);
		card_name = iniparser_getstring(plugins_ini, key_name, NULL);
		if (!card_name)
			continue;

		syslog(LOG_DEBUG,
		       "Creating plugin for direction %s, pcm %s, ctl %s, card %s",
		       direction == CRAS_STREAM_OUTPUT ? "output" : "input",
		       pcm_name, ctl_name, card_name);

		alsa_plugin_io_create(direction, pcm_name, ctl_name, card_name);
	}
}
