/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <fcntl.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <syslog.h>

#include "cras_audio_area.h"
#include "cras_config.h"
#include "cras_messages.h"
#include "cras_rclient.h"
#include "cras_rstream.h"
#include "cras_rstream_config.h"
#include "cras_server_metrics.h"
#include "cras_shm.h"
#include "cras_types.h"
#include "cras_system_state.h"

void cras_rstream_config_init(
	struct cras_rclient *client, cras_stream_id_t stream_id,
	enum CRAS_STREAM_TYPE stream_type, enum CRAS_CLIENT_TYPE client_type,
	enum CRAS_STREAM_DIRECTION direction, uint32_t dev_idx, uint32_t flags,
	uint32_t effects, const struct cras_audio_format *format,
	size_t buffer_frames, size_t cb_threshold, int *audio_fd,
	int *client_shm_fd, size_t client_shm_size,
	const uint64_t buffer_offsets[2],
	struct cras_rstream_config *stream_config)
{
	stream_config->stream_id = stream_id;
	stream_config->stream_type = stream_type;
	stream_config->client_type = client_type;
	stream_config->direction = direction;
	stream_config->dev_idx = dev_idx;
	stream_config->flags = flags;
	stream_config->effects = effects;
	stream_config->format = format;
	stream_config->buffer_frames = buffer_frames;
	stream_config->cb_threshold = cb_threshold;
	stream_config->audio_fd = *audio_fd;
	*audio_fd = -1;
	stream_config->client_shm_fd = *client_shm_fd;
	*client_shm_fd = -1;
	stream_config->client_shm_size = client_shm_size;
	stream_config->buffer_offsets[0] = buffer_offsets[0];
	stream_config->buffer_offsets[1] = buffer_offsets[1];
	stream_config->client = client;
}

struct cras_rstream_config cras_rstream_config_init_with_message(
	struct cras_rclient *client, const struct cras_connect_message *msg,
	int *aud_fd, int *client_shm_fd,
	const struct cras_audio_format *remote_fmt)
{
	struct cras_rstream_config stream_config;

	const uint64_t buffer_offsets[2] = { msg->buffer_offsets[0],
					     msg->buffer_offsets[1] };
	cras_rstream_config_init(client, msg->stream_id, msg->stream_type,
				 msg->client_type, msg->direction, msg->dev_idx,
				 msg->flags, msg->effects, remote_fmt,
				 msg->buffer_frames, msg->cb_threshold, aud_fd,
				 client_shm_fd, msg->client_shm_size,
				 buffer_offsets, &stream_config);
	return stream_config;
}

void cras_rstream_config_cleanup(struct cras_rstream_config *stream_config)
{
	if (stream_config->audio_fd >= 0)
		close(stream_config->audio_fd);
	if (stream_config->client_shm_fd >= 0)
		close(stream_config->client_shm_fd);
}
