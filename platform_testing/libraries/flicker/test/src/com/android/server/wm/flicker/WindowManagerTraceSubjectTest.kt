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

import com.android.server.wm.flicker.traces.FlickerSubjectException
import com.android.server.wm.flicker.traces.windowmanager.WindowManagerTraceSubject
import com.android.server.wm.flicker.traces.windowmanager.WindowManagerTraceSubject.Companion.assertThat
import org.junit.FixMethodOrder
import org.junit.Test
import org.junit.runners.MethodSorters

/**
 * Contains [WindowManagerTraceSubject] tests. To run this test: `atest
 * FlickerLibTest:WindowManagerTraceSubjectTest`
 */
@FixMethodOrder(MethodSorters.NAME_ASCENDING)
class WindowManagerTraceSubjectTest {
    private val chromeTrace by lazy { readWmTraceFromFile("wm_trace_openchrome.pb") }
    private val imeTrace by lazy { readWmTraceFromFile("wm_trace_ime.pb") }

    @Test
    fun testVisibleAppWindowForRange() {
        assertThat(chromeTrace)
            .showsAppWindowOnTop("NexusLauncherActivity")
            .showsAboveAppWindow("ScreenDecorOverlay")
            .forRange(9213763541297L, 9215536878453L)
        assertThat(chromeTrace)
            .showsAppWindowOnTop("com.android.chrome")
            .showsAppWindow("NexusLauncherActivity")
            .showsAboveAppWindow("ScreenDecorOverlay")
            .then()
            .showsAppWindowOnTop("com.android.chrome")
            .hidesAppWindow("NexusLauncherActivity")
            .showsAboveAppWindow("ScreenDecorOverlay")
            .forRange(9215551505798L, 9216093628925L)
    }

    @Test
    fun testCanTransitionInAppWindow() {
        assertThat(chromeTrace)
            .showsAppWindowOnTop("NexusLauncherActivity")
            .showsAboveAppWindow("ScreenDecorOverlay")
            .then()
            .showsAppWindowOnTop("com.android.chrome")
            .showsAboveAppWindow("ScreenDecorOverlay")
            .forAllEntries()
    }

    @Test
    fun testCanInspectBeginning() {
        assertThat(chromeTrace)
            .first()
            .showsAppWindowOnTop("NexusLauncherActivity")
            .isAboveAppWindow("ScreenDecorOverlay")
    }

    @Test
    fun testCanInspectAppWindowOnTop() {
        assertThat(chromeTrace)
            .first()
            .showsAppWindowOnTop("NexusLauncherActivity", "InvalidWindow")

        val failure = assertThrows(FlickerSubjectException::class.java) {
            assertThat(chromeTrace)
                .first()
                .showsAppWindowOnTop("AnotherInvalidWindow", "InvalidWindow")
                .fail("Could not detect the top app window")
        }
        assertFailure(failure).factValue("Could not find").contains("InvalidWindow")
    }

    @Test
    fun testCanInspectEnd() {
        assertThat(chromeTrace)
            .last()
            .showsAppWindowOnTop("com.android.chrome")
            .isAboveAppWindow("ScreenDecorOverlay")
    }

    @Test
    fun testCanTransitionNonAppWindow() {
        assertThat(imeTrace)
            .skipUntilFirstAssertion()
            .hidesNonAppWindow("InputMethod")
            .then()
            .showsNonAppWindow("InputMethod")
            .forAllEntries()
    }

    @Test(expected = AssertionError::class)
    fun testCanDetectOverlappingWindows() {
        assertThat(imeTrace)
            .noWindowsOverlap("InputMethod", "NavigationBar", "ImeActivity")
            .forAllEntries()
    }

    @Test
    fun testCanTransitionAboveAppWindow() {
        assertThat(imeTrace)
            .skipUntilFirstAssertion()
            .hidesAboveAppWindow("InputMethod")
            .then()
            .showsAboveAppWindow("InputMethod")
            .forAllEntries()
    }

    @Test
    fun testCanTransitionBelowAppWindow() {
        val trace = readWmTraceFromFile("wm_trace_open_app_cold.pb")
        assertThat(trace)
            .skipUntilFirstAssertion()
            .showsBelowAppWindow("Wallpaper")
            .then()
            .hidesBelowAppWindow("Wallpaper")
            .forAllEntries()
    }

    @Test
    fun testCanDetectVisibleWindowsMoreThanOneConsecutiveEntry() {
        val trace = readWmTraceFromFile("wm_trace_valid_visible_windows.pb")
        assertThat(trace).visibleWindowsShownMoreThanOneConsecutiveEntry().forAllEntries()
    }
}
