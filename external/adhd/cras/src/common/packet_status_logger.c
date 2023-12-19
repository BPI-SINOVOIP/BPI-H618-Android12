/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>
#include <time.h>

#include "cras_util.h"
#include "packet_status_logger.h"

void packet_status_logger_init(struct packet_status_logger *logger)
{
	memset(logger->data, 0, PACKET_STATUS_LEN_BYTES);
	logger->size = PACKET_STATUS_LEN_BYTES * 8;
	logger->wp = 0;
	logger->num_wraps = 0;
	clock_gettime(CLOCK_MONOTONIC_RAW, &logger->ts);
}

void packet_status_logger_update(struct packet_status_logger *logger, bool val)
{
	if (val) {
		logger->data[logger->wp / 8] |= 1UL << (logger->wp % 8);
	} else {
		logger->data[logger->wp / 8] &= ~(1UL << (logger->wp % 8));
	}
	logger->wp++;
	if (logger->wp >= logger->size) {
		logger->wp %= logger->size;
		logger->num_wraps += 1;
	}
	if (logger->wp == 0 || (logger->num_wraps == 0 && logger->wp == 1))
		clock_gettime(CLOCK_MONOTONIC_RAW, &logger->ts);
}
