/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_HFP_AG_PROFILE_H_
#define CRAS_HFP_AG_PROFILE_H_

#include <dbus/dbus.h>
#include <stdbool.h>

#include "cras_bt_device.h"
#include "cras_hfp_slc.h"

/*
 * For service record profile, 'SupportedFearues' attribute bit mapping
 * for HFP AG. Bits 0 to 4 are identical to the unsolicited result code
 * of +BRSF command.
 */
#define FEATURES_AG_THREE_WAY_CALLING 0x0001
#define FEATURES_AG_EC_ANDOR_NR 0x0002
#define FEATURES_AG_VOICE_RECOGNITION 0x0004
#define FEATURES_AG_INBAND_RINGTONE 0x0008
#define FEATURES_AG_ATTACH_NUMBER_TO_VOICETAG 0x0010
#define FEATURES_AG_WIDE_BAND_SPEECH 0x0020

struct hfp_slc_handle;

/* Adds a profile instance for HFP AG (Hands-Free Profile Audio Gateway). */
int cras_hfp_ag_profile_create(DBusConnection *conn);

/* Adds a profile instance for HSP AG (Headset Profile Audio Gateway). */
int cras_hsp_ag_profile_create(DBusConnection *conn);

/* Starts the HFP audio gateway for audio input/output. */
int cras_hfp_ag_start(struct cras_bt_device *device);

/*
 * Suspends all connected audio gateways except the one associated to device.
 * Used to stop previously running HFP/HSP audio when a new device is connected.
 * Args:
 *    device - The device that we want to keep connection while others should
 *        be removed.
 */
int cras_hfp_ag_remove_conflict(struct cras_bt_device *device);

/* Suspends audio gateway associated with given bt device. */
void cras_hfp_ag_suspend_connected_device(struct cras_bt_device *device);

/* Gets the active SLC handle. Used for HFP qualification. */
struct hfp_slc_handle *cras_hfp_ag_get_active_handle();

/* Gets the SLC handle for given cras_bt_device. */
struct hfp_slc_handle *cras_hfp_ag_get_slc(struct cras_bt_device *device);

/* Gets the logger for WBS packet status. */
struct packet_status_logger *cras_hfp_ag_get_wbs_logger();

#endif /* CRAS_HFP_AG_PROFILE_H_ */
