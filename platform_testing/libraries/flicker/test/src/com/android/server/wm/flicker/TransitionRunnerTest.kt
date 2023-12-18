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

import androidx.test.platform.app.InstrumentationRegistry
import com.android.server.wm.flicker.dsl.FlickerBuilder
import com.google.common.truth.Truth
import org.junit.FixMethodOrder
import org.junit.Test
import org.junit.runners.MethodSorters

/**
 * Contains [TransitionRunnerTest] and [TransitionRunnerCached] tests.
 *
 * To run this test: `atest FlickerLibTest:TransitionRunnerTest`
 */
@FixMethodOrder(MethodSorters.NAME_ASCENDING)
class TransitionRunnerTest {
    private val instrumentation = InstrumentationRegistry.getInstrumentation()

    @Test
    fun canRunTransition() {
        val runner = TransitionRunner()
        var executed = false
        val flicker = FlickerBuilder(instrumentation)
            .apply {
                transitions {
                    executed = true
                }
            }.build(runner)
        Truth.assertThat(executed).isFalse()
        val result = runner.execute(flicker)
        Truth.assertThat(executed).isTrue()
        Truth.assertThat(result.error).isNull()
        Truth.assertThat(result.runs).hasSize(4)
    }

    @Test
    fun canRunTransitionCached() {
        val runner = TransitionRunnerCached()
        var executed = false
        val flicker = FlickerBuilder(instrumentation)
            .apply {
                transitions {
                    executed = true
                }
            }.build(runner)
        val result = runner.execute(flicker)
        executed = false
        val cachedResult = runner.execute(flicker)
        Truth.assertThat(executed).isFalse()
        Truth.assertThat(cachedResult).isEqualTo(result)
    }
}