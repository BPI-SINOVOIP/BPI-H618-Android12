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
Generate payload.bin from a single image.
"""

import argparse
import logging
import os
import re
import shutil
import sys
from zipfile import ZipFile

import common
from ota_from_target_files import (Payload, PayloadSigner)

logger = logging.getLogger(__name__)
OPTIONS = common.OPTIONS


def _ParseArgs():
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument("--tools", metavar="PATH", type=str, nargs="*",
                      help="A list of real paths of tools that this script depends on.")
  parser.add_argument("--key", type=str,
                      help="Key to use to sign the package. If unspecified, script does not sign "
                           "the package and payload_properties.txt is not generated.")
  parser.add_argument("--out", type=str, required=True,
                      help="Required output directory to payload.bin and payload_properties.txt")
  parser.add_argument("input", metavar="NAME:IMAGE", nargs="+",
                      help="Name of the image and path to the image, e.g. boot:path/to/boot.img")
  args = parser.parse_args()

  return args


def _PrepareEnvironment(args):
  common.InitLogging()
  if not args.tools:
    return
  for path in args.tools:
    name = os.path.basename(path)
    # Use absolute path because GetBootImageTimestamp changes cwd when running some tools.
    common.SetHostToolLocation(name, os.path.abspath(path))
    # brillo_update_payload is a shell script that depends on this environment variable.
    if name == "delta_generator":
      os.environ["GENERATOR"] = path


def CreateOtaFromRawImages(args):
  _PrepareEnvironment(args)

  tf = common.MakeTempFile("target_files", ".zip")
  payload_additional_args = ["--is_partial_update", "true"]
  with ZipFile(tf, "w") as zip:
    names = []
    for pair_str in args.input:
      pair = pair_str.split(":")
      assert len(pair) == 2, "Incorrect format: " + pair_str
      name, img_path = tuple(pair)
      zip.write(img_path, arcname=os.path.join("IMAGES", name + ".img"))
      names.append(name)
      if name == "boot":
        timestamp = common.GetBootImageTimestamp(img_path)
        assert timestamp is not None, "Cannot extract timestamp from boot image"
        payload_additional_args += ["--partition_timestamps",
                                    "boot:" + str(timestamp)]

    zip.writestr("META/ab_partitions.txt", "\n".join(names) + "\n")
    zip.writestr("META/dynamic_partitions_info.txt", """
virtual_ab=true
super_partition_groups=
    """)

  payload = Payload()
  payload.Generate(tf, None, payload_additional_args)

  if args.key:
    OPTIONS.package_key = args.key
    OPTIONS.key_passwords = common.GetKeyPasswords([OPTIONS.package_key])
    payload_signer = PayloadSigner()
    payload.Sign(payload_signer)

  shutil.copy(payload.payload_file, os.path.join(args.out, "payload.bin"))
  if payload.payload_properties:
    shutil.copy(payload.payload_properties, os.path.join(args.out, "payload_properties.txt"))


if __name__ == "__main__":
  try:
    common.CloseInheritedPipes()
    args = _ParseArgs()
    CreateOtaFromRawImages(args)
  except common.ExternalError:
    logger.exception("\n   ERROR:\n")
    sys.exit(1)
  finally:
    common.Cleanup()
