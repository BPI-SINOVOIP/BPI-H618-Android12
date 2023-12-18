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

"""Unittests for module_finder."""

# pylint: disable=line-too-long
# pylint: disable=too-many-lines
# pylint: disable=unsubscriptable-object

import copy
import re
import unittest
import os

from unittest import mock

import atest_error
import atest_configs
import atest_utils
import constants
import module_info
import unittest_constants as uc
import unittest_utils

from test_finders import module_finder
from test_finders import test_finder_utils
from test_finders import test_info
from test_runners import atest_tf_test_runner as atf_tr

MODULE_CLASS = '%s:%s' % (uc.MODULE_NAME, uc.CLASS_NAME)
MODULE_PACKAGE = '%s:%s' % (uc.MODULE_NAME, uc.PACKAGE)
CC_MODULE_CLASS = '%s:%s' % (uc.CC_MODULE_NAME, uc.CC_CLASS_NAME)
KERNEL_TEST_CLASS = 'test_class_1'
KERNEL_TEST_CONFIG = 'KernelTest.xml.data'
KERNEL_MODULE_CLASS = '%s:%s' % (constants.REQUIRED_KERNEL_TEST_MODULES[0],
                                 KERNEL_TEST_CLASS)
KERNEL_CONFIG_FILE = os.path.join(uc.TEST_DATA_DIR, KERNEL_TEST_CONFIG)
KERNEL_CLASS_FILTER = test_info.TestFilter(KERNEL_TEST_CLASS, frozenset())
KERNEL_MODULE_CLASS_DATA = {constants.TI_REL_CONFIG: KERNEL_CONFIG_FILE,
                            constants.TI_FILTER: frozenset([KERNEL_CLASS_FILTER])}
KERNEL_MODULE_CLASS_INFO = test_info.TestInfo(
    constants.REQUIRED_KERNEL_TEST_MODULES[0],
    atf_tr.AtestTradefedTestRunner.NAME,
    uc.CLASS_BUILD_TARGETS, KERNEL_MODULE_CLASS_DATA)
FLAT_METHOD_INFO = test_info.TestInfo(
    uc.MODULE_NAME,
    atf_tr.AtestTradefedTestRunner.NAME,
    uc.MODULE_BUILD_TARGETS,
    data={constants.TI_FILTER: frozenset([uc.FLAT_METHOD_FILTER]),
          constants.TI_REL_CONFIG: uc.CONFIG_FILE})
MODULE_CLASS_METHOD = '%s#%s' % (MODULE_CLASS, uc.METHOD_NAME)
CC_MODULE_CLASS_METHOD = '%s#%s' % (CC_MODULE_CLASS, uc.CC_METHOD_NAME)
CLASS_INFO_MODULE_2 = test_info.TestInfo(
    uc.MODULE2_NAME,
    atf_tr.AtestTradefedTestRunner.NAME,
    uc.CLASS_BUILD_TARGETS,
    data={constants.TI_FILTER: frozenset([uc.CLASS_FILTER]),
          constants.TI_REL_CONFIG: uc.CONFIG2_FILE})
CC_CLASS_INFO_MODULE_2 = test_info.TestInfo(
    uc.CC_MODULE2_NAME,
    atf_tr.AtestTradefedTestRunner.NAME,
    uc.CLASS_BUILD_TARGETS,
    data={constants.TI_FILTER: frozenset([uc.CC_CLASS_FILTER]),
          constants.TI_REL_CONFIG: uc.CC_CONFIG2_FILE})
DEFAULT_INSTALL_PATH = ['/path/to/install']
ROBO_MOD_PATH = ['/shared/robo/path']
NON_RUN_ROBO_MOD_NAME = 'robo_mod'
RUN_ROBO_MOD_NAME = 'run_robo_mod'
NON_RUN_ROBO_MOD = {constants.MODULE_NAME: NON_RUN_ROBO_MOD_NAME,
                    constants.MODULE_PATH: ROBO_MOD_PATH,
                    constants.MODULE_CLASS: ['random_class']}
RUN_ROBO_MOD = {constants.MODULE_NAME: RUN_ROBO_MOD_NAME,
                constants.MODULE_PATH: ROBO_MOD_PATH,
                constants.MODULE_CLASS: [constants.MODULE_CLASS_ROBOLECTRIC]}

SEARCH_DIR_RE = re.compile(r'^find ([^ ]*).*$')

#pylint: disable=unused-argument
def classoutside_side_effect(find_cmd, shell=False):
    """Mock the check output of a find cmd where class outside module path."""
    search_dir = SEARCH_DIR_RE.match(find_cmd).group(1).strip()
    if search_dir == uc.ROOT:
        return uc.FIND_ONE
    return None


#pylint: disable=protected-access
class ModuleFinderUnittests(unittest.TestCase):
    """Unit tests for module_finder.py"""

    def setUp(self):
        """Set up stuff for testing."""
        self.mod_finder = module_finder.ModuleFinder()
        self.mod_finder.module_info = mock.Mock(spec=module_info.ModuleInfo)
        self.mod_finder.module_info.path_to_module_info = {}
        self.mod_finder.root_dir = uc.ROOT

    def test_is_vts_module(self):
        """Test _load_module_info_file regular operation."""
        mod_name = 'mod'
        is_vts_module_info = {'compatibility_suites': ['vts10', 'tests']}
        self.mod_finder.module_info.get_module_info.return_value = is_vts_module_info
        self.assertTrue(self.mod_finder._is_vts_module(mod_name))

        is_not_vts_module = {'compatibility_suites': ['vts10', 'cts']}
        self.mod_finder.module_info.get_module_info.return_value = is_not_vts_module
        self.assertFalse(self.mod_finder._is_vts_module(mod_name))

    # pylint: disable=unused-argument
    @mock.patch.object(module_finder.ModuleFinder, '_get_build_targets',
                       return_value=copy.deepcopy(uc.MODULE_BUILD_TARGETS))
    def test_find_test_by_module_name(self, _get_targ):
        """Test find_test_by_module_name."""
        self.mod_finder.module_info.is_robolectric_test.return_value = False
        self.mod_finder.module_info.has_test_config.return_value = True
        mod_info = {'installed': ['/path/to/install'],
                    'path': [uc.MODULE_DIR],
                    constants.MODULE_CLASS: [],
                    constants.MODULE_COMPATIBILITY_SUITES: []}
        self.mod_finder.module_info.get_module_info.return_value = mod_info
        t_infos = self.mod_finder.find_test_by_module_name(uc.MODULE_NAME)
        unittest_utils.assert_equal_testinfos(
            self,
            t_infos[0],
            uc.MODULE_INFO)
        self.mod_finder.module_info.get_module_info.return_value = None
        self.mod_finder.module_info.is_testable_module.return_value = False
        self.assertIsNone(self.mod_finder.find_test_by_module_name('Not_Module'))

    @mock.patch('builtins.input', return_value='1')
    @mock.patch.object(module_finder.ModuleFinder, '_get_build_targets',
                       return_value=copy.deepcopy(uc.MODULE_BUILD_TARGETS))
    def test_find_test_by_module_name_w_multiple_config(
            self, _get_targ, _mock_input):
        """Test find_test_by_module_name."""
        atest_configs.GLOBAL_ARGS = mock.Mock()
        atest_configs.GLOBAL_ARGS.test_config_select = True
        self.mod_finder.module_info.is_robolectric_test.return_value = False
        self.mod_finder.module_info.has_test_config.return_value = True
        mod_info = {'installed': ['/path/to/install'],
                    'path': [uc.MODULE_DIR],
                    constants.MODULE_CLASS: [],
                    constants.MODULE_COMPATIBILITY_SUITES: [],
                    constants.MODULE_TEST_CONFIG: [
                        uc.CONFIG_FILE,
                        uc.EXTRA_CONFIG_FILE]}
        self.mod_finder.module_info.get_module_info.return_value = mod_info
        t_infos = self.mod_finder.find_test_by_module_name(uc.MODULE_NAME)
        # Only select one test
        self.assertEqual(len(t_infos), 1)
        # The t_info should be the EXTRA_CONFIG_FILE one.
        unittest_utils.assert_equal_testinfos(
            self, t_infos[0], uc.MODULE_INFO_W_CONFIG)

    @mock.patch.object(module_finder.ModuleFinder, '_get_build_targets',
                       return_value=copy.deepcopy(uc.MODULE_BUILD_TARGETS))
    def test_find_test_by_module_name_w_multiple_config_all(
            self, _get_targ,):
        """Test find_test_by_module_name."""
        atest_configs.GLOBAL_ARGS = mock.Mock()
        atest_configs.GLOBAL_ARGS.test_config_select = False
        self.mod_finder.module_info.is_robolectric_test.return_value = False
        self.mod_finder.module_info.has_test_config.return_value = True
        mod_info = {'installed': ['/path/to/install'],
                    'path': [uc.MODULE_DIR],
                    constants.MODULE_CLASS: [],
                    constants.MODULE_COMPATIBILITY_SUITES: [],
                    constants.MODULE_TEST_CONFIG: [
                        uc.CONFIG_FILE,
                        uc.EXTRA_CONFIG_FILE]}
        self.mod_finder.module_info.get_module_info.return_value = mod_info
        t_infos = self.mod_finder.find_test_by_module_name(uc.MODULE_NAME)
        unittest_utils.assert_equal_testinfos(self, t_infos[0], uc.MODULE_INFO)
        unittest_utils.assert_equal_testinfos(
            self, t_infos[1], uc.MODULE_INFO_W_CONFIG)

    @mock.patch.object(test_finder_utils, 'find_host_unit_tests',
                       return_value=[])
    @mock.patch.object(atest_utils, 'is_build_file', return_value=True)
    @mock.patch.object(test_finder_utils, 'is_parameterized_java_class',
                       return_value=False)
    @mock.patch.object(test_finder_utils, 'has_method_in_file',
                       return_value=True)
    @mock.patch.object(module_finder.ModuleFinder, '_is_vts_module',
                       return_value=False)
    @mock.patch.object(module_finder.ModuleFinder, '_get_build_targets')
    @mock.patch('subprocess.check_output', return_value=uc.FIND_ONE)
    @mock.patch.object(test_finder_utils, 'get_fully_qualified_class_name',
                       return_value=uc.FULL_CLASS_NAME)
    @mock.patch('os.path.isfile', side_effect=unittest_utils.isfile_side_effect)
    @mock.patch('os.path.isdir', return_value=True)
    #pylint: disable=unused-argument
    def test_find_test_by_class_name(self, _isdir, _isfile, _fqcn,
                                     mock_checkoutput, mock_build,
                                     _vts, _has_method_in_file,
                                     _is_parameterized, _is_build_file,
                                     _mock_unit_tests):
        """Test find_test_by_class_name."""
        mock_build.return_value = uc.CLASS_BUILD_TARGETS
        self.mod_finder.module_info.is_auto_gen_test_config.return_value = False
        self.mod_finder.module_info.is_robolectric_test.return_value = False
        self.mod_finder.module_info.has_test_config.return_value = True
        self.mod_finder.module_info.get_module_names.return_value = [uc.MODULE_NAME]
        self.mod_finder.module_info.get_module_info.return_value = {
            constants.MODULE_INSTALLED: DEFAULT_INSTALL_PATH,
            constants.MODULE_NAME: uc.MODULE_NAME,
            constants.MODULE_CLASS: [],
            constants.MODULE_COMPATIBILITY_SUITES: []}
        t_infos = self.mod_finder.find_test_by_class_name(uc.CLASS_NAME)
        unittest_utils.assert_equal_testinfos(
            self, t_infos[0], uc.CLASS_INFO)

        # with method
        mock_build.return_value = copy.deepcopy(uc.MODULE_BUILD_TARGETS)
        class_with_method = '%s#%s' % (uc.CLASS_NAME, uc.METHOD_NAME)
        t_infos = self.mod_finder.find_test_by_class_name(class_with_method)
        unittest_utils.assert_equal_testinfos(
            self, t_infos[0], uc.METHOD_INFO)
        mock_build.return_value = copy.deepcopy(uc.MODULE_BUILD_TARGETS)
        class_methods = '%s,%s' % (class_with_method, uc.METHOD2_NAME)
        t_infos = self.mod_finder.find_test_by_class_name(class_methods)
        unittest_utils.assert_equal_testinfos(
            self, t_infos[0],
            FLAT_METHOD_INFO)
        # module and rel_config passed in
        mock_build.return_value = uc.CLASS_BUILD_TARGETS
        t_infos = self.mod_finder.find_test_by_class_name(
            uc.CLASS_NAME, uc.MODULE_NAME, uc.CONFIG_FILE)
        unittest_utils.assert_equal_testinfos(
            self, t_infos[0], uc.CLASS_INFO)
        # find output fails to find class file
        mock_checkoutput.return_value = ''
        self.assertIsNone(self.mod_finder.find_test_by_class_name('Not class'))
        # class is outside given module path
        mock_checkoutput.side_effect = classoutside_side_effect
        t_infos = self.mod_finder.find_test_by_class_name(uc.CLASS_NAME,
                                                          uc.MODULE2_NAME,
                                                          uc.CONFIG2_FILE)
        unittest_utils.assert_equal_testinfos(
            self, t_infos[0],
            CLASS_INFO_MODULE_2)

    @mock.patch.object(test_finder_utils, 'is_parameterized_java_class',
                       return_value=False)
    @mock.patch.object(test_finder_utils, 'has_method_in_file',
                       return_value=True)
    @mock.patch.object(module_finder.ModuleFinder, '_is_vts_module',
                       return_value=False)
    @mock.patch.object(module_finder.ModuleFinder, '_get_build_targets')
    @mock.patch('subprocess.check_output', return_value=uc.FIND_ONE)
    @mock.patch.object(test_finder_utils, 'get_fully_qualified_class_name',
                       return_value=uc.FULL_CLASS_NAME)
    @mock.patch('os.path.isfile', side_effect=unittest_utils.isfile_side_effect)
    #pylint: disable=unused-argument
    def test_find_test_by_module_and_class(self, _isfile, _fqcn,
                                           mock_checkoutput, mock_build,
                                           _vts, _has_method_in_file,
                                           _is_parameterized):
        """Test find_test_by_module_and_class."""
        # Native test was tested in test_find_test_by_cc_class_name().
        self.mod_finder.module_info.is_native_test.return_value = False
        self.mod_finder.module_info.is_auto_gen_test_config.return_value = False
        self.mod_finder.module_info.is_robolectric_test.return_value = False
        self.mod_finder.module_info.has_test_config.return_value = True
        mock_build.return_value = uc.CLASS_BUILD_TARGETS
        mod_info = {constants.MODULE_INSTALLED: DEFAULT_INSTALL_PATH,
                    constants.MODULE_PATH: [uc.MODULE_DIR],
                    constants.MODULE_CLASS: [],
                    constants.MODULE_COMPATIBILITY_SUITES: []}
        self.mod_finder.module_info.get_module_info.return_value = mod_info
        t_infos = self.mod_finder.find_test_by_module_and_class(MODULE_CLASS)
        unittest_utils.assert_equal_testinfos(self, t_infos[0], uc.CLASS_INFO)
        # with method
        mock_build.return_value = copy.deepcopy(uc.MODULE_BUILD_TARGETS)
        t_infos = self.mod_finder.find_test_by_module_and_class(MODULE_CLASS_METHOD)
        unittest_utils.assert_equal_testinfos(self, t_infos[0], uc.METHOD_INFO)
        self.mod_finder.module_info.is_testable_module.return_value = False
        # bad module, good class, returns None
        bad_module = '%s:%s' % ('BadMod', uc.CLASS_NAME)
        self.mod_finder.module_info.get_module_info.return_value = None
        self.assertIsNone(self.mod_finder.find_test_by_module_and_class(bad_module))
        # find output fails to find class file
        mock_checkoutput.return_value = ''
        bad_class = '%s:%s' % (uc.MODULE_NAME, 'Anything')
        self.mod_finder.module_info.get_module_info.return_value = mod_info
        self.assertIsNone(self.mod_finder.find_test_by_module_and_class(bad_class))

    @mock.patch.object(module_finder.ModuleFinder, 'find_test_by_kernel_class_name',
                       return_value=None)
    @mock.patch.object(module_finder.ModuleFinder, '_is_vts_module',
                       return_value=False)
    @mock.patch.object(module_finder.ModuleFinder, '_get_build_targets')
    @mock.patch('subprocess.check_output', return_value=uc.FIND_CC_ONE)
    @mock.patch.object(test_finder_utils, 'find_class_file',
                       side_effect=[None, None, '/'])
    @mock.patch('os.path.isfile', side_effect=unittest_utils.isfile_side_effect)
    #pylint: disable=unused-argument
    def test_find_test_by_module_and_class_part_2(self, _isfile, mock_fcf,
                                                  mock_checkoutput, mock_build,
                                                  _vts, _find_kernel):
        """Test find_test_by_module_and_class for MODULE:CC_CLASS."""
        # Native test was tested in test_find_test_by_cc_class_name()
        self.mod_finder.module_info.is_native_test.return_value = False
        self.mod_finder.module_info.is_auto_gen_test_config.return_value = False
        self.mod_finder.module_info.is_robolectric_test.return_value = False
        self.mod_finder.module_info.has_test_config.return_value = True
        self.mod_finder.module_info.get_paths.return_value = []
        mock_build.return_value = uc.CLASS_BUILD_TARGETS
        mod_info = {constants.MODULE_INSTALLED: DEFAULT_INSTALL_PATH,
                    constants.MODULE_PATH: [uc.CC_MODULE_DIR],
                    constants.MODULE_CLASS: [],
                    constants.MODULE_COMPATIBILITY_SUITES: []}
        self.mod_finder.module_info.get_module_info.return_value = mod_info
        t_infos = self.mod_finder.find_test_by_module_and_class(CC_MODULE_CLASS)
        unittest_utils.assert_equal_testinfos(self, t_infos[0], uc.CC_MODULE_CLASS_INFO)
        # with method
        mock_build.return_value = copy.deepcopy(uc.MODULE_BUILD_TARGETS)
        mock_fcf.side_effect = [None, None, '/']
        t_infos = self.mod_finder.find_test_by_module_and_class(CC_MODULE_CLASS_METHOD)
        unittest_utils.assert_equal_testinfos(self, t_infos[0], uc.CC_METHOD_INFO)
        # bad module, good class, returns None
        bad_module = '%s:%s' % ('BadMod', uc.CC_CLASS_NAME)
        self.mod_finder.module_info.get_module_info.return_value = None
        self.mod_finder.module_info.is_testable_module.return_value = False
        self.assertIsNone(self.mod_finder.find_test_by_module_and_class(bad_module))

    @mock.patch.object(module_finder.ModuleFinder, '_get_module_test_config',
                       return_value=[KERNEL_CONFIG_FILE])
    @mock.patch.object(module_finder.ModuleFinder, '_is_vts_module',
                       return_value=False)
    @mock.patch.object(module_finder.ModuleFinder, '_get_build_targets')
    @mock.patch('subprocess.check_output', return_value=uc.FIND_CC_ONE)
    @mock.patch.object(test_finder_utils, 'find_class_file',
                       side_effect=[None, None, '/'])
    @mock.patch('os.path.isfile', side_effect=unittest_utils.isfile_side_effect)
    #pylint: disable=unused-argument
    def test_find_test_by_module_and_class_for_kernel_test(
            self, _isfile, mock_fcf, mock_checkoutput, mock_build, _vts,
            _test_config):
        """Test find_test_by_module_and_class for MODULE:CC_CLASS."""
        # Kernel test was tested in find_test_by_kernel_class_name()
        self.mod_finder.module_info.is_native_test.return_value = False
        self.mod_finder.module_info.is_auto_gen_test_config.return_value = False
        self.mod_finder.module_info.is_robolectric_test.return_value = False
        self.mod_finder.module_info.has_test_config.return_value = True
        self.mod_finder.module_info.get_paths.return_value = []
        mock_build.return_value = uc.CLASS_BUILD_TARGETS
        mod_info = {constants.MODULE_INSTALLED: DEFAULT_INSTALL_PATH,
                    constants.MODULE_PATH: [uc.CC_MODULE_DIR],
                    constants.MODULE_CLASS: [],
                    constants.MODULE_COMPATIBILITY_SUITES: []}
        self.mod_finder.module_info.get_module_info.return_value = mod_info
        t_infos = self.mod_finder.find_test_by_module_and_class(KERNEL_MODULE_CLASS)
        unittest_utils.assert_equal_testinfos(self, t_infos[0], KERNEL_MODULE_CLASS_INFO)

    @mock.patch.object(test_finder_utils, 'find_host_unit_tests',
                       return_value=[])
    @mock.patch.object(module_finder.ModuleFinder, '_is_vts_module',
                       return_value=False)
    @mock.patch.object(module_finder.ModuleFinder, '_get_build_targets')
    @mock.patch('subprocess.check_output', return_value=uc.FIND_PKG)
    @mock.patch('os.path.isfile', side_effect=unittest_utils.isfile_side_effect)
    @mock.patch('os.path.isdir', return_value=True)
    #pylint: disable=unused-argument
    def test_find_test_by_package_name(self, _isdir, _isfile, mock_checkoutput,
                                       mock_build, _vts, _mock_unit_tests):
        """Test find_test_by_package_name."""
        self.mod_finder.module_info.is_auto_gen_test_config.return_value = False
        self.mod_finder.module_info.is_robolectric_test.return_value = False
        self.mod_finder.module_info.has_test_config.return_value = True
        mock_build.return_value = uc.CLASS_BUILD_TARGETS
        self.mod_finder.module_info.get_module_names.return_value = [uc.MODULE_NAME]
        self.mod_finder.module_info.get_module_info.return_value = {
            constants.MODULE_INSTALLED: DEFAULT_INSTALL_PATH,
            constants.MODULE_NAME: uc.MODULE_NAME,
            constants.MODULE_CLASS: [],
            constants.MODULE_COMPATIBILITY_SUITES: []
            }
        t_infos = self.mod_finder.find_test_by_package_name(uc.PACKAGE)
        unittest_utils.assert_equal_testinfos(
            self, t_infos[0],
            uc.PACKAGE_INFO)
        # with method, should raise
        pkg_with_method = '%s#%s' % (uc.PACKAGE, uc.METHOD_NAME)
        self.assertRaises(atest_error.MethodWithoutClassError,
                          self.mod_finder.find_test_by_package_name,
                          pkg_with_method)
        # module and rel_config passed in
        t_infos = self.mod_finder.find_test_by_package_name(
            uc.PACKAGE, uc.MODULE_NAME, uc.CONFIG_FILE)
        unittest_utils.assert_equal_testinfos(
            self, t_infos[0], uc.PACKAGE_INFO)
        # find output fails to find class file
        mock_checkoutput.return_value = ''
        self.assertIsNone(self.mod_finder.find_test_by_package_name('Not pkg'))

    @mock.patch('os.path.isdir', return_value=False)
    @mock.patch.object(module_finder.ModuleFinder, '_is_vts_module',
                       return_value=False)
    @mock.patch.object(module_finder.ModuleFinder, '_get_build_targets')
    @mock.patch('subprocess.check_output', return_value=uc.FIND_PKG)
    @mock.patch('os.path.isfile', side_effect=unittest_utils.isfile_side_effect)
    #pylint: disable=unused-argument
    def test_find_test_by_module_and_package(self, _isfile, mock_checkoutput,
                                             mock_build, _vts, _isdir):
        """Test find_test_by_module_and_package."""
        self.mod_finder.module_info.is_auto_gen_test_config.return_value = False
        self.mod_finder.module_info.is_robolectric_test.return_value = False
        self.mod_finder.module_info.has_test_config.return_value = True
        self.mod_finder.module_info.get_paths.return_value = []
        mock_build.return_value = uc.CLASS_BUILD_TARGETS
        mod_info = {constants.MODULE_INSTALLED: DEFAULT_INSTALL_PATH,
                    constants.MODULE_PATH: [uc.MODULE_DIR],
                    constants.MODULE_CLASS: [],
                    constants.MODULE_COMPATIBILITY_SUITES: []}
        self.mod_finder.module_info.get_module_info.return_value = mod_info
        t_infos = self.mod_finder.find_test_by_module_and_package(MODULE_PACKAGE)
        self.assertEqual(t_infos, None)
        _isdir.return_value = True
        t_infos = self.mod_finder.find_test_by_module_and_package(MODULE_PACKAGE)
        unittest_utils.assert_equal_testinfos(self, t_infos[0], uc.PACKAGE_INFO)

        # with method, raises
        module_pkg_with_method = '%s:%s#%s' % (uc.MODULE2_NAME, uc.PACKAGE,
                                               uc.METHOD_NAME)
        self.assertRaises(atest_error.MethodWithoutClassError,
                          self.mod_finder.find_test_by_module_and_package,
                          module_pkg_with_method)
        # bad module, good pkg, returns None
        self.mod_finder.module_info.is_testable_module.return_value = False
        bad_module = '%s:%s' % ('BadMod', uc.PACKAGE)
        self.mod_finder.module_info.get_module_info.return_value = None
        self.assertIsNone(self.mod_finder.find_test_by_module_and_package(bad_module))
        # find output fails to find package path
        mock_checkoutput.return_value = ''
        bad_pkg = '%s:%s' % (uc.MODULE_NAME, 'Anything')
        self.mod_finder.module_info.get_module_info.return_value = mod_info
        self.assertIsNone(self.mod_finder.find_test_by_module_and_package(bad_pkg))

    @mock.patch.object(test_finder_utils, 'find_host_unit_tests',
                       return_value=[])
    @mock.patch.object(test_finder_utils, 'get_cc_test_classes_methods',
                       return_value=(set(), set(), set()))
    @mock.patch.object(atest_utils, 'is_build_file', return_value=True)
    @mock.patch.object(test_finder_utils, 'is_parameterized_java_class',
                       return_value=False)
    @mock.patch.object(test_finder_utils, 'has_method_in_file',
                       return_value=True)
    @mock.patch.object(test_finder_utils, 'has_cc_class',
                       return_value=True)
    @mock.patch.object(module_finder.ModuleFinder, '_get_build_targets')
    @mock.patch.object(module_finder.ModuleFinder, '_is_vts_module',
                       return_value=False)
    @mock.patch.object(test_finder_utils, 'get_fully_qualified_class_name',
                       return_value=uc.FULL_CLASS_NAME)
    @mock.patch('os.path.realpath',
                side_effect=unittest_utils.realpath_side_effect)
    @mock.patch('os.path.isfile', side_effect=unittest_utils.isfile_side_effect)
    @mock.patch.object(test_finder_utils, 'find_parent_module_dir')
    @mock.patch('os.path.exists')
    #pylint: disable=unused-argument
    def test_find_test_by_path(
            self, mock_pathexists, mock_dir, _isfile, _real, _fqcn, _vts,
            mock_build, _has_cc_class, _has_method_in_file, _is_parameterized,
            _is_build_file, _get_cc_test_classed, _mock_unit_tests):
        """Test find_test_by_path."""
        self.mod_finder.module_info.is_robolectric_test.return_value = False
        self.mod_finder.module_info.has_test_config.return_value = True
        mock_build.return_value = set()
        # Check that we don't return anything with invalid test references.
        mock_pathexists.return_value = False
        unittest_utils.assert_equal_testinfos(
            self, None, self.mod_finder.find_test_by_path('bad/path'))
        mock_pathexists.return_value = True
        mock_dir.return_value = None
        unittest_utils.assert_equal_testinfos(
            self, None, self.mod_finder.find_test_by_path('no/module'))
        self.mod_finder.module_info.get_module_names.return_value = [uc.MODULE_NAME]
        self.mod_finder.module_info.get_module_info.return_value = {
            constants.MODULE_INSTALLED: DEFAULT_INSTALL_PATH,
            constants.MODULE_NAME: uc.MODULE_NAME,
            constants.MODULE_CLASS: [],
            constants.MODULE_COMPATIBILITY_SUITES: []}

        # Happy path testing.
        mock_dir.return_value = uc.MODULE_DIR

        class_path = '%s.kt' % uc.CLASS_NAME
        mock_build.return_value = uc.CLASS_BUILD_TARGETS
        t_infos = self.mod_finder.find_test_by_path(class_path)
        unittest_utils.assert_equal_testinfos(
            self, uc.CLASS_INFO, t_infos[0])

        class_with_method = '%s#%s' % (class_path, uc.METHOD_NAME)
        mock_build.return_value = copy.deepcopy(uc.MODULE_BUILD_TARGETS)
        t_infos = self.mod_finder.find_test_by_path(class_with_method)
        unittest_utils.assert_equal_testinfos(
            self, t_infos[0], uc.METHOD_INFO)

        class_path = '%s.java' % uc.CLASS_NAME
        mock_build.return_value = uc.CLASS_BUILD_TARGETS
        t_infos = self.mod_finder.find_test_by_path(class_path)
        unittest_utils.assert_equal_testinfos(
            self, uc.CLASS_INFO, t_infos[0])

        class_with_method = '%s#%s' % (class_path, uc.METHOD_NAME)
        mock_build.return_value = copy.deepcopy(uc.MODULE_BUILD_TARGETS)
        t_infos = self.mod_finder.find_test_by_path(class_with_method)
        unittest_utils.assert_equal_testinfos(
            self, t_infos[0], uc.METHOD_INFO)

        class_with_methods = '%s,%s' % (class_with_method, uc.METHOD2_NAME)
        mock_build.return_value = copy.deepcopy(uc.MODULE_BUILD_TARGETS)
        t_infos = self.mod_finder.find_test_by_path(class_with_methods)
        unittest_utils.assert_equal_testinfos(
            self, t_infos[0],
            FLAT_METHOD_INFO)

        # Cc path testing.
        self.mod_finder.module_info.get_module_names.return_value = [uc.CC_MODULE_NAME]
        self.mod_finder.module_info.get_module_info.return_value = {
            constants.MODULE_INSTALLED: DEFAULT_INSTALL_PATH,
            constants.MODULE_NAME: uc.CC_MODULE_NAME,
            constants.MODULE_CLASS: [],
            constants.MODULE_COMPATIBILITY_SUITES: []}
        mock_dir.return_value = uc.CC_MODULE_DIR
        class_path = '%s' % uc.CC_PATH
        mock_build.return_value = uc.CLASS_BUILD_TARGETS
        t_infos = self.mod_finder.find_test_by_path(class_path)
        unittest_utils.assert_equal_testinfos(
            self, uc.CC_PATH_INFO2, t_infos[0])

    @mock.patch.object(module_finder.ModuleFinder, '_get_build_targets',
                       return_value=copy.deepcopy(uc.MODULE_BUILD_TARGETS))
    @mock.patch.object(module_finder.ModuleFinder, '_is_vts_module',
                       return_value=False)
    @mock.patch.object(test_finder_utils, 'find_parent_module_dir',
                       return_value=os.path.relpath(uc.TEST_DATA_DIR, uc.ROOT))
    #pylint: disable=unused-argument
    def test_find_test_by_path_part_2(self, _find_parent, _is_vts, _get_build):
        """Test find_test_by_path for directories."""
        self.mod_finder.module_info.is_auto_gen_test_config.return_value = False
        self.mod_finder.module_info.is_robolectric_test.return_value = False
        self.mod_finder.module_info.has_test_config.return_value = True
        # Dir with java files in it, should run as package
        class_dir = os.path.join(uc.TEST_DATA_DIR, 'path_testing')
        self.mod_finder.module_info.get_module_names.return_value = [uc.MODULE_NAME]
        self.mod_finder.module_info.get_module_info.return_value = {
            constants.MODULE_INSTALLED: DEFAULT_INSTALL_PATH,
            constants.MODULE_NAME: uc.MODULE_NAME,
            constants.MODULE_CLASS: [],
            constants.MODULE_COMPATIBILITY_SUITES: []}
        t_infos = self.mod_finder.find_test_by_path(class_dir)
        unittest_utils.assert_equal_testinfos(
            self, uc.PATH_INFO, t_infos[0])
        # Dir with no java files in it, should run whole module
        empty_dir = os.path.join(uc.TEST_DATA_DIR, 'path_testing_empty')
        t_infos = self.mod_finder.find_test_by_path(empty_dir)
        unittest_utils.assert_equal_testinfos(
            self, uc.EMPTY_PATH_INFO,
            t_infos[0])
        # Dir with cc files in it, should run as cc class
        class_dir = os.path.join(uc.TEST_DATA_DIR, 'cc_path_testing')
        self.mod_finder.module_info.get_module_names.return_value = [uc.CC_MODULE_NAME]
        self.mod_finder.module_info.get_module_info.return_value = {
            constants.MODULE_INSTALLED: DEFAULT_INSTALL_PATH,
            constants.MODULE_NAME: uc.CC_MODULE_NAME,
            constants.MODULE_CLASS: [],
            constants.MODULE_COMPATIBILITY_SUITES: []}
        t_infos = self.mod_finder.find_test_by_path(class_dir)
        unittest_utils.assert_equal_testinfos(
            self, uc.CC_PATH_INFO, t_infos[0])

    @mock.patch.object(test_finder_utils, 'find_host_unit_tests',
                       return_value=[])
    @mock.patch.object(atest_utils, 'is_build_file', return_value=True)
    @mock.patch.object(test_finder_utils, 'has_method_in_file',
                       return_value=True)
    @mock.patch.object(module_finder.ModuleFinder, '_is_vts_module',
                       return_value=False)
    @mock.patch.object(module_finder.ModuleFinder, '_get_build_targets')
    @mock.patch('subprocess.check_output', return_value=uc.CC_FIND_ONE)
    @mock.patch('os.path.isfile', side_effect=unittest_utils.isfile_side_effect)
    @mock.patch('os.path.isdir', return_value=True)
    #pylint: disable=unused-argument
    def test_find_test_by_cc_class_name(self, _isdir, _isfile,
                                        mock_checkoutput, mock_build,
                                        _vts, _has_method, _is_build_file,
                                       _mock_unit_tests):
        """Test find_test_by_cc_class_name."""
        mock_build.return_value = uc.CLASS_BUILD_TARGETS
        self.mod_finder.module_info.is_auto_gen_test_config.return_value = False
        self.mod_finder.module_info.is_robolectric_test.return_value = False
        self.mod_finder.module_info.has_test_config.return_value = True
        self.mod_finder.module_info.get_module_names.return_value = [uc.CC_MODULE_NAME]
        self.mod_finder.module_info.get_module_info.return_value = {
            constants.MODULE_INSTALLED: DEFAULT_INSTALL_PATH,
            constants.MODULE_NAME: uc.CC_MODULE_NAME,
            constants.MODULE_CLASS: [],
            constants.MODULE_COMPATIBILITY_SUITES: []}
        t_infos = self.mod_finder.find_test_by_cc_class_name(uc.CC_CLASS_NAME)
        unittest_utils.assert_equal_testinfos(
            self, t_infos[0], uc.CC_CLASS_INFO)

        # with method
        mock_build.return_value = copy.deepcopy(uc.MODULE_BUILD_TARGETS)
        class_with_method = '%s#%s' % (uc.CC_CLASS_NAME, uc.CC_METHOD_NAME)
        t_infos = self.mod_finder.find_test_by_cc_class_name(class_with_method)
        unittest_utils.assert_equal_testinfos(
            self,
            t_infos[0],
            uc.CC_METHOD_INFO)
        mock_build.return_value = copy.deepcopy(uc.MODULE_BUILD_TARGETS)
        class_methods = '%s,%s' % (class_with_method, uc.CC_METHOD2_NAME)
        t_infos = self.mod_finder.find_test_by_cc_class_name(class_methods)
        unittest_utils.assert_equal_testinfos(
            self, t_infos[0],
            uc.CC_METHOD2_INFO)
        # module and rel_config passed in
        mock_build.return_value = uc.CLASS_BUILD_TARGETS
        t_infos = self.mod_finder.find_test_by_cc_class_name(
            uc.CC_CLASS_NAME, uc.CC_MODULE_NAME, uc.CC_CONFIG_FILE)
        unittest_utils.assert_equal_testinfos(
            self, t_infos[0], uc.CC_CLASS_INFO)
        # find output fails to find class file
        mock_checkoutput.return_value = ''
        self.assertIsNone(self.mod_finder.find_test_by_cc_class_name(
            'Not class'))
        # class is outside given module path
        mock_checkoutput.return_value = uc.CC_FIND_ONE
        t_infos = self.mod_finder.find_test_by_cc_class_name(
            uc.CC_CLASS_NAME,
            uc.CC_MODULE2_NAME,
            uc.CC_CONFIG2_FILE)
        unittest_utils.assert_equal_testinfos(
            self, t_infos[0],
            CC_CLASS_INFO_MODULE_2)

    def test_get_testable_modules_with_ld(self):
        """Test get_testable_modules_with_ld"""
        self.mod_finder.module_info.get_testable_modules.return_value = [
            uc.MODULE_NAME, uc.MODULE2_NAME]
        # Without a misfit constraint
        ld1 = self.mod_finder.get_testable_modules_with_ld(uc.TYPO_MODULE_NAME)
        self.assertEqual([[16, uc.MODULE2_NAME], [1, uc.MODULE_NAME]], ld1)
        # With a misfit constraint
        ld2 = self.mod_finder.get_testable_modules_with_ld(uc.TYPO_MODULE_NAME, 2)
        self.assertEqual([[1, uc.MODULE_NAME]], ld2)

    def test_get_fuzzy_searching_modules(self):
        """Test get_fuzzy_searching_modules"""
        self.mod_finder.module_info.get_testable_modules.return_value = [
            uc.MODULE_NAME, uc.MODULE2_NAME]
        result = self.mod_finder.get_fuzzy_searching_results(uc.TYPO_MODULE_NAME)
        self.assertEqual(uc.MODULE_NAME, result[0])

    def test_get_build_targets_w_vts_core(self):
        """Test _get_build_targets."""
        self.mod_finder.module_info.is_auto_gen_test_config.return_value = True
        self.mod_finder.module_info.get_paths.return_value = []
        mod_info = {constants.MODULE_COMPATIBILITY_SUITES:
                        [constants.VTS_CORE_SUITE]}
        self.mod_finder.module_info.get_module_info.return_value = mod_info
        self.assertEqual(self.mod_finder._get_build_targets('', ''),
                         {constants.VTS_CORE_TF_MODULE})

    @mock.patch.object(test_finder_utils, 'is_parameterized_java_class',
                       return_value=False)
    @mock.patch.object(module_finder.ModuleFinder, '_is_vts_module',
                       return_value=False)
    @mock.patch.object(module_finder.ModuleFinder, '_get_build_targets')
    @mock.patch('subprocess.check_output', return_value='')
    @mock.patch.object(test_finder_utils, 'get_fully_qualified_class_name',
                       return_value=uc.FULL_CLASS_NAME)
    @mock.patch('os.path.isfile', side_effect=unittest_utils.isfile_side_effect)
    @mock.patch('os.path.isdir', return_value=True)
    #pylint: disable=unused-argument
    def test_find_test_by_class_name_w_module(self, _isdir, _isfile, _fqcn,
                                              mock_checkoutput, mock_build,
                                              _vts, _is_parameterized):
        """Test test_find_test_by_class_name with module but without class found."""
        mock_build.return_value = uc.CLASS_BUILD_TARGETS
        self.mod_finder.module_info.is_auto_gen_test_config.return_value = False
        self.mod_finder.module_info.is_robolectric_test.return_value = False
        self.mod_finder.module_info.has_test_config.return_value = True
        self.mod_finder.module_info.get_module_names.return_value = [uc.MODULE_NAME]
        self.mod_finder.module_info.get_module_info.return_value = {
            constants.MODULE_INSTALLED: DEFAULT_INSTALL_PATH,
            constants.MODULE_NAME: uc.MODULE_NAME,
            constants.MODULE_CLASS: [],
            constants.MODULE_COMPATIBILITY_SUITES: []}
        self.mod_finder.module_info.get_paths.return_value = [uc.TEST_DATA_CONFIG]
        t_infos = self.mod_finder.find_test_by_class_name(
            uc.FULL_CLASS_NAME, module_name=uc.MODULE_NAME,
            rel_config=uc.CONFIG_FILE)
        unittest_utils.assert_equal_testinfos(
            self, t_infos[0], uc.CLASS_INFO)

    @mock.patch.object(module_finder.ModuleFinder, '_is_vts_module',
                       return_value=False)
    @mock.patch.object(module_finder.ModuleFinder, '_get_build_targets')
    @mock.patch('subprocess.check_output', return_value='')
    @mock.patch('os.path.isfile', side_effect=unittest_utils.isfile_side_effect)
    @mock.patch('os.path.isdir', return_value=True)
    #pylint: disable=unused-argument
    def test_find_test_by_package_name_w_module(self, _isdir, _isfile,
                                                mock_checkoutput, mock_build,
                                                _vts):
        """Test find_test_by_package_name with module but without package found."""
        self.mod_finder.module_info.is_auto_gen_test_config.return_value = False
        self.mod_finder.module_info.is_robolectric_test.return_value = False
        self.mod_finder.module_info.has_test_config.return_value = True
        mock_build.return_value = uc.CLASS_BUILD_TARGETS
        self.mod_finder.module_info.get_module_names.return_value = [uc.MODULE_NAME]
        self.mod_finder.module_info.get_module_info.return_value = {
            constants.MODULE_INSTALLED: DEFAULT_INSTALL_PATH,
            constants.MODULE_NAME: uc.MODULE_NAME,
            constants.MODULE_CLASS: [],
            constants.MODULE_COMPATIBILITY_SUITES: []
        }
        self.mod_finder.module_info.get_paths.return_value = [uc.TEST_DATA_CONFIG]
        t_infos = self.mod_finder.find_test_by_package_name(
            uc.PACKAGE, module_name=uc.MODULE_NAME, rel_config=uc.CONFIG_FILE)
        unittest_utils.assert_equal_testinfos(
            self, t_infos[0],
            uc.PACKAGE_INFO)

    @mock.patch.object(atest_utils, 'is_build_file', return_value=True)
    @mock.patch.object(test_finder_utils, 'is_parameterized_java_class',
                       return_value=True)
    @mock.patch.object(test_finder_utils, 'has_method_in_file',
                       return_value=True)
    @mock.patch.object(test_finder_utils, 'has_cc_class',
                       return_value=True)
    @mock.patch.object(module_finder.ModuleFinder, '_get_build_targets')
    @mock.patch.object(module_finder.ModuleFinder, '_is_vts_module',
                       return_value=False)
    @mock.patch.object(test_finder_utils, 'get_fully_qualified_class_name',
                       return_value=uc.FULL_CLASS_NAME)
    @mock.patch('os.path.realpath',
                side_effect=unittest_utils.realpath_side_effect)
    @mock.patch('os.path.isfile', side_effect=unittest_utils.isfile_side_effect)
    @mock.patch.object(test_finder_utils, 'find_parent_module_dir')
    @mock.patch('os.path.exists')
    #pylint: disable=unused-argument
    def test_find_test_by_path_is_parameterized_java(
            self, mock_pathexists, mock_dir, _isfile, _real, _fqcn, _vts,
            mock_build, _has_cc_class, _has_method_in_file, _is_parameterized,
            _is_build_file):
        """Test find_test_by_path and input path is parameterized class."""
        self.mod_finder.module_info.is_robolectric_test.return_value = False
        self.mod_finder.module_info.has_test_config.return_value = True
        mock_build.return_value = set()
        mock_pathexists.return_value = True
        self.mod_finder.module_info.get_module_names.return_value = [uc.MODULE_NAME]
        self.mod_finder.module_info.get_module_info.return_value = {
            constants.MODULE_INSTALLED: DEFAULT_INSTALL_PATH,
            constants.MODULE_NAME: uc.MODULE_NAME,
            constants.MODULE_CLASS: [],
            constants.MODULE_COMPATIBILITY_SUITES: []}
        # Happy path testing.
        mock_dir.return_value = uc.MODULE_DIR
        class_path = '%s.java' % uc.CLASS_NAME
        # Input include only one method
        class_with_method = '%s#%s' % (class_path, uc.METHOD_NAME)
        mock_build.return_value = copy.deepcopy(uc.MODULE_BUILD_TARGETS)
        t_infos = self.mod_finder.find_test_by_path(class_with_method)
        unittest_utils.assert_equal_testinfos(
            self, t_infos[0], uc.PARAMETERIZED_METHOD_INFO)
        # Input include multiple methods
        class_with_methods = '%s,%s' % (class_with_method, uc.METHOD2_NAME)
        mock_build.return_value = copy.deepcopy(uc.MODULE_BUILD_TARGETS)
        t_infos = self.mod_finder.find_test_by_path(class_with_methods)
        unittest_utils.assert_equal_testinfos(
            self, t_infos[0], uc.PARAMETERIZED_FLAT_METHOD_INFO)

    @mock.patch.object(test_finder_utils, 'find_host_unit_tests',
                           return_value=[])
    @mock.patch.object(atest_utils, 'is_build_file', return_value=True)
    @mock.patch.object(test_finder_utils, 'is_parameterized_java_class',
                       return_value=True)
    @mock.patch.object(test_finder_utils, 'has_method_in_file',
                       return_value=True)
    @mock.patch.object(module_finder.ModuleFinder, '_is_vts_module',
                       return_value=False)
    @mock.patch.object(module_finder.ModuleFinder, '_get_build_targets')
    @mock.patch('subprocess.check_output', return_value=uc.FIND_ONE)
    @mock.patch.object(test_finder_utils, 'get_fully_qualified_class_name',
                       return_value=uc.FULL_CLASS_NAME)
    @mock.patch('os.path.isfile', side_effect=unittest_utils.isfile_side_effect)
    @mock.patch('os.path.isdir', return_value=True)
    #pylint: disable=unused-argument
    def test_find_test_by_class_name_is_parameterized(
            self, _isdir, _isfile, _fqcn, mock_checkoutput, mock_build, _vts,
            _has_method_in_file, _is_parameterized, _is_build_file,
            _mock_unit_tests):
        """Test find_test_by_class_name and the class is parameterized java."""
        mock_build.return_value = uc.CLASS_BUILD_TARGETS
        self.mod_finder.module_info.is_auto_gen_test_config.return_value = False
        self.mod_finder.module_info.is_robolectric_test.return_value = False
        self.mod_finder.module_info.has_test_config.return_value = True
        self.mod_finder.module_info.get_module_names.return_value = [uc.MODULE_NAME]
        self.mod_finder.module_info.get_module_info.return_value = {
            constants.MODULE_INSTALLED: DEFAULT_INSTALL_PATH,
            constants.MODULE_NAME: uc.MODULE_NAME,
            constants.MODULE_CLASS: [],
            constants.MODULE_COMPATIBILITY_SUITES: []}
        # With method
        mock_build.return_value = copy.deepcopy(uc.MODULE_BUILD_TARGETS)
        class_with_method = '%s#%s' % (uc.CLASS_NAME, uc.METHOD_NAME)
        t_infos = self.mod_finder.find_test_by_class_name(class_with_method)
        unittest_utils.assert_equal_testinfos(
            self, t_infos[0], uc.PARAMETERIZED_METHOD_INFO)
        # With multiple method
        mock_build.return_value = copy.deepcopy(uc.MODULE_BUILD_TARGETS)
        class_methods = '%s,%s' % (class_with_method, uc.METHOD2_NAME)
        t_infos = self.mod_finder.find_test_by_class_name(class_methods)
        unittest_utils.assert_equal_testinfos(
            self, t_infos[0], uc.PARAMETERIZED_FLAT_METHOD_INFO)

    # pylint: disable=unused-argument
    @mock.patch.object(module_finder.ModuleFinder, '_get_build_targets',
                       return_value=copy.deepcopy(uc.MODULE_BUILD_TARGETS))
    def test_find_test_by_config_name(self, _get_targ):
        """Test find_test_by_config_name."""
        self.mod_finder.module_info.is_robolectric_test.return_value = False
        self.mod_finder.module_info.has_test_config.return_value = True

        mod_info = {'installed': ['/path/to/install'],
                    'path': [uc.MODULE_DIR],
                    constants.MODULE_TEST_CONFIG: [uc.CONFIG_FILE,
                                                   uc.EXTRA_CONFIG_FILE],
                    constants.MODULE_CLASS: [],
                    constants.MODULE_COMPATIBILITY_SUITES: []}
        name_to_module_info = {uc.MODULE_NAME: mod_info}
        self.mod_finder.module_info.name_to_module_info = name_to_module_info
        t_infos = self.mod_finder.find_test_by_config_name(uc.MODULE_CONFIG_NAME)
        unittest_utils.assert_equal_testinfos(
            self,
            t_infos[0],
            uc.TEST_CONFIG_MODULE_INFO)

    @mock.patch.object(test_finder_utils, 'is_parameterized_java_class',
                       return_value=False)
    @mock.patch.object(test_finder_utils, 'has_method_in_file',
                       return_value=True)
    @mock.patch.object(test_finder_utils, 'has_cc_class',
                       return_value=True)
    @mock.patch.object(module_finder.ModuleFinder, '_get_build_targets')
    @mock.patch.object(module_finder.ModuleFinder, '_is_vts_module',
                       return_value=False)
    @mock.patch.object(test_finder_utils, 'get_fully_qualified_class_name',
                       return_value=uc.FULL_CLASS_NAME)
    @mock.patch('os.path.realpath',
                side_effect=unittest_utils.realpath_side_effect)
    @mock.patch('os.path.isfile', side_effect=unittest_utils.isfile_side_effect)
    @mock.patch.object(test_finder_utils, 'find_parent_module_dir')
    @mock.patch('os.path.exists')
    #pylint: disable=unused-argument
    def test_find_test_by_path_w_src_verify(
            self, mock_pathexists, mock_dir, _isfile, _real, _fqcn, _vts,
            mock_build, _has_cc_class, _has_method_in_file, _is_parameterized):
        """Test find_test_by_path with src information."""
        self.mod_finder.module_info.is_robolectric_test.return_value = False
        self.mod_finder.module_info.has_test_config.return_value = True
        self.mod_finder.module_info.get_module_names.return_value = [uc.MODULE_NAME]
        mock_build.return_value = uc.CLASS_BUILD_TARGETS

        # Happy path testing.
        mock_dir.return_value = uc.MODULE_DIR
        # Test path not in module's src list.
        class_path = '%s.java' % uc.CLASS_NAME
        self.mod_finder.module_info.get_module_info.return_value = {
            constants.MODULE_INSTALLED: DEFAULT_INSTALL_PATH,
            constants.MODULE_NAME: uc.MODULE_NAME,
            constants.MODULE_CLASS: [],
            constants.MODULE_COMPATIBILITY_SUITES: [],
            constants.MODULE_SRCS: ['not_matched_%s' % class_path]}
        t_infos = self.mod_finder.find_test_by_path(class_path)
        self.assertEqual(0, len(t_infos))

        # Test input file is in module's src list.
        class_path = '%s.java' % uc.CLASS_NAME
        self.mod_finder.module_info.get_module_info.return_value = {
            constants.MODULE_INSTALLED: DEFAULT_INSTALL_PATH,
            constants.MODULE_NAME: uc.MODULE_NAME,
            constants.MODULE_CLASS: [],
            constants.MODULE_COMPATIBILITY_SUITES: [],
            constants.MODULE_SRCS: [class_path]}
        t_infos = self.mod_finder.find_test_by_path(class_path)
        unittest_utils.assert_equal_testinfos(self, uc.CLASS_INFO, t_infos[0])

    @mock.patch.object(test_finder_utils, 'get_cc_test_classes_methods')
    @mock.patch.object(atest_utils, 'is_build_file', return_value=True)
    @mock.patch.object(test_finder_utils, 'is_parameterized_java_class',
                       return_value=False)
    @mock.patch.object(test_finder_utils, 'has_method_in_file',
                       return_value=True)
    @mock.patch.object(test_finder_utils, 'has_cc_class',
                       return_value=True)
    @mock.patch.object(module_finder.ModuleFinder, '_get_build_targets')
    @mock.patch.object(module_finder.ModuleFinder, '_is_vts_module',
                       return_value=False)
    @mock.patch.object(test_finder_utils, 'get_fully_qualified_class_name',
                       return_value=uc.FULL_CLASS_NAME)
    @mock.patch('os.path.realpath',
                side_effect=unittest_utils.realpath_side_effect)
    @mock.patch('os.path.isfile', side_effect=unittest_utils.isfile_side_effect)
    @mock.patch.object(test_finder_utils, 'find_parent_module_dir')
    @mock.patch('os.path.exists')
    #pylint: disable=unused-argument
    def test_find_test_by_path_for_cc_file(self, mock_pathexists, mock_dir,
        _isfile, _real, _fqcn, _vts, mock_build, _has_cc_class,
        _has_method_in_file, _is_parameterized, _is_build_file,
        _mock_get_cc_test_class):
        """Test find_test_by_path for handling correct CC filter."""
        self.mod_finder.module_info.is_robolectric_test.return_value = False
        self.mod_finder.module_info.has_test_config.return_value = True
        mock_build.return_value = set()
        # Check that we don't return anything with invalid test references.
        mock_pathexists.return_value = False
        mock_pathexists.return_value = True
        mock_dir.return_value = None
        self.mod_finder.module_info.get_module_names.return_value = [uc.MODULE_NAME]
        self.mod_finder.module_info.get_module_info.return_value = {
            constants.MODULE_INSTALLED: DEFAULT_INSTALL_PATH,
            constants.MODULE_NAME: uc.MODULE_NAME,
            constants.MODULE_CLASS: [],
            constants.MODULE_COMPATIBILITY_SUITES: []}
        # Happy path testing.
        mock_dir.return_value = uc.MODULE_DIR
        # Cc path testing if get_cc_test_classes_methods found those information.
        self.mod_finder.module_info.get_module_names.return_value = [uc.CC_MODULE_NAME]
        self.mod_finder.module_info.get_module_info.return_value = {
            constants.MODULE_INSTALLED: DEFAULT_INSTALL_PATH,
            constants.MODULE_NAME: uc.CC_MODULE_NAME,
            constants.MODULE_CLASS: [],
            constants.MODULE_COMPATIBILITY_SUITES: []}
        mock_dir.return_value = uc.CC_MODULE_DIR
        class_path = '%s' % uc.CC_PATH
        mock_build.return_value = uc.CLASS_BUILD_TARGETS
        # Test without paramertize test
        founded_classed = {'class1'}
        founded_methods = {'method1'}
        founded_para_classes = set()
        _mock_get_cc_test_class.return_value = (founded_classed,
                                                founded_methods,
                                                founded_para_classes)
        cc_path_data = {constants.TI_REL_CONFIG: uc.CC_CONFIG_FILE,
                        constants.TI_FILTER: frozenset(
                            {test_info.TestFilter(class_name='class1.*',
                                                  methods=frozenset())})}
        cc_path_info = test_info.TestInfo(uc.CC_MODULE_NAME,
                                          atf_tr.AtestTradefedTestRunner.NAME,
                                          uc.CLASS_BUILD_TARGETS, cc_path_data)
        t_infos = self.mod_finder.find_test_by_path(class_path)
        unittest_utils.assert_equal_testinfos(self, cc_path_info, t_infos[0])
        # Test with paramertize test defined in input path
        founded_classed = {'class1'}
        founded_methods = {'method1'}
        founded_para_classes = {'class1'}
        _mock_get_cc_test_class.return_value = (founded_classed,
                                                founded_methods,
                                                founded_para_classes)
        cc_path_data = {constants.TI_REL_CONFIG: uc.CC_CONFIG_FILE,
                        constants.TI_FILTER: frozenset(
                            {test_info.TestFilter(class_name='*/class1.*',
                                                  methods=frozenset())})}
        cc_path_info = test_info.TestInfo(uc.CC_MODULE_NAME,
                                          atf_tr.AtestTradefedTestRunner.NAME,
                                          uc.CLASS_BUILD_TARGETS, cc_path_data)
        t_infos = self.mod_finder.find_test_by_path(class_path)
        unittest_utils.assert_equal_testinfos(self, cc_path_info, t_infos[0])

    # pylint: disable=unused-argument
    @mock.patch.object(module_finder.ModuleFinder, '_is_vts_module',
                       return_value=False)
    @mock.patch.object(module_finder.ModuleFinder, '_get_build_targets',
                       return_value=copy.deepcopy(uc.MODULE_BUILD_TARGETS))
    def test_process_test_info(self, _get_targ, _is_vts):
        """Test _process_test_info."""
        mod_info = {'installed': ['/path/to/install'],
                    'path': [uc.MODULE_DIR],
                    constants.MODULE_CLASS: [
                        constants.MODULE_CLASS_JAVA_LIBRARIES],
                    constants.MODULE_COMPATIBILITY_SUITES: []}
        self.mod_finder.module_info.is_robolectric_test.return_value = False
        self.mod_finder.module_info.is_auto_gen_test_config.return_value = True
        self.mod_finder.module_info.get_module_info.return_value = mod_info
        processed_info = self.mod_finder._process_test_info(
            copy.copy(uc.MODULE_INFO))
        unittest_utils.assert_equal_testinfos(
            self,
            processed_info,
            uc.MODULE_INFO_W_DALVIK)

    @mock.patch.object(test_finder_utils, 'get_annotated_methods')
    def test_is_srcs_match_method_annotation_include_anno(
        self, _mock_get_anno_methods):
        """Test _is_srcs_match_method_annotation with include annotation."""
        annotation_dict = {constants.INCLUDE_ANNOTATION: 'includeAnnotation1'}
        input_method = 'my_input_method'
        input_srcs = ['src1']
        # Test if input method matched include annotation.
        _mock_get_anno_methods.return_value = {input_method,
                                               'not_my_input_method'}

        is_matched = self.mod_finder._is_srcs_match_method_annotation(
            input_method, input_srcs, annotation_dict)

        self.assertTrue(is_matched)
        # Test if input method not matched include annotation.
        _mock_get_anno_methods.return_value = {'not_my_input_method'}

        is_matched = self.mod_finder._is_srcs_match_method_annotation(
            input_method, input_srcs, annotation_dict)

        self.assertFalse(is_matched)

    @mock.patch.object(test_finder_utils, 'get_annotated_methods')
    @mock.patch.object(test_finder_utils, 'get_java_methods')
    def test_is_srcs_match_method_exclude_anno(self, _mock_get_java_methods,
        _mock_get_exclude_anno_methods):
        """Test _is_srcs_match_method_annotation with exclude annotation."""
        annotation_dict = {constants.EXCLUDE_ANNOTATION: 'excludeAnnotation1'}
        input_method = 'my_input_method'
        input_srcs = ['src1']
        _mock_get_java_methods.return_value = {input_method,
                                               'method1',
                                               'method2'}
        # Test if input method matched exclude annotation.
        _mock_get_exclude_anno_methods.return_value = {input_method, 'method1'}

        is_matched = self.mod_finder._is_srcs_match_method_annotation(
            input_method, input_srcs, annotation_dict)

        self.assertFalse(is_matched)

        # Test if input method not matched exclude annotation.
        _mock_get_exclude_anno_methods.return_value = {'method2'}

        is_matched = self.mod_finder._is_srcs_match_method_annotation(
            input_method, input_srcs, annotation_dict)

        self.assertTrue(is_matched)

    @mock.patch.object(atest_utils, 'get_android_junit_config_filters')
    @mock.patch.object(test_finder_utils, 'get_test_config_and_srcs')
    def test_get_matched_test_infos_no_filter(self, _mock_get_conf_srcs,
        _mock_get_filters):
        """Test _get_matched_test_infos without test filters."""
        test_info1 = 'test_info1'
        test_infos = [test_info1]
        test_config = 'test_config'
        test_srcs = ['src1', 'src2']
        _mock_get_conf_srcs.return_value = test_config, test_srcs
        filter_dict = {}
        _mock_get_filters.return_value = filter_dict

        self.assertEqual(
            self.mod_finder._get_matched_test_infos(test_infos, {'method'}),
            test_infos)

    @mock.patch.object(module_finder.ModuleFinder,
                       '_is_srcs_match_method_annotation')
    @mock.patch.object(atest_utils, 'get_android_junit_config_filters')
    @mock.patch.object(test_finder_utils, 'get_test_config_and_srcs')
    def test_get_matched_test_infos_get_filter_method_match(
        self, _mock_get_conf_srcs, _mock_get_filters, _mock_method_match):
        """Test _get_matched_test_infos with test filters and method match."""
        test_infos = [KERNEL_MODULE_CLASS_INFO]
        test_config = 'test_config'
        test_srcs = ['src1', 'src2']
        _mock_get_conf_srcs.return_value = test_config, test_srcs
        filter_dict = {'include-annotation': 'annotate1'}
        _mock_get_filters.return_value = filter_dict
        _mock_method_match.return_value = True

        unittest_utils.assert_strict_equal(
            self,
            self.mod_finder._get_matched_test_infos(test_infos, {'method'}),
            test_infos)

    @mock.patch.object(module_finder.ModuleFinder,
                       '_is_srcs_match_method_annotation')
    @mock.patch.object(atest_utils, 'get_android_junit_config_filters')
    @mock.patch.object(test_finder_utils, 'get_test_config_and_srcs')
    def test_get_matched_test_infos_filter_method_not_match(
        self, _mock_get_conf_srcs, _mock_get_filters, _mock_method_match):
        """Test _get_matched_test_infos but method not match."""
        test_infos = [KERNEL_MODULE_CLASS_INFO]
        test_config = 'test_config'
        test_srcs = ['src1', 'src2']
        _mock_get_conf_srcs.return_value = test_config, test_srcs
        filter_dict = {'include-annotation': 'annotate1'}
        _mock_get_filters.return_value = filter_dict
        _mock_method_match.return_value = False

        self.assertEqual(
            self.mod_finder._get_matched_test_infos(test_infos, {'method'}),
            [])

    @mock.patch.object(module_finder.ModuleFinder, '_get_matched_test_infos')
    @mock.patch.object(module_finder.ModuleFinder, '_get_test_infos',
                       return_value=uc.MODULE_INFO)
    @mock.patch.object(module_finder.ModuleFinder, '_get_test_info_filter',
                       return_value=uc.CLASS_FILTER)
    @mock.patch.object(test_finder_utils, 'find_class_file',
                       return_value=['path1'])
    def test_find_test_by_class_name_not_matched_filters(
        self, _mock_class_path, _mock_test_filters,
        _mock_test_infos, _mock_matched_test_infos):
        """Test find_test_by_class_name which has not matched filters."""
        found_test_infos = [uc.MODULE_INFO, uc.MODULE_INFO2]
        _mock_test_infos.return_value = found_test_infos
        matched_test_infos = [uc.MODULE_INFO2]
        _mock_matched_test_infos.return_value = matched_test_infos

        # Test if class without method
        test_infos = self.mod_finder.find_test_by_class_name('my.test.class')
        self.assertEqual(len(test_infos), 2)
        unittest_utils.assert_equal_testinfos(
            self, test_infos[0], uc.MODULE_INFO)
        unittest_utils.assert_equal_testinfos(
            self, test_infos[1], uc.MODULE_INFO2)

        # Test if class with method
        test_infos = self.mod_finder.find_test_by_class_name(
            'my.test.class#myMethod')
        self.assertEqual(len(test_infos), 1)
        unittest_utils.assert_equal_testinfos(
            self, test_infos[0], uc.MODULE_INFO2)

    @mock.patch.object(module_finder.ModuleFinder, '_get_test_infos',
                       return_value=None)
    @mock.patch.object(module_finder.ModuleFinder, '_get_test_info_filter',
                       return_value=uc.CLASS_FILTER)
    @mock.patch.object(test_finder_utils, 'find_class_file',
                       return_value=['path1'])
    def test_find_test_by_class_name_get_test_infos_none(
        self, _mock_class_path, _mock_test_filters, _mock_test_infos):
        """Test find_test_by_class_name which has not matched test infos."""
        self.assertEqual(
            self.mod_finder.find_test_by_class_name('my.test.class'),
            None)

if __name__ == '__main__':
    unittest.main()
