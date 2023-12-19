#!/bin/bash
# Updater script for bazel prebuilt binaries in AOSP.
#
# This script handles both linux and darwin binaries in a single invocation. See
# README.md for more details.
#
# Usage: update.sh <commit>

set -euo pipefail

function err() {
    >&2 echo "$@"
    exit 1
}

# Check that the necessary tools are installed.
function check_prereqs() {
    if ! [[ "${PWD}" =~ .*/prebuilts/bazel/linux-x86_64$ ]]; then
        err "Error: Run this script from within the prebuilts/bazel/linux-x86_64 directory."
    fi

    for cmd in jq curl; do
        if ! command -v "${cmd}" &> /dev/null; then
            err "Error: This script requires ${cmd}. Install it and ensure it is on your PATH."
        fi
    done
}

check_prereqs
commit="$1"; shift

ci_url="https://storage.googleapis.com/bazel-builds/metadata/${commit}.json"
platforms_json="$(curl -s "${ci_url}" | jq '{ platforms: .platforms }')"
if [[ $? != 0 ]]; then
    err "Failed to download or parse ${ci_url}. Exiting."
fi

darwin_nojdk_url="$(echo "${platforms_json}" | jq --raw-output '.[].macos.nojdk_url')"
darwin_nojdk_sha256="$(echo "${platforms_json}" | jq --raw-output '.[].macos.nojdk_sha256')"

linux_nojdk_url="$(echo "${platforms_json}" | jq --raw-output '.[].linux.nojdk_url')"
linux_nojdk_sha256="$(echo "${platforms_json}" | jq --raw-output '.[].linux.nojdk_sha256')"

function download_and_verify() {
    local os=$1; shift
    local url=$1; shift
    local sha256=$1; shift

    echo "Cleaning previous ${os} bazel binary.."
    rm -f "bazel_nojdk-*-${os}-x86_64"

    echo "Downloading ${os} bazel binary for ${commit}.."
    downloaded_file="bazel_nojdk-${commit}-${os}-x86_64"
    curl "${url}" --output "${downloaded_file}"
    echo "Verifying checksum for ${downloaded_file} to be ${sha256}.."
    echo "${sha256} ${downloaded_file}" | sha256sum --check --status

    echo "Setting up bazel symlink for ${os}.."
    rm -f bazel && ln -s "${downloaded_file}" bazel
    chmod +x "${downloaded_file}"
}


# Update Linux binary.
download_and_verify "linux" "${linux_nojdk_url}" "${linux_nojdk_sha256}"

(
    # Update macOS binary.
    cd "$(dirname "$0")/../darwin-x86_64"
    download_and_verify "darwin" "${darwin_nojdk_url}" "${darwin_nojdk_sha256}"
)

echo "Done."
