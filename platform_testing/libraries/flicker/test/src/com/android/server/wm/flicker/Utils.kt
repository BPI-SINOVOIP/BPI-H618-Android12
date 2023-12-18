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

package com.android.server.wm.flicker

import android.content.Context
import androidx.test.platform.app.InstrumentationRegistry
import com.android.server.wm.flicker.traces.FlickerSubjectException
import com.android.server.wm.traces.common.layers.LayersTrace
import com.android.server.wm.traces.common.windowmanager.WindowManagerTrace
import com.android.server.wm.traces.parser.layers.LayersTraceParser
import com.android.server.wm.traces.parser.windowmanager.WindowManagerTraceParser
import com.google.common.io.ByteStreams
import com.google.common.truth.ExpectFailure
import com.google.common.truth.TruthFailureSubject
import java.nio.file.Paths

internal fun readWmTraceFromFile(relativePath: String): WindowManagerTrace {
    return try {
        WindowManagerTraceParser.parseFromTrace(readTestFile(relativePath),
            source = Paths.get(relativePath))
    } catch (e: Exception) {
        throw RuntimeException(e)
    }
}

internal fun readWmTraceFromDumpFile(relativePath: String): WindowManagerTrace {
    return try {
        WindowManagerTraceParser.parseFromDump(readTestFile(relativePath))
    } catch (e: Exception) {
        throw RuntimeException(e)
    }
}

internal fun readLayerTraceFromFile(
    relativePath: String,
    ignoreOrphanLayers: Boolean = true
): LayersTrace {
    return try {
        LayersTraceParser.parseFromTrace(readTestFile(relativePath),
            source = Paths.get(relativePath)) { ignoreOrphanLayers }
    } catch (e: Exception) {
        throw RuntimeException(e)
    }
}

@Throws(Exception::class)
internal fun readTestFile(relativePath: String): ByteArray {
    val context: Context = InstrumentationRegistry.getInstrumentation().context
    val inputStream = context.resources.assets.open("testdata/$relativePath")
    return ByteStreams.toByteArray(inputStream)
}

/**
 * Runs `r` and asserts that an exception of type `expectedThrowable` is thrown.
 * @param expectedThrowable the type of throwable that is expected to be thrown
 * @param r the [Runnable] which is run and expected to throw.
 * @throws AssertionError if `r` does not throw, or throws a runnable that is not an
 * instance of `expectedThrowable`.
 */
// TODO: remove once Android migrates to JUnit 4.13, which provides assertThrows
fun assertThrows(expectedThrowable: Class<out Throwable>, r: () -> Any): Throwable {
    try {
        r()
    } catch (t: Throwable) {
        when {
            expectedThrowable.isInstance(t) -> return t
            t is Exception ->
                throw AssertionError("Expected $expectedThrowable, but got ${t.javaClass}", t)
            // Re-throw Errors and other non-Exception throwables.
            else -> throw t
        }
    }
    error("Expected $expectedThrowable, but nothing was thrown")
}

fun assertFailure(failure: Throwable?): TruthFailureSubject {
    val target = when (failure) {
        is FlickerSubjectException -> failure.cause
        is AssertionError -> failure
        else -> error("Expected assertion error, received $failure")
    }
    require(target is AssertionError) { "Unknown failure $target" }
    return ExpectFailure.assertThat(target)
}