/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _CRAS_ALSA_UCM_SECTION_H
#define _CRAS_ALSA_UCM_SECTION_H

#include "cras_types.h"
#include "cras_alsa_mixer_name.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Represents an ALSA UCM section. */
struct ucm_section {
	/* Section name. */
	const char *name;
	/* Value of PlaybackPCM or CapturePCM. */
	const char *pcm_name;
	/* Device PCM index. */
	int dev_idx;
	/* Device PCM index to associate this section to. */
	int dependent_dev_idx;
	/* Output or Input. */
	enum CRAS_STREAM_DIRECTION dir;
	/* Associated jack's name. */
	const char *jack_name;
	/* Associated jack's type. */
	const char *jack_type;
	/* Switch number for jack from linux/input.h, or -1. */
	int jack_switch;
	/* (Playback/Capture)MixerElem value. */
	const char *mixer_name;
	/* CoupledMixers value. */
	struct mixer_name *coupled;
	struct ucm_section *prev, *next;
};

/* Create a single UCM section.
 *
 * Args:
 *    name - Section name (must not be NULL).
 *    pcm_name - PCM name used for snd_pcm_open.
 *    dev_idx - Section's device index (PCM number).
 *    dependent_dev_idx - Another ALSA device index (PCM number) under which
 *        we want to make this section a node.
 *    dir - Device direction: INPUT or OUTPUT.
 *    jack_name - Name of an associated jack (or NULL).
 *    jack_type - Type of the associated jack (or NULL).
 *
 * Returns:
 *    A valid pointer on success, NULL for memory allocation error.
 */
struct ucm_section *ucm_section_create(const char *name, const char *pcm_name,
				       int dev_idx, int dependent_dev_idx,
				       enum CRAS_STREAM_DIRECTION dir,
				       const char *jack_name,
				       const char *jack_type);

/* Sets the mixer_name value for the given section.
 *
 * Args:
 *    section - Section to manipulate.
 *    name - The name of the control.
 *
 * Returns:
 *    0 for success, -EINVAL for invalid arguments, or -ENOMEM.
 */
int ucm_section_set_mixer_name(struct ucm_section *section, const char *name);

/* Add a single coupled control to this section.
 * Control has the same direction as the section.
 *
 * Args:
 *    section - Section to manipulate.
 *    name - Coupled control name to add.
 *    type - The type of control.
 *
 * Returns:
 *    0 for success, -EINVAL for invalid arguments, or -ENOMEM.
 */
int ucm_section_add_coupled(struct ucm_section *section, const char *name,
			    mixer_name_type type);

/* Concatenate a list of coupled controls to this section.
 *
 * Args:
 *    section - Section to manipulate.
 *    coupled - Coupled control names to add.
 *
 * Returns:
 *    0 for success, -EINVAL for invalid arguments (NULL args).
 */
int ucm_section_concat_coupled(struct ucm_section *section,
			       struct mixer_name *coupled);

/* Frees a list of sections.
 *
 * Args:
 *    sections - List of sections to free.
 */
void ucm_section_free_list(struct ucm_section *sections);

/* Dump details on this section to syslog(LOG_DEBUG).
 *
 * Args:
 *    section - Section to dump.
 */
void ucm_section_dump(struct ucm_section *section);

#ifdef __cplusplus
}
#endif

#endif /* _CRAS_ALSA_MIXER_NAME_H */
