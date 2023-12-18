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

import com.android.server.wm.flicker.assertions.AssertionsChecker
import com.android.server.wm.flicker.assertions.FlickerSubject
import com.android.server.wm.flicker.traces.FlickerFailureStrategy
import com.android.server.wm.flicker.traces.FlickerSubjectException
import com.android.server.wm.traces.common.ITraceEntry
import com.google.common.truth.FailureMetadata
import com.google.common.truth.StandardSubjectBuilder
import com.google.common.truth.Subject
import org.junit.FixMethodOrder
import org.junit.Test
import org.junit.runners.MethodSorters

/**
 * Contains [AssertionsChecker] tests. To run this test: `atest
 * FlickerLibTest:AssertionsCheckerTest`
 */
@FixMethodOrder(MethodSorters.NAME_ASCENDING)
class AssertionsCheckerTest {
    @Test
    fun emptyRangePasses() {
        val checker = AssertionsChecker<SimpleEntrySubject>()
        checker.add("isData42") { it.isData42() }
        checker.test(emptyList())
    }

    @Test
    fun canCheckChangingAssertions() {
        val checker = AssertionsChecker<SimpleEntrySubject>()
        checker.add("isData42") { it.isData42() }
        checker.add("isData0") { it.isData0() }
        checker.test(getTestEntries(42, 0, 0, 0, 0))
    }

    @Test
    fun canCheckChangingAssertions_withNoAssertions() {
        val checker = AssertionsChecker<SimpleEntrySubject>()
        checker.test(getTestEntries(42, 0, 0, 0, 0))
    }

    @Test
    fun canCheckChangingAssertions_withSingleAssertion() {
        val checker = AssertionsChecker<SimpleEntrySubject>()
        checker.add("isData42") { it.isData42() }
        checker.test(getTestEntries(42, 42, 42, 42, 42))
    }

    @Test
    fun canFailCheckChangingAssertions_ifStartingAssertionFails() {
        val checker = AssertionsChecker<SimpleEntrySubject>()
        checker.add("isData42") { it.isData42() }
        checker.add("isData0") { it.isData0() }
        try {
            checker.test(getTestEntries(0, 0, 0, 0, 0))
        } catch (failure: Throwable) {
            require(failure is FlickerSubjectException) { "Unknown failure $failure" }
            assertFailure(failure.cause)
                .factValue("expected").isEqualTo("42")
            assertFailure(failure.cause)
                .factValue("but was").isEqualTo("0")
        }
    }

    @Test
    fun canCheckChangingAssertions_skipUntilFirstSuccess() {
        val checker = AssertionsChecker<SimpleEntrySubject>()
        checker.skipUntilFirstAssertion()
        checker.add("isData42") { it.isData42() }
        checker.add("isData0") { it.isData0() }
        checker.test(getTestEntries(0, 42, 0, 0, 0))
    }

    @Test
    fun canFailCheckChangingAssertions_ifStartingAssertionAlwaysPasses() {
        val checker = AssertionsChecker<SimpleEntrySubject>()
        checker.add("isData42") { it.isData42() }
        checker.add("isData0") { it.isData0() }
        try {
            checker.test(getTestEntries(42, 42, 42, 42, 42))
        } catch (failure: Throwable) {
            require(failure is FlickerSubjectException) { "Unknown failure $failure" }
            assertFailure(failure.cause)
                .hasMessageThat()
                .contains("Assertion never became false: isData42")
        }
    }

    @Test
    fun canFailCheckChangingAssertions_ifUsingCompoundAssertion() {
        val checker = AssertionsChecker<SimpleEntrySubject>()
        checker.add("isData42/0") { it.isData42().isData0() }
        try {
            checker.test(getTestEntries(0, 0, 0, 0, 0))
        } catch (failure: Throwable) {
            require(failure is FlickerSubjectException) { "Unknown failure $failure" }
            assertFailure(failure.cause)
                .factValue("expected").isEqualTo("42")
            assertFailure(failure.cause)
                .factValue("but was").isEqualTo("0")
        }
    }

    private class SimpleEntrySubject(
        failureMetadata: FailureMetadata,
        private val entry: SimpleEntry
    ) : FlickerSubject(failureMetadata, entry) {
        override val defaultFacts: String = "SimpleEntry(${entry.mData})"
        override fun clone(): FlickerSubject {
            return SimpleEntrySubject(fm, entry)
        }

        fun isData42() = apply {
            check("is42").that(entry.mData).isEqualTo(42)
        }

        fun isData0() = apply {
            check("is0").that(entry.mData).isEqualTo(0)
        }

        companion object {
            /**
             * Boiler-plate Subject.Factory for LayersTraceSubject
             */
            private val FACTORY: Factory<Subject, SimpleEntry> =
                Factory { fm, subject -> SimpleEntrySubject(fm, subject) }

            /**
             * User-defined entry point
             */
            @JvmStatic
            fun assertThat(entry: SimpleEntry): SimpleEntrySubject {
                val strategy = FlickerFailureStrategy()
                val subject = StandardSubjectBuilder.forCustomFailureStrategy(strategy)
                    .about(FACTORY)
                    .that(entry) as SimpleEntrySubject
                strategy.init(subject)
                return subject
            }
        }
    }

    data class SimpleEntry(override val timestamp: Long, val mData: Int) : ITraceEntry

    companion object {
        /**
         * Returns a list of SimpleEntry objects with `data` and incremental timestamps starting
         * at 0.
         */
        private fun getTestEntries(vararg data: Int): List<SimpleEntrySubject> =
                data.indices.map { SimpleEntrySubject
                    .assertThat(SimpleEntry(it.toLong(), data[it])) }
    }
}
