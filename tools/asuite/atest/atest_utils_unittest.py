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

"""Unittests for atest_utils."""

# pylint: disable=line-too-long

import hashlib
import os
import subprocess
import sys
import tempfile
import unittest

from io import StringIO
from unittest import mock

import atest_error
import atest_utils
import constants
import unittest_utils
import unittest_constants

from test_finders import test_info


TEST_MODULE_NAME_A = 'ModuleNameA'
TEST_RUNNER_A = 'FakeTestRunnerA'
TEST_BUILD_TARGET_A = set(['bt1', 'bt2'])
TEST_DATA_A = {'test_data_a_1': 'a1',
               'test_data_a_2': 'a2'}
TEST_SUITE_A = 'FakeSuiteA'
TEST_MODULE_CLASS_A = 'FAKE_MODULE_CLASS_A'
TEST_INSTALL_LOC_A = set(['host', 'device'])
TEST_FINDER_A = 'MODULE'
TEST_INFO_A = test_info.TestInfo(TEST_MODULE_NAME_A, TEST_RUNNER_A,
                                 TEST_BUILD_TARGET_A, TEST_DATA_A,
                                 TEST_SUITE_A, TEST_MODULE_CLASS_A,
                                 TEST_INSTALL_LOC_A)
TEST_INFO_A.test_finder = TEST_FINDER_A
TEST_ZIP_DATA_DIR = 'zip_files'
TEST_SINGLE_ZIP_NAME = 'single_file.zip'
TEST_MULTI_ZIP_NAME = 'multi_file.zip'

REPO_INFO_OUTPUT = '''Manifest branch: test_branch
Manifest merge branch: refs/heads/test_branch
Manifest groups: all,-notdefault
----------------------------
'''

#pylint: disable=protected-access
class AtestUtilsUnittests(unittest.TestCase):
    """Unit tests for atest_utils.py"""

    def test_capture_fail_section_has_fail_section(self):
        """Test capture_fail_section when has fail section."""
        test_list = ['AAAAAA', 'FAILED: Error1', '^\n', 'Error2\n',
                     '[  6% 191/2997] BBBBBB\n', 'CCCCC',
                     '[  20% 322/2997] DDDDDD\n', 'EEEEE']
        want_list = ['FAILED: Error1', '^\n', 'Error2\n']
        self.assertEqual(want_list,
                         atest_utils._capture_fail_section(test_list))

    def test_capture_fail_section_no_fail_section(self):
        """Test capture_fail_section when no fail section."""
        test_list = ['[ 6% 191/2997] XXXXX', 'YYYYY: ZZZZZ']
        want_list = []
        self.assertEqual(want_list,
                         atest_utils._capture_fail_section(test_list))

    def test_is_test_mapping(self):
        """Test method is_test_mapping."""
        tm_option_attributes = [
            'test_mapping',
            'include_subdirs'
        ]
        for attr_to_test in tm_option_attributes:
            args = mock.Mock()
            for attr in tm_option_attributes:
                setattr(args, attr, attr == attr_to_test)
            args.tests = []
            args.host_unit_test_only = False
            self.assertTrue(
                atest_utils.is_test_mapping(args),
                'Failed to validate option %s' % attr_to_test)

        args = mock.Mock()
        for attr in tm_option_attributes:
            setattr(args, attr, False)
        args.tests = []
        args.host_unit_test_only = True
        self.assertFalse(atest_utils.is_test_mapping(args))

        args = mock.Mock()
        for attr in tm_option_attributes:
            setattr(args, attr, False)
        args.tests = [':group_name']
        args.host_unit_test_only = False
        self.assertTrue(atest_utils.is_test_mapping(args))

        args = mock.Mock()
        for attr in tm_option_attributes:
            setattr(args, attr, False)
        args.tests = [':test1', 'test2']
        args.host_unit_test_only = False
        self.assertFalse(atest_utils.is_test_mapping(args))

        args = mock.Mock()
        for attr in tm_option_attributes:
            setattr(args, attr, False)
        args.tests = ['test2']
        args.host_unit_test_only = False
        self.assertFalse(atest_utils.is_test_mapping(args))

    @mock.patch('curses.tigetnum')
    def test_has_colors(self, mock_curses_tigetnum):
        """Test method _has_colors."""
        # stream is file I/O
        stream = open('/tmp/test_has_colors.txt', 'wb')
        self.assertFalse(atest_utils._has_colors(stream))
        stream.close()

        # stream is not a tty(terminal).
        stream = mock.Mock()
        stream.isatty.return_value = False
        self.assertFalse(atest_utils._has_colors(stream))

        # stream is a tty(terminal) and colors < 2.
        stream = mock.Mock()
        stream.isatty.return_value = True
        mock_curses_tigetnum.return_value = 1
        self.assertFalse(atest_utils._has_colors(stream))

        # stream is a tty(terminal) and colors > 2.
        stream = mock.Mock()
        stream.isatty.return_value = True
        mock_curses_tigetnum.return_value = 256
        self.assertTrue(atest_utils._has_colors(stream))


    @mock.patch('atest_utils._has_colors')
    def test_colorize(self, mock_has_colors):
        """Test method colorize."""
        original_str = "test string"
        green_no = 2

        # _has_colors() return False.
        mock_has_colors.return_value = False
        converted_str = atest_utils.colorize(original_str, green_no,
                                             highlight=True)
        self.assertEqual(original_str, converted_str)

        # Green with highlight.
        mock_has_colors.return_value = True
        converted_str = atest_utils.colorize(original_str, green_no,
                                             highlight=True)
        green_highlight_string = '\x1b[1;42m%s\x1b[0m' % original_str
        self.assertEqual(green_highlight_string, converted_str)

        # Green, no highlight.
        mock_has_colors.return_value = True
        converted_str = atest_utils.colorize(original_str, green_no,
                                             highlight=False)
        green_no_highlight_string = '\x1b[1;32m%s\x1b[0m' % original_str
        self.assertEqual(green_no_highlight_string, converted_str)


    @mock.patch('atest_utils._has_colors')
    def test_colorful_print(self, mock_has_colors):
        """Test method colorful_print."""
        testing_str = "color_print_test"
        green_no = 2

        # _has_colors() return False.
        mock_has_colors.return_value = False
        capture_output = StringIO()
        sys.stdout = capture_output
        atest_utils.colorful_print(testing_str, green_no, highlight=True,
                                   auto_wrap=False)
        sys.stdout = sys.__stdout__
        uncolored_string = testing_str
        self.assertEqual(capture_output.getvalue(), uncolored_string)

        # Green with highlight, but no wrap.
        mock_has_colors.return_value = True
        capture_output = StringIO()
        sys.stdout = capture_output
        atest_utils.colorful_print(testing_str, green_no, highlight=True,
                                   auto_wrap=False)
        sys.stdout = sys.__stdout__
        green_highlight_no_wrap_string = '\x1b[1;42m%s\x1b[0m' % testing_str
        self.assertEqual(capture_output.getvalue(),
                         green_highlight_no_wrap_string)

        # Green, no highlight, no wrap.
        mock_has_colors.return_value = True
        capture_output = StringIO()
        sys.stdout = capture_output
        atest_utils.colorful_print(testing_str, green_no, highlight=False,
                                   auto_wrap=False)
        sys.stdout = sys.__stdout__
        green_no_high_no_wrap_string = '\x1b[1;32m%s\x1b[0m' % testing_str
        self.assertEqual(capture_output.getvalue(),
                         green_no_high_no_wrap_string)

        # Green with highlight and wrap.
        mock_has_colors.return_value = True
        capture_output = StringIO()
        sys.stdout = capture_output
        atest_utils.colorful_print(testing_str, green_no, highlight=True,
                                   auto_wrap=True)
        sys.stdout = sys.__stdout__
        green_highlight_wrap_string = '\x1b[1;42m%s\x1b[0m\n' % testing_str
        self.assertEqual(capture_output.getvalue(), green_highlight_wrap_string)

        # Green with wrap, but no highlight.
        mock_has_colors.return_value = True
        capture_output = StringIO()
        sys.stdout = capture_output
        atest_utils.colorful_print(testing_str, green_no, highlight=False,
                                   auto_wrap=True)
        sys.stdout = sys.__stdout__
        green_wrap_no_highlight_string = '\x1b[1;32m%s\x1b[0m\n' % testing_str
        self.assertEqual(capture_output.getvalue(),
                         green_wrap_no_highlight_string)

    @mock.patch('socket.gethostname')
    @mock.patch('subprocess.check_output')
    def test_is_external_run(self, mock_output, mock_hostname):
        """Test method is_external_run."""
        mock_output.return_value = ''
        mock_hostname.return_value = ''
        self.assertTrue(atest_utils.is_external_run())

        mock_output.return_value = 'test@other.com'
        mock_hostname.return_value = 'abc.com'
        self.assertTrue(atest_utils.is_external_run())

        mock_output.return_value = 'test@other.com'
        mock_hostname.return_value = 'abc.google.com'
        self.assertFalse(atest_utils.is_external_run())

        mock_output.return_value = 'test@other.com'
        mock_hostname.return_value = 'abc.google.def.com'
        self.assertTrue(atest_utils.is_external_run())

        mock_output.return_value = 'test@google.com'
        self.assertFalse(atest_utils.is_external_run())

        mock_output.return_value = 'test@other.com'
        mock_hostname.return_value = 'c.googlers.com'
        self.assertFalse(atest_utils.is_external_run())

        mock_output.return_value = 'test@other.com'
        mock_hostname.return_value = 'a.googlers.com'
        self.assertTrue(atest_utils.is_external_run())

        mock_output.side_effect = OSError()
        self.assertTrue(atest_utils.is_external_run())

        mock_output.side_effect = subprocess.CalledProcessError(1, 'cmd')
        self.assertTrue(atest_utils.is_external_run())

    @mock.patch('metrics.metrics_base.get_user_type')
    def test_print_data_collection_notice(self, mock_get_user_type):
        """Test method print_data_collection_notice."""

        # get_user_type return 1(external).
        mock_get_user_type.return_value = 1
        notice_str = ('\n==================\nNotice:\n'
                      '  We collect anonymous usage statistics'
                      ' in accordance with our'
                      ' Content Licenses (https://source.android.com/setup/start/licenses),'
                      ' Contributor License Agreement (https://opensource.google.com/docs/cla/),'
                      ' Privacy Policy (https://policies.google.com/privacy) and'
                      ' Terms of Service (https://policies.google.com/terms).'
                      '\n==================\n\n')
        capture_output = StringIO()
        sys.stdout = capture_output
        atest_utils.print_data_collection_notice()
        sys.stdout = sys.__stdout__
        uncolored_string = notice_str
        self.assertEqual(capture_output.getvalue(), uncolored_string)

        # get_user_type return 0(internal).
        mock_get_user_type.return_value = 0
        notice_str = ('\n==================\nNotice:\n'
                      '  We collect usage statistics'
                      ' in accordance with our'
                      ' Content Licenses (https://source.android.com/setup/start/licenses),'
                      ' Contributor License Agreement (https://cla.developers.google.com/),'
                      ' Privacy Policy (https://policies.google.com/privacy) and'
                      ' Terms of Service (https://policies.google.com/terms).'
                      '\n==================\n\n')
        capture_output = StringIO()
        sys.stdout = capture_output
        atest_utils.print_data_collection_notice()
        sys.stdout = sys.__stdout__
        uncolored_string = notice_str
        self.assertEqual(capture_output.getvalue(), uncolored_string)

    @mock.patch('builtins.input')
    @mock.patch('json.load')
    def test_update_test_runner_cmd(self, mock_json_load_data, mock_input):
        """Test method handle_test_runner_cmd without enable do_verification."""
        former_cmd_str = 'Former cmds ='
        write_result_str = 'Save result mapping to test_result'
        tmp_file = tempfile.NamedTemporaryFile()
        input_cmd = 'atest_args'
        runner_cmds = ['cmd1', 'cmd2']
        capture_output = StringIO()
        sys.stdout = capture_output
        # Previous data is empty. Should not enter strtobool.
        # If entered, exception will be raised cause test fail.
        mock_json_load_data.return_value = {}
        atest_utils.handle_test_runner_cmd(input_cmd,
                                           runner_cmds,
                                           do_verification=False,
                                           result_path=tmp_file.name)
        sys.stdout = sys.__stdout__
        self.assertEqual(capture_output.getvalue().find(former_cmd_str), -1)
        # Previous data is the same as the new input. Should not enter strtobool.
        # If entered, exception will be raised cause test fail
        capture_output = StringIO()
        sys.stdout = capture_output
        mock_json_load_data.return_value = {input_cmd:runner_cmds}
        atest_utils.handle_test_runner_cmd(input_cmd,
                                           runner_cmds,
                                           do_verification=False,
                                           result_path=tmp_file.name)
        sys.stdout = sys.__stdout__
        self.assertEqual(capture_output.getvalue().find(former_cmd_str), -1)
        self.assertEqual(capture_output.getvalue().find(write_result_str), -1)
        # Previous data has different cmds. Should enter strtobool not update,
        # should not find write_result_str.
        prev_cmds = ['cmd1']
        mock_input.return_value = 'n'
        capture_output = StringIO()
        sys.stdout = capture_output
        mock_json_load_data.return_value = {input_cmd:prev_cmds}
        atest_utils.handle_test_runner_cmd(input_cmd,
                                           runner_cmds,
                                           do_verification=False,
                                           result_path=tmp_file.name)
        sys.stdout = sys.__stdout__
        self.assertEqual(capture_output.getvalue().find(write_result_str), -1)

    @mock.patch('json.load')
    def test_verify_test_runner_cmd(self, mock_json_load_data):
        """Test method handle_test_runner_cmd without enable update_result."""
        tmp_file = tempfile.NamedTemporaryFile()
        input_cmd = 'atest_args'
        runner_cmds = ['cmd1', 'cmd2']
        # Previous data is the same as the new input. Should not raise exception.
        mock_json_load_data.return_value = {input_cmd:runner_cmds}
        atest_utils.handle_test_runner_cmd(input_cmd,
                                           runner_cmds,
                                           do_verification=True,
                                           result_path=tmp_file.name)
        # Previous data has different cmds. Should enter strtobool and hit
        # exception.
        prev_cmds = ['cmd1']
        mock_json_load_data.return_value = {input_cmd:prev_cmds}
        self.assertRaises(atest_error.DryRunVerificationError,
                          atest_utils.handle_test_runner_cmd,
                          input_cmd,
                          runner_cmds,
                          do_verification=True,
                          result_path=tmp_file.name)

    def test_get_test_info_cache_path(self):
        """Test method get_test_info_cache_path."""
        input_file_name = 'mytest_name'
        cache_root = '/a/b/c'
        expect_hashed_name = ('%s.cache' % hashlib.md5(str(input_file_name).
                                                       encode()).hexdigest())
        self.assertEqual(os.path.join(cache_root, expect_hashed_name),
                         atest_utils.get_test_info_cache_path(input_file_name,
                                                              cache_root))

    def test_get_and_load_cache(self):
        """Test method update_test_info_cache and load_test_info_cache."""
        test_reference = 'myTestRefA'
        test_cache_dir = tempfile.mkdtemp()
        atest_utils.update_test_info_cache(test_reference, [TEST_INFO_A],
                                           test_cache_dir)
        unittest_utils.assert_equal_testinfo_sets(
            self, set([TEST_INFO_A]),
            atest_utils.load_test_info_cache(test_reference, test_cache_dir))

    @mock.patch('os.getcwd')
    def test_get_build_cmd(self, mock_cwd):
        """Test method get_build_cmd."""
        build_top = '/home/a/b/c'
        rel_path = 'd/e'
        mock_cwd.return_value = os.path.join(build_top, rel_path)
        os_environ_mock = {constants.ANDROID_BUILD_TOP: build_top}
        with mock.patch.dict('os.environ', os_environ_mock, clear=True):
            expected_cmd = ['../../build/soong/soong_ui.bash', '--make-mode']
            self.assertEqual(expected_cmd, atest_utils.get_build_cmd())

    @mock.patch('subprocess.check_output')
    def test_get_modified_files(self, mock_co):
        """Test method get_modified_files"""
        mock_co.side_effect = [
            x.encode('utf-8') for x in ['/a/b/',
                                        '\n',
                                        'test_fp1.java\nc/test_fp2.java']]
        self.assertEqual({'/a/b/test_fp1.java', '/a/b/c/test_fp2.java'},
                         atest_utils.get_modified_files(''))
        mock_co.side_effect = [
            x.encode('utf-8') for x in ['/a/b/',
                                        'test_fp4',
                                        '/test_fp3.java']]
        self.assertEqual({'/a/b/test_fp4', '/a/b/test_fp3.java'},
                         atest_utils.get_modified_files(''))

    def test_delimiter(self):
        """Test method delimiter"""
        self.assertEqual('\n===\n\n', atest_utils.delimiter('=', 3, 1, 2))

    def test_has_python_module(self):
        """Test method has_python_module"""
        self.assertFalse(atest_utils.has_python_module('M_M'))
        self.assertTrue(atest_utils.has_python_module('os'))

    @mock.patch.object(atest_utils, 'matched_tf_error_log', return_value=True)
    def test_read_zip_single_text(self, _matched):
        """Test method extract_zip_text include only one text file."""
        zip_path = os.path.join(unittest_constants.TEST_DATA_DIR,
                                TEST_ZIP_DATA_DIR, TEST_SINGLE_ZIP_NAME)
        expect_content = '\nfile1_line1\nfile1_line2\n'
        self.assertEqual(expect_content, atest_utils.extract_zip_text(zip_path))

    @mock.patch.object(atest_utils, 'matched_tf_error_log', return_value=True)
    def test_read_zip_multi_text(self, _matched):
        """Test method extract_zip_text include multiple text files."""
        zip_path = os.path.join(unittest_constants.TEST_DATA_DIR,
                                TEST_ZIP_DATA_DIR, TEST_MULTI_ZIP_NAME)
        expect_content = ('\nfile1_line1\nfile1_line2\n\nfile2_line1\n'
                          'file2_line2\n')
        self.assertEqual(expect_content, atest_utils.extract_zip_text(zip_path))

    def test_matched_tf_error_log(self):
        """Test method extract_zip_text include multiple text files."""
        matched_content = '05-25 17:37:04 E/XXXXX YYYYY'
        not_matched_content = '05-25 17:37:04 I/XXXXX YYYYY'
        # Test matched content
        self.assertEqual(True,
                         atest_utils.matched_tf_error_log(matched_content))
        # Test not matched content
        self.assertEqual(False,
                         atest_utils.matched_tf_error_log(not_matched_content))

    @mock.patch('os.chmod')
    @mock.patch('shutil.copy2')
    @mock.patch('atest_utils.has_valid_cert')
    @mock.patch('subprocess.check_output')
    @mock.patch('os.path.exists')
    def test_get_flakes(self, mock_path_exists, mock_output, mock_valid_cert,
                        _cpc, _cm):
        """Test method get_flakes."""
        # Test par file does not exist.
        mock_path_exists.return_value = False
        self.assertEqual(None, atest_utils.get_flakes())
        # Test par file exists.
        mock_path_exists.return_value = True
        mock_output.return_value = (b'flake_percent:0.10001\n'
                                    b'postsubmit_flakes_per_week:12.0')
        mock_valid_cert.return_value = True
        expected_flake_info = {'flake_percent':'0.10001',
                               'postsubmit_flakes_per_week':'12.0'}
        self.assertEqual(expected_flake_info,
                         atest_utils.get_flakes())
        # Test no valid cert
        mock_valid_cert.return_value = False
        self.assertEqual(None,
                         atest_utils.get_flakes())

    @mock.patch('subprocess.check_call')
    def test_has_valid_cert(self, mock_call):
        """Test method has_valid_cert."""
        # raise subprocess.CalledProcessError
        mock_call.raiseError.side_effect = subprocess.CalledProcessError
        self.assertFalse(atest_utils.has_valid_cert())
        with mock.patch("constants.CERT_STATUS_CMD", ''):
            self.assertFalse(atest_utils.has_valid_cert())
        with mock.patch("constants.CERT_STATUS_CMD", 'CMD'):
            # has valid cert
            mock_call.return_value = 0
            self.assertTrue(atest_utils.has_valid_cert())
            # no valid cert
            mock_call.return_value = 4
            self.assertFalse(atest_utils.has_valid_cert())

    # pylint: disable=no-member
    def test_read_test_record_proto(self):
        """Test method read_test_record."""
        test_record_file_path = os.path.join(unittest_constants.TEST_DATA_DIR,
                                             "test_record.proto.testonly")
        test_record = atest_utils.read_test_record(test_record_file_path)
        self.assertEqual(test_record.children[0].inline_test_record.test_record_id,
                         'x86 hello_world_test')

    def test_is_valid_json_file_file_not_exist(self):
        """Test method is_valid_json_file if file not exist."""
        json_file_path = os.path.join(unittest_constants.TEST_DATA_DIR,
                                      "not_exist.json")
        self.assertFalse(atest_utils.is_valid_json_file(json_file_path))

    def test_is_valid_json_file_content_valid(self):
        """Test method is_valid_json_file if file exist and content is valid."""
        json_file_path = os.path.join(unittest_constants.TEST_DATA_DIR,
                                      "module-info.json")
        self.assertTrue(atest_utils.is_valid_json_file(json_file_path))

    def test_is_valid_json_file_content_not_valid(self):
        """Test method is_valid_json_file if file exist but content is valid."""
        json_file_path = os.path.join(unittest_constants.TEST_DATA_DIR,
                                      "not-valid-module-info.json")
        self.assertFalse(atest_utils.is_valid_json_file(json_file_path))

    @mock.patch('subprocess.check_output')
    @mock.patch('os.getenv')
    def test_get_manifest_branch(self, mock_env, mock_check_output):
        """Test method get_manifest_branch"""
        mock_env.return_value = 'any_path'
        mock_check_output.return_value = REPO_INFO_OUTPUT
        self.assertEqual('test_branch', atest_utils.get_manifest_branch())

        mock_env.return_value = 'any_path'
        mock_check_output.return_value = 'not_matched_branch_pattern.'
        self.assertEqual(None, atest_utils.get_manifest_branch())

        mock_env.return_value = 'any_path'
        mock_check_output.side_effect = subprocess.CalledProcessError(
            1,
            'repo info')
        self.assertEqual(None, atest_utils.get_manifest_branch())

        mock_env.return_value = None
        mock_check_output.return_value = REPO_INFO_OUTPUT
        self.assertEqual(None, atest_utils.get_manifest_branch())

    def test_has_wildcard(self):
        """Test method of has_wildcard"""
        self.assertFalse(atest_utils.has_wildcard('test1'))
        self.assertFalse(atest_utils.has_wildcard(['test1']))
        self.assertTrue(atest_utils.has_wildcard('test1?'))
        self.assertTrue(atest_utils.has_wildcard(['test1', 'b*', 'a?b*']))

    # pylint: disable=anomalous-backslash-in-string
    def test_quote(self):
        """Test method of quote()"""
        target_str = r'TEST_(F|P)[0-9].*\w$'
        expected_str = '\'TEST_(F|P)[0-9].*\w$\''
        self.assertEqual(atest_utils.quote(target_str), expected_str)
        self.assertEqual(atest_utils.quote('TEST_P224'), 'TEST_P224')

    @mock.patch('builtins.input', return_value='')
    def test_prompt_with_yn_result(self, mock_input):
        """Test method of prompt_with_yn_result"""
        msg = 'Do you want to continue?'
        mock_input.return_value = ''
        self.assertTrue(atest_utils.prompt_with_yn_result(msg, True))
        self.assertFalse(atest_utils.prompt_with_yn_result(msg, False))
        mock_input.return_value = 'y'
        self.assertTrue(atest_utils.prompt_with_yn_result(msg, True))
        mock_input.return_value = 'nO'
        self.assertFalse(atest_utils.prompt_with_yn_result(msg, True))

    def test_get_android_junit_config_filters(self):
        """Test method of get_android_junit_config_filters"""
        no_filter_test_config = os.path.join(
            unittest_constants.TEST_DATA_DIR,
            "filter_configs", "no_filter.cfg")
        self.assertEqual({},
                         atest_utils.get_android_junit_config_filters(
                             no_filter_test_config))

        filtered_test_config = os.path.join(
            unittest_constants.TEST_DATA_DIR,
            'filter_configs', 'filter.cfg')
        filter_dict = atest_utils.get_android_junit_config_filters(
            filtered_test_config)
        include_annotations = filter_dict.get(constants.INCLUDE_ANNOTATION)
        include_annotations.sort()
        self.assertEqual(
            ['include1', 'include2'],
            include_annotations)
        exclude_annotation = filter_dict.get(constants.EXCLUDE_ANNOTATION)
        exclude_annotation.sort()
        self.assertEqual(
            ['exclude1', 'exclude2'],
            exclude_annotation)

    def test_md5sum(self):
        """Test method of md5sum"""
        exist_string = os.path.join(unittest_constants.TEST_DATA_DIR,
                                    unittest_constants.JSON_FILE)
        inexist_string = os.path.join(unittest_constants.TEST_DATA_DIR,
                                      unittest_constants.CLASS_NAME)
        self.assertEqual(
            atest_utils.md5sum(exist_string), 'c26aab9baae99bcfb97633b69e9ceefd')
        self.assertEqual(
            atest_utils.md5sum(inexist_string), '')

    def test_check_md5(self):
        """Test method of check_md5"""
        file1 = os.path.join(unittest_constants.TEST_DATA_DIR,
                            unittest_constants.JSON_FILE)
        checksum_file = '/tmp/_tmp_module-info.json'
        atest_utils.save_md5([file1], '/tmp/_tmp_module-info.json')
        self.assertTrue(atest_utils.check_md5(checksum_file))
        os.remove(checksum_file)
        self.assertFalse(atest_utils.check_md5(checksum_file))
        self.assertTrue(atest_utils.check_md5(checksum_file, missing_ok=True))

    def test_get_config_parameter(self):
        """Test method of get_config_parameter"""
        parameter_config = os.path.join(
            unittest_constants.TEST_DATA_DIR,
            "parameter_config", "parameter.cfg")
        no_parameter_config = os.path.join(
            unittest_constants.TEST_DATA_DIR,
            "parameter_config", "no_parameter.cfg")

        # Test parameter empty value
        self.assertEqual(set(),
                         atest_utils.get_config_parameter(
                             no_parameter_config))

        # Test parameter empty value
        self.assertEqual({'value_1', 'value_2', 'value_3', 'value_4'},
                         atest_utils.get_config_parameter(
                             parameter_config))

if __name__ == "__main__":
    unittest.main()
