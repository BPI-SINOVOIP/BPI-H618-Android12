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

import com.android.server.wm.flicker.assertions.AssertionData
import com.android.server.wm.flicker.assertions.FlickerAssertionError
import com.google.common.truth.Truth

/**
 * Result of a flicker run, including transitions, errors and create tags
 */
data class FlickerResult(
    /**
     * Result of each transition run
     */
    @JvmField val runs: List<FlickerRunResult> = listOf(),
    /**
     * List of test created during the execution
     */
    @JvmField val tags: Set<String> = setOf(),
    /**
     * Error which happened during the transition
     */
    @JvmField val error: Throwable? = null
) {
    /**
     * List of failures during assertion
     */
    private val failures: MutableList<FlickerAssertionError> = mutableListOf()

    /**
     * Asserts if the transition of this flicker test has ben executed
     */
    private fun checkIsExecuted() {
        Truth.assertWithMessage(error?.message).that(error).isNull()
        Truth.assertWithMessage("Transition was not executed").that(runs).isNotEmpty()
    }

    /**
     * Run the assertions on the trace
     *
     * @throws AssertionError If the assertions fail or the transition crashed
     */
    internal fun checkAssertions(
        assertions: List<AssertionData>
    ): List<FlickerAssertionError> {
        checkIsExecuted()
        val currFailures: List<FlickerAssertionError> = runs.flatMap { run ->
            assertions.filter { it.tag == run.assertionTag }.mapNotNull { assertion ->
                try {
                    assertion.checkAssertion(run)
                    null
                } catch (error: Throwable) {
                    FlickerAssertionError(error, assertion, run)
                }
            }
        }
        failures.addAll(currFailures)
        return currFailures
    }

    /**
     * Remove from the device the trace files associated with passed runs.
     *
     * If an test fails, or if the transition crashes, retain all traces related to
     * that run.
     */
    fun cleanUp() {
        if (error != null) {
            return
        }
        runs.forEach {
            if (it.canDelete(failures)) {
                it.cleanUp()
            }
        }
    }

    fun isEmpty(): Boolean = error == null && runs.isEmpty()

    fun isNotEmpty(): Boolean = !isEmpty()
}