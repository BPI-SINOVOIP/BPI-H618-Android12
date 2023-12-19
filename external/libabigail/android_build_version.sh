#!/bin/sh

set -x
set -e

dir="$1"
out="$2"

version=$(git -C "$dir" rev-parse --short=8 HEAD)-soong

echo "#define ANDROID_BUILD_VERSION \"${version}\"" > "$out"
