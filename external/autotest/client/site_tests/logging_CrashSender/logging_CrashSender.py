# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os, re
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.crash import crash_test


_SECONDS_SEND_SPREAD = 3600

class logging_CrashSender(crash_test.CrashTest):
    """
      End-to-end test of crash_sender.
    """
    version = 1


    def _check_hardware_info(self, result):
        # Get board name
        lsb_release = utils.read_file('/etc/lsb-release')
        board_match = re.search(r'CHROMEOS_RELEASE_BOARD=(.*)', lsb_release)
        if not ('Board: %s' % board_match.group(1)) in result['output']:
            raise error.TestFail('Missing board name %s in output' %
                                 board_match.group(1))
        # Get hwid
        with os.popen("crossystem hwid 2>/dev/null", "r") as hwid_proc:
            hwclass = hwid_proc.read()
        if not hwclass:
            hwclass = 'undefined'
        if not ('HWClass: %s' % hwclass) in result['output']:
            raise error.TestFail('Expected hwclass %s in output' % hwclass)


    def _check_send_result(self, result, report_kind, payload_name,
                           exec_name):
        if result['report_exists']:
            raise error.TestFail('Test report was not deleted after sending')
        if result['rate_count'] != 1:
            raise error.TestFail('Rate limit was not updated properly: #%d' %
                                 result['rate_count'])
        if not result['send_attempt']:
            raise error.TestFail('Sender did not attempt the send')
        if not result['send_success']:
            raise error.TestFail('Send did not complete successfully')
        if (result['sleep_time'] < 0 or
            result['sleep_time'] >= _SECONDS_SEND_SPREAD):
            raise error.TestFail('Sender did not sleep for an appropriate '
                                 'amount of time: #%d' % result['sleep_time'])
        if result['report_kind'] != report_kind:
            raise error.TestFail('Incorrect report kind "%s", expected "%s"',
                                 result['report_kind'], report_kind)
        desired_payload = self.get_crash_dir_name(payload_name)
        if result['report_payload'] != desired_payload:
            raise error.TestFail('Payload filename was incorrect, got "%s", '
                                 'expected "%s"', result['report_payload'],
                                 desired_payload)
        if result['exec_name'] != exec_name:
            raise error.TestFail('ExecName was incorrect, expected "%s", '
                                 'got "%s"', exec_name, result['exec_name'])


    def _test_sender_simple_minidump(self):
        """Test sending a single minidump crash report."""
        result = self._call_sender_one_crash()
        self._check_send_result(result, 'minidump',
                                '%s.dmp' % self._FAKE_TEST_BASENAME, 'fake')
        if (not 'Version: my_ver' in result['output']):
            raise error.TestFail('Simple minidump send failed')
        self._check_hardware_info(result)
        # Also test "Image type" field.  Note that it will not be "dev" even
        # on a dev build because crash-test-in-progress will exist.
        if result['image_type']:
            raise error.TestFail('Image type "%s" should not exist' %
                                 result['image_type'])
        # Also test "Boot mode" field.  Note that it will not be "dev" even
        # when booting in dev mode because crash-test-in-progress will exist.
        if result['boot_mode']:
            raise error.TestFail('Boot mode "%s" should not exist' %
                                 result['boot_mode'])


    def _test_sender_reports_disabled(self):
        """Test that when reporting is disabled, we don't send."""
        result = self._call_sender_one_crash(reports_enabled=False)
        if (result['report_exists'] or
            not 'Crash reporting is disabled' in result['output'] or
            result['send_attempt']):
            raise error.TestFail('Sender did not handle reports disabled')


    def run_once(self):
        """ Run all tests once """
        self.run_crash_tests([
            'sender_simple_minidump',
            'sender_reports_disabled']);
