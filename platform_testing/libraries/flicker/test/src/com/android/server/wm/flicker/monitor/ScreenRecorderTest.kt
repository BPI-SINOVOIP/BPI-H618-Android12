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

package com.android.server.wm.flicker.monitor

import android.app.Instrumentation
import android.os.SystemClock
import androidx.test.platform.app.InstrumentationRegistry
import com.android.server.wm.flicker.FlickerRunResult
import com.android.server.wm.flicker.getDefaultFlickerOutputDir
import com.google.common.truth.Truth
import org.junit.After
import org.junit.Before
import org.junit.FixMethodOrder
import org.junit.Test
import org.junit.runners.MethodSorters
import java.nio.file.Files

/**
 * Contains [ScreenRecorder] tests. To run this test: `atest
 * FlickerLibTest:ScreenRecorderTest`
 */
@FixMethodOrder(MethodSorters.NAME_ASCENDING)
class ScreenRecorderTest {
    private val instrumentation: Instrumentation = InstrumentationRegistry.getInstrumentation()
    private lateinit var mScreenRecorder: ScreenRecorder
    @Before
    fun setup() {
        val outputDir = getDefaultFlickerOutputDir()
        mScreenRecorder = ScreenRecorder(outputDir, instrumentation.targetContext)
    }

    @After
    fun teardown() {
        mScreenRecorder.stop()
        mScreenRecorder.outputPath.toFile().delete()
    }

    @Test
    fun videoIsRecorded() {
        mScreenRecorder.start()
        SystemClock.sleep(100)
        mScreenRecorder.stop()
        val file = mScreenRecorder.outputPath.toFile()
        Truth.assertWithMessage("Screen recording file not found")
            .that(file.exists())
            .isTrue()
    }

    @Test
    fun videoCanBeSaved() {
        mScreenRecorder.start()
        SystemClock.sleep(100)
        mScreenRecorder.stop()
        val builder = FlickerRunResult.Builder()
        mScreenRecorder.save("test", builder)
        val traces = builder.buildAll().mapNotNull { result ->
            result.traceFiles.firstOrNull {
                it.toString().contains("transition")
            }
        }
        traces.forEach {
            Truth.assertWithMessage("Trace file $it not found").that(Files.exists(it)).isTrue()
        }
        traces.forEach {
            Files.deleteIfExists(it)
        }
    }
}
