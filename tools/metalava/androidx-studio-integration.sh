#!/bin/bash
set -e

cd "$(dirname $0)/../../"
SCRIPT_DIR="$(pwd)"
echo "Script running from $(pwd)"

# resolve DIST_DIR
if [ -z "$DIST_DIR" ]; then
  DIST_DIR="$SCRIPT_DIR/out/dist"
fi
mkdir -p "$DIST_DIR"

export OUT_DIR=out
export DIST_DIR="$DIST_DIR"

JAVA_HOME="$(pwd)/prebuilts/studio/jdk/jdk11/linux" tools/gradlew -p tools/ publishLocal --stacktrace

export LINT_VERSION=`grep -oP "(?<=baseVersion = ).*" tools/buildSrc/base/version.properties`
export LINT_REPO="$(pwd)/out/repo"

tools/gradlew -p tools/metalava --no-daemon --stacktrace -PlintRepo=$LINT_REPO -PlintVersion=$LINT_VERSION
