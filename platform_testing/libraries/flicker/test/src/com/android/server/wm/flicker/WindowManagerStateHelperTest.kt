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

package com.android.server.wm.flicker

import android.content.ComponentName
import android.view.Display
import android.view.Surface
import androidx.test.filters.FlakyTest
import androidx.test.platform.app.InstrumentationRegistry
import com.android.server.wm.flicker.traces.windowmanager.WindowManagerStateSubject
import com.android.server.wm.traces.common.Buffer
import com.android.server.wm.traces.common.Color
import com.android.server.wm.traces.common.Rect
import com.android.server.wm.traces.common.RectF
import com.android.server.wm.traces.common.Region
import com.android.server.wm.traces.common.layers.Layer
import com.android.server.wm.traces.common.layers.LayerTraceEntryBuilder
import com.android.server.wm.traces.common.layers.Transform
import com.android.server.wm.traces.common.windowmanager.WindowManagerTrace
import com.android.server.wm.traces.parser.windowmanager.WindowManagerStateHelper
import com.google.common.truth.Truth
import org.junit.FixMethodOrder
import org.junit.Test
import org.junit.runners.MethodSorters

/**
 * Contains [WindowManagerStateHelper] tests. To run this test: `atest
 * FlickerLibTest:WindowManagerTraceHelperTest`
 */
@FixMethodOrder(MethodSorters.NAME_ASCENDING)
class WindowManagerStateHelperTest {
    class TestWindowManagerStateHelper(
        /**
         * Predicate to supply a new UI information
         */
        deviceDumpSupplier: () -> Dump,
        numRetries: Int = 5,
        retryIntervalMs: Long = 500L
    ) : WindowManagerStateHelper(InstrumentationRegistry.getInstrumentation(),
        deviceDumpSupplier, numRetries, retryIntervalMs) {
        var wmState = computeState(ignoreInvalidStates = true).wmState
        override fun computeState(ignoreInvalidStates: Boolean): Dump {
            val state = super.computeState(ignoreInvalidStates)
            wmState = state.wmState
            return state
        }
    }

    private fun String.toComponentName() =
        ComponentName.unflattenFromString(this) ?: error("Unable to extract component name")

    private val chromeComponentName = ("com.android.chrome/org.chromium.chrome.browser" +
        ".firstrun.FirstRunActivity").toComponentName()
    private val simpleAppComponentName = "com.android.server.wm.flicker.testapp/.SimpleActivity"
        .toComponentName()

    private fun createImaginaryLayer(name: String, index: Int, id: Int, parentId: Int): Layer {
        val transform = Transform(0, Transform.Matrix(0f, 0f, 0f, 0f, 0f, 0f))
        val rect = RectF(
            left = index.toFloat(),
            top = index.toFloat(),
            right = index.toFloat() + 1,
            bottom = index.toFloat() + 1
        )
        return Layer(
            name,
            id,
            parentId,
            z = 0,
            visibleRegion = Region(rect.toRect()),
            activeBuffer = Buffer(1, 1, 1, 1),
            flags = 0,
            _bounds = rect,
            color = Color(0f, 0f, 0f, 1f),
            _isOpaque = true,
            shadowRadius = 0f,
            cornerRadius = 0f,
            type = "",
            _screenBounds = rect,
            transform = transform,
            _sourceBounds = rect,
            currFrame = 0,
            effectiveScalingMode = 0,
            bufferTransform = transform,
            hwcCompositionType = 0,
            hwcCrop = RectF.EMPTY,
            hwcFrame = Rect.EMPTY,
            crop = rect.toRect(),
            backgroundBlurRadius = 0,
            isRelativeOf = false,
            zOrderRelativeOfId = -1
        )
    }

    private fun createImaginaryVisibleLayers(names: List<String>): List<Layer> {
        val root = createImaginaryLayer("root", -1, id = "root".hashCode(), parentId = -1)
        val layers = mutableListOf(root)
        names.forEachIndexed { index, name ->
            layers.add(
                createImaginaryLayer(name, index, id = name.hashCode(), parentId = root.id)
            )
        }
        return layers
    }

    private fun WindowManagerTrace.asSupplier(
        startingTimestamp: Long = 0
    ): () -> WindowManagerStateHelper.Dump {
        val iterator = this.dropWhile { it.timestamp < startingTimestamp }.iterator()
        return {
            if (iterator.hasNext()) {
                val wmState = iterator.next()
                val layerList = mutableListOf(WindowManagerStateHelper.STATUS_BAR_LAYER_NAME,
                    WindowManagerStateHelper.NAV_BAR_LAYER_NAME)

                if (wmState.inputMethodWindowState?.isSurfaceShown == true) {
                    layerList.add(WindowManagerStateHelper.IME_LAYER_NAME)
                }
                val layerTraceEntry = LayerTraceEntryBuilder(timestamp = 0,
                    layers = createImaginaryVisibleLayers(layerList)).build()
                WindowManagerStateHelper.Dump(
                    wmState,
                    layerTraceEntry
                )
            } else {
                error("Reached the end of the trace")
            }
        }
    }

    @Test
    fun canWaitForIme() {
        val trace = readWmTraceFromFile("wm_trace_ime.pb")
        val supplier = trace.asSupplier()
        val helper = TestWindowManagerStateHelper(supplier, numRetries = trace.entries.size,
            retryIntervalMs = 1)
        try {
            WindowManagerStateSubject
                .assertThat(helper.wmState)
                .isImeWindowVisible(Display.DEFAULT_DISPLAY)
            error("IME state should not be available")
        } catch (e: AssertionError) {
            helper.waitImeWindowShown(Display.DEFAULT_DISPLAY)
            WindowManagerStateSubject
                .assertThat(helper.wmState)
                .isImeWindowVisible(Display.DEFAULT_DISPLAY)
        }
    }

    @Test
    fun canFailImeNotShown() {
        val supplier = readWmTraceFromFile("wm_trace_ime.pb").asSupplier()
        val helper = TestWindowManagerStateHelper(supplier, retryIntervalMs = 1)
        try {
            WindowManagerStateSubject
                .assertThat(helper.wmState)
                .isImeWindowVisible()
            error("IME state should not be available")
        } catch (e: AssertionError) {
            helper.waitImeWindowShown()
            WindowManagerStateSubject
                .assertThat(helper.wmState)
                .isImeWindowInvisible()
        }
    }

    @Test
    fun canWaitForWindow() {
        val trace = readWmTraceFromFile("wm_trace_open_app_cold.pb")
        val supplier = trace.asSupplier()
        val helper = TestWindowManagerStateHelper(supplier, numRetries = trace.entries.size,
            retryIntervalMs = 1)
        try {
            WindowManagerStateSubject
                .assertThat(helper.wmState)
                .contains(simpleAppComponentName)
            error("Chrome window should not exist in the start of the trace")
        } catch (e: AssertionError) {
            helper.waitForVisibleWindow(simpleAppComponentName)
            WindowManagerStateSubject
                .assertThat(helper.wmState)
                .isVisible(simpleAppComponentName)
        }
    }

    @Test
    fun canFailWindowNotShown() {
        val trace = readWmTraceFromFile("wm_trace_open_app_cold.pb")
        val supplier = trace.asSupplier()
        val helper = TestWindowManagerStateHelper(supplier, numRetries = 3, retryIntervalMs = 1)
        try {
            WindowManagerStateSubject
                .assertThat(helper.wmState)
                .contains(simpleAppComponentName)
            error("SimpleActivity window should not exist in the start of the trace")
        } catch (e: AssertionError) {
            helper.waitForVisibleWindow(simpleAppComponentName)
            WindowManagerStateSubject
                .assertThat(helper.wmState)
                .notContains(simpleAppComponentName)
        }
    }

    @Test
    fun canDetectHomeActivityVisibility() {
        val trace = readWmTraceFromFile("wm_trace_open_and_close_chrome.pb")
        val supplier = trace.asSupplier()
        val helper = TestWindowManagerStateHelper(supplier, numRetries = trace.entries.size,
            retryIntervalMs = 1)
        WindowManagerStateSubject
            .assertThat(helper.wmState)
            .isHomeActivityVisible()
        helper.waitForVisibleWindow(chromeComponentName)
        WindowManagerStateSubject
            .assertThat(helper.wmState)
            .isHomeActivityVisible(false)
        helper.waitForHomeActivityVisible()
        WindowManagerStateSubject
            .assertThat(helper.wmState)
            .isHomeActivityVisible()
    }

    @Test
    fun canWaitActivityRemoved() {
        val trace = readWmTraceFromFile("wm_trace_open_and_close_chrome.pb")
        val supplier = trace.asSupplier()
        val helper = TestWindowManagerStateHelper(supplier, numRetries = trace.entries.size,
            retryIntervalMs = 1)
        WindowManagerStateSubject
            .assertThat(helper.wmState)
            .isHomeActivityVisible()
            .notContains(chromeComponentName)
        helper.waitForVisibleWindow(chromeComponentName)
        WindowManagerStateSubject
            .assertThat(helper.wmState)
            .isVisible(chromeComponentName)
        helper.waitForActivityRemoved(chromeComponentName)
        WindowManagerStateSubject
            .assertThat(helper.wmState)
            .notContains(chromeComponentName)
            .isHomeActivityVisible()
    }

    @Test
    fun canWaitAppStateIdle() {
        val trace = readWmTraceFromFile("wm_trace_open_and_close_chrome.pb")
        val supplier = trace.asSupplier(startingTimestamp = 69443911868523)
        val helper = TestWindowManagerStateHelper(supplier, numRetries = trace.entries.size,
            retryIntervalMs = 1)
        try {
            WindowManagerStateSubject
                .assertThat(helper.wmState)
                .isValid()
            error("Initial state in the trace should not be valid")
        } catch (e: AssertionError) {
            Truth.assertWithMessage("App transition never became idle")
                .that(helper.waitForAppTransitionIdle())
                .isTrue()
            WindowManagerStateSubject
                .assertThat(helper.wmState)
                .isValid()
        }
    }

    @Test
    fun canWaitForRotation() {
        val trace = readWmTraceFromFile("wm_trace_rotation.pb")
        val supplier = trace.asSupplier()
        val helper = TestWindowManagerStateHelper(supplier, numRetries = trace.entries.size,
            retryIntervalMs = 1)
        WindowManagerStateSubject
            .assertThat(helper.wmState)
            .hasRotation(Surface.ROTATION_0)
        helper.waitForRotation(Surface.ROTATION_270)
        WindowManagerStateSubject
            .assertThat(helper.wmState)
            .hasRotation(Surface.ROTATION_270)
        helper.waitForRotation(Surface.ROTATION_0)
        WindowManagerStateSubject
            .assertThat(helper.wmState)
            .hasRotation(Surface.ROTATION_0)
    }

    @Test
    fun canFailRotationNotReached() {
        val trace = readWmTraceFromFile("wm_trace_rotation.pb")
        val supplier = trace.asSupplier()
        val helper = TestWindowManagerStateHelper(supplier, numRetries = trace.entries.size,
            retryIntervalMs = 1)
        WindowManagerStateSubject
            .assertThat(helper.wmState)
            .hasRotation(Surface.ROTATION_0)
        try {
            helper.waitForRotation(Surface.ROTATION_90)
            error("Should not have reached orientation ${Surface.ROTATION_90}")
        } catch (e: IllegalStateException) {
            WindowManagerStateSubject
                .assertThat(helper.wmState)
                .isNotRotation(Surface.ROTATION_90)
                .hasRotation(Surface.ROTATION_0)
        }
    }

    @Test
    fun canDetectResumedActivitiesInStacks() {
        val trace = readWmTraceFromDumpFile("wm_trace_resumed_activities_in_stack.pb")
        val entry = trace.first()
        Truth.assertWithMessage("Trace should have a resumed activity in stacks")
            .that(entry.resumedActivities)
            .asList()
            .hasSize(1)
    }

    @FlakyTest
    @Test
    fun canWaitForRecents() {
        val trace = readWmTraceFromFile("wm_trace_open_recents.pb")
        val supplier = trace.asSupplier()
        val helper = TestWindowManagerStateHelper(supplier, numRetries = trace.entries.size,
            retryIntervalMs = 1)
        WindowManagerStateSubject
            .assertThat(helper.wmState)
            .isRecentsActivityVisible(visible = false)
        helper.waitForRecentsActivityVisible()
        WindowManagerStateSubject
            .assertThat(helper.wmState)
            .isRecentsActivityVisible()
    }
}