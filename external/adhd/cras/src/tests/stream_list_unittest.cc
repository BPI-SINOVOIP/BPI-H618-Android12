// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>
#include <stdio.h>

extern "C" {
#include "cras_rstream.h"
#include "stream_list.h"
}

namespace {

static unsigned int add_called;
static int added_cb(struct cras_rstream* rstream) {
  add_called++;
  return 0;
}

static unsigned int rm_called;
static struct cras_rstream* rmed_stream;
static int removed_cb(struct cras_rstream* rstream) {
  rm_called++;
  rmed_stream = rstream;
  return 0;
}

static unsigned int create_called;
static struct cras_rstream_config* create_config;
static int create_rstream_cb(struct cras_rstream_config* stream_config,
                             struct cras_rstream** stream) {
  create_called++;
  create_config = stream_config;
  *stream = (struct cras_rstream*)malloc(sizeof(struct cras_rstream));
  (*stream)->stream_id = stream_config->stream_id;
  (*stream)->direction = stream_config->direction;
  if (stream_config->format)
    (*stream)->format = *(stream_config->format);
  (*stream)->cb_threshold = stream_config->cb_threshold;
  (*stream)->client_type = stream_config->client_type;
  (*stream)->stream_type = stream_config->stream_type;
  clock_gettime(CLOCK_MONOTONIC_RAW, &(*stream)->start_ts);

  return 0;
}

static unsigned int destroy_called;
static struct cras_rstream* destroyed_stream;
static void destroy_rstream_cb(struct cras_rstream* rstream) {
  destroy_called++;
  destroyed_stream = rstream;
  free(rstream);
}

static void reset_test_data() {
  add_called = 0;
  rm_called = 0;
  create_called = 0;
  destroy_called = 0;
}

TEST(StreamList, AddRemove) {
  struct stream_list* l;
  struct cras_rstream* s1;
  struct cras_rstream_config s1_config;

  s1_config.stream_id = 0x3003;
  s1_config.direction = CRAS_STREAM_OUTPUT;
  s1_config.format = NULL;

  reset_test_data();
  l = stream_list_create(added_cb, removed_cb, create_rstream_cb,
                         destroy_rstream_cb, NULL);
  stream_list_add(l, &s1_config, &s1);
  EXPECT_EQ(1, add_called);
  EXPECT_EQ(1, create_called);
  EXPECT_EQ(&s1_config, create_config);
  EXPECT_EQ(0, stream_list_rm(l, 0x3003));
  EXPECT_EQ(1, rm_called);
  EXPECT_EQ(s1, rmed_stream);
  EXPECT_EQ(1, destroy_called);
  EXPECT_EQ(s1, destroyed_stream);
  stream_list_destroy(l);
}

TEST(StreamList, AddInDescendingOrderByChannels) {
  struct stream_list* l;
  struct cras_rstream* s1;
  struct cras_rstream* s2;
  struct cras_rstream* s3;
  struct cras_audio_format s1_format, s2_format, s3_format;
  struct cras_rstream_config s1_config, s2_config, s3_config;

  s1_config.stream_id = 0x4001;
  s1_config.direction = CRAS_STREAM_INPUT;
  s1_format.num_channels = 6;
  s1_config.format = &s1_format;

  s2_config.stream_id = 0x4002;
  s2_config.direction = CRAS_STREAM_OUTPUT;
  s2_format.num_channels = 8;
  s2_config.format = &s2_format;

  s3_config.stream_id = 0x4003;
  s3_config.direction = CRAS_STREAM_OUTPUT;
  s3_format.num_channels = 2;
  s3_config.format = &s3_format;

  reset_test_data();
  l = stream_list_create(added_cb, removed_cb, create_rstream_cb,
                         destroy_rstream_cb, NULL);
  stream_list_add(l, &s1_config, &s1);
  EXPECT_EQ(1, add_called);
  EXPECT_EQ(1, create_called);
  EXPECT_EQ(6, stream_list_get(l)->format.num_channels);

  stream_list_add(l, &s2_config, &s2);
  EXPECT_EQ(2, add_called);
  EXPECT_EQ(2, create_called);
  EXPECT_EQ(8, stream_list_get(l)->format.num_channels);
  EXPECT_EQ(6, stream_list_get(l)->next->format.num_channels);

  stream_list_add(l, &s3_config, &s3);
  EXPECT_EQ(3, add_called);
  EXPECT_EQ(3, create_called);
  EXPECT_EQ(8, stream_list_get(l)->format.num_channels);
  EXPECT_EQ(6, stream_list_get(l)->next->format.num_channels);
  EXPECT_EQ(2, stream_list_get(l)->next->next->format.num_channels);
  EXPECT_EQ(0, stream_list_rm(l, 0x4001));
  EXPECT_EQ(0, stream_list_rm(l, 0x4002));
  EXPECT_EQ(0, stream_list_rm(l, 0x4003));
  stream_list_destroy(l);
}

TEST(StreamList, DetectRtcStreamPair) {
  struct stream_list* l;
  struct cras_rstream *s1, *s2, *s3, *s4;
  struct cras_rstream_config s1_config, s2_config, s3_config, s4_config;

  s1_config.stream_id = 0x5001;
  s1_config.direction = CRAS_STREAM_OUTPUT;
  s1_config.cb_threshold = 480;
  s1_config.client_type = CRAS_CLIENT_TYPE_CHROME;
  s1_config.stream_type = CRAS_STREAM_TYPE_DEFAULT;
  s1_config.format = NULL;

  s2_config.stream_id = 0x5002;
  s2_config.direction = CRAS_STREAM_INPUT;
  s2_config.cb_threshold = 480;
  s2_config.client_type = CRAS_CLIENT_TYPE_CHROME;
  s2_config.stream_type = CRAS_STREAM_TYPE_DEFAULT;
  s2_config.format = NULL;

  // s3 is not a RTC stream because the cb threshold is not 480.
  s3_config.stream_id = 0x5003;
  s3_config.direction = CRAS_STREAM_INPUT;
  s3_config.cb_threshold = 500;
  s3_config.client_type = CRAS_CLIENT_TYPE_CHROME;
  s3_config.stream_type = CRAS_STREAM_TYPE_DEFAULT;
  s3_config.format = NULL;

  // s4 is not a RTC stream because it is not from the same client with s1.
  s4_config.stream_id = 0x5004;
  s4_config.direction = CRAS_STREAM_INPUT;
  s4_config.cb_threshold = 480;
  s4_config.client_type = CRAS_CLIENT_TYPE_LACROS;
  s4_config.stream_type = CRAS_STREAM_TYPE_DEFAULT;
  s4_config.format = NULL;

  reset_test_data();
  l = stream_list_create(added_cb, removed_cb, create_rstream_cb,
                         destroy_rstream_cb, NULL);
  stream_list_add(l, &s1_config, &s1);
  EXPECT_EQ(1, add_called);
  EXPECT_EQ(1, create_called);
  EXPECT_EQ(&s1_config, create_config);

  stream_list_add(l, &s2_config, &s2);
  detect_rtc_stream_pair(l, s2);
  stream_list_add(l, &s3_config, &s3);
  detect_rtc_stream_pair(l, s3);
  stream_list_add(l, &s4_config, &s4);
  detect_rtc_stream_pair(l, s4);

  EXPECT_EQ(CRAS_STREAM_TYPE_VOICE_COMMUNICATION, s1->stream_type);
  EXPECT_EQ(CRAS_STREAM_TYPE_VOICE_COMMUNICATION, s2->stream_type);
  EXPECT_EQ(CRAS_STREAM_TYPE_DEFAULT, s3->stream_type);
  EXPECT_EQ(CRAS_STREAM_TYPE_DEFAULT, s4->stream_type);

  EXPECT_EQ(0, stream_list_rm(l, 0x5001));
  EXPECT_EQ(0, stream_list_rm(l, 0x5002));
  EXPECT_EQ(0, stream_list_rm(l, 0x5003));
  EXPECT_EQ(0, stream_list_rm(l, 0x5004));
  stream_list_destroy(l);
}

extern "C" {

struct cras_timer* cras_tm_create_timer(struct cras_tm* tm,
                                        unsigned int ms,
                                        void (*cb)(struct cras_timer* t,
                                                   void* data),
                                        void* cb_data) {
  return reinterpret_cast<struct cras_timer*>(0x404);
}

void cras_tm_cancel_timer(struct cras_tm* tm, struct cras_timer* t) {}
}

}  // namespace

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
