/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_BT_BATTERY_PROVIDER_H_
#define CRAS_BT_BATTERY_PROVIDER_H_

#include <dbus/dbus.h>
#include <stdbool.h>

#include "cras_bt_adapter.h"

/* Object to represent a battery that is exposed to BlueZ. */
struct cras_bt_battery {
	char *address;
	char *object_path;
	char *device_path;
	uint32_t level;
	struct cras_bt_battery *next;
};

/* Object to register as battery provider so that bluetoothd will monitor
 * battery objects that we expose.
 */
struct cras_bt_battery_provider {
	const char *object_path;
	const char *interface;
	DBusConnection *conn;
	bool is_registered;
	struct cras_observer_client *observer;
	struct cras_bt_battery *batteries;
};

/* Registers battery provider to bluetoothd. This is used when a Bluetooth
 * adapter got enumerated.
 * Args:
 *    conn - The D-Bus connection.
 *    adapter - The enumerated bluetooth adapter.
 */
int cras_bt_register_battery_provider(DBusConnection *conn,
				      const struct cras_bt_adapter *adapter);

/* Resets internal state of battery provider. */
void cras_bt_battery_provider_reset();

#endif /* CRAS_BT_BATTERY_PROVIDER_H_ */
