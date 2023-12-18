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

private const val MILLISECOND_AS_NANOSECONDS: Long = 1000000
private const val SECOND_AS_NANOSECONDS: Long = 1000000000
private const val MINUTE_AS_NANOSECONDS: Long = 60000000000
private const val HOUR_AS_NANOSECONDS: Long = 3600000000000
private const val DAY_AS_NANOSECONDS: Long = 86400000000000

internal fun prettyTimestamp(timestampNs: Long): String {
    // Necessary for compatibility with JS Number
    var remainingNs = "$timestampNs".toLong()
    val prettyTimestamp = StringBuilder()

    val timeUnitToNanoSeconds = mapOf(
        "d" to DAY_AS_NANOSECONDS,
        "h" to HOUR_AS_NANOSECONDS,
        "m" to MINUTE_AS_NANOSECONDS,
        "s" to SECOND_AS_NANOSECONDS,
        "ms" to MILLISECOND_AS_NANOSECONDS
    )

    for ((timeUnit, ns) in timeUnitToNanoSeconds) {
        val convertedTime = remainingNs / ns
        remainingNs %= ns
        prettyTimestamp.append("$convertedTime$timeUnit")
    }

    return prettyTimestamp.toString()
}