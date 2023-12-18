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

set -e

build_common() {
    local current_dir=`pwd`
    local bash_src_dir=$(realpath $(dirname ${BASH_SOURCE[0]}))
    local build_type=$1
    local cmake_options=""
    shift
    if [[ ${build_type} == "agl" ]]; then
        cmake_options="${cmake_options} -DCMAKE_TOOLCHAIN_FILE=${bash_src_dir}/toolchain/agl_toolchain.cmake"
    fi

    mkdir -p ${TROUT_SRC_ROOT:-${bash_src_dir}}/out/${build_type}_build && cd $_
    cmake -G Ninja ${cmake_options} ../..
    ninja $@
    cd ${current_dir}
}

build_host_tools() {
    build_common host $@
}

build_agl_service() {
    build_common agl $@
}

if [[ ! $(which aprotoc) && ! $(which protoc-gen-grpc-cpp-plugin) ]]; then
    build_host_tools protoc grpc_cpp_plugin
fi

build_agl_service android_audio_controller
build_agl_service android_audio_controller_test
build_agl_service dumpstate_grpc_server
build_agl_service dumpstate_tests
build_agl_service garage_mode_helper
build_agl_service vehicle_hal_grpc_server
build_agl_service watchdog_test_service
