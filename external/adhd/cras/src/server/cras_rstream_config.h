/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Remote Stream Configuration
 */
#ifndef CRAS_RSTREAM_CONFIG_H_
#define CRAS_RSTREAM_CONFIG_H_

#include "buffer_share.h"
#include "cras_shm.h"
#include "cras_types.h"

struct cras_connect_message;
struct dev_mix;

/* Config for creating an rstream.
 *    stream_type - CRAS_STREAM_TYPE.
 *    client_type - CRAS_CLIENT_TYPE.
 *    direction - CRAS_STREAM_OUTPUT or CRAS_STREAM_INPUT.
 *    dev_idx - Pin to this device if != NO_DEVICE.
 *    flags - Any special handling for this stream.
 *    effects - Bit map of effects to be enabled on this stream.
 *    format - The audio format the stream wishes to use.
 *    buffer_frames - Total number of audio frames to buffer.
 *    cb_threshold - # of frames when to request more from the client.
 *    audio_fd - The fd to read/write audio signals to. May be -1 for server
 *               stream. Some functions may mutably borrow the config and move
 *               the fd ownership.
 *    client_shm_fd - The shm fd to use to back the samples area. May be -1.
 *                    Some functions may dup this fd while borrowing the config.
 *    client_shm_size - The size of shm area backed by client_shm_fd.
 *    buffer_offsets - Initial values for buffer_offset for a client shm stream.
 *    client - The client that owns this stream.
 */
struct cras_rstream_config {
	cras_stream_id_t stream_id;
	enum CRAS_STREAM_TYPE stream_type;
	enum CRAS_CLIENT_TYPE client_type;
	enum CRAS_STREAM_DIRECTION direction;
	uint32_t dev_idx;
	uint32_t flags;
	uint32_t effects;
	const struct cras_audio_format *format;
	size_t buffer_frames;
	size_t cb_threshold;
	int audio_fd;
	int client_shm_fd;
	size_t client_shm_size;
	uint32_t buffer_offsets[2];
	struct cras_rclient *client;
};

/* Fills cras_rstream_config with given parameters.
 *
 * Args:
 *   audio_fd - The audio fd pointer from client. Its ownership will be moved to
 *              stream_config.
 *   client_shm_fd - The shared memory fd pointer for samples from client. Its
 *                   ownership will be moved to stream_config.
 *   Other args - See comments in struct cras_rstream_config.
 */
void cras_rstream_config_init(
	struct cras_rclient *client, cras_stream_id_t stream_id,
	enum CRAS_STREAM_TYPE stream_type, enum CRAS_CLIENT_TYPE client_type,
	enum CRAS_STREAM_DIRECTION direction, uint32_t dev_idx, uint32_t flags,
	uint32_t effects, const struct cras_audio_format *format,
	size_t buffer_frames, size_t cb_threshold, int *audio_fd,
	int *client_shm_fd, size_t client_shm_size,
	const uint64_t buffer_offsets[2],
	struct cras_rstream_config *stream_config);

/* Fills cras_rstream_config with given parameters and a cras_connect_message.
 *
 * Args:
 *   client - The rclient which handles the connect message.
 *   msg - The cras_connect_message from client.
 *   aud_fd - The audio fd pointer from client. Its ownership will be moved to
 *            stream_config.
 *   client_shm_fd - The shared memory fd pointer for samples from client. Its
 *                   ownership will be moved to stream_config.
 *   remote_format - The remote_format for the config.
 *
 * Returns a cras_rstream_config struct filled in with params from the message.
 */
struct cras_rstream_config cras_rstream_config_init_with_message(
	struct cras_rclient *client, const struct cras_connect_message *msg,
	int *aud_fd, int *client_shm_fd,
	const struct cras_audio_format *remote_format);

/* Cleans up given cras_rstream_config. All fds inside the config will be
 * closed.
 *
 * Args:
 *   stream_config - The config to be cleaned up.
 */
void cras_rstream_config_cleanup(struct cras_rstream_config *stream_config);

#endif /* CRAS_RSTREAM_CONFIG_H_ */
