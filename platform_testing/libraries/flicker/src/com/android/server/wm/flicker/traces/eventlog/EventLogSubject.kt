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

package com.android.server.wm.flicker.traces.eventlog

import com.android.server.wm.flicker.assertions.AssertionsChecker
import com.android.server.wm.flicker.assertions.FlickerSubject
import com.google.common.truth.Fact
import com.google.common.truth.FailureMetadata
import com.google.common.truth.Subject.Factory
import com.google.common.truth.Truth

/**
 * Truth subject for [FocusEvent] objects.
 */
class EventLogSubject private constructor(
    failureMetadata: FailureMetadata,
    private val trace: List<FocusEvent>
) : FlickerSubject(failureMetadata, trace) {
    override val defaultFacts: String by lazy {
        val first = subjects.first().defaultFacts
        val last = subjects.last().defaultFacts
        "EventLogSubject($first, $last)"
    }

    /** {@inheritDoc} */
    override fun clone(): FlickerSubject {
        return EventLogSubject(fm, trace)
    }

    private val subjects by lazy {
        trace.map { FocusEventSubject.assertThat(it, this) }
    }

    private val assertionsChecker = AssertionsChecker<FocusEventSubject>()
    private val _focusChanges by lazy {
        val focusList = mutableListOf<String>()
        trace.firstOrNull { !it.hasFocus() }?.let { focusList.add(it.window) }
        focusList + trace.filter { it.hasFocus() }.map { it.window }
    }

    fun focusChanges(windows: Array<out String>) = apply {
        if (windows.isNotEmpty()) {
            val focusChanges = _focusChanges
                .dropWhile { !it.contains(windows.first()) }
                .take(windows.size)
            val success = windows.size <= focusChanges.size &&
                focusChanges.zip(windows).all { (focus, search) -> focus.contains(search) }

            if (!success) {
                fail(Fact.fact("Expected", windows.joinToString(",")),
                    Fact.fact("Found", focusChanges.joinToString(",")))
            }
        }
    }

    fun focusDoesNotChange() = apply {
        check("Focus should not change")
            .that(_focusChanges)
            .isEmpty()
    }

    companion object {
        /**
         * Boiler-plate Subject.Factory for EventLogSubject
         */
        private val FACTORY = Factory { fm: FailureMetadata, subject: List<FocusEvent> ->
            EventLogSubject(fm, subject)
        }

        /**
         * User-defined entry point
         */
        fun assertThat(entry: List<FocusEvent>) =
                Truth.assertAbout(FACTORY).that(entry) as EventLogSubject

        /**
         * Static method for getting the subject factory (for use with assertAbout())
         */
        fun entries(): Factory<EventLogSubject, List<FocusEvent>> {
            return FACTORY
        }
    }

    /**
     * Run the assertions.
     */
    fun forAllEntries(): Unit = assertionsChecker.test(subjects)
}
