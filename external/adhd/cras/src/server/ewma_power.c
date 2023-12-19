/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "ewma_power.h"
#include "math.h"

/* One sample per 1ms. */
#define EWMA_SAMPLE_RATE 1000

/* Smooth factor for EWMA, 1 - expf(-1.0/(rate * 0.01))
 * where the 0.01 corresponds to 10ms interval that is chosen and
 * being used in Chrome for a long time.
 * Here the |rate| is set to the down sampled EWMA_SAMPLE_RATE and
 * whenever it changes the calculated |smooth_factor| should be updated
 * accordingly.
 */
const static float smooth_factor = 0.095;

void ewma_power_disable(struct ewma_power *ewma)
{
	ewma->enabled = 0;
}

void ewma_power_init(struct ewma_power *ewma, unsigned int rate)
{
	ewma->enabled = 1;
	ewma->power_set = 0;
	ewma->step_fr = rate / EWMA_SAMPLE_RATE;
}

void ewma_power_calculate(struct ewma_power *ewma, const int16_t *buf,
			  unsigned int channels, unsigned int size)
{
	int i, ch;
	float power, f;

	if (!ewma->enabled)
		return;
	for (i = 0; i < size; i += ewma->step_fr * channels) {
		power = 0.0f;
		for (ch = 0; ch < channels; ch++) {
			f = buf[i + ch] / 32768.0f;
			power += f * f / channels;
		}
		if (!ewma->power_set) {
			ewma->power = power;
			ewma->power_set = 1;
		} else {
			ewma->power = smooth_factor * power +
				      (1 - smooth_factor) * ewma->power;
		}
	}
}

void ewma_power_calculate_area(struct ewma_power *ewma, const int16_t *buf,
			       struct cras_audio_area *area, unsigned int size)
{
	int i, ch;
	float power, f;

	if (!ewma->enabled)
		return;
	for (i = 0; i < size; i += ewma->step_fr * area->num_channels) {
		power = 0.0f;
		for (ch = 0; ch < area->num_channels; ch++) {
			if (area->channels[ch].ch_set == 0)
				continue;
			f = buf[i + ch] / 32768.0f;
			power += f * f / area->num_channels;
		}
		if (!ewma->power_set) {
			ewma->power = power;
			ewma->power_set = 1;
		} else {
			ewma->power = smooth_factor * power +
				      (1 - smooth_factor) * ewma->power;
		}
	}
}
