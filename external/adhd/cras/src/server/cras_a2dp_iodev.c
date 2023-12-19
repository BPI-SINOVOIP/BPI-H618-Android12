/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <linux/sockios.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <syslog.h>
#include <time.h>

#include "audio_thread.h"
#include "audio_thread_log.h"
#include "byte_buffer.h"
#include "cras_iodev_list.h"
#include "cras_a2dp_endpoint.h"
#include "cras_a2dp_info.h"
#include "cras_a2dp_iodev.h"
#include "cras_audio_area.h"
#include "cras_audio_thread_monitor.h"
#include "cras_bt_device.h"
#include "cras_iodev.h"
#include "cras_util.h"
#include "rtp.h"
#include "utlist.h"

#define PCM_BUF_MAX_SIZE_FRAMES (4096 * 4)
#define PCM_BUF_MAX_SIZE_BYTES (PCM_BUF_MAX_SIZE_FRAMES * 4)

/* Threshold for reasonable a2dp throttle log in audio dump. */
static const struct timespec throttle_log_threshold = {
	0, 20000000 /* 20ms */
};

/* Threshold for severe a2dp throttle event. */
static const struct timespec throttle_event_threshold = {
	2, 0 /* 2s */
};

/* Child of cras_iodev to handle bluetooth A2DP streaming.
 * Members:
 *    base - The cras_iodev structure "base class"
 *    a2dp - The codec and encoded state of a2dp_io.
 *    transport - The transport object for bluez media API.
 *    sock_depth_frames - Socket depth in frames of the a2dp socket.
 *    pcm_buf - Buffer to hold pcm samples before encode.
 *    destroyed - Flag to note if this a2dp_io is about to destroy.
 *    next_flush_time - The time when it is okay for next flush call.
 *    flush_period - The time period between two a2dp packet writes.
 *    write_block - How many frames of audio samples are transferred in one
 *        a2dp packet write.
 */
struct a2dp_io {
	struct cras_iodev base;
	struct a2dp_info a2dp;
	struct cras_bt_transport *transport;
	unsigned sock_depth_frames;
	struct byte_buffer *pcm_buf;
	int destroyed;
	struct timespec next_flush_time;
	struct timespec flush_period;
	unsigned int write_block;
};

static int encode_and_flush(const struct cras_iodev *iodev);

static int update_supported_formats(struct cras_iodev *iodev)
{
	struct a2dp_io *a2dpio = (struct a2dp_io *)iodev;
	size_t rate = 0;
	size_t channel;
	a2dp_sbc_t a2dp;

	cras_bt_transport_configuration(a2dpio->transport, &a2dp, sizeof(a2dp));

	channel = (a2dp.channel_mode == SBC_CHANNEL_MODE_MONO) ? 1 : 2;

	if (a2dp.frequency & SBC_SAMPLING_FREQ_48000)
		rate = 48000;
	else if (a2dp.frequency & SBC_SAMPLING_FREQ_44100)
		rate = 44100;
	else if (a2dp.frequency & SBC_SAMPLING_FREQ_32000)
		rate = 32000;
	else if (a2dp.frequency & SBC_SAMPLING_FREQ_16000)
		rate = 16000;

	free(iodev->supported_rates);
	iodev->supported_rates = (size_t *)malloc(2 * sizeof(rate));
	iodev->supported_rates[0] = rate;
	iodev->supported_rates[1] = 0;

	free(iodev->supported_channel_counts);
	iodev->supported_channel_counts = (size_t *)malloc(2 * sizeof(channel));
	iodev->supported_channel_counts[0] = channel;
	iodev->supported_channel_counts[1] = 0;

	free(iodev->supported_formats);
	iodev->supported_formats =
		(snd_pcm_format_t *)malloc(2 * sizeof(snd_pcm_format_t));
	iodev->supported_formats[0] = SND_PCM_FORMAT_S16_LE;
	iodev->supported_formats[1] = (snd_pcm_format_t)0;

	return 0;
}

static unsigned int bt_local_queued_frames(const struct cras_iodev *iodev)
{
	struct a2dp_io *a2dpio = (struct a2dp_io *)iodev;
	return a2dp_queued_frames(&a2dpio->a2dp) +
	       buf_queued(a2dpio->pcm_buf) /
		       cras_get_format_bytes(iodev->format);
}

static int frames_queued(const struct cras_iodev *iodev,
			 struct timespec *tstamp)
{
	int local_queued_frames = bt_local_queued_frames(iodev);
	clock_gettime(CLOCK_MONOTONIC_RAW, tstamp);
	return MIN(iodev->buffer_size, local_queued_frames);
}

/*
 * Utility function to fill zero frames until buffer level reaches
 * target_level. This is useful to allocate just enough data to write
 * to controller, while not introducing extra latency.
 */
static int fill_zeros_to_target_level(struct cras_iodev *iodev,
				      unsigned int target_level)
{
	unsigned int local_queued_frames = bt_local_queued_frames(iodev);

	if (local_queued_frames < target_level)
		return cras_iodev_fill_odev_zeros(
			iodev, target_level - local_queued_frames);
	return 0;
}

/*
 * dev_io_playback_write() has the logic to detect underrun scenario
 * and calls into this underrun ops, by comparing buffer level with
 * number of frames just written. Note that it's not correct 100% of
 * the time in a2dp case, because we lose track of samples once they're
 * flushed to socket.
 */
static int output_underrun(struct cras_iodev *iodev)
{
	unsigned int local_queued_frames = bt_local_queued_frames(iodev);

	/*
	 * Examples to help understand the check:
	 *
	 * [False-positive underrun]
	 * Assume min_buffer_level = 1000, written 900, and flushes
	 * 800 of data. Audio thread sees 1000 + 900 - 800 = 1100 of
	 * data left. This is merely 100(< 900) above min_buffer_level
	 * so audio_thread thinks it underruns, but actually not.
	 *
	 * [True underrun]
	 * min_buffer_level = 1000, written 200, and flushes 800 of
	 * data. Now that buffer runs lower than min_buffer_level so
	 * it's indeed an underrun.
	 */
	if (local_queued_frames > iodev->min_buffer_level)
		return 0;

	return cras_iodev_fill_odev_zeros(iodev, iodev->min_cb_level);
}

/*
 * This will be called multiple times when a2dpio is in no_stream state
 * frames_to_play_in_sleep ops determins how regular this will be called.
 */
static int enter_no_stream(struct a2dp_io *a2dpio)
{
	struct cras_iodev *odev = &a2dpio->base;
	int rc;
	/*
         * Setting target level to 3 times of min_buffer_level.
         * We want hw_level to stay bewteen 1-2 times of min_buffer_level on
	 * top of the underrun threshold(i.e one min_cb_level).
         */
	rc = fill_zeros_to_target_level(odev, 3 * odev->min_buffer_level);
	if (rc)
		syslog(LOG_ERR, "Error in A2DP enter_no_stream");
	return encode_and_flush(odev);
}

/*
 * This is called when stream data is available to write. Prepare audio
 * data to one min_buffer_level. Don't flush it now because stream data is
 * coming right up which will trigger next flush at appropriate time.
 */
static int leave_no_stream(struct a2dp_io *a2dpio)
{
	struct cras_iodev *odev = &a2dpio->base;

	/*
	 * Since stream data is ready, just make sure hw_level doesn't underrun
	 * after one flush. Hence setting the target level to 2 times of
	 * min_buffer_level.
         */
	return fill_zeros_to_target_level(odev, 2 * odev->min_buffer_level);
}

/*
 * Makes sure there's enough data(zero frames) to flush when no stream presents.
 * Note that the underrun condition is when real buffer level goes below
 * min_buffer_level, so we want to keep data at a reasonable higher level on top
 * of that.
 */
static int no_stream(struct cras_iodev *odev, int enable)
{
	struct a2dp_io *a2dpio = (struct a2dp_io *)odev;

	if (enable)
		return enter_no_stream(a2dpio);
	else
		return leave_no_stream(a2dpio);
}

/* Encode as much PCM data as we can until the buffer level of a2dp_info
 * reaches MTU.
 * Returns:
 *    0 for success, otherwise negative error code.
 */
static int encode_a2dp_packet(struct a2dp_io *a2dpio)
{
	int processed;
	size_t format_bytes = cras_get_format_bytes(a2dpio->base.format);

	while (buf_queued(a2dpio->pcm_buf)) {
		processed = a2dp_encode(
			&a2dpio->a2dp, buf_read_pointer(a2dpio->pcm_buf),
			buf_readable(a2dpio->pcm_buf), format_bytes,
			cras_bt_transport_write_mtu(a2dpio->transport));
		if (processed == -ENOSPC || processed == 0)
			break;
		if (processed < 0)
			return processed;

		buf_increment_read(a2dpio->pcm_buf, processed);
	}
	return 0;
}

/*
 * To be called when a2dp socket becomes writable.
 */
static int a2dp_socket_write_cb(void *arg, int revent)
{
	struct cras_iodev *iodev = (struct cras_iodev *)arg;
	return encode_and_flush(iodev);
}

static int configure_dev(struct cras_iodev *iodev)
{
	struct a2dp_io *a2dpio = (struct a2dp_io *)iodev;
	int sock_depth;
	int err;
	socklen_t optlen;
	int a2dp_payload_length;

	err = cras_bt_transport_acquire(a2dpio->transport);
	if (err < 0) {
		syslog(LOG_ERR, "transport_acquire failed");
		return err;
	}

	/* Apply the node's volume after transport is acquired. Doing this
	 * is necessary because the volume can not sync to hardware until
	 * it is opened. */
	iodev->set_volume(iodev);

	/* Assert format is set before opening device. */
	if (iodev->format == NULL)
		return -EINVAL;
	iodev->format->format = SND_PCM_FORMAT_S16_LE;
	cras_iodev_init_audio_area(iodev, iodev->format->num_channels);

	a2dpio->pcm_buf = byte_buffer_create(PCM_BUF_MAX_SIZE_BYTES);
	if (!a2dpio->pcm_buf)
		return -ENOMEM;

	/* Set up the socket to hold two MTUs full of data before returning
	 * EAGAIN.  This will allow the write to be throttled when a reasonable
	 * amount of data is queued. */
	sock_depth = 2 * cras_bt_transport_write_mtu(a2dpio->transport);
	setsockopt(cras_bt_transport_fd(a2dpio->transport), SOL_SOCKET,
		   SO_SNDBUF, &sock_depth, sizeof(sock_depth));
	optlen = sizeof(sock_depth);
	getsockopt(cras_bt_transport_fd(a2dpio->transport), SOL_SOCKET,
		   SO_SNDBUF, &sock_depth, &optlen);
	a2dpio->sock_depth_frames = a2dp_block_size(&a2dpio->a2dp, sock_depth) /
				    cras_get_format_bytes(iodev->format);
	/*
	 * Per avdtp_write, subtract the room for packet header first.
	 * Calculate how many frames are encapsulated in one a2dp packet, and
	 * the corresponding time period between two packets.
	 */
	a2dp_payload_length = cras_bt_transport_write_mtu(a2dpio->transport) -
			      sizeof(struct rtp_header) -
			      sizeof(struct rtp_payload);
	a2dpio->write_block =
		a2dp_block_size(&a2dpio->a2dp, a2dp_payload_length) /
		cras_get_format_bytes(iodev->format);
	cras_frames_to_time(a2dpio->write_block, iodev->format->frame_rate,
			    &a2dpio->flush_period);

	/* PCM buffer size plus one encoded a2dp packet. */
	iodev->buffer_size = PCM_BUF_MAX_SIZE_FRAMES + a2dpio->write_block;

	/*
	 * Buffer level less than one write_block can't be send over a2dp
	 * packet. Configure min_buffer_level to this value so when stream
	 * underruns, audio thread can take action to fill some zeros.
	 */
	iodev->min_buffer_level = a2dpio->write_block;

	audio_thread_add_events_callback(
		cras_bt_transport_fd(a2dpio->transport), a2dp_socket_write_cb,
		iodev, POLLOUT | POLLERR | POLLHUP);
	audio_thread_config_events_callback(
		cras_bt_transport_fd(a2dpio->transport), TRIGGER_NONE);
	return 0;
}

static int start(const struct cras_iodev *iodev)
{
	struct a2dp_io *a2dpio = (struct a2dp_io *)iodev;

	/*
	 * This is called when iodev in open state, at the moment when
	 * output sample is ready. Initialize the next_flush_time for
	 * following flush calls.
	 */
	clock_gettime(CLOCK_MONOTONIC_RAW, &a2dpio->next_flush_time);

	return 0;
}

static int close_dev(struct cras_iodev *iodev)
{
	int err;
	struct a2dp_io *a2dpio = (struct a2dp_io *)iodev;
	struct cras_bt_device *device;

	if (!a2dpio->transport)
		return 0;

	/* Remove audio thread callback and sync before releasing
	 * the transport. */
	audio_thread_rm_callback_sync(cras_iodev_list_get_audio_thread(),
				      cras_bt_transport_fd(a2dpio->transport));

	err = cras_bt_transport_release(a2dpio->transport, !a2dpio->destroyed);
	if (err < 0)
		syslog(LOG_ERR, "transport_release failed");

	device = cras_bt_transport_device(a2dpio->transport);
	if (device)
		cras_bt_device_cancel_suspend(device);
	a2dp_reset(&a2dpio->a2dp);
	byte_buffer_destroy(&a2dpio->pcm_buf);
	cras_iodev_free_format(iodev);
	cras_iodev_free_audio_area(iodev);
	return 0;
}

static unsigned int frames_to_play_in_sleep(struct cras_iodev *iodev,
					    unsigned int *hw_level,
					    struct timespec *hw_tstamp)
{
	struct a2dp_io *a2dpio = (struct a2dp_io *)iodev;
	int frames_until;

	*hw_level = frames_queued(iodev, hw_tstamp);
	if (*hw_level < a2dpio->write_block)
		*hw_level = 0;
	else
		*hw_level -= a2dpio->write_block;

	frames_until = cras_frames_until_time(&a2dpio->next_flush_time,
					      iodev->format->frame_rate);
	if (frames_until > 0)
		return frames_until;

	/* If time has passed next_flush_time, for example when socket write
	 * throttles, sleep a moderate of time so that audio thread doesn't
	 * busy wake up. */
	return a2dpio->write_block;
}

/* Encodes PCM data to a2dp frames and try to flush it to the socket.
 * Returns:
 *    0 when the flush succeeded, -1 when error occurred.
 */
static int encode_and_flush(const struct cras_iodev *iodev)
{
	int err;
	size_t format_bytes;
	int written = 0;
	unsigned int queued_frames;
	struct a2dp_io *a2dpio;
	struct cras_bt_device *device;
	struct timespec now, ts;
	static const struct timespec flush_wake_fuzz_ts = {
		0, 1000000 /* 1ms */
	};

	a2dpio = (struct a2dp_io *)iodev;
	format_bytes = cras_get_format_bytes(iodev->format);
	device = cras_bt_transport_device(a2dpio->transport);

	/* If bt device has been destroyed, this a2dp iodev will soon be
	 * destroyed as well. */
	if (device == NULL)
		return -EINVAL;

	ATLOG(atlog, AUDIO_THREAD_A2DP_FLUSH, iodev->state,
	      a2dpio->next_flush_time.tv_sec, a2dpio->next_flush_time.tv_nsec);

	/* Only allow data to be flushed after start() ops is called. */
	if ((iodev->state != CRAS_IODEV_STATE_NORMAL_RUN) &&
	    (iodev->state != CRAS_IODEV_STATE_NO_STREAM_RUN))
		return 0;

	err = encode_a2dp_packet(a2dpio);
	if (err < 0)
		return err;

do_flush:
	/* If flush gets called before targeted next flush time, do nothing. */
	clock_gettime(CLOCK_MONOTONIC_RAW, &now);
	add_timespecs(&now, &flush_wake_fuzz_ts);
	if (!timespec_after(&now, &a2dpio->next_flush_time)) {
		if (iodev->buffer_size == bt_local_queued_frames(iodev)) {
			/*
			 * If buffer is full, audio thread will no longer call
			 * into get/put buffer in subsequent wake-ups. In that
			 * case set the registered callback to be triggered at
			 * next audio thread wake up.
			 */
			audio_thread_config_events_callback(
				cras_bt_transport_fd(a2dpio->transport),
				TRIGGER_WAKEUP);
			cras_audio_thread_event_a2dp_overrun();
			syslog(LOG_WARNING, "Buffer overrun in A2DP iodev");
		}
		return 0;
	}

	/* If the A2DP write schedule miss exceeds a small threshold, log it for
	 * debug purpose. */
	subtract_timespecs(&now, &a2dpio->next_flush_time, &ts);
	if (timespec_after(&ts, &throttle_log_threshold))
		ATLOG(atlog, AUDIO_THREAD_A2DP_THROTTLE_TIME, ts.tv_sec,
		      ts.tv_nsec, bt_local_queued_frames(iodev));

	/* Log an event if the A2DP write schedule miss exceeds a large threshold
	 * that we consider it as something severe. */
	if (timespec_after(&ts, &throttle_event_threshold))
		cras_audio_thread_event_a2dp_throttle();

	written = a2dp_write(&a2dpio->a2dp,
			     cras_bt_transport_fd(a2dpio->transport),
			     cras_bt_transport_write_mtu(a2dpio->transport));
	ATLOG(atlog, AUDIO_THREAD_A2DP_WRITE, written,
	      a2dp_queued_frames(&a2dpio->a2dp), 0);
	if (written == -EAGAIN) {
		/* If EAGAIN error lasts longer than 5 seconds, suspend the
		 * a2dp connection. */
		cras_bt_device_schedule_suspend(device, 5000,
						A2DP_LONG_TX_FAILURE);
		audio_thread_config_events_callback(
			cras_bt_transport_fd(a2dpio->transport), TRIGGER_POLL);
		return 0;
	} else if (written < 0) {
		/* Suspend a2dp immediately when receives error other than
		 * EAGAIN. */
		cras_bt_device_cancel_suspend(device);
		cras_bt_device_schedule_suspend(device, 0, A2DP_TX_FATAL_ERROR);
		/* Stop polling the socket in audio thread. Main thread will
		 * close this iodev soon. */
		audio_thread_config_events_callback(
			cras_bt_transport_fd(a2dpio->transport), TRIGGER_NONE);
		return written;
	}

	/* Update the next flush time if one block successfully been written. */
	if (written)
		add_timespecs(&a2dpio->next_flush_time, &a2dpio->flush_period);

	/* a2dp_write no longer return -EAGAIN when reaches here, disable
	 * the polling write callback. */
	audio_thread_config_events_callback(
		cras_bt_transport_fd(a2dpio->transport), TRIGGER_NONE);

	/* Data succcessfully written to a2dp socket, cancel any scheduled
	 * suspend timer. */
	cras_bt_device_cancel_suspend(device);

	/* If it looks okay to write more and we do have queued data, try
	 * encode more. But avoid the case when PCM buffer level is too close
	 * to min_buffer_level so that another A2DP write could causes underrun.
	 */
	queued_frames = buf_queued(a2dpio->pcm_buf) / format_bytes;
	if (written &&
	    (iodev->min_buffer_level + a2dpio->write_block < queued_frames)) {
		err = encode_a2dp_packet(a2dpio);
		if (err < 0)
			return err;
		goto do_flush;
	}

	return 0;
}

static int delay_frames(const struct cras_iodev *iodev)
{
	const struct a2dp_io *a2dpio = (struct a2dp_io *)iodev;
	struct timespec tstamp;

	/* The number of frames in the pcm buffer plus two mtu packets */
	return frames_queued(iodev, &tstamp) + a2dpio->sock_depth_frames;
}

static int get_buffer(struct cras_iodev *iodev, struct cras_audio_area **area,
		      unsigned *frames)
{
	size_t format_bytes;
	struct a2dp_io *a2dpio;

	a2dpio = (struct a2dp_io *)iodev;

	format_bytes = cras_get_format_bytes(iodev->format);

	if (iodev->direction != CRAS_STREAM_OUTPUT)
		return 0;

	*frames = MIN(*frames, buf_writable(a2dpio->pcm_buf) / format_bytes);
	iodev->area->frames = *frames;
	cras_audio_area_config_buf_pointers(iodev->area, iodev->format,
					    buf_write_pointer(a2dpio->pcm_buf));
	*area = iodev->area;
	return 0;
}

static int put_buffer(struct cras_iodev *iodev, unsigned nwritten)
{
	size_t written_bytes;
	size_t format_bytes;
	struct a2dp_io *a2dpio = (struct a2dp_io *)iodev;

	format_bytes = cras_get_format_bytes(iodev->format);
	written_bytes = nwritten * format_bytes;

	if (written_bytes > buf_writable(a2dpio->pcm_buf))
		return -EINVAL;

	buf_increment_write(a2dpio->pcm_buf, written_bytes);

	return encode_and_flush(iodev);
}

static int flush_buffer(struct cras_iodev *iodev)
{
	return 0;
}

static void set_volume(struct cras_iodev *iodev)
{
	size_t volume;
	struct a2dp_io *a2dpio = (struct a2dp_io *)iodev;
	struct cras_bt_device *device =
		cras_bt_transport_device(a2dpio->transport);

	if (!cras_bt_device_get_use_hardware_volume(device))
		return;

	volume = iodev->active_node->volume * 127 / 100;

	if (a2dpio->transport)
		cras_bt_transport_set_volume(a2dpio->transport, volume);
}

static void update_active_node(struct cras_iodev *iodev, unsigned node_idx,
			       unsigned dev_enabled)
{
}

void free_resources(struct a2dp_io *a2dpio)
{
	struct cras_ionode *node;

	node = a2dpio->base.active_node;
	if (node) {
		cras_iodev_rm_node(&a2dpio->base, node);
		free(node);
	}
	free(a2dpio->base.supported_channel_counts);
	free(a2dpio->base.supported_rates);
	free(a2dpio->base.supported_formats);
	destroy_a2dp(&a2dpio->a2dp);
}

struct cras_iodev *a2dp_iodev_create(struct cras_bt_transport *transport)
{
	int err;
	struct a2dp_io *a2dpio;
	struct cras_iodev *iodev;
	struct cras_ionode *node;
	a2dp_sbc_t a2dp;
	struct cras_bt_device *device;
	const char *name;

	a2dpio = (struct a2dp_io *)calloc(1, sizeof(*a2dpio));
	if (!a2dpio)
		goto error;

	a2dpio->transport = transport;
	cras_bt_transport_configuration(a2dpio->transport, &a2dp, sizeof(a2dp));
	err = init_a2dp(&a2dpio->a2dp, &a2dp);
	if (err) {
		syslog(LOG_ERR, "Fail to init a2dp");
		goto error;
	}

	iodev = &a2dpio->base;

	/* A2DP only does output now */
	iodev->direction = CRAS_STREAM_OUTPUT;

	/* Set iodev's name by bluetooth device's readable name, if
	 * the readable name is not available, use address instead.
	 */
	device = cras_bt_transport_device(transport);
	name = cras_bt_device_name(device);
	if (!name)
		name = cras_bt_transport_object_path(a2dpio->transport);

	snprintf(iodev->info.name, sizeof(iodev->info.name), "%s", name);
	iodev->info.name[ARRAY_SIZE(iodev->info.name) - 1] = '\0';
	iodev->info.stable_id = cras_bt_device_get_stable_id(device);

	iodev->configure_dev = configure_dev;
	iodev->frames_queued = frames_queued;
	iodev->delay_frames = delay_frames;
	iodev->get_buffer = get_buffer;
	iodev->put_buffer = put_buffer;
	iodev->flush_buffer = flush_buffer;
	iodev->no_stream = no_stream;
	iodev->output_underrun = output_underrun;
	iodev->close_dev = close_dev;
	iodev->update_supported_formats = update_supported_formats;
	iodev->update_active_node = update_active_node;
	iodev->set_volume = set_volume;
	iodev->start = start;
	iodev->frames_to_play_in_sleep = frames_to_play_in_sleep;

	/* Create an empty ionode */
	node = (struct cras_ionode *)calloc(1, sizeof(*node));
	node->dev = iodev;
	strcpy(node->name, iodev->info.name);
	node->plugged = 1;
	node->type = CRAS_NODE_TYPE_BLUETOOTH;
	node->volume = 100;
	gettimeofday(&node->plugged_time, NULL);

	/* Prepare active node before append, so bt_io can extract correct
	 * info from A2DP iodev and node. */
	cras_iodev_add_node(iodev, node);
	cras_iodev_set_active_node(iodev, node);
	cras_bt_device_append_iodev(
		device, iodev, cras_bt_transport_profile(a2dpio->transport));

	/* Record max supported channels into cras_iodev_info. */
	iodev->info.max_supported_channels =
		(a2dp.channel_mode == SBC_CHANNEL_MODE_MONO) ? 1 : 2;

	ewma_power_disable(&iodev->ewma);

	return iodev;
error:
	if (a2dpio) {
		free_resources(a2dpio);
		free(a2dpio);
	}
	return NULL;
}

void a2dp_iodev_destroy(struct cras_iodev *iodev)
{
	struct a2dp_io *a2dpio = (struct a2dp_io *)iodev;
	struct cras_bt_device *device;

	a2dpio->destroyed = 1;
	device = cras_bt_transport_device(a2dpio->transport);

	/* A2DP does output only */
	cras_bt_device_rm_iodev(device, iodev);

	/* Free resources when device successfully removed. */
	free_resources(a2dpio);
	cras_iodev_free_resources(iodev);
	free(a2dpio);
}
