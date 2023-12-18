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

package com.android.server.wm.traces.common

import kotlin.math.min

/**
 * A formatter to print floats with 3 decimal digits.
 *
 * This is necessary because multiplatform kotlin projects don't support String.format
 * yet (issue KT-21644)
 */
object FloatFormatter {
    fun format(value: Float): String {
        val strValue = value.toString()
        val dotIndex = strValue.indexOf(".")
        return if (dotIndex > -1) {
            strValue.substring(0, min(strValue.length, dotIndex + 4))
        } else {
            "$strValue.000"
        }
    }
}