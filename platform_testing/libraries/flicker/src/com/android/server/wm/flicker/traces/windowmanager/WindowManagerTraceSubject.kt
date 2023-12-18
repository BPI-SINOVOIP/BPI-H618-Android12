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

package com.android.server.wm.flicker.traces.windowmanager

import com.android.server.wm.flicker.assertions.Assertion
import com.android.server.wm.flicker.assertions.FlickerSubject
import com.android.server.wm.flicker.traces.FlickerFailureStrategy
import com.android.server.wm.flicker.traces.FlickerTraceSubject
import com.android.server.wm.traces.common.Rect
import com.android.server.wm.traces.common.Region
import com.android.server.wm.traces.common.windowmanager.WindowManagerTrace
import com.android.server.wm.traces.common.windowmanager.windows.WindowState
import com.android.server.wm.traces.parser.windowmanager.WindowManagerStateHelper
import com.google.common.truth.FailureMetadata
import com.google.common.truth.FailureStrategy
import com.google.common.truth.StandardSubjectBuilder
import com.google.common.truth.Subject

/**
 * Truth subject for [WindowManagerTrace] objects, used to make assertions over behaviors that
 * occur throughout a whole trace.
 *
 * To make assertions over a trace it is recommended to create a subject using
 * [WindowManagerTraceSubject.assertThat](myTrace). Alternatively, it is also possible to use
 * Truth.assertAbout(WindowManagerTraceSubject.FACTORY), however it will provide less debug
 * information because it uses Truth's default [FailureStrategy].
 *
 * Example:
 *    val trace = WindowManagerTraceParser.parseFromTrace(myTraceFile)
 *    val subject = WindowManagerTraceSubject.assertThat(trace)
 *        .contains("ValidWindow")
 *        .notContains("ImaginaryWindow")
 *        .showsAboveAppWindow("NavigationBar")
 *        .forAllEntries()
 *
 * Example2:
 *    val trace = WindowManagerTraceParser.parseFromTrace(myTraceFile)
 *    val subject = WindowManagerTraceSubject.assertThat(trace) {
 *        check("Custom check") { myCustomAssertion(this) }
 *    }
 */
class WindowManagerTraceSubject private constructor(
    fm: FailureMetadata,
    val trace: WindowManagerTrace
) : FlickerTraceSubject<WindowManagerStateSubject>(fm, trace) {
    override val defaultFacts: String = buildString {
        if (trace.hasSource()) {
            append("Path: ${trace.source}")
            append("\n")
        }
        append("Trace: $trace")
    }

    override val subjects by lazy {
        trace.entries.map { WindowManagerStateSubject.assertThat(it, this) }
    }

    /** {@inheritDoc} */
    override fun clone(): FlickerSubject {
        return WindowManagerTraceSubject(fm, trace)
    }

    /**
     * Signal that the last assertion set is complete. The next assertion added will start a new
     * set of assertions.
     *
     * E.g.: checkA().then().checkB()
     *
     * Will produce two sets of assertions (checkA) and (checkB) and checkB will only be checked
     * after checkA passes.
     */
    fun then(): WindowManagerTraceSubject =
        apply { startAssertionBlock() }

    /**
     * Ignores the first entries in the trace, until the first assertion passes. If it reaches the
     * end of the trace without passing any assertion, return a failure with the name/reason from
     * the first assertion
     *
     * @return
     */
    fun skipUntilFirstAssertion(): WindowManagerTraceSubject =
        apply { assertionsChecker.skipUntilFirstAssertion() }

    fun isEmpty(): WindowManagerTraceSubject = apply {
        check("Trace is empty").that(trace).isEmpty()
    }

    fun isNotEmpty(): WindowManagerTraceSubject = apply {
        check("Trace is not empty").that(trace).isNotEmpty()
    }

    /**
     * Checks if the non-app window with title containing [partialWindowTitle] exists above the app
     * windows and is visible
     *
     * @param partialWindowTitle window title to search
     */
    fun showsAboveAppWindow(vararg partialWindowTitle: String): WindowManagerTraceSubject = apply {
        addAssertion("showsAboveAppWindow($partialWindowTitle)") {
            it.isAboveAppWindow(*partialWindowTitle)
        }
    }

    /**
     * Checks if the non-app window with title containing [partialWindowTitle] exists above the app
     * windows and is invisible
     *
     * @param partialWindowTitle window title to search
     */
    fun hidesAboveAppWindow(vararg partialWindowTitle: String): WindowManagerTraceSubject = apply {
        addAssertion("hidesAboveAppWindow($partialWindowTitle)") {
            it.isAboveAppWindow(*partialWindowTitle, isVisible = false)
        }
    }

    /**
     * Checks if the non-app window with title containing [partialWindowTitle] exists below the app
     * windows and is visible
     *
     * @param partialWindowTitle window title to search
     */
    fun showsBelowAppWindow(vararg partialWindowTitle: String): WindowManagerTraceSubject = apply {
        addAssertion("showsBelowAppWindow($partialWindowTitle)") {
            it.isBelowAppWindow(*partialWindowTitle)
        }
    }

    /**
     * Checks if the non-app window with title containing [partialWindowTitle] exists below the app
     * windows and is invisible
     *
     * @param partialWindowTitle window title to search
     */
    fun hidesBelowAppWindow(vararg partialWindowTitle: String): WindowManagerTraceSubject = apply {
        addAssertion("hidesBelowAppWindow($partialWindowTitle)") {
            it.isBelowAppWindow(*partialWindowTitle, isVisible = false)
        }
    }

    /**
     * Checks if non-app window with title containing the [partialWindowTitle] exists above or
     * below the app windows and is visible
     *
     * @param partialWindowTitle window title to search
     */
    fun showsNonAppWindow(vararg partialWindowTitle: String): WindowManagerTraceSubject = apply {
        addAssertion("showsNonAppWindow($partialWindowTitle)") {
            it.containsNonAppWindow(*partialWindowTitle)
        }
    }

    /**
     * Checks if non-app window with title containing the [partialWindowTitle] exists above or
     * below the app windows and is invisible
     *
     * @param partialWindowTitle window title to search
     */
    fun hidesNonAppWindow(vararg partialWindowTitle: String): WindowManagerTraceSubject = apply {
        addAssertion("hidesNonAppWindow($partialWindowTitle)") {
            it.containsNonAppWindow(*partialWindowTitle, isVisible = false)
        }
    }

    /**
     * Checks if an app window with title containing the [partialWindowTitles] is on top
     *
     * @param partialWindowTitles window title to search
     */
    fun showsAppWindowOnTop(vararg partialWindowTitles: String): WindowManagerTraceSubject = apply {
        val assertionName = "showsAppWindowOnTop(${partialWindowTitles.joinToString(",")})"
        addAssertion(assertionName) {
            check("No window titles to search")
                .that(partialWindowTitles)
                .isNotEmpty()
            it.showsAppWindowOnTop(*partialWindowTitles)
        }
    }

    /**
     * Checks if app window with title containing the [partialWindowTitle] is not on top
     *
     * @param partialWindowTitle window title to search
     */
    fun appWindowNotOnTop(vararg partialWindowTitle: String): WindowManagerTraceSubject = apply {
        addAssertion("hidesAppWindowOnTop($partialWindowTitle)") {
            it.containsAppWindow(*partialWindowTitle, isVisible = false)
        }
    }

    /**
     * Checks if app window with title containing the [partialWindowTitle] is visible
     *
     * @param partialWindowTitle window title to search
     */
    fun showsAppWindow(vararg partialWindowTitle: String): WindowManagerTraceSubject = apply {
        addAssertion("showsAppWindow($partialWindowTitle)") {
            it.containsAppWindow(*partialWindowTitle, isVisible = true)
        }
    }

    /**
     * Checks if app window with title containing the [partialWindowTitle] is invisible
     *
     * @param partialWindowTitle window title to search
     */
    fun hidesAppWindow(vararg partialWindowTitle: String): WindowManagerTraceSubject = apply {
        addAssertion("hidesAppWindow($partialWindowTitle)") {
            it.containsAppWindow(*partialWindowTitle, isVisible = false)
        }
    }

    /**
     * Checks if no app windows containing the [partialWindowTitles] overlap with each other.
     *
     * @param partialWindowTitles partial titles of windows to check
     */
    fun noWindowsOverlap(vararg partialWindowTitles: String): WindowManagerTraceSubject = apply {
        val repr = partialWindowTitles.joinToString(", ")
        require(partialWindowTitles.size > 1) {
            "Must give more than one window to check! (Given $repr)"
        }
        addAssertion("noWindowsOverlap($repr)") {
            it.noWindowsOverlap(*partialWindowTitles)
        }
    }

    /**
     * Checks if the window named [aboveWindowTitle] is above the one named [belowWindowTitle] in
     * z-order.
     *
     * @param aboveWindowTitle partial name of the expected top window
     * @param belowWindowTitle partial name of the expected bottom window
     */
    fun isAboveWindow(
        aboveWindowTitle: String,
        belowWindowTitle: String
    ): WindowManagerTraceSubject = apply {
        require(aboveWindowTitle != belowWindowTitle)
        addAssertion("$aboveWindowTitle is above $belowWindowTitle") {
            it.isAboveWindow(aboveWindowTitle, belowWindowTitle)
        }
    }

    /**
     * Asserts that the visible area covered by the first [WindowState] with [WindowState.title]
     * containing [partialWindowTitle] covers at least [testRegion], that is, if its area of the
     * window's bounds cover each point in the region.
     *
     * @param partialWindowTitle Name of the layer to search
     * @param testRegion Expected visible area of the window
     */
    fun coversAtLeast(
        testRegion: Region,
        partialWindowTitle: String
    ): WindowManagerTraceSubject = apply {
        addAssertion("coversAtLeastRegion($partialWindowTitle, $testRegion)") {
            it.frameRegion(partialWindowTitle).coversAtLeast(testRegion)
        }
    }

    /**
     * Asserts that the visible area covered by the first [WindowState] with [WindowState.title]
     * containing [partialWindowTitle] covers at least [testRegion], that is, if its area of the
     * window's bounds cover each point in the region.
     *
     * @param partialWindowTitle Name of the layer to search
     * @param testRegion Expected visible area of the window
     */
    fun coversAtLeast(
        testRegion: android.graphics.Region,
        partialWindowTitle: String
    ): WindowManagerTraceSubject = apply {
        addAssertion("coversAtLeastRegion($partialWindowTitle, $testRegion)") {
            it.frameRegion(partialWindowTitle).coversAtLeast(testRegion)
        }
    }

    /**
     * Asserts that the visible area covered by the first [WindowState] with [WindowState.title]
     * containing [partialWindowTitle] covers at least [testRect], that is, if its area of the
     * window's bounds cover each point in the region.
     *
     * @param partialWindowTitle Name of the layer to search
     * @param testRect Expected visible area of the window
     */
    fun coversAtLeast(
        testRect: android.graphics.Rect,
        partialWindowTitle: String
    ): WindowManagerTraceSubject = apply {
        addAssertion("coversAtLeastRegion($partialWindowTitle, $testRect)") {
            it.frameRegion(partialWindowTitle).coversAtLeast(testRect)
        }
    }

    /**
     * Asserts that the visible area covered by the first [WindowState] with [WindowState.title]
     * containing [partialWindowTitle] covers at least [testRect], that is, if its area of the
     * window's bounds cover each point in the region.
     *
     * @param partialWindowTitle Name of the layer to search
     * @param testRect Expected visible area of the window
     */
    fun coversAtLeast(
        testRect: Rect,
        partialWindowTitle: String
    ): WindowManagerTraceSubject = apply {
        addAssertion("coversAtLeastRegion($partialWindowTitle, $testRect)") {
            it.frameRegion(partialWindowTitle).coversAtLeast(testRect)
        }
    }

    /**
     * Asserts that the visible area covered by the first [WindowState] with [WindowState.title]
     * containing [partialWindowTitle] covers at most [testRegion], that is, if the area of the
     * window state bounds don't cover any point outside of [testRegion].
     *
     * @param partialWindowTitle Name of the layer to search
     * @param testRegion Expected visible area of the window
     */
    fun coversAtMost(
        testRegion: Region,
        partialWindowTitle: String
    ): WindowManagerTraceSubject = apply {
        addAssertion("coversAtMostRegion($partialWindowTitle, $testRegion)") {
            it.frameRegion(partialWindowTitle).coversAtMost(testRegion)
        }
    }

    /**
     * Asserts that the visible area covered by the first [WindowState] with [WindowState.title]
     * containing [partialWindowTitle] covers at most [testRegion], that is, if the area of the
     * window state bounds don't cover any point outside of [testRegion].
     *
     * @param partialWindowTitle Name of the layer to search
     * @param testRegion Expected visible area of the window
     */
    fun coversAtMost(
        testRegion: android.graphics.Region,
        partialWindowTitle: String
    ): WindowManagerTraceSubject = apply {
        addAssertion("coversAtMostRegion($partialWindowTitle, $testRegion)") {
            it.frameRegion(partialWindowTitle).coversAtMost(testRegion)
        }
    }

    /**
     * Asserts that the visible area covered by the first [WindowState] with [WindowState.title]
     * containing [partialWindowTitle] covers at most [testRect], that is, if the area of the
     * window state bounds don't cover any point outside of [testRect].
     *
     * @param partialWindowTitle Name of the layer to search
     * @param testRect Expected visible area of the window
     */
    fun coversAtMost(
        testRect: Rect,
        partialWindowTitle: String
    ): WindowManagerTraceSubject = apply {
        addAssertion("coversAtMostRegion($partialWindowTitle, $testRect)") {
            it.frameRegion(partialWindowTitle).coversAtMost(testRect)
        }
    }

    /**
     * Asserts that the visible area covered by the first [WindowState] with [WindowState.title]
     * containing [partialWindowTitle] covers at most [testRect], that is, if the area of the
     * window state bounds don't cover any point outside of [testRect].
     *
     * @param partialWindowTitle Name of the layer to search
     * @param testRect Expected visible area of the window
     */
    fun coversAtMost(
        testRect: android.graphics.Rect,
        partialWindowTitle: String
    ): WindowManagerTraceSubject = apply {
        addAssertion("coversAtMostRegion($partialWindowTitle, $testRect)") {
            it.frameRegion(partialWindowTitle).coversAtMost(testRect)
        }
    }

    /**
     * Asserts that the visible area covered by the first [WindowState] with [WindowState.title]
     * containing [partialWindowTitle] covers exactly [testRegion].
     *
     * @param partialWindowTitle Name of the layer to search
     * @param testRegion Expected visible area of the window
     */
    fun coversExactly(
        testRegion: android.graphics.Region,
        partialWindowTitle: String
    ): WindowManagerTraceSubject = apply {
        addAssertion("coversExactly($partialWindowTitle, $testRegion)") {
            it.frameRegion(partialWindowTitle).coversExactly(testRegion)
        }
    }

    /**
     * Asserts that the visible area covered by the first [WindowState] with [WindowState.title]
     * containing [partialWindowTitle] covers exactly [testRect].
     *
     * @param partialWindowTitle Name of the layer to search
     * @param testRect Expected visible area of the window
     */
    fun coversExactly(
        testRect: Rect,
        partialWindowTitle: String
    ): WindowManagerTraceSubject = apply {
        addAssertion("coversExactly($partialWindowTitle, $testRect)") {
            it.frameRegion(partialWindowTitle).coversExactly(testRect)
        }
    }

    /**
     * Asserts that the visible area covered by the first [WindowState] with [WindowState.title]
     * containing [partialWindowTitle] covers exactly [testRect].
     *
     * @param partialWindowTitle Name of the layer to search
     * @param testRect Expected visible area of the window
     */
    fun coversExactly(
        testRect: android.graphics.Rect,
        partialWindowTitle: String
    ): WindowManagerTraceSubject = apply {
        addAssertion("coversExactly($partialWindowTitle, $testRect)") {
            it.frameRegion(partialWindowTitle).coversExactly(testRect)
        }
    }

    /**
     * Checks that all visible layers are shown for more than one consecutive entry
     */
    fun visibleWindowsShownMoreThanOneConsecutiveEntry(
        ignoreWindows: List<String> = listOf(WindowManagerStateHelper.SPLASH_SCREEN_NAME,
            WindowManagerStateHelper.SNAPSHOT_WINDOW_NAME)
    ): WindowManagerTraceSubject = apply {
        visibleEntriesShownMoreThanOneConsecutiveTime { subject ->
            subject.wmState.windowStates
                .filter { it.isVisible }
                .filter {
                    ignoreWindows.none { windowName -> windowName in it.title }
                }
                .map { it.name }
                .toSet()
        }
    }

    /**
     * Executes a custom [assertion] on the current subject
     */
    operator fun invoke(
        name: String,
        assertion: Assertion<WindowManagerStateSubject>
    ): WindowManagerTraceSubject = apply { addAssertion(name, assertion) }

    /**
     * Run the assertions for all trace entries within the specified time range
     */
    fun forRange(startTime: Long, endTime: Long) {
        val subjectsInRange = subjects.filter { it.wmState.timestamp in startTime..endTime }
        assertionsChecker.test(subjectsInRange)
    }

    /**
     * User-defined entry point for the trace entry with [timestamp]
     *
     * @param timestamp of the entry
     */
    fun entry(timestamp: Long): WindowManagerStateSubject =
        subjects.first { it.wmState.timestamp == timestamp }

    companion object {
        /**
         * Boiler-plate Subject.Factory for WmTraceSubject
         */
        private val FACTORY: Factory<Subject, WindowManagerTrace> =
            Factory { fm, subject -> WindowManagerTraceSubject(fm, subject) }

        /**
         * Creates a [WindowManagerTraceSubject] representing a WindowManager trace,
         * which can be used to make assertions.
         *
         * @param trace WindowManager trace
         */
        @JvmStatic
        fun assertThat(trace: WindowManagerTrace): WindowManagerTraceSubject {
            val strategy = FlickerFailureStrategy()
            val subject = StandardSubjectBuilder.forCustomFailureStrategy(strategy)
                .about(FACTORY)
                .that(trace) as WindowManagerTraceSubject
            strategy.init(subject)
            return subject
        }

        /**
         * Static method for getting the subject factory (for use with assertAbout())
         */
        @JvmStatic
        fun entries(): Factory<Subject, WindowManagerTrace> = FACTORY
    }
}
