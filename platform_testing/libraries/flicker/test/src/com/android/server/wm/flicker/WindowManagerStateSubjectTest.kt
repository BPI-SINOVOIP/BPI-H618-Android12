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

import android.graphics.Region
import com.android.server.wm.flicker.traces.FlickerSubjectException
import com.android.server.wm.flicker.traces.windowmanager.WindowManagerStateSubject
import com.android.server.wm.flicker.traces.windowmanager.WindowManagerTraceSubject.Companion.assertThat
import com.android.server.wm.traces.common.windowmanager.WindowManagerTrace
import org.junit.FixMethodOrder
import org.junit.Test
import org.junit.runners.MethodSorters
import java.lang.AssertionError

/**
 * Contains [WindowManagerStateSubject] tests. To run this test: `atest
 * FlickerLibTest:WindowManagerStateSubjectTest`
 */
@FixMethodOrder(MethodSorters.NAME_ASCENDING)
class WindowManagerStateSubjectTest {
    private val trace: WindowManagerTrace by lazy { readWmTraceFromFile("wm_trace_openchrome.pb") }

    @Test
    fun canDetectAboveAppWindowVisibility_isVisible() {
        assertThat(trace)
            .entry(9213763541297)
            .isAboveAppWindow("NavigationBar")
            .isAboveAppWindow("ScreenDecorOverlay")
            .isAboveAppWindow("StatusBar")
    }

    @Test
    fun canDetectAboveAppWindowVisibility_isInvisible() {
        val subject = assertThat(trace).entry(9213763541297)
        var failure = assertThrows(AssertionError::class.java) {
            subject.isAboveAppWindow("pip-dismiss-overlay")
        }
        assertFailure(failure).factValue("Is Invisible").contains("pip-dismiss-overlay")

        failure = assertThrows(AssertionError::class.java) {
            subject.isAboveAppWindow("NavigationBar", isVisible = false)
        }
        assertFailure(failure).factValue("Is Visible").contains("NavigationBar")
    }

    @Test
    fun canDetectWindowCoversAtLeastRegion_exactSize() {
        val entry = assertThat(trace)
            .entry(9213763541297)

        entry.frameRegion("StatusBar").coversAtLeast(Region(0, 0, 1440, 171))
        entry.frameRegion("com.google.android.apps.nexuslauncher")
            .coversAtLeast(Region(0, 0, 1440, 2960))
    }

    @Test
    fun canDetectWindowCoversAtLeastRegion_smallerRegion() {
        val entry = assertThat(trace)
            .entry(9213763541297)
        entry.frameRegion("StatusBar").coversAtLeast(Region(0, 0, 100, 100))
        entry.frameRegion("com.google.android.apps.nexuslauncher")
            .coversAtLeast(Region(0, 0, 100, 100))
    }

    @Test
    fun canDetectWindowCoversAtLeastRegion_largerRegion() {
        val subject = assertThat(trace).entry(9213763541297)
        var failure = assertThrows(FlickerSubjectException::class.java) {
            subject.frameRegion("StatusBar").coversAtLeast(Region(0, 0, 1441, 171))
        }
        assertFailure(failure).factValue("Uncovered region").contains("SkRegion((1440,0,1441,171))")

        failure = assertThrows(FlickerSubjectException::class.java) {
            subject.frameRegion("com.google.android.apps.nexuslauncher")
                .coversAtLeast(Region(0, 0, 1440, 2961))
        }
        assertFailure(failure).factValue("Uncovered region")
            .contains("SkRegion((0,2960,1440,2961))")
    }

    @Test
    fun canDetectWindowCoversAtMostRegion_extactSize() {
        val entry = assertThat(trace)
            .entry(9213763541297)
        entry.frameRegion("StatusBar").coversAtMost(Region(0, 0, 1440, 171))
        entry.frameRegion("com.google.android.apps.nexuslauncher")
            .coversAtMost(Region(0, 0, 1440, 2960))
    }

    @Test
    fun canDetectWindowCoversAtMostRegion_smallerRegion() {
        val subject = assertThat(trace).entry(9213763541297)
        var failure = assertThrows(FlickerSubjectException::class.java) {
            subject.frameRegion("StatusBar").coversAtMost(Region(0, 0, 100, 100))
        }
        assertFailure(failure).factValue("Out-of-bounds region")
            .contains("SkRegion((100,0,1440,100)(0,100,1440,171))")

        failure = assertThrows(FlickerSubjectException::class.java) {
            subject.frameRegion("com.google.android.apps.nexuslauncher")
                .coversAtMost(Region(0, 0, 100, 100))
        }
        assertFailure(failure).factValue("Out-of-bounds region")
            .contains("SkRegion((100,0,1440,100)(0,100,1440,2960))")
    }

    @Test
    fun canDetectWindowCoversAtMostRegion_largerRegion() {
        val entry = assertThat(trace)
            .entry(9213763541297)

        entry.frameRegion("StatusBar").coversAtMost(Region(0, 0, 1441, 171))
        entry.frameRegion("com.google.android.apps.nexuslauncher")
            .coversAtMost(Region(0, 0, 1440, 2961))
    }

    @Test
    fun canDetectBelowAppWindowVisibility() {
        assertThat(trace)
            .entry(9213763541297)
            .containsNonAppWindow("wallpaper")
    }

    @Test
    fun canDetectAppWindowVisibility() {
        assertThat(trace)
            .entry(9213763541297)
            .containsAppWindow("com.google.android.apps.nexuslauncher")

        assertThat(trace)
            .entry(9215551505798)
            .containsAppWindow("com.android.chrome")
    }

    @Test
    fun canFailWithReasonForVisibilityChecks_windowNotFound() {
        val failure = assertThrows(FlickerSubjectException::class.java) {
            assertThat(trace)
                .entry(9213763541297)
                .containsNonAppWindow("ImaginaryWindow")
        }
        assertFailure(failure).factValue("Could not find")
            .contains("ImaginaryWindow")
    }

    @Test
    fun canFailWithReasonForVisibilityChecks_windowNotVisible() {
        val failure = assertThrows(FlickerSubjectException::class.java) {
            assertThat(trace)
                .entry(9213763541297)
                .containsNonAppWindow("InputMethod")
        }
        assertFailure(failure).factValue("Is Invisible")
            .contains("InputMethod")
    }

    @Test
    fun canDetectAppZOrder() {
        assertThat(trace)
            .entry(9215551505798)
            .containsAppWindow("com.google.android.apps.nexuslauncher", isVisible = true)
            .showsAppWindowOnTop("com.android.chrome")
    }

    @Test
    fun canFailWithReasonForZOrderChecks_windowNotOnTop() {
        val failure = assertThrows(FlickerSubjectException::class.java) {
            assertThat(trace)
                .entry(9215551505798)
                .showsAppWindowOnTop("com.google.android.apps.nexuslauncher")
        }
        assertFailure(failure)
            .factValue("Found")
            .contains("Splash Screen com.android.chrome")
    }
}