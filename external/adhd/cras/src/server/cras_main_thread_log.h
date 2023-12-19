/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_MAIN_THREAD_LOG_H_
#define CRAS_MAIN_THREAD_LOG_H_

#include <stdint.h>

#include "cras_types.h"

#define CRAS_MAIN_THREAD_LOGGING 1

#if (CRAS_MAIN_THREAD_LOGGING)
#define MAINLOG(log, event, data1, data2, data3)                               \
	main_thread_event_log_data(log, event, data1, data2, data3);
#else
#define MAINLOG(log, event, data1, data2, data3)
#endif

extern struct main_thread_event_log *main_log;

static inline struct main_thread_event_log *main_thread_event_log_init()
{
	struct main_thread_event_log *log;
	log = (struct main_thread_event_log *)calloc(
		1, sizeof(struct main_thread_event_log));
	if (!log)
		return NULL;

	log->len = MAIN_THREAD_EVENT_LOG_SIZE;
	return log;
}

static inline void
main_thread_event_log_deinit(struct main_thread_event_log *log)
{
	if (log)
		free(log);
}

static inline void main_thread_event_log_data(struct main_thread_event_log *log,
					      enum MAIN_THREAD_LOG_EVENTS event,
					      uint32_t data1, uint32_t data2,
					      uint32_t data3)
{
	struct timespec now;

	if (!log)
		return;

	clock_gettime(CLOCK_MONOTONIC_RAW, &now);
	log->log[log->write_pos].tag_sec =
		(event << 24) | ((now.tv_sec % 86400) & 0x00ffffff);
	log->log[log->write_pos].nsec = now.tv_nsec;
	log->log[log->write_pos].data1 = data1;
	log->log[log->write_pos].data2 = data2;
	log->log[log->write_pos].data3 = data3;
	log->write_pos++;
	log->write_pos %= MAIN_THREAD_EVENT_LOG_SIZE;
}

#endif /* CRAS_MAIN_THREAD_LOG_H_ */
