#!/usr/bin/env bash

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

set -e

if [ $# != 1 ]; then
  echo "Usage: golden_test.sh [check|update]"
fi

function _golden_test() {
  local update=0
  case $1 in
    check)
      update=0
      ;;
    update)
      update=1
      ;;
    *)
      echo "Argument must be 'check' or 'update'"
      exit 1
      ;;
  esac

  # warning: this list must be kept in sync with system/tools/aidl/Android.bp
  modules=(
    "aidl-test-interface-cpp-source"
    "aidl-test-interface-java-source"
    "aidl-test-interface-ndk_platform-source"
    "aidl-test-interface-rust-source"
    "aidl_test_loggable_interface-cpp-source"
    "aidl_test_loggable_interface-java-source"
    "aidl_test_loggable_interface-ndk_platform-source"
    "aidl_test_loggable_interface-ndk-source"
  )

  local root="."
  if [ "$ANDROID_BUILD_TOP" != "" ]; then
    root="$ANDROID_BUILD_TOP"
  fi

  if [ "$update" = 1 ]; then
    "$root"/build/soong/soong_ui.bash --make-mode \
      $(for i in "${modules[@]}"; do
          echo "out/soong/.intermediates/system/tools/aidl/$i/timestamp"
        done)
  fi

  local e=0
  for module in "${modules[@]}"; do
    local built="$root/out/soong/.intermediates/system/tools/aidl/$module/"
    local golden="$root/system/tools/aidl/tests/golden_output/$module/"

    if [ "$update" = 1 ]; then
      rm -rf "$golden"
      mkdir -p "$golden"
      cp -r "$built/gen" "$golden"
    else
      diff -r "$built" "$golden" || e=1
    fi
  done

  if [ "$e" = 1 ]; then
    echo "ERROR: The AIDL compiler is outputting files in an unknown way."
    echo "ERROR: to accept these changes, please run:"
    echo "ERROR:     \$ANDROID_BUILD_TOP/system/tools/aidl/tests/golden_test.sh update"
    exit 1
  else
    if [ "$update" = 1 ]; then
      echo "UPDATE GOLDEN TEST SUCCESS"
    fi
  fi
}

_golden_test "$@"
