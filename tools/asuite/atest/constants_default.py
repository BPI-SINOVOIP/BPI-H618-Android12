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

"""
Various globals used by atest.
"""

# pylint: disable=line-too-long

import os
import re

MODE = 'DEFAULT'

# Result server constants for atest_utils.
RESULT_SERVER = ''
RESULT_SERVER_ARGS = []
RESULT_SERVER_TIMEOUT = 5
# Result arguments if tests are configured in TEST_MAPPING.
TEST_MAPPING_RESULT_SERVER_ARGS = []

# Google service key for gts tests.
GTS_GOOGLE_SERVICE_ACCOUNT = ''

# Arg constants.
WAIT_FOR_DEBUGGER = 'WAIT_FOR_DEBUGGER'
DISABLE_INSTALL = 'DISABLE_INSTALL'
DISABLE_TEARDOWN = 'DISABLE_TEARDOWN'
PRE_PATCH_ITERATIONS = 'PRE_PATCH_ITERATIONS'
POST_PATCH_ITERATIONS = 'POST_PATCH_ITERATIONS'
PRE_PATCH_FOLDER = 'PRE_PATCH_FOLDER'
POST_PATCH_FOLDER = 'POST_PATCH_FOLDER'
SERIAL = 'SERIAL'
SHARDING = 'SHARDING'
ALL_ABI = 'ALL_ABI'
HOST = 'HOST'
CUSTOM_ARGS = 'CUSTOM_ARGS'
DRY_RUN = 'DRY_RUN'
ANDROID_SERIAL = 'ANDROID_SERIAL'
INSTANT = 'INSTANT'
USER_TYPE = 'USER_TYPE'
ITERATIONS = 'ITERATIONS'
RERUN_UNTIL_FAILURE = 'RERUN_UNTIL_FAILURE'
RETRY_ANY_FAILURE = 'RETRY_ANY_FAILURE'
TF_DEBUG = 'TF_DEBUG'
COLLECT_TESTS_ONLY = 'COLLECT_TESTS_ONLY'
TF_TEMPLATE = 'TF_TEMPLATE'
FLAKES_INFO = 'FLAKES_INFO'
TF_EARLY_DEVICE_RELEASE = 'TF_EARLY_DEVICE_RELEASE'
REQUEST_UPLOAD_RESULT = 'REQUEST_UPLOAD_RESULT'
MODULES_IN = 'MODULES-IN-'

# Application exit codes.
EXIT_CODE_SUCCESS = 0
EXIT_CODE_ENV_NOT_SETUP = 1
EXIT_CODE_BUILD_FAILURE = 2
EXIT_CODE_ERROR = 3
EXIT_CODE_TEST_NOT_FOUND = 4
EXIT_CODE_TEST_FAILURE = 5
EXIT_CODE_VERIFY_FAILURE = 6
EXIT_CODE_OUTSIDE_ROOT = 7
EXIT_CODE_AVD_CREATE_FAILURE = 8
EXIT_CODE_AVD_INVALID_ARGS = 9
# Conditions that atest should exit without sending result to metrics.
EXIT_CODES_BEFORE_TEST = [EXIT_CODE_ENV_NOT_SETUP,
                          EXIT_CODE_TEST_NOT_FOUND,
                          EXIT_CODE_OUTSIDE_ROOT,
                          EXIT_CODE_AVD_CREATE_FAILURE,
                          EXIT_CODE_AVD_INVALID_ARGS]

# Codes of specific events. These are exceptions that don't stop anything
# but sending metrics.
ACCESS_CACHE_FAILURE = 101
ACCESS_HISTORY_FAILURE = 102
IMPORT_FAILURE = 103
MLOCATEDB_LOCKED = 104

# Test finder constants.
MODULE_CONFIG = 'AndroidTest.xml'
MODULE_COMPATIBILITY_SUITES = 'compatibility_suites'
MODULE_NAME = 'module_name'
MODULE_PATH = 'path'
MODULE_CLASS = 'class'
MODULE_INSTALLED = 'installed'
MODULE_CLASS_ROBOLECTRIC = 'ROBOLECTRIC'
MODULE_CLASS_NATIVE_TESTS = 'NATIVE_TESTS'
MODULE_CLASS_JAVA_LIBRARIES = 'JAVA_LIBRARIES'
MODULE_TEST_CONFIG = 'test_config'
MODULE_MAINLINE_MODULES = 'test_mainline_modules'
MODULE_DEPENDENCIES = 'dependencies'
MODULE_SRCS = 'srcs'
MODULE_IS_UNIT_TEST = 'is_unit_test'

# Env constants
ANDROID_BUILD_TOP = 'ANDROID_BUILD_TOP'
ANDROID_OUT = 'OUT'
ANDROID_OUT_DIR = 'OUT_DIR'
ANDROID_OUT_DIR_COMMON_BASE = 'OUT_DIR_COMMON_BASE'
ANDROID_HOST_OUT = 'ANDROID_HOST_OUT'
ANDROID_PRODUCT_OUT = 'ANDROID_PRODUCT_OUT'
ANDROID_TARGET_PRODUCT = 'TARGET_PRODUCT'

# Test Info data keys
# Value of include-filter option.
TI_FILTER = 'filter'
TI_REL_CONFIG = 'rel_config'
TI_MODULE_CLASS = 'module_class'
# Value of module-arg option
TI_MODULE_ARG = 'module-arg'

# Google TF
GTF_MODULE = 'google-tradefed'
GTF_TARGET = 'google-tradefed-core'

# TEST_MAPPING filename
TEST_MAPPING = 'TEST_MAPPING'
# Test group for tests in TEST_MAPPING
TEST_GROUP_PRESUBMIT = 'presubmit'
TEST_GROUP_PRESUBMIT_LARGE = 'presubmit-large'
TEST_GROUP_POSTSUBMIT = 'postsubmit'
TEST_GROUP_ALL = 'all'
DEFAULT_TEST_GROUPS = [TEST_GROUP_PRESUBMIT, TEST_GROUP_PRESUBMIT_LARGE]
# Key in TEST_MAPPING file for a list of imported TEST_MAPPING file
TEST_MAPPING_IMPORTS = 'imports'

# TradeFed command line args
TF_INCLUDE_FILTER_OPTION = 'include-filter'
TF_EXCLUDE_FILTER_OPTION = 'exclude-filter'
TF_INCLUDE_FILTER = '--include-filter'
TF_EXCLUDE_FILTER = '--exclude-filter'
TF_ATEST_INCLUDE_FILTER = '--atest-include-filter'
TF_ATEST_INCLUDE_FILTER_VALUE_FMT = '{test_name}:{test_filter}'
TF_MODULE_ARG = '--module-arg'
TF_MODULE_ARG_VALUE_FMT = '{test_name}:{option_name}:{option_value}'
TF_SUITE_FILTER_ARG_VALUE_FMT = '"{test_name} {option_value}"'
TF_SKIP_LOADING_CONFIG_JAR = '--skip-loading-config-jar'
TF_MODULE_FILTER = '--module'

# Suite Plans
SUITE_PLANS = frozenset(['cts'])

# Constants of Steps
REBUILD_MODULE_INFO_FLAG = '--rebuild-module-info'
BUILD_STEP = 'build'
INSTALL_STEP = 'install'
TEST_STEP = 'test'
ALL_STEPS = [BUILD_STEP, INSTALL_STEP, TEST_STEP]

# ANSI code shift for colorful print
BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE = range(8)

# Types of Levenshetine Distance Cost
COST_TYPO = (1, 1, 1)
COST_SEARCH = (8, 1, 5)
LD_RANGE = 2

# Value of TestInfo install_locations.
DEVICELESS_TEST = 'host'
DEVICE_TEST = 'device'
BOTH_TEST = 'both'

# Metrics
NO_METRICS_ARG = '--no-metrics'
METRICS_URL = 'http://asuite-218222.appspot.com/atest/metrics'
EXTERNAL = 'EXTERNAL_RUN'
INTERNAL = 'INTERNAL_RUN'
INTERNAL_EMAIL = '@google.com'
INTERNAL_HOSTNAME = ['.google.com', 'c.googlers.com']
CONTENT_LICENSES_URL = 'https://source.android.com/setup/start/licenses'
CONTRIBUTOR_AGREEMENT_URL = {
    'INTERNAL': 'https://cla.developers.google.com/',
    'EXTERNAL': 'https://opensource.google.com/docs/cla/'
}
PRIVACY_POLICY_URL = 'https://policies.google.com/privacy'
TERMS_SERVICE_URL = 'https://policies.google.com/terms'
TOOL_NAME = 'atest'
USER_FROM_TOOL = 'USER_FROM_TOOL'
TF_PREPARATION = 'tf-preparation'

# Detect type for local_detect_event.
# Next expansion : DETECT_TYPE_XXX = 8
DETECT_TYPE_BUG_DETECTED = 0
DETECT_TYPE_ACLOUD_CREATE = 1
DETECT_TYPE_FIND_BUILD = 2
DETECT_TYPE_NO_FLAKE = 3
DETECT_TYPE_HAS_FLAKE = 4
DETECT_TYPE_TF_TEARDOWN_LOGCAT = 5
DETECT_TYPE_REBUILD_MODULE_INFO = 6
DETECT_TYPE_NOT_REBUILD_MODULE_INFO = 7
DETECT_TYPE_ONLY_BUILD_MODULE_INFO = 8
# XTS suite types encode from 100 to 199
DETECT_TYPE_XTS_SUITE = {'cts': 101,
                         'vts': 104}

# Considering a trade-off between speed and size, we set UPPER_LIMIT to 100000
# to make maximum file space 10M(100000(records)*100(byte/record)) at most.
# Therefore, to update history file will spend 1 sec at most in each run.
UPPER_LIMIT = 100000
TRIM_TO_SIZE = 50000

# VTS plans
VTS_STAGING_PLAN = 'vts-staging-default'

# TreeHugger TEST_MAPPING SUITE_PLANS
TEST_MAPPING_SUITES = ['device-tests', 'general-tests']

# VTS10 TF
VTS_TF_MODULE = 'vts10-tradefed'

# VTS TF
VTS_CORE_TF_MODULE = 'vts-tradefed'

# VTS suite set
VTS_CORE_SUITE = 'vts'

# ATest TF
ATEST_TF_MODULE = 'atest-tradefed'

# Build environment variable for each build on ATest
# With RECORD_ALL_DEPS enabled, ${ANDROID_PRODUCT_OUT}/module-info.json will
# generate modules' dependencies info when make.
# With SOONG_COLLECT_JAVA_DEPS enabled, out/soong/module_bp_java_deps.json will
# be generated when make.
ATEST_BUILD_ENV = {'RECORD_ALL_DEPS':'true', 'SOONG_COLLECT_JAVA_DEPS':'true',
                   'SOONG_COLLECT_CC_DEPS':'true'}

# Atest index path and relative dirs/caches.
INDEX_DIR = os.path.join(os.getenv(ANDROID_HOST_OUT, ''), 'indexes')
LOCATE_CACHE = os.path.join(INDEX_DIR, 'mlocate.db')
LOCATE_CACHE_MD5 = os.path.join(INDEX_DIR, 'mlocate.md5')
INT_INDEX = os.path.join(INDEX_DIR, 'integration.idx')
CLASS_INDEX = os.path.join(INDEX_DIR, 'classes.idx')
CC_CLASS_INDEX = os.path.join(INDEX_DIR, 'cc_classes.idx')
PACKAGE_INDEX = os.path.join(INDEX_DIR, 'packages.idx')
QCLASS_INDEX = os.path.join(INDEX_DIR, 'fqcn.idx')
MODULE_INDEX = os.path.join(INDEX_DIR, 'modules.idx')
VERSION_FILE = os.path.join(os.path.dirname(__file__), 'VERSION')

# Regeular Expressions
CC_EXT_RE = re.compile(r'.*\.(cc|cpp)$')
JAVA_EXT_RE = re.compile(r'.*\.(java|kt)$')
# e.g. /path/to/ccfile.cc: TEST_F(test_name, method_name){
CC_OUTPUT_RE = re.compile(r'(?P<file_path>/.*):\s*TEST(_F|_P)?[ ]*\('
                          r'(?P<test_name>\w+)\s*,\s*(?P<method_name>\w+)\)'
                          r'\s*\{')
CC_GREP_RE = r'^[ ]*TEST(_P|_F)?[ ]*\([[:alnum:]].*,'
# e.g. /path/to/Javafile.java:package com.android.settings.accessibility
# grab the path, Javafile(class) and com.android.settings.accessibility(package)
CLASS_OUTPUT_RE = re.compile(r'(?P<java_path>.*/(?P<class>[A-Z]\w+)\.\w+)[:].*')
QCLASS_OUTPUT_RE = re.compile(r'(?P<java_path>.*/(?P<class>[A-Z]\w+)\.\w+)'
                              r'[:]\s*package\s+(?P<package>[^(;|\s)]+)\s*')
PACKAGE_OUTPUT_RE = re.compile(r'(?P<java_dir>/.*/).*[.](java|kt)[:]\s*package\s+'
                               r'(?P<package>[^(;|\s)]+)\s*')

ATEST_RESULT_ROOT = '/tmp/atest_result'
ATEST_TEST_RECORD_PROTO = 'test_record.proto'
LATEST_RESULT_FILE = os.path.join(ATEST_RESULT_ROOT, 'LATEST', 'test_result')
ACLOUD_REPORT_FILE_RE = re.compile(r'.*--report[_-]file(=|\s+)(?P<report_file>[\w/.]+)')
TEST_WITH_MAINLINE_MODULES_RE = re.compile(r'(?P<test>.*)\[(?P<mainline_modules>.*'
                                           r'[.](apk|apks|apex))\]$')

# Tests list which need vts_kernel_tests as test dependency
REQUIRED_KERNEL_TEST_MODULES = [
    'vts_ltp_test_arm',
    'vts_ltp_test_arm_64',
    'vts_linux_kselftest_arm_32',
    'vts_linux_kselftest_arm_64',
    'vts_linux_kselftest_x86_32',
    'vts_linux_kselftest_x86_64',
    'vts_ltp_test_arm_64_lowmem',
    'vts_ltp_test_arm_64_hwasan',
    'vts_ltp_test_arm_64_lowmem_hwasan',
    'vts_ltp_test_arm_lowmem',
    'vts_ltp_test_x86_64',
    'vts_ltp_test_x86'
]

# XTS suite set dependency.
SUITE_DEPS = {}

# Tradefed log file name term.
TF_HOST_LOG = 'host_log_*'

# Flake service par path
FLAKE_SERVICE_PATH = '/foo'
FLAKE_TMP_PATH = '/tmp'
FLAKE_FILE = 'flakes_info.par'
FLAKE_TARGET = 'aosp_cf_x86_phone-userdebug'
FLAKE_BRANCH = 'aosp-master'
FLAKE_TEST_NAME = 'suite/test-mapping-presubmit-retry_cloud-tf'
FLAKE_PERCENT = 'flake_percent'
FLAKE_POSTSUBMIT = 'postsubmit_flakes_per_week'

# cert status command
CERT_STATUS_CMD = ''

ASUITE_REPO_PROJECT_NAME = 'platform/tools/asuite'

# logstorage api scope.
SCOPE_BUILD_API_SCOPE = ''
STORAGE_API_VERSION = ''
STORAGE_SERVICE_NAME = ''
DO_NOT_UPLOAD = 'DO_NOT_UPLOAD'
CLIENT_ID = ''
CLIENT_SECRET = ''
CREDENTIAL_FILE_NAME = ''
TOKEN_FILE_PATH = ''
INVOCATION_ID = 'INVOCATION_ID'
WORKUNIT_ID = 'WORKUNIT_ID'
RESULT_LINK = ''
TF_GLOBAL_CONFIG = ''
UPLOAD_TEST_RESULT_MSG = 'Upload test result?'

# messages that share among libraries.
REBUILD_MODULE_INFO_MSG = ('(This can happen after a repo sync or if the test'
                           ' is new. Running with "{}" may resolve the issue.)')

# AndroidJUnitTest related argument.
ANDROID_JUNIT_CLASS = 'com.android.tradefed.testtype.AndroidJUnitTest'
INCLUDE_ANNOTATION = 'include-annotation'
EXCLUDE_ANNOTATION = 'exclude-annotation'
SUPPORTED_FILTERS = [INCLUDE_ANNOTATION, EXCLUDE_ANNOTATION]

# Tradefed config-descriptor metadata.
CONFIG_DESCRIPTOR = 'config-descriptor:metadata'
PARAMETER_KEY = 'parameter'

# Tradefed related constant.
TF_TEST_ARG = '--test-arg'
TF_AND_JUNIT_CLASS = 'com.android.tradefed.testtype.AndroidJUnitTest'
TF_EXCLUDE_ANNOTATE = 'exclude-annotation'
INSTANT_MODE_ANNOTATE = 'android.platform.test.annotations.AppModeInstant'
TF_PARA_INSTANT_APP = 'instant_app'
TF_PARA_SECOND_USR = 'secondary_user'
TF_PARA_MULTIABI = 'multi_abi'
DEFAULT_EXCLUDE_PARAS = {TF_PARA_INSTANT_APP,
                         TF_PARA_SECOND_USR,
                         TF_PARA_MULTIABI
                         }
DEFAULT_EXCLUDE_NOT_PARAS = {'not_' + TF_PARA_INSTANT_APP,
                            'not_' + TF_PARA_SECOND_USR,
                            'not_' + TF_PARA_MULTIABI}
