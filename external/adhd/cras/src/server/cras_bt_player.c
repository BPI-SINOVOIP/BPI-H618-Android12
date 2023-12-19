/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <dbus/dbus.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include "cras_bt_adapter.h"
#include "cras_bt_constants.h"
#include "cras_bt_player.h"
#include "cras_dbus_util.h"
#include "cras_utf8.h"
#include "utlist.h"

static void cras_bt_on_player_registered(DBusPendingCall *pending_call,
					 void *data)
{
	DBusMessage *reply;

	reply = dbus_pending_call_steal_reply(pending_call);
	dbus_pending_call_unref(pending_call);

	if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
		syslog(LOG_ERR, "RegisterPlayer returned error: %s",
		       dbus_message_get_error_name(reply));
		dbus_message_unref(reply);
		return;
	}

	dbus_message_unref(reply);
}

static int cras_bt_add_player(DBusConnection *conn,
			      const struct cras_bt_adapter *adapter,
			      struct cras_bt_player *player)
{
	const char *adapter_path;
	DBusMessage *method_call;
	DBusMessageIter message_iter, dict;
	DBusPendingCall *pending_call;

	adapter_path = cras_bt_adapter_object_path(adapter);
	method_call = dbus_message_new_method_call(BLUEZ_SERVICE, adapter_path,
						   BLUEZ_INTERFACE_MEDIA,
						   "RegisterPlayer");
	if (!method_call)
		return -ENOMEM;

	dbus_message_iter_init_append(method_call, &message_iter);
	dbus_message_iter_append_basic(&message_iter, DBUS_TYPE_OBJECT_PATH,
				       &player->object_path);

	dbus_message_iter_open_container(
		&message_iter, DBUS_TYPE_ARRAY,
		DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING DBUS_TYPE_STRING_AS_STRING
			DBUS_TYPE_VARIANT_AS_STRING
				DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
		&dict);

	append_key_value(&dict, "PlaybackStatus", DBUS_TYPE_STRING,
			 DBUS_TYPE_STRING_AS_STRING, &player->playback_status);
	append_key_value(&dict, "Identity", DBUS_TYPE_STRING,
			 DBUS_TYPE_STRING_AS_STRING, &player->identity);
	append_key_value(&dict, "LoopStatus", DBUS_TYPE_STRING,
			 DBUS_TYPE_STRING_AS_STRING, &player->loop_status);
	append_key_value(&dict, "Position", DBUS_TYPE_INT64,
			 DBUS_TYPE_INT64_AS_STRING, &player->position);
	append_key_value(&dict, "Shuffle", DBUS_TYPE_BOOLEAN,
			 DBUS_TYPE_BOOLEAN_AS_STRING, &player->shuffle);
	append_key_value(&dict, "CanGoNext", DBUS_TYPE_BOOLEAN,
			 DBUS_TYPE_BOOLEAN_AS_STRING, &player->can_go_next);
	append_key_value(&dict, "CanGoPrevious", DBUS_TYPE_BOOLEAN,
			 DBUS_TYPE_BOOLEAN_AS_STRING, &player->can_go_prev);
	append_key_value(&dict, "CanPlay", DBUS_TYPE_BOOLEAN,
			 DBUS_TYPE_BOOLEAN_AS_STRING, &player->can_play);
	append_key_value(&dict, "CanPause", DBUS_TYPE_BOOLEAN,
			 DBUS_TYPE_BOOLEAN_AS_STRING, &player->can_pause);
	append_key_value(&dict, "CanControl", DBUS_TYPE_BOOLEAN,
			 DBUS_TYPE_BOOLEAN_AS_STRING, &player->can_control);

	dbus_message_iter_close_container(&message_iter, &dict);

	if (!dbus_connection_send_with_reply(conn, method_call, &pending_call,
					     DBUS_TIMEOUT_USE_DEFAULT)) {
		dbus_message_unref(method_call);
		return -ENOMEM;
	}

	dbus_message_unref(method_call);
	if (!pending_call)
		return -EIO;

	if (!dbus_pending_call_set_notify(
		    pending_call, cras_bt_on_player_registered, player, NULL)) {
		dbus_pending_call_cancel(pending_call);
		dbus_pending_call_unref(pending_call);
		return -ENOMEM;
	}
	return 0;
}

/* Note that player properties will be used mostly for AVRCP qualification and
 * not for normal use cases. The corresponding media events won't be routed by
 * CRAS until we have a plan to provide general system API to handle media
 * control.
 */
static struct cras_bt_player player = {
	.object_path = CRAS_DEFAULT_PLAYER,
	.playback_status = NULL,
	.identity = NULL,
	.loop_status = "None",
	.shuffle = 0,
	.metadata = NULL,
	.position = 0,
	.can_go_next = 0,
	.can_go_prev = 0,
	.can_play = 0,
	.can_pause = 0,
	.can_control = 0,
	.message_cb = NULL,
};

static DBusHandlerResult cras_bt_player_handle_message(DBusConnection *conn,
						       DBusMessage *message,
						       void *arg)
{
	const char *msg = dbus_message_get_member(message);

	if (player.message_cb)
		player.message_cb(msg);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static struct cras_bt_player_metadata *cras_bt_player_metadata_init()
{
	struct cras_bt_player_metadata *metadata =
		malloc(sizeof(struct cras_bt_player_metadata));
	metadata->title = calloc(1, CRAS_PLAYER_METADATA_SIZE_MAX);
	metadata->album = calloc(1, CRAS_PLAYER_METADATA_SIZE_MAX);
	metadata->artist = calloc(1, CRAS_PLAYER_METADATA_SIZE_MAX);
	metadata->length = 0;

	return metadata;
}

static void cras_bt_player_init()
{
	player.playback_status = malloc(CRAS_PLAYER_PLAYBACK_STATUS_SIZE_MAX);
	player.identity = malloc(CRAS_PLAYER_IDENTITY_SIZE_MAX);

	strcpy(player.playback_status, CRAS_PLAYER_PLAYBACK_STATUS_DEFAULT);
	strcpy(player.identity, CRAS_PLAYER_IDENTITY_DEFAULT);
	player.position = 0;

	player.metadata = cras_bt_player_metadata_init();
}

static void cras_bt_player_append_metadata_artist(DBusMessageIter *iter,
						  const char *artist)
{
	DBusMessageIter dict, varient, array;
	const char *artist_key = "xesam:artist";

	dbus_message_iter_open_container(iter, DBUS_TYPE_DICT_ENTRY, NULL,
					 &dict);
	dbus_message_iter_append_basic(&dict, DBUS_TYPE_STRING, &artist_key);
	dbus_message_iter_open_container(
		&dict, DBUS_TYPE_VARIANT,
		DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_STRING_AS_STRING, &varient);
	dbus_message_iter_open_container(&varient, DBUS_TYPE_ARRAY,
					 DBUS_TYPE_STRING_AS_STRING, &array);
	dbus_message_iter_append_basic(&array, DBUS_TYPE_STRING, &artist);
	dbus_message_iter_close_container(&varient, &array);
	dbus_message_iter_close_container(&dict, &varient);
	dbus_message_iter_close_container(iter, &dict);
}

static void cras_bt_player_append_metadata(DBusMessageIter *iter,
					   const char *title,
					   const char *artist,
					   const char *album,
					   dbus_int64_t length)
{
	DBusMessageIter varient, array;
	dbus_message_iter_open_container(
		iter, DBUS_TYPE_VARIANT,
		DBUS_TYPE_ARRAY_AS_STRING DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
			DBUS_TYPE_STRING_AS_STRING DBUS_TYPE_VARIANT_AS_STRING
				DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
		&varient);
	dbus_message_iter_open_container(
		&varient, DBUS_TYPE_ARRAY,
		DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING DBUS_TYPE_STRING_AS_STRING
			DBUS_TYPE_VARIANT_AS_STRING
				DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
		&array);
	if (!is_utf8_string(title)) {
		syslog(LOG_INFO, "Non-utf8 title: %s", title);
		title = "";
	}
	if (!is_utf8_string(album)) {
		syslog(LOG_INFO, "Non-utf8 album: %s", album);
		album = "";
	}
	if (!is_utf8_string(artist)) {
		syslog(LOG_INFO, "Non-utf8 artist: %s", artist);
		artist = "";
	}

	append_key_value(&array, "xesam:title", DBUS_TYPE_STRING,
			 DBUS_TYPE_STRING_AS_STRING, &title);
	append_key_value(&array, "xesam:album", DBUS_TYPE_STRING,
			 DBUS_TYPE_STRING_AS_STRING, &album);
	append_key_value(&array, "mpris:length", DBUS_TYPE_INT64,
			 DBUS_TYPE_INT64_AS_STRING, &length);
	cras_bt_player_append_metadata_artist(&array, artist);

	dbus_message_iter_close_container(&varient, &array);
	dbus_message_iter_close_container(iter, &varient);
}

static bool cras_bt_player_parse_metadata(const char *title, const char *album,
					  const char *artist,
					  const dbus_int64_t length)
{
	bool require_update = false;

	if (title && strcmp(player.metadata->title, title)) {
		snprintf(player.metadata->title, CRAS_PLAYER_METADATA_SIZE_MAX,
			 "%s", title);
		require_update = true;
	}
	if (artist && strcmp(player.metadata->artist, artist)) {
		snprintf(player.metadata->artist, CRAS_PLAYER_METADATA_SIZE_MAX,
			 "%s", artist);
		require_update = true;
	}
	if (album && strcmp(player.metadata->album, album)) {
		snprintf(player.metadata->album, CRAS_PLAYER_METADATA_SIZE_MAX,
			 "%s", album);
		require_update = true;
	}
	if (length && player.metadata->length != length) {
		player.metadata->length = length;
		require_update = true;
	}

	return require_update;
}

int cras_bt_player_create(DBusConnection *conn)
{
	static const DBusObjectPathVTable player_vtable = {
		.message_function = cras_bt_player_handle_message
	};

	DBusError dbus_error;
	struct cras_bt_adapter **adapters;
	size_t num_adapters, i;

	dbus_error_init(&dbus_error);

	cras_bt_player_init();
	if (!dbus_connection_register_object_path(
		    conn, player.object_path, &player_vtable, &dbus_error)) {
		syslog(LOG_ERR, "Cannot register player %s",
		       player.object_path);
		dbus_error_free(&dbus_error);
		return -ENOMEM;
	}

	num_adapters = cras_bt_adapter_get_list(&adapters);
	for (i = 0; i < num_adapters; ++i)
		cras_bt_add_player(conn, adapters[i], &player);
	free(adapters);
	return 0;
}

int cras_bt_register_player(DBusConnection *conn,
			    const struct cras_bt_adapter *adapter)
{
	return cras_bt_add_player(conn, adapter, &player);
}

int cras_bt_player_update_playback_status(DBusConnection *conn,
					  const char *status)
{
	DBusMessage *msg;
	DBusMessageIter iter, dict;
	const char *playerInterface = BLUEZ_INTERFACE_MEDIA_PLAYER;

	if (!player.playback_status)
		return -ENXIO;

	/* Verify the string value matches one of the possible status defined in
	 * bluez/profiles/audio/avrcp.c
	 */
	if (strcasecmp(status, "stopped") != 0 &&
	    strcasecmp(status, "playing") != 0 &&
	    strcasecmp(status, "paused") != 0 &&
	    strcasecmp(status, "forward-seek") != 0 &&
	    strcasecmp(status, "reverse-seek") != 0 &&
	    strcasecmp(status, "error") != 0)
		return -EINVAL;

	if (!strcasecmp(player.playback_status, status))
		return 0;

	strcpy(player.playback_status, status);

	msg = dbus_message_new_signal(CRAS_DEFAULT_PLAYER,
				      DBUS_INTERFACE_PROPERTIES,
				      "PropertiesChanged");
	if (!msg)
		return -ENOMEM;

	dbus_message_iter_init_append(msg, &iter);
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING,
				       &playerInterface);
	dbus_message_iter_open_container(
		&iter, DBUS_TYPE_ARRAY,
		DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING DBUS_TYPE_STRING_AS_STRING
			DBUS_TYPE_VARIANT_AS_STRING
				DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
		&dict);
	append_key_value(&dict, "PlaybackStatus", DBUS_TYPE_STRING,
			 DBUS_TYPE_STRING_AS_STRING, &status);
	dbus_message_iter_close_container(&iter, &dict);

	if (!dbus_connection_send(conn, msg, NULL)) {
		dbus_message_unref(msg);
		return -ENOMEM;
	}

	dbus_message_unref(msg);
	return 0;
}

int cras_bt_player_update_identity(DBusConnection *conn, const char *identity)
{
	DBusMessage *msg;
	DBusMessageIter iter, dict;
	const char *playerInterface = BLUEZ_INTERFACE_MEDIA_PLAYER;

	if (!player.identity)
		return -ENXIO;

	if (!identity)
		return -EINVAL;

	if (!is_utf8_string(identity)) {
		syslog(LOG_INFO, "Non-utf8 identity: %s", identity);
		identity = "";
	}

	if (!strcasecmp(player.identity, identity))
		return 0;

	strcpy(player.identity, identity);

	msg = dbus_message_new_signal(CRAS_DEFAULT_PLAYER,
				      DBUS_INTERFACE_PROPERTIES,
				      "PropertiesChanged");
	if (!msg)
		return -ENOMEM;

	dbus_message_iter_init_append(msg, &iter);
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING,
				       &playerInterface);
	dbus_message_iter_open_container(
		&iter, DBUS_TYPE_ARRAY,
		DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING DBUS_TYPE_STRING_AS_STRING
			DBUS_TYPE_VARIANT_AS_STRING
				DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
		&dict);
	append_key_value(&dict, "Identity", DBUS_TYPE_STRING,
			 DBUS_TYPE_STRING_AS_STRING, &identity);
	dbus_message_iter_close_container(&iter, &dict);

	if (!dbus_connection_send(conn, msg, NULL)) {
		dbus_message_unref(msg);
		return -ENOMEM;
	}

	dbus_message_unref(msg);
	return 0;
}

int cras_bt_player_update_position(DBusConnection *conn,
				   const dbus_int64_t position)
{
	DBusMessage *msg;
	DBusMessageIter iter, dict;
	const char *playerInterface = BLUEZ_INTERFACE_MEDIA_PLAYER;

	if (position < 0)
		return -EINVAL;

	player.position = position;

	msg = dbus_message_new_signal(CRAS_DEFAULT_PLAYER,
				      DBUS_INTERFACE_PROPERTIES,
				      "PropertiesChanged");
	if (!msg)
		return -ENOMEM;

	dbus_message_iter_init_append(msg, &iter);
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING,
				       &playerInterface);
	dbus_message_iter_open_container(
		&iter, DBUS_TYPE_ARRAY,
		DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING DBUS_TYPE_STRING_AS_STRING
			DBUS_TYPE_VARIANT_AS_STRING
				DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
		&dict);
	append_key_value(&dict, "Position", DBUS_TYPE_INT64,
			 DBUS_TYPE_INT64_AS_STRING, &player.position);
	dbus_message_iter_close_container(&iter, &dict);

	if (!dbus_connection_send(conn, msg, NULL)) {
		dbus_message_unref(msg);
		return -ENOMEM;
	}

	dbus_message_unref(msg);
	return 0;
}

int cras_bt_player_update_metadata(DBusConnection *conn, const char *title,
				   const char *artist, const char *album,
				   const dbus_int64_t length)
{
	DBusMessage *msg;
	DBusMessageIter iter, array, dict;
	const char *property = "Metadata";
	const char *playerInterface = BLUEZ_INTERFACE_MEDIA_PLAYER;

	if (!player.metadata)
		return -ENXIO;

	msg = dbus_message_new_signal(CRAS_DEFAULT_PLAYER,
				      DBUS_INTERFACE_PROPERTIES,
				      "PropertiesChanged");
	if (!msg)
		return -ENOMEM;

	dbus_message_iter_init_append(msg, &iter);
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING,
				       &playerInterface);
	dbus_message_iter_open_container(
		&iter, DBUS_TYPE_ARRAY,
		DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING DBUS_TYPE_STRING_AS_STRING
			DBUS_TYPE_VARIANT_AS_STRING
				DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
		&array);
	dbus_message_iter_open_container(&array, DBUS_TYPE_DICT_ENTRY, NULL,
					 &dict);
	dbus_message_iter_append_basic(&dict, DBUS_TYPE_STRING, &property);

	if (!cras_bt_player_parse_metadata(title, album, artist, length)) {
		/* Nothing to update. */
		dbus_message_unref(msg);
		return 0;
	}

	cras_bt_player_append_metadata(&dict, player.metadata->title,
				       player.metadata->artist,
				       player.metadata->album,
				       player.metadata->length);

	dbus_message_iter_close_container(&array, &dict);
	dbus_message_iter_close_container(&iter, &array);

	if (!dbus_connection_send(conn, msg, NULL)) {
		dbus_message_unref(msg);
		return -ENOMEM;
	}

	dbus_message_unref(msg);
	return 0;
}
