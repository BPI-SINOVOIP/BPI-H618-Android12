/*
* Copyright (C) 2016 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#pragma once

#ifdef _MSC_VER
#  define EMUGL_COMMON_API extern "C" __declspec(dllexport)
#else
#  define EMUGL_COMMON_API extern "C" __attribute((visibility("default")))
#endif

EMUGL_COMMON_API void emugl_crash_reporter(const char* message);

// TODO: Put back the varargs impl and stuff
#define crashhandler_die(...)

#undef EMUGL_COMMON_API