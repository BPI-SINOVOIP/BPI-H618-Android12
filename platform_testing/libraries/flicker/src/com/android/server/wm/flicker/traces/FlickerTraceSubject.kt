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

package com.android.server.wm.flicker.traces

import com.android.server.wm.flicker.assertions.Assertion
import com.android.server.wm.flicker.assertions.AssertionsChecker
import com.android.server.wm.flicker.assertions.FlickerSubject
import com.google.common.truth.FailureMetadata

/**
 * Base subject for flicker trace assertions
 */
abstract class FlickerTraceSubject<EntrySubject : FlickerSubject>(
    fm: FailureMetadata,
    data: Any?
) : FlickerSubject(fm, data) {
    protected val assertionsChecker = AssertionsChecker<EntrySubject>()
    private var newAssertionBlock = true

    abstract val subjects: List<EntrySubject>

    protected fun addAssertion(name: String, assertion: Assertion<EntrySubject>) {
        if (newAssertionBlock) {
            assertionsChecker.add(name, assertion)
        } else {
            assertionsChecker.append(name, assertion)
        }
        newAssertionBlock = false
    }

    /**
     * Run the assertions for all trace entries
     */
    fun forAllEntries() {
        assertionsChecker.test(subjects)
    }

    /**
     * User-defined entry point for the first trace entry
     */
    fun first(): EntrySubject = subjects.first()

    /**
     * User-defined entry point for the last trace entry
     */
    fun last(): EntrySubject = subjects.last()

    /**
     * Signal that the last assertion set is complete. The next assertion added will start a new
     * set of assertions.
     *
     * E.g.: checkA().then().checkB()
     *
     * Will produce two sets of assertions (checkA) and (checkB) and checkB will only be checked
     * after checkA passes.
     */
    protected fun startAssertionBlock() {
        newAssertionBlock = true
    }

    /**
     * Checks whether all the trace entries on the list are visible for more than one consecutive
     * entry
     *
     * @param [visibleEntries] a list of all the entries with their name and index
     */
    protected fun visibleEntriesShownMoreThanOneConsecutiveTime(
        visibleEntriesProvider: (EntrySubject) -> Set<String>
    ) {
        var lastVisible = visibleEntriesProvider(subjects.first())
        val lastNew = lastVisible.toMutableSet()

        subjects.drop(1).forEachIndexed { index, entrySubject ->
            val currentVisible = visibleEntriesProvider(entrySubject)
            val newVisible = currentVisible.filter { it !in lastVisible }
            lastNew.removeAll(currentVisible)

            if (lastNew.isNotEmpty()) {
                val prevEntry = subjects[index]
                prevEntry.fail("$lastNew is not visible for 2 entries")
            }
            lastNew.addAll(newVisible)
            lastVisible = currentVisible
        }

        if (lastNew.isNotEmpty()) {
            val lastEntry = subjects.last()
            lastEntry.fail("$lastNew is not visible for 2 entries")
        }
    }
}