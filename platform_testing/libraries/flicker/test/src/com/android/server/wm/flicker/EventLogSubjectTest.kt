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

import com.android.server.wm.flicker.traces.eventlog.EventLogSubject
import com.android.server.wm.flicker.traces.eventlog.FocusEvent
import org.junit.Test

/**
 * Contains [EventLogSubject] tests. To run this test: `atest
 * FlickerLibTest:EventLogSubjectTest`
 */
class EventLogSubjectTest {
    @Test
    fun canDetectFocusChanges() {
        val builder = FlickerRunResult.Builder()
        builder.eventLog =
                listOf(FocusEvent(0, "WinB", FocusEvent.Focus.GAINED, "test"),
                        FocusEvent(0, "test WinA window", FocusEvent.Focus.LOST, "test"),
                        FocusEvent(0, "WinB", FocusEvent.Focus.LOST, "test"),
                        FocusEvent(0, "test WinC", FocusEvent.Focus.GAINED, "test"))
        val result = builder.buildEventLogResult().eventLogSubject
        require(result != null) { "Event log subject was not built" }
        result.focusChanges(arrayOf("WinA", "WinB", "WinC"))
                .forAllEntries()
        result.focusChanges(arrayOf("WinA", "WinB")).forAllEntries()
        result.focusChanges(arrayOf("WinB", "WinC")).forAllEntries()
        result.focusChanges(arrayOf("WinA")).forAllEntries()
        result.focusChanges(arrayOf("WinB")).forAllEntries()
        result.focusChanges(arrayOf("WinC")).forAllEntries()
    }

    @Test
    fun canDetectFocusDoesNotChange() {
        val result = FlickerRunResult.Builder().buildEventLogResult().eventLogSubject
        require(result != null) { "Event log subject was not built" }
        result.focusDoesNotChange().forAllEntries()
    }
}