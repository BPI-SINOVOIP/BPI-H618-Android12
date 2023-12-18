#!/bin/sh
# Owner: Hanming Zeng (hanmeow@)
# Running this script will uninstall git pre-commit hook

# directory where this script is in
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

rm "${DIR}/../.git/hooks/pre-commit"

# Track Usage
MY_PATH=${PWD//\//"%2F"}
curl "https://us-central1-si-sw-eng-prod-team.cloudfunctions.net/trackAutoFormatUsage?user=${USER}&pwd=${MY_PATH}&timestamp=$(date +%s)&type=UNINSTALL" > /dev/null 2>&1
