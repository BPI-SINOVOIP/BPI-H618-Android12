#!/bin/bash

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
#

bash_src_dir=$(realpath $(dirname ${BASH_SOURCE[0]}))
dest_dir=$(realpath "${1:-$PWD}")

echo Initializing an AGL Server build at "${dest_dir}"
mkdir -p "${dest_dir}/manifest"
mkdir -p "${dest_dir}/repo"
cp "${bash_src_dir}/repo_manifest.xml" "${dest_dir}/manifest/default.xml"
cd "${dest_dir}/manifest"
git init > /dev/null
git add default.xml
git commit -am "Manifest" > /dev/null
git_branch_name=$(git branch --show-current)
cd "../repo"
yes | repo init -u ../manifest -b ${git_branch_name} -q
repo sync -c -q -j2
