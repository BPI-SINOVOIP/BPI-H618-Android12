#!/usr/bin/env python3
#
# Copyright 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Main entrypoint for all of atest's unittest."""

import logging
import os
import sys
import unittest

from importlib import import_module

import atest_utils

COVERAGE = 'coverage'
RUN_COVERAGE = COVERAGE in sys.argv
SHOW_MISSING = '--show-missing' in sys.argv
# Setup logging to be silent so unittests can pass through TF.
logging.disable(logging.ERROR)

def get_test_modules():
    """Returns a list of testable modules.

    Finds all the test files (*_unittest.py) and get their no-absolute
    path (internal/lib/utils_test.py) and translate it to an import path and
    strip the py ext (internal.lib.utils_test).

    Returns:
        List of strings (the testable module import path).
    """
    testable_modules = []
    base_path = os.path.dirname(os.path.realpath(__file__))

    for dirpath, _, files in os.walk(base_path):
        for f in files:
            if f.endswith("_unittest.py"):
                # Now transform it into a no-absolute import path.
                full_file_path = os.path.join(dirpath, f)
                rel_file_path = os.path.relpath(full_file_path, base_path)
                rel_file_path, _ = os.path.splitext(rel_file_path)
                rel_file_path = rel_file_path.replace(os.sep, ".")
                testable_modules.append(rel_file_path)

    return testable_modules

def run_test_modules(test_modules):
    """Main method of running unit tests.

    Args:
        test_modules; a list of module names.

    Returns:
        result: a namespace of unittest result.
    """
    for mod in test_modules:
        import_module(mod)

    loader = unittest.defaultTestLoader
    test_suite = loader.loadTestsFromNames(test_modules)
    runner = unittest.TextTestRunner(verbosity=2)
    return runner.run(test_suite)

# pylint: disable=import-outside-toplevel
def main(run_coverage=False, show_missing=False):
    """Main unittest entry.

    Args:
        cov_args: A list of coverage arguments.

    Returns:
        0 if success. None-zero if fails.
    """
    if not all((run_coverage, atest_utils.has_python_module(COVERAGE))):
        result = run_test_modules(get_test_modules())
        if not result.wasSuccessful():
            sys.exit(not result.wasSuccessful())
        sys.exit(0)

    from coverage import coverage
    # The cover_pylib=False ignores only std libs; therefore, these 3rd-party
    # libs must be omitted before creating coverage class.
    ignore_libs = ['*/__init__.py',
                   '*dist-packages/*.py',
                   '*site-packages/*.py']
    cov = coverage(omit=ignore_libs)
    cov.erase()
    cov.start()
    result = run_test_modules(get_test_modules())
    if not result.wasSuccessful():
        cov.erase()
        sys.exit(not result.wasSuccessful())
    cov.stop()
    cov.save()
    cov.report(show_missing=show_missing)
    cov.html_report()


if __name__ == '__main__':
    if len(sys.argv) > 1:
        main(RUN_COVERAGE, SHOW_MISSING)
    else:
        main()
