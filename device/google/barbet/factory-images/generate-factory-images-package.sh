#!/bin/sh

# Copyright 2020 The Android Open Source Project
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

source ../../../common/clear-factory-images-variables.sh
BUILD=6955710
DEVICE=barbet
PRODUCT=barbet
VERSION=RD2A.201106.001
SRCPREFIX=signed-
BOOTLOADER=b9-0.4-6939415
RADIO=g7250-00017-201029-B-6939687
source ../../../common/generate-factory-images-common.sh
