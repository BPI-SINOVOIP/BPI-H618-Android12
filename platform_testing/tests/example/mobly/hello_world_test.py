#!/usr/bin/env python3
#
# Copyright (C) 2020 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import argparse
import errno
import logging
import os
import sys
import tempfile
import time

from mobly import base_test
from mobly import test_runner
from mobly.controllers import android_device


SINGLE_DEVICE_TEST_CONFIG_FORMAT = """
TestBeds:
- Name: SampleTestBed
  Controllers:
    AndroidDevice:
    - serial: {serial}
MoblyParams: {{LogPath: {log_path}}}
"""

TEST_CONFIG_FILE_NAME = 'mobly_test_config.yaml'
DEFAULT_TESTBED_NAME = 'localDevTestBed'
MOBLY_LOG_DIR_NAME = 'mobly'

def _create_config_file(args):
  """Create the test configuration file on local host.

  Args:
    args: Populated argument object, including attributes: device_serial,
      log_path.

  Returns:
    String of created configuration file absolute path.
  """
  mobly_dir = os.path.join(_get_default_log_root_path(), DEFAULT_TESTBED_NAME,
                           str(time.time()))
  mobly_config_file_path = os.path.join(mobly_dir, TEST_CONFIG_FILE_NAME)
  log_path = args.log_path if args.log_path else mobly_dir
  config_text = SINGLE_DEVICE_TEST_CONFIG_FORMAT.format(
      serial=args.device_serial,
      log_path=log_path)
  if not os.path.exists(os.path.dirname(mobly_config_file_path)):
    try:
      os.makedirs(os.path.dirname(mobly_config_file_path))
    except OSError as os_error:
      if os_error.errno != errno.EEXIST:
        raise
  with open(mobly_config_file_path, 'w') as config_file:
    config_file.write(config_text)
  return mobly_config_file_path


def _get_default_log_root_path():
  return os.path.join(
      os.environ.get('TEST_UNDECLARED_OUTPUTS_DIR',
                     os.environ.get('GOOGLE_LOG_DIR', tempfile.gettempdir())),
      MOBLY_LOG_DIR_NAME)


def _parse_arguments(argv=None):
  """Parse command line arguments.

  Args:
    argv: List of arguments, if none, get from sys.argv.

  Returns:
    A three item tuple containing the populated argument objects and
    the list of remaining argument strings.

  Raises:
    Exception: If no '--' exists in arguments passed in.
  """
  logging.info('arguments: %s', argv)

  if not argv:
    argv = sys.argv[1:]
  if '--' not in argv:
    raise ValueError("""
    Usage exception:
      Arguments should be parsed in after '--'.
      For example:
        <python_binary> -- --config=<config_file> --test_bed=<testbed_name>
    """)
  index = argv.index('--')
  # Parse arguments for this script only. It is better to not use the
  # abbreviated (single character) version of the arguments to avoid conflict
  # with mobly.
  # Exception is the device_serial, since it is a convention.
  local_parser = argparse.ArgumentParser(description='Mobly Hello World Test.')
  local_parser.add_argument(
      '-s',
      '--device_serial',
      type=str,
      help='Device serial number to run tests.')
  local_parser.add_argument('--log_path', type=str, help='Test log file path.')
  local_args, other_args = local_parser.parse_known_args(argv[index + 1:])
  logging.info('Parsed local arguments: %s; Not parsed arguments: %s',
               local_args, other_args)
  filtered_argv = argv[:index + 1] + other_args  # Remove all local arguments.

  # The following arguments are the same as mobly test_runner.
  parser = argparse.ArgumentParser(description='Mobly Hello World Test.')
  parser.add_argument(
      '-c',
      '--config',
      nargs=1,
      type=str,
      metavar='<PATH>',
      help='Path to the test configuration file.')
  args, unrec_argv = parser.parse_known_args(other_args)
  logging.info('Parsed arguments: %s; Not parsed arguments: %s', args,
               unrec_argv)
  return local_args, args, filtered_argv


class HelloWorldTest(base_test.BaseTestClass):

  def setup_class(self):
    # Registering android_device controller module declares the test's
    # dependency on Android device hardware. By default, we expect at least one
    # object is created from this.
    self.ads = self.register_controller(android_device)
    self.dut = self.ads[0]

  def test_hello(self):
    pass


if __name__ == '__main__':
  local_args, args, filtered_argv = _parse_arguments(sys.argv[1:])
  sys.argv = sys.argv[:1]
  if not args.config:
    logging.info('Mobly test config file was not provided. Creating one.')
    config_file = _create_config_file(local_args)
    sys.argv.append('--config=%s' % config_file)
    logging.info('Mobly test config file: %s', config_file)
  logging.info('Sys args: %s', sys.argv)
  test_runner.main()
