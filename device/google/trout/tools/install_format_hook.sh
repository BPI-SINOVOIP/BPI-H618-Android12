#!/bin/sh
# Owner: Hanming Zeng (hanmeow@)

# Please note: this script will override any local pre-commit hooks under ./git/hooks

# This script will create a "precommit" file in ./git/hooks to auto
# format code before commit using git clang-format

# To undo this script, run "rm .git/hooks/pre-commit"

# directory where this script is in
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
GIT_ROOT=$(git rev-parse --show-toplevel)

git config --local clangFormat.style file
git config --local --path clangFormat.stylePath $GIT_ROOT/.clang-format

cp "${DIR}/pre-commit" "${GIT_ROOT}/.git/hooks/pre-commit"
chmod +x "${GIT_ROOT}/.git/hooks/pre-commit"

# Track Usage
MY_PATH=${PWD//\//"%2F"}
curl "https://us-central1-si-sw-eng-prod-team.cloudfunctions.net/trackAutoFormatUsage?user=${USER}&pwd=${MY_PATH}&timestamp=$(date +%s)&type=INSTALL" > /dev/null 2>&1
