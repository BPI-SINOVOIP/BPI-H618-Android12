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

import com.android.server.wm.flicker.assertions.FlickerSubject
import com.android.server.wm.flicker.traces.FlickerFailureStrategy
import com.google.common.truth.FailureMetadata
import com.google.common.truth.StandardSubjectBuilder

class FocusEventSubject(
    fm: FailureMetadata,
    val event: FocusEvent,
    val trace: EventLogSubject?
) : FlickerSubject(fm, event) {
    override val defaultFacts by lazy { event.toString() }

    /** {@inheritDoc} */
    override fun clone(): FlickerSubject {
        return FocusEventSubject(fm, event, trace)
    }

    fun hasFocus() {
        check("Does not have focus")
            .that(event.hasFocus())
            .isTrue()
    }

    fun hasNotFocus() {
        check("Does not have focus")
            .that(event.hasFocus())
            .isFalse()
    }

    companion object {
        /**
         * Boiler-plate Subject.Factory for EventLogSubject
         *
         * @param trace containing this event
         */
        private fun getFactory(
            trace: EventLogSubject? = null
        ): Factory<FocusEventSubject, FocusEvent> =
            Factory { fm, subject -> FocusEventSubject(fm, subject, trace) }

        /**
         * User-defined entry point
         *
         * @param event Focus event
         * @param trace containing this event
         */
        @JvmStatic
        @JvmOverloads
        fun assertThat(event: FocusEvent, trace: EventLogSubject? = null): FocusEventSubject {
            val strategy = FlickerFailureStrategy()
            val subject = StandardSubjectBuilder.forCustomFailureStrategy(strategy)
                .about(getFactory(trace))
                .that(event) as FocusEventSubject
            strategy.init(subject)
            return subject
        }
    }
}