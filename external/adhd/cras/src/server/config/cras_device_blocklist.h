/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Keeps a list of playback devices that should be ignored for a card.  This is
 * useful for devices that present non-functional alsa devices.  For instance
 * some mics show a phantom playback device.
 */
#ifndef CRAS_DEVICE_BLOCKLIST_H_
#define CRAS_DEVICE_BLOCKLIST_H_

#include <stdint.h>

#include "cras_types.h"

struct cras_device_blocklist;

/* Creates a blocklist of devices that should never be added to the system.
 * Args:
 *    config_path - Path containing the config files.
 * Returns:
 *    A pointer to the created blocklist on success, NULL on failure.
 */
struct cras_device_blocklist *
cras_device_blocklist_create(const char *config_path);

/* Destroys a blocklist returned by cras_device_blocklist_create().
 * Args:
 *    blocklist - Blocklist returned by cras_device_blocklist_create()
 */
void cras_device_blocklist_destroy(struct cras_device_blocklist *blocklist);

/* Checks if a playback device on a USB card is blocklisted.
 * Args:
 *    blocklist - Blocklist returned by cras_device_blocklist_create()
 *    vendor_id - USB vendor ID.
 *    product_id - USB product ID.
 *    device_index - Index of the alsa device in the card.
 * Returns:
 *  1 if the device is blocklisted, 0 otherwise.
 */
int cras_device_blocklist_check(struct cras_device_blocklist *blocklist,
				unsigned vendor_id, unsigned product_id,
				unsigned desc_checksum, unsigned device_index);

#endif /* CRAS_CARD_DEVICE_BLOCKLIST_H_ */
