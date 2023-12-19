// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>
#include <stdio.h>
#include <string.h>

extern "C" {
#include "cras_alert.h"
#include "cras_board_config.h"
#include "cras_shm.h"
#include "cras_system_state.h"
#include "cras_types.h"
}

namespace {
static struct cras_alsa_card* kFakeAlsaCard;
size_t cras_alsa_card_create_called;
size_t cras_alsa_card_destroy_called;
static size_t add_stub_called;
static size_t rm_stub_called;
static size_t add_task_stub_called;
static size_t callback_stub_called;
static void* select_data_value;
static void* task_data_value;
static size_t add_callback_called;
static cras_alert_cb add_callback_cb;
static void* add_callback_arg;
static size_t rm_callback_called;
static cras_alert_cb rm_callback_cb;
static void* rm_callback_arg;
static size_t alert_pending_called;
static char* device_config_dir;
static const char* cras_alsa_card_config_dir;
static size_t cras_observer_notify_output_volume_called;
static size_t cras_observer_notify_output_mute_called;
static size_t cras_observer_notify_capture_mute_called;
static size_t cras_observer_notify_suspend_changed_called;
static size_t cras_observer_notify_num_active_streams_called;
static size_t cras_observer_notify_input_streams_with_permission_called;
static size_t cras_iodev_list_reset_for_noise_cancellation_called;
static struct cras_board_config fake_board_config;
static size_t cras_alert_process_all_pending_alerts_called;

static void ResetStubData() {
  cras_alsa_card_create_called = 0;
  cras_alsa_card_destroy_called = 0;
  kFakeAlsaCard = reinterpret_cast<struct cras_alsa_card*>(0x33);
  add_stub_called = 0;
  rm_stub_called = 0;
  add_task_stub_called = 0;
  callback_stub_called = 0;
  add_callback_called = 0;
  rm_callback_called = 0;
  alert_pending_called = 0;
  device_config_dir = NULL;
  cras_alsa_card_config_dir = NULL;
  cras_observer_notify_output_volume_called = 0;
  cras_observer_notify_output_mute_called = 0;
  cras_observer_notify_capture_mute_called = 0;
  cras_observer_notify_suspend_changed_called = 0;
  cras_observer_notify_num_active_streams_called = 0;
  cras_observer_notify_input_streams_with_permission_called = 0;
  cras_alert_process_all_pending_alerts_called = 0;
  cras_iodev_list_reset_for_noise_cancellation_called = 0;
  memset(&fake_board_config, 0, sizeof(fake_board_config));
}

static int add_stub(int fd,
                    void (*cb)(void* data, int revents),
                    void* callback_data,
                    int events,
                    void* select_data) {
  add_stub_called++;
  select_data_value = select_data;
  return 0;
}

static void rm_stub(int fd, void* select_data) {
  rm_stub_called++;
  select_data_value = select_data;
}

static int add_task_stub(void (*cb)(void* data),
                         void* callback_data,
                         void* task_data) {
  add_task_stub_called++;
  task_data_value = task_data;
  return 0;
}

static void callback_stub(void* data, int revents) {
  callback_stub_called++;
}

static void task_stub(void* data) {
  callback_stub_called++;
}

static void do_sys_init() {
  char* shm_name;
  ASSERT_GT(asprintf(&shm_name, "/cras-%d", getpid()), 0);
  int rw_shm_fd;
  int ro_shm_fd;
  struct cras_server_state* exp_state =
      (struct cras_server_state*)cras_shm_setup(shm_name, sizeof(*exp_state),
                                                &rw_shm_fd, &ro_shm_fd);
  if (!exp_state)
    exit(-1);
  cras_system_state_init(device_config_dir, shm_name, rw_shm_fd, ro_shm_fd,
                         exp_state, sizeof(*exp_state));
  free(shm_name);
}

TEST(SystemStateSuite, DefaultVolume) {
  do_sys_init();
  EXPECT_EQ(100, cras_system_get_volume());
  EXPECT_EQ(0, cras_system_get_mute());
  EXPECT_EQ(0, cras_system_get_capture_mute());
  cras_system_state_deinit();
}

TEST(SystemStateSuite, SetVolume) {
  do_sys_init();
  cras_system_set_volume(0);
  EXPECT_EQ(0, cras_system_get_volume());
  cras_system_set_volume(50);
  EXPECT_EQ(50, cras_system_get_volume());
  cras_system_set_volume(CRAS_MAX_SYSTEM_VOLUME);
  EXPECT_EQ(CRAS_MAX_SYSTEM_VOLUME, cras_system_get_volume());
  cras_system_set_volume(CRAS_MAX_SYSTEM_VOLUME + 1);
  EXPECT_EQ(CRAS_MAX_SYSTEM_VOLUME, cras_system_get_volume());
  cras_system_state_deinit();
  EXPECT_EQ(4, cras_observer_notify_output_volume_called);
}

TEST(SystemStateSuite, SetMinMaxVolume) {
  do_sys_init();
  cras_system_set_volume_limits(-10000, -600);
  EXPECT_EQ(-10000, cras_system_get_min_volume());
  EXPECT_EQ(-600, cras_system_get_max_volume());
  cras_system_state_deinit();
}

TEST(SystemStateSuite, SetUserMute) {
  ResetStubData();
  do_sys_init();

  EXPECT_EQ(0, cras_system_get_mute());

  cras_system_set_user_mute(0);
  EXPECT_EQ(0, cras_system_get_mute());
  EXPECT_EQ(0, cras_observer_notify_output_mute_called);

  cras_system_set_user_mute(1);
  EXPECT_EQ(1, cras_system_get_mute());
  EXPECT_EQ(1, cras_observer_notify_output_mute_called);

  cras_system_set_user_mute(22);
  EXPECT_EQ(1, cras_system_get_mute());
  EXPECT_EQ(1, cras_observer_notify_output_mute_called);

  cras_system_state_deinit();
}

TEST(SystemStateSuite, SetMute) {
  ResetStubData();
  do_sys_init();

  EXPECT_EQ(0, cras_system_get_mute());

  cras_system_set_mute(0);
  EXPECT_EQ(0, cras_system_get_mute());
  EXPECT_EQ(0, cras_observer_notify_output_mute_called);

  cras_system_set_mute(1);
  EXPECT_EQ(1, cras_system_get_mute());
  EXPECT_EQ(1, cras_observer_notify_output_mute_called);

  cras_system_set_mute(22);
  EXPECT_EQ(1, cras_system_get_mute());
  EXPECT_EQ(1, cras_observer_notify_output_mute_called);

  cras_system_state_deinit();
}

TEST(SystemStateSuite, SetSystemMuteThenSwitchUserMute) {
  ResetStubData();
  do_sys_init();

  EXPECT_EQ(0, cras_system_get_mute());

  // Set system mute.
  cras_system_set_mute(1);

  // Switching user mute will not notify observer.
  EXPECT_EQ(1, cras_observer_notify_output_mute_called);
  cras_system_set_user_mute(1);
  EXPECT_EQ(1, cras_observer_notify_output_mute_called);
  cras_system_set_user_mute(0);
  EXPECT_EQ(1, cras_observer_notify_output_mute_called);

  // Unset system mute.
  cras_system_set_mute(0);
  EXPECT_EQ(2, cras_observer_notify_output_mute_called);

  cras_system_state_deinit();
}

TEST(SystemStateSuite, SetUserMuteThenSwitchSystemMute) {
  ResetStubData();
  do_sys_init();

  EXPECT_EQ(0, cras_system_get_mute());

  // Set user mute.
  cras_system_set_user_mute(1);

  // Switching system mute will not notify observer.
  EXPECT_EQ(1, cras_observer_notify_output_mute_called);
  cras_system_set_mute(1);
  EXPECT_EQ(1, cras_observer_notify_output_mute_called);
  cras_system_set_mute(0);
  EXPECT_EQ(1, cras_observer_notify_output_mute_called);

  // Unset user mute.
  cras_system_set_user_mute(0);
  EXPECT_EQ(2, cras_observer_notify_output_mute_called);

  cras_system_state_deinit();
}

TEST(SystemStateSuite, CaptureMuteChangedCallbackMultiple) {
  do_sys_init();
  ResetStubData();

  cras_system_set_capture_mute(1);
  EXPECT_EQ(1, cras_system_get_capture_mute());
  EXPECT_EQ(1, cras_observer_notify_capture_mute_called);
  cras_system_set_capture_mute(0);
  EXPECT_EQ(0, cras_system_get_capture_mute());
  EXPECT_EQ(2, cras_observer_notify_capture_mute_called);

  cras_system_state_deinit();
}

TEST(SystemStateSuite, MuteLocked) {
  do_sys_init();
  ResetStubData();

  cras_system_set_mute(1);
  EXPECT_EQ(1, cras_system_get_mute());
  EXPECT_EQ(0, cras_system_get_mute_locked());
  EXPECT_EQ(1, cras_observer_notify_output_mute_called);

  cras_system_set_mute_locked(1);
  cras_system_set_mute(0);
  EXPECT_EQ(1, cras_system_get_mute());
  EXPECT_EQ(1, cras_system_get_mute_locked());
  EXPECT_EQ(1, cras_observer_notify_output_mute_called);

  cras_system_set_capture_mute(1);
  EXPECT_EQ(1, cras_system_get_capture_mute());
  EXPECT_EQ(0, cras_system_get_capture_mute_locked());
  EXPECT_EQ(1, cras_observer_notify_capture_mute_called);

  cras_system_set_capture_mute_locked(1);
  cras_system_set_capture_mute(0);
  EXPECT_EQ(1, cras_system_get_capture_mute());
  EXPECT_EQ(1, cras_system_get_capture_mute_locked());
  cras_system_state_deinit();
  EXPECT_EQ(2, cras_observer_notify_capture_mute_called);
}

TEST(SystemStateSuite, Suspend) {
  do_sys_init();
  ResetStubData();

  cras_system_set_suspended(1);
  EXPECT_EQ(1, cras_observer_notify_suspend_changed_called);
  EXPECT_EQ(1, cras_alert_process_all_pending_alerts_called);
  EXPECT_EQ(1, cras_system_get_suspended());

  cras_system_set_suspended(0);
  EXPECT_EQ(2, cras_observer_notify_suspend_changed_called);
  EXPECT_EQ(0, cras_system_get_suspended());

  cras_system_state_deinit();
}

TEST(SystemStateSuite, AddCardFailCreate) {
  ResetStubData();
  kFakeAlsaCard = NULL;
  cras_alsa_card_info info;

  info.card_type = ALSA_CARD_TYPE_INTERNAL;
  info.card_index = 0;
  do_sys_init();
  EXPECT_EQ(-ENOMEM, cras_system_add_alsa_card(&info));
  EXPECT_EQ(1, cras_alsa_card_create_called);
  EXPECT_EQ(cras_alsa_card_config_dir, device_config_dir);
  cras_system_state_deinit();
}

TEST(SystemStateSuite, AddCard) {
  ResetStubData();
  cras_alsa_card_info info;

  info.card_type = ALSA_CARD_TYPE_INTERNAL;
  info.card_index = 0;
  do_sys_init();
  EXPECT_EQ(0, cras_system_add_alsa_card(&info));
  EXPECT_EQ(1, cras_alsa_card_create_called);
  EXPECT_EQ(cras_alsa_card_config_dir, device_config_dir);
  // Adding the same card again should fail.
  ResetStubData();
  EXPECT_NE(0, cras_system_add_alsa_card(&info));
  EXPECT_EQ(0, cras_alsa_card_create_called);
  // Removing card should destroy it.
  cras_system_remove_alsa_card(0);
  EXPECT_EQ(1, cras_alsa_card_destroy_called);
  cras_system_state_deinit();
}

TEST(SystemSettingsRegisterSelectDescriptor, AddSelectFd) {
  void* stub_data = reinterpret_cast<void*>(44);
  void* select_data = reinterpret_cast<void*>(33);
  int rc;

  ResetStubData();
  do_sys_init();
  rc = cras_system_add_select_fd(7, callback_stub, stub_data, POLLIN);
  EXPECT_NE(0, rc);
  EXPECT_EQ(0, add_stub_called);
  EXPECT_EQ(0, rm_stub_called);
  rc = cras_system_set_select_handler(add_stub, rm_stub, select_data);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(0, add_stub_called);
  EXPECT_EQ(0, rm_stub_called);
  rc = cras_system_set_select_handler(add_stub, rm_stub, select_data);
  EXPECT_EQ(-EEXIST, rc);
  EXPECT_EQ(0, add_stub_called);
  EXPECT_EQ(0, rm_stub_called);
  rc = cras_system_add_select_fd(7, callback_stub, stub_data, POLLIN);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(1, add_stub_called);
  EXPECT_EQ(select_data, select_data_value);
  cras_system_rm_select_fd(7);
  EXPECT_EQ(1, rm_stub_called);
  EXPECT_EQ(0, callback_stub_called);
  EXPECT_EQ(select_data, select_data_value);
  cras_system_state_deinit();
}

TEST(SystemSettingsAddTask, AddTask) {
  void* stub_data = reinterpret_cast<void*>(44);
  void* task_data = reinterpret_cast<void*>(33);
  int rc;

  do_sys_init();
  rc = cras_system_add_task(task_stub, stub_data);
  EXPECT_NE(0, rc);
  EXPECT_EQ(0, add_task_stub_called);
  rc = cras_system_set_add_task_handler(add_task_stub, task_data);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(0, add_task_stub_called);
  rc = cras_system_add_task(task_stub, stub_data);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(1, add_task_stub_called);
  EXPECT_EQ(task_data, task_data_value);

  cras_system_state_deinit();
}

TEST(SystemSettingsStreamCount, StreamCount) {
  ResetStubData();
  do_sys_init();

  EXPECT_EQ(0, cras_system_state_get_active_streams());
  cras_system_state_stream_added(CRAS_STREAM_OUTPUT, CRAS_CLIENT_TYPE_CHROME);
  EXPECT_EQ(1, cras_system_state_get_active_streams());
  struct cras_timespec ts1;
  cras_system_state_get_last_stream_active_time(&ts1);
  cras_system_state_stream_removed(CRAS_STREAM_OUTPUT, CRAS_CLIENT_TYPE_CHROME);
  EXPECT_EQ(0, cras_system_state_get_active_streams());
  struct cras_timespec ts2;
  cras_system_state_get_last_stream_active_time(&ts2);
  EXPECT_NE(0, memcmp(&ts1, &ts2, sizeof(ts1)));
  cras_system_state_deinit();
}

TEST(SystemSettingsStreamCount, StreamCountByDirection) {
  ResetStubData();
  do_sys_init();

  EXPECT_EQ(0, cras_system_state_get_active_streams());
  cras_system_state_stream_added(CRAS_STREAM_OUTPUT, CRAS_CLIENT_TYPE_CHROME);
  cras_system_state_stream_added(CRAS_STREAM_INPUT, CRAS_CLIENT_TYPE_CHROME);
  cras_system_state_stream_added(CRAS_STREAM_POST_MIX_PRE_DSP,
                                 CRAS_CLIENT_TYPE_CHROME);
  EXPECT_EQ(1, cras_observer_notify_input_streams_with_permission_called);
  EXPECT_EQ(
      1, cras_system_state_get_active_streams_by_direction(CRAS_STREAM_OUTPUT));
  EXPECT_EQ(
      1, cras_system_state_get_active_streams_by_direction(CRAS_STREAM_INPUT));
  EXPECT_EQ(1, cras_system_state_get_active_streams_by_direction(
                   CRAS_STREAM_POST_MIX_PRE_DSP));
  EXPECT_EQ(3, cras_system_state_get_active_streams());
  EXPECT_EQ(3, cras_observer_notify_num_active_streams_called);
  cras_system_state_stream_removed(CRAS_STREAM_OUTPUT, CRAS_CLIENT_TYPE_CHROME);
  cras_system_state_stream_removed(CRAS_STREAM_INPUT, CRAS_CLIENT_TYPE_CHROME);
  cras_system_state_stream_removed(CRAS_STREAM_POST_MIX_PRE_DSP,
                                   CRAS_CLIENT_TYPE_CHROME);
  EXPECT_EQ(2, cras_observer_notify_input_streams_with_permission_called);
  EXPECT_EQ(
      0, cras_system_state_get_active_streams_by_direction(CRAS_STREAM_OUTPUT));
  EXPECT_EQ(
      0, cras_system_state_get_active_streams_by_direction(CRAS_STREAM_INPUT));
  EXPECT_EQ(0, cras_system_state_get_active_streams_by_direction(
                   CRAS_STREAM_POST_MIX_PRE_DSP));
  EXPECT_EQ(0, cras_system_state_get_active_streams());
  EXPECT_EQ(6, cras_observer_notify_num_active_streams_called);

  cras_system_state_deinit();
}

TEST(SystemStateSuite, IgnoreUCMSuffix) {
  fake_board_config.ucm_ignore_suffix = strdup("TEST1,TEST2,TEST3");
  do_sys_init();

  EXPECT_EQ(1, cras_system_check_ignore_ucm_suffix("TEST1"));
  EXPECT_EQ(1, cras_system_check_ignore_ucm_suffix("TEST2"));
  EXPECT_EQ(1, cras_system_check_ignore_ucm_suffix("TEST3"));
  EXPECT_EQ(0, cras_system_check_ignore_ucm_suffix("TEST4"));
  cras_system_state_deinit();
}

TEST(SystemStateSuite, SetNoiseCancellationEnabled) {
  ResetStubData();
  do_sys_init();

  EXPECT_EQ(0, cras_system_get_noise_cancellation_enabled());

  cras_system_set_noise_cancellation_enabled(0);
  EXPECT_EQ(0, cras_system_get_noise_cancellation_enabled());
  EXPECT_EQ(0, cras_iodev_list_reset_for_noise_cancellation_called);

  cras_system_set_noise_cancellation_enabled(1);
  EXPECT_EQ(1, cras_system_get_noise_cancellation_enabled());
  EXPECT_EQ(1, cras_iodev_list_reset_for_noise_cancellation_called);

  cras_system_set_noise_cancellation_enabled(1);
  EXPECT_EQ(1, cras_system_get_noise_cancellation_enabled());
  // cras_iodev_list_reset_for_noise_cancellation shouldn't be called if state
  // is already enabled/disabled.
  EXPECT_EQ(1, cras_iodev_list_reset_for_noise_cancellation_called);

  cras_system_set_noise_cancellation_enabled(0);
  EXPECT_EQ(0, cras_system_get_noise_cancellation_enabled());
  EXPECT_EQ(2, cras_iodev_list_reset_for_noise_cancellation_called);

  cras_system_state_deinit();
}

extern "C" {

struct cras_alsa_card* cras_alsa_card_create(
    struct cras_alsa_card_info* info,
    const char* device_config_dir,
    struct cras_device_blocklist* blocklist) {
  cras_alsa_card_create_called++;
  cras_alsa_card_config_dir = device_config_dir;
  return kFakeAlsaCard;
}

void cras_alsa_card_destroy(struct cras_alsa_card* alsa_card) {
  cras_alsa_card_destroy_called++;
}

size_t cras_alsa_card_get_index(const struct cras_alsa_card* alsa_card) {
  return 0;
}

struct cras_device_blocklist* cras_device_blocklist_create(
    const char* config_path) {
  return NULL;
}

void cras_device_blocklist_destroy(struct cras_device_blocklist* blocklist) {}

struct cras_alert* cras_alert_create(cras_alert_prepare prepare,
                                     unsigned int flags) {
  return NULL;
}

void cras_alert_destroy(struct cras_alert* alert) {}

int cras_alert_add_callback(struct cras_alert* alert,
                            cras_alert_cb cb,
                            void* arg) {
  add_callback_called++;
  add_callback_cb = cb;
  add_callback_arg = arg;
  return 0;
}

int cras_alert_rm_callback(struct cras_alert* alert,
                           cras_alert_cb cb,
                           void* arg) {
  rm_callback_called++;
  rm_callback_cb = cb;
  rm_callback_arg = arg;
  return 0;
}

void cras_alert_pending(struct cras_alert* alert) {
  alert_pending_called++;
}

cras_tm* cras_tm_init() {
  return static_cast<cras_tm*>(malloc(sizeof(unsigned int)));
}

void cras_tm_deinit(cras_tm* tm) {
  free(tm);
}

void cras_observer_notify_output_volume(int32_t volume) {
  cras_observer_notify_output_volume_called++;
}

void cras_observer_notify_output_mute(int muted,
                                      int user_muted,
                                      int mute_locked) {
  cras_observer_notify_output_mute_called++;
}

void cras_observer_notify_capture_mute(int muted, int mute_locked) {
  cras_observer_notify_capture_mute_called++;
}

void cras_observer_notify_suspend_changed(int suspended) {
  cras_observer_notify_suspend_changed_called++;
}

void cras_observer_notify_num_active_streams(enum CRAS_STREAM_DIRECTION dir,
                                             uint32_t num_active_streams) {
  cras_observer_notify_num_active_streams_called++;
}

void cras_observer_notify_input_streams_with_permission(
    uint32_t num_input_streams[CRAS_NUM_CLIENT_TYPE]) {
  cras_observer_notify_input_streams_with_permission_called++;
}

void cras_board_config_get(const char* config_path,
                           struct cras_board_config* board_config) {
  *board_config = fake_board_config;
}

void cras_alert_process_all_pending_alerts() {
  cras_alert_process_all_pending_alerts_called++;
}

void cras_iodev_list_reset_for_noise_cancellation() {
  cras_iodev_list_reset_for_noise_cancellation_called++;
}

}  // extern "C"
}  // namespace

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
