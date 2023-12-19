#!/bin/bash

set -o errexit
set -o xtrace

DIR=$(dirname $0)

"${DIR}"/generate_bp.py "${DIR}"/selected_targets.json | \
  grep -v 'PACKAGE_VERSION' | \
  grep -v 'PACKAGE_STRING' >"${DIR}"/../Android.bp

# The alsa device causes a double lock on a mutex, don't use it
sed -i -e 's/WEBRTC_ENABLE_LINUX_ALSA/WEBRTC_DUMMY_FILE_DEVICES/g' "${DIR}/../Android.bp"

source "${DIR}"/../../../build/envsetup.sh

bpfmt -w "${DIR}"/../Android.bp

