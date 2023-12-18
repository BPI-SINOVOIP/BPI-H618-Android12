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

package com.android.server.wm.flicker.assertions

import com.android.server.wm.flicker.FlickerRunResult
import kotlin.reflect.KClass

/**
 * Class containing basic data about a trace assertion for Flicker DSL
 */
data class AssertionData internal constructor(
    /**
     * Segment of the trace where the assertion will be applied (e.g., start, end).
     */
    @JvmField val tag: String,
    /**
     * Expected run result type
     */
    @JvmField val expectedSubjectClass: KClass<out FlickerSubject>,
    /**
     * Assertion command
     */
    @JvmField val assertion: FlickerSubject.() -> Unit
) {
    /**
     * Extracts the data from the result and executes the assertion
     *
     * @param run Run to be asserted
     */
    fun checkAssertion(run: FlickerRunResult) {
        val subjects = run.getSubjects().firstOrNull { expectedSubjectClass.isInstance(it) }
        subjects?.run { assertion(this) }
    }
}
