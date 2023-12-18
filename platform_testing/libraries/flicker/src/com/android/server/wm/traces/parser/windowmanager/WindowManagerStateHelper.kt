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

package com.android.server.wm.traces.parser.windowmanager

import android.app.ActivityTaskManager
import android.app.Instrumentation
import android.app.WindowConfiguration
import android.content.ComponentName
import android.graphics.Rect
import android.graphics.Region
import android.os.SystemClock
import android.util.Log
import android.view.Display
import androidx.annotation.VisibleForTesting
import androidx.test.platform.app.InstrumentationRegistry
import com.android.server.wm.traces.common.layers.LayerTraceEntry
import com.android.server.wm.traces.common.windowmanager.WindowManagerState
import com.android.server.wm.traces.parser.getCurrentStateDump
import com.android.server.wm.traces.common.windowmanager.windows.ConfigurationContainer
import com.android.server.wm.traces.common.windowmanager.windows.WindowContainer
import com.android.server.wm.traces.common.windowmanager.windows.WindowState
import com.android.server.wm.traces.parser.Condition
import com.android.server.wm.traces.parser.LOG_TAG
import com.android.server.wm.traces.parser.toActivityName
import com.android.server.wm.traces.parser.toAndroidRegion
import com.android.server.wm.traces.parser.toWindowName

open class WindowManagerStateHelper @JvmOverloads constructor(
    /**
     * Instrumentation to run the tests
     */
    private val instrumentation: Instrumentation = InstrumentationRegistry.getInstrumentation(),
    /**
     * Predicate to supply a new UI information
     */
    private val deviceDumpSupplier: () -> Dump = {
        val currState = getCurrentStateDump(
            instrumentation.uiAutomation)
        Dump(
            currState.wmTrace?.entries?.first() ?: error("Unable to parse WM trace"),
            currState.layersTrace?.entries?.first() ?: error("Unable to parse Layers trace")
        )
    },
    /**
     * Number of attempts to satisfy a wait condition
     */
    private val numRetries: Int = 5,
    /**
     * Interval between wait for state dumps during wait conditions
     */
    private val retryIntervalMs: Long = 500L
) {
    /**
     * Fetches the current device state
     */
    val currentState: Dump
        get() = computeState(ignoreInvalidStates = true)

    /**
     * Queries the supplier for a new device state
     *
     * @param ignoreInvalidStates If false, retries up to [numRetries] times (with a sleep
     * interval of [retryIntervalMs] ms to obtain a complete WM state, otherwise returns the
     * first state
     */
    protected open fun computeState(ignoreInvalidStates: Boolean = false): Dump {
        var newState = deviceDumpSupplier.invoke()
        for (retryNr in 0..numRetries) {
            val wmState = newState.wmState
            if (!ignoreInvalidStates && wmState.isIncomplete()) {
                Log.w(LOG_TAG, "***Incomplete AM state: ${wmState.getIsIncompleteReason()}" +
                    " Waiting ${retryIntervalMs}ms and retrying ($retryNr/$numRetries)...")
                SystemClock.sleep(retryIntervalMs)
                newState = deviceDumpSupplier.invoke()
            } else {
                break
            }
        }

        return newState
    }

    private fun ConfigurationContainer.isWindowingModeCompatible(
        requestedWindowingMode: Int
    ): Boolean {
        return when (requestedWindowingMode) {
            WindowConfiguration.WINDOWING_MODE_UNDEFINED -> true
            WindowConfiguration.WINDOWING_MODE_SPLIT_SCREEN_SECONDARY ->
                (windowingMode == WindowConfiguration.WINDOWING_MODE_FULLSCREEN ||
                    windowingMode == WindowConfiguration.WINDOWING_MODE_SPLIT_SCREEN_SECONDARY)
            else -> windowingMode == requestedWindowingMode
        }
    }

    /**
     * Wait for the activities to appear in proper stacks and for valid state in AM and WM.
     * @param waitForActivitiesVisible array of activity states to wait for.
     */
    fun waitForValidState(vararg waitForActivitiesVisible: WaitForValidActivityState): Boolean {
        val success = Condition.waitFor<WindowManagerState>("valid stacks and activities states",
            retryLimit = numRetries, retryIntervalMs = retryIntervalMs) {
            // TODO: Get state of AM and WM at the same time to avoid mismatches caused by
            // requesting dump in some intermediate state.
            val state = computeState()
            !(shouldWaitForValidityCheck(state) ||
                shouldWaitForValidStacks(state) ||
                shouldWaitForActivities(state, *waitForActivitiesVisible) ||
                shouldWaitForWindows(state))
        }
        if (!success) {
            Log.e(LOG_TAG, "***Waiting for states failed: " +
                waitForActivitiesVisible.contentToString())
        }
        return success
    }

    fun waitForFullScreenApp(componentName: ComponentName): Boolean =
            waitForValidState(
                    WaitForValidActivityState
                            .Builder(componentName)
                            .setWindowingMode(WindowConfiguration.WINDOWING_MODE_FULLSCREEN)
                            .setActivityType(WindowConfiguration.ACTIVITY_TYPE_STANDARD)
                            .build())

    fun waitForHomeActivityVisible(): Boolean {
        return waitFor { it.wmState.homeActivity?.isVisible == true } &&
            waitForNavBarStatusBarVisible() &&
            waitForAppTransitionIdle()
    }

    fun waitForRecentsActivityVisible(): Boolean =
        waitFor("recents activity to be visible") {
            it.wmState.isRecentsActivityVisible
        }

    fun waitForAodShowing(): Boolean =
        waitFor("AOD showing") {
            it.wmState.keyguardControllerState.isAodShowing
        }

    fun waitForKeyguardGone(): Boolean =
        waitFor("Keyguard gone") {
            !it.wmState.keyguardControllerState.isKeyguardShowing
        }

    /**
     * Wait for specific rotation for the default display. Values are Surface#Rotation
     */
    @JvmOverloads
    fun waitForRotation(rotation: Int, displayId: Int = Display.DEFAULT_DISPLAY): Boolean =
        waitFor("Rotation: $rotation") {
            val currRotation = it.wmState.getRotation(displayId)
            val rotationLayerExists = it.layerState.isVisible(ROTATION_LAYER_NAME)
            val blackSurfaceLayerExists = it.layerState.isVisible(BLACK_SURFACE_LAYER_NAME)
            val anyLayerAnimating = it.layerState.visibleLayers.any { layer ->
                !layer.transform.isSimpleRotation
            }
            Log.v(LOG_TAG, "currRotation($currRotation) " +
                "anyLayerAnimating($anyLayerAnimating) " +
                "blackSurfaceLayerExists($blackSurfaceLayerExists) " +
                "rotationLayerExists($rotationLayerExists)")
            currRotation == rotation &&
                !anyLayerAnimating &&
                !rotationLayerExists &&
                !blackSurfaceLayerExists
        }

    /**
     * Wait for specific orientation for the default display.
     * Values are ActivityInfo.ScreenOrientation
     */
    @JvmOverloads
    fun waitForLastOrientation(
        orientation: Int,
        displayId: Int = Display.DEFAULT_DISPLAY
    ): Boolean =
        waitFor("LastOrientation: $orientation") {
            val result = it.wmState.getOrientation(displayId)
            Log.v(LOG_TAG, "Current: $result Expected: $orientation")
            result == orientation
        }

    fun waitForActivityState(activity: ComponentName, activityState: String): Boolean {
        val activityName = activity.toActivityName()
        return waitFor("state of $activityName to be $activityState") {
            it.wmState.hasActivityState(activityName, activityState)
        }
    }

    /**
     * Waits until the navigation and status bars are visible (windows and layers)
     */
    fun waitForNavBarStatusBarVisible(): Boolean =
        waitFor("Navigation and Status bar to be visible") {
            val navBarWindowVisible = it.wmState.isWindowVisible(NAV_BAR_WINDOW_NAME)
            val statusBarWindowVisible = it.wmState.isWindowVisible(STATUS_BAR_WINDOW_NAME)
            val navBarLayerVisible = it.layerState.isVisible(NAV_BAR_LAYER_NAME)
            val navBarLayerAlpha = it.layerState.getLayerWithBuffer(NAV_BAR_LAYER_NAME)
                ?.color?.a ?: 0f
            val statusBarLayerVisible = it.layerState.isVisible(STATUS_BAR_LAYER_NAME)
            val statusBarLayerAlpha = it.layerState.getLayerWithBuffer(STATUS_BAR_LAYER_NAME)
                ?.color?.a ?: 0f
            val result = navBarWindowVisible &&
                navBarLayerVisible &&
                statusBarWindowVisible &&
                statusBarLayerVisible &&
                navBarLayerAlpha == 1f &&
                statusBarLayerAlpha == 1f

            Log.v(LOG_TAG, "Current $result " +
                "navBarWindowVisible($navBarWindowVisible) " +
                "navBarLayerVisible($navBarLayerVisible) " +
                "statusBarWindowVisible($statusBarWindowVisible) " +
                "statusBarLayerVisible($statusBarLayerVisible) " +
                "navBarLayerAlpha($navBarLayerAlpha) " +
                "statusBarLayerAlpha($statusBarLayerAlpha)")

            result
        }

    fun waitForVisibleWindow(activity: ComponentName): Boolean {
        val activityName = activity.toActivityName()
        val windowName = activity.toWindowName()
        return waitFor("$activityName to exist") {
            val containsActivity = it.wmState.containsActivity(activityName)
            val containsWindow = it.wmState.containsWindow(windowName)
            val activityVisible = containsActivity && it.wmState.isActivityVisible(activityName)
            val windowVisible = containsWindow && it.wmState.isWindowSurfaceShown(windowName)
            val result = containsActivity &&
                containsWindow &&
                activityVisible &&
                windowVisible

            Log.v(LOG_TAG, "Current: $result " +
                "containsActivity($containsActivity) " +
                "containsWindow($containsWindow) " +
                "activityVisible($activityVisible) " +
                "windowVisible($windowVisible)")

            result
        }
    }

    fun waitForActivityRemoved(activity: ComponentName): Boolean {
        val activityName = activity.toActivityName()
        val windowName = activity.toWindowName()
        return waitFor("$activityName to be removed") {
            val containsActivity = it.wmState.containsActivity(activityName)
            val containsWindow = it.wmState.containsWindow(windowName)
            val result = !containsActivity && !containsWindow

            Log.v(LOG_TAG, "Current: $result" +
                "containsActivity($containsActivity)" +
                "containsWindow($containsWindow)")
            result
        }
    }

    fun waitForPendingActivityContain(activity: ComponentName): Boolean {
        val activityName: String = activity.toActivityName()
        return waitFor("$activityName in pending list") {
            it.wmState.pendingActivityContain(activityName)
        }
    }

    @JvmOverloads
    fun waitForAppTransitionIdle(displayId: Int = Display.DEFAULT_DISPLAY): Boolean =
        waitFor("app transition idle on Display $displayId") {
            val result =
                it.wmState.getDisplay(displayId)?.appTransitionState
            Log.v(LOG_TAG, "Current: $result")
            WindowManagerState.APP_STATE_IDLE == result
        }

    fun waitForWindowSurfaceDisappeared(componentName: ComponentName): Boolean {
        val windowName = componentName.toWindowName()
        return waitFor("$windowName's surface is disappeared") {
            !it.wmState.isWindowSurfaceShown(windowName)
        }
    }

    fun waitForSurfaceAppeared(surfaceName: String): Boolean {
        return waitFor("$surfaceName surface is appeared") {
            it.wmState.isWindowSurfaceShown(surfaceName)
        }
    }

    fun waitWindowingModeTopFocus(
        windowingMode: Int,
        topFocus: Boolean,
        message: String
    ): Boolean = waitFor(message) {
        val stack = it.wmState.getStandardStackByWindowingMode(windowingMode)
        (stack != null && topFocus == (it.wmState.focusedStackId == stack.rootTaskId))
    }

    @JvmOverloads
    fun waitFor(
        message: String = "",
        waitCondition: (Dump) -> Boolean
    ): Boolean = Condition.waitFor<Dump>(message, retryLimit = numRetries,
        retryIntervalMs = retryIntervalMs) {
        val state = computeState()
        waitCondition.invoke(state)
    }

    /**
     * @return true if should wait for valid stacks state.
     */
    private fun shouldWaitForValidStacks(state: Dump): Boolean {
        if (state.wmState.stackCount == 0) {
            Log.i(LOG_TAG, "***stackCount=0")
            return true
        }
        if (!state.wmState.keyguardControllerState.isKeyguardShowing &&
            state.wmState.resumedActivities.isEmpty()) {
            if (!state.wmState.keyguardControllerState.isKeyguardShowing) {
                Log.i(LOG_TAG, "***resumedActivitiesCount=0")
            } else {
                Log.i(LOG_TAG, "***isKeyguardShowing=true")
            }
            return true
        }
        if (state.wmState.focusedActivity.isEmpty()) {
            Log.i(LOG_TAG, "***focusedActivity=null")
            return true
        }
        return false
    }

    /**
     * @return true if should wait for some activities to become visible.
     */
    private fun shouldWaitForActivities(
        state: Dump,
        vararg waitForActivitiesVisible: WaitForValidActivityState
    ): Boolean {
        if (waitForActivitiesVisible.isEmpty()) {
            return false
        }
        // If the caller is interested in waiting for some particular activity windows to be
        // visible before compute the state. Check for the visibility of those activity windows
        // and for placing them in correct stacks (if requested).
        var allActivityWindowsVisible = true
        var tasksInCorrectStacks = true
        for (activityState in waitForActivitiesVisible) {
            val matchingWindowStates = state.wmState.getMatchingVisibleWindowState(
                activityState.windowName ?: "")
            val activityWindowVisible = matchingWindowStates.isNotEmpty()

            if (!activityWindowVisible) {
                Log.i(LOG_TAG, "Activity window not visible: ${activityState.windowName}")
                allActivityWindowsVisible = false
            } else if (activityState.activityName != null &&
                !state.wmState.isActivityVisible(activityState.activityName.toActivityName())) {
                Log.i(LOG_TAG, "Activity not visible: ${activityState.activityName}")
                allActivityWindowsVisible = false
            } else {
                // Check if window is already the correct state requested by test.
                var windowInCorrectState = false
                for (ws in matchingWindowStates) {
                    if (activityState.stackId != ActivityTaskManager.INVALID_STACK_ID &&
                        ws.stackId != activityState.stackId) {
                        continue
                    }
                    if (!ws.isWindowingModeCompatible(activityState.windowingMode)) {
                        continue
                    }
                    if (activityState.activityType != WindowConfiguration.ACTIVITY_TYPE_UNDEFINED &&
                        ws.activityType != activityState.activityType) {
                        continue
                    }
                    windowInCorrectState = true
                    break
                }
                if (!windowInCorrectState) {
                    Log.i(LOG_TAG, "Window in incorrect stack: $activityState")
                    tasksInCorrectStacks = false
                }
            }
        }
        return !allActivityWindowsVisible || !tasksInCorrectStacks
    }

    /**
     * @return true if should wait for the valid windows state.
     */
    private fun shouldWaitForWindows(state: Dump): Boolean {
        return when {
            state.wmState.frontWindow == null -> {
                Log.i(LOG_TAG, "***frontWindow=null")
                true
            }
            state.wmState.focusedWindow.isEmpty() -> {
                Log.i(LOG_TAG, "***focusedWindow=null")
                true
            }
            state.wmState.focusedApp.isEmpty() -> {
                Log.i(LOG_TAG, "***focusedApp=null")
                true
            }
            else -> false
        }
    }

    private fun shouldWaitForValidityCheck(state: Dump): Boolean {
        return !state.wmState.isComplete()
    }

    /**
     * Waits until the IME window and layer are visible
     */
    @JvmOverloads
    fun waitImeWindowShown(displayId: Int = Display.DEFAULT_DISPLAY): Boolean =
        waitFor("IME window shown") {
            val imeSurfaceShown = it.wmState.inputMethodWindowState?.isSurfaceShown == true
            val imeDisplay = it.wmState.inputMethodWindowState?.displayId
            val imeLayerShown = it.layerState.isVisible(IME_LAYER_NAME)
            val result = imeSurfaceShown && imeLayerShown && imeDisplay == displayId

            Log.v(LOG_TAG, "Current: $result " +
                "imeSurfaceShown($imeSurfaceShown) " +
                "imeLayerShown($imeLayerShown) " +
                "imeDisplay($imeDisplay)")
            result
        }

    /**
     * Waits until the IME layer is no longer visible. Cannot wait for the window as
     * its visibility information is updated at a later state and is not reliable in
     * the trace
     */
    fun waitImeWindowGone(): Boolean =
        waitFor("IME window gone") {
            val imeLayerShown = it.layerState.isVisible(IME_LAYER_NAME)
            Log.v(LOG_TAG, "imeLayerShown($imeLayerShown)")
            !imeLayerShown
        }

    /**
     * Obtains a [WindowContainer] from the current device state, or null if the WindowContainer
     * doesn't exist
     */
    fun getWindow(activity: ComponentName): WindowState? {
        val windowName = activity.toWindowName()
        return this.currentState.wmState.windowStates
            .firstOrNull { it.title == windowName }
    }

    /**
     * Obtains the region of a window in the state, or an empty [Rect] is there are none
     */
    fun getWindowRegion(activity: ComponentName): Region {
        val window = getWindow(activity)
        return window?.frameRegion?.toAndroidRegion() ?: Region()
    }

    companion object {
        @VisibleForTesting
        const val NAV_BAR_WINDOW_NAME = "NavigationBar0"
        @VisibleForTesting
        const val STATUS_BAR_WINDOW_NAME = "StatusBar"
        @VisibleForTesting
        const val NAV_BAR_LAYER_NAME = "$NAV_BAR_WINDOW_NAME#0"
        @VisibleForTesting
        const val STATUS_BAR_LAYER_NAME = "$STATUS_BAR_WINDOW_NAME#0"
        @VisibleForTesting
        const val ROTATION_LAYER_NAME = "RotationLayer#0"
        @VisibleForTesting
        const val BLACK_SURFACE_LAYER_NAME = "BackColorSurface#0"
        @VisibleForTesting
        const val IME_LAYER_NAME = "InputMethod#0"
        @VisibleForTesting
        const val SPLASH_SCREEN_NAME = "Splash Screen"
        @VisibleForTesting
        const val SNAPSHOT_WINDOW_NAME = "SnapshotStartingWindow"
    }

    data class Dump(
        /**
         * Window manager state
         */
        @JvmField val wmState: WindowManagerState,
        /**
         * Layers state
         */
        @JvmField val layerState: LayerTraceEntry
    )
}