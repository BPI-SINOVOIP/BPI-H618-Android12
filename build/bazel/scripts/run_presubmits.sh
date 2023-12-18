#!/bin/bash -eu

set -o pipefail

if [[ ! -d "build/bazel/ci" ]]; then
  echo "Please run this script from TOP".
  exit -1
fi

echo "Running presubmit scripts..."
echo

echo bp2build.sh
build/bazel/ci/bp2build.sh
echo

echo mixed_libc.sh
build/bazel/ci/mixed_libc.sh
echo

echo run_integration_tests.sh
build/soong/tests/run_integration_tests.sh
echo

echo "All Tests Passed! You Are Awesome!"
