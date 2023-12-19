#!/bin/bash
#
# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script rebuilds and installs compiler wrappers

if [[ ! -e /etc/cros_chroot_version ]]; then
  echo "Please run this script inside chroot"
  exit 1
fi
set -e
cd "$(dirname "$(readlink -m "$0")")"
echo "Updated files:"
# Update the host wrapper
./build.py --config=cros.host --use_ccache=false --use_llvm_next=false --output_file=./clang_host_wrapper
sudo mv ./clang_host_wrapper /usr/bin/clang_host_wrapper
echo "/usr/bin/clang_host_wrapper"
sudo cp ../binary_search_tool/bisect_driver.py /usr/bin
echo "/usr/bin/clang_host_wrapper/bisect_driver.py"
# Update the target wrappers
for GCC in cross-x86_64-cros-linux-gnu/gcc cross-armv7a-cros-linux-gnueabihf/gcc cross-aarch64-cros-linux-gnu/gcc; do
  if ! FILES="$(equery f ${GCC})"; then
    if [[ $(equery l "${GCC}" 2>&1 | wc -c) -eq 0 ]]; then
      echo "no ${GCC} package found; skipping" >&2
      continue
    fi
    # Something went wrong, and the equery above probably complained about it.
    exit 1
  fi
  ./build.py --config=cros.hardened --use_ccache=false --use_llvm_next=false --output_file=./sysroot_wrapper.hardened.noccache
  sudo mv ./sysroot_wrapper.hardened.noccache "$(grep sysroot_wrapper.hardened.noccache <<< "${FILES}")"
  grep sysroot_wrapper.hardened.noccache <<< "${FILES}"
  ./build.py --config=cros.hardened --use_ccache=true --use_llvm_next=false --output_file=./sysroot_wrapper.hardened.ccache
  sudo mv ./sysroot_wrapper.hardened.ccache "$(grep sysroot_wrapper.hardened.ccache <<< "${FILES}")"
  grep sysroot_wrapper.hardened.ccache <<< "${FILES}"
  sudo cp ../binary_search_tool/bisect_driver.py "$(grep bisect_driver.py <<< "${FILES}")"
  grep bisect_driver.py <<< "${FILES}"
done
