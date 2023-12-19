#!/bin/bash

# Fail on any error.
set -e

DEST="${KOKORO_ARTIFACTS_DIR}/dest"
OUT="${KOKORO_ARTIFACTS_DIR}/out"
PYTHON_SRC=${KOKORO_ARTIFACTS_DIR}/git/cpython3

BASEDIR="$( cd "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
if [ "$(uname)" == "Darwin" ]; then
    # http://g3doc/devtools/kokoro/g3doc/userdocs/macos/selecting_xcode
    sudo xcode-select -s /Applications/Xcode_12.2.app/Contents/Developer
    export SDKROOT=/Applications/Xcode_12.2.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk
    (cd "${PYTHON_SRC}"; git apply "${BASEDIR}/0001-Enable-arm64-builds.patch")
fi

python3 --version
python3 ${BASEDIR}/build.py "${PYTHON_SRC}" "${OUT}" "${DEST}" "${KOKORO_BUILD_ID}"
