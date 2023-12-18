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

/* Maximum numbers of emulators that can run concurrently without
 * forcing their port numbers with the -port option.
 * This is not a hard limit. Instead, when starting up, the program
 * will try to bind to localhost ports 5554 + n*2, for n in
 * [0..MAX_EMULATORS), if it fails, it will refuse to start.
 * You can route around this by using the -port or -ports options
 * to specify the ports manually.
 */
#define MAX_ANDROID_EMULATORS  16

#define ANDROID_CONSOLE_BASEPORT 5554
