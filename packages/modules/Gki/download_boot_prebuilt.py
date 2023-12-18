#!/usr/bin/env python3
#
# Copyright (C) 2019-2020 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import argparse
import collections
import functools
import glob
import json
import logging
import os
import pathlib
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
import urllib.request

from concurrent import futures
from pathlib import Path

BASE_URL = "https://ci.android.com/builds/submitted/{build_id}/{target}/latest/raw"
SUPPORTED_ARCHS = ["arm64"]
VARIANTS = ["userdebug"]
BOOT_PREBUILT_REL_DIR = "packages/modules/BootPrebuilt"
ANDROID_BUILD_TOP = os.environ["ANDROID_BUILD_TOP"]

logger = logging.getLogger(__name__)
logging.basicConfig(level=logging.INFO)

def parse_args():
  parser = argparse.ArgumentParser()
  parser.add_argument(
      "build_id",
      type=int,
      help="the build id to download the build for, e.g. 6148204")
  parser.add_argument(
      "--bug",
      type=str,
      default=None,
      help="optional bug number for git commit.")

  return parser.parse_args()


def download_file(url, dest_filename):
  logger.info("Downloading %s -> %s", url, dest_filename)
  urllib.request.urlretrieve(url, dest_filename)


def get_artifact_download_spec(build_id, device, variant, dest_dir):
  target = "{}-{}".format(device, variant)
  url_base = BASE_URL.format(build_id=build_id, target=target)
  filename = "{}-img-{}.zip".format(device, build_id)
  url = os.path.join(url_base, filename)
  dest_filename = os.path.join(dest_dir, filename)
  return url, dest_filename


def update_prebuilt(build_id, boot_prebuilt, ver, arch, variant, bug):
  device = "aosp_" + arch
  arch_dir = os.path.join(boot_prebuilt, ver, arch)
  variant_dir = os.path.join(arch_dir, variant)
  boot_img_name = "boot-{}.img".format(ver)
  stored_img_name = "boot-{}.img".format(variant)
  try:
    subprocess.check_call(["repo", "start", "boot-prebuilt-{}".format(build_id)], cwd=arch_dir)

    os.makedirs(variant_dir)
    url, dest_filename = get_artifact_download_spec(build_id, device, variant, variant_dir)
    download_file(url, dest_filename)
    args = ["unzip", "-d", variant_dir, dest_filename, boot_img_name]
    logger.info("Calling: %s", " ".join(args))
    subprocess.check_call(args)
    shutil.move(os.path.join(variant_dir, boot_img_name), os.path.join(arch_dir, stored_img_name))

  finally:
    shutil.rmtree(variant_dir)

    message = """Update prebuilts to {build_id}.

Test: Treehugger
Bug: {bug}
""".format(build_id=build_id, bug=bug or "N/A")

    logger.info("Creating commit for %s", arch_dir)
    subprocess.check_call(["git", "add", "."], cwd=arch_dir)
    subprocess.check_call(["git", "commit", "-m", message], cwd=arch_dir)


def main():
  args = parse_args()
  with futures.ThreadPoolExecutor(max_workers=10) as pool:
    fs = []
    boot_prebuilt = os.path.join(ANDROID_BUILD_TOP, BOOT_PREBUILT_REL_DIR)
    for ver in os.listdir(boot_prebuilt):
      if not re.match(r'\d+[.]\d+', ver):
        continue
      for arch in os.listdir(os.path.join(boot_prebuilt, ver)):
        if arch not in SUPPORTED_ARCHS:
          continue
        for variant in VARIANTS:
          fs.append((ver, arch, pool.submit(update_prebuilt, args.build_id, boot_prebuilt, ver,
                                            arch, variant, args.bug)))

    futures.wait([f for ver, arch, f in fs])
    success_dirs = []
    logger.info("===============")
    logger.info("Summary:")
    for ver, arch, future in fs:
      if future.exception():
        logger.error("%s/%s: %s", ver, arch, future.exception())
      else:
        logger.info("%s/%s: Updated.", ver, arch)
        success_dirs.append(os.path.join(BOOT_PREBUILT_REL_DIR, ver, arch))

    if success_dirs:
      args = ["repo", "upload", "--verify", "--hashtag-branch", "--br",
              "boot-prebuilt-{}".format(args.build_id)] + success_dirs
      logger.info(" ".join(args))
      subprocess.check_call(args, cwd=ANDROID_BUILD_TOP)


if __name__ == "__main__":
  sys.exit(main())
