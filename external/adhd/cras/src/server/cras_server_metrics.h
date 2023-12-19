/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_SERVER_METRICS_H_
#define CRAS_SERVER_METRICS_H_

#include <stdbool.h>

#include "cras_iodev.h"
#include "cras_rstream.h"

extern const char kNoCodecsFoundMetric[];

enum CRAS_METRICS_BT_SCO_ERROR_TYPE {
	CRAS_METRICS_SCO_SKT_SUCCESS = 0,
	CRAS_METRICS_SCO_SKT_CONNECT_ERROR = 1,
	CRAS_METRICS_SCO_SKT_OPEN_ERROR = 2,
	CRAS_METRICS_SCO_SKT_POLL_TIMEOUT = 3,
	CRAS_METRICS_SCO_SKT_POLL_ERR_HUP = 4,
};

/* Logs the error type happens when setting up SCO connection. This is mainly
 * used to track whether the setup of SCO connection succeeds and the frequency
 * of different errors. This will also be used to track if our fixes for these
 * errors address the issues we find.
 */
int cras_server_metrics_hfp_sco_connection_error(
	enum CRAS_METRICS_BT_SCO_ERROR_TYPE type);

/* Logs an enum representing which spec does HFP headset supports battery
 * indicator. Apple, HFP, none or both. */
int cras_server_metrics_hfp_battery_indicator(int battery_indicator_support);

/* Logs an enum representing the spec through which the battery level change
 * event reported. Apple or HFP.*/
int cras_server_metrics_hfp_battery_report(int battery_report);

/* Logs if connected HFP headset supports wideband speech. */
int cras_server_metrics_hfp_wideband_support(bool supported);

/* Logs the selected codec in HFP wideband connection. */
int cras_server_metrics_hfp_wideband_selected_codec(int codec);

/* Logs the number of packet loss per 1000 packets under HFP capture. */
int cras_server_metrics_hfp_packet_loss(float packet_loss_ratio);

/* Logs runtime of a device. */
int cras_server_metrics_device_runtime(struct cras_iodev *iodev);

/* Logs the gain of a device. */
int cras_server_metrics_device_gain(struct cras_iodev *iodev);

/* Logs the volume of a device. */
int cras_server_metrics_device_volume(struct cras_iodev *iodev);

/* Logs the highest delay time of a device. */
int cras_server_metrics_highest_device_delay(
	unsigned int hw_level, unsigned int largest_cb_level,
	enum CRAS_STREAM_DIRECTION direction);

/* Logs the highest hardware level of a device. */
int cras_server_metrics_highest_hw_level(unsigned hw_level,
					 enum CRAS_STREAM_DIRECTION direction);

/* Logs the number of underruns of a device. */
int cras_server_metrics_num_underruns(unsigned num_underruns);

/* Logs the missed callback event. */
int cras_server_metrics_missed_cb_event(struct cras_rstream *stream);

/* Logs information when a stream creates. */
int cras_server_metrics_stream_create(const struct cras_rstream_config *config);

/* Logs information when a stream destroys. */
int cras_server_metrics_stream_destroy(const struct cras_rstream *stream);

/* Logs the number of busyloops for different time periods. */
int cras_server_metrics_busyloop(struct timespec *ts, unsigned count);

/* Logs the length of busyloops. */
int cras_server_metrics_busyloop_length(unsigned length);

/* Initialize metrics logging stuff. */
int cras_server_metrics_init();

#endif /* CRAS_SERVER_METRICS_H_ */
