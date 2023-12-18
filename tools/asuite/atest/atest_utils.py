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
Utility functions for atest.
"""


# pylint: disable=import-outside-toplevel
# pylint: disable=too-many-lines

from __future__ import print_function

import fnmatch
import hashlib
import importlib
import itertools
import json
import logging
import os
import pickle
import re
import shutil
import subprocess
import sys
import sysconfig
import time
import zipfile

import xml.etree.ElementTree as ET

from distutils.util import strtobool

# This is a workaround of b/144743252, where the http.client failed to loaded
# because the googleapiclient was found before the built-in libs; enabling
# embedded launcher(b/135639220) has not been reliable and other issue will
# raise.
# The workaround is repositioning the built-in libs before other 3rd libs in
# PYTHONPATH(sys.path) to eliminate the symptom of failed loading http.client.
sys.path.insert(0, os.path.dirname(sysconfig.get_paths()['purelib']))
sys.path.insert(0, os.path.dirname(sysconfig.get_paths()['stdlib']))

#pylint: disable=wrong-import-position
import atest_decorator
import atest_error
import constants

# This proto related module will be auto generated in build time.
# pylint: disable=no-name-in-module
# pylint: disable=import-error
try:
    from tools.asuite.atest.tf_proto import test_record_pb2
except ImportError as err:
    pass
# b/147562331 only occurs when running atest in source code. We don't encourge
# the users to manually "pip3 install protobuf", therefore when the exception
# occurs, we don't collect data and the tab completion is for args is silence.
try:
    from metrics import metrics
    from metrics import metrics_base
    from metrics import metrics_utils
except ImportError as err:
    # TODO(b/182854938): remove this ImportError after refactor metrics dir.
    try:
        from asuite.metrics import metrics
        from asuite.metrics import metrics_base
        from asuite.metrics import metrics_utils
    except ImportError as err:
        # This exception occurs only when invoking atest in source code.
        print("You shouldn't see this message unless you ran 'atest-src'."
              "To resolve the issue, please run:\n\t{}\n"
              "and try again.".format('pip3 install protobuf'))
        print('Import error, %s', err)
        print('sys.path: %s', sys.path)
        sys.exit(constants.IMPORT_FAILURE)

_BASH_RESET_CODE = '\033[0m\n'
# Arbitrary number to limit stdout for failed runs in _run_limited_output.
# Reason for its use is that the make command itself has its own carriage
# return output mechanism that when collected line by line causes the streaming
# full_output list to be extremely large.
_FAILED_OUTPUT_LINE_LIMIT = 100
# Regular expression to match the start of a ninja compile:
# ex: [ 99% 39710/39711]
_BUILD_COMPILE_STATUS = re.compile(r'\[\s*(\d{1,3}%\s+)?\d+/\d+\]')
_BUILD_FAILURE = 'FAILED: '
CMD_RESULT_PATH = os.path.join(os.environ.get(constants.ANDROID_BUILD_TOP,
                                              os.getcwd()),
                               'tools/asuite/atest/test_data',
                               'test_commands.json')
BUILD_TOP_HASH = hashlib.md5(os.environ.get(constants.ANDROID_BUILD_TOP, '').
                             encode()).hexdigest()
TEST_INFO_CACHE_ROOT = os.path.join(os.path.expanduser('~'), '.atest',
                                    'info_cache', BUILD_TOP_HASH[:8])
_DEFAULT_TERMINAL_WIDTH = 80
_DEFAULT_TERMINAL_HEIGHT = 25
_BUILD_CMD = 'build/soong/soong_ui.bash'
_FIND_MODIFIED_FILES_CMDS = (
    "cd {};"
    "local_branch=$(git rev-parse --abbrev-ref HEAD);"
    "remote_branch=$(git branch -r | grep '\\->' | awk '{{print $1}}');"
    # Get the number of commits from local branch to remote branch.
    "ahead=$(git rev-list --left-right --count $local_branch...$remote_branch "
    "| awk '{{print $1}}');"
    # Get the list of modified files from HEAD to previous $ahead generation.
    "git diff HEAD~$ahead --name-only")
_ANDROID_BUILD_EXT = ('.bp', '.mk')

# Set of special chars for various purposes.
_REGEX_CHARS = {'[', '(', '{', '|', '\\', '*', '?', '+', '^'}
_WILDCARD_CHARS = {'?', '*'}

def get_build_cmd():
    """Compose build command with no-absolute path and flag "--make-mode".

    Returns:
        A list of soong build command.
    """
    make_cmd = ('%s/%s' %
                (os.path.relpath(os.environ.get(
                    constants.ANDROID_BUILD_TOP, os.getcwd()), os.getcwd()),
                 _BUILD_CMD))
    return [make_cmd, '--make-mode']


def _capture_fail_section(full_log):
    """Return the error message from the build output.

    Args:
        full_log: List of strings representing full output of build.

    Returns:
        capture_output: List of strings that are build errors.
    """
    am_capturing = False
    capture_output = []
    for line in full_log:
        if am_capturing and _BUILD_COMPILE_STATUS.match(line):
            break
        if am_capturing or line.startswith(_BUILD_FAILURE):
            capture_output.append(line)
            am_capturing = True
            continue
    return capture_output


def _capture_limited_output(full_log):
    """Return the limited error message from capture_failed_section.

    Args:
        full_log: List of strings representing full output of build.

    Returns:
        output: List of strings that are build errors.
    """
    # Parse out the build error to output.
    output = _capture_fail_section(full_log)
    if not output:
        output = full_log
    if len(output) >= _FAILED_OUTPUT_LINE_LIMIT:
        output = output[-_FAILED_OUTPUT_LINE_LIMIT:]
    output = 'Output (may be trimmed):\n%s' % ''.join(output)
    return output


# TODO: b/187122993 refine subprocess with 'with-statement' in fixit week.
def _run_limited_output(cmd, env_vars=None):
    """Runs a given command and streams the output on a single line in stdout.

    Args:
        cmd: A list of strings representing the command to run.
        env_vars: Optional arg. Dict of env vars to set during build.

    Raises:
        subprocess.CalledProcessError: When the command exits with a non-0
            exitcode.
    """
    # Send stderr to stdout so we only have to deal with a single pipe.
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, env=env_vars)
    sys.stdout.write('\n')
    term_width, _ = get_terminal_size()
    white_space = " " * int(term_width)
    full_output = []
    while proc.poll() is None:
        line = proc.stdout.readline().decode('utf-8')
        # Readline will often return empty strings.
        if not line:
            continue
        full_output.append(line)
        # Trim the line to the width of the terminal.
        # Note: Does not handle terminal resizing, which is probably not worth
        #       checking the width every loop.
        if len(line) >= term_width:
            line = line[:term_width - 1]
        # Clear the last line we outputted.
        sys.stdout.write('\r%s\r' % white_space)
        sys.stdout.write('%s' % line.strip())
        sys.stdout.flush()
    # Reset stdout (on bash) to remove any custom formatting and newline.
    sys.stdout.write(_BASH_RESET_CODE)
    sys.stdout.flush()
    # Wait for the Popen to finish completely before checking the returncode.
    proc.wait()
    if proc.returncode != 0:
        # get error log from "OUT_DIR/error.log"
        error_log_file = os.path.join(get_build_out_dir(), "error.log")
        output = []
        if os.path.isfile(error_log_file):
            if os.stat(error_log_file).st_size > 0:
                with open(error_log_file) as f:
                    output = f.read()
        if not output:
            output = _capture_limited_output(full_output)
        raise subprocess.CalledProcessError(proc.returncode, cmd, output)


def get_build_out_dir():
    """Get android build out directory.

    Returns:
        String of the out directory.
    """
    build_top = os.environ.get(constants.ANDROID_BUILD_TOP)
    # Get the out folder if user specified $OUT_DIR
    custom_out_dir = os.environ.get(constants.ANDROID_OUT_DIR)
    custom_out_dir_common_base = os.environ.get(
        constants.ANDROID_OUT_DIR_COMMON_BASE)
    user_out_dir = None
    if custom_out_dir:
        if os.path.isabs(custom_out_dir):
            user_out_dir = custom_out_dir
        else:
            user_out_dir = os.path.join(build_top, custom_out_dir)
    elif custom_out_dir_common_base:
        # When OUT_DIR_COMMON_BASE is set, the output directory for each
        # separate source tree is named after the directory holding the
        # source tree.
        build_top_basename = os.path.basename(build_top)
        if os.path.isabs(custom_out_dir_common_base):
            user_out_dir = os.path.join(custom_out_dir_common_base,
                                        build_top_basename)
        else:
            user_out_dir = os.path.join(build_top, custom_out_dir_common_base,
                                        build_top_basename)
    if user_out_dir:
        return user_out_dir
    return os.path.join(build_top, "out")


def build(build_targets, verbose=False, env_vars=None):
    """Shell out and make build_targets.

    Args:
        build_targets: A set of strings of build targets to make.
        verbose: Optional arg. If True output is streamed to the console.
                 If False, only the last line of the build output is outputted.
        env_vars: Optional arg. Dict of env vars to set during build.

    Returns:
        Boolean of whether build command was successful, True if nothing to
        build.
    """
    if not build_targets:
        logging.debug('No build targets, skipping build.')
        return True
    full_env_vars = os.environ.copy()
    if env_vars:
        full_env_vars.update(env_vars)
    print('\n%s\n%s' % (colorize("Building Dependencies...", constants.CYAN),
                        ', '.join(build_targets)))
    logging.debug('Building Dependencies: %s', ' '.join(build_targets))
    cmd = get_build_cmd() + list(build_targets)
    logging.debug('Executing command: %s', cmd)
    try:
        if verbose:
            subprocess.check_call(cmd, stderr=subprocess.STDOUT,
                                  env=full_env_vars)
        else:
            # TODO: Save output to a log file.
            _run_limited_output(cmd, env_vars=full_env_vars)
        logging.info('Build successful')
        return True
    except subprocess.CalledProcessError as err:
        logging.error('Error building: %s', build_targets)
        print(constants.REBUILD_MODULE_INFO_MSG.format(
            colorize(constants.REBUILD_MODULE_INFO_FLAG,
                     constants.RED)))
        if err.output:
            logging.error(err.output)
        return False


def _can_upload_to_result_server():
    """Return True if we can talk to result server."""
    # TODO: Also check if we have a slow connection to result server.
    if constants.RESULT_SERVER:
        try:
            from urllib.request import urlopen
            urlopen(constants.RESULT_SERVER,
                    timeout=constants.RESULT_SERVER_TIMEOUT).close()
            return True
        # pylint: disable=broad-except
        except Exception as err:
            logging.debug('Talking to result server raised exception: %s', err)
    return False


# pylint: disable=unused-argument
def get_result_server_args(for_test_mapping=False):
    """Return list of args for communication with result server.

    Args:
        for_test_mapping: True if the test run is for Test Mapping to include
            additional reporting args. Default is False.
    """
    # Customize test mapping argument here if needed.
    return constants.RESULT_SERVER_ARGS

def sort_and_group(iterable, key):
    """Sort and group helper function."""
    return itertools.groupby(sorted(iterable, key=key), key=key)


def is_test_mapping(args):
    """Check if the atest command intends to run tests in test mapping.

    When atest runs tests in test mapping, it must have at most one test
    specified. If a test is specified, it must be started with  `:`,
    which means the test value is a test group name in TEST_MAPPING file, e.g.,
    `:postsubmit`.

    If --host-unit-test-only be applied, it's not test mapping.
    If any test mapping options is specified, the atest command must also be
    set to run tests in test mapping files.

    Args:
        args: arg parsed object.

    Returns:
        True if the args indicates atest shall run tests in test mapping. False
        otherwise.
    """
    return (
        not args.host_unit_test_only and
        (args.test_mapping or
        args.include_subdirs or
        not args.tests or
        (len(args.tests) == 1 and args.tests[0][0] == ':')))


@atest_decorator.static_var("cached_has_colors", {})
def _has_colors(stream):
    """Check the output stream is colorful.

    Args:
        stream: The standard file stream.

    Returns:
        True if the file stream can interpreter the ANSI color code.
    """
    cached_has_colors = _has_colors.cached_has_colors
    if stream in cached_has_colors:
        return cached_has_colors[stream]
    cached_has_colors[stream] = True
    # Following from Python cookbook, #475186
    if not hasattr(stream, "isatty"):
        cached_has_colors[stream] = False
        return False
    if not stream.isatty():
        # Auto color only on TTYs
        cached_has_colors[stream] = False
        return False
    try:
        import curses
        curses.setupterm()
        cached_has_colors[stream] = curses.tigetnum("colors") > 2
    # pylint: disable=broad-except
    except Exception as err:
        logging.debug('Checking colorful raised exception: %s', err)
        cached_has_colors[stream] = False
    return cached_has_colors[stream]


def colorize(text, color, highlight=False):
    """ Convert to colorful string with ANSI escape code.

    Args:
        text: A string to print.
        color: ANSI code shift for colorful print. They are defined
               in constants_default.py.
        highlight: True to print with highlight.

    Returns:
        Colorful string with ANSI escape code.
    """
    clr_pref = '\033[1;'
    clr_suff = '\033[0m'
    has_colors = _has_colors(sys.stdout)
    if has_colors:
        if highlight:
            ansi_shift = 40 + color
        else:
            ansi_shift = 30 + color
        clr_str = "%s%dm%s%s" % (clr_pref, ansi_shift, text, clr_suff)
    else:
        clr_str = text
    return clr_str


def colorful_print(text, color, highlight=False, auto_wrap=True):
    """Print out the text with color.

    Args:
        text: A string to print.
        color: ANSI code shift for colorful print. They are defined
               in constants_default.py.
        highlight: True to print with highlight.
        auto_wrap: If True, Text wraps while print.
    """
    output = colorize(text, color, highlight)
    if auto_wrap:
        print(output)
    else:
        print(output, end="")


def get_terminal_size():
    """Get terminal size and return a tuple.

    Returns:
        2 integers: the size of X(columns) and Y(lines/rows).
    """
    # Determine the width of the terminal. We'll need to clear this many
    # characters when carriage returning. Set default value as 80.
    columns, rows = shutil.get_terminal_size(
        fallback=(_DEFAULT_TERMINAL_WIDTH,
                  _DEFAULT_TERMINAL_HEIGHT))
    return columns, rows


def is_external_run():
    # TODO(b/133905312): remove this function after aidegen calling
    #       metrics_base.get_user_type directly.
    """Check is external run or not.

    Determine the internal user by passing at least one check:
      - whose git mail domain is from google
      - whose hostname is from google
    Otherwise is external user.

    Returns:
        True if this is an external run, False otherwise.
    """
    return metrics_base.get_user_type() == metrics_base.EXTERNAL_USER


def print_data_collection_notice():
    """Print the data collection notice."""
    anonymous = ''
    user_type = 'INTERNAL'
    if metrics_base.get_user_type() == metrics_base.EXTERNAL_USER:
        anonymous = ' anonymous'
        user_type = 'EXTERNAL'
    notice = ('  We collect%s usage statistics in accordance with our Content '
              'Licenses (%s), Contributor License Agreement (%s), Privacy '
              'Policy (%s) and Terms of Service (%s).'
             ) % (anonymous,
                  constants.CONTENT_LICENSES_URL,
                  constants.CONTRIBUTOR_AGREEMENT_URL[user_type],
                  constants.PRIVACY_POLICY_URL,
                  constants.TERMS_SERVICE_URL
                 )
    print(delimiter('=', 18, prenl=1))
    colorful_print("Notice:", constants.RED)
    colorful_print("%s" % notice, constants.GREEN)
    print(delimiter('=', 18, postnl=1))


def handle_test_runner_cmd(input_test, test_cmds, do_verification=False,
                           result_path=CMD_RESULT_PATH):
    """Handle the runner command of input tests.

    Args:
        input_test: A string of input tests pass to atest.
        test_cmds: A list of strings for running input tests.
        do_verification: A boolean to indicate the action of this method.
                         True: Do verification without updating result map and
                               raise DryRunVerificationError if verifying fails.
                         False: Update result map, if the former command is
                                different with current command, it will confirm
                                with user if they want to update or not.
        result_path: The file path for saving result.
    """
    full_result_content = {}
    if os.path.isfile(result_path):
        with open(result_path) as json_file:
            full_result_content = json.load(json_file)
    former_test_cmds = full_result_content.get(input_test, [])
    test_cmds = _normalize(test_cmds)
    former_test_cmds = _normalize(former_test_cmds)
    if not _are_identical_cmds(test_cmds, former_test_cmds):
        if do_verification:
            raise atest_error.DryRunVerificationError(
                'Dry run verification failed, former commands: {}'.format(
                    former_test_cmds))
        if former_test_cmds:
            # If former_test_cmds is different from test_cmds, ask users if they
            # are willing to update the result.
            print('Former cmds = %s' % former_test_cmds)
            print('Current cmds = %s' % test_cmds)
            if not prompt_with_yn_result('Do you want to update former result '
                                         'to the latest one?', True):
                print('SKIP updating result!!!')
                return
    else:
        # If current commands are the same as the formers, no need to update
        # result.
        return
    full_result_content[input_test] = test_cmds
    with open(result_path, 'w') as outfile:
        json.dump(full_result_content, outfile, indent=0)
        print('Save result mapping to %s' % result_path)

def _normalize(cmd_list):
    """Method that normalize commands. Note that '--atest-log-file-path' is not
    considered a critical argument, therefore, it will be removed during
    the comparison. Also, atest can be ran in any place, so verifying relative
    path, LD_LIBRARY_PATH, and --proto-output-file is regardless as well.

    Args:
        cmd_list: A list with one element. E.g. ['cmd arg1 arg2 True']

    Returns:
        A list with elements. E.g. ['cmd', 'arg1', 'arg2', 'True']
    """
    _cmd = ' '.join(cmd_list).split()
    for cmd in _cmd:
        if cmd.startswith('--atest-log-file-path'):
            _cmd.remove(cmd)
            continue
        if cmd.startswith('LD_LIBRARY_PATH='):
            _cmd.remove(cmd)
            continue
        if cmd.startswith('--proto-output-file='):
            _cmd.remove(cmd)
            continue
        if _BUILD_CMD in cmd:
            _cmd.remove(cmd)
            _cmd.append(os.path.join('./', _BUILD_CMD))
            continue
    return _cmd

def _are_identical_cmds(current_cmds, former_cmds):
    """Tell two commands are identical.

    Args:
        current_cmds: A list of strings for running input tests.
        former_cmds: A list of strings recorded from the previous run.

    Returns:
        True if both commands are identical, False otherwise.
    """
    # Always sort cmd list to make it comparable.
    current_cmds.sort()
    former_cmds.sort()
    return current_cmds == former_cmds

def _get_hashed_file_name(main_file_name):
    """Convert the input string to a md5-hashed string. If file_extension is
       given, returns $(hashed_string).$(file_extension), otherwise
       $(hashed_string).cache.

    Args:
        main_file_name: The input string need to be hashed.

    Returns:
        A string as hashed file name with .cache file extension.
    """
    hashed_fn = hashlib.md5(str(main_file_name).encode())
    hashed_name = hashed_fn.hexdigest()
    return hashed_name + '.cache'

def md5sum(filename):
    """Generate MD5 checksum of a file.

    Args:
        name: A string of a filename.

    Returns:
        A string of hashed MD5 checksum.
    """
    if not os.path.isfile(filename):
        return ""
    with open(filename, 'rb') as target:
        content = target.read()
    return hashlib.md5(content).hexdigest()

def check_md5(check_file, missing_ok=False):
    """Method equivalent to 'md5sum --check /file/to/check'.

    Args:
        check_file: A string of filename that stores filename and its
                   md5 checksum.
        missing_ok: A boolean that considers OK even when the check_file does
                    not exist. Using missing_ok=True allows ignoring md5 check
                    especially for initial run that the check_file has not yet
                    generated. Using missing_ok=False ensures the consistency of
                    files, and guarantees the process is successfully completed.

    Returns:
        When missing_ok is True (soft check):
          - True if the checksum is consistent with the actual MD5, even the
            check_file is missing or not a valid JSON.
          - False when the checksum is inconsistent with the actual MD5.
        When missing_ok is False (ensure the process completed properly):
          - True if the checksum is consistent with the actual MD5.
          - False otherwise.
    """
    if not os.path.isfile(check_file):
        if not missing_ok:
            logging.warning(
                'Unable to verify: %s not found.', check_file)
        return missing_ok
    if not is_valid_json_file(check_file):
        logging.warning(
            'Unable to verify: %s invalid JSON format.', check_file)
        return missing_ok
    with open(check_file, 'r+') as _file:
        content = json.load(_file)
        for filename, md5 in content.items():
            if md5sum(filename) != md5:
                logging.debug('%s has altered.', filename)
                return False
    return True

def save_md5(filenames, save_file):
    """Method equivalent to 'md5sum file1 file2 > /file/to/check'

    Args:
        filenames: A list of filenames.
        save_file: Filename for storing files and their md5 checksums.
    """
    if os.path.isfile(save_file):
        os.remove(save_file)
    data = {}
    for name in filenames:
        if not os.path.isfile(name):
            logging.warning('%s is not a file.', name)
        data.update({name: md5sum(name)})
    with open(save_file, 'w+') as _file:
        json.dump(data, _file)

def get_cache_root():
    """Get the root path dir for cache.

    Use branch and target information as cache_root.
    The path will look like ~/.atest/info_cache/$hash(branch+target)

    Returns:
        A string of the path of the root dir of cache.
    """
    manifest_branch = get_manifest_branch()
    if not manifest_branch:
        manifest_branch = os.environ.get(
            constants.ANDROID_BUILD_TOP, constants.ANDROID_BUILD_TOP)
    # target
    build_target = os.path.basename(
        os.environ.get(constants.ANDROID_PRODUCT_OUT,
                       constants.ANDROID_PRODUCT_OUT))
    branch_target_hash = hashlib.md5(
        (constants.MODE + manifest_branch + build_target).encode()).hexdigest()
    return os.path.join(os.path.expanduser('~'), '.atest','info_cache',
                        branch_target_hash[:8])

def get_test_info_cache_path(test_reference, cache_root=None):
    """Get the cache path of the desired test_infos.

    Args:
        test_reference: A string of the test.
        cache_root: Folder path where stores caches.

    Returns:
        A string of the path of test_info cache.
    """
    if not cache_root:
        cache_root = get_cache_root()
    return os.path.join(cache_root, _get_hashed_file_name(test_reference))

def update_test_info_cache(test_reference, test_infos,
                           cache_root=None):
    """Update cache content which stores a set of test_info objects through
       pickle module, each test_reference will be saved as a cache file.

    Args:
        test_reference: A string referencing a test.
        test_infos: A set of TestInfos.
        cache_root: Folder path for saving caches.
    """
    if not cache_root:
        cache_root = get_cache_root()
    if not os.path.isdir(cache_root):
        os.makedirs(cache_root)
    cache_path = get_test_info_cache_path(test_reference, cache_root)
    # Save test_info to files.
    try:
        with open(cache_path, 'wb') as test_info_cache_file:
            logging.debug('Saving cache %s.', cache_path)
            pickle.dump(test_infos, test_info_cache_file, protocol=2)
    except (pickle.PicklingError, TypeError, IOError) as err:
        # Won't break anything, just log this error, and collect the exception
        # by metrics.
        logging.debug('Exception raised: %s', err)
        metrics_utils.handle_exc_and_send_exit_event(
            constants.ACCESS_CACHE_FAILURE)


def load_test_info_cache(test_reference, cache_root=None):
    """Load cache by test_reference to a set of test_infos object.

    Args:
        test_reference: A string referencing a test.
        cache_root: Folder path for finding caches.

    Returns:
        A list of TestInfo namedtuple if cache found, else None.
    """
    if not cache_root:
        cache_root = get_cache_root()
    cache_file = get_test_info_cache_path(test_reference, cache_root)
    if os.path.isfile(cache_file):
        logging.debug('Loading cache %s.', cache_file)
        try:
            with open(cache_file, 'rb') as config_dictionary_file:
                return pickle.load(config_dictionary_file, encoding='utf-8')
        except (pickle.UnpicklingError,
                ValueError,
                TypeError,
                EOFError,
                IOError) as err:
            # Won't break anything, just remove the old cache, log this error,
            # and collect the exception by metrics.
            logging.debug('Exception raised: %s', err)
            os.remove(cache_file)
            metrics_utils.handle_exc_and_send_exit_event(
                constants.ACCESS_CACHE_FAILURE)
    return None

def clean_test_info_caches(tests, cache_root=None):
    """Clean caches of input tests.

    Args:
        tests: A list of test references.
        cache_root: Folder path for finding caches.
    """
    if not cache_root:
        cache_root = get_cache_root()
    for test in tests:
        cache_file = get_test_info_cache_path(test, cache_root)
        if os.path.isfile(cache_file):
            logging.debug('Removing cache: %s', cache_file)
            try:
                os.remove(cache_file)
            except IOError as err:
                logging.debug('Exception raised: %s', err)
                metrics_utils.handle_exc_and_send_exit_event(
                    constants.ACCESS_CACHE_FAILURE)

def get_modified_files(root_dir):
    """Get the git modified files. The git path here is git top level of
    the root_dir. It's inevitable to utilise different commands to fulfill
    2 scenario:
        1. locate unstaged/staged files
        2. locate committed files but not yet merged.
    the 'git_status_cmd' fulfils the former while the 'find_modified_files'
    fulfils the latter.

    Args:
        root_dir: the root where it starts finding.

    Returns:
        A set of modified files altered since last commit.
    """
    modified_files = set()
    try:
        find_git_cmd = 'cd {}; git rev-parse --show-toplevel'.format(root_dir)
        git_paths = subprocess.check_output(
            find_git_cmd, shell=True).decode().splitlines()
        for git_path in git_paths:
            # Find modified files from git working tree status.
            git_status_cmd = ("repo forall {} -c git status --short | "
                              "awk '{{print $NF}}'").format(git_path)
            modified_wo_commit = subprocess.check_output(
                git_status_cmd, shell=True).decode().rstrip().splitlines()
            for change in modified_wo_commit:
                modified_files.add(
                    os.path.normpath('{}/{}'.format(git_path, change)))
            # Find modified files that are committed but not yet merged.
            find_modified_files = _FIND_MODIFIED_FILES_CMDS.format(git_path)
            commit_modified_files = subprocess.check_output(
                find_modified_files, shell=True).decode().splitlines()
            for line in commit_modified_files:
                modified_files.add(os.path.normpath('{}/{}'.format(
                    git_path, line)))
    except (OSError, subprocess.CalledProcessError) as err:
        logging.debug('Exception raised: %s', err)
    return modified_files

def delimiter(char, length=_DEFAULT_TERMINAL_WIDTH, prenl=0, postnl=0):
    """A handy delimiter printer.

    Args:
        char: A string used for delimiter.
        length: An integer for the replication.
        prenl: An integer that insert '\n' before delimiter.
        postnl: An integer that insert '\n' after delimiter.

    Returns:
        A string of delimiter.
    """
    return prenl * '\n' + char * length + postnl * '\n'

def find_files(path, file_name=constants.TEST_MAPPING):
    """Find all files with given name under the given path.

    Args:
        path: A string of path in source.
        file_name: The file name pattern for finding matched files.

    Returns:
        A list of paths of the files with the matching name under the given
        path.
    """
    match_files = []
    for root, _, filenames in os.walk(path):
        for filename in fnmatch.filter(filenames, file_name):
            match_files.append(os.path.join(root, filename))
    return match_files

def extract_zip_text(zip_path):
    """Extract the text files content for input zip file.

    Args:
        zip_path: The file path of zip.

    Returns:
        The string in input zip file.
    """
    content = ''
    try:
        with zipfile.ZipFile(zip_path) as zip_file:
            for filename in zip_file.namelist():
                if os.path.isdir(filename):
                    continue
                # Force change line if multiple text files in zip
                content = content + '\n'
                # read the file
                with zip_file.open(filename) as extract_file:
                    for line in extract_file:
                        if matched_tf_error_log(line.decode()):
                            content = content + line.decode()
    except zipfile.BadZipfile as err:
        logging.debug('Exception raised: %s', err)
    return content

def matched_tf_error_log(content):
    """Check if the input content matched tradefed log pattern.
    The format will look like this.
    05-25 17:37:04 W/XXXXXX
    05-25 17:37:04 E/XXXXXX

    Args:
        content: Log string.

    Returns:
        True if the content matches the regular expression for tradefed error or
        warning log.
    """
    reg = ('^((0[1-9])|(1[0-2]))-((0[1-9])|([12][0-9])|(3[0-1])) '
           '(([0-1][0-9])|([2][0-3])):([0-5][0-9]):([0-5][0-9]) (E|W/)')
    if re.search(reg, content):
        return True
    return False

def has_valid_cert():
    """Check whether the certificate is valid.

    Returns: True if the cert is valid.
    """
    if not constants.CERT_STATUS_CMD:
        return False
    try:
        return (not subprocess.check_call(constants.CERT_STATUS_CMD,
                                          stdout=subprocess.DEVNULL,
                                          stderr=subprocess.DEVNULL))
    except subprocess.CalledProcessError:
        return False

# pylint: disable=too-many-locals
def get_flakes(branch='',
               target='',
               test_name='',
               test_module='',
               test_method=''):
    """Get flake information.

    Args:
        branch: A string of branch name.
        target: A string of target.
        test_name: A string of test suite name.
        test_module: A string of test module.
        test_method: A string of test method.

    Returns:
        A dictionary of flake info. None if no flakes service exists.
    """
    if not branch:
        branch = constants.FLAKE_BRANCH
    if not target:
        target = constants.FLAKE_TARGET
    if not test_name:
        test_name = constants.FLAKE_TEST_NAME
    # Currently lock the flake information from test-mapping test
    # which only runs on cuttlefish(x86) devices.
    # TODO: extend supporting other devices
    if test_module:
        test_module = 'x86 {}'.format(test_module)
    flake_service = os.path.join(constants.FLAKE_SERVICE_PATH,
                                 constants.FLAKE_FILE)
    if not os.path.exists(flake_service):
        logging.debug('Get flakes: Flake service path not exist.')
        # Send (3, 0) to present no flakes info because service does not exist.
        metrics.LocalDetectEvent(
            detect_type=constants.DETECT_TYPE_NO_FLAKE,
            result=0)
        return None
    if not has_valid_cert():
        logging.debug('Get flakes: No valid cert.')
        # Send (3, 1) to present no flakes info because no valid cert.
        metrics.LocalDetectEvent(
            detect_type=constants.DETECT_TYPE_NO_FLAKE,
            result=1)
        return None
    flake_info = {}
    start = time.time()
    try:
        shutil.copy2(flake_service, constants.FLAKE_TMP_PATH)
        tmp_service = os.path.join(constants.FLAKE_TMP_PATH,
                                   constants.FLAKE_FILE)
        os.chmod(tmp_service, 0o0755)
        cmd = [tmp_service, branch, target, test_name, test_module, test_method]
        logging.debug('Executing: %s', ' '.join(cmd))
        output = subprocess.check_output(cmd).decode()
        percent_template = "{}:".format(constants.FLAKE_PERCENT)
        postsubmit_template = "{}:".format(constants.FLAKE_POSTSUBMIT)
        for line in output.splitlines():
            if line.startswith(percent_template):
                flake_info[constants.FLAKE_PERCENT] = line.replace(
                    percent_template, '')
            if line.startswith(postsubmit_template):
                flake_info[constants.FLAKE_POSTSUBMIT] = line.replace(
                    postsubmit_template, '')
    # pylint: disable=broad-except
    except Exception as e:
        logging.debug('Exception:%s', e)
        return None
    # Send (4, time) to present having flakes info and it spent time.
    duration = round(time.time()-start)
    logging.debug('Took %ss to get flakes info', duration)
    metrics.LocalDetectEvent(
        detect_type=constants.DETECT_TYPE_HAS_FLAKE,
        result=duration)
    return flake_info

def read_test_record(path):
    """A Helper to read test record proto.

    Args:
        path: The proto file path.

    Returns:
        The test_record proto instance.
    """
    with open(path, 'rb') as proto_file:
        msg = test_record_pb2.TestRecord()
        msg.ParseFromString(proto_file.read())
    return msg

def has_python_module(module_name):
    """Detect if the module can be loaded without importing it in real.

    Args:
        cmd: A string of the tested module name.

    Returns:
        True if found, False otherwise.
    """
    return bool(importlib.util.find_spec(module_name))

def is_valid_json_file(path):
    """Detect if input path exist and content is valid.

    Args:
        path: The json file path.

    Returns:
        True if file exist and content is valid, False otherwise.
    """
    if isinstance(path, bytes):
        path = path.decode('utf-8')
    try:
        if os.path.isfile(path):
            with open(path) as json_file:
                json.load(json_file)
            return True
        logging.warning('%s: File not found.', path)
    except json.JSONDecodeError:
        logging.warning('Exception happened while loading %s.', path)
    return False

def get_manifest_branch():
    """Get the manifest branch via repo info command.

    Returns:
        None if no system environment parameter ANDROID_BUILD_TOP or
        running 'repo info' command error, otherwise the manifest branch
    """
    build_top = os.getenv(constants.ANDROID_BUILD_TOP, None)
    if not build_top:
        return None
    try:
        # Command repo need use default lib "http", add non-default lib
        # might cause repo command execution error.
        splitter = ':'
        env_vars = os.environ.copy()
        org_python_path = env_vars['PYTHONPATH'].split(splitter)
        default_python_path = [p for p in org_python_path
                               if not p.startswith('/tmp/Soong.python_')]
        env_vars['PYTHONPATH'] = splitter.join(default_python_path)
        output = subprocess.check_output(
            ['repo', 'info', '-o', constants.ASUITE_REPO_PROJECT_NAME],
            env=env_vars,
            cwd=build_top,
            universal_newlines=True)
        branch_re = re.compile(r'Manifest branch:\s*(?P<branch>.*)')
        match = branch_re.match(output)
        if match:
            return match.group('branch')
        logging.warning('Unable to detect branch name through:\n %s', output)
    except subprocess.CalledProcessError:
        logging.warning('Exception happened while getting branch')
    return None

def get_build_target():
    """Get the build target form system environment TARGET_PRODUCT."""
    return os.getenv(constants.ANDROID_TARGET_PRODUCT, None)

def parse_mainline_modules(test):
    """Parse test reference into test and mainline modules.

    Args:
        test: An String of test reference.

    Returns:
        A string of test without mainline modules,
        A string of mainline modules.
    """
    result = constants.TEST_WITH_MAINLINE_MODULES_RE.match(test)
    if not result:
        return test, ""
    test_wo_mainline_modules = result.group('test')
    mainline_modules = result.group('mainline_modules')
    return test_wo_mainline_modules, mainline_modules

def has_wildcard(test_name):
    """ Tell whether the test_name(either a list or string) contains wildcard
    symbols.

    Args:
        test_name: A list or a str.

    Return:
        True if test_name contains wildcard, False otherwise.
    """
    if isinstance(test_name, str):
        return any(char in test_name for char in _WILDCARD_CHARS)
    if isinstance(test_name, list):
        for name in test_name:
            if has_wildcard(name):
                return True
    return False

def is_build_file(path):
    """ If input file is one of an android build file.

    Args:
        path: A string of file path.

    Return:
        True if path is android build file, False otherwise.
    """
    return bool(os.path.splitext(path)[-1] in _ANDROID_BUILD_EXT)

def quote(input_str):
    """ If the input string -- especially in custom args -- contains shell-aware
    characters, insert a pair of "\" to the input string.

    e.g. unit(test|testing|testing) -> 'unit(test|testing|testing)'

    Args:
        input_str: A string from user input.

    Returns: A string with single quotes if regex chars were detected.
    """
    if has_chars(input_str, _REGEX_CHARS):
        return "\'" + input_str + "\'"
    return input_str

def has_chars(input_str, chars):
    """ Check if the input string contains one of the designated characters.

    Args:
        input_str: A string from user input.
        chars: An iterable object.

    Returns:
        True if the input string contains one of the special chars.
    """
    for char in chars:
        if char in input_str:
            return True
    return False

def prompt_with_yn_result(msg, default=True):
    """Prompt message and get yes or no result.

    Args:
        msg: The question you want asking.
        default: boolean to True/Yes or False/No
    Returns:
        default value if get KeyboardInterrupt or ValueError exception.
    """
    suffix = '[Y/n]: ' if default else '[y/N]: '
    try:
        return strtobool(input(msg+suffix))
    except (ValueError, KeyboardInterrupt):
        return default

def get_android_junit_config_filters(test_config):
    """Get the dictionary of a input config for junit config's filters

    Args:
        test_config: The path of the test config.
    Returns:
        A dictionary include all the filters in the input config.
    """
    filter_dict = {}
    xml_root = ET.parse(test_config).getroot()
    option_tags = xml_root.findall('.//option')
    for tag in option_tags:
        name = tag.attrib['name'].strip()
        if name in constants.SUPPORTED_FILTERS:
            filter_values = filter_dict.get(name, [])
            value = tag.attrib['value'].strip()
            filter_values.append(value)
            filter_dict.update({name: filter_values})
    return filter_dict

def get_config_parameter(test_config):
    """Get all the parameter values for the input config

    Args:
        test_config: The path of the test config.
    Returns:
        A set include all the parameters of the input config.
    """
    parameters = set()
    xml_root = ET.parse(test_config).getroot()
    option_tags = xml_root.findall('.//option')
    for tag in option_tags:
        name = tag.attrib['name'].strip()
        if name == constants.CONFIG_DESCRIPTOR:
            key = tag.attrib['key'].strip()
            if key == constants.PARAMETER_KEY:
                value = tag.attrib['value'].strip()
                parameters.add(value)
    return parameters
