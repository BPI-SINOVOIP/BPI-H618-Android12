// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Generated from files in cras/src/server/rust/src in adhd.

#ifndef RATE_ESTIMATOR_H_
#define RATE_ESTIMATOR_H_

#include <time.h>

/**
 * An estimator holding the required information to determine the actual frame
 * rate of an audio device.
 *
 * # Members
 *    * `last_level` - Buffer level of the audio device at last check time.
 *    * `level_diff` - Number of frames written to or read from audio device
 *                     since the last check time. Rate estimator will use this
 *                     change plus the difference of buffer level to derive the
 *                     number of frames audio device has actually processed.
 *    * `window_start` - The start time of the current window.
 *    * `window_size` - The size of the window.
 *    * `window_frames` - The number of frames accumulated in current window.
 *    * `lsq` - The helper used to estimate sample rate.
 *    * `smooth_factor` - A scaling factor used to average the previous and new
 *                        rate estimates to ensure that estimates do not change
 *                        too quickly.
 *    * `estimated_rate` - The estimated rate at which samples are consumed.
 */
typedef struct rate_estimator rate_estimator;

/**
 * # Safety
 *
 * To use this function safely, `re` must be a pointer returned from
 * rate_estimator_create, or null.
 */
void rate_estimator_add_frames(rate_estimator *re, int frames);

/**
 * # Safety
 *
 * To use this function safely, `re` must be a pointer returned from
 * rate_estimator_create, or null, and `now` must be a valid pointer to a
 * timespec.
 */
int32_t rate_estimator_check(rate_estimator *re, int level,
			     const struct timespec *now);

/**
 * # Safety
 *
 * To use this function safely, `window_size` must be a valid pointer to a
 * timespec.
 */
rate_estimator *rate_estimator_create(unsigned int rate,
				      const struct timespec *window_size,
				      double smooth_factor);

/**
 * # Safety
 *
 * To use this function safely, `re` must be a pointer returned from
 * rate_estimator_create, or null.
 */
void rate_estimator_destroy(rate_estimator *re);

/**
 * # Safety
 *
 * To use this function safely, `re` must be a pointer returned from
 * rate_estimator_create, or null.
 */
double rate_estimator_get_rate(const rate_estimator *re);

/**
 * # Safety
 *
 * To use this function safely, `re` must be a pointer returned from
 * rate_estimator_create, or null.
 */
void rate_estimator_reset_rate(rate_estimator *re, unsigned int rate);

#endif /* RATE_ESTIMATOR_H_ */
