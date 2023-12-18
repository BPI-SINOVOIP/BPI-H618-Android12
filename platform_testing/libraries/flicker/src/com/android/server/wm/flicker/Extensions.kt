/*
 * Copyright (C) 2020 The Android Open Source Project
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

@file:JvmName("Extensions")
package com.android.server.wm.flicker

import java.nio.file.Paths

internal const val FLICKER_TAG = "FLICKER"

/**
 * Gets the default flicker output dir.
 * By default the data is stored in /sdcard/flicker instead of
 * using the app's internal data directory to be accessible by
 * other components (i.e. FilePuller)
 */
fun getDefaultFlickerOutputDir() =
        Paths.get("/sdcard/flicker")

internal fun String.containsAny(vararg values: String): Boolean {
    return values.isEmpty() || values.any { search -> this.contains(search) }
}