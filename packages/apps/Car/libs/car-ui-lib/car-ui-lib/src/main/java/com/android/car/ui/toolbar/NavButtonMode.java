/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.car.ui.toolbar;

/**
 * An enum representing the nav button in the toolbar. The nav button can also be thought of
 * as the back button. Besides {@link #DISABLED}, all of these modes are purely aesthetic and don't
 * affect the functionality of the nav button at all.
 */
public enum NavButtonMode {
    /** A back button */
    BACK,
    /** A close button */
    CLOSE,
    /** A down button, used to indicate that the page will animate down when navigating away */
    DOWN,
    /** Don't show the nav button */
    DISABLED,
}
