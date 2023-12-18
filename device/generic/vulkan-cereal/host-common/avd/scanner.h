// Copyright 2020 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

// Implements a simple helper object used to retrieve the list of
// available Android Virtual Devices.
//
// Typical usage is:
//
//     AvdScanner* scanner = avdScanner_new(NULL);
//     while ((name = avdScanner_next(scanner)) != NULL) {
//        printf("%s\n", name);
//     }
//     avdScanner_free(scanner);
//

// Opaque type to an object used to scan all available AVDs
// under $ANDROID_SDK_HOME.
typedef struct AvdScanner AvdScanner;

// Create a new AVD scanner instance, used to parse the directory
// at |sdk_home|. If this parameter is NULL, this will find the
// directory from the environment, e.g. ANDROID_SDK_HOME if it is
// defined, or a platform-specific location (e.g. ~/.android/ on Unix).
AvdScanner* avdScanner_new(const char* sdk_home);

// Return the name of the next AVD detected by the scanner.
// This will be NULL at the end of the scan.
const char* avdScanner_next(AvdScanner* scanner);

// Release an AvdScanner object and associated resources.
// This can be called before the end of a scan.
void avdScanner_free(AvdScanner* scanner);

ANDROID_END_HEADER
