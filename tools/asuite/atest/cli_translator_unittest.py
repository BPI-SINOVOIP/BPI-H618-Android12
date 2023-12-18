#!/usr/bin/env python3
#
# Copyright 2017, The Android Open Source Project
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

"""Unittests for cli_translator."""

# pylint: disable=line-too-long

import unittest
import json
import os
import re
import sys

from importlib import reload
from io import StringIO
from unittest import mock

import cli_translator as cli_t
import constants
import module_info
import test_finder_handler
import test_mapping
import unittest_constants as uc
import unittest_utils

from metrics import metrics
from test_finders import module_finder
from test_finders import test_finder_base
from test_finders import test_finder_utils


# TEST_MAPPING related consts
TEST_MAPPING_TOP_DIR = os.path.join(uc.TEST_DATA_DIR, 'test_mapping')
TEST_MAPPING_DIR = os.path.join(TEST_MAPPING_TOP_DIR, 'folder1')
TEST_1 = test_mapping.TestDetail({'name': 'test1', 'host': True})
TEST_2 = test_mapping.TestDetail({'name': 'test2'})
TEST_3 = test_mapping.TestDetail({'name': 'test3'})
TEST_4 = test_mapping.TestDetail({'name': 'test4'})
TEST_5 = test_mapping.TestDetail({'name': 'test5'})
TEST_6 = test_mapping.TestDetail({'name': 'test6'})
TEST_7 = test_mapping.TestDetail({'name': 'test7'})
TEST_8 = test_mapping.TestDetail({'name': 'test8'})
TEST_9 = test_mapping.TestDetail({'name': 'test9'})
TEST_10 = test_mapping.TestDetail({'name': 'test10'})

SEARCH_DIR_RE = re.compile(r'^find ([^ ]*).*$')


#pylint: disable=unused-argument
def gettestinfos_side_effect(test_names, test_mapping_test_details=None,
                             is_rebuild_module_info=False):
    """Mock return values for _get_test_info."""
    test_infos = set()
    for test_name in test_names:
        if test_name == uc.MODULE_NAME:
            test_infos.add(uc.MODULE_INFO)
        if test_name == uc.CLASS_NAME:
            test_infos.add(uc.CLASS_INFO)
        if test_name == uc.HOST_UNIT_TEST_NAME_1:
            test_infos.add(uc.MODULE_INFO_HOST_1)
        if test_name == uc.HOST_UNIT_TEST_NAME_2:
            test_infos.add(uc.MODULE_INFO_HOST_2)
    return test_infos


#pylint: disable=protected-access
#pylint: disable=no-self-use
class CLITranslatorUnittests(unittest.TestCase):
    """Unit tests for cli_t.py"""

    def setUp(self):
        """Run before execution of every test"""
        self.ctr = cli_t.CLITranslator()

        # Create a mock of args.
        self.args = mock.Mock
        self.args.tests = []
        # Test mapping related args
        self.args.test_mapping = False
        self.args.include_subdirs = False
        self.args.enable_file_patterns = False
        self.args.rebuild_module_info = False
        # Cache finder related args
        self.args.clear_cache = False
        self.ctr.mod_info = mock.Mock
        self.ctr.mod_info.name_to_module_info = {}

    def tearDown(self):
        """Run after execution of every test"""
        reload(uc)

    @mock.patch('builtins.input', return_value='n')
    @mock.patch.object(module_finder.ModuleFinder, 'find_test_by_module_name')
    @mock.patch.object(module_finder.ModuleFinder, 'get_fuzzy_searching_results')
    @mock.patch.object(metrics, 'FindTestFinishEvent')
    @mock.patch.object(test_finder_handler, 'get_find_methods_for_test')
    # pylint: disable=too-many-locals
    def test_get_test_infos(self, mock_getfindmethods, _metrics,
                            mock_getfuzzyresults, mock_findtestbymodule,
                            mock_input):
        """Test _get_test_infos method."""
        ctr = cli_t.CLITranslator()
        find_method_return_module_info = lambda x, y: uc.MODULE_INFOS
        # pylint: disable=invalid-name
        find_method_return_module_class_info = (lambda x, test: uc.MODULE_INFOS
                                                if test == uc.MODULE_NAME
                                                else uc.CLASS_INFOS)
        find_method_return_nothing = lambda x, y: None
        one_test = [uc.MODULE_NAME]
        mult_test = [uc.MODULE_NAME, uc.CLASS_NAME]

        # Let's make sure we return what we expect.
        expected_test_infos = {uc.MODULE_INFO}
        mock_getfindmethods.return_value = [
            test_finder_base.Finder(None, find_method_return_module_info, None)]
        unittest_utils.assert_strict_equal(
            self, ctr._get_test_infos(one_test), expected_test_infos)

        # Check we receive multiple test infos.
        expected_test_infos = {uc.MODULE_INFO, uc.CLASS_INFO}
        mock_getfindmethods.return_value = [
            test_finder_base.Finder(None, find_method_return_module_class_info,
                                    None)]
        unittest_utils.assert_strict_equal(
            self, ctr._get_test_infos(mult_test), expected_test_infos)

        # Check return null set when we have no tests found or multiple results.
        mock_getfindmethods.return_value = [
            test_finder_base.Finder(None, find_method_return_nothing, None)]
        null_test_info = set()
        mock_getfuzzyresults.return_value = []
        self.assertEqual(null_test_info, ctr._get_test_infos(one_test))
        self.assertEqual(null_test_info, ctr._get_test_infos(mult_test))

        # Check returning test_info when the user says Yes.
        mock_input.return_value = "Y"
        mock_getfindmethods.return_value = [
            test_finder_base.Finder(None, find_method_return_module_info, None)]
        mock_getfuzzyresults.return_value = one_test
        mock_findtestbymodule.return_value = uc.MODULE_INFO
        unittest_utils.assert_strict_equal(
            self, ctr._get_test_infos([uc.TYPO_MODULE_NAME]), {uc.MODULE_INFO})

        # Check the method works for test mapping.
        test_detail1 = test_mapping.TestDetail(uc.TEST_MAPPING_TEST)
        test_detail2 = test_mapping.TestDetail(uc.TEST_MAPPING_TEST_WITH_OPTION)
        expected_test_infos = {uc.MODULE_INFO, uc.CLASS_INFO}
        mock_getfindmethods.return_value = [
            test_finder_base.Finder(None, find_method_return_module_class_info,
                                    None)]
        test_infos = ctr._get_test_infos(
            mult_test, [test_detail1, test_detail2])
        unittest_utils.assert_strict_equal(
            self, test_infos, expected_test_infos)
        for test_info in test_infos:
            if test_info == uc.MODULE_INFO:
                self.assertEqual(
                    test_detail1.options,
                    test_info.data[constants.TI_MODULE_ARG])
            else:
                self.assertEqual(
                    test_detail2.options,
                    test_info.data[constants.TI_MODULE_ARG])

    @mock.patch.object(metrics, 'FindTestFinishEvent')
    @mock.patch.object(test_finder_handler, 'get_find_methods_for_test')
    def test_get_test_infos_2(self, mock_getfindmethods, _metrics):
        """Test _get_test_infos method."""
        ctr = cli_t.CLITranslator()
        find_method_return_module_info2 = lambda x, y: uc.MODULE_INFOS2
        find_method_ret_mod_cls_info2 = (
            lambda x, test: uc.MODULE_INFOS2
            if test == uc.MODULE_NAME else uc.CLASS_INFOS2)
        one_test = [uc.MODULE_NAME]
        mult_test = [uc.MODULE_NAME, uc.CLASS_NAME]
        # Let's make sure we return what we expect.
        expected_test_infos = {uc.MODULE_INFO, uc.MODULE_INFO2}
        mock_getfindmethods.return_value = [
            test_finder_base.Finder(None, find_method_return_module_info2,
                                    None)]
        unittest_utils.assert_strict_equal(
            self, ctr._get_test_infos(one_test), expected_test_infos)
        # Check we receive multiple test infos.
        expected_test_infos = {uc.MODULE_INFO, uc.CLASS_INFO, uc.MODULE_INFO2,
                               uc.CLASS_INFO2}
        mock_getfindmethods.return_value = [
            test_finder_base.Finder(None, find_method_ret_mod_cls_info2,
                                    None)]
        unittest_utils.assert_strict_equal(
            self, ctr._get_test_infos(mult_test), expected_test_infos)
        # Check the method works for test mapping.
        test_detail1 = test_mapping.TestDetail(uc.TEST_MAPPING_TEST)
        test_detail2 = test_mapping.TestDetail(uc.TEST_MAPPING_TEST_WITH_OPTION)
        expected_test_infos = {uc.MODULE_INFO, uc.CLASS_INFO, uc.MODULE_INFO2,
                               uc.CLASS_INFO2}
        mock_getfindmethods.return_value = [
            test_finder_base.Finder(None, find_method_ret_mod_cls_info2,
                                    None)]
        test_infos = ctr._get_test_infos(
            mult_test, [test_detail1, test_detail2])
        unittest_utils.assert_strict_equal(
            self, test_infos, expected_test_infos)
        for test_info in test_infos:
            if test_info in [uc.MODULE_INFO, uc.MODULE_INFO2]:
                self.assertEqual(
                    test_detail1.options,
                    test_info.data[constants.TI_MODULE_ARG])
            elif test_info in [uc.CLASS_INFO, uc.CLASS_INFO2]:
                self.assertEqual(
                    test_detail2.options,
                    test_info.data[constants.TI_MODULE_ARG])

    @mock.patch.dict('os.environ', {constants.ANDROID_BUILD_TOP:'/'})
    @mock.patch.object(module_finder.ModuleFinder, 'get_fuzzy_searching_results')
    @mock.patch.object(metrics, 'FindTestFinishEvent')
    @mock.patch.object(test_finder_handler, 'get_find_methods_for_test')
    def test_get_test_infos_with_mod_info(
            self, mock_getfindmethods, _metrics, mock_getfuzzyresults,):
        """Test _get_test_infos method."""
        mod_info = module_info.ModuleInfo(
            module_file=os.path.join(uc.TEST_DATA_DIR, uc.JSON_FILE))
        ctr = cli_t.CLITranslator(module_info=mod_info)
        null_test_info = set()
        mock_getfindmethods.return_value = []
        mock_getfuzzyresults.return_value = []
        unittest_utils.assert_strict_equal(
            self, ctr._get_test_infos('not_exist_module'), null_test_info)

    @mock.patch.object(cli_t.CLITranslator, '_get_test_infos',
                       side_effect=gettestinfos_side_effect)
    def test_translate_class(self, _info):
        """Test translate method for tests by class name."""
        # Check that we can find a class.
        self.args.tests = [uc.CLASS_NAME]
        self.args.host_unit_test_only = False
        targets, test_infos = self.ctr.translate(self.args)
        unittest_utils.assert_strict_equal(
            self, targets, uc.CLASS_BUILD_TARGETS)
        unittest_utils.assert_strict_equal(self, test_infos, {uc.CLASS_INFO})

    @mock.patch.object(cli_t.CLITranslator, '_get_test_infos',
                       side_effect=gettestinfos_side_effect)
    def test_translate_module(self, _info):
        """Test translate method for tests by module or class name."""
        # Check that we get all the build targets we expect.
        self.args.tests = [uc.MODULE_NAME, uc.CLASS_NAME]
        self.args.host_unit_test_only = False
        targets, test_infos = self.ctr.translate(self.args)
        unittest_utils.assert_strict_equal(
            self, targets, uc.MODULE_CLASS_COMBINED_BUILD_TARGETS)
        unittest_utils.assert_strict_equal(self, test_infos, {uc.MODULE_INFO,
                                                              uc.CLASS_INFO})

    @mock.patch.object(test_finder_utils, 'find_host_unit_tests', return_value=[])
    @mock.patch.object(cli_t.CLITranslator, '_find_tests_by_test_mapping')
    @mock.patch.object(cli_t.CLITranslator, '_get_test_infos',
                       side_effect=gettestinfos_side_effect)
    def test_translate_test_mapping(self, _info, mock_testmapping,
        _find_unit_tests):
        """Test translate method for tests in test mapping."""
        # Check that test mappings feeds into get_test_info properly.
        test_detail1 = test_mapping.TestDetail(uc.TEST_MAPPING_TEST)
        test_detail2 = test_mapping.TestDetail(uc.TEST_MAPPING_TEST_WITH_OPTION)
        mock_testmapping.return_value = ([test_detail1, test_detail2], None)
        self.args.tests = []
        self.args.host = False
        self.args.host_unit_test_only = False
        targets, test_infos = self.ctr.translate(self.args)
        unittest_utils.assert_strict_equal(
            self, targets, uc.MODULE_CLASS_COMBINED_BUILD_TARGETS)
        unittest_utils.assert_strict_equal(self, test_infos, {uc.MODULE_INFO,
                                                              uc.CLASS_INFO})

    @mock.patch.object(cli_t.CLITranslator, '_find_tests_by_test_mapping')
    @mock.patch.object(cli_t.CLITranslator, '_get_test_infos',
                       side_effect=gettestinfos_side_effect)
    def test_translate_test_mapping_all(self, _info, mock_testmapping):
        """Test translate method for tests in test mapping."""
        # Check that test mappings feeds into get_test_info properly.
        test_detail1 = test_mapping.TestDetail(uc.TEST_MAPPING_TEST)
        test_detail2 = test_mapping.TestDetail(uc.TEST_MAPPING_TEST_WITH_OPTION)
        mock_testmapping.return_value = ([test_detail1, test_detail2], None)
        self.args.tests = ['src_path:all']
        self.args.test_mapping = True
        self.args.host = False
        targets, test_infos = self.ctr.translate(self.args)
        unittest_utils.assert_strict_equal(
            self, targets, uc.MODULE_CLASS_COMBINED_BUILD_TARGETS)
        unittest_utils.assert_strict_equal(self, test_infos, {uc.MODULE_INFO,
                                                              uc.CLASS_INFO})

    def test_find_tests_by_test_mapping_presubmit(self):
        """Test _find_tests_by_test_mapping method to locate presubmit tests."""
        os_environ_mock = {constants.ANDROID_BUILD_TOP: uc.TEST_DATA_DIR}
        with mock.patch.dict('os.environ', os_environ_mock, clear=True):
            tests, all_tests = self.ctr._find_tests_by_test_mapping(
                path=TEST_MAPPING_DIR, file_name='test_mapping_sample',
                checked_files=set())
        expected = set([TEST_1, TEST_2, TEST_5, TEST_7, TEST_9])
        expected_all_tests = {'presubmit': expected,
                              'postsubmit': set(
                                  [TEST_3, TEST_6, TEST_8, TEST_10]),
                              'other_group': set([TEST_4])}
        self.assertEqual(expected, tests)
        self.assertEqual(expected_all_tests, all_tests)

    def test_find_tests_by_test_mapping_postsubmit(self):
        """Test _find_tests_by_test_mapping method to locate postsubmit tests.
        """
        os_environ_mock = {constants.ANDROID_BUILD_TOP: uc.TEST_DATA_DIR}
        with mock.patch.dict('os.environ', os_environ_mock, clear=True):
            tests, all_tests = self.ctr._find_tests_by_test_mapping(
                path=TEST_MAPPING_DIR,
                test_groups=[constants.TEST_GROUP_POSTSUBMIT],
                file_name='test_mapping_sample', checked_files=set())
        expected_presubmit = set([TEST_1, TEST_2, TEST_5, TEST_7, TEST_9])
        expected = set([TEST_3, TEST_6, TEST_8, TEST_10])
        expected_all_tests = {'presubmit': expected_presubmit,
                              'postsubmit': set(
                                  [TEST_3, TEST_6, TEST_8, TEST_10]),
                              'other_group': set([TEST_4])}
        self.assertEqual(expected, tests)
        self.assertEqual(expected_all_tests, all_tests)

    def test_find_tests_by_test_mapping_all_group(self):
        """Test _find_tests_by_test_mapping method to locate postsubmit tests.
        """
        os_environ_mock = {constants.ANDROID_BUILD_TOP: uc.TEST_DATA_DIR}
        with mock.patch.dict('os.environ', os_environ_mock, clear=True):
            tests, all_tests = self.ctr._find_tests_by_test_mapping(
                path=TEST_MAPPING_DIR, test_groups=[constants.TEST_GROUP_ALL],
                file_name='test_mapping_sample', checked_files=set())
        expected_presubmit = set([TEST_1, TEST_2, TEST_5, TEST_7, TEST_9])
        expected = set([
            TEST_1, TEST_2, TEST_3, TEST_4, TEST_5, TEST_6, TEST_7, TEST_8,
            TEST_9, TEST_10])
        expected_all_tests = {'presubmit': expected_presubmit,
                              'postsubmit': set(
                                  [TEST_3, TEST_6, TEST_8, TEST_10]),
                              'other_group': set([TEST_4])}
        self.assertEqual(expected, tests)
        self.assertEqual(expected_all_tests, all_tests)

    def test_find_tests_by_test_mapping_include_subdir(self):
        """Test _find_tests_by_test_mapping method to include sub directory."""
        os_environ_mock = {constants.ANDROID_BUILD_TOP: uc.TEST_DATA_DIR}
        with mock.patch.dict('os.environ', os_environ_mock, clear=True):
            tests, all_tests = self.ctr._find_tests_by_test_mapping(
                path=TEST_MAPPING_TOP_DIR, file_name='test_mapping_sample',
                include_subdirs=True, checked_files=set())
        expected = set([TEST_1, TEST_2, TEST_5, TEST_7, TEST_9])
        expected_all_tests = {'presubmit': expected,
                              'postsubmit': set([
                                  TEST_3, TEST_6, TEST_8, TEST_10]),
                              'other_group': set([TEST_4])}
        self.assertEqual(expected, tests)
        self.assertEqual(expected_all_tests, all_tests)

    @mock.patch('builtins.input', return_value='')
    def test_confirm_running(self, mock_input):
        """Test _confirm_running method."""
        self.assertTrue(self.ctr._confirm_running([TEST_1]))
        mock_input.return_value = 'N'
        self.assertFalse(self.ctr._confirm_running([TEST_2]))

    def test_print_fuzzy_searching_results(self):
        """Test _print_fuzzy_searching_results"""
        modules = [uc.MODULE_NAME, uc.MODULE2_NAME]
        capture_output = StringIO()
        sys.stdout = capture_output
        self.ctr._print_fuzzy_searching_results(modules)
        sys.stdout = sys.__stdout__
        output = 'Did you mean the following modules?\n{0}\n{1}\n'.format(
            uc.MODULE_NAME, uc.MODULE2_NAME)
        self.assertEqual(capture_output.getvalue(), output)

    def test_filter_comments(self):
        """Test filter_comments method"""
        file_with_comments = os.path.join(TEST_MAPPING_TOP_DIR,
                                          'folder6',
                                          'test_mapping_sample_with_comments')
        file_with_comments_golden = os.path.join(TEST_MAPPING_TOP_DIR,
                                                 'folder6',
                                                 'test_mapping_sample_golden')
        test_mapping_dict = json.loads(
            self.ctr.filter_comments(file_with_comments))
        test_mapping_dict_gloden = None
        with open(file_with_comments_golden) as json_file:
            test_mapping_dict_gloden = json.load(json_file)

        self.assertEqual(test_mapping_dict, test_mapping_dict_gloden)

    @mock.patch.dict('os.environ', {constants.ANDROID_BUILD_TOP:'/'})
    @mock.patch.object(module_info.ModuleInfo, 'get_testable_modules')
    def test_extract_testable_modules_by_wildcard(self, mock_mods):
        """Test _extract_testable_modules_by_wildcard method."""
        mod_info = module_info.ModuleInfo(
            module_file=os.path.join(uc.TEST_DATA_DIR, uc.JSON_FILE))
        ctr = cli_t.CLITranslator(module_info=mod_info)
        mock_mods.return_value = ['test1', 'test2', 'test3', 'test11',
                                  'Test22', 'Test100', 'aTest101']
        # test '*'
        expr1 = ['test*']
        result1 = ['test1', 'test2', 'test3', 'test11']
        self.assertEqual(ctr._extract_testable_modules_by_wildcard(expr1),
                         result1)
        # test '?'
        expr2 = ['test?']
        result2 = ['test1', 'test2', 'test3']
        self.assertEqual(ctr._extract_testable_modules_by_wildcard(expr2),
                         result2)
        # test '*' and '?'
        expr3 = ['*Test???']
        result3 = ['Test100', 'aTest101']
        self.assertEqual(ctr._extract_testable_modules_by_wildcard(expr3),
                         result3)

    @mock.patch.object(test_finder_utils, 'find_host_unit_tests',
                       return_value=[uc.HOST_UNIT_TEST_NAME_1,
                                     uc.HOST_UNIT_TEST_NAME_2])
    @mock.patch.object(cli_t.CLITranslator, '_find_tests_by_test_mapping')
    @mock.patch.object(cli_t.CLITranslator, '_get_test_infos',
                       side_effect=gettestinfos_side_effect)
    def test_translate_test_mapping_host_unit_test(self, _info, mock_testmapping,
        _find_unit_tests):
        """Test translate method for tests belong to host unit tests."""
        # Check that test mappings feeds into get_test_info properly.
        test_detail1 = test_mapping.TestDetail(uc.TEST_MAPPING_TEST)
        test_detail2 = test_mapping.TestDetail(uc.TEST_MAPPING_TEST_WITH_OPTION)
        mock_testmapping.return_value = ([test_detail1, test_detail2], None)
        self.args.tests = []
        self.args.host = False
        self.args.host_unit_test_only = False
        _, test_infos = self.ctr.translate(self.args)
        unittest_utils.assert_strict_equal(self,
                                           test_infos,
                                           {uc.MODULE_INFO,
                                            uc.CLASS_INFO,
                                            uc.MODULE_INFO_HOST_1,
                                            uc.MODULE_INFO_HOST_2})

if __name__ == '__main__':
    unittest.main()
