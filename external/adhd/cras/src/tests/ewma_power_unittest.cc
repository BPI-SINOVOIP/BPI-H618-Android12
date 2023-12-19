// Copyright (c) 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>

extern "C" {
#include "ewma_power.h"
}

namespace {

TEST(EWMAPower, RelativePowerValue) {
  struct ewma_power ewma;
  int16_t buf[480];
  float f;
  int i;

  for (i = 0; i < 480; i++)
    buf[i] = 0x00fe;

  ewma_power_init(&ewma, 48000);
  EXPECT_EQ(48, ewma.step_fr);

  ewma_power_calculate(&ewma, buf, 1, 480);
  EXPECT_LT(0.0f, ewma.power);

  // After 10ms of silence the power value decreases.
  f = ewma.power;
  for (i = 0; i < 480; i++)
    buf[i] = 0x00;
  ewma_power_calculate(&ewma, buf, 1, 480);
  EXPECT_LT(ewma.power, f);

  // After 300ms of silence the power value decreases to insignificant low.
  for (i = 0; i < 30; i++)
    ewma_power_calculate(&ewma, buf, 1, 480);
  EXPECT_LT(ewma.power, 1.0e-10);
}

TEST(EWMAPower, PowerInStereoData) {
  struct ewma_power ewma;
  int16_t buf[960];
  int i;
  float f;

  ewma_power_init(&ewma, 48000);

  for (i = 0; i < 960; i += 2) {
    buf[i] = 0x0;
    buf[i + 1] = 0x00fe;
  }
  ewma_power_calculate(&ewma, buf, 2, 480);
  EXPECT_LT(0.0f, ewma.power);

  // After 10ms of silence the power value decreases.
  f = ewma.power;
  for (i = 0; i < 960; i++)
    buf[i] = 0x0;
  ewma_power_calculate(&ewma, buf, 2, 480);
  EXPECT_LT(ewma.power, f);

  // After 300ms of silence the power value decreases to insignificant low.
  for (i = 0; i < 30; i++)
    ewma_power_calculate(&ewma, buf, 2, 480);
  EXPECT_LT(ewma.power, 1.0e-10);

  // Assume the data is silent in the other channel.
  ewma_power_init(&ewma, 48000);

  for (i = 0; i < 960; i += 2) {
    buf[i] = 0x0ffe;
    buf[i + 1] = 0x0;
  }
  ewma_power_calculate(&ewma, buf, 2, 480);
  EXPECT_LT(0.0f, ewma.power);
}

TEST(EWMAPower, PowerInAudioArea) {
  struct ewma_power ewma;
  struct cras_audio_area* area = cras_audio_area_create(4);
  struct cras_audio_format* fmt =
      cras_audio_format_create(SND_PCM_FORMAT_S16_LE, 48000, 4);
  int8_t layout[CRAS_CH_MAX] = {0, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  int16_t buf[1920];
  int i;
  float f;

  cras_audio_format_set_channel_layout(fmt, layout);
  cras_audio_area_config_channels(area, fmt);

  for (i = 0; i < 1920; i += 4) {
    buf[i] = 0x0ffe;
    buf[i + 1] = 0x0;
    buf[i + 2] = 0x0;
    buf[i + 3] = 0x0ffe;
  }
  ewma_power_init(&ewma, 48000);
  ewma_power_calculate_area(&ewma, buf, area, 480);
  f = ewma.power;
  EXPECT_LT(0.0f, f);

  /* Change the layout in the same audio area. Expect the power be lower because
   * one of the channel is now silent. */
  layout[CRAS_CH_FR] = 2;
  cras_audio_format_set_channel_layout(fmt, layout);
  cras_audio_area_config_channels(area, fmt);
  ewma_power_init(&ewma, 48000);
  ewma_power_calculate_area(&ewma, buf, area, 480);
  EXPECT_GT(f, ewma.power);

  /* Change layout to the two silent channels. Expect power is 0.0f. */
  layout[CRAS_CH_FL] = 1;
  cras_audio_format_set_channel_layout(fmt, layout);
  cras_audio_area_config_channels(area, fmt);
  ewma_power_init(&ewma, 48000);
  ewma_power_calculate_area(&ewma, buf, area, 480);
  EXPECT_EQ(0.0f, ewma.power);

  cras_audio_format_destroy(fmt);
  cras_audio_area_destroy(area);
}

}  // namespace

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
