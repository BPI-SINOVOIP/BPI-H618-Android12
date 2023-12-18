#!/bin/bash -eux
# Verifies mixed builds succeeds when building "libc".
# This verification script is designed to be used for continuous integration
# tests, though may also be used for manual developer verification.

if [[ -z ${DIST_DIR+x} ]]; then
  echo "DIST_DIR not set. Using out/dist. This should only be used for manual developer testing."
  DIST_DIR="out/dist"
fi

# Run a mixed build of "libc"
build/soong/soong_ui.bash --make-mode USE_BAZEL_ANALYSIS=1 BAZEL_STARTUP_ARGS="--max_idle_secs=5" BAZEL_BUILD_ARGS="--color=no --curses=no --show_progress_rate_limit=5" TARGET_PRODUCT=aosp_arm64 TARGET_BUILD_VARIANT=userdebug libc dist DIST_DIR=$DIST_DIR

# Verify there are artifacts under the out directory that originated from bazel.
echo "Verifying OUT_DIR contains bazel-out..."
if find out/ | grep bazel-out &>/dev/null; then
  echo "bazel-out found."
else
  echo "bazel-out not found. This may indicate that mixed builds are silently not running."
  exit 1
fi

# Run a mixed build of "libbacktrace"
# This is a small module which uses propagated includes from libc; thus will
# fail if includes are not propagated appropriately from bazel-built libc.
build/soong/soong_ui.bash --make-mode USE_BAZEL_ANALYSIS=1 BAZEL_STARTUP_ARGS="--max_idle_secs=5" BAZEL_BUILD_ARGS="--color=no --curses=no --show_progress_rate_limit=5" TARGET_PRODUCT=aosp_arm64 TARGET_BUILD_VARIANT=userdebug libbacktrace dist DIST_DIR=$DIST_DIR
