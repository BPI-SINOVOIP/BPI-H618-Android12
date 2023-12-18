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

import android.content.ComponentName
import android.view.Display
import com.android.server.wm.flicker.assertions.Assertion
import com.android.server.wm.flicker.assertions.FlickerSubject
import com.android.server.wm.flicker.containsAny
import com.android.server.wm.flicker.traces.FlickerFailureStrategy
import com.android.server.wm.flicker.traces.RegionSubject
import com.android.server.wm.traces.common.windowmanager.WindowManagerState
import com.android.server.wm.traces.common.windowmanager.windows.Activity
import com.android.server.wm.traces.common.windowmanager.windows.WindowState
import com.android.server.wm.traces.parser.toActivityName
import com.android.server.wm.traces.parser.toAndroidRegion
import com.android.server.wm.traces.parser.toWindowName
import com.google.common.truth.Fact
import com.google.common.truth.FailureMetadata
import com.google.common.truth.FailureStrategy
import com.google.common.truth.StandardSubjectBuilder
import com.google.common.truth.Subject
import com.google.common.truth.Subject.Factory

/**
 * Truth subject for [WindowManagerState] objects, used to make assertions over behaviors that
 * occur on a single WM state.
 *
 * To make assertions over a specific state from a trace it is recommended to create a subject
 * using [WindowManagerTraceSubject.assertThat](myTrace) and select the specific state using:
 *     [WindowManagerTraceSubject.first]
 *     [WindowManagerTraceSubject.last]
 *     [WindowManagerTraceSubject.entry]
 *
 * Alternatively, it is also possible to use [WindowManagerStateSubject.assertThat](myState) or
 * Truth.assertAbout([WindowManagerStateSubject.getFactory]), however they will provide less debug
 * information because it uses Truth's default [FailureStrategy].
 *
 * Example:
 *    val trace = WindowManagerTraceParser.parseFromTrace(myTraceFile)
 *    val subject = WindowManagerTraceSubject.assertThat(trace).first()
 *        .contains("ValidWindow")
 *        .notContains("ImaginaryWindow")
 *        .showsAboveAppWindow("NavigationBar")
 *        .invoke { myCustomAssertion(this) }
 */
class WindowManagerStateSubject private constructor(
    fm: FailureMetadata,
    val wmState: WindowManagerState,
    val trace: WindowManagerTraceSubject?
) : FlickerSubject(fm, wmState) {
    override val defaultFacts = "${trace?.defaultFacts ?: ""}\nEntry: $wmState"

    val subjects by lazy {
        wmState.windowStates.map { WindowStateSubject.assertThat(it, this) }
    }

    /**
     * Executes a custom [assertion] on the current subject
     */
    operator fun invoke(assertion: Assertion<WindowManagerState>): WindowManagerStateSubject =
        apply { assertion(this.wmState) }

    /** {@inheritDoc} */
    override fun clone(): FlickerSubject {
        return WindowManagerStateSubject(fm, wmState, trace)
    }

    /**
     * Asserts that the current WindowManager state doesn't contain [WindowState]s
     */
    fun isEmpty(): WindowManagerStateSubject = apply {
        check("State is empty")
            .that(wmState.windowStates)
            .isEmpty()
    }

    /**
     * Asserts that the current WindowManager state contains [WindowState]s
     */
    fun isNotEmpty(): WindowManagerStateSubject = apply {
        check("State is not empty")
            .that(wmState.windowStates)
            .isNotEmpty()
    }

    /**
     * Obtains the region occupied by all windows with name containing any of [partialWindowTitles]
     *
     * @param partialWindowTitles Name of the layer to search
     */
    fun frameRegion(vararg partialWindowTitles: String): RegionSubject {
        val selectedWindows = subjects.filter { it.name.containsAny(*partialWindowTitles) }

        if (selectedWindows.isEmpty()) {
            fail("Could not find", selectedWindows.joinToString(", "))
        }

        val visibleWindows = selectedWindows.filter { it.isVisible }
        val frameRegions = visibleWindows.mapNotNull { it.windowState?.frameRegion }.toTypedArray()
        return RegionSubject.assertThat(frameRegions, selectedWindows)
    }

    /**
     * Asserts that the WindowManager state contains a [WindowState] with [WindowState.title]
     * containing any of [partialWindowTitles].
     *
     * @param partialWindowTitles window titles to search to search
     */
    fun contains(vararg partialWindowTitles: String): WindowManagerStateSubject = apply {
        val found = if (partialWindowTitles.isNotEmpty()) {
            wmState.windowStates.any { it.name.containsAny(*partialWindowTitles) }
        } else {
            wmState.windowStates.isNotEmpty()
        }

        if (!found) {
            fail("Could not find", partialWindowTitles.joinToString(", "))
        }
    }

    /**
     * Asserts that the WindowManager state doesn't contain a [WindowState] with
     * [WindowState.title] containing [partialWindowTitles].
     *
     * @param partialWindowTitles Title of the window to search
     */
    fun notContains(vararg partialWindowTitles: String): WindowManagerStateSubject = apply {
        val found = wmState.windowStates.none { it.name.containsAny(*partialWindowTitles) }
        if (!found) {
            fail("Could find", partialWindowTitles.joinToString(", "))
        }
    }

    /**
     * Asserts that a [WindowState] with [WindowState.title] containing [partialWindowTitles] is visible.
     *
     * @param partialWindowTitles Title of the window to search
     */
    fun isVisible(vararg partialWindowTitles: String): WindowManagerStateSubject = apply {
        wmState.windowStates.checkVisibility(*partialWindowTitles, isVisible = true)
    }

    /**
     * Asserts that a [WindowState] with [WindowState.title] containing [partialWindowTitles] doesn't
     * exist or is invisible.
     *
     * @param partialWindowTitles Title of the window to search
     */
    fun isInvisible(vararg partialWindowTitles: String): WindowManagerStateSubject = apply {
        wmState.windowStates.checkVisibility(*partialWindowTitles, isVisible = false)
    }

    private fun Array<WindowState>.checkIsVisible(vararg partialWindowTitles: String) {
        this@WindowManagerStateSubject.contains(*partialWindowTitles)
        val visibleWindows = this.filter { it.isVisible }
            .filter { it.name.containsAny(*partialWindowTitles) }

        if (visibleWindows.isEmpty()) {
            fail("Is Invisible", partialWindowTitles.joinToString(", "))
        }
    }

    private fun Array<WindowState>.checkIsInvisible(vararg partialWindowTitles: String) {
        try {
            notContains(*partialWindowTitles)
        } catch (e: AssertionError) {
            val invisibleWindows = this.filterNot { it.isVisible }
                .filter { it.name.containsAny(*partialWindowTitles) }
            if (invisibleWindows.isEmpty()) {
                fail("Is Visible", partialWindowTitles.joinToString(", "))
            }
        }
    }

    private fun Array<WindowState>.checkVisibility(
        vararg partialWindowTitles: String,
        isVisible: Boolean
    ) {
        if (isVisible) {
            checkIsVisible(*partialWindowTitles)
        } else {
            checkIsInvisible(*partialWindowTitles)
        }
    }

    /**
     * Asserts that the non-app window ([WindowManagerState.nonAppWindows]) with title
     * containing [partialWindowTitles] exists, is above all app windows ([WindowManagerState.appWindows])
     * and has a visibility equal to [isVisible]
     *
     * This assertion can be used, for example, to assert that the Status and Navigation bars
     * are visible and shown above the app
     *
     * @param partialWindowTitles window title to search
     * @param isVisible if the found window should be visible or not
     */
    @JvmOverloads
    fun isAboveAppWindow(
        vararg partialWindowTitles: String,
        isVisible: Boolean = true
    ): WindowManagerStateSubject = apply {
        wmState.aboveAppWindows.checkVisibility(*partialWindowTitles, isVisible = isVisible)
    }

    /**
     * Asserts that the non-app window ([WindowManagerState.nonAppWindows]) with title
     * containing [partialWindowTitles] exists, is below all app windows ([WindowManagerState.appWindows])
     * and has a visibility equal to [isVisible]
     *
     * This assertion can be used, for example, to assert that the wallpaper is visible and
     * shown below the app
     *
     * @param partialWindowTitles window title to search
     * @param isVisible if the found window should be visible or not
     */
    @JvmOverloads
    fun isBelowAppWindow(
        vararg partialWindowTitles: String,
        isVisible: Boolean = true
    ): WindowManagerStateSubject = apply {
        wmState.belowAppWindows.checkVisibility(*partialWindowTitles, isVisible = isVisible)
    }

    /**
     * Asserts that a window A with title containing [aboveWindowTitle] exists,
     * a window B with title containing [belowWindowTitle] also exists, and that
     * A is shown above B.
     *
     * This assertion can be used, for example, to assert that a PIP window is shown above
     * other apps.
     *
     * @param aboveWindowTitle name of the window that should be above
     * @param belowWindowTitle name of the window that should be below
     */
    fun isAboveWindow(aboveWindowTitle: String, belowWindowTitle: String) {
        // windows are ordered by z-order, from top to bottom
        val aboveZ = wmState.windowStates.indexOfFirst { aboveWindowTitle in it.name }
        val belowZ = wmState.windowStates.indexOfFirst { belowWindowTitle in it.name }

        contains(aboveWindowTitle)
        contains(belowWindowTitle)
        if (aboveZ >= belowZ) {
            fail("$aboveWindowTitle is above $belowWindowTitle")
        }
    }

    /**
     * Asserts that the WindowManager state contains a non-app [WindowState] with
     * [WindowState.title] containing [partialWindowTitles] and that its visibility is
     * equal to [isVisible]
     *
     * @param partialWindowTitles window title to search
     * @param isVisible if the found window should be visible or not
     */
    @JvmOverloads
    fun containsNonAppWindow(
        vararg partialWindowTitles: String,
        isVisible: Boolean = true
    ): WindowManagerStateSubject = apply {
        wmState.nonAppWindows.checkVisibility(*partialWindowTitles, isVisible = isVisible)
    }

    /**
     * Asserts that the title of the top visible app window in the state contains any
     * of [partialWindowTitles]
     *
     * @param partialWindowTitles window title to search
     */
    fun showsAppWindowOnTop(vararg partialWindowTitles: String): WindowManagerStateSubject = apply {
        contains(*partialWindowTitles)
        val windowOnTop = wmState.topVisibleAppWindow.containsAny(*partialWindowTitles)

        if (!windowOnTop) {
            fail(Fact.fact("Not on top", partialWindowTitles.joinToString(", ")),
                Fact.fact("Found", wmState.topVisibleAppWindow))
        }
    }

    /**
     * Asserts that the [WindowState.bounds] of the [WindowState] with [WindowState.title]
     * contained in any of [partialWindowTitles] don't overlap.
     *
     * @param partialWindowTitles Title of the windows that should not overlap
     */
    fun noWindowsOverlap(vararg partialWindowTitles: String): WindowManagerStateSubject = apply {
        partialWindowTitles.forEach { contains(it) }
        val foundWindows = partialWindowTitles.toSet()
            .associateWith { title -> wmState.windowStates.find { it.name.contains(title) } }
            // keep entries only for windows that we actually found by removing nulls
            .filterValues { it != null }
            .mapValues { (_, v) -> v!!.frameRegion }

        val regions = foundWindows.entries.toList()
        for (i in regions.indices) {
            val (ourTitle, ourRegion) = regions[i]
            for (j in i + 1 until regions.size) {
                val (otherTitle, otherRegion) = regions[j]
                if (ourRegion.toAndroidRegion().op(otherRegion.toAndroidRegion(),
                        android.graphics.Region.Op.INTERSECT)) {
                    fail(Fact.fact("Overlap", ourTitle), Fact.fact("Overlap", otherTitle))
                }
            }
        }
    }

    /**
     * Asserts that the WindowManager state contains an app [WindowState] with
     * [WindowState.title] containing [partialWindowTitles] and that its visibility
     * is equal to [isVisible]
     *
     * @param partialWindowTitles window title to search
     * @param isVisible if the found window should be visible or not
     */
    @JvmOverloads
    fun containsAppWindow(
        vararg partialWindowTitles: String,
        isVisible: Boolean = true
    ): WindowManagerStateSubject = apply {
        wmState.appWindows.checkVisibility(*partialWindowTitles, isVisible = isVisible)
    }

    /**
     * Asserts that the display with id [displayId] has rotation [rotation]
     *
     * @param rotation to assert
     * @param displayId of the target display
     */
    @JvmOverloads
    fun hasRotation(
        rotation: Int,
        displayId: Int = Display.DEFAULT_DISPLAY
    ): WindowManagerStateSubject = apply {
        check("Rotation should be $rotation")
            .that(rotation)
            .isEqualTo(wmState.getRotation(displayId))
    }

    /**
     * Asserts that the display with id [displayId] has rotation [rotation]
     *
     * @param rotation to assert
     * @param displayId of the target display
     */
    @JvmOverloads
    fun isNotRotation(
        rotation: Int,
        displayId: Int = Display.DEFAULT_DISPLAY
    ): WindowManagerStateSubject = apply {
        check("Rotation should not be $rotation")
            .that(rotation)
            .isNotEqualTo(wmState.getRotation(displayId))
    }

    /**
     * Asserts that the WindowManager state contains a [WindowState] with [WindowState.title]
     * equal to [ComponentName.toWindowName] and an [Activity] with [Activity.title] equal to
     * [ComponentName.toActivityName]
     *
     * @param activity Component name to search
     */
    fun contains(activity: ComponentName): WindowManagerStateSubject = apply {
        val windowName = activity.toWindowName()
        val activityName = activity.toActivityName()
        check("Activity=$activityName must exist.")
            .that(wmState.containsActivity(activityName)).isTrue()
        check("Window=$windowName must exits.")
            .that(wmState.containsWindow(windowName)).isTrue()
    }

    /**
     * Asserts that the WindowManager state doesn't contain a [WindowState] with [WindowState.title]
     * equal to [ComponentName.toWindowName] nor an [Activity] with [Activity.title] equal to
     * [ComponentName.toActivityName]
     *
     * @param activity Component name to search
     */
    fun notContains(activity: ComponentName): WindowManagerStateSubject = apply {
        val windowName = activity.toWindowName()
        val activityName = activity.toActivityName()
        check("Activity=$activityName must NOT exist.")
            .that(wmState.containsActivity(activityName)).isFalse()
        check("Window=$windowName must NOT exits.")
            .that(wmState.containsWindow(windowName)).isFalse()
    }

    @JvmOverloads
    fun isRecentsActivityVisible(visible: Boolean = true): WindowManagerStateSubject = apply {
        if (wmState.isHomeRecentsComponent) {
            isHomeActivityVisible()
        } else {
            check("Recents activity is ${if (visible) "" else "not"} visible")
                .that(wmState.isRecentsActivityVisible)
                .isEqualTo(visible)
        }
    }

    /**
     * Asserts that the WindowManager state is valid, that is, if it has:
     *   - a resumed activity
     *   - a focused activity
     *   - a focused window
     *   - a front window
     *   - a focused app
     */
    fun isValid(): WindowManagerStateSubject = apply {
        check("Must have stacks").that(wmState.stackCount).isGreaterThan(0)
        // TODO: Update when keyguard will be shown on multiple displays
        if (!wmState.keyguardControllerState.isKeyguardShowing) {
            check("There should be at least one resumed activity in the system.")
                .that(wmState.resumedActivitiesCount).isGreaterThan(0)
        }
        check("Must have focus activity.")
            .that(wmState.focusedActivity).isNotEmpty()
        wmState.rootTasks.forEach { aStack ->
            val stackId = aStack.rootTaskId
            aStack.tasks.forEach { aTask ->
                check("Stack can only contain its own tasks")
                    .that(stackId).isEqualTo(aTask.rootTaskId)
            }
        }
        check("Must have front window.")
            .that(wmState.frontWindow).isNotEmpty()
        check("Must have focused window.")
            .that(wmState.focusedWindow).isNotEmpty()
        check("Must have app.")
            .that(wmState.focusedApp).isNotEmpty()
    }

    /**
     * Asserts that the [WindowManagerState.focusedActivity] and [WindowManagerState.focusedApp]
     * match [activity]
     *
     * @param activity Component name to search
     */
    fun hasFocusedActivity(activity: ComponentName): WindowManagerStateSubject = apply {
        val activityComponentName = activity.toActivityName()
        check("Focused activity invalid")
            .that(activityComponentName)
            .isEqualTo(wmState.focusedActivity)
        check("Focused app invalid")
            .that(activityComponentName)
            .isEqualTo(wmState.focusedApp)
    }

    /**
     * Asserts that the [WindowManagerState.focusedActivity] and [WindowManagerState.focusedApp]
     * don't match [activity]
     *
     * @param activity Component name to search
     */
    fun hasNotFocusedActivity(activity: ComponentName): WindowManagerStateSubject = apply {
        val activityComponentName = activity.toActivityName()
        check("Has focused activity")
            .that(wmState.focusedActivity)
            .isNotEqualTo(activityComponentName)
        check("Has focused app")
            .that(wmState.focusedApp)
            .isNotEqualTo(activityComponentName)
    }

    /**
     * Asserts that the display [displayId] has a [WindowManagerState.focusedApp]
     * matching [activity]
     *
     * @param activity Component name to search
     */
    @JvmOverloads
    fun hasFocusedApp(
        activity: ComponentName,
        displayId: Int = Display.DEFAULT_DISPLAY
    ): WindowManagerStateSubject = apply {
        val activityComponentName = activity.toActivityName()
        check("Focused app invalid")
            .that(activityComponentName)
            .isEqualTo(wmState.getDisplay(displayId)?.focusedApp)
    }

    /**
     * Asserts that WindowManager state has a [WindowManagerState.resumedActivities]
     * matching [activity]
     *
     * @param activity Component name to search
     */
    fun hasResumedActivity(activity: ComponentName): WindowManagerStateSubject = apply {
        val activityComponentName = activity.toActivityName()
        check("Invalid resumed activity")
            .that(wmState.resumedActivities)
            .asList()
            .contains(activityComponentName)
    }

    /**
     * Asserts that WindowManager state [WindowManagerState.resumedActivities] doesn't
     * match [activity]
     *
     * @param activity Component name to search
     */
    fun hasNotResumedActivity(activity: ComponentName): WindowManagerStateSubject = apply {
        val activityComponentName = activity.toActivityName()
        check("Has resumed activity")
            .that(wmState.resumedActivities)
            .asList()
            .doesNotContain(activityComponentName)
    }

    /**
     * Asserts that title of the [WindowManagerState.focusedWindow] on the state matches
     * [windowTitle]
     *
     * @param windowTitle window title to search
     */
    fun isFocused(windowTitle: String): WindowManagerStateSubject = apply {
        check("Invalid focused window")
            .that(windowTitle)
            .isEqualTo(wmState.focusedWindow)
    }

    /**
     * Asserts that [WindowManagerState.focusedWindow] on the WindowManager state doesn't
     * match [windowTitle]
     *
     * @param windowTitle window title to search
     */
    fun isWindowNotFocused(windowTitle: String): WindowManagerStateSubject = apply {
        check("Has focused window")
            .that(wmState.focusedWindow)
            .isNotEqualTo(windowTitle)
    }

    /**
     * Asserts that the WindowManager state contains a [WindowState] with [WindowState.title]
     * equal to [ComponentName.toWindowName] and an [Activity] with [Activity.title] equal to
     * [ComponentName.toActivityName] and both are visible
     *
     * @param activity Component name to search
     */
    fun isVisible(activity: ComponentName): WindowManagerStateSubject =
        hasActivityAndWindowVisibility(activity, visible = true)

    /**
     * Asserts that the WindowManager state contains a [WindowState] with [WindowState.title]
     * equal to [ComponentName.toWindowName] and an [Activity] with [Activity.title] equal to
     * [ComponentName.toActivityName] and both are invisible
     *
     * @param activity Component name to search
     */
    fun isInvisible(activity: ComponentName): WindowManagerStateSubject =
        hasActivityAndWindowVisibility(activity, visible = false)

    private fun hasActivityAndWindowVisibility(
        activity: ComponentName,
        visible: Boolean
    ): WindowManagerStateSubject = apply {
        // Check existence of activity and window.
        val windowName = activity.toWindowName()
        val activityName = activity.toActivityName()
        check("Activity=$activityName must exist.")
            .that(wmState.containsActivity(activityName)).isTrue()
        check("Window=$windowName must exist.")
            .that(wmState.containsWindow(windowName)).isTrue()

        // Check visibility of activity and window.
        check("Activity=$activityName must ${if (visible) "" else " NOT"} be visible.")
            .that(visible).isEqualTo(wmState.isActivityVisible(activityName))
        check("Window=$windowName must ${if (visible) "" else " NOT"} have shown surface.")
            .that(visible).isEqualTo(wmState.isWindowSurfaceShown(windowName))
    }

    /**
     * Asserts that the WindowManager state home activity visibility is equal to [isVisible]
     *
     * @param isVisible if the home activity should be visible of not
     */
    @JvmOverloads
    fun isHomeActivityVisible(isVisible: Boolean = true): WindowManagerStateSubject = apply {
        if (isVisible) {
            check("Home activity doesn't exist")
                .that(wmState.homeActivity)
                .isNotNull()

            check("Home activity is not visible")
                .that(wmState.homeActivity?.isVisible)
                .isTrue()
        } else {
            check("Home activity is visible")
                .that(wmState.homeActivity?.isVisible ?: false)
                .isFalse()
        }
    }

    /**
     * Asserts that the IME surface is visible in the display [displayId]
     */
    @JvmOverloads
    fun isImeWindowVisible(
        displayId: Int = Display.DEFAULT_DISPLAY
    ): WindowManagerStateSubject = apply {
        val imeWinState = wmState.inputMethodWindowState
        check("IME window must exist")
            .that(imeWinState).isNotNull()
        check("IME window must be shown")
            .that(imeWinState?.isSurfaceShown ?: false).isTrue()
        check("IME window must be on the given display")
            .that(displayId).isEqualTo(imeWinState?.displayId ?: -1)
    }

    /**
     * Asserts that the IME surface is invisible in the display [displayId]
     */
    @JvmOverloads
    fun isImeWindowInvisible(
        displayId: Int = Display.DEFAULT_DISPLAY
    ): WindowManagerStateSubject = apply {
        val imeWinState = wmState.inputMethodWindowState
        check("IME window must not be shown")
            .that(imeWinState?.isSurfaceShown ?: false).isFalse()
        if (imeWinState?.isSurfaceShown == true) {
            check("IME window must not be on the given display")
                .that(displayId).isNotEqualTo(imeWinState.displayId)
        }
    }

    /**
     * Asserts that an activity [activity] exists and is in PIP mode
     */
    fun isInPipMode(
        activity: ComponentName
    ): WindowManagerStateSubject = apply {
        val windowName = activity.toWindowName()
        contains(windowName)
        val pinnedWindows = wmState.pinnedWindows
            .map { it.title }
        check("Window not in PIP mode")
            .that(pinnedWindows)
            .contains(windowName)
    }

    /**
     * Obtains a [WindowStateSubject] for the first occurrence of a [WindowState] with
     * [WindowState.title] containing [name].
     *
     * Always returns a subject, event when the layer doesn't exist. To verify if layer
     * actually exists in the hierarchy use [WindowStateSubject.exists] or
     * [WindowStateSubject.doesNotExist]
     *
     * @return WindowStateSubject that can be used to make assertions on a single [WindowState]
     * matching [name].
     */
    fun windowState(name: String): WindowStateSubject {
        return subjects.firstOrNull {
            it.windowState?.name?.contains(name) == true
        } ?: WindowStateSubject.assertThat(name, this)
    }

    override fun toString(): String {
        return "WindowManagerStateSubject($wmState)"
    }

    companion object {
        /**
         * Boiler-plate Subject.Factory for WindowManagerStateSubject
         *
         * @param trace containing the entry
         */
        private fun getFactory(
            trace: WindowManagerTraceSubject? = null
        ): Factory<Subject, WindowManagerState> =
            Factory { fm, subject -> WindowManagerStateSubject(fm, subject, trace) }

        /**
         * User-defined entry point
         *
         * @param entry to assert
         * @param trace containing the entry
         */
        @JvmStatic
        @JvmOverloads
        fun assertThat(
            entry: WindowManagerState,
            trace: WindowManagerTraceSubject? = null
        ): WindowManagerStateSubject {
            val strategy = FlickerFailureStrategy()
            val subject = StandardSubjectBuilder.forCustomFailureStrategy(strategy)
                .about(getFactory(trace))
                .that(entry) as WindowManagerStateSubject
            strategy.init(subject)
            return subject
        }
    }
}