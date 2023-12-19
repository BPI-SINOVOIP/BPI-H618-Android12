/* Copyright 2021 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <gtest/gtest.h>

extern "C" {
#include "cras_client.c"
#include "cras_client.h"

inline int libcras_unsupported_func(struct libcras_client* client) {
  CHECK_VERSION(client, INT_MAX);
  return 0;
}

cras_stream_id_t cb_stream_id;
uint8_t* cb_buf;
unsigned int cb_frames;
struct timespec cb_latency;
void* cb_usr_arg;
int get_stream_cb_called;
struct timespec now;

int get_stream_cb(struct libcras_stream_cb_data* data) {
  get_stream_cb_called++;
  EXPECT_NE((void*)NULL, data);
  EXPECT_EQ(0, libcras_stream_cb_data_get_stream_id(data, &cb_stream_id));
  EXPECT_EQ(0, libcras_stream_cb_data_get_buf(data, &cb_buf));
  EXPECT_EQ(0, libcras_stream_cb_data_get_frames(data, &cb_frames));
  EXPECT_EQ(0, libcras_stream_cb_data_get_latency(data, &cb_latency));
  EXPECT_EQ(0, libcras_stream_cb_data_get_usr_arg(data, &cb_usr_arg));
  return 0;
}
}

namespace {
class CrasAbiTestSuite : public testing::Test {
 protected:
  struct cras_audio_shm* InitShm(int frames) {
    struct cras_audio_shm* shm =
        static_cast<struct cras_audio_shm*>(calloc(1, sizeof(*shm)));
    shm->header =
        static_cast<cras_audio_shm_header*>(calloc(1, sizeof(*shm->header)));
    cras_shm_set_frame_bytes(shm, 4);
    uint32_t used_size = frames * 4;
    cras_shm_set_used_size(shm, used_size);
    shm->samples_info.length = used_size * 2;
    memcpy(&shm->header->config, &shm->config, sizeof(shm->config));
    return shm;
  }

  void DestroyShm(struct cras_audio_shm* shm) {
    if (shm)
      free(shm->header);
    free(shm);
  }

  virtual void SetUp() { get_stream_cb_called = 0; }
};

TEST_F(CrasAbiTestSuite, CheckUnsupportedFunction) {
  auto* client = libcras_client_create();
  EXPECT_NE((void*)NULL, client);
  EXPECT_EQ(-ENOSYS, libcras_unsupported_func(client));
  libcras_client_destroy(client);
}

TEST_F(CrasAbiTestSuite, BasicStream) {
  auto* client = libcras_client_create();
  EXPECT_NE((void*)NULL, client);
  auto* stream = libcras_stream_params_create();
  EXPECT_NE((void*)NULL, stream);
  /* Returns timeout because there is no real CRAS server in unittest. */
  EXPECT_EQ(-ETIMEDOUT, libcras_client_connect_timeout(client, 0));
  EXPECT_EQ(0, libcras_client_run_thread(client));
  EXPECT_EQ(0, libcras_stream_params_set(stream, CRAS_STREAM_INPUT, 480, 480,
                                         CRAS_STREAM_TYPE_DEFAULT,
                                         CRAS_CLIENT_TYPE_TEST, 0, NULL, NULL,
                                         NULL, 48000, SND_PCM_FORMAT_S16, 2));
  cras_stream_id_t id;
  /* Fails to add a stream because the stream callback is not set. */
  EXPECT_EQ(-EINVAL, libcras_client_add_pinned_stream(client, 0, &id, stream));
  /* Fails to set a stream volume because the stream is not added. */
  EXPECT_EQ(-EINVAL, libcras_client_set_stream_volume(client, id, 1.0));
  EXPECT_EQ(0, libcras_client_rm_stream(client, id));
  EXPECT_EQ(0, libcras_client_stop(client));
  libcras_stream_params_destroy(stream);
  libcras_client_destroy(client);
}

TEST_F(CrasAbiTestSuite, StreamCallback) {
  struct client_stream stream;
  struct cras_stream_params params;
  stream.id = 0x123;
  stream.direction = CRAS_STREAM_INPUT;
  stream.flags = 0;
  stream.config = &params;
  params.stream_cb = get_stream_cb;
  params.cb_threshold = 480;
  params.user_data = (void*)0x321;
  stream.shm = InitShm(960);
  stream.shm->header->write_offset[0] = 960 * 4;
  stream.shm->header->write_buf_idx = 0;
  stream.shm->header->read_offset[0] = 0;
  stream.shm->header->read_buf_idx = 0;
  now.tv_sec = 100;
  now.tv_nsec = 0;
  stream.shm->header->ts.tv_sec = 90;
  stream.shm->header->ts.tv_nsec = 0;

  handle_capture_data_ready(&stream, 480);

  EXPECT_EQ(1, get_stream_cb_called);
  EXPECT_EQ(stream.id, cb_stream_id);
  EXPECT_EQ(cras_shm_get_write_buffer_base(stream.shm), cb_buf);
  EXPECT_EQ(480, cb_frames);
  EXPECT_EQ(10, cb_latency.tv_sec);
  EXPECT_EQ(0, cb_latency.tv_nsec);
  EXPECT_EQ((void*)0x321, cb_usr_arg);

  DestroyShm(stream.shm);
}

}  // namespace

extern "C" {

int clock_gettime(clockid_t clk_id, struct timespec* tp) {
  *tp = now;
  return 0;
}
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  openlog(NULL, LOG_PERROR, LOG_USER);
  return RUN_ALL_TESTS();
}
