/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <dbus/dbus.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include "cras_bt_adapter.h"
#include "cras_bt_battery_provider.h"
#include "cras_bt_constants.h"
#include "cras_dbus_util.h"
#include "cras_observer.h"
#include "utlist.h"

/* CRAS registers one battery provider to BlueZ, so we use a singleton. */
static struct cras_bt_battery_provider battery_provider = {
	.object_path = CRAS_DEFAULT_BATTERY_PROVIDER,
	.interface = BLUEZ_INTERFACE_BATTERY_PROVIDER,
	.conn = NULL,
	.is_registered = false,
	.observer = NULL,
	.batteries = NULL,
};

static int cmp_battery_address(const struct cras_bt_battery *battery,
			       const char *address)
{
	return strcmp(battery->address, address);
}

static void replace_colon_with_underscore(char *str)
{
	for (int i = 0; str[i]; i++) {
		if (str[i] == ':')
			str[i] = '_';
	}
}

/* Converts address XX:XX:XX:XX:XX:XX to Battery Provider object path:
 * /org/chromium/Cras/Bluetooth/BatteryProvider/XX_XX_XX_XX_XX_XX
 */
static char *address_to_battery_path(const char *address)
{
	char *object_path = malloc(strlen(CRAS_DEFAULT_BATTERY_PROVIDER) +
				   strlen(address) + 2);

	sprintf(object_path, "%s/%s", CRAS_DEFAULT_BATTERY_PROVIDER, address);
	replace_colon_with_underscore(object_path);

	return object_path;
}

/* Converts address XX:XX:XX:XX:XX:XX to device object path:
 * /org/bluez/hci0/dev_XX_XX_XX_XX_XX_XX
 */
static char *address_to_device_path(const char *address)
{
	char *object_path = malloc(strlen(CRAS_DEFAULT_BATTERY_PREFIX) +
				   strlen(address) + 1);

	sprintf(object_path, "%s%s", CRAS_DEFAULT_BATTERY_PREFIX, address);
	replace_colon_with_underscore(object_path);

	return object_path;
}

static struct cras_bt_battery *battery_new(const char *address, uint32_t level)
{
	struct cras_bt_battery *battery;

	battery = calloc(1, sizeof(struct cras_bt_battery));
	battery->address = strdup(address);
	battery->object_path = address_to_battery_path(address);
	battery->device_path = address_to_device_path(address);
	battery->level = level;

	return battery;
}

static void battery_free(struct cras_bt_battery *battery)
{
	if (battery->address)
		free(battery->address);
	if (battery->object_path)
		free(battery->object_path);
	if (battery->device_path)
		free(battery->device_path);
	free(battery);
}

static void populate_battery_properties(DBusMessageIter *iter,
					const struct cras_bt_battery *battery)
{
	DBusMessageIter dict, entry, variant;
	const char *property_percentage = "Percentage";
	const char *property_device = "Device";
	uint8_t level = battery->level;

	dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, "{sv}", &dict);

	dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, NULL,
					 &entry);
	dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING,
				       &property_percentage);
	dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT,
					 DBUS_TYPE_BYTE_AS_STRING, &variant);
	dbus_message_iter_append_basic(&variant, DBUS_TYPE_BYTE, &level);
	dbus_message_iter_close_container(&entry, &variant);
	dbus_message_iter_close_container(&dict, &entry);

	dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, NULL,
					 &entry);
	dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING,
				       &property_device);
	dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT,
					 DBUS_TYPE_OBJECT_PATH_AS_STRING,
					 &variant);
	dbus_message_iter_append_basic(&variant, DBUS_TYPE_OBJECT_PATH,
				       &battery->device_path);
	dbus_message_iter_close_container(&entry, &variant);
	dbus_message_iter_close_container(&dict, &entry);

	dbus_message_iter_close_container(iter, &dict);
}

/* Creates a new battery object and exposes it on D-Bus. */
static struct cras_bt_battery *
get_or_create_battery(struct cras_bt_battery_provider *provider,
		      const char *address, uint32_t level)
{
	struct cras_bt_battery *battery;
	DBusMessage *msg;
	DBusMessageIter iter, dict, entry;

	LL_SEARCH(provider->batteries, battery, address, cmp_battery_address);

	if (battery)
		return battery;

	syslog(LOG_DEBUG, "Creating new battery for %s", address);

	battery = battery_new(address, level);
	LL_APPEND(provider->batteries, battery);

	msg = dbus_message_new_signal(CRAS_DEFAULT_BATTERY_PROVIDER,
				      DBUS_INTERFACE_OBJECT_MANAGER,
				      DBUS_SIGNAL_INTERFACES_ADDED);

	dbus_message_iter_init_append(msg, &iter);
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_OBJECT_PATH,
				       &battery->object_path);
	dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sa{sv}}",
					 &dict);
	dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, NULL,
					 &entry);
	dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING,
				       &provider->interface);
	populate_battery_properties(&entry, battery);
	dbus_message_iter_close_container(&dict, &entry);
	dbus_message_iter_close_container(&iter, &dict);

	if (!dbus_connection_send(provider->conn, msg, NULL)) {
		syslog(LOG_ERR,
		       "Error sending " DBUS_SIGNAL_INTERFACES_ADDED " signal");
	}

	dbus_message_unref(msg);

	return battery;
}

/* Updates the level of a battery object and signals it on D-Bus. */
static void
update_battery_level(const struct cras_bt_battery_provider *provider,
		     struct cras_bt_battery *battery, uint32_t level)
{
	DBusMessage *msg;
	DBusMessageIter iter;

	if (battery->level == level)
		return;

	battery->level = level;

	msg = dbus_message_new_signal(battery->object_path,
				      DBUS_INTERFACE_PROPERTIES,
				      DBUS_SIGNAL_PROPERTIES_CHANGED);

	dbus_message_iter_init_append(msg, &iter);
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING,
				       &provider->interface);
	populate_battery_properties(&iter, battery);

	if (!dbus_connection_send(provider->conn, msg, NULL)) {
		syslog(LOG_ERR, "Error sending " DBUS_SIGNAL_PROPERTIES_CHANGED
				" signal");
	}

	dbus_message_unref(msg);
}

/* Invoked when HFP sends an alert about a battery value change. */
static void on_bt_battery_changed(void *context, const char *address,
				  uint32_t level)
{
	struct cras_bt_battery_provider *provider = context;

	syslog(LOG_DEBUG, "Battery changed for address %s, level %d", address,
	       level);

	if (!provider->is_registered) {
		syslog(LOG_WARNING, "Received battery level update while "
				    "battery provider is not registered");
		return;
	}

	struct cras_bt_battery *battery =
		get_or_create_battery(provider, address, level);

	update_battery_level(provider, battery, level);
}

/* Invoked when we receive a D-Bus return of RegisterBatteryProvider from
 * BlueZ.
 */
static void
cras_bt_on_battery_provider_registered(DBusPendingCall *pending_call,
				       void *data)
{
	DBusMessage *reply;
	struct cras_bt_battery_provider *provider = data;
	struct cras_observer_ops observer_ops;

	reply = dbus_pending_call_steal_reply(pending_call);
	dbus_pending_call_unref(pending_call);

	if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
		syslog(LOG_ERR, "RegisterBatteryProvider returned error: %s",
		       dbus_message_get_error_name(reply));
		dbus_message_unref(reply);
		return;
	}

	syslog(LOG_INFO, "RegisterBatteryProvider succeeded");

	provider->is_registered = true;

	memset(&observer_ops, 0, sizeof(observer_ops));
	observer_ops.bt_battery_changed = on_bt_battery_changed;
	provider->observer = cras_observer_add(&observer_ops, provider);

	dbus_message_unref(reply);
}

int cras_bt_register_battery_provider(DBusConnection *conn,
				      const struct cras_bt_adapter *adapter)
{
	const char *adapter_path;
	DBusMessage *method_call;
	DBusMessageIter message_iter;
	DBusPendingCall *pending_call;

	if (battery_provider.is_registered) {
		syslog(LOG_ERR, "Battery Provider already registered");
		return -EBUSY;
	}

	if (battery_provider.conn)
		dbus_connection_unref(battery_provider.conn);

	battery_provider.conn = conn;
	dbus_connection_ref(battery_provider.conn);

	adapter_path = cras_bt_adapter_object_path(adapter);
	method_call = dbus_message_new_method_call(
		BLUEZ_SERVICE, adapter_path,
		BLUEZ_INTERFACE_BATTERY_PROVIDER_MANAGER,
		"RegisterBatteryProvider");
	if (!method_call)
		return -ENOMEM;

	dbus_message_iter_init_append(method_call, &message_iter);
	dbus_message_iter_append_basic(&message_iter, DBUS_TYPE_OBJECT_PATH,
				       &battery_provider.object_path);

	if (!dbus_connection_send_with_reply(conn, method_call, &pending_call,
					     DBUS_TIMEOUT_USE_DEFAULT)) {
		dbus_message_unref(method_call);
		return -ENOMEM;
	}

	dbus_message_unref(method_call);

	if (!pending_call)
		return -EIO;

	if (!dbus_pending_call_set_notify(
		    pending_call, cras_bt_on_battery_provider_registered,
		    &battery_provider, NULL)) {
		dbus_pending_call_cancel(pending_call);
		dbus_pending_call_unref(pending_call);
		return -ENOMEM;
	}

	return 0;
}

/* Removes a battery object and signals the removal on D-Bus as well. */
static void cleanup_battery(struct cras_bt_battery_provider *provider,
			    struct cras_bt_battery *battery)
{
	DBusMessage *msg;
	DBusMessageIter iter, entry;

	if (!battery)
		return;

	LL_DELETE(provider->batteries, battery);

	msg = dbus_message_new_signal(CRAS_DEFAULT_BATTERY_PROVIDER,
				      DBUS_INTERFACE_OBJECT_MANAGER,
				      DBUS_SIGNAL_INTERFACES_REMOVED);

	dbus_message_iter_init_append(msg, &iter);
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_OBJECT_PATH,
				       &battery->object_path);
	dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY,
					 DBUS_TYPE_STRING_AS_STRING, &entry);
	dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING,
				       &provider->interface);
	dbus_message_iter_close_container(&iter, &entry);

	if (!dbus_connection_send(provider->conn, msg, NULL)) {
		syslog(LOG_ERR, "Error sending " DBUS_SIGNAL_INTERFACES_REMOVED
				" signal");
	}

	dbus_message_unref(msg);

	battery_free(battery);
}

void cras_bt_battery_provider_reset()
{
	struct cras_bt_battery *battery;

	syslog(LOG_INFO, "Resetting battery provider");

	if (!battery_provider.is_registered)
		return;

	battery_provider.is_registered = false;

	LL_FOREACH (battery_provider.batteries, battery) {
		cleanup_battery(&battery_provider, battery);
	}

	if (battery_provider.conn) {
		dbus_connection_unref(battery_provider.conn);
		battery_provider.conn = NULL;
	}

	if (battery_provider.observer) {
		cras_observer_remove(battery_provider.observer);
		battery_provider.observer = NULL;
	}
}
