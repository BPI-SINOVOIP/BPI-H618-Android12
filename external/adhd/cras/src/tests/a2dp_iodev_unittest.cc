// Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>
#include <stdint.h>
#include <stdio.h>

extern "C" {
#include "cras_a2dp_iodev.c"

#include "audio_thread.h"
#include "audio_thread_log.h"
#include "cras_audio_area.h"
#include "cras_bt_transport.h"
#include "cras_iodev.h"
}

#define FAKE_OBJECT_PATH "/fake/obj/path"

#define MAX_A2DP_ENCODE_CALLS 8
#define MAX_A2DP_WRITE_CALLS 4

/* Fake the codec to encode (512/4) frames into 128 bytes. */
#define FAKE_A2DP_CODE_SIZE 512
#define FAKE_A2DP_FRAME_LENGTH 128

static struct cras_bt_transport* fake_transport;
static cras_audio_format format;
static size_t cras_bt_device_append_iodev_called;
static size_t cras_bt_device_rm_iodev_called;
static size_t cras_iodev_add_node_called;
static size_t cras_iodev_rm_node_called;
static size_t cras_iodev_set_active_node_called;
static size_t cras_bt_transport_acquire_called;
static size_t cras_bt_transport_configuration_called;
static size_t cras_bt_transport_release_called;
static size_t init_a2dp_called;
static int init_a2dp_return_val;
static size_t destroy_a2dp_called;
static size_t a2dp_reset_called;
static size_t cras_iodev_free_format_called;
static size_t cras_iodev_free_resources_called;
static int a2dp_write_return_val[MAX_A2DP_WRITE_CALLS];
static unsigned int a2dp_write_index;
static int a2dp_encode_called;
static cras_audio_area* mock_audio_area;
static thread_callback write_callback;
static void* write_callback_data;
static const char* fake_device_name = "fake device name";
static const char* cras_bt_device_name_ret;
static unsigned int cras_bt_transport_write_mtu_ret;
static int cras_iodev_fill_odev_zeros_called;
static unsigned int cras_iodev_fill_odev_zeros_frames;
static int audio_thread_config_events_callback_called;
static enum AUDIO_THREAD_EVENTS_CB_TRIGGER
    audio_thread_config_events_callback_trigger;

void ResetStubData() {
  cras_bt_device_append_iodev_called = 0;
  cras_bt_device_rm_iodev_called = 0;
  cras_iodev_add_node_called = 0;
  cras_iodev_rm_node_called = 0;
  cras_iodev_set_active_node_called = 0;
  cras_bt_transport_acquire_called = 0;
  cras_bt_transport_configuration_called = 0;
  cras_bt_transport_release_called = 0;
  init_a2dp_called = 0;
  init_a2dp_return_val = 0;
  destroy_a2dp_called = 0;
  a2dp_reset_called = 0;
  cras_iodev_free_format_called = 0;
  cras_iodev_free_resources_called = 0;
  a2dp_write_index = 0;
  a2dp_encode_called = 0;
  /* Fake the MTU value. min_buffer_level will be derived from this value. */
  cras_bt_transport_write_mtu_ret = 950;
  cras_iodev_fill_odev_zeros_called = 0;

  fake_transport = reinterpret_cast<struct cras_bt_transport*>(0x123);

  if (!mock_audio_area) {
    mock_audio_area = (cras_audio_area*)calloc(
        1, sizeof(*mock_audio_area) + sizeof(cras_channel_area) * 2);
  }

  write_callback = NULL;
}

int iodev_set_format(struct cras_iodev* iodev, struct cras_audio_format* fmt) {
  fmt->format = SND_PCM_FORMAT_S16_LE;
  fmt->num_channels = 2;
  fmt->frame_rate = 44100;
  iodev->format = fmt;
  return 0;
}

namespace {

static struct timespec time_now;
class A2dpIodev : public testing::Test {
 protected:
  virtual void SetUp() {
    ResetStubData();
    time_now.tv_sec = 0;
    time_now.tv_nsec = 0;
    atlog = (audio_thread_event_log*)calloc(1, sizeof(audio_thread_event_log));
  }

  virtual void TearDown() {
    free(mock_audio_area);
    mock_audio_area = NULL;
    free(atlog);
  }
};

TEST_F(A2dpIodev, InitializeA2dpIodev) {
  struct cras_iodev* iodev;

  cras_bt_device_name_ret = NULL;
  iodev = a2dp_iodev_create(fake_transport);

  ASSERT_NE(iodev, (void*)NULL);
  ASSERT_EQ(iodev->direction, CRAS_STREAM_OUTPUT);
  ASSERT_EQ(1, cras_bt_transport_configuration_called);
  ASSERT_EQ(1, init_a2dp_called);
  ASSERT_EQ(1, cras_bt_device_append_iodev_called);
  ASSERT_EQ(1, cras_iodev_add_node_called);
  ASSERT_EQ(1, cras_iodev_set_active_node_called);

  /* Assert iodev name matches the object path when bt device doesn't
   * have its readable name populated. */
  ASSERT_STREQ(FAKE_OBJECT_PATH, iodev->info.name);

  a2dp_iodev_destroy(iodev);

  ASSERT_EQ(1, cras_bt_device_rm_iodev_called);
  ASSERT_EQ(1, cras_iodev_rm_node_called);
  ASSERT_EQ(1, destroy_a2dp_called);
  ASSERT_EQ(1, cras_iodev_free_resources_called);

  cras_bt_device_name_ret = fake_device_name;
  /* Assert iodev name matches the bt device's name */
  iodev = a2dp_iodev_create(fake_transport);
  ASSERT_STREQ(fake_device_name, iodev->info.name);

  a2dp_iodev_destroy(iodev);
}

TEST_F(A2dpIodev, InitializeFail) {
  struct cras_iodev* iodev;

  init_a2dp_return_val = -1;
  iodev = a2dp_iodev_create(fake_transport);

  ASSERT_EQ(iodev, (void*)NULL);
  ASSERT_EQ(1, cras_bt_transport_configuration_called);
  ASSERT_EQ(1, init_a2dp_called);
  ASSERT_EQ(0, cras_bt_device_append_iodev_called);
  ASSERT_EQ(0, cras_iodev_add_node_called);
  ASSERT_EQ(0, cras_iodev_set_active_node_called);
  ASSERT_EQ(0, cras_iodev_rm_node_called);
}

TEST_F(A2dpIodev, OpenIodev) {
  struct cras_iodev* iodev;

  iodev = a2dp_iodev_create(fake_transport);

  iodev_set_format(iodev, &format);
  iodev->configure_dev(iodev);
  iodev->start(iodev);
  iodev->state = CRAS_IODEV_STATE_NORMAL_RUN;

  ASSERT_EQ(1, cras_bt_transport_acquire_called);

  iodev->close_dev(iodev);
  ASSERT_EQ(1, cras_bt_transport_release_called);
  ASSERT_EQ(1, a2dp_reset_called);
  ASSERT_EQ(1, cras_iodev_free_format_called);

  a2dp_iodev_destroy(iodev);
}

TEST_F(A2dpIodev, GetPutBuffer) {
  struct cras_iodev* iodev;
  struct cras_audio_area *area1, *area2, *area3;
  uint8_t* last_buf_head;
  unsigned frames;
  struct timespec tstamp;
  struct a2dp_io* a2dpio;

  iodev = a2dp_iodev_create(fake_transport);
  a2dpio = (struct a2dp_io*)iodev;

  iodev_set_format(iodev, &format);
  iodev->configure_dev(iodev);
  ASSERT_NE(write_callback, (void*)NULL);

  iodev->start(iodev);
  iodev->state = CRAS_IODEV_STATE_NORMAL_RUN;

  /* (950 - 13) / 128 * 512 / 4 */
  ASSERT_EQ(iodev->min_buffer_level, 896);

  frames = 1500;
  iodev->get_buffer(iodev, &area1, &frames);
  ASSERT_EQ(1500, frames);
  ASSERT_EQ(1500, area1->frames);
  last_buf_head = area1->channels[0].buf;
  iodev->put_buffer(iodev, 1000);
  /* 1000 frames takes 8 encode call, FAKE_A2DP_CODE_SIZE / 4 = 128
   * and 7 * 128 < 1000 < 8 * 128
   */
  EXPECT_EQ(8, a2dp_encode_called);
  /* Expect flushed one block, leaving 1000 - 896 = 104 queued and
   * next_flush_time has shifted. */
  EXPECT_EQ(1, a2dp_write_index);
  EXPECT_EQ(104, iodev->frames_queued(iodev, &tstamp));
  EXPECT_GT(a2dpio->next_flush_time.tv_nsec, 0);

  /* Assert buffer possition shifted 1000 * 4 bytes */
  frames = 1000;
  iodev->get_buffer(iodev, &area2, &frames);
  ASSERT_EQ(1000, frames);
  ASSERT_EQ(1000, area2->frames);
  ASSERT_EQ(4000, area2->channels[0].buf - last_buf_head);
  last_buf_head = area2->channels[0].buf;

  iodev->put_buffer(iodev, 700);
  EXPECT_EQ(804, iodev->frames_queued(iodev, &tstamp));
  /* Assert that even next_flush_time is not met, pcm data still processed.
   * Expect to takes 7 more encode calls to process the 804 frames of data.
   * and 6 * 128 < 804 < 7 * 128
   */
  EXPECT_EQ(15, a2dp_encode_called);
  EXPECT_EQ(768, a2dpio->a2dp.samples);

  time_now.tv_nsec = 25000000;
  frames = 50;
  iodev->get_buffer(iodev, &area3, &frames);
  ASSERT_EQ(50, frames);
  /* Assert buffer possition shifted 700 * 4 bytes */
  EXPECT_EQ(2800, area3->channels[0].buf - last_buf_head);

  iodev->put_buffer(iodev, 50);
  /* 804 + 50 = 854 queued, 768 of them are encoded. */
  EXPECT_EQ(854, iodev->frames_queued(iodev, &tstamp));
  EXPECT_EQ(768, a2dpio->a2dp.samples);
  /* Expect one a2dp encode call was executed for the left un-encoded frames.
   * 854 - 768 = 86 < 128 */
  EXPECT_EQ(16, a2dp_encode_called);
  /* Even time now has passed next_flush_time, no a2dp write gets called
   * because the number of encoded samples is not sufficient for a flush. */
  EXPECT_EQ(1, a2dp_write_index);

  iodev->close_dev(iodev);
  a2dp_iodev_destroy(iodev);
}

TEST_F(A2dpIodev, FramesQueued) {
  struct cras_iodev* iodev;
  struct cras_audio_area* area;
  struct timespec tstamp;
  unsigned frames;
  struct a2dp_io* a2dpio;

  iodev = a2dp_iodev_create(fake_transport);
  a2dpio = (struct a2dp_io*)iodev;

  iodev_set_format(iodev, &format);
  time_now.tv_sec = 0;
  time_now.tv_nsec = 0;
  iodev->configure_dev(iodev);
  ASSERT_NE(write_callback, (void*)NULL);
  /* a2dp_block_size(mtu) / format_bytes
   * (950 - 13) / 128 * 512 / 4 = 896 */
  EXPECT_EQ(896, a2dpio->write_block);

  iodev->start(iodev);
  iodev->state = CRAS_IODEV_STATE_NORMAL_RUN;

  frames = 256;
  iodev->get_buffer(iodev, &area, &frames);
  ASSERT_EQ(256, frames);
  ASSERT_EQ(256, area->frames);

  /* Data less than write_block hence not written. */
  iodev->put_buffer(iodev, 200);
  EXPECT_EQ(200, iodev->frames_queued(iodev, &tstamp));
  EXPECT_EQ(tstamp.tv_sec, time_now.tv_sec);
  EXPECT_EQ(tstamp.tv_nsec, time_now.tv_nsec);

  /* 200 + 800 - 896 = 104 */
  a2dp_write_return_val[0] = 0;
  frames = 800;
  iodev->get_buffer(iodev, &area, &frames);
  iodev->put_buffer(iodev, 800);
  EXPECT_EQ(104, iodev->frames_queued(iodev, &tstamp));

  /* Some time has passed, same amount of frames are queued. */
  time_now.tv_nsec = 15000000;
  write_callback(write_callback_data, POLLOUT);
  EXPECT_EQ(104, iodev->frames_queued(iodev, &tstamp));

  /* Put 900 more frames. next_flush_time not yet passed so expect
   * total 900 + 104 = 1004 are queued. */
  frames = 900;
  iodev->get_buffer(iodev, &area, &frames);
  iodev->put_buffer(iodev, 900);
  EXPECT_EQ(1004, iodev->frames_queued(iodev, &tstamp));

  /* Time passes next_flush_time, 1004 + 300 - 896 = 408 */
  time_now.tv_nsec = 25000000;
  frames = 300;
  iodev->get_buffer(iodev, &area, &frames);
  iodev->put_buffer(iodev, 300);
  EXPECT_EQ(408, iodev->frames_queued(iodev, &tstamp));

  iodev->close_dev(iodev);
  a2dp_iodev_destroy(iodev);
}

TEST_F(A2dpIodev, SleepTimeWithWriteThrottle) {
  struct cras_iodev* iodev;
  struct cras_audio_area* area;
  unsigned frames;
  unsigned int level;
  unsigned long target;
  struct timespec tstamp;
  struct a2dp_io* a2dpio;

  iodev = a2dp_iodev_create(fake_transport);
  a2dpio = (struct a2dp_io*)iodev;

  iodev_set_format(iodev, &format);
  iodev->configure_dev(iodev);
  ASSERT_NE(write_callback, (void*)NULL);
  /* a2dp_block_size(mtu) / format_bytes
   * 900 / 128 * 512 / 4 = 896 */
  EXPECT_EQ(896, a2dpio->write_block);

  iodev->start(iodev);
  iodev->state = CRAS_IODEV_STATE_NORMAL_RUN;

  /* Both time now and next_flush_time are at 0. Expect write_block of
   * time to sleep */
  EXPECT_EQ(a2dpio->write_block,
            iodev->frames_to_play_in_sleep(iodev, &level, &tstamp));

  /* Fake that 1000 frames are put and one block got flushed.
   * Expect next_wake_time be fast forward by one flush_period. */
  frames = 1000;
  iodev->get_buffer(iodev, &area, &frames);
  ASSERT_EQ(1000, frames);
  ASSERT_EQ(1000, area->frames);

  /* Expect the first block be flushed at time 0. */
  time_now.tv_nsec = 0;
  a2dp_write_return_val[0] = 0;
  EXPECT_EQ(0, iodev->put_buffer(iodev, 1000));
  EXPECT_EQ(104, iodev->frames_queued(iodev, &tstamp)); /* 1000 - 896 */

  /* Same amount of frames are queued after some time has passed. */
  time_now.tv_nsec = 10000000;
  EXPECT_EQ(104, iodev->frames_queued(iodev, &tstamp));

  /* Expect to sleep the time between now(10ms) and next_flush_time(~20.3ms). */
  frames = iodev->frames_to_play_in_sleep(iodev, &level, &tstamp);
  target =
      a2dpio->write_block - time_now.tv_nsec * format.frame_rate / 1000000000;
  EXPECT_GE(frames + 1, target);
  EXPECT_GE(target + 1, frames);

  /* Time now has passed the next flush time(~20.3ms), expect to return
   * write_block of time to sleep. */
  time_now.tv_nsec = 25000000;
  EXPECT_EQ(a2dpio->write_block,
            iodev->frames_to_play_in_sleep(iodev, &level, &tstamp));

  a2dp_write_return_val[1] = 0;
  frames = 1000;
  iodev->get_buffer(iodev, &area, &frames);
  EXPECT_EQ(0, iodev->put_buffer(iodev, 1000));
  EXPECT_EQ(208, iodev->frames_queued(iodev, &tstamp)); /* 104 + 1000 - 896 */

  /* Flush another write_block of data, next_wake_time fast forward by
   * another flush_period. Expect to sleep the time between now(25ms)
   * and next_flush_time(~40.6ms). */
  frames = iodev->frames_to_play_in_sleep(iodev, &level, &tstamp);
  target = a2dpio->write_block * 2 -
           time_now.tv_nsec * format.frame_rate / 1000000000;
  EXPECT_GE(frames + 1, target);
  EXPECT_GE(target + 1, frames);

  /* Put 1000 more frames, and make a fake failure to this flush. */
  time_now.tv_nsec = 45000000;
  a2dp_write_return_val[2] = -EAGAIN;
  frames = 1000;
  iodev->get_buffer(iodev, &area, &frames);
  EXPECT_EQ(0, iodev->put_buffer(iodev, 1000));

  /* Last a2dp write call failed with -EAGAIN, time now(45ms) is after
   * next_flush_time. Expect to return exact |write_block| equivalant
   * of time to sleep. */
  EXPECT_EQ(1208, iodev->frames_queued(iodev, &tstamp)); /* 208 + 1000 */
  EXPECT_EQ(a2dpio->write_block,
            iodev->frames_to_play_in_sleep(iodev, &level, &tstamp));

  /* Fake the event that socket becomes writable so data continues to flush.
   * next_flush_time fast forwards by another flush_period. */
  a2dp_write_return_val[3] = 0;
  write_callback(write_callback_data, POLLOUT);
  EXPECT_EQ(312, iodev->frames_queued(iodev, &tstamp)); /* 1208 - 896 */

  /* Expect to sleep the time between now and next_flush_time(~60.9ms). */
  frames = iodev->frames_to_play_in_sleep(iodev, &level, &tstamp);
  target = a2dpio->write_block * 3 -
           time_now.tv_nsec * format.frame_rate / 1000000000;
  EXPECT_GE(frames + 1, target);
  EXPECT_GE(target + 1, frames);

  iodev->close_dev(iodev);
  a2dp_iodev_destroy(iodev);
}

TEST_F(A2dpIodev, EnableThreadCallbackAtBufferFull) {
  struct cras_iodev* iodev;
  struct cras_audio_area* area;
  struct timespec tstamp;
  unsigned frames;
  struct a2dp_io* a2dpio;

  iodev = a2dp_iodev_create(fake_transport);
  a2dpio = (struct a2dp_io*)iodev;

  iodev_set_format(iodev, &format);
  time_now.tv_sec = 0;
  time_now.tv_nsec = 0;
  iodev->configure_dev(iodev);
  ASSERT_NE(write_callback, (void*)NULL);

  iodev->start(iodev);
  iodev->state = CRAS_IODEV_STATE_NORMAL_RUN;

  audio_thread_config_events_callback_called = 0;
  a2dp_write_return_val[0] = 0;
  frames = iodev->buffer_size;
  iodev->get_buffer(iodev, &area, &frames);
  EXPECT_LE(frames, iodev->buffer_size);
  EXPECT_EQ(0, iodev->put_buffer(iodev, frames));
  EXPECT_EQ(1, a2dp_write_index);
  EXPECT_EQ(a2dpio->flush_period.tv_nsec, a2dpio->next_flush_time.tv_nsec);
  EXPECT_EQ(1, audio_thread_config_events_callback_called);
  EXPECT_EQ(TRIGGER_NONE, audio_thread_config_events_callback_trigger);

  /* Fastfoward time 1ms, not yet reaches the next flush time. */
  time_now.tv_nsec = 1000000;

  /* Cram into iodev as much data as possible. Expect its buffer to
   * be full because flush time does not yet met. */
  frames = iodev->buffer_size;
  iodev->get_buffer(iodev, &area, &frames);
  EXPECT_LE(frames, iodev->buffer_size);
  EXPECT_EQ(0, iodev->put_buffer(iodev, frames));
  frames = iodev->frames_queued(iodev, &tstamp);
  EXPECT_EQ(frames, iodev->buffer_size);

  /* Expect a2dp_write didn't get called in last get/put buffer. And
   * audio thread callback has been enabled. */
  EXPECT_EQ(1, a2dp_write_index);
  EXPECT_EQ(2, audio_thread_config_events_callback_called);
  EXPECT_EQ(TRIGGER_WAKEUP, audio_thread_config_events_callback_trigger);

  iodev->close_dev(iodev);
  a2dp_iodev_destroy(iodev);
}

TEST_F(A2dpIodev, FlushAtLowBufferLevel) {
  struct cras_iodev* iodev;
  struct cras_audio_area* area;
  struct timespec tstamp;
  unsigned frames;

  iodev = a2dp_iodev_create(fake_transport);

  iodev_set_format(iodev, &format);
  iodev->configure_dev(iodev);
  ASSERT_NE(write_callback, (void*)NULL);

  /* (950 - 13)/ 128 * 512 / 4 */
  ASSERT_EQ(iodev->min_buffer_level, 896);

  iodev->start(iodev);
  iodev->state = CRAS_IODEV_STATE_NORMAL_RUN;

  frames = 1500;
  iodev->get_buffer(iodev, &area, &frames);
  ASSERT_EQ(1500, frames);
  ASSERT_EQ(1500, area->frames);

  /*
   * Assert put_buffer shouldn't trigger the 2nd call to a2dp_encode()
   * because buffer is low: 896 < 1500 < 896 * 2
   */
  a2dp_write_return_val[0] = 0;
  EXPECT_EQ(0, iodev->put_buffer(iodev, 1500));
  EXPECT_EQ(1, a2dp_write_index);

  /* 1500 - 896 */
  time_now.tv_nsec = 25000000;
  EXPECT_EQ(604, iodev->frames_queued(iodev, &tstamp));
  EXPECT_EQ(tstamp.tv_sec, time_now.tv_sec);
  EXPECT_EQ(tstamp.tv_nsec, time_now.tv_nsec);
  iodev->close_dev(iodev);
  a2dp_iodev_destroy(iodev);
}

TEST_F(A2dpIodev, HandleUnderrun) {
  struct cras_iodev* iodev;
  struct cras_audio_area* area;
  struct timespec tstamp;
  unsigned frames;

  iodev = a2dp_iodev_create(fake_transport);

  iodev_set_format(iodev, &format);
  time_now.tv_sec = 0;
  time_now.tv_nsec = 0;
  iodev->configure_dev(iodev);
  /* (950 - 13) / 128 * 512 / 4 */
  EXPECT_EQ(896, iodev->min_buffer_level);

  iodev->start(iodev);
  iodev->state = CRAS_IODEV_STATE_NORMAL_RUN;

  frames = 300;
  iodev->get_buffer(iodev, &area, &frames);
  ASSERT_EQ(300, frames);
  ASSERT_EQ(300, area->frames);
  a2dp_write_return_val[0] = -EAGAIN;

  time_now.tv_nsec = 10000000;
  iodev->put_buffer(iodev, 300);

  time_now.tv_nsec = 20000000;
  EXPECT_EQ(300, iodev->frames_queued(iodev, &tstamp));

  /* Frames queued below min_buffer_level, which is derived from transport MTU.
   * Assert min_cb_level of zero frames are filled. */
  iodev->min_cb_level = 150;
  iodev->output_underrun(iodev);
  ASSERT_EQ(1, cras_iodev_fill_odev_zeros_called);
  EXPECT_EQ(150, cras_iodev_fill_odev_zeros_frames);

  iodev->close_dev(iodev);
  a2dp_iodev_destroy(iodev);
}

TEST_F(A2dpIodev, LeavingNoStreamStateWithSmallStreamDoesntUnderrun) {
  struct cras_iodev* iodev;
  struct cras_audio_area* area;
  struct timespec tstamp;
  unsigned frames;
  struct a2dp_io* a2dpio;

  iodev = a2dp_iodev_create(fake_transport);
  a2dpio = (struct a2dp_io*)iodev;

  iodev_set_format(iodev, &format);
  time_now.tv_sec = 0;
  time_now.tv_nsec = 0;
  iodev->configure_dev(iodev);
  ASSERT_NE(write_callback, (void*)NULL);
  /* (950 - 13)/ 128 * 512 / 4 */
  ASSERT_EQ(896, iodev->min_buffer_level);

  iodev->start(iodev);
  iodev->state = CRAS_IODEV_STATE_NORMAL_RUN;

  /* Put iodev in no_stream state. Verify it doesn't underrun after each
   * call of no_stream ops. */
  a2dp_write_return_val[0] = 0;
  iodev->no_stream(iodev, 1);
  EXPECT_EQ(1, a2dp_write_index);
  EXPECT_EQ(a2dpio->flush_period.tv_nsec, a2dpio->next_flush_time.tv_nsec);
  frames = iodev->frames_queued(iodev, &tstamp);
  EXPECT_LE(iodev->min_buffer_level, frames);

  /* Some time has passed and a small stream of 200 frames block is added.
   * Verify leaving no_stream state doesn't underrun immediately. */
  time_now.tv_nsec = 20000000;
  iodev->no_stream(iodev, 1);
  frames = 200;
  iodev->get_buffer(iodev, &area, &frames);
  iodev->put_buffer(iodev, 200);
  frames = iodev->frames_queued(iodev, &tstamp);
  EXPECT_LE(iodev->min_buffer_level, frames);

  iodev->close_dev(iodev);
  a2dp_iodev_destroy(iodev);
}

TEST_F(A2dpIodev, NoStreamStateFillZerosToTargetLevel) {
  struct cras_iodev* iodev;
  struct cras_audio_area* area;
  struct timespec tstamp;
  unsigned frames;
  struct a2dp_io* a2dpio;

  iodev = a2dp_iodev_create(fake_transport);
  a2dpio = (struct a2dp_io*)iodev;

  iodev_set_format(iodev, &format);
  time_now.tv_sec = 0;
  time_now.tv_nsec = 0;
  iodev->configure_dev(iodev);
  ASSERT_NE(write_callback, (void*)NULL);
  /* (950 - 13)/ 128 * 512 / 4 */
  ASSERT_EQ(896, iodev->min_buffer_level);

  iodev->start(iodev);
  iodev->state = CRAS_IODEV_STATE_NORMAL_RUN;

  iodev->min_cb_level = 480;
  frames = 200;
  iodev->get_buffer(iodev, &area, &frames);
  iodev->put_buffer(iodev, 200);

  a2dp_write_return_val[0] = 0;
  iodev->no_stream(iodev, 1);
  EXPECT_EQ(1, a2dp_write_index);
  EXPECT_EQ(a2dpio->flush_period.tv_nsec, a2dpio->next_flush_time.tv_nsec);

  /* Some time has passed but not yet reach next flush. Entering no_stream
   * fills buffer to 3 times of min_buffer_level. */
  time_now.tv_nsec = 10000000;
  iodev->no_stream(iodev, 1);
  frames = iodev->frames_queued(iodev, &tstamp);
  EXPECT_EQ(3 * iodev->min_buffer_level, frames);

  /* Time has passed next flush time, expect one block is flushed.  */
  a2dp_write_return_val[1] = 0;
  time_now.tv_nsec = 25000000;
  iodev->no_stream(iodev, 1);
  frames = iodev->frames_queued(iodev, &tstamp);
  ASSERT_EQ(2 * iodev->min_buffer_level, frames);
  EXPECT_EQ(2, a2dp_write_index);

  /* Leaving no_stream state fills buffer level back to  2 * min_buffer_level.
   */
  a2dp_write_return_val[2] = 0;
  time_now.tv_nsec = 30000000;
  iodev->no_stream(iodev, 0);
  frames = iodev->frames_queued(iodev, &tstamp);
  ASSERT_EQ(2 * iodev->min_buffer_level, frames);
  EXPECT_EQ(2, a2dp_write_index);

  iodev->close_dev(iodev);
  a2dp_iodev_destroy(iodev);
}

TEST_F(A2dpIodev, EnterNoStreamStateAtHighBufferLevelDoesntFillMore) {
  struct cras_iodev* iodev;
  struct cras_audio_area* area;
  struct timespec tstamp;
  unsigned frames, start_level;
  struct a2dp_io* a2dpio;

  iodev = a2dp_iodev_create(fake_transport);
  a2dpio = (struct a2dp_io*)iodev;

  iodev_set_format(iodev, &format);
  time_now.tv_sec = 0;
  time_now.tv_nsec = 0;
  iodev->configure_dev(iodev);
  ASSERT_NE(write_callback, (void*)NULL);
  /* (950 - 13)/ 128 * 512 / 4 */
  ASSERT_EQ(896, iodev->min_buffer_level);

  iodev->start(iodev);
  iodev->state = CRAS_IODEV_STATE_NORMAL_RUN;

  a2dp_write_return_val[0] = 0;
  start_level = 6000;
  frames = start_level;
  iodev->get_buffer(iodev, &area, &frames);
  iodev->put_buffer(iodev, frames);
  frames = iodev->frames_queued(iodev, &tstamp);
  /* Assert one block has fluxhed */
  EXPECT_EQ(start_level - iodev->min_buffer_level, frames);
  EXPECT_EQ(1, a2dp_write_index);
  EXPECT_EQ(a2dpio->flush_period.tv_nsec, a2dpio->next_flush_time.tv_nsec);

  a2dp_write_return_val[1] = 0;
  time_now.tv_nsec = 25000000;
  iodev->no_stream(iodev, 1);
  frames = iodev->frames_queued(iodev, &tstamp);
  /* Next flush time meets requirement so another block is flushed. */
  ASSERT_EQ(start_level - 2 * iodev->min_buffer_level, frames);

  a2dp_write_return_val[2] = 0;
  time_now.tv_nsec = 50000000;
  iodev->no_stream(iodev, 1);
  frames = iodev->frames_queued(iodev, &tstamp);
  /* Another block flushed at leaving no stream state. No more data
   * filled because level is high. */
  ASSERT_EQ(start_level - 3 * iodev->min_buffer_level, frames);

  iodev->close_dev(iodev);
  a2dp_iodev_destroy(iodev);
}
}  // namespace

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

extern "C" {

int cras_bt_transport_configuration(const struct cras_bt_transport* transport,
                                    void* configuration,
                                    int len) {
  memset(configuration, 0, len);
  cras_bt_transport_configuration_called++;
  return 0;
}

int cras_bt_transport_acquire(struct cras_bt_transport* transport) {
  cras_bt_transport_acquire_called++;
  return 0;
}

int cras_bt_transport_release(struct cras_bt_transport* transport,
                              unsigned int blocking) {
  cras_bt_transport_release_called++;
  return 0;
}

int cras_bt_transport_fd(const struct cras_bt_transport* transport) {
  return 0;
}

const char* cras_bt_transport_object_path(
    const struct cras_bt_transport* transport) {
  return FAKE_OBJECT_PATH;
}

uint16_t cras_bt_transport_write_mtu(
    const struct cras_bt_transport* transport) {
  return cras_bt_transport_write_mtu_ret;
}

int cras_bt_transport_set_volume(struct cras_bt_transport* transport,
                                 uint16_t volume) {
  return 0;
}

void cras_iodev_free_format(struct cras_iodev* iodev) {
  cras_iodev_free_format_called++;
}

void cras_iodev_free_resources(struct cras_iodev* iodev) {
  cras_iodev_free_resources_called++;
}

// Cras iodev
void cras_iodev_add_node(struct cras_iodev* iodev, struct cras_ionode* node) {
  cras_iodev_add_node_called++;
  iodev->nodes = node;
}

void cras_iodev_rm_node(struct cras_iodev* iodev, struct cras_ionode* node) {
  cras_iodev_rm_node_called++;
  iodev->nodes = NULL;
}

void cras_iodev_set_active_node(struct cras_iodev* iodev,
                                struct cras_ionode* node) {
  cras_iodev_set_active_node_called++;
  iodev->active_node = node;
}

// From cras_bt_transport
struct cras_bt_device* cras_bt_transport_device(
    const struct cras_bt_transport* transport) {
  return reinterpret_cast<struct cras_bt_device*>(0x456);
  ;
}

enum cras_bt_device_profile cras_bt_transport_profile(
    const struct cras_bt_transport* transport) {
  return CRAS_BT_DEVICE_PROFILE_A2DP_SOURCE;
}

// From cras_bt_device
const char* cras_bt_device_name(const struct cras_bt_device* device) {
  return cras_bt_device_name_ret;
}

const char* cras_bt_device_object_path(const struct cras_bt_device* device) {
  return "/org/bluez/hci0/dev_1A_2B_3C_4D_5E_6F";
}

int cras_bt_device_get_stable_id(const struct cras_bt_device* device) {
  return 123;
}

void cras_bt_device_append_iodev(struct cras_bt_device* device,
                                 struct cras_iodev* iodev,
                                 enum cras_bt_device_profile profile) {
  cras_bt_device_append_iodev_called++;
}

void cras_bt_device_rm_iodev(struct cras_bt_device* device,
                             struct cras_iodev* iodev) {
  cras_bt_device_rm_iodev_called++;
}

int cras_bt_device_get_use_hardware_volume(struct cras_bt_device* device) {
  return 0;
}

int cras_bt_device_cancel_suspend(struct cras_bt_device* device) {
  return 0;
}

int cras_bt_device_schedule_suspend(
    struct cras_bt_device* device,
    unsigned int msec,
    enum cras_bt_device_suspend_reason suspend_reason) {
  return 0;
}

int init_a2dp(struct a2dp_info* a2dp, a2dp_sbc_t* sbc) {
  init_a2dp_called++;
  memset(a2dp, 0, sizeof(*a2dp));
  a2dp->frame_length = FAKE_A2DP_FRAME_LENGTH;
  a2dp->codesize = FAKE_A2DP_CODE_SIZE;
  return init_a2dp_return_val;
}

void destroy_a2dp(struct a2dp_info* a2dp) {
  destroy_a2dp_called++;
}

int a2dp_codesize(struct a2dp_info* a2dp) {
  return a2dp->codesize;
}

int a2dp_block_size(struct a2dp_info* a2dp, int encoded_bytes) {
  return encoded_bytes / a2dp->frame_length * a2dp->codesize;
}

int a2dp_queued_frames(const struct a2dp_info* a2dp) {
  return a2dp->samples;
}

void a2dp_reset(struct a2dp_info* a2dp) {
  a2dp_reset_called++;
  a2dp->samples = 0;
}

int a2dp_encode(struct a2dp_info* a2dp,
                const void* pcm_buf,
                int pcm_buf_size,
                int format_bytes,
                size_t link_mtu) {
  int processed = 0;
  a2dp_encode_called++;

  if (a2dp->a2dp_buf_used + a2dp->frame_length > link_mtu)
    return 0;
  if (pcm_buf_size < a2dp->codesize)
    return 0;

  processed += a2dp->codesize;
  a2dp->a2dp_buf_used += a2dp->frame_length;
  a2dp->samples += processed / format_bytes;

  return processed;
}

int a2dp_write(struct a2dp_info* a2dp, int stream_fd, size_t link_mtu) {
  int ret, samples;
  if (a2dp->frame_length + a2dp->a2dp_buf_used < link_mtu)
    return 0;

  ret = a2dp_write_return_val[a2dp_write_index++];
  if (ret < 0)
    return ret;

  samples = a2dp->samples;
  a2dp->samples = 0;
  a2dp->a2dp_buf_used = 0;
  return samples;
}

int clock_gettime(clockid_t clk_id, struct timespec* tp) {
  *tp = time_now;
  return 0;
}

void cras_iodev_init_audio_area(struct cras_iodev* iodev, int num_channels) {
  iodev->area = mock_audio_area;
}

void cras_iodev_free_audio_area(struct cras_iodev* iodev) {}

int cras_iodev_fill_odev_zeros(struct cras_iodev* odev, unsigned int frames) {
  struct cras_audio_area* area;
  cras_iodev_fill_odev_zeros_called++;
  cras_iodev_fill_odev_zeros_frames = frames;

  odev->get_buffer(odev, &area, &frames);
  odev->put_buffer(odev, frames);
  return 0;
}

void cras_audio_area_config_buf_pointers(struct cras_audio_area* area,
                                         const struct cras_audio_format* fmt,
                                         uint8_t* base_buffer) {
  mock_audio_area->channels[0].buf = base_buffer;
}

struct audio_thread* cras_iodev_list_get_audio_thread() {
  return NULL;
}
// From ewma_power
void ewma_power_disable(struct ewma_power* ewma) {}

// From audio_thread
struct audio_thread_event_log* atlog;

void audio_thread_add_events_callback(int fd,
                                      thread_callback cb,
                                      void* data,
                                      int events) {
  write_callback = cb;
  write_callback_data = data;
}

int audio_thread_rm_callback_sync(struct audio_thread* thread, int fd) {
  return 0;
}

void audio_thread_config_events_callback(
    int fd,
    enum AUDIO_THREAD_EVENTS_CB_TRIGGER trigger) {
  audio_thread_config_events_callback_called++;
  audio_thread_config_events_callback_trigger = trigger;
}
}

int cras_audio_thread_event_a2dp_overrun() {
  return 0;
}

int cras_audio_thread_event_a2dp_throttle() {
  return 0;
}
