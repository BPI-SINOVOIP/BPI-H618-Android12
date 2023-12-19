/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef PACKET_STATUS_LOGGER_
#define PACKET_STATUS_LOGGER_

#include <stdint.h>
#include <stdbool.h>

#define PACKET_STATUS_LEN_BYTES 64
#define WBS_FRAME_NS 7500000

/* Avoid 32, 40, 64 consecutive hex characters so CrOS feedback redact
 * tool doesn't trim our dump. */
#define PACKET_STATUS_LOG_LINE_WRAP 50

/*
 * Object to log consecutive packets' status.
 * Members:
 *    data - Bytes to store packets' status.
 *    size - Total number of bits in |data|.
 *    wp - Position of the next bit to log packet status.
 *    num_wraps - Number of times the ring buffer has wrapped.
 *    ts - The timestamp of the last time when the first bit of |data| updated.
 */
struct packet_status_logger {
	uint8_t data[PACKET_STATUS_LEN_BYTES];
	int size;
	int wp;
	int num_wraps;
	struct timespec ts;
};

/* Initializes the packet status logger. */
void packet_status_logger_init(struct packet_status_logger *logger);

/* Updates the next packet status to logger. */
void packet_status_logger_update(struct packet_status_logger *logger, bool val);

/* Rewinds logger's time stamp to calculate the beginning.
 * If logger's ring buffer hasn't wrapped, simply return logger_ts.
 * Otherwise beginning_ts = logger_ts - WBS_FRAME_NS * (size - wp)
 */
static inline void
packet_status_logger_begin_ts(const struct packet_status_logger *logger,
			      struct timespec *ts)
{
	long nsec = WBS_FRAME_NS * (logger->size - logger->wp);

	*ts = logger->ts;
	if (logger->num_wraps == 0)
		return;
	while (nsec > 1000000000L) {
		ts->tv_sec--;
		nsec -= 1000000000L;
	}
	ts->tv_nsec -= nsec;
	if (ts->tv_nsec < 0) {
		ts->tv_sec--;
		ts->tv_nsec += 1000000000L;
	}
}

/* Fast-forwards the logger's time stamp to calculate the end.
 * In other words, end_ts = logger_ts + WBS_FRAME_NS * wp
 */
static inline void
packet_status_logger_end_ts(const struct packet_status_logger *logger,
			    struct timespec *ts)
{
	*ts = logger->ts;
	ts->tv_nsec += WBS_FRAME_NS * logger->wp;
	while (ts->tv_nsec > 1000000000L) {
		ts->tv_sec++;
		ts->tv_nsec -= 1000000000L;
	}
}

/* Prints the logger data in hex format */
static inline void
packet_status_logger_dump_hex(const struct packet_status_logger *logger)
{
	int i = logger->wp / 8;

	/* Print the bits after wp only if buffer has wrapped. */
	if (logger->num_wraps) {
		if (logger->wp % 8)
			printf("%.2x",
			       logger->data[i] & (0xff << (logger->wp % 8)));
		for (; i < PACKET_STATUS_LEN_BYTES; i++)
			printf("%.2x", logger->data[i]);
	}
	for (i = 0; i < logger->wp / 8; i++)
		printf("%.2x", logger->data[i]);
	if (logger->wp % 8)
		printf("%.2x", logger->data[i] & (~(0xff << (logger->wp % 8))));
	printf("\n");
}

/* Prints the logger data in binary format */
static inline void
packet_status_logger_dump_binary(const struct packet_status_logger *logger)
{
	/* Don't print the bits after wp if buffer hasn't wrapped. */
	int head = logger->num_wraps ? logger->wp : 0;
	int len = logger->num_wraps ? logger->size : logger->wp;
	int i, j;

	for (i = 0; i < len; ++i) {
		j = (head + i) % logger->size;
		printf("%d", (logger->data[j / 8] >> (j % 8)) & 1U);
		if ((i + 1) % PACKET_STATUS_LOG_LINE_WRAP == 0)
			printf("\n");
	}
	/* Fill indicator digit 'D' until the last line wraps. */
	if (len % PACKET_STATUS_LOG_LINE_WRAP) {
		while (len % PACKET_STATUS_LOG_LINE_WRAP) {
			printf("D");
			++len;
		}
		printf("\n");
	}
}

#endif /* PACKET_STATUS_LOGGER_ */
