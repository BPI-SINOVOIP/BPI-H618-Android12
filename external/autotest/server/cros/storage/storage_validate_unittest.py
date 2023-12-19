#!/usr/bin/env python2
# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import time
import unittest
import mock

import common
from autotest_lib.server.cros.storage import storage_validate
from autotest_lib.server.cros.device_health_profile import device_health_profile
from autotest_lib.server.cros.device_health_profile import profile_constants


class MockHostInfoStore(object):
    def __init__(self):
        self.board = 'mock_board'
        self.model = 'mock_model'


class MockHost(object):
    def __init__(self, hostname):
        self.hostname = hostname
        self.host_info_store = mock.Mock()
        self.host_info_store.get.return_value = MockHostInfoStore()
        self.job = None

    def check_cached_up_status(self):
        return True

    def is_up(self):
        return True

    def send_file(self, source, dest):
        return True

    def get_file(self, source, dest):
        return True

    def is_file_exists(self, file_path):
        return False


def create_device_health_profile():
    servohost = MockHost('dummy_servohost_hostname')
    dhp = device_health_profile.DeviceHealthProfile(
            hostname='dummy_dut_hostname',
            host_info=MockHostInfoStore(),
            result_dir=None)
    dhp.init_profile(servohost)
    return dhp


def _add_days_to_time(secs, days):
    new_time = time.localtime(secs + (days * 24 * 60 * 60))
    return time.strftime(profile_constants.TIME_PATTERN, new_time)


class BadblocksRunReadyTestCase(unittest.TestCase):
    dhp = create_device_health_profile()

    def test_is_time_to_run_badblocks_ro(self):
        self.dhp.refresh_badblocks_ro_run_time()
        last_time = self.dhp.get_badblocks_ro_run_time_epoch()
        # sleep for a second to make difference from now to avoid flakiness
        time.sleep(1)
        self.assertFalse(
                storage_validate._is_time_to_run_badblocks_ro(self.dhp))
        # set 5 days ago
        self.dhp._update_profile(
                profile_constants.LAST_BADBLOCKS_RO_RUN_TIME_KEY,
                _add_days_to_time(last_time, -5))
        self.assertFalse(
                storage_validate._is_time_to_run_badblocks_ro(self.dhp))
        # set 6 days ago
        self.dhp._update_profile(
                profile_constants.LAST_BADBLOCKS_RO_RUN_TIME_KEY,
                _add_days_to_time(last_time, -6))
        self.assertTrue(storage_validate._is_time_to_run_badblocks_ro(
                self.dhp))
        # set 7 days ago
        self.dhp._update_profile(
                profile_constants.LAST_BADBLOCKS_RO_RUN_TIME_KEY,
                _add_days_to_time(last_time, -7))
        self.assertTrue(storage_validate._is_time_to_run_badblocks_ro(
                self.dhp))

    def test_is_time_to_run_badblocks_rw(self):
        self.dhp.refresh_badblocks_rw_run_time()
        last_time = self.dhp.get_badblocks_rw_run_time_epoch()
        # sleep for a second to make difference from now to avoid flakiness
        time.sleep(1)
        self.assertFalse(
                storage_validate._is_time_to_run_badblocks_rw(self.dhp))
        # set 59 days ago
        self.dhp._update_profile(
                profile_constants.LAST_BADBLOCKS_RW_RUN_TIME_KEY,
                _add_days_to_time(last_time, -59))
        self.assertFalse(
                storage_validate._is_time_to_run_badblocks_rw(self.dhp))
        # set 60 days ago
        self.dhp._update_profile(
                profile_constants.LAST_BADBLOCKS_RW_RUN_TIME_KEY,
                _add_days_to_time(last_time, -60))
        self.assertTrue(storage_validate._is_time_to_run_badblocks_rw(
                self.dhp))
        # set 61 days ago
        self.dhp._update_profile(
                profile_constants.LAST_BADBLOCKS_RW_RUN_TIME_KEY,
                _add_days_to_time(last_time, -61))
        self.assertTrue(storage_validate._is_time_to_run_badblocks_rw(
                self.dhp))


if __name__ == '__main__':
    unittest.main()
