#!/usr/bin/env python3
# Copyright 2019, The Android Open Source Project
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

"""asuite_run_unittests

This is a unit test wrapper to run tests of aidegen, atest or both.
"""


from __future__ import print_function

import argparse
import os
import shlex
import subprocess
import sys

ASUITE_HOME = os.path.dirname(os.path.realpath(__file__))
ASUITE_PLUGIN_PATH = os.path.join(ASUITE_HOME, "asuite_plugin")
ATEST_CMD = os.path.join(ASUITE_HOME, "atest", "atest_run_unittests.py")
ATEST2_CMD = os.path.join(ASUITE_HOME, "atest-py2", "atest_run_unittests.py")
AIDEGEN_CMD = "atest aidegen_unittests --host"
PLUGIN_LIB_CMD = "atest plugin_lib_unittests --host"
GRADLE_TEST = "/gradlew test"
# Definition of exit codes.
EXIT_ALL_CLEAN = 0
EXIT_TEST_FAIL = 1

def run_unittests(files):
    """Parse modified files and tell if they belong to aidegen, atest or both.

    Args:
        files: a list of files.

    Returns:
        True if subprocess.check_call() returns 0.
    """
    cmd_dict = {}
    for f in files:
        if 'atest' in f:
            cmd_dict.update({ATEST_CMD: None})
        if 'atest-py2' in f:
            cmd_dict.update({ATEST2_CMD: None})
        if 'aidegen' in f:
            cmd_dict.update({AIDEGEN_CMD: None})
        if 'plugin_lib' in f:
            cmd_dict.update({PLUGIN_LIB_CMD: None})
        if 'asuite_plugin' in f:
            cmd = ASUITE_PLUGIN_PATH + GRADLE_TEST
            cmd_dict.update({cmd : ASUITE_PLUGIN_PATH})
    try:
        for cmd, path in cmd_dict.items():
            subprocess.check_call(shlex.split(cmd), cwd=path)
    except subprocess.CalledProcessError as error:
        print('Unit test failed at:\n\n{}'.format(error.output))
        raise
    return True


def get_files_to_upload():
    """Parse args or modified files and return them as a list.

    Returns:
        A list of files to upload.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('preupload_files', nargs='*', help='Files to upload.')
    args = parser.parse_args()
    files_to_upload = args.preupload_files
    if not files_to_upload:
        # When running by users directly, only consider:
        # added(A), renamed(R) and modified(M) files
        # and store them in files_to_upload.
        cmd = "git status --short | egrep ^[ARM] | awk '{print $NF}'"
        preupload_files = subprocess.check_output(cmd, shell=True,
                                                  encoding='utf-8').splitlines()
        if preupload_files:
            print('Files to upload: %s' % preupload_files)
            files_to_upload = preupload_files
        else:
            sys.exit(EXIT_ALL_CLEAN)
    return files_to_upload

if __name__ == '__main__':
    if not run_unittests(get_files_to_upload()):
        sys.exit(EXIT_TEST_FAIL)
