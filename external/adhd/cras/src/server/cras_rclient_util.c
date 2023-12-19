/* Copyright 2019 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <syslog.h>

#include "cras_iodev_list.h"
#include "cras_messages.h"
#include "cras_observer.h"
#include "cras_rclient.h"
#include "cras_rclient_util.h"
#include "cras_rstream.h"
#include "cras_server_metrics.h"
#include "cras_system_state.h"
#include "cras_tm.h"
#include "cras_types.h"
#include "cras_util.h"
#include "stream_list.h"

int rclient_send_message_to_client(const struct cras_rclient *client,
				   const struct cras_client_message *msg,
				   int *fds, unsigned int num_fds)
{
	return cras_send_with_fds(client->fd, (const void *)msg, msg->length,
				  fds, num_fds);
}

void rclient_destroy(struct cras_rclient *client)
{
	cras_observer_remove(client->observer);
	stream_list_rm_all_client_streams(cras_iodev_list_get_stream_list(),
					  client);
	free(client);
}

int rclient_validate_message_fds(const struct cras_server_message *msg,
				 int *fds, unsigned int num_fds)
{
	switch (msg->id) {
	case CRAS_SERVER_CONNECT_STREAM:
		if (num_fds > 2)
			goto error;
		break;
	case CRAS_SERVER_SET_AEC_DUMP:
		if (num_fds > 1)
			goto error;
		break;
	default:
		if (num_fds > 0)
			goto error;
		break;
	}

	return 0;

error:
	syslog(LOG_ERR, "Message %d should not have %u fds attached.", msg->id,
	       num_fds);
	return -EINVAL;
}

static int
rclient_validate_stream_connect_message(const struct cras_rclient *client,
					const struct cras_connect_message *msg)
{
	if (!cras_valid_stream_id(msg->stream_id, client->id)) {
		syslog(LOG_ERR,
		       "stream_connect: invalid stream_id: %x for "
		       "client: %zx.\n",
		       msg->stream_id, client->id);
		return -EINVAL;
	}

	int direction = cras_stream_direction_mask(msg->direction);
	if (direction < 0 || !(client->supported_directions & direction)) {
		syslog(LOG_ERR,
		       "stream_connect: invalid stream direction: %x for "
		       "client: %zx.\n",
		       msg->direction, client->id);
		return -EINVAL;
	}

	if (!cras_validate_client_type(msg->client_type)) {
		syslog(LOG_ERR,
		       "stream_connect: invalid stream client_type: %x for "
		       "client: %zx.\n",
		       msg->client_type, client->id);
	}
	return 0;
}

static int rclient_validate_stream_connect_fds(int audio_fd, int client_shm_fd,
					       size_t client_shm_size)
{
	/* check audio_fd is valid. */
	if (audio_fd < 0) {
		syslog(LOG_ERR, "Invalid audio fd in stream connect.\n");
		return -EBADF;
	}

	/* check client_shm_fd is valid if client wants to use client shm. */
	if (client_shm_size > 0 && client_shm_fd < 0) {
		syslog(LOG_ERR,
		       "client_shm_fd must be valid if client_shm_size > 0.\n");
		return -EBADF;
	} else if (client_shm_size == 0 && client_shm_fd >= 0) {
		syslog(LOG_ERR,
		       "client_shm_fd can be valid only if client_shm_size > 0.\n");
		return -EINVAL;
	}
	return 0;
}

int rclient_validate_stream_connect_params(
	const struct cras_rclient *client,
	const struct cras_connect_message *msg, int audio_fd, int client_shm_fd)
{
	int rc;

	rc = rclient_validate_stream_connect_message(client, msg);
	if (rc)
		return rc;

	rc = rclient_validate_stream_connect_fds(audio_fd, client_shm_fd,
						 msg->client_shm_size);
	if (rc)
		return rc;

	return 0;
}

int rclient_handle_client_stream_connect(struct cras_rclient *client,
					 const struct cras_connect_message *msg,
					 int aud_fd, int client_shm_fd)
{
	struct cras_rstream *stream;
	struct cras_client_stream_connected stream_connected;
	struct cras_client_message *reply;
	struct cras_audio_format remote_fmt;
	struct cras_rstream_config stream_config;
	int rc, header_fd, samples_fd;
	size_t samples_size;
	int stream_fds[2];

	rc = rclient_validate_stream_connect_params(client, msg, aud_fd,
						    client_shm_fd);
	remote_fmt = unpack_cras_audio_format(&msg->format);
	if (rc == 0 && !cras_audio_format_valid(&remote_fmt)) {
		rc = -EINVAL;
	}
	if (rc) {
		if (client_shm_fd >= 0)
			close(client_shm_fd);
		if (aud_fd >= 0)
			close(aud_fd);
		goto reply_err;
	}

	/* When full, getting an error is preferable to blocking. */
	cras_make_fd_nonblocking(aud_fd);

	stream_config = cras_rstream_config_init_with_message(
		client, msg, &aud_fd, &client_shm_fd, &remote_fmt);
	/* Overwrite client_type if client->client_type is set. */
	if (client->client_type != CRAS_CLIENT_TYPE_UNKNOWN)
		stream_config.client_type = client->client_type;
	rc = stream_list_add(cras_iodev_list_get_stream_list(), &stream_config,
			     &stream);
	if (rc)
		goto cleanup_config;

	detect_rtc_stream_pair(cras_iodev_list_get_stream_list(), stream);

	/* Tell client about the stream setup. */
	syslog(LOG_DEBUG, "Send connected for stream %x\n", msg->stream_id);

	// Check that shm size is at most UINT32_MAX for non-shm streams.
	samples_size = cras_rstream_get_samples_shm_size(stream);
	if (samples_size > UINT32_MAX && stream_config.client_shm_fd < 0) {
		syslog(LOG_ERR,
		       "Non client-provided shm stream has samples shm larger "
		       "than uint32_t: %zu",
		       samples_size);
		if (aud_fd >= 0)
			close(aud_fd);
		rc = -EINVAL;
		goto cleanup_config;
	}
	cras_fill_client_stream_connected(&stream_connected, 0, /* No error. */
					  msg->stream_id, &remote_fmt,
					  samples_size,
					  cras_rstream_get_effects(stream));
	reply = &stream_connected.header;

	rc = cras_rstream_get_shm_fds(stream, &header_fd, &samples_fd);
	if (rc)
		goto cleanup_config;

	stream_fds[0] = header_fd;
	/* If we're using client-provided shm, samples_fd here refers to the
	 * same shm area as client_shm_fd */
	stream_fds[1] = samples_fd;

	rc = client->ops->send_message_to_client(client, reply, stream_fds, 2);
	if (rc < 0) {
		syslog(LOG_ERR, "Failed to send connected messaged\n");
		stream_list_rm(cras_iodev_list_get_stream_list(),
			       stream->stream_id);
		goto cleanup_config;
	}

	/* Cleanup local object explicitly. */
	cras_rstream_config_cleanup(&stream_config);
	return 0;

cleanup_config:
	cras_rstream_config_cleanup(&stream_config);

reply_err:
	/* Send the error code to the client. */
	cras_fill_client_stream_connected(&stream_connected, rc, msg->stream_id,
					  &remote_fmt, 0, msg->effects);
	reply = &stream_connected.header;
	client->ops->send_message_to_client(client, reply, NULL, 0);

	return rc;
}

/* Handles messages from the client requesting that a stream be removed from the
 * server. */
int rclient_handle_client_stream_disconnect(
	struct cras_rclient *client,
	const struct cras_disconnect_stream_message *msg)
{
	if (!cras_valid_stream_id(msg->stream_id, client->id)) {
		syslog(LOG_ERR,
		       "stream_disconnect: invalid stream_id: %x for "
		       "client: %zx.\n",
		       msg->stream_id, client->id);
		return -EINVAL;
	}
	return stream_list_rm(cras_iodev_list_get_stream_list(),
			      msg->stream_id);
}

/* Creates a client structure and sends a message back informing the client that
 * the connection has succeeded. */
struct cras_rclient *rclient_generic_create(int fd, size_t id,
					    const struct cras_rclient_ops *ops,
					    int supported_directions)
{
	struct cras_rclient *client;
	struct cras_client_connected msg;
	int state_fd;

	client = (struct cras_rclient *)calloc(1, sizeof(struct cras_rclient));
	if (!client)
		return NULL;

	client->fd = fd;
	client->id = id;
	client->ops = ops;
	client->supported_directions = supported_directions;

	cras_fill_client_connected(&msg, client->id);
	state_fd = cras_sys_state_shm_fd();
	client->ops->send_message_to_client(client, &msg.header, &state_fd, 1);

	return client;
}

/* A generic entry point for handling a message from the client. Called from
 * the main server context. */
int rclient_handle_message_from_client(struct cras_rclient *client,
				       const struct cras_server_message *msg,
				       int *fds, unsigned int num_fds)
{
	int rc = 0;
	assert(client && msg);

	rc = rclient_validate_message_fds(msg, fds, num_fds);
	if (rc < 0) {
		for (int i = 0; i < (int)num_fds; i++)
			if (fds[i] >= 0)
				close(fds[i]);
		return rc;
	}
	int fd = num_fds > 0 ? fds[0] : -1;

	switch (msg->id) {
	case CRAS_SERVER_CONNECT_STREAM: {
		int client_shm_fd = num_fds > 1 ? fds[1] : -1;
		if (MSG_LEN_VALID(msg, struct cras_connect_message)) {
			rclient_handle_client_stream_connect(
				client,
				(const struct cras_connect_message *)msg, fd,
				client_shm_fd);
		} else {
			return -EINVAL;
		}
		break;
	}
	case CRAS_SERVER_DISCONNECT_STREAM:
		if (!MSG_LEN_VALID(msg, struct cras_disconnect_stream_message))
			return -EINVAL;
		rclient_handle_client_stream_disconnect(
			client,
			(const struct cras_disconnect_stream_message *)msg);
		break;
	default:
		break;
	}

	return rc;
}
