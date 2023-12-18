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
 * An enum describing the possible search states a toolbar could be in.
 * See {@link ToolbarController}.
 */
public enum SearchMode {
    /** Don't show a search bar */
    DISABLED,
    /** Show a search bar */
    SEARCH,
    /**
     * Show a search bar, but don't include any visual indication it's for searching.
     * It could just be for editing text.
     */
    EDIT,
}
