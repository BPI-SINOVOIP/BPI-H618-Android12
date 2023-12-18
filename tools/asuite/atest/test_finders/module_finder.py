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

"""
Module Finder class.
"""

# pylint: disable=line-too-long

import logging
import os

import atest_configs
import atest_error
import atest_utils
import constants

from test_finders import test_info
from test_finders import test_finder_base
from test_finders import test_finder_utils
from test_runners import atest_tf_test_runner
from test_runners import robolectric_test_runner
from test_runners import vts_tf_test_runner

_ANDROID_MK = 'Android.mk'

# These are suites in LOCAL_COMPATIBILITY_SUITE that aren't really suites so
# we can ignore them.
_SUITES_TO_IGNORE = frozenset({'general-tests', 'device-tests', 'tests'})

class ModuleFinder(test_finder_base.TestFinderBase):
    """Module finder class."""
    NAME = 'MODULE'
    _TEST_RUNNER = atest_tf_test_runner.AtestTradefedTestRunner.NAME
    _ROBOLECTRIC_RUNNER = robolectric_test_runner.RobolectricTestRunner.NAME
    _VTS_TEST_RUNNER = vts_tf_test_runner.VtsTradefedTestRunner.NAME

    def __init__(self, module_info=None):
        super().__init__()
        self.root_dir = os.environ.get(constants.ANDROID_BUILD_TOP)
        self.module_info = module_info

    def _determine_testable_module(self, path, file_path=None):
        """Determine which module the user is trying to test.

        Returns the module to test. If there are multiple possibilities, will
        ask the user. Otherwise will return the only module found.

        Args:
            path: String path of module to look for.
            file_path: String path of input file.

        Returns:
            A list of the module names.
        """
        testable_modules = []
        # A list to save those testable modules but srcs information is empty.
        testable_modules_no_srcs = []
        for mod in self.module_info.get_module_names(path):
            mod_info = self.module_info.get_module_info(mod)
            # Robolectric tests always exist in pairs of 2, one module to build
            # the test and another to run it. For now, we are assuming they are
            # isolated in their own folders and will return if we find one.
            if self.module_info.is_robolectric_test(mod):
                # return a list with one module name if it is robolectric.
                return [mod]
            if self.module_info.is_testable_module(mod_info):
                # If test module defined srcs, input file_path should be defined
                # in the src list of module.
                module_srcs = mod_info.get(constants.MODULE_SRCS, [])
                if file_path and os.path.relpath(
                    file_path, self.root_dir) not in module_srcs:
                    logging.debug('Skip module: %s for %s', mod, file_path)
                    # Collect those modules if they don't have srcs information
                    # in module-info, use this list if there's no other matched
                    # module with src information.
                    if not module_srcs:
                        testable_modules_no_srcs.append(
                            mod_info.get(constants.MODULE_NAME))
                    continue
                testable_modules.append(mod_info.get(constants.MODULE_NAME))
        if not testable_modules:
            testable_modules.extend(testable_modules_no_srcs)
        return test_finder_utils.extract_test_from_tests(testable_modules)

    def _is_vts_module(self, module_name):
        """Returns True if the module is a vts10 module, else False."""
        mod_info = self.module_info.get_module_info(module_name)
        suites = []
        if mod_info:
            suites = mod_info.get('compatibility_suites', [])
        # Pull out all *ts (cts, tvts, etc) suites.
        suites = [suite for suite in suites if suite not in _SUITES_TO_IGNORE]
        return len(suites) == 1 and 'vts10' in suites

    def _update_to_vts_test_info(self, test):
        """Fill in the fields with vts10 specific info.

        We need to update the runner to use the vts10 runner and also find the
        test specific dependencies.

        Args:
            test: TestInfo to update with vts10 specific details.

        Return:
            TestInfo that is ready for the vts10 test runner.
        """
        test.test_runner = self._VTS_TEST_RUNNER
        config_file = os.path.join(self.root_dir,
                                   test.data[constants.TI_REL_CONFIG])
        # Need to get out dir (special logic is to account for custom out dirs).
        # The out dir is used to construct the build targets for the test deps.
        out_dir = os.environ.get(constants.ANDROID_HOST_OUT)
        custom_out_dir = os.environ.get(constants.ANDROID_OUT_DIR)
        # If we're not an absolute custom out dir, get no-absolute out dir path.
        if custom_out_dir is None or not os.path.isabs(custom_out_dir):
            out_dir = os.path.relpath(out_dir, self.root_dir)
        vts_out_dir = os.path.join(out_dir, 'vts10', 'android-vts10', 'testcases')
        # Parse dependency of default staging plans.
        xml_paths = test_finder_utils.search_integration_dirs(
            constants.VTS_STAGING_PLAN,
            self.module_info.get_paths(constants.VTS_TF_MODULE))
        vts_xmls = set()
        vts_xmls.add(config_file)
        for xml_path in xml_paths:
            vts_xmls |= test_finder_utils.get_plans_from_vts_xml(xml_path)
        for config_file in vts_xmls:
            # Add in vts10 test build targets.
            test.build_targets |= test_finder_utils.get_targets_from_vts_xml(
                config_file, vts_out_dir, self.module_info)
        test.build_targets.add('vts-test-core')
        test.build_targets.add(test.test_name)
        return test

    def _update_to_robolectric_test_info(self, test):
        """Update the fields for a robolectric test.

        Args:
          test: TestInfo to be updated with robolectric fields.

        Returns:
          TestInfo with robolectric fields.
        """
        test.test_runner = self._ROBOLECTRIC_RUNNER
        test.test_name = self.module_info.get_robolectric_test_name(test.test_name)
        return test

    def _process_test_info(self, test):
        """Process the test info and return some fields updated/changed.

        We need to check if the test found is a special module (like vts10) and
        update the test_info fields (like test_runner) appropriately.

        Args:
            test: TestInfo that has been filled out by a find method.

        Return:
            TestInfo that has been modified as needed and return None if
            this module can't be found in the module_info.
        """
        module_name = test.test_name
        mod_info = self.module_info.get_module_info(module_name)
        if not mod_info:
            return None
        test.module_class = mod_info['class']
        test.install_locations = test_finder_utils.get_install_locations(
            mod_info['installed'])
        # Check if this is only a vts10 module.
        if self._is_vts_module(test.test_name):
            return self._update_to_vts_test_info(test)
        if self.module_info.is_robolectric_test(test.test_name):
            return self._update_to_robolectric_test_info(test)
        rel_config = test.data[constants.TI_REL_CONFIG]
        test.build_targets = self._get_build_targets(module_name, rel_config)
        # For device side java test, it will use
        # com.android.compatibility.testtype.DalvikTest as test runner in
        # cts-dalvik-device-test-runner.jar
        if self.module_info.is_auto_gen_test_config(module_name):
            if constants.MODULE_CLASS_JAVA_LIBRARIES in test.module_class:
                test.build_targets.update(test_finder_utils.DALVIK_TEST_DEPS)
        # Update test name if the test belong to extra config which means it's
        # test config name is not the same as module name. For extra config, it
        # index will be greater or equal to 1.
        try:
            if (mod_info.get(constants.MODULE_TEST_CONFIG, []).index(rel_config)
                    > 0):
                config_test_name = os.path.splitext(os.path.basename(
                    rel_config))[0]
                logging.debug('Replace test_info.name(%s) to %s',
                              test.test_name, config_test_name)
                test.test_name = config_test_name
        except ValueError:
            pass
        return test

    def _get_build_targets(self, module_name, rel_config):
        """Get the test deps.

        Args:
            module_name: name of the test.
            rel_config: XML for the given test.

        Returns:
            Set of build targets.
        """
        targets = set()
        if not self.module_info.is_auto_gen_test_config(module_name):
            config_file = os.path.join(self.root_dir, rel_config)
            targets = test_finder_utils.get_targets_from_xml(config_file,
                                                             self.module_info)
        if constants.VTS_CORE_SUITE in self.module_info.get_module_info(
                module_name).get(constants.MODULE_COMPATIBILITY_SUITES, []):
            targets.add(constants.VTS_CORE_TF_MODULE)
        for suite in self.module_info.get_module_info(
            module_name).get(constants.MODULE_COMPATIBILITY_SUITES, []):
            targets.update(constants.SUITE_DEPS.get(suite, []))
        for module_path in self.module_info.get_paths(module_name):
            mod_dir = module_path.replace('/', '-')
            targets.add(constants.MODULES_IN + mod_dir)
        # (b/156457698) Force add vts_kernel_tests as build target if our test
        # belong to REQUIRED_KERNEL_TEST_MODULES due to required_module option
        # not working for sh_test in soong.
        if module_name in constants.REQUIRED_KERNEL_TEST_MODULES:
            targets.add('vts_kernel_tests')
        # (b/184567849) Force adding module_name as a build_target. This will
        # allow excluding MODULES-IN-* and prevent from missing build targets.
        if module_name and self.module_info.is_module(module_name):
            targets.add(module_name)
        return targets

    def _get_module_test_config(self, module_name, rel_config=None):
        """Get the value of test_config in module_info.

        Get the value of 'test_config' in module_info if its
        auto_test_config is not true.
        In this case, the test_config is specified by user.
        If not, return rel_config.

        Args:
            module_name: A string of the test's module name.
            rel_config: XML for the given test.

        Returns:
            A list of string of test_config path if found, else return rel_config.
        """
        default_all_config = not (atest_configs.GLOBAL_ARGS and
                                  atest_configs.GLOBAL_ARGS.test_config_select)
        mod_info = self.module_info.get_module_info(module_name)
        if mod_info:
            test_configs = []
            test_config_list = mod_info.get(constants.MODULE_TEST_CONFIG, [])
            if test_config_list:
                # multiple test configs
                if len(test_config_list) > 1:
                    test_configs = test_finder_utils.extract_test_from_tests(
                        test_config_list, default_all=default_all_config)
                else:
                    test_configs = test_config_list
            if test_configs:
                return test_configs
            # Double check if below section is needed.
            if (not self.module_info.is_auto_gen_test_config(module_name)
                    and len(test_configs) > 0):
                return test_configs
        return [rel_config] if rel_config else []

    # pylint: disable=too-many-branches
    # pylint: disable=too-many-locals
    def _get_test_info_filter(self, path, methods, **kwargs):
        """Get test info filter.

        Args:
            path: A string of the test's path.
            methods: A set of method name strings.
            rel_module_dir: Optional. A string of the module dir no-absolute to
                root.
            class_name: Optional. A string of the class name.
            is_native_test: Optional. A boolean variable of whether to search
                for a native test or not.

        Returns:
            A set of test info filter.
        """
        _, file_name = test_finder_utils.get_dir_path_and_filename(path)
        ti_filter = frozenset()
        if kwargs.get('is_native_test', None):
            ti_filter = frozenset([test_info.TestFilter(
                test_finder_utils.get_cc_filter(
                    kwargs.get('class_name', '*'), methods), frozenset())])
        # Path to java file.
        elif file_name and constants.JAVA_EXT_RE.match(file_name):
            full_class_name = test_finder_utils.get_fully_qualified_class_name(
                path)
            # If input class is parameterized java class, adding * to the end of
            # method filter string to make sure the generated method name could
            # be run.
            if test_finder_utils.is_parameterized_java_class(path):
                update_methods = []
                for method in methods:
                    update_methods.append(method + '*')
                methods = frozenset(update_methods)
            ti_filter = frozenset(
                [test_info.TestFilter(full_class_name, methods)])
        # Path to cc file.
        elif file_name and constants.CC_EXT_RE.match(file_name):
            # TODO (b/173019813) Should setup correct filter for an input file.
            if not test_finder_utils.has_cc_class(path):
                raise atest_error.MissingCCTestCaseError(
                    "Can't find CC class in %s" % path)
            # Extract class_name, method_name and parameterized_class from
            # the given cc path.
            file_classes, _, file_para_classes = (
                test_finder_utils.get_cc_test_classes_methods(path))
            cc_filters = []
            # When instantiate tests found, recompose the class name in
            # $(InstantiationName)/$(ClassName)
            for file_class in file_classes:
                if file_class in file_para_classes:
                    file_class = '*/%s' % file_class
                cc_filters.append(
                    test_info.TestFilter(
                        test_finder_utils.get_cc_filter(file_class, methods),
                        frozenset()))
            ti_filter = frozenset(cc_filters)
        # If input path is a folder and have class_name information.
        elif (not file_name and kwargs.get('class_name', None)):
            ti_filter = frozenset(
                [test_info.TestFilter(kwargs.get('class_name', None), methods)])
        # Path to non-module dir, treat as package.
        elif (not file_name
              and kwargs.get('rel_module_dir', None) !=
              os.path.relpath(path, self.root_dir)):
            dir_items = [os.path.join(path, f) for f in os.listdir(path)]
            for dir_item in dir_items:
                if constants.JAVA_EXT_RE.match(dir_item):
                    package_name = test_finder_utils.get_package_name(dir_item)
                    if package_name:
                        # methods should be empty frozenset for package.
                        if methods:
                            raise atest_error.MethodWithoutClassError(
                                '%s: Method filtering requires class'
                                % str(methods))
                        ti_filter = frozenset(
                            [test_info.TestFilter(package_name, methods)])
                        break
        logging.debug('_get_test_info_filter() ti_filter: %s', ti_filter)
        return ti_filter

    def _get_rel_config(self, test_path):
        """Get config file's no-absolute path.

        Args:
            test_path: A string of the test absolute path.

        Returns:
            A string of config's no-absolute path, else None.
        """
        test_dir = os.path.dirname(test_path)
        rel_module_dir = test_finder_utils.find_parent_module_dir(
            self.root_dir, test_dir, self.module_info)
        if rel_module_dir:
            return os.path.join(rel_module_dir, constants.MODULE_CONFIG)
        return None

    def _get_test_infos(self, test_path, rel_config, module_name, test_filter):
        """Get test_info for test_path.

        Args:
            test_path: A string of the test path.
            rel_config: A string of rel path of config.
            module_name: A string of the module name to use.
            test_filter: A test info filter.

        Returns:
            A list of TestInfo namedtuple if found, else None.
        """
        if not rel_config:
            rel_config = self._get_rel_config(test_path)
            if not rel_config:
                return None
        if module_name:
            module_names = [module_name]
        else:
            module_names = self._determine_testable_module(
                os.path.dirname(rel_config),
                test_path if self._is_comparted_src(test_path) else None)
        test_infos = []
        if module_names:
            for mname in module_names:
                # The real test config might be record in module-info.
                rel_configs = self._get_module_test_config(
                    mname, rel_config=rel_config)
                for rel_cfg in rel_configs:
                    mod_info = self.module_info.get_module_info(mname)
                    tinfo = self._process_test_info(test_info.TestInfo(
                        test_name=mname,
                        test_runner=self._TEST_RUNNER,
                        build_targets=set(),
                        data={constants.TI_FILTER: test_filter,
                              constants.TI_REL_CONFIG: rel_cfg},
                        compatibility_suites=mod_info.get(
                            constants.MODULE_COMPATIBILITY_SUITES, [])))
                    if tinfo:
                        test_infos.append(tinfo)
        return test_infos

    def find_test_by_module_name(self, module_name):
        """Find test for the given module name.

        Args:
            module_name: A string of the test's module name.

        Returns:
            A list that includes only 1 populated TestInfo namedtuple
            if found, otherwise None.
        """
        tinfos = []
        mod_info = self.module_info.get_module_info(module_name)
        if self.module_info.is_testable_module(mod_info):
            # path is a list with only 1 element.
            rel_config = os.path.join(mod_info['path'][0],
                                      constants.MODULE_CONFIG)
            rel_configs = self._get_module_test_config(module_name,
                                                       rel_config=rel_config)
            for rel_config in rel_configs:
                tinfo = self._process_test_info(test_info.TestInfo(
                    test_name=module_name,
                    test_runner=self._TEST_RUNNER,
                    build_targets=set(),
                    data={constants.TI_REL_CONFIG: rel_config,
                          constants.TI_FILTER: frozenset()},
                    compatibility_suites=mod_info.get(
                        constants.MODULE_COMPATIBILITY_SUITES, [])))
                if tinfo:
                    tinfos.append(tinfo)
            if tinfos:
                return tinfos
        return None

    def find_test_by_kernel_class_name(self, module_name, class_name):
        """Find kernel test for the given class name.

        Args:
            module_name: A string of the module name to use.
            class_name: A string of the test's class name.

        Returns:
            A list of populated TestInfo namedtuple if test found, else None.
        """

        class_name, methods = test_finder_utils.split_methods(class_name)
        test_configs = self._get_module_test_config(module_name)
        if not test_configs:
            return None
        tinfos = []
        for test_config in test_configs:
            test_config_path = os.path.join(self.root_dir, test_config)
            mod_info = self.module_info.get_module_info(module_name)
            ti_filter = frozenset(
                [test_info.TestFilter(class_name, methods)])
            if test_finder_utils.is_test_from_kernel_xml(test_config_path, class_name):
                tinfo = self._process_test_info(test_info.TestInfo(
                    test_name=module_name,
                    test_runner=self._TEST_RUNNER,
                    build_targets=set(),
                    data={constants.TI_REL_CONFIG: test_config,
                          constants.TI_FILTER: ti_filter},
                    compatibility_suites=mod_info.get(
                        constants.MODULE_COMPATIBILITY_SUITES, [])))
                if tinfo:
                    tinfos.append(tinfo)
        if tinfos:
            return tinfos
        return None

    def find_test_by_class_name(self, class_name, module_name=None,
                                rel_config=None, is_native_test=False):
        """Find test files given a class name.

        If module_name and rel_config not given it will calculate it determine
        it by looking up the tree from the class file.

        Args:
            class_name: A string of the test's class name.
            module_name: Optional. A string of the module name to use.
            rel_config: Optional. A string of module dir no-absolute to repo root.
            is_native_test: A boolean variable of whether to search for a
            native test or not.

        Returns:
            A list of populated TestInfo namedtuple if test found, else None.
        """
        class_name, methods = test_finder_utils.split_methods(class_name)
        search_class_name = class_name
        # For parameterized gtest, test class will be automerged to
        # $(class_prefix)/$(base_class) name. Using $(base_class) for searching
        # matched TEST_P to make sure test class is matched.
        if '/' in search_class_name:
            search_class_name = str(search_class_name).split('/')[-1]
        if rel_config:
            search_dir = os.path.join(self.root_dir,
                                      os.path.dirname(rel_config))
        else:
            search_dir = self.root_dir
        test_paths = test_finder_utils.find_class_file(search_dir, search_class_name,
                                                       is_native_test, methods)
        if not test_paths and rel_config:
            logging.info('Did not find class (%s) under module path (%s), '
                         'researching from repo root.', class_name, rel_config)
            test_paths = test_finder_utils.find_class_file(self.root_dir,
                                                           search_class_name,
                                                           is_native_test,
                                                           methods)
        test_paths = test_paths if test_paths is not None else []
        # If we already have module name, use path in module-info as test_path.
        if not test_paths:
            if not module_name:
                return None
            # Use the module path as test_path.
            module_paths = self.module_info.get_paths(module_name)
            test_paths = []
            for rel_module_path in module_paths:
                test_paths.append(os.path.join(self.root_dir, rel_module_path))
        tinfos = []
        for test_path in test_paths:
            test_filter = self._get_test_info_filter(
                test_path, methods, class_name=class_name,
                is_native_test=is_native_test)
            test_infos = self._get_test_infos(
                test_path, rel_config, module_name, test_filter)
            # If input include methods, check if tinfo match.
            if test_infos and len(test_infos) > 1 and methods:
                test_infos = self._get_matched_test_infos(test_infos, methods)
            if test_infos:
                tinfos.extend(test_infos)
        return tinfos if tinfos else None

    def _get_matched_test_infos(self, test_infos, methods):
        """Get the test_infos matched the given methods.

        Args:
            test_infos: A list of TestInfo obj.
            methods: A set of method name strings.

        Returns:
            A list of matched TestInfo namedtuple, else None.
        """
        matched_test_infos = set()
        for tinfo in test_infos:
            test_config, test_srcs = test_finder_utils.get_test_config_and_srcs(
                tinfo, self.module_info)
            if test_config:
                filter_dict = atest_utils.get_android_junit_config_filters(
                    test_config)
                # Always treat the test_info is matched if no filters found.
                if not filter_dict.keys():
                    matched_test_infos.add(tinfo)
                    continue
                for method in methods:
                    if self._is_srcs_match_method_annotation(method, test_srcs,
                                                             filter_dict):
                        logging.debug('For method:%s Test:%s matched '
                                      'filter_dict: %s', method,
                                      tinfo.test_name, filter_dict)
                        matched_test_infos.add(tinfo)
        return list(matched_test_infos)

    def _is_srcs_match_method_annotation(self, method, srcs, annotation_dict):
        """Check if input srcs matched annotation.

        Args:
            method: A string of test method name.
            srcs: A list of source file of test.
            annotation_dict: A dictionary record the include and exclude
                             annotations.

        Returns:
            True if input method matched the annotation of input srcs, else
            None.
        """
        include_annotations = annotation_dict.get(
            constants.INCLUDE_ANNOTATION, [])
        exclude_annotations = annotation_dict.get(
            constants.EXCLUDE_ANNOTATION, [])
        for src in srcs:
            include_methods = set()
            src_path = os.path.join(self.root_dir, src)
            # Add methods matched include_annotations.
            for annotation in include_annotations:
                include_methods.update(
                    test_finder_utils.get_annotated_methods(
                        annotation, src_path))
            if exclude_annotations:
                # For exclude annotation, get all the method in the input srcs,
                # and filter out the matched annotation.
                exclude_methods = set()
                all_methods = test_finder_utils.get_java_methods(src_path)
                for annotation in exclude_annotations:
                    exclude_methods.update(
                        test_finder_utils.get_annotated_methods(
                            annotation, src_path))
                include_methods = all_methods - exclude_methods
            if method in include_methods:
                return True
        return False

    def find_test_by_module_and_class(self, module_class):
        """Find the test info given a MODULE:CLASS string.

        Args:
            module_class: A string of form MODULE:CLASS or MODULE:CLASS#METHOD.

        Returns:
            A list of populated TestInfo namedtuple if found, else None.
        """
        if ':' not in module_class:
            return None
        module_name, class_name = module_class.split(':')
        # module_infos is a list with at most 1 element.
        module_infos = self.find_test_by_module_name(module_name)
        module_info = module_infos[0] if module_infos else None
        if not module_info:
            return None
        find_result = None
        # If the target module is NATIVE_TEST, search CC classes only.
        if not self.module_info.is_native_test(module_name):
            # Find by java class.
            find_result = self.find_test_by_class_name(
                class_name, module_info.test_name,
                module_info.data.get(constants.TI_REL_CONFIG))
        # kernel target test is also define as NATIVE_TEST in build system.
        # TODO (b/157210083) Update find_test_by_kernel_class_name method to
        # support gen_rule use case.
        if not find_result:
            find_result = self.find_test_by_kernel_class_name(
                module_name, class_name)
        # Find by cc class.
        if not find_result:
            find_result = self.find_test_by_cc_class_name(
                class_name, module_info.test_name,
                module_info.data.get(constants.TI_REL_CONFIG))
        return find_result

    def find_test_by_package_name(self, package, module_name=None,
                                  rel_config=None):
        """Find the test info given a PACKAGE string.

        Args:
            package: A string of the package name.
            module_name: Optional. A string of the module name.
            ref_config: Optional. A string of rel path of config.

        Returns:
            A list of populated TestInfo namedtuple if found, else None.
        """
        _, methods = test_finder_utils.split_methods(package)
        if methods:
            raise atest_error.MethodWithoutClassError('%s: Method filtering '
                                                      'requires class' % (
                                                          methods))
        # Confirm that packages exists and get user input for multiples.
        if rel_config:
            search_dir = os.path.join(self.root_dir,
                                      os.path.dirname(rel_config))
        else:
            search_dir = self.root_dir
        package_paths = test_finder_utils.run_find_cmd(
            test_finder_utils.FIND_REFERENCE_TYPE.PACKAGE, search_dir, package)
        package_paths = package_paths if package_paths is not None else []
        # Package path will be the full path to the dir represented by package.
        if not package_paths:
            if not module_name:
                return None
            module_paths = self.module_info.get_paths(module_name)
            for rel_module_path in module_paths:
                package_paths.append(os.path.join(self.root_dir, rel_module_path))
        test_filter = frozenset([test_info.TestFilter(package, frozenset())])
        test_infos = []
        for package_path in package_paths:
            tinfo = self._get_test_infos(package_path, rel_config,
                                         module_name, test_filter)
            if tinfo:
                test_infos.extend(tinfo)
        return test_infos if test_infos else None

    def find_test_by_module_and_package(self, module_package):
        """Find the test info given a MODULE:PACKAGE string.

        Args:
            module_package: A string of form MODULE:PACKAGE

        Returns:
            A list of populated TestInfo namedtuple if found, else None.
        """
        module_name, package = module_package.split(':')
        # module_infos is a list with at most 1 element.
        module_infos = self.find_test_by_module_name(module_name)
        module_info = module_infos[0] if module_infos else None
        if not module_info:
            return None
        return self.find_test_by_package_name(
            package, module_info.test_name,
            module_info.data.get(constants.TI_REL_CONFIG))

    def find_test_by_path(self, rel_path):
        """Find the first test info matching the given path.

        Strategy:
            path_to_java_file --> Resolve to CLASS
            path_to_cc_file --> Resolve to CC CLASS
            path_to_module_file -> Resolve to MODULE
            path_to_module_dir -> Resolve to MODULE
            path_to_dir_with_class_files--> Resolve to PACKAGE
            path_to_any_other_dir --> Resolve as MODULE

        Args:
            rel_path: A string of the relative path to $BUILD_TOP.

        Returns:
            A list of populated TestInfo namedtuple if test found, else None
        """
        logging.debug('Finding test by path: %s', rel_path)
        path, methods = test_finder_utils.split_methods(rel_path)
        # TODO: See if this can be generalized and shared with methods above
        # create absolute path from cwd and remove symbolic links
        path = os.path.realpath(path)
        if not os.path.exists(path):
            return None
        if (methods and
                not test_finder_utils.has_method_in_file(path, methods)):
            return None
        dir_path, _ = test_finder_utils.get_dir_path_and_filename(path)
        # Module/Class
        rel_module_dir = test_finder_utils.find_parent_module_dir(
            self.root_dir, dir_path, self.module_info)
        if not rel_module_dir:
            # Try to find unit-test for input path.
            path = os.path.relpath(
                os.path.realpath(rel_path),
                os.environ.get(constants.ANDROID_BUILD_TOP, ''))
            unit_tests = test_finder_utils.find_host_unit_tests(
                self.module_info, path)
            if unit_tests:
                tinfos = []
                for unit_test in unit_tests:
                    tinfo = self._get_test_infos(path, constants.MODULE_CONFIG,
                                                 unit_test, frozenset())
                    if tinfo:
                        tinfos.extend(tinfo)
                return tinfos
            return None
        rel_config = os.path.join(rel_module_dir, constants.MODULE_CONFIG)
        test_filter = self._get_test_info_filter(path, methods,
                                                 rel_module_dir=rel_module_dir)
        return self._get_test_infos(path, rel_config, None, test_filter)

    def find_test_by_cc_class_name(self, class_name, module_name=None,
                                   rel_config=None):
        """Find test files given a cc class name.

        If module_name and rel_config not given, test will be determined
        by looking up the tree for files which has input class.

        Args:
            class_name: A string of the test's class name.
            module_name: Optional. A string of the module name to use.
            rel_config: Optional. A string of module dir no-absolute to repo root.

        Returns:
            A list of populated TestInfo namedtuple if test found, else None.
        """
        # Check if class_name is prepended with file name. If so, trim the
        # prefix and keep only the class_name.
        if '.' in class_name:
            # Assume the class name has a format of file_name.class_name
            class_name = class_name[class_name.rindex('.')+1:]
            logging.info('Search with updated class name: %s', class_name)
        return self.find_test_by_class_name(
            class_name, module_name, rel_config, is_native_test=True)

    def get_testable_modules_with_ld(self, user_input, ld_range=0):
        """Calculate the edit distances of the input and testable modules.

        The user input will be calculated across all testable modules and
        results in integers generated by Levenshtein Distance algorithm.
        To increase the speed of the calculation, a bound can be applied to
        this method to prevent from calculating every testable modules.

        Guessing from typos, e.g. atest atest_unitests, implies a tangible range
        of length that Atest only needs to search within it, and the default of
        the bound is 2.

        Guessing from keywords however, e.g. atest --search Camera, means that
        the uncertainty of the module name is way higher, and Atest should walk
        through all testable modules and return the highest possibilities.

        Args:
            user_input: A string of the user input.
            ld_range: An integer that range the searching scope. If the length
                      of user_input is 10, then Atest will calculate modules of
                      which length is between 8 and 12. 0 is equivalent to
                      unlimited.

        Returns:
            A List of LDs and possible module names. If the user_input is "fax",
            the output will be like:
            [[2, "fog"], [2, "Fix"], [4, "duck"], [7, "Duckies"]]

            Which means the most lilely names of "fax" are fog and Fix(LD=2),
            while Dickies is the most unlikely one(LD=7).
        """
        atest_utils.colorful_print('\nSearching for similar module names using '
                                   'fuzzy search...', constants.CYAN)
        testable_modules = sorted(self.module_info.get_testable_modules(),
                                  key=len)
        lower_bound = len(user_input) - ld_range
        upper_bound = len(user_input) + ld_range
        testable_modules_with_ld = []
        for module_name in testable_modules:
            # Dispose those too short or too lengthy.
            if ld_range != 0:
                if len(module_name) < lower_bound:
                    continue
                if len(module_name) > upper_bound:
                    break
            testable_modules_with_ld.append(
                [test_finder_utils.get_levenshtein_distance(
                    user_input, module_name), module_name])
        return testable_modules_with_ld

    def get_fuzzy_searching_results(self, user_input):
        """Give results which have no more than allowance of edit distances.

        Args:
            user_input: the target module name for fuzzy searching.

        Return:
            A list of guessed modules.
        """
        modules_with_ld = self.get_testable_modules_with_ld(
            user_input, ld_range=constants.LD_RANGE)
        guessed_modules = []
        for _distance, _module in modules_with_ld:
            if _distance <= abs(constants.LD_RANGE):
                guessed_modules.append(_module)
        return guessed_modules

    def find_test_by_config_name(self, config_name):
        """Find test for the given config name.

        Args:
            config_name: A string of the test's config name.

        Returns:
            A list that includes only 1 populated TestInfo namedtuple
            if found, otherwise None.
        """
        for module_name, mod_info in self.module_info.name_to_module_info.items():
            test_configs = mod_info.get(constants.MODULE_TEST_CONFIG, [])
            for test_config in test_configs:
                test_config_name = os.path.splitext(
                    os.path.basename(test_config))[0]
                if test_config_name == config_name:
                    tinfo = test_info.TestInfo(
                        test_name=test_config_name,
                        test_runner=self._TEST_RUNNER,
                        build_targets=self._get_build_targets(module_name,
                                                              test_config),
                        data={constants.TI_REL_CONFIG: test_config,
                              constants.TI_FILTER: frozenset()},
                        compatibility_suites=mod_info.get(
                            constants.MODULE_COMPATIBILITY_SUITES, []))
                    if tinfo:
                        # There should have only one test_config with the same
                        # name in source tree.
                        return [tinfo]
        return None

    def _is_comparted_src(self, path):
        """Check if the input path need to match srcs information in module.

        If path is a folder or android build file, we don't need to compart
        with module's srcs.

        Args:
            path: A string of the test's path.

        Returns:
            True if input path need to match with module's src info, else False.
        """
        if os.path.isdir(path):
            return False
        if atest_utils.is_build_file(path):
            return False
        return True
