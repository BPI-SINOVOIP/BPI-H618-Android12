#!/usr/bin/env python3
#
# Copyright 2018, The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Unittests for atest_tf_test_runner."""

# pylint: disable=line-too-long

import os
import sys
import tempfile
import unittest
import json

from io import StringIO
from unittest import mock

import atest_utils
import constants
import unittest_constants as uc
import unittest_utils

from logstorage import atest_gcp_utils
from test_finders import test_finder_utils
from test_finders import test_info
from test_runners import event_handler
from test_runners import atest_tf_test_runner as atf_tr

#pylint: disable=protected-access
#pylint: disable=invalid-name
METRICS_DIR = '%s/baseline-metrics' % uc.TEST_INFO_DIR
METRICS_DIR_ARG = '--metrics-folder %s ' % METRICS_DIR
# TODO(147567606): Replace {serial} with {extra_args} for general extra
# arguments testing.
RUN_CMD_ARGS = ('{metrics}--log-level-display VERBOSE --log-level VERBOSE'
                '{device_early_release}{serial}')
LOG_ARGS = atf_tr.AtestTradefedTestRunner._LOG_ARGS.format(
    log_path=os.path.join(uc.TEST_INFO_DIR, atf_tr.LOG_FOLDER_NAME),
    proto_path=os.path.join(uc.TEST_INFO_DIR, constants.ATEST_TEST_RECORD_PROTO))
RUN_ENV_STR = 'tf_env_var=test'
RUN_CMD = atf_tr.AtestTradefedTestRunner._RUN_CMD.format(
    env=RUN_ENV_STR,
    exe=atf_tr.AtestTradefedTestRunner.EXECUTABLE,
    template=atf_tr.AtestTradefedTestRunner._TF_TEMPLATE,
    tf_customize_template='{tf_customize_template}',
    args=RUN_CMD_ARGS,
    log_args=LOG_ARGS)
FULL_CLASS2_NAME = 'android.jank.cts.ui.SomeOtherClass'
CLASS2_FILTER = test_info.TestFilter(FULL_CLASS2_NAME, frozenset())
METHOD2_FILTER = test_info.TestFilter(uc.FULL_CLASS_NAME, frozenset([uc.METHOD2_NAME]))
MODULE_ARG1 = [(constants.TF_INCLUDE_FILTER_OPTION, "A"),
               (constants.TF_INCLUDE_FILTER_OPTION, "B")]
MODULE_ARG2 = []
CLASS2_METHOD_FILTER = test_info.TestFilter(FULL_CLASS2_NAME,
                                            frozenset([uc.METHOD_NAME, uc.METHOD2_NAME]))
MODULE2_INFO = test_info.TestInfo(uc.MODULE2_NAME,
                                  atf_tr.AtestTradefedTestRunner.NAME,
                                  set(),
                                  data={constants.TI_REL_CONFIG: uc.CONFIG2_FILE,
                                        constants.TI_FILTER: frozenset()})
CLASS1_BUILD_TARGETS = {'class_1_build_target'}
CLASS1_INFO = test_info.TestInfo(uc.MODULE_NAME,
                                 atf_tr.AtestTradefedTestRunner.NAME,
                                 CLASS1_BUILD_TARGETS,
                                 data={constants.TI_REL_CONFIG: uc.CONFIG_FILE,
                                       constants.TI_FILTER: frozenset([uc.CLASS_FILTER])})
CLASS2_BUILD_TARGETS = {'class_2_build_target'}
CLASS2_INFO = test_info.TestInfo(uc.MODULE_NAME,
                                 atf_tr.AtestTradefedTestRunner.NAME,
                                 CLASS2_BUILD_TARGETS,
                                 data={constants.TI_REL_CONFIG: uc.CONFIG_FILE,
                                       constants.TI_FILTER: frozenset([CLASS2_FILTER])})
CLASS3_BUILD_TARGETS = {'class_3_build_target'}
CLASS3_INFO = test_info.TestInfo(uc.MODULE_NAME,
                                 atf_tr.AtestTradefedTestRunner.NAME,
                                 CLASS3_BUILD_TARGETS,
                                 data={constants.TI_REL_CONFIG: uc.CONFIG_FILE,
                                       constants.TI_FILTER: frozenset(),
                                       constants.TI_MODULE_ARG: MODULE_ARG1})
CLASS4_BUILD_TARGETS = {'class_4_build_target'}
CLASS4_INFO = test_info.TestInfo(uc.MODULE_NAME,
                                 atf_tr.AtestTradefedTestRunner.NAME,
                                 CLASS4_BUILD_TARGETS,
                                 data={constants.TI_REL_CONFIG: uc.CONFIG_FILE,
                                       constants.TI_FILTER: frozenset(),
                                       constants.TI_MODULE_ARG: MODULE_ARG2})
CLASS1_CLASS2_MODULE_INFO = test_info.TestInfo(
    uc.MODULE_NAME,
    atf_tr.AtestTradefedTestRunner.NAME,
    uc.MODULE_BUILD_TARGETS | CLASS1_BUILD_TARGETS | CLASS2_BUILD_TARGETS,
    uc.MODULE_DATA)
FLAT_CLASS_INFO = test_info.TestInfo(
    uc.MODULE_NAME,
    atf_tr.AtestTradefedTestRunner.NAME,
    CLASS1_BUILD_TARGETS | CLASS2_BUILD_TARGETS,
    data={constants.TI_REL_CONFIG: uc.CONFIG_FILE,
          constants.TI_FILTER: frozenset([uc.CLASS_FILTER, CLASS2_FILTER])})
FLAT2_CLASS_INFO = test_info.TestInfo(
    uc.MODULE_NAME,
    atf_tr.AtestTradefedTestRunner.NAME,
    CLASS3_BUILD_TARGETS | CLASS4_BUILD_TARGETS,
    data={constants.TI_REL_CONFIG: uc.CONFIG_FILE,
          constants.TI_FILTER: frozenset(),
          constants.TI_MODULE_ARG: MODULE_ARG1 + MODULE_ARG2})
GTF_INT_CONFIG = os.path.join(uc.GTF_INT_DIR, uc.GTF_INT_NAME + '.xml')
CLASS2_METHOD_INFO = test_info.TestInfo(
    uc.MODULE_NAME,
    atf_tr.AtestTradefedTestRunner.NAME,
    set(),
    data={constants.TI_REL_CONFIG: uc.CONFIG_FILE,
          constants.TI_FILTER:
              frozenset([test_info.TestFilter(
                  FULL_CLASS2_NAME, frozenset([uc.METHOD_NAME, uc.METHOD2_NAME]))])})
METHOD_AND_CLASS2_METHOD = test_info.TestInfo(
    uc.MODULE_NAME,
    atf_tr.AtestTradefedTestRunner.NAME,
    uc.MODULE_BUILD_TARGETS,
    data={constants.TI_REL_CONFIG: uc.CONFIG_FILE,
          constants.TI_FILTER: frozenset([uc.METHOD_FILTER, CLASS2_METHOD_FILTER])})
METHOD_METHOD2_AND_CLASS2_METHOD = test_info.TestInfo(
    uc.MODULE_NAME,
    atf_tr.AtestTradefedTestRunner.NAME,
    uc.MODULE_BUILD_TARGETS,
    data={constants.TI_REL_CONFIG: uc.CONFIG_FILE,
          constants.TI_FILTER: frozenset([uc.FLAT_METHOD_FILTER, CLASS2_METHOD_FILTER])})
METHOD2_INFO = test_info.TestInfo(
    uc.MODULE_NAME,
    atf_tr.AtestTradefedTestRunner.NAME,
    set(),
    data={constants.TI_REL_CONFIG: uc.CONFIG_FILE,
          constants.TI_FILTER: frozenset([METHOD2_FILTER])})

INT_INFO = test_info.TestInfo(
    uc.INT_NAME,
    atf_tr.AtestTradefedTestRunner.NAME,
    set(),
    test_finder='INTEGRATION')

MOD_INFO = test_info.TestInfo(
    uc.MODULE_NAME,
    atf_tr.AtestTradefedTestRunner.NAME,
    set(),
    test_finder='MODULE')

MOD_INFO_NO_TEST_FINDER = test_info.TestInfo(
    uc.MODULE_NAME,
    atf_tr.AtestTradefedTestRunner.NAME,
    set())

EVENTS_NORMAL = [
    ('TEST_MODULE_STARTED', {
        'moduleContextFileName':'serial-util1146216{974}2772610436.ser',
        'moduleName':'someTestModule'}),
    ('TEST_RUN_STARTED', {'testCount': 2}),
    ('TEST_STARTED', {'start_time':52, 'className':'someClassName',
                      'testName':'someTestName'}),
    ('TEST_ENDED', {'end_time':1048, 'className':'someClassName',
                    'testName':'someTestName'}),
    ('TEST_STARTED', {'start_time':48, 'className':'someClassName2',
                      'testName':'someTestName2'}),
    ('TEST_FAILED', {'className':'someClassName2', 'testName':'someTestName2',
                     'trace': 'someTrace'}),
    ('TEST_ENDED', {'end_time':9876450, 'className':'someClassName2',
                    'testName':'someTestName2'}),
    ('TEST_RUN_ENDED', {}),
    ('TEST_MODULE_ENDED', {'foo': 'bar'}),
]

class AtestTradefedTestRunnerUnittests(unittest.TestCase):
    """Unit tests for atest_tf_test_runner.py"""

    #pylint: disable=arguments-differ
    @mock.patch.object(atf_tr.AtestTradefedTestRunner, '_get_ld_library_path')
    @mock.patch.dict('os.environ', {constants.ANDROID_BUILD_TOP:'/'})
    def setUp(self, mock_get_ld_library_path):
        mock_get_ld_library_path.return_value = RUN_ENV_STR
        self.tr = atf_tr.AtestTradefedTestRunner(results_dir=uc.TEST_INFO_DIR)

    def tearDown(self):
        mock.patch.stopall()

    @mock.patch.object(atf_tr.AtestTradefedTestRunner,
                       '_start_socket_server')
    @mock.patch.object(atf_tr.AtestTradefedTestRunner,
                       'run')
    @mock.patch.object(atf_tr.AtestTradefedTestRunner,
                       '_create_test_args', return_value=['some_args'])
    @mock.patch.object(atf_tr.AtestTradefedTestRunner,
                       'generate_run_commands', return_value='some_cmd')
    @mock.patch.object(atf_tr.AtestTradefedTestRunner,
                       '_process_connection', return_value=None)
    @mock.patch('select.select')
    @mock.patch('os.killpg', return_value=None)
    @mock.patch('os.getpgid', return_value=None)
    @mock.patch('signal.signal', return_value=None)
    def test_run_tests_pretty(self, _signal, _pgid, _killpg, mock_select,
                              _process, _run_cmd, _test_args,
                              mock_run, mock_start_socket_server):
        """Test _run_tests_pretty method."""
        mock_subproc = mock.Mock()
        mock_run.return_value = mock_subproc
        mock_subproc.returncode = 0
        mock_subproc.poll.side_effect = [True, True, None]
        mock_server = mock.Mock()
        mock_server.getsockname.return_value = ('', '')
        mock_start_socket_server.return_value = mock_server
        mock_reporter = mock.Mock()

        # Test no early TF exit
        mock_conn = mock.Mock()
        mock_server.accept.return_value = (mock_conn, 'some_addr')
        mock_server.close.return_value = True
        mock_select.side_effect = [([mock_server], None, None),
                                   ([mock_conn], None, None)]
        self.tr.run_tests_pretty([MODULE2_INFO], {}, mock_reporter)

        # Test early TF exit
        tmp_file = tempfile.NamedTemporaryFile()
        with open(tmp_file.name, 'w') as f:
            f.write("tf msg")
        self.tr.test_log_file = tmp_file
        mock_select.side_effect = [([], None, None)]
        mock_subproc.poll.side_effect = None
        capture_output = StringIO()
        sys.stdout = capture_output
        self.assertRaises(atf_tr.TradeFedExitError, self.tr.run_tests_pretty,
                          [MODULE2_INFO], {}, mock_reporter)
        sys.stdout = sys.__stdout__
        self.assertTrue('tf msg' in capture_output.getvalue())

    @mock.patch.object(atf_tr.AtestTradefedTestRunner, '_process_connection')
    @mock.patch('select.select')
    def test_start_monitor_2_connection(self, mock_select, mock_process):
        """Test _start_monitor method."""
        mock_server = mock.Mock()
        mock_subproc = mock.Mock()
        mock_reporter = mock.Mock()
        mock_conn1 = mock.Mock()
        mock_conn2 = mock.Mock()
        mock_server.accept.side_effect = [(mock_conn1, 'addr 1'),
                                          (mock_conn2, 'addr 2')]
        mock_select.side_effect = [([mock_server], None, None),
                                   ([mock_server], None, None),
                                   ([mock_conn1], None, None),
                                   ([mock_conn2], None, None),
                                   ([mock_conn1], None, None),
                                   ([mock_conn2], None, None)]
        mock_process.side_effect = ['abc', 'def', False, False]
        mock_subproc.poll.side_effect = [None, None, None, None,
                                         None, True]
        self.tr._start_monitor(mock_server, mock_subproc, mock_reporter, {})
        self.assertEqual(mock_process.call_count, 4)
        calls = [mock.call.accept(), mock.call.close()]
        mock_server.assert_has_calls(calls)
        mock_conn1.assert_has_calls([mock.call.close()])
        mock_conn2.assert_has_calls([mock.call.close()])

    @mock.patch.object(atf_tr.AtestTradefedTestRunner, '_process_connection')
    @mock.patch('select.select')
    def test_start_monitor_tf_exit_before_2nd_connection(self,
                                                         mock_select,
                                                         mock_process):
        """Test _start_monitor method."""
        mock_server = mock.Mock()
        mock_subproc = mock.Mock()
        mock_reporter = mock.Mock()
        mock_conn1 = mock.Mock()
        mock_conn2 = mock.Mock()
        mock_server.accept.side_effect = [(mock_conn1, 'addr 1'),
                                          (mock_conn2, 'addr 2')]
        mock_select.side_effect = [([mock_server], None, None),
                                   ([mock_server], None, None),
                                   ([mock_conn1], None, None),
                                   ([mock_conn2], None, None),
                                   ([mock_conn1], None, None),
                                   ([mock_conn2], None, None)]
        mock_process.side_effect = ['abc', 'def', False, False]
        # TF exit early but have not processed data in socket buffer.
        mock_subproc.poll.side_effect = [None, None, True, True,
                                         True, True]
        self.tr._start_monitor(mock_server, mock_subproc, mock_reporter, {})
        self.assertEqual(mock_process.call_count, 4)
        calls = [mock.call.accept(), mock.call.close()]
        mock_server.assert_has_calls(calls)
        mock_conn1.assert_has_calls([mock.call.close()])
        mock_conn2.assert_has_calls([mock.call.close()])


    def test_start_socket_server(self):
        """Test start_socket_server method."""
        server = self.tr._start_socket_server()
        host, port = server.getsockname()
        self.assertEqual(host, atf_tr.SOCKET_HOST)
        self.assertLessEqual(port, 65535)
        self.assertGreaterEqual(port, 1024)
        server.close()

    @mock.patch('os.path.exists')
    @mock.patch.dict('os.environ', {'APE_API_KEY':'/tmp/123.json'})
    def test_try_set_gts_authentication_key_is_set_by_user(self, mock_exist):
        """Test try_set_authentication_key_is_set_by_user method."""
        # Test key is set by user.
        self.tr._try_set_gts_authentication_key()
        mock_exist.assert_not_called()

    @mock.patch('os.path.join', return_value='/tmp/file_not_exist.json')
    def test_try_set_gts_authentication_key_not_set(self, _):
        """Test try_set_authentication_key_not_set method."""
        # Delete the environment variable if it's set. This is fine for this
        # method because it's for validating the APE_API_KEY isn't set.
        if os.environ.get('APE_API_KEY'):
            del os.environ['APE_API_KEY']
        self.tr._try_set_gts_authentication_key()
        self.assertEqual(os.environ.get('APE_API_KEY'), None)

    @mock.patch.object(event_handler.EventHandler, 'process_event')
    def test_process_connection(self, mock_pe):
        """Test _process_connection method."""
        mock_socket = mock.Mock()
        for name, data in EVENTS_NORMAL:
            data_map = {mock_socket: ''}
            socket_data = '%s %s' % (name, json.dumps(data))
            mock_socket.recv.return_value = socket_data
            self.tr._process_connection(data_map, mock_socket, mock_pe)

        calls = [mock.call.process_event(name, data) for name, data in EVENTS_NORMAL]
        mock_pe.assert_has_calls(calls)
        mock_socket.recv.return_value = ''
        self.assertFalse(self.tr._process_connection(data_map, mock_socket, mock_pe))

    @mock.patch.object(event_handler.EventHandler, 'process_event')
    def test_process_connection_multiple_lines_in_single_recv(self, mock_pe):
        """Test _process_connection when recv reads multiple lines in one go."""
        mock_socket = mock.Mock()
        squashed_events = '\n'.join(['%s %s' % (name, json.dumps(data))
                                     for name, data in EVENTS_NORMAL])
        socket_data = [squashed_events, '']
        mock_socket.recv.side_effect = socket_data
        data_map = {mock_socket: ''}
        self.tr._process_connection(data_map, mock_socket, mock_pe)
        calls = [mock.call.process_event(name, data) for name, data in EVENTS_NORMAL]
        mock_pe.assert_has_calls(calls)

    @mock.patch.object(event_handler.EventHandler, 'process_event')
    def test_process_connection_with_buffering(self, mock_pe):
        """Test _process_connection when events overflow socket buffer size."""
        mock_socket = mock.Mock()
        module_events = [EVENTS_NORMAL[0], EVENTS_NORMAL[-1]]
        socket_events = ['%s %s' % (name, json.dumps(data))
                         for name, data in module_events]
        # test try-block code by breaking apart first event after first }
        index = socket_events[0].index('}') + 1
        socket_data = [socket_events[0][:index], socket_events[0][index:]]
        # test non-try block buffering with second event
        socket_data.extend([socket_events[1][:-4], socket_events[1][-4:], ''])
        mock_socket.recv.side_effect = socket_data
        data_map = {mock_socket: ''}
        self.tr._process_connection(data_map, mock_socket, mock_pe)
        self.tr._process_connection(data_map, mock_socket, mock_pe)
        self.tr._process_connection(data_map, mock_socket, mock_pe)
        self.tr._process_connection(data_map, mock_socket, mock_pe)
        calls = [mock.call.process_event(name, data) for name, data in module_events]
        mock_pe.assert_has_calls(calls)

    @mock.patch.object(event_handler.EventHandler, 'process_event')
    def test_process_connection_with_not_completed_event_data(self, mock_pe):
        """Test _process_connection when event have \n prefix."""
        mock_socket = mock.Mock()
        mock_socket.recv.return_value = ('\n%s %s'
                                         %(EVENTS_NORMAL[0][0],
                                           json.dumps(EVENTS_NORMAL[0][1])))
        data_map = {mock_socket: ''}
        self.tr._process_connection(data_map, mock_socket, mock_pe)
        calls = [mock.call.process_event(EVENTS_NORMAL[0][0],
                                         EVENTS_NORMAL[0][1])]
        mock_pe.assert_has_calls(calls)

    @mock.patch('os.environ.get', return_value=None)
    @mock.patch.object(atf_tr.AtestTradefedTestRunner, '_generate_metrics_folder')
    @mock.patch('atest_utils.get_result_server_args')
    def test_generate_run_commands_without_serial_env(self, mock_resultargs, mock_mertrics, _):
        """Test generate_run_command method."""
        # Basic Run Cmd
        mock_resultargs.return_value = []
        mock_mertrics.return_value = ''
        unittest_utils.assert_strict_equal(
            self,
            self.tr.generate_run_commands([], {}),
            [RUN_CMD.format(env=RUN_ENV_STR,
                            metrics='',
                            serial='',
                            tf_customize_template='',
                            device_early_release=' --no-early-device-release')])
        mock_mertrics.return_value = METRICS_DIR
        unittest_utils.assert_strict_equal(
            self,
            self.tr.generate_run_commands([], {}),
            [RUN_CMD.format(metrics=METRICS_DIR_ARG,
                            serial='',
                            tf_customize_template='',
                            device_early_release=' --no-early-device-release')])
        # Run cmd with result server args.
        result_arg = '--result_arg'
        mock_resultargs.return_value = [result_arg]
        mock_mertrics.return_value = ''
        unittest_utils.assert_strict_equal(
            self,
            self.tr.generate_run_commands(
                [], {}),
            [RUN_CMD.format(metrics='',
                            serial='',
                            tf_customize_template='',
                            device_early_release=' --no-early-device-release') + ' ' + result_arg])

    @mock.patch('os.environ.get')
    @mock.patch.object(atf_tr.AtestTradefedTestRunner, '_generate_metrics_folder')
    @mock.patch('atest_utils.get_result_server_args')
    def test_generate_run_commands_with_serial_env(self, mock_resultargs, mock_mertrics, mock_env):
        """Test generate_run_command method."""
        # Basic Run Cmd
        env_device_serial = 'env-device-0'
        mock_resultargs.return_value = []
        mock_mertrics.return_value = ''
        mock_env.return_value = env_device_serial
        env_serial_arg = ' --serial %s' % env_device_serial
        # Serial env be set and without --serial arg.
        unittest_utils.assert_strict_equal(
            self,
            self.tr.generate_run_commands([], {}),
            [RUN_CMD.format(metrics='',
                            serial=env_serial_arg,
                            tf_customize_template='',
                            device_early_release=' --no-early-device-release')])
        # Serial env be set but with --serial arg.
        arg_device_serial = 'arg-device-0'
        arg_serial_arg = ' --serial %s' % arg_device_serial
        unittest_utils.assert_strict_equal(
            self,
            self.tr.generate_run_commands([], {constants.SERIAL:arg_device_serial}),
            [RUN_CMD.format(metrics='',
                            serial=arg_serial_arg,
                            tf_customize_template='',
                            device_early_release=' --no-early-device-release')])
        # Serial env be set but with -n arg
        unittest_utils.assert_strict_equal(
            self,
            self.tr.generate_run_commands([], {constants.HOST: True}),
            [RUN_CMD.format(metrics='',
                            serial='',
                            tf_customize_template='',
                            device_early_release=' --no-early-device-release') +
             ' -n --prioritize-host-config --skip-host-arch-check'])


    def test_flatten_test_filters(self):
        """Test _flatten_test_filters method."""
        # No Flattening
        filters = self.tr._flatten_test_filters({uc.CLASS_FILTER})
        unittest_utils.assert_strict_equal(self, frozenset([uc.CLASS_FILTER]),
                                           filters)
        filters = self.tr._flatten_test_filters({CLASS2_FILTER})
        unittest_utils.assert_strict_equal(
            self, frozenset([CLASS2_FILTER]), filters)
        filters = self.tr._flatten_test_filters({uc.METHOD_FILTER})
        unittest_utils.assert_strict_equal(
            self, frozenset([uc.METHOD_FILTER]), filters)
        filters = self.tr._flatten_test_filters({uc.METHOD_FILTER,
                                                 CLASS2_METHOD_FILTER})
        unittest_utils.assert_strict_equal(
            self, frozenset([uc.METHOD_FILTER, CLASS2_METHOD_FILTER]), filters)
        # Flattening
        filters = self.tr._flatten_test_filters({uc.METHOD_FILTER,
                                                 METHOD2_FILTER})
        unittest_utils.assert_strict_equal(
            self, filters, frozenset([uc.FLAT_METHOD_FILTER]))
        filters = self.tr._flatten_test_filters({uc.METHOD_FILTER,
                                                 METHOD2_FILTER,
                                                 CLASS2_METHOD_FILTER,})
        unittest_utils.assert_strict_equal(
            self, filters, frozenset([uc.FLAT_METHOD_FILTER,
                                      CLASS2_METHOD_FILTER]))

    def test_flatten_test_infos(self):
        """Test _flatten_test_infos method."""
        # No Flattening
        test_infos = self.tr._flatten_test_infos({uc.MODULE_INFO})
        unittest_utils.assert_equal_testinfo_sets(self, test_infos,
                                                  {uc.MODULE_INFO})

        test_infos = self.tr._flatten_test_infos([uc.MODULE_INFO, MODULE2_INFO])
        unittest_utils.assert_equal_testinfo_sets(
            self, test_infos, {uc.MODULE_INFO, MODULE2_INFO})

        test_infos = self.tr._flatten_test_infos({CLASS1_INFO})
        unittest_utils.assert_equal_testinfo_sets(self, test_infos,
                                                  {CLASS1_INFO})

        test_infos = self.tr._flatten_test_infos({uc.INT_INFO})
        unittest_utils.assert_equal_testinfo_sets(self, test_infos,
                                                  {uc.INT_INFO})

        test_infos = self.tr._flatten_test_infos({uc.METHOD_INFO})
        unittest_utils.assert_equal_testinfo_sets(self, test_infos,
                                                  {uc.METHOD_INFO})

        # Flattening
        test_infos = self.tr._flatten_test_infos({CLASS1_INFO, CLASS2_INFO})
        unittest_utils.assert_equal_testinfo_sets(self, test_infos,
                                                  {FLAT_CLASS_INFO})

        test_infos = self.tr._flatten_test_infos({CLASS1_INFO, uc.INT_INFO,
                                                  CLASS2_INFO})
        unittest_utils.assert_equal_testinfo_sets(self, test_infos,
                                                  {uc.INT_INFO,
                                                   FLAT_CLASS_INFO})

        test_infos = self.tr._flatten_test_infos({CLASS1_INFO, uc.MODULE_INFO,
                                                  CLASS2_INFO})
        unittest_utils.assert_equal_testinfo_sets(self, test_infos,
                                                  {CLASS1_CLASS2_MODULE_INFO})

        test_infos = self.tr._flatten_test_infos({MODULE2_INFO, uc.INT_INFO,
                                                  CLASS1_INFO, CLASS2_INFO,
                                                  uc.GTF_INT_INFO})
        unittest_utils.assert_equal_testinfo_sets(self, test_infos,
                                                  {uc.INT_INFO, uc.GTF_INT_INFO,
                                                   FLAT_CLASS_INFO,
                                                   MODULE2_INFO})

        test_infos = self.tr._flatten_test_infos({uc.METHOD_INFO,
                                                  CLASS2_METHOD_INFO})
        unittest_utils.assert_equal_testinfo_sets(self, test_infos,
                                                  {METHOD_AND_CLASS2_METHOD})

        test_infos = self.tr._flatten_test_infos({uc.METHOD_INFO, METHOD2_INFO,
                                                  CLASS2_METHOD_INFO})
        unittest_utils.assert_equal_testinfo_sets(
            self, test_infos, {METHOD_METHOD2_AND_CLASS2_METHOD})
        test_infos = self.tr._flatten_test_infos({uc.METHOD_INFO, METHOD2_INFO,
                                                  CLASS2_METHOD_INFO,
                                                  MODULE2_INFO,
                                                  uc.INT_INFO})
        unittest_utils.assert_equal_testinfo_sets(
            self, test_infos, {uc.INT_INFO, MODULE2_INFO,
                               METHOD_METHOD2_AND_CLASS2_METHOD})

        test_infos = self.tr._flatten_test_infos({CLASS3_INFO, CLASS4_INFO})
        unittest_utils.assert_equal_testinfo_sets(self, test_infos,
                                                  {FLAT2_CLASS_INFO})

    @mock.patch.object(test_finder_utils, 'get_test_config_and_srcs')
    def test_create_test_args(self, mock_config):
        """Test _create_test_args method."""
        # Only compile '--skip-loading-config-jar' in TF if it's not
        # INTEGRATION finder or the finder property isn't set.
        mock_config.return_value = '', ''
        args = self.tr._create_test_args([MOD_INFO])
        self.assertTrue(constants.TF_SKIP_LOADING_CONFIG_JAR in args)

        args = self.tr._create_test_args([INT_INFO])
        self.assertFalse(constants.TF_SKIP_LOADING_CONFIG_JAR in args)

        args = self.tr._create_test_args([MOD_INFO_NO_TEST_FINDER])
        self.assertFalse(constants.TF_SKIP_LOADING_CONFIG_JAR in args)

        args = self.tr._create_test_args([MOD_INFO_NO_TEST_FINDER, INT_INFO])
        self.assertFalse(constants.TF_SKIP_LOADING_CONFIG_JAR in args)

        args = self.tr._create_test_args([MOD_INFO_NO_TEST_FINDER])
        self.assertFalse(constants.TF_SKIP_LOADING_CONFIG_JAR in args)

        args = self.tr._create_test_args([MOD_INFO_NO_TEST_FINDER, INT_INFO, MOD_INFO])
        self.assertFalse(constants.TF_SKIP_LOADING_CONFIG_JAR in args)

    @mock.patch('os.environ.get', return_value=None)
    @mock.patch.object(atf_tr.AtestTradefedTestRunner, '_generate_metrics_folder')
    @mock.patch('atest_utils.get_result_server_args')
    def test_generate_run_commands_collect_tests_only(self,
                                                      mock_resultargs,
                                                      mock_mertrics, _):
        """Test generate_run_command method."""
        # Testing  without collect-tests-only
        mock_resultargs.return_value = []
        mock_mertrics.return_value = ''
        extra_args = {}
        unittest_utils.assert_strict_equal(
            self,
            self.tr.generate_run_commands([], extra_args),
            [RUN_CMD.format(
                metrics='',
                serial='',
                tf_customize_template='',
                device_early_release=' --no-early-device-release')])
        # Testing  with collect-tests-only
        mock_resultargs.return_value = []
        mock_mertrics.return_value = ''
        extra_args = {constants.COLLECT_TESTS_ONLY: True}
        unittest_utils.assert_strict_equal(
            self,
            self.tr.generate_run_commands([], extra_args),
            [RUN_CMD.format(
                metrics='',
                serial=' --collect-tests-only',
                tf_customize_template='',
                device_early_release=' --no-early-device-release')])


    @mock.patch('os.environ.get', return_value=None)
    @mock.patch.object(atf_tr.AtestTradefedTestRunner, '_generate_metrics_folder')
    @mock.patch('atest_utils.get_result_server_args')
    def test_generate_run_commands_with_tf_template(self, mock_resultargs, mock_mertrics, _):
        """Test generate_run_command method."""
        tf_tmplate_key1 = 'tf_tmplate_key1'
        tf_tmplate_val1 = 'tf_tmplate_val1'
        tf_tmplate_key2 = 'tf_tmplate_key2'
        tf_tmplate_val2 = 'tf_tmplate_val2'
        # Testing with only one tradefed template command
        mock_resultargs.return_value = []
        mock_mertrics.return_value = ''
        extra_args = {constants.TF_TEMPLATE:
                          ['{}={}'.format(tf_tmplate_key1,
                                          tf_tmplate_val1)]}
        unittest_utils.assert_strict_equal(
            self,
            self.tr.generate_run_commands([], extra_args),
            [RUN_CMD.format(
                metrics='',
                serial='',
                device_early_release=' --no-early-device-release',
                tf_customize_template=
                '--template:map {}={}').format(tf_tmplate_key1,
                                               tf_tmplate_val1)])
        # Testing with two tradefed template commands
        extra_args = {constants.TF_TEMPLATE:
                          ['{}={}'.format(tf_tmplate_key1,
                                          tf_tmplate_val1),
                           '{}={}'.format(tf_tmplate_key2,
                                          tf_tmplate_val2)]}
        unittest_utils.assert_strict_equal(
            self,
            self.tr.generate_run_commands([], extra_args),
            [RUN_CMD.format(
                metrics='',
                serial='',
                device_early_release=' --no-early-device-release',
                tf_customize_template=
                '--template:map {}={} --template:map {}={}').format(
                    tf_tmplate_key1,
                    tf_tmplate_val1,
                    tf_tmplate_key2,
                    tf_tmplate_val2)])

    @mock.patch.object(atest_gcp_utils.GCPHelper, 'get_credential_with_auth_flow')
    @mock.patch('builtins.input')
    def test_request_consent_of_upload_test_result_yes(self,
                                                       mock_input,
                                                       mock_get_credential_with_auth_flow):
        """test request_consent_of_upload_test_result method."""
        constants.CREDENTIAL_FILE_NAME = 'cred_file'
        constants.GCP_ACCESS_TOKEN = 'access_token'
        tmp_folder = tempfile.mkdtemp()
        mock_input.return_value = 'Y'
        not_upload_file = os.path.join(tmp_folder,
                                       constants.DO_NOT_UPLOAD)

        self.tr._request_consent_of_upload_test_result(tmp_folder, True)
        self.assertEqual(1, mock_get_credential_with_auth_flow.call_count)
        self.assertFalse(os.path.exists(not_upload_file))

        self.tr._request_consent_of_upload_test_result(tmp_folder, True)
        self.assertEqual(2, mock_get_credential_with_auth_flow.call_count)
        self.assertFalse(os.path.exists(not_upload_file))

    @mock.patch.object(atest_gcp_utils.GCPHelper, 'get_credential_with_auth_flow')
    @mock.patch('builtins.input')
    def test_request_consent_of_upload_test_result_no(self,
                                                      mock_input,
                                                      mock_get_credential_with_auth_flow):
        """test request_consent_of_upload_test_result method."""
        mock_input.return_value = 'N'
        constants.CREDENTIAL_FILE_NAME = 'cred_file'
        constants.GCP_ACCESS_TOKEN = 'access_token'
        tmp_folder = tempfile.mkdtemp()
        not_upload_file = os.path.join(tmp_folder,
                                       constants.DO_NOT_UPLOAD)

        self.tr._request_consent_of_upload_test_result(tmp_folder, True)
        self.assertTrue(os.path.exists(not_upload_file))
        self.assertEqual(0, mock_get_credential_with_auth_flow.call_count)
        self.tr._request_consent_of_upload_test_result(tmp_folder, True)
        self.assertEqual(0, mock_get_credential_with_auth_flow.call_count)

    @mock.patch('os.environ.get', return_value=None)
    @mock.patch.object(atf_tr.AtestTradefedTestRunner, '_generate_metrics_folder')
    @mock.patch('atest_utils.get_result_server_args')
    def test_generate_run_commands_with_tf_early_device_release(
            self, mock_resultargs, mock_mertrics, _):
        """Test generate_run_command method."""
        # Testing  without collect-tests-only
        mock_resultargs.return_value = []
        mock_mertrics.return_value = ''
        extra_args = {constants.TF_EARLY_DEVICE_RELEASE: True}
        unittest_utils.assert_strict_equal(
            self,
            self.tr.generate_run_commands([], extra_args),
            [RUN_CMD.format(
                metrics='',
                serial='',
                tf_customize_template='',
                device_early_release='')])

    @mock.patch.object(atf_tr.AtestTradefedTestRunner, '_prepare_data')
    @mock.patch.object(atf_tr.AtestTradefedTestRunner, '_request_consent_of_upload_test_result')
    def test_do_upload_flow(self, mock_request, mock_prepare):
        """test _do_upload_flow method."""
        fake_extra_args = {}
        fake_creds = mock.Mock()
        fake_creds.token_response = {'access_token': 'fake_token'}
        mock_request.return_value = fake_creds
        fake_inv = {'invocationId': 'inv_id'}
        fake_workunit = {'id': 'workunit_id'}
        mock_prepare.return_value = fake_inv, fake_workunit
        constants.TOKEN_FILE_PATH = tempfile.NamedTemporaryFile().name
        creds, inv = self.tr._do_upload_flow(fake_extra_args)
        self.assertEqual(fake_creds, creds)
        self.assertEqual(fake_inv, inv)
        self.assertEqual(fake_extra_args[constants.INVOCATION_ID],
                         fake_inv['invocationId'])
        self.assertEqual(fake_extra_args[constants.WORKUNIT_ID],
                         fake_workunit['id'])

        mock_request.return_value = None
        creds, inv = self.tr._do_upload_flow(fake_extra_args)
        self.assertEqual(None, creds)
        self.assertEqual(None, inv)

    @mock.patch.object(test_finder_utils, 'get_test_config_and_srcs')
    def test_has_instant_app_config(self, mock_config):
        """test _has_instant_app_config method."""
        no_instant_config = os.path.join(
            uc.TEST_DATA_DIR, "parameter_config", "parameter.cfg")
        instant_config = os.path.join(
            uc.TEST_DATA_DIR, "parameter_config", "instant_app_parameter.cfg")
        # Test find instant app config
        mock_config.return_value = instant_config, ''
        self.assertTrue(
            atf_tr.AtestTradefedTestRunner._has_instant_app_config(
                ['test_info'], 'module_info_obj'))
        # Test not find instant app config
        mock_config.return_value = no_instant_config, ''
        self.assertFalse(
            atf_tr.AtestTradefedTestRunner._has_instant_app_config(
                ['test_info'], 'module_info_obj'))

    @mock.patch.object(atf_tr.AtestTradefedTestRunner,
                       '_has_instant_app_config', return_value=True)
    @mock.patch('os.environ.get', return_value=None)
    @mock.patch.object(atf_tr.AtestTradefedTestRunner,
                       '_generate_metrics_folder')
    @mock.patch('atest_utils.get_result_server_args')
    def test_generate_run_commands_has_instant_app_config(
        self, mock_resultargs, mock_mertrics, _, _mock_has_config):
        """Test generate_run_command method which has instant app config."""
        # Basic Run Cmd
        mock_resultargs.return_value = []
        mock_mertrics.return_value = ''
        extra_tf_arg = (
            '{tf_test_arg} {tf_class}:{option_name}:{option_value}'.format(
            tf_test_arg = constants.TF_TEST_ARG,
            tf_class=constants.TF_AND_JUNIT_CLASS,
            option_name=constants.TF_EXCLUDE_ANNOTATE,
            option_value=constants.INSTANT_MODE_ANNOTATE))
        unittest_utils.assert_strict_equal(
            self,
            self.tr.generate_run_commands([], {}),
            [RUN_CMD.format(env=RUN_ENV_STR,
                            metrics='',
                            serial='',
                            tf_customize_template='',
                            device_early_release=' --no-early-device-release '
                                                 + extra_tf_arg)])

    @mock.patch.object(atest_utils, 'get_config_parameter')
    @mock.patch.object(test_finder_utils, 'get_test_config_and_srcs')
    def test_is_parameter_auto_enabled_cfg(self, mock_config, mock_cfg_para):
        """test _is_parameter_auto_enabled_cfg method."""
        # Test if TF_PARA_INSTANT_APP is match
        mock_config.return_value = 'test_config', ''
        mock_cfg_para.return_value = {list(constants.DEFAULT_EXCLUDE_PARAS)[1],
                                      list(constants.DEFAULT_EXCLUDE_PARAS)[0]}
        self.assertFalse(
            atf_tr.AtestTradefedTestRunner._is_parameter_auto_enabled_cfg(
                ['test_info'], 'module_info_obj'))
        # Test if DEFAULT_EXCLUDE_NOT_PARAS is match
        mock_cfg_para.return_value = {
            list(constants.DEFAULT_EXCLUDE_NOT_PARAS)[2],
            list(constants.DEFAULT_EXCLUDE_NOT_PARAS)[0]}
        self.assertFalse(
            atf_tr.AtestTradefedTestRunner._is_parameter_auto_enabled_cfg(
                ['test_info'], 'module_info_obj'))
        # Test if have parameter not in default exclude paras
        mock_cfg_para.return_value = {
            'not match parameter',
            list(constants.DEFAULT_EXCLUDE_PARAS)[1],
            list(constants.DEFAULT_EXCLUDE_NOT_PARAS)[2]}
        self.assertTrue(
            atf_tr.AtestTradefedTestRunner._is_parameter_auto_enabled_cfg(
                ['test_info'], 'module_info_obj'))

    @mock.patch.object(atf_tr.AtestTradefedTestRunner,
                       '_is_parameter_auto_enabled_cfg',
                       return_value=True)
    @mock.patch.object(test_finder_utils, 'get_test_config_and_srcs')
    def test_create_test_args_with_auto_enable_parameter(
        self, mock_config, _mock_is_enable):
        """Test _create_test_args method with auto enabled parameter config."""
        # Should have --m on args and should not have --include-filter.
        mock_config.return_value = '', ''
        args = self.tr._create_test_args([MOD_INFO])
        self.assertTrue(constants.TF_MODULE_FILTER in args)
        self.assertFalse(constants.TF_INCLUDE_FILTER in args)

    @mock.patch.object(atf_tr.AtestTradefedTestRunner,
                       '_is_parameter_auto_enabled_cfg')
    @mock.patch.object(test_finder_utils, 'get_test_config_and_srcs')
    def test_parse_extra_args(self, mock_config, _mock_is_enable):
        """Test _parse_extra_args ."""
        # If extra_arg enable instant_app or secondary users, should not have
        # --exclude-module-rameters even though test config parameter is auto
        # enabled.
        mock_config.return_value = '', ''
        _mock_is_enable.return_value = True
        args, _ = self.tr._parse_extra_args([MOD_INFO], [constants.INSTANT])
        self.assertFalse('--exclude-module-parameters' in args)

        # If extra_arg not enable instant_app or secondary users, should have
        # --exclude-module-rameters if config parameter is auto enabled.
        _mock_is_enable.return_value = True
        args, _ = self.tr._parse_extra_args([MOD_INFO], [constants.ALL_ABI])
        self.assertTrue('--exclude-module-parameters' in args)

        # If extra_arg not enable instant_app or secondary users, should not
        # have --exclude-module-rameters if config parameter is not auto enabled
        _mock_is_enable.return_value = False
        args, _ = self.tr._parse_extra_args([MOD_INFO], [constants.ALL_ABI])
        self.assertFalse('--exclude-module-parameters' in args)

if __name__ == '__main__':
    unittest.main()
