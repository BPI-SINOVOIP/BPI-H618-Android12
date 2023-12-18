#!/usr/bin/env python3

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

"""
Extract boot.img from a GKI APEX.

Usage: extract_img_from_apex [--tool host_tool [--tool ...]] \
           input_gki.apex out_dir
"""

import logging
import os
import sys

import common

APEX_PAYLOAD_FILE = "apex_payload.img"
APEX_PAYLOAD_BIN_PATH = "/etc/ota/payload.bin"

logger = logging.getLogger(__name__)
OPTIONS = common.OPTIONS


def ExtractImagesFromApex(apex, out_dir):
  if not os.path.isdir(out_dir):
    os.makedirs(out_dir)
  apex_dir = common.UnzipTemp(apex, [APEX_PAYLOAD_FILE])
  apex_payload = os.path.join(apex_dir, APEX_PAYLOAD_FILE)

  payload_bin = common.MakeTempFile("payload_", ".bin")
  debugfs_command = ['debugfs', '-R', "dump {} {}".format(
      APEX_PAYLOAD_BIN_PATH, payload_bin), apex_payload]

  common.RunAndCheckOutput(debugfs_command)
  assert os.path.getsize(payload_bin) > 0, payload_bin + " is an empty file!"

  boot_img = os.path.join(out_dir, 'boot.img')
  with open(boot_img, 'w') as _:
    pass

  delta_generator_command = [
      'delta_generator',
      '--is_partial_update=true',
      '--in_file=' + payload_bin,
      '--partition_names=boot',
      '--new_partitions=' + boot_img
  ]
  common.RunAndCheckOutput(delta_generator_command)


def main(argv):
  def option_handler(o, a):
    if o == "--tool":
      name = os.path.basename(a)
      common.SetHostToolLocation(name, a)
    else:
      return False
    return True

  args = common.ParseOptions(
      argv, __doc__,
      extra_long_opts=["tool="],
      extra_option_handler=option_handler)

  if len(args) != 2:
    common.Usage(__doc__)
    sys.exit(1)

  common.InitLogging()

  ExtractImagesFromApex(args[0], args[1])
  logger.info("done.")


if __name__ == '__main__':
  try:
    common.CloseInheritedPipes()
    main(sys.argv[1:])
  except common.ExternalError:
    logger.exception("\n   ERROR:\n")
    sys.exit(1)
  finally:
    common.Cleanup()
