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

#include <stdint.h>
// #include "android/settings-agent.h"
// #include "android/skin/rect.h"
// #include "android/utils/compiler.h"

// Window agent's possible message types
typedef enum {
    WINDOW_MESSAGE_GENERIC,
    WINDOW_MESSAGE_INFO,
    WINDOW_MESSAGE_WARNING,
    WINDOW_MESSAGE_ERROR,
    WINDOW_MESSAGE_OK,
} WindowMessageType;

static const int kWindowMessageTimeoutInfinite = -1;

typedef struct EmulatorWindow EmulatorWindow;

typedef void (*UiUpdateFunc)(void* data);

typedef struct QAndroidEmulatorWindowAgent {
    // Get a pointer to the emulator window structure.
    EmulatorWindow* (*getEmulatorWindow)();

    // Rotate the screen clockwise by 90 degrees.
    // Returns true on success, false otherwise.
    bool (*rotate90Clockwise)(void);

    // Rotate to specific |rotation|
    bool (*rotate)(int skinRotation);

    // Returns the current rotation.
    int (*getRotation)(void);

    // Shows a message to the user.
    void (*showMessage)(const char* message,
                        WindowMessageType type,
                        int timeoutMs);

    // Shows a message to the user + custom dismiss op.
    void (*showMessageWithDismissCallback)(const char* message,
                                           WindowMessageType type,
                                           const char* dismissText,
                                           void* context,
                                           void (*func)(void*),
                                           int timeoutMs);
    // Fold/Unfold device
    bool (*fold)(bool is_fold);
    // Query folded state
    bool (*isFolded)(void);
    bool (*getFoldedArea)(int* x, int* y, int* w, int* h);

// Update UI indicator which shows which foldable posture device is in
    void (*updateFoldablePostureIndicator)(bool confirmFoldedArea);

    // Set the UI display region
    void (*setUIDisplayRegion)(int, int, int, int);
    bool (*getMultiDisplay)(uint32_t,
                            int32_t*,
                            int32_t*,
                            uint32_t*,
                            uint32_t*,
                            uint32_t*,
                            uint32_t*,
                            bool*);
    void (*setNoSkin)(void);
    void (*restoreSkin)(void);
    void (*updateUIMultiDisplayPage)(uint32_t);
    bool (*getMonitorRect)(uint32_t*, uint32_t*);
    // return true if extended controls window's visibility has changed.
    bool (*startExtendedWindow)(void);
    bool (*quitExtendedWindow)(void);
    bool (*setUiTheme)(int settingsTheme);
    void (*runOnUiThread)(UiUpdateFunc f, void* data, bool wait);
    bool (*isRunningInUiThread)(void);
} QAndroidEmulatorWindowAgent;
