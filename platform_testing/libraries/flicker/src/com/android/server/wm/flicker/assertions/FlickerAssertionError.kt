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

package com.android.server.wm.flicker.assertions

import com.android.server.wm.flicker.FlickerRunResult
import com.android.server.wm.flicker.traces.FlickerSubjectException
import java.nio.file.Path
import kotlin.AssertionError

class FlickerAssertionError(
    cause: Throwable,
    @JvmField val assertion: AssertionData,
    @JvmField val iteration: Int,
    @JvmField val assertionTag: String,
    @JvmField val traceFiles: List<Path>
) : AssertionError(cause) {
    constructor(cause: Throwable, assertion: AssertionData, run: FlickerRunResult)
        : this(cause, assertion, run.iteration, run.assertionTag, run.traceFiles)

    override val message: String
        get() = buildString {
            append("\n")
            append("Test failed")
            append("\n")
            append("Iteration: ")
            append(iteration)
            append("\n")
            append("Tag: ")
            append(assertionTag)
            append("\n")
            append("Files: ")
            append("\n")
            traceFiles.forEach {
                append("\t")
                append(it)
                append("\n")
            }
            // For subject exceptions, add the facts (layer/window/entry/etc)
            // and the original cause of failure
            if (cause is FlickerSubjectException) {
                append(cause.facts)
                append("\n")
                cause.cause?.message?.let { append(it) }
            } else {
                cause?.message?.let { append(it) }
            }
        }
}