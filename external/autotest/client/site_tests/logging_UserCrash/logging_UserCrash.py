# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, time
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import cros_ui, upstart
from autotest_lib.client.cros.crash import user_crash_test


_COLLECTION_ERROR_SIGNATURE = 'crash_reporter-user-collection'
_MAX_CRASH_DIRECTORY_SIZE = 32
_CRASH_REPORTER_ENABLED_PATH = '/var/lib/crash_reporter/crash-handling-enabled'


class logging_UserCrash(user_crash_test.UserCrashTest):
    """Verifies crash reporting for user processes."""
    version = 1


    def _get_uptime(self):
        with open('/proc/uptime', 'r') as f:
            uptime_seconds = float(f.readline().split()[0])

        return uptime_seconds


    # This test has a critical tast counterpart, but the tast version only
    # performs one of the two functions that this test does. In particular,
    # the tast variant does not verify that crash reporter state is valid before
    # any tests run and re-initialize crash reporter.
    # TODO(https://crbug.com/1085194): Write a tast test to verify that crash
    # reporter's state is good on a "clean" system.
    def _test_reporter_startup(self):
        """Test that the core_pattern is set up by crash reporter."""
        # Turn off crash filtering so we see the original setting.
        self.disable_crash_filtering()
        output = utils.read_file(self._CORE_PATTERN).rstrip()
        expected_core_pattern = ('|%s --user=%%P:%%s:%%u:%%g:%%f' %
                                 self._CRASH_REPORTER_PATH)
        if output != expected_core_pattern:
            raise error.TestFail('core pattern should have been %s, not %s' %
                                 (expected_core_pattern, output))

        # Check that we wrote out the file indicating that crash_reporter is
        # enabled AFTER the system was booted. This replaces the old technique
        # of looking for the log message which was flakey when the logs got
        # flooded.
        # NOTE: This technique doesn't need to be highly accurate, we are only
        # verifying that the flag was written after boot and there are multiple
        # seconds between those steps, and a file from a prior boot will almost
        # always have been written out much further back in time than our
        # current boot time.
        if not os.path.isfile(_CRASH_REPORTER_ENABLED_PATH):
            raise error.TestFail(
                'crash reporter enabled file flag is not present at %s' %
                _CRASH_REPORTER_ENABLED_PATH)
        flag_time = time.time() - os.path.getmtime(_CRASH_REPORTER_ENABLED_PATH)
        uptime = self._get_uptime()
        if (flag_time > uptime):
            raise error.TestFail(
                'user space crash handling was not started during last boot')


    def _test_chronos_crasher(self):
        """Test a user space crash when running as chronos is handled."""
        self._check_crashing_process(
                'chronos',
                extra_meta_contents='upload_var_in_progress_integration_test='
                'logging_UserCrash')


    def _test_chronos_crasher_no_consent(self):
        """Test that without consent no files are stored."""
        results = self._check_crashing_process('chronos', consent=False)


    def _test_root_crasher(self):
        """Test a user space crash when running as root is handled."""
        self._check_crashing_process('root')


    def _test_root_crasher_no_consent(self):
        """Test that without consent no files are stored."""
        results = self._check_crashing_process('root', consent=False)


    def _test_max_enqueued_crashes(self):
        """Test that _MAX_CRASH_DIRECTORY_SIZE is enforced."""
        self._log_reader.set_start_by_current()
        username = 'root'

        crash_dir = self._get_crash_dir(username)
        full_message = ('Crash directory %s already full with %d pending '
                        'reports' % (crash_dir, _MAX_CRASH_DIRECTORY_SIZE))

        # Fill up the queue.
        for i in range(0, _MAX_CRASH_DIRECTORY_SIZE):
            result = self._run_crasher_process(username)
            if not result['crashed']:
                raise error.TestFail('failure while setting up queue: %d' %
                                     result['returncode'])
            if self._log_reader.can_find(full_message):
                raise error.TestFail('unexpected full message: ' +
                                     full_message)

        crash_dir_size = len(os.listdir(crash_dir))
        # For debugging
        utils.system('ls -l %s' % crash_dir)
        logging.info('Crash directory had %d entries', crash_dir_size)

        # Crash a bunch more times, but make sure no new reports
        # are enqueued.
        for i in range(0, 10):
            self._log_reader.set_start_by_current()
            result = self._run_crasher_process(username)
            logging.info('New log messages: %s', self._log_reader.get_logs())
            if not result['crashed']:
                raise error.TestFail('failure after setting up queue: %d' %
                                     result['returncode'])
            utils.poll_for_condition(
                    lambda: self._log_reader.can_find(full_message),
                    timeout=20,
                    exception=error.TestFail('expected full message: ' +
                                             full_message))
            if crash_dir_size != len(os.listdir(crash_dir)):
                utils.system('ls -l %s' % crash_dir)
                raise error.TestFail('expected no new files (now %d were %d)',
                                     len(os.listdir(crash_dir)),
                                     crash_dir_size)


    def initialize(self):
        user_crash_test.UserCrashTest.initialize(self)

        # If the device has a GUI, return the device to the sign-in screen, as
        # some tests will fail inside a user session.
        if upstart.has_service('ui'):
            cros_ui.restart()


    # TODO(kmixter): Test crashing a process as ntp or some other
    # non-root, non-chronos user.

    def run_once(self):
        """ Run all tests once """
        self._prepare_crasher()
        self._populate_symbols()

        # Run the test once without re-initializing
        # to catch problems with the default crash reporting setup
        self.run_crash_tests(['reporter_startup'],
                              initialize_crash_reporter=False,
                              must_run_all=False)

        self.run_crash_tests(['reporter_startup',
                              'chronos_crasher',
                              'chronos_crasher_no_consent',
                              'root_crasher',
                              'root_crasher_no_consent',
                              'max_enqueued_crashes'],
                              initialize_crash_reporter=True)
