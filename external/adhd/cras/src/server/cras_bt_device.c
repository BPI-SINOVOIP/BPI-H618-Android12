/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* for ppoll */
#endif

#include <dbus/dbus.h>

#include <errno.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <syslog.h>

#include "bluetooth.h"
#include "cras_a2dp_endpoint.h"
#include "cras_bt_adapter.h"
#include "cras_bt_device.h"
#include "cras_bt_constants.h"
#include "cras_bt_log.h"
#include "cras_bt_io.h"
#include "cras_bt_profile.h"
#include "cras_hfp_ag_profile.h"
#include "cras_hfp_slc.h"
#include "cras_iodev.h"
#include "cras_iodev_list.h"
#include "cras_main_message.h"
#include "cras_server_metrics.h"
#include "cras_system_state.h"
#include "cras_tm.h"
#include "sfh.h"
#include "utlist.h"

/*
 * Bluetooth Core 5.0 spec, vol 4, part B, section 2 describes
 * the recommended HCI packet size in one USB transfer for CVSD
 * and MSBC codec.
 */
#define USB_MSBC_PKT_SIZE 60
#define USB_CVSD_PKT_SIZE 48
#define DEFAULT_SCO_PKT_SIZE USB_CVSD_PKT_SIZE

static const unsigned int PROFILE_SWITCH_DELAY_MS = 500;
static const unsigned int PROFILE_DROP_SUSPEND_DELAY_MS = 5000;

/* Check profile connections every 2 seconds and rerty 30 times maximum.
 * Attemp to connect profiles which haven't been ready every 3 retries.
 */
static const unsigned int CONN_WATCH_PERIOD_MS = 2000;
static const unsigned int CONN_WATCH_MAX_RETRIES = 30;

/* This is used when a critical SCO failure happens and is worth scheduling a
 * suspend in case for some reason BT headset stays connected in baseband and
 * confuses user.
 */
static const unsigned int SCO_SUSPEND_DELAY_MS = 5000;

static const unsigned int CRAS_SUPPORTED_PROFILES =
	CRAS_BT_DEVICE_PROFILE_A2DP_SINK | CRAS_BT_DEVICE_PROFILE_HFP_HANDSFREE;

/* Object to represent a general bluetooth device, and used to
 * associate with some CRAS modules if it supports audio.
 * Members:
 *    conn - The dbus connection object used to send message to bluetoothd.
 *    object_path - Object path of the bluetooth device.
 *    adapter - The object path of the adapter associates with this device.
 *    address - The BT address of this device.
 *    name - The readable name of this device.
 *    bluetooth_class - The bluetooth class of this device.
 *    paired - If this device is paired.
 *    trusted - If this device is trusted.
 *    connected - If this devices is connected.
 *    connected_profiles - OR'ed all connected audio profiles.
 *    profiles - OR'ed by all audio profiles this device supports.
 *    hidden_profiles - OR'ed by all audio profiles this device actually
 *        supports but is not scanned by BlueZ.
 *    bt_iodevs - The pointer to the cras_iodevs of this device.
 *    active_profile - The flag to indicate the active audio profile this
 *        device is currently using.
 *    conn_watch_retries - The retry count for conn_watch_timer.
 *    conn_watch_timer - The timer used to watch connected profiles and start
 *        BT audio input/ouput when all profiles are ready.
 *    suspend_timer - The timer used to suspend device.
 *    switch_profile_timer - The timer used to delay enabling iodev after
 *        profile switch.
 *    sco_fd - The file descriptor of the SCO connection.
 *    sco_ref_count - The reference counts of the SCO connection.
 *    suspend_reason - The reason code for why suspend is scheduled.
 *    stable_id - The unique and persistent id of this bt_device.
 */
struct cras_bt_device {
	DBusConnection *conn;
	char *object_path;
	char *adapter_obj_path;
	char *address;
	char *name;
	uint32_t bluetooth_class;
	int paired;
	int trusted;
	int connected;
	unsigned int connected_profiles;
	unsigned int profiles;
	unsigned int hidden_profiles;
	struct cras_iodev *bt_iodevs[CRAS_NUM_DIRECTIONS];
	unsigned int active_profile;
	int use_hardware_volume;
	int conn_watch_retries;
	struct cras_timer *conn_watch_timer;
	struct cras_timer *suspend_timer;
	struct cras_timer *switch_profile_timer;
	int sco_fd;
	size_t sco_ref_count;
	enum cras_bt_device_suspend_reason suspend_reason;
	unsigned int stable_id;

	struct cras_bt_device *prev, *next;
};

enum BT_DEVICE_COMMAND {
	BT_DEVICE_CANCEL_SUSPEND,
	BT_DEVICE_SCHEDULE_SUSPEND,
	BT_DEVICE_SWITCH_PROFILE,
	BT_DEVICE_SWITCH_PROFILE_ENABLE_DEV,
};

struct bt_device_msg {
	struct cras_main_message header;
	enum BT_DEVICE_COMMAND cmd;
	struct cras_bt_device *device;
	struct cras_iodev *dev;
	unsigned int arg1;
	unsigned int arg2;
};

static struct cras_bt_device *devices;

enum cras_bt_device_profile cras_bt_device_profile_from_uuid(const char *uuid)
{
	if (strcmp(uuid, HSP_HS_UUID) == 0)
		return CRAS_BT_DEVICE_PROFILE_HSP_HEADSET;
	else if (strcmp(uuid, HSP_AG_UUID) == 0)
		return CRAS_BT_DEVICE_PROFILE_HSP_AUDIOGATEWAY;
	else if (strcmp(uuid, HFP_HF_UUID) == 0)
		return CRAS_BT_DEVICE_PROFILE_HFP_HANDSFREE;
	else if (strcmp(uuid, HFP_AG_UUID) == 0)
		return CRAS_BT_DEVICE_PROFILE_HFP_AUDIOGATEWAY;
	else if (strcmp(uuid, A2DP_SOURCE_UUID) == 0)
		return CRAS_BT_DEVICE_PROFILE_A2DP_SOURCE;
	else if (strcmp(uuid, A2DP_SINK_UUID) == 0)
		return CRAS_BT_DEVICE_PROFILE_A2DP_SINK;
	else if (strcmp(uuid, AVRCP_REMOTE_UUID) == 0)
		return CRAS_BT_DEVICE_PROFILE_AVRCP_REMOTE;
	else if (strcmp(uuid, AVRCP_TARGET_UUID) == 0)
		return CRAS_BT_DEVICE_PROFILE_AVRCP_TARGET;
	else
		return 0;
}

struct cras_bt_device *cras_bt_device_create(DBusConnection *conn,
					     const char *object_path)
{
	struct cras_bt_device *device;

	device = calloc(1, sizeof(*device));
	if (device == NULL)
		return NULL;

	device->conn = conn;
	device->object_path = strdup(object_path);
	if (device->object_path == NULL) {
		free(device);
		return NULL;
	}
	device->stable_id =
		SuperFastHash(device->object_path, strlen(device->object_path),
			      strlen(device->object_path));

	DL_APPEND(devices, device);

	return device;
}

static void on_connect_profile_reply(DBusPendingCall *pending_call, void *data)
{
	DBusMessage *reply;

	reply = dbus_pending_call_steal_reply(pending_call);
	dbus_pending_call_unref(pending_call);

	if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR)
		syslog(LOG_ERR, "Connect profile message replied error: %s",
		       dbus_message_get_error_name(reply));

	dbus_message_unref(reply);
}

static void on_disconnect_reply(DBusPendingCall *pending_call, void *data)
{
	DBusMessage *reply;

	reply = dbus_pending_call_steal_reply(pending_call);
	dbus_pending_call_unref(pending_call);

	if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR)
		syslog(LOG_ERR, "Disconnect message replied error");

	dbus_message_unref(reply);
}

int cras_bt_device_connect_profile(DBusConnection *conn,
				   struct cras_bt_device *device,
				   const char *uuid)
{
	DBusMessage *method_call;
	DBusError dbus_error;
	DBusPendingCall *pending_call;

	method_call =
		dbus_message_new_method_call(BLUEZ_SERVICE, device->object_path,
					     BLUEZ_INTERFACE_DEVICE,
					     "ConnectProfile");
	if (!method_call)
		return -ENOMEM;

	if (!dbus_message_append_args(method_call, DBUS_TYPE_STRING, &uuid,
				      DBUS_TYPE_INVALID))
		return -ENOMEM;

	dbus_error_init(&dbus_error);

	pending_call = NULL;
	if (!dbus_connection_send_with_reply(conn, method_call, &pending_call,
					     DBUS_TIMEOUT_USE_DEFAULT)) {
		dbus_message_unref(method_call);
		syslog(LOG_ERR, "Failed to send Disconnect message");
		return -EIO;
	}

	dbus_message_unref(method_call);
	if (!dbus_pending_call_set_notify(
		    pending_call, on_connect_profile_reply, conn, NULL)) {
		dbus_pending_call_cancel(pending_call);
		dbus_pending_call_unref(pending_call);
		return -EIO;
	}
	return 0;
}

int cras_bt_device_disconnect(DBusConnection *conn,
			      struct cras_bt_device *device)
{
	DBusMessage *method_call;
	DBusError dbus_error;
	DBusPendingCall *pending_call;

	method_call =
		dbus_message_new_method_call(BLUEZ_SERVICE, device->object_path,
					     BLUEZ_INTERFACE_DEVICE,
					     "Disconnect");
	if (!method_call)
		return -ENOMEM;

	dbus_error_init(&dbus_error);

	pending_call = NULL;
	if (!dbus_connection_send_with_reply(conn, method_call, &pending_call,
					     DBUS_TIMEOUT_USE_DEFAULT)) {
		dbus_message_unref(method_call);
		syslog(LOG_ERR, "Failed to send Disconnect message");
		return -EIO;
	}

	dbus_message_unref(method_call);
	if (!dbus_pending_call_set_notify(pending_call, on_disconnect_reply,
					  conn, NULL)) {
		dbus_pending_call_cancel(pending_call);
		dbus_pending_call_unref(pending_call);
		return -EIO;
	}
	return 0;
}

static void cras_bt_device_destroy(struct cras_bt_device *device)
{
	struct cras_tm *tm = cras_system_state_get_tm();
	DL_DELETE(devices, device);

	if (device->conn_watch_timer)
		cras_tm_cancel_timer(tm, device->conn_watch_timer);
	if (device->switch_profile_timer)
		cras_tm_cancel_timer(tm, device->switch_profile_timer);
	if (device->suspend_timer)
		cras_tm_cancel_timer(tm, device->suspend_timer);
	free(device->adapter_obj_path);
	free(device->object_path);
	free(device->address);
	free(device->name);
	free(device);
}

void cras_bt_device_remove(struct cras_bt_device *device)
{
	/*
	 * We expect BT stack to disconnect this device before removing it,
	 * but it may not the case if there's issue at BT side. Print error
	 * log whenever this happens.
	 */
	if (device->connected)
		syslog(LOG_ERR, "Removing dev with connected profiles %u",
		       device->connected_profiles);
	/*
	 * Possibly clean up the associated A2DP and HFP AG iodevs that are
	 * still accessing this device.
	 */
	cras_a2dp_suspend_connected_device(device);
	cras_hfp_ag_suspend_connected_device(device);
	cras_bt_device_destroy(device);
}

void cras_bt_device_reset()
{
	while (devices) {
		syslog(LOG_INFO, "Bluetooth Device: %s removed",
		       devices->address);
		cras_bt_device_destroy(devices);
	}
}

struct cras_bt_device *cras_bt_device_get(const char *object_path)
{
	struct cras_bt_device *device;

	DL_FOREACH (devices, device) {
		if (strcmp(device->object_path, object_path) == 0)
			return device;
	}

	return NULL;
}

const char *cras_bt_device_object_path(const struct cras_bt_device *device)
{
	return device->object_path;
}

int cras_bt_device_get_stable_id(const struct cras_bt_device *device)
{
	return device->stable_id;
}

struct cras_bt_adapter *
cras_bt_device_adapter(const struct cras_bt_device *device)
{
	return cras_bt_adapter_get(device->adapter_obj_path);
}

const char *cras_bt_device_address(const struct cras_bt_device *device)
{
	return device->address;
}

const char *cras_bt_device_name(const struct cras_bt_device *device)
{
	return device->name;
}

int cras_bt_device_paired(const struct cras_bt_device *device)
{
	return device->paired;
}

int cras_bt_device_trusted(const struct cras_bt_device *device)
{
	return device->trusted;
}

int cras_bt_device_connected(const struct cras_bt_device *device)
{
	return device->connected;
}

int cras_bt_device_supports_profile(const struct cras_bt_device *device,
				    enum cras_bt_device_profile profile)
{
	return !!(device->profiles & profile);
}

void cras_bt_device_append_iodev(struct cras_bt_device *device,
				 struct cras_iodev *iodev,
				 enum cras_bt_device_profile profile)
{
	struct cras_iodev *bt_iodev;

	bt_iodev = device->bt_iodevs[iodev->direction];

	if (bt_iodev) {
		cras_bt_io_append(bt_iodev, iodev, profile);
	} else {
		device->bt_iodevs[iodev->direction] =
			cras_bt_io_create(device, iodev, profile);
	}
}

/*
 * Sets the audio nodes to 'plugged' means UI can select it and open it
 * for streams. Sets to 'unplugged' to hide these nodes from UI, when device
 * disconnects in progress.
 */
static void bt_device_set_nodes_plugged(struct cras_bt_device *device,
					int plugged)
{
	struct cras_iodev *iodev;

	iodev = device->bt_iodevs[CRAS_STREAM_INPUT];
	if (iodev)
		cras_iodev_set_node_plugged(iodev->active_node, plugged);

	iodev = device->bt_iodevs[CRAS_STREAM_OUTPUT];
	if (iodev)
		cras_iodev_set_node_plugged(iodev->active_node, plugged);
}

static void bt_device_switch_profile(struct cras_bt_device *device,
				     struct cras_iodev *bt_iodev,
				     int enable_dev);

void cras_bt_device_rm_iodev(struct cras_bt_device *device,
			     struct cras_iodev *iodev)
{
	struct cras_iodev *bt_iodev;
	int rc;

	bt_device_set_nodes_plugged(device, 0);

	bt_iodev = device->bt_iodevs[iodev->direction];
	if (bt_iodev) {
		unsigned try_profile;

		/* Check what will the preffered profile be if we remove dev. */
		try_profile = cras_bt_io_try_remove(bt_iodev, iodev);
		if (!try_profile)
			goto destroy_bt_io;

		/* If the check result doesn't match with the active
		 * profile we are currently using, switch to the
		 * preffered profile before actually remove the iodev.
		 */
		if (!cras_bt_io_on_profile(bt_iodev, try_profile)) {
			device->active_profile = try_profile;
			bt_device_switch_profile(device, bt_iodev, 0);
		}
		rc = cras_bt_io_remove(bt_iodev, iodev);
		if (rc) {
			syslog(LOG_ERR, "Fail to fallback to profile %u",
			       try_profile);
			goto destroy_bt_io;
		}
	}
	return;

destroy_bt_io:
	device->bt_iodevs[iodev->direction] = NULL;
	cras_bt_io_destroy(bt_iodev);

	if (!device->bt_iodevs[CRAS_STREAM_INPUT] &&
	    !device->bt_iodevs[CRAS_STREAM_OUTPUT])
		cras_bt_device_set_active_profile(device, 0);
}

void cras_bt_device_a2dp_configured(struct cras_bt_device *device)
{
	BTLOG(btlog, BT_A2DP_CONFIGURED, device->connected_profiles, 0);
	device->connected_profiles |= CRAS_BT_DEVICE_PROFILE_A2DP_SINK;
}

int cras_bt_device_has_a2dp(struct cras_bt_device *device)
{
	struct cras_iodev *odev = device->bt_iodevs[CRAS_STREAM_OUTPUT];

	/* Check if there is an output iodev with A2DP node attached. */
	return odev &&
	       cras_bt_io_get_profile(odev, CRAS_BT_DEVICE_PROFILE_A2DP_SOURCE);
}

int cras_bt_device_can_switch_to_a2dp(struct cras_bt_device *device)
{
	struct cras_iodev *idev = device->bt_iodevs[CRAS_STREAM_INPUT];

	return cras_bt_device_has_a2dp(device) &&
	       (!idev || !cras_iodev_is_open(idev));
}

static void bt_device_remove_conflict(struct cras_bt_device *device)
{
	struct cras_bt_device *connected;

	/* Suspend other HFP audio gateways that conflict with device. */
	cras_hfp_ag_remove_conflict(device);

	/* Check if there's conflict A2DP headset and suspend it. */
	connected = cras_a2dp_connected_device();
	if (connected && (connected != device))
		cras_a2dp_suspend_connected_device(connected);
}

static void bt_device_conn_watch_cb(struct cras_timer *timer, void *arg);

int cras_bt_device_audio_gateway_initialized(struct cras_bt_device *device)
{
	BTLOG(btlog, BT_AUDIO_GATEWAY_INIT, device->profiles, 0);
	/* Marks HFP/HSP as connected. This is what connection watcher
	 * checks. */
	device->connected_profiles |= (CRAS_BT_DEVICE_PROFILE_HFP_HANDSFREE |
				       CRAS_BT_DEVICE_PROFILE_HSP_HEADSET);

	/* If device connects HFP but not reporting correct UUID, manually add
	 * it to allow CRAS to enumerate audio node for it. We're seeing this
	 * behavior on qualification test software. */
	if (!cras_bt_device_supports_profile(
		    device, CRAS_BT_DEVICE_PROFILE_HFP_HANDSFREE)) {
		unsigned int profiles =
			device->profiles | CRAS_BT_DEVICE_PROFILE_HFP_HANDSFREE;
		cras_bt_device_set_supported_profiles(device, profiles);
		device->hidden_profiles |= CRAS_BT_DEVICE_PROFILE_HFP_HANDSFREE;
		bt_device_conn_watch_cb(NULL, (void *)device);
	}

	return 0;
}

unsigned int
cras_bt_device_get_active_profile(const struct cras_bt_device *device)
{
	return device->active_profile;
}

void cras_bt_device_set_active_profile(struct cras_bt_device *device,
				       unsigned int profile)
{
	device->active_profile = profile;
}

static void cras_bt_device_log_profile(const struct cras_bt_device *device,
				       enum cras_bt_device_profile profile)
{
	switch (profile) {
	case CRAS_BT_DEVICE_PROFILE_HFP_HANDSFREE:
		syslog(LOG_DEBUG, "Bluetooth Device: %s is HFP handsfree",
		       device->address);
		break;
	case CRAS_BT_DEVICE_PROFILE_HFP_AUDIOGATEWAY:
		syslog(LOG_DEBUG, "Bluetooth Device: %s is HFP audio gateway",
		       device->address);
		break;
	case CRAS_BT_DEVICE_PROFILE_A2DP_SOURCE:
		syslog(LOG_DEBUG, "Bluetooth Device: %s is A2DP source",
		       device->address);
		break;
	case CRAS_BT_DEVICE_PROFILE_A2DP_SINK:
		syslog(LOG_DEBUG, "Bluetooth Device: %s is A2DP sink",
		       device->address);
		break;
	case CRAS_BT_DEVICE_PROFILE_AVRCP_REMOTE:
		syslog(LOG_DEBUG, "Bluetooth Device: %s is AVRCP remote",
		       device->address);
		break;
	case CRAS_BT_DEVICE_PROFILE_AVRCP_TARGET:
		syslog(LOG_DEBUG, "Bluetooth Device: %s is AVRCP target",
		       device->address);
		break;
	case CRAS_BT_DEVICE_PROFILE_HSP_HEADSET:
		syslog(LOG_DEBUG, "Bluetooth Device: %s is HSP headset",
		       device->address);
		break;
	case CRAS_BT_DEVICE_PROFILE_HSP_AUDIOGATEWAY:
		syslog(LOG_DEBUG, "Bluetooth Device: %s is HSP audio gateway",
		       device->address);
		break;
	}
}

static void cras_bt_device_log_profiles(const struct cras_bt_device *device,
					unsigned int profiles)
{
	unsigned int profile;

	while (profiles) {
		/* Get the LSB of profiles */
		profile = profiles & -profiles;
		cras_bt_device_log_profile(device, profile);
		profiles ^= profile;
	}
}

static int
cras_bt_device_is_profile_connected(const struct cras_bt_device *device,
				    enum cras_bt_device_profile profile)
{
	return !!(device->connected_profiles & profile);
}

static void
bt_device_schedule_suspend(struct cras_bt_device *device, unsigned int msec,
			   enum cras_bt_device_suspend_reason suspend_reason);

/* Callback used to periodically check if supported profiles are connected. */
static void bt_device_conn_watch_cb(struct cras_timer *timer, void *arg)
{
	struct cras_tm *tm;
	struct cras_bt_device *device = (struct cras_bt_device *)arg;
	int rc;
	bool a2dp_supported;
	bool a2dp_connected;
	bool hfp_supported;
	bool hfp_connected;

	BTLOG(btlog, BT_DEV_CONN_WATCH_CB, device->conn_watch_retries,
	      device->profiles);
	device->conn_watch_timer = NULL;

	/* Skip the callback if it is not an audio device. */
	if (!device->profiles)
		return;

	a2dp_supported = cras_bt_device_supports_profile(
		device, CRAS_BT_DEVICE_PROFILE_A2DP_SINK);
	a2dp_connected = cras_bt_device_is_profile_connected(
		device, CRAS_BT_DEVICE_PROFILE_A2DP_SINK);
	hfp_supported = cras_bt_device_supports_profile(
		device, CRAS_BT_DEVICE_PROFILE_HFP_HANDSFREE);
	hfp_connected = cras_bt_device_is_profile_connected(
		device, CRAS_BT_DEVICE_PROFILE_HFP_HANDSFREE);

	/* If not both A2DP and HFP are supported, simply wait for BlueZ
	 * to notify us about the new connection.
	 * Otherwise, when seeing one but not the other profile is connected,
	 * send message to ask BlueZ to connect the pending one.
	 */
	if (a2dp_supported && hfp_supported) {
		/* If both a2dp and hfp are not connected, do nothing. BlueZ
		 * should be responsible to notify connection of one profile.
		 */
		if (!a2dp_connected && hfp_connected)
			cras_bt_device_connect_profile(device->conn, device,
						       A2DP_SINK_UUID);
		if (a2dp_connected && !hfp_connected)
			cras_bt_device_connect_profile(device->conn, device,
						       HFP_HF_UUID);
	}

	if (a2dp_supported != a2dp_connected || hfp_supported != hfp_connected)
		goto arm_retry_timer;

	/* Expected profiles are all connected, no more connection watch
	 * callback will be scheduled.
	 * Base on the decision that we expose only the latest connected
	 * BT audio device to user, treat all other connected devices as
	 * conflict and remove them before we start A2DP/HFP of this device.
	 */
	bt_device_remove_conflict(device);

	if (cras_bt_device_is_profile_connected(
		    device, CRAS_BT_DEVICE_PROFILE_A2DP_SINK))
		cras_a2dp_start(device);

	if (cras_bt_device_is_profile_connected(
		    device, CRAS_BT_DEVICE_PROFILE_HFP_HANDSFREE)) {
		rc = cras_hfp_ag_start(device);
		if (rc) {
			syslog(LOG_ERR, "Start audio gateway failed, rc %d",
			       rc);
			bt_device_schedule_suspend(device, 0,
						   HFP_AG_START_FAILURE);
		}
	}
	bt_device_set_nodes_plugged(device, 1);
	return;

arm_retry_timer:

	syslog(LOG_DEBUG, "conn_watch_retries: %d", device->conn_watch_retries);

	if (--device->conn_watch_retries) {
		tm = cras_system_state_get_tm();
		device->conn_watch_timer =
			cras_tm_create_timer(tm, CONN_WATCH_PERIOD_MS,
					     bt_device_conn_watch_cb, device);
	} else {
		syslog(LOG_ERR, "Connection watch timeout.");
		bt_device_schedule_suspend(device, 0, CONN_WATCH_TIME_OUT);
	}
}

static void
cras_bt_device_start_new_conn_watch_timer(struct cras_bt_device *device)
{
	struct cras_tm *tm = cras_system_state_get_tm();

	if (device->conn_watch_timer) {
		cras_tm_cancel_timer(tm, device->conn_watch_timer);
	}
	device->conn_watch_retries = CONN_WATCH_MAX_RETRIES;
	device->conn_watch_timer = cras_tm_create_timer(
		tm, CONN_WATCH_PERIOD_MS, bt_device_conn_watch_cb, device);
}

static void bt_device_cancel_suspend(struct cras_bt_device *device);

void cras_bt_device_set_connected(struct cras_bt_device *device, int value)
{
	struct cras_tm *tm = cras_system_state_get_tm();
	if (!device->connected && value) {
		BTLOG(btlog, BT_DEV_CONNECTED, device->profiles,
		      device->stable_id);
	}

	if (device->connected && !value) {
		BTLOG(btlog, BT_DEV_DISCONNECTED, device->profiles,
		      device->stable_id);
		cras_bt_profile_on_device_disconnected(device);
		/* Device is disconnected, resets connected profiles and the
		 * suspend timer which scheduled earlier. */
		device->connected_profiles = 0;
		bt_device_cancel_suspend(device);
	}

	device->connected = value;

	if (!device->connected && device->conn_watch_timer) {
		cras_tm_cancel_timer(tm, device->conn_watch_timer);
		device->conn_watch_timer = NULL;
	}
}

void cras_bt_device_notify_profile_dropped(struct cras_bt_device *device,
					   enum cras_bt_device_profile profile)
{
	device->connected_profiles &= ~profile;

	/* Do nothing if device already disconnected. */
	if (!device->connected)
		return;

	/* If any profile, a2dp or hfp/hsp, has dropped for some reason,
	 * we shall make sure this device is fully disconnected within
	 * given time so that user does not see a headset stay connected
	 * but works with partial function.
	 */
	bt_device_schedule_suspend(device, PROFILE_DROP_SUSPEND_DELAY_MS,
				   UNEXPECTED_PROFILE_DROP);
}

/* Refresh the list of known supported profiles.
 * Args:
 *    device - The BT device holding scanned profiles bitmap.
 *    profiles - The OR'ed profiles the device claims to support as is notified
 *               by BlueZ.
 * Returns:
 *    The OR'ed profiles that are both supported by Cras and isn't previously
 *    supported by the device.
 */
int cras_bt_device_set_supported_profiles(struct cras_bt_device *device,
					  unsigned int profiles)
{
	/* Do nothing if no new profiles. */
	if ((device->profiles & profiles) == profiles)
		return 0;

	unsigned int new_profiles = profiles & ~device->profiles;

	/* Log this event as we might need to re-intialize the BT audio nodes
	 * if new audio profile is reported for already connected device. */
	if (device->connected && (new_profiles & CRAS_SUPPORTED_PROFILES))
		BTLOG(btlog, BT_NEW_AUDIO_PROFILE_AFTER_CONNECT,
		      device->profiles, new_profiles);
	cras_bt_device_log_profiles(device, new_profiles);
	device->profiles = profiles | device->hidden_profiles;

	return (new_profiles & CRAS_SUPPORTED_PROFILES);
}

void cras_bt_device_update_properties(struct cras_bt_device *device,
				      DBusMessageIter *properties_array_iter,
				      DBusMessageIter *invalidated_array_iter)
{
	int watch_needed = 0;
	while (dbus_message_iter_get_arg_type(properties_array_iter) !=
	       DBUS_TYPE_INVALID) {
		DBusMessageIter properties_dict_iter, variant_iter;
		const char *key;
		int type;

		dbus_message_iter_recurse(properties_array_iter,
					  &properties_dict_iter);

		dbus_message_iter_get_basic(&properties_dict_iter, &key);
		dbus_message_iter_next(&properties_dict_iter);

		dbus_message_iter_recurse(&properties_dict_iter, &variant_iter);
		type = dbus_message_iter_get_arg_type(&variant_iter);

		if (type == DBUS_TYPE_STRING || type == DBUS_TYPE_OBJECT_PATH) {
			const char *value;

			dbus_message_iter_get_basic(&variant_iter, &value);

			if (strcmp(key, "Adapter") == 0) {
				free(device->adapter_obj_path);
				device->adapter_obj_path = strdup(value);
			} else if (strcmp(key, "Address") == 0) {
				free(device->address);
				device->address = strdup(value);
			} else if (strcmp(key, "Alias") == 0) {
				free(device->name);
				device->name = strdup(value);
			}

		} else if (type == DBUS_TYPE_UINT32) {
			uint32_t value;

			dbus_message_iter_get_basic(&variant_iter, &value);

			if (strcmp(key, "Class") == 0)
				device->bluetooth_class = value;

		} else if (type == DBUS_TYPE_BOOLEAN) {
			int value;

			dbus_message_iter_get_basic(&variant_iter, &value);

			if (strcmp(key, "Paired") == 0) {
				device->paired = value;
			} else if (strcmp(key, "Trusted") == 0) {
				device->trusted = value;
			} else if (strcmp(key, "Connected") == 0) {
				cras_bt_device_set_connected(device, value);
				watch_needed = device->connected &&
					       cras_bt_device_supports_profile(
						       device,
						       CRAS_SUPPORTED_PROFILES);
			}

		} else if (strcmp(dbus_message_iter_get_signature(&variant_iter),
				  "as") == 0 &&
			   strcmp(key, "UUIDs") == 0) {
			DBusMessageIter uuid_array_iter;
			unsigned int profiles = 0;

			dbus_message_iter_recurse(&variant_iter,
						  &uuid_array_iter);
			while (dbus_message_iter_get_arg_type(
				       &uuid_array_iter) != DBUS_TYPE_INVALID) {
				const char *uuid;

				dbus_message_iter_get_basic(&uuid_array_iter,
							    &uuid);
				profiles |=
					cras_bt_device_profile_from_uuid(uuid);

				dbus_message_iter_next(&uuid_array_iter);
			}

			/* If updated properties includes new audio profile and
			 * device is connected, we need to start connection
			 * watcher. This is needed because on some bluetooth
			 * devices, supported profiles do not present when
			 * device interface is added and they are updated later.
			 */
			if (cras_bt_device_set_supported_profiles(device,
								  profiles))
				watch_needed = device->connected;
		}

		dbus_message_iter_next(properties_array_iter);
	}

	while (invalidated_array_iter &&
	       dbus_message_iter_get_arg_type(invalidated_array_iter) !=
		       DBUS_TYPE_INVALID) {
		const char *key;

		dbus_message_iter_get_basic(invalidated_array_iter, &key);

		if (strcmp(key, "Adapter") == 0) {
			free(device->adapter_obj_path);
			device->adapter_obj_path = NULL;
		} else if (strcmp(key, "Address") == 0) {
			free(device->address);
			device->address = NULL;
		} else if (strcmp(key, "Alias") == 0) {
			free(device->name);
			device->name = NULL;
		} else if (strcmp(key, "Class") == 0) {
			device->bluetooth_class = 0;
		} else if (strcmp(key, "Paired") == 0) {
			device->paired = 0;
		} else if (strcmp(key, "Trusted") == 0) {
			device->trusted = 0;
		} else if (strcmp(key, "Connected") == 0) {
			device->connected = 0;
		} else if (strcmp(key, "UUIDs") == 0) {
			device->profiles = device->hidden_profiles;
		}

		dbus_message_iter_next(invalidated_array_iter);
	}

	if (watch_needed)
		cras_bt_device_start_new_conn_watch_timer(device);
}

/* Converts bluetooth address string into sockaddr structure. The address
 * string is expected of the form 1A:2B:3C:4D:5E:6F, and each of the six
 * hex values will be parsed into sockaddr in inverse order.
 * Args:
 *    str - The string version of bluetooth address
 *    addr - The struct to be filled with converted address
 */
static int bt_address(const char *str, struct sockaddr *addr)
{
	int i;

	if (strlen(str) != 17) {
		syslog(LOG_ERR, "Invalid bluetooth address %s", str);
		return -1;
	}

	memset(addr, 0, sizeof(*addr));
	addr->sa_family = AF_BLUETOOTH;
	for (i = 5; i >= 0; i--) {
		addr->sa_data[i] = (unsigned char)strtol(str, NULL, 16);
		str += 3;
	}

	return 0;
}

/* Apply codec specific settings to the socket fd. */
static int apply_codec_settings(int fd, uint8_t codec)
{
	struct bt_voice voice;
	uint32_t pkt_status;

	memset(&voice, 0, sizeof(voice));
	if (codec == HFP_CODEC_ID_CVSD)
		return 0;

	if (codec != HFP_CODEC_ID_MSBC) {
		syslog(LOG_ERR, "Unsupported codec %d", codec);
		return -1;
	}

	voice.setting = BT_VOICE_TRANSPARENT;

	if (setsockopt(fd, SOL_BLUETOOTH, BT_VOICE, &voice, sizeof(voice)) <
	    0) {
		syslog(LOG_ERR, "Failed to apply voice setting");
		return -1;
	}

	pkt_status = 1;
	if (setsockopt(fd, SOL_BLUETOOTH, BT_PKT_STATUS, &pkt_status,
		       sizeof(pkt_status))) {
		syslog(LOG_ERR, "Failed to enable BT_PKT_STATUS");
	}
	return 0;
}

int cras_bt_device_sco_connect(struct cras_bt_device *device, int codec)
{
	int sk = 0, err;
	struct sockaddr addr;
	struct cras_bt_adapter *adapter;
	struct timespec timeout = { 1, 0 };
	struct pollfd pollfd;

	adapter = cras_bt_device_adapter(device);
	if (!adapter) {
		syslog(LOG_ERR, "No adapter found for device %s at SCO connect",
		       cras_bt_device_object_path(device));
		goto error;
	}

	sk = socket(PF_BLUETOOTH, SOCK_SEQPACKET | O_NONBLOCK | SOCK_CLOEXEC,
		    BTPROTO_SCO);
	if (sk < 0) {
		syslog(LOG_ERR, "Failed to create socket: %s (%d)",
		       strerror(errno), errno);
		cras_server_metrics_hfp_sco_connection_error(
			CRAS_METRICS_SCO_SKT_OPEN_ERROR);
		return -errno;
	}

	/* Bind to local address */
	if (bt_address(cras_bt_adapter_address(adapter), &addr))
		goto error;
	if (bind(sk, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		syslog(LOG_ERR, "Failed to bind socket: %s (%d)",
		       strerror(errno), errno);
		goto error;
	}

	/* Connect to remote in nonblocking mode */
	fcntl(sk, F_SETFL, O_NONBLOCK);

	if (bt_address(cras_bt_device_address(device), &addr))
		goto error;

	err = apply_codec_settings(sk, codec);
	if (err)
		goto error;

	err = connect(sk, (struct sockaddr *)&addr, sizeof(addr));
	if (err && errno != EINPROGRESS) {
		syslog(LOG_ERR, "Failed to connect: %s (%d)", strerror(errno),
		       errno);
		cras_server_metrics_hfp_sco_connection_error(
			CRAS_METRICS_SCO_SKT_CONNECT_ERROR);
		goto error;
	}

	pollfd.fd = sk;
	pollfd.events = POLLOUT;

	err = ppoll(&pollfd, 1, &timeout, NULL);
	if (err <= 0) {
		syslog(LOG_ERR, "Connect SCO: poll for writable timeout");
		cras_server_metrics_hfp_sco_connection_error(
			CRAS_METRICS_SCO_SKT_POLL_TIMEOUT);
		goto error;
	}

	if (pollfd.revents & (POLLERR | POLLHUP)) {
		syslog(LOG_ERR,
		       "SCO socket error, revents: %u. Suspend in %u seconds",
		       pollfd.revents, SCO_SUSPEND_DELAY_MS);
		cras_server_metrics_hfp_sco_connection_error(
			CRAS_METRICS_SCO_SKT_POLL_ERR_HUP);
		bt_device_schedule_suspend(device, SCO_SUSPEND_DELAY_MS,
					   HFP_SCO_SOCKET_ERROR);
		goto error;
	}

	cras_server_metrics_hfp_sco_connection_error(
		CRAS_METRICS_SCO_SKT_SUCCESS);
	BTLOG(btlog, BT_SCO_CONNECT, 1, sk);
	return sk;

error:
	BTLOG(btlog, BT_SCO_CONNECT, 0, sk);
	if (sk)
		close(sk);
	return -1;
}

int cras_bt_device_sco_packet_size(struct cras_bt_device *device,
				   int sco_socket, int codec)
{
	struct sco_options so;
	socklen_t len = sizeof(so);
	struct cras_bt_adapter *adapter;
	uint32_t wbs_pkt_len = 0;
	socklen_t optlen = sizeof(wbs_pkt_len);

	adapter = cras_bt_adapter_get(device->adapter_obj_path);

	if (cras_bt_adapter_on_usb(adapter)) {
		if (codec == HFP_CODEC_ID_MSBC) {
			/* BT_SNDMTU and BT_RCVMTU return the same value. */
			if (getsockopt(sco_socket, SOL_BLUETOOTH, BT_SNDMTU,
				       &wbs_pkt_len, &optlen))
				syslog(LOG_ERR, "Failed to get BT_SNDMTU");

			return (wbs_pkt_len > 0) ? wbs_pkt_len :
						   USB_MSBC_PKT_SIZE;
		} else {
			return USB_CVSD_PKT_SIZE;
		}
	}

	/* For non-USB cases, query the SCO MTU from driver. */
	if (getsockopt(sco_socket, SOL_SCO, SCO_OPTIONS, &so, &len) < 0) {
		syslog(LOG_ERR, "Get SCO options error: %s", strerror(errno));
		return DEFAULT_SCO_PKT_SIZE;
	}
	return so.mtu;
}

void cras_bt_device_set_use_hardware_volume(struct cras_bt_device *device,
					    int use_hardware_volume)
{
	struct cras_iodev *iodev;

	device->use_hardware_volume = use_hardware_volume;
	iodev = device->bt_iodevs[CRAS_STREAM_OUTPUT];
	if (iodev)
		iodev->software_volume_needed = !use_hardware_volume;
}

int cras_bt_device_get_use_hardware_volume(struct cras_bt_device *device)
{
	return device->use_hardware_volume;
}

static void init_bt_device_msg(struct bt_device_msg *msg,
			       enum BT_DEVICE_COMMAND cmd,
			       struct cras_bt_device *device,
			       struct cras_iodev *dev, unsigned int arg1,
			       unsigned int arg2)
{
	memset(msg, 0, sizeof(*msg));
	msg->header.type = CRAS_MAIN_BT;
	msg->header.length = sizeof(*msg);
	msg->cmd = cmd;
	msg->device = device;
	msg->dev = dev;
	msg->arg1 = arg1;
	msg->arg2 = arg2;
}

int cras_bt_device_cancel_suspend(struct cras_bt_device *device)
{
	struct bt_device_msg msg;
	int rc;

	init_bt_device_msg(&msg, BT_DEVICE_CANCEL_SUSPEND, device, NULL, 0, 0);
	rc = cras_main_message_send((struct cras_main_message *)&msg);
	return rc;
}

int cras_bt_device_schedule_suspend(
	struct cras_bt_device *device, unsigned int msec,
	enum cras_bt_device_suspend_reason suspend_reason)
{
	struct bt_device_msg msg;
	int rc;

	init_bt_device_msg(&msg, BT_DEVICE_SCHEDULE_SUSPEND, device, NULL, msec,
			   suspend_reason);
	rc = cras_main_message_send((struct cras_main_message *)&msg);
	return rc;
}

/* This diagram describes how the profile switching happens. When
 * certain conditions met, bt iodev will call the APIs below to interact
 * with main thread to switch to another active profile.
 *
 * Audio thread:
 *  +--------------------------------------------------------------+
 *  | bt iodev                                                     |
 *  |              +------------------+    +-----------------+     |
 *  |              | condition met to |    | open, close, or |     |
 *  |           +--| change profile   |<---| append profile  |<--+ |
 *  |           |  +------------------+    +-----------------+   | |
 *  +-----------|------------------------------------------------|-+
 *              |                                                |
 * Main thread: |
 *  +-----------|------------------------------------------------|-+
 *  |           |                                                | |
 *  |           |      +------------+     +----------------+     | |
 *  |           +----->| set active |---->| switch profile |-----+ |
 *  |                  | profile    |     +----------------+       |
 *  | bt device        +------------+                              |
 *  +--------------------------------------------------------------+
 */
int cras_bt_device_switch_profile_enable_dev(struct cras_bt_device *device,
					     struct cras_iodev *bt_iodev)
{
	struct bt_device_msg msg;
	int rc;

	init_bt_device_msg(&msg, BT_DEVICE_SWITCH_PROFILE_ENABLE_DEV, device,
			   bt_iodev, 0, 0);
	rc = cras_main_message_send((struct cras_main_message *)&msg);
	return rc;
}

int cras_bt_device_switch_profile(struct cras_bt_device *device,
				  struct cras_iodev *bt_iodev)
{
	struct bt_device_msg msg;
	int rc;

	init_bt_device_msg(&msg, BT_DEVICE_SWITCH_PROFILE, device, bt_iodev, 0,
			   0);
	rc = cras_main_message_send((struct cras_main_message *)&msg);
	return rc;
}

static void profile_switch_delay_cb(struct cras_timer *timer, void *arg)
{
	struct cras_bt_device *device = (struct cras_bt_device *)arg;
	struct cras_iodev *iodev;

	device->switch_profile_timer = NULL;
	iodev = device->bt_iodevs[CRAS_STREAM_OUTPUT];
	if (!iodev)
		return;

	/*
	 * During the |PROFILE_SWITCH_DELAY_MS| time interval, BT iodev could
	 * have been enabled by others, and its active profile may have changed.
	 * If iodev has been enabled, that means it has already picked up a
	 * reasonable profile to use and audio thread is accessing iodev now.
	 * We should NOT call into update_active_node from main thread
	 * because that may mess up the active node content.
	 */
	iodev->update_active_node(iodev, 0, 1);
	cras_iodev_list_resume_dev(iodev->info.idx);
}

static void bt_device_switch_profile_with_delay(struct cras_bt_device *device,
						unsigned int delay_ms)
{
	struct cras_tm *tm = cras_system_state_get_tm();

	if (device->switch_profile_timer) {
		cras_tm_cancel_timer(tm, device->switch_profile_timer);
		device->switch_profile_timer = NULL;
	}
	device->switch_profile_timer = cras_tm_create_timer(
		tm, delay_ms, profile_switch_delay_cb, device);
}

/* Switches associated bt iodevs to use the active profile. This is
 * achieved by close the iodevs, update their active nodes, and then
 * finally reopen them. */
static void bt_device_switch_profile(struct cras_bt_device *device,
				     struct cras_iodev *bt_iodev,
				     int enable_dev)
{
	struct cras_iodev *iodev;
	int dir;

	/* If a bt iodev is active, temporarily force close it.
	 * Note that we need to check all bt_iodevs for the situation that both
	 * input and output are active while switches from HFP/HSP to A2DP.
	 */
	for (dir = 0; dir < CRAS_NUM_DIRECTIONS; dir++) {
		iodev = device->bt_iodevs[dir];
		if (!iodev)
			continue;
		cras_iodev_list_suspend_dev(iodev->info.idx);
	}

	for (dir = 0; dir < CRAS_NUM_DIRECTIONS; dir++) {
		iodev = device->bt_iodevs[dir];
		if (!iodev)
			continue;

		/* If the iodev was active or this profile switching is
		 * triggered at opening iodev, add it to active dev list.
		 * However for the output iodev, adding it back to active dev
		 * list could cause immediate switching from HFP to A2DP if
		 * there exists an output stream. Certain headset/speaker
		 * would fail to playback afterwards when the switching happens
		 * too soon, so put this task in a delayed callback.
		 */
		if (dir == CRAS_STREAM_INPUT) {
			iodev->update_active_node(iodev, 0, 1);
			cras_iodev_list_resume_dev(iodev->info.idx);
		} else {
			bt_device_switch_profile_with_delay(
				device, PROFILE_SWITCH_DELAY_MS);
		}
	}
}

static void bt_device_suspend_cb(struct cras_timer *timer, void *arg)
{
	struct cras_bt_device *device = (struct cras_bt_device *)arg;

	BTLOG(btlog, BT_DEV_SUSPEND_CB, device->profiles,
	      device->suspend_reason);
	device->suspend_timer = NULL;

	/* Error log the reason so we can track them in user reports. */
	switch (device->suspend_reason) {
	case A2DP_LONG_TX_FAILURE:
		syslog(LOG_ERR, "Suspend dev: A2DP long Tx failure");
		break;
	case A2DP_TX_FATAL_ERROR:
		syslog(LOG_ERR, "Suspend dev: A2DP Tx fatal error");
		break;
	case CONN_WATCH_TIME_OUT:
		syslog(LOG_ERR, "Suspend dev: Conn watch times out");
		break;
	case HFP_SCO_SOCKET_ERROR:
		syslog(LOG_ERR, "Suspend dev: SCO socket error");
		break;
	case HFP_AG_START_FAILURE:
		syslog(LOG_ERR, "Suspend dev: HFP AG start failure");
		break;
	case UNEXPECTED_PROFILE_DROP:
		syslog(LOG_ERR, "Suspend dev: Unexpected profile drop");
		break;
	}

	cras_a2dp_suspend_connected_device(device);
	cras_hfp_ag_suspend_connected_device(device);
	cras_bt_device_disconnect(device->conn, device);
}

static void
bt_device_schedule_suspend(struct cras_bt_device *device, unsigned int msec,
			   enum cras_bt_device_suspend_reason suspend_reason)
{
	struct cras_tm *tm = cras_system_state_get_tm();

	if (device->suspend_timer)
		return;
	device->suspend_reason = suspend_reason;
	device->suspend_timer =
		cras_tm_create_timer(tm, msec, bt_device_suspend_cb, device);
}

static void bt_device_cancel_suspend(struct cras_bt_device *device)
{
	struct cras_tm *tm = cras_system_state_get_tm();
	if (device->suspend_timer == NULL)
		return;
	cras_tm_cancel_timer(tm, device->suspend_timer);
	device->suspend_timer = NULL;
}

static void bt_device_process_msg(struct cras_main_message *msg, void *arg)
{
	struct bt_device_msg *bt_msg = (struct bt_device_msg *)msg;
	struct cras_bt_device *device = NULL;

	DL_FOREACH (devices, device) {
		if (device == bt_msg->device)
			break;
	}

	/* Do nothing if target device no longer exists. */
	if (device == NULL)
		return;

	switch (bt_msg->cmd) {
	case BT_DEVICE_SWITCH_PROFILE:
		bt_device_switch_profile(bt_msg->device, bt_msg->dev, 0);
		break;
	case BT_DEVICE_SWITCH_PROFILE_ENABLE_DEV:
		bt_device_switch_profile(bt_msg->device, bt_msg->dev, 1);
		break;
	case BT_DEVICE_SCHEDULE_SUSPEND:
		bt_device_schedule_suspend(bt_msg->device, bt_msg->arg1,
					   bt_msg->arg2);
		break;
	case BT_DEVICE_CANCEL_SUSPEND:
		bt_device_cancel_suspend(bt_msg->device);
		break;
	default:
		break;
	}
}

void cras_bt_device_start_monitor()
{
	cras_main_message_add_handler(CRAS_MAIN_BT, bt_device_process_msg,
				      NULL);
}

void cras_bt_device_update_hardware_volume(struct cras_bt_device *device,
					   int volume)
{
	struct cras_iodev *iodev;

	iodev = device->bt_iodevs[CRAS_STREAM_OUTPUT];
	if (iodev == NULL)
		return;

	/* Check if this BT device is okay to use hardware volume. If not
	 * then ignore the reported volume change event.
	 */
	if (!cras_bt_device_get_use_hardware_volume(device))
		return;

	iodev->active_node->volume = volume;
	cras_iodev_list_notify_node_volume(iodev->active_node);
}

int cras_bt_device_get_sco(struct cras_bt_device *device, int codec)
{
	if (device->sco_ref_count == 0) {
		device->sco_fd = cras_bt_device_sco_connect(device, codec);
		if (device->sco_fd < 0)
			return device->sco_fd;
	}

	++device->sco_ref_count;
	return 0;
}

void cras_bt_device_put_sco(struct cras_bt_device *device)
{
	if (device->sco_ref_count == 0)
		return;

	if (--device->sco_ref_count == 0)
		close(device->sco_fd);
}
