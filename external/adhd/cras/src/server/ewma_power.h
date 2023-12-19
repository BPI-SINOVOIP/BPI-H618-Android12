/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef EWMA_POWER_H_
#define EWMA_POWER_H_

#include <stdbool.h>
#include <stdint.h>

#include "cras_audio_area.h"

/*
 * The exponentially weighted moving average power module used to
 * calculate the energe level in audio stream.
 * Members:
 *    power_set - Flag to note if the first power value has set.
 *    enabled - Flag to enable ewma calculation. Set to false to
 *        make all calculations no-ops.
 *    power - The power value.
 *    step_fr - How many frames to sample one for EWMA calculation.
 */
struct ewma_power {
	bool power_set;
	bool enabled;
	float power;
	unsigned int step_fr;
};

/*
 * Disables the ewma instance.
 */
void ewma_power_disable(struct ewma_power *ewma);

/*
 * Initializes the ewma_power object.
 * Args:
 *    ewma - The ewma_power object to initialize.
 *    rate - The sample rate of the audio data that the ewma object
 *        will calculate power from.
 */
void ewma_power_init(struct ewma_power *ewma, unsigned int rate);

/*
 * Feeds an audio buffer to ewma_power object to calculate the
 * latest power value.
 * Args:
 *    ewma - The ewma_power object to calculate power.
 *    buf - Pointer to the audio data.
 *    channels - Number of channels of the audio data.
 *    size - Length in frames of the audio data.
 */
void ewma_power_calculate(struct ewma_power *ewma, const int16_t *buf,
			  unsigned int channels, unsigned int size);

/*
 * Feeds non-interleaved audio data to ewma_power to calculate the
 * latest power value. This is similar to ewma_power_calculate but
 * accepts cras_audio_area.
 */
void ewma_power_calculate_area(struct ewma_power *ewma, const int16_t *buf,
			       struct cras_audio_area *area, unsigned int size);

#endif /* EWMA_POWER_H_ */
