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

package com.android.server.wm.flicker.dsl

import android.app.Instrumentation
import android.support.test.launcherhelper.ILauncherStrategy
import android.support.test.launcherhelper.LauncherStrategyFactory
import androidx.test.uiautomator.UiDevice
import com.android.server.wm.flicker.Flicker
import com.android.server.wm.flicker.FlickerDslMarker
import com.android.server.wm.flicker.TransitionRunner
import com.android.server.wm.flicker.monitor.EventLogMonitor
import com.android.server.wm.flicker.getDefaultFlickerOutputDir
import com.android.server.wm.flicker.monitor.ITransitionMonitor
import com.android.server.wm.flicker.monitor.LayersTraceMonitor
import com.android.server.wm.flicker.monitor.ScreenRecorder
import com.android.server.wm.flicker.monitor.WindowAnimationFrameStatsMonitor
import com.android.server.wm.flicker.monitor.WindowManagerTraceMonitor
import com.android.server.wm.traces.common.layers.LayersTrace
import com.android.server.wm.traces.common.layers.LayerTraceEntry
import com.android.server.wm.traces.common.windowmanager.WindowManagerState
import com.android.server.wm.traces.common.windowmanager.WindowManagerTrace
import com.android.server.wm.traces.parser.windowmanager.WindowManagerStateHelper
import java.nio.file.Path

/**
 * Build Flicker tests using Flicker DSL
 */
@FlickerDslMarker
class FlickerBuilder private constructor(
    private val instrumentation: Instrumentation,
    private val launcherStrategy: ILauncherStrategy,
    private val includeJankyRuns: Boolean,
    private val outputDir: Path,
    private val wmHelper: WindowManagerStateHelper,
    private var testName: String,
    private var iterations: Int,
    private val setupCommands: TestCommandsBuilder,
    private val teardownCommands: TestCommandsBuilder,
    private val transitionCommands: MutableList<Flicker.() -> Any>,
    val device: UiDevice,
    private val traceMonitors: MutableList<ITransitionMonitor>
) {
    private val frameStatsMonitor: WindowAnimationFrameStatsMonitor? = if (includeJankyRuns) {
        null
    } else {
        WindowAnimationFrameStatsMonitor(instrumentation)
    }

    @JvmOverloads
    /**
     * Default flicker builder constructor
     */
    constructor(
        /**
         * Instrumentation to run the tests
         */
        instrumentation: Instrumentation,
        /**
         * Strategy used to interact with the launcher
         */
        launcherStrategy: ILauncherStrategy = LauncherStrategyFactory
                .getInstance(instrumentation).launcherStrategy,
        /**
         * Include or discard janky runs
         */
        includeJankyRuns: Boolean = true,
        /**
         * Output directory for the test results
         */
        outputDir: Path = getDefaultFlickerOutputDir(),
        /**
         * Helper object for WM Synchronization
         */
        wmHelper: WindowManagerStateHelper = WindowManagerStateHelper(instrumentation)
    ) : this(
        instrumentation,
        launcherStrategy,
        includeJankyRuns,
        outputDir,
        wmHelper,
        testName = "",
        iterations = 1,
        setupCommands = TestCommandsBuilder(),
        teardownCommands = TestCommandsBuilder(),
        transitionCommands = mutableListOf(),
        device = UiDevice.getInstance(instrumentation),
        traceMonitors = mutableListOf<ITransitionMonitor>()
            .also {
                it.add(WindowManagerTraceMonitor(outputDir))
                it.add(LayersTraceMonitor(outputDir))
                it.add(ScreenRecorder(outputDir, instrumentation.targetContext))
                it.add(EventLogMonitor())
            }
    )

    /**
     * Copy constructor
     */
    constructor(otherBuilder: FlickerBuilder): this(
        otherBuilder.instrumentation,
        otherBuilder.launcherStrategy,
        otherBuilder.includeJankyRuns,
        otherBuilder.outputDir.toAbsolutePath(),
        otherBuilder.wmHelper,
        otherBuilder.testName,
        otherBuilder.iterations,
        TestCommandsBuilder(otherBuilder.setupCommands),
        TestCommandsBuilder(otherBuilder.teardownCommands),
        otherBuilder.transitionCommands.toMutableList(),
        UiDevice.getInstance(otherBuilder.instrumentation),
        otherBuilder.traceMonitors.toMutableList()
    )

    /**
     * Test name used to store the test results
     *
     * If reused throughout the test, only the last value is stored
     */
    fun withTestName(testName: () -> String) {
        val name = testName()
        require(!name.contains(" ")) {
            "The test tag can not contain spaces since it is a part of the file name"
        }
        this.testName = name
    }

    /**
     * Disable [WindowManagerTraceMonitor].
     */
    fun withoutWindowManagerTracing() {
        withWindowManagerTracing { null }
    }

    /**
     * Configure a [WindowManagerTraceMonitor] to obtain [WindowManagerTrace]
     *
     * By default the tracing is always active. To disable tracing return null
     *
     * If this tracing is disabled, the assertions for [WindowManagerTrace] and
     * [WindowManagerState] will not be executed
     */
    fun withWindowManagerTracing(traceMonitor: (Path) -> WindowManagerTraceMonitor?) {
        traceMonitors.removeIf { it is WindowManagerTraceMonitor }
        val newMonitor = traceMonitor(outputDir)

        if (newMonitor != null) {
            traceMonitors.add(newMonitor)
        }
    }

    /**
     * Disable [LayersTraceMonitor].
     */
    fun withoutLayerTracing() {
        withLayerTracing { null }
    }

    /**
     * Configure a [LayersTraceMonitor] to obtain [LayersTrace].
     *
     * By default the tracing is always active. To disable tracing return null
     *
     * If this tracing is disabled, the assertions for [LayersTrace] and [LayerTraceEntry]
     * will not be executed
     */
    fun withLayerTracing(traceMonitor: (Path) -> LayersTraceMonitor?) {
        traceMonitors.removeIf { it is LayersTraceMonitor }
        val newMonitor = traceMonitor(outputDir)

        if (newMonitor != null) {
            traceMonitors.add(newMonitor)
        }
    }

    /**
     * Configure a [ScreenRecorder].
     *
     * By default the tracing is always active. To disable tracing return null
     */
    fun withScreenRecorder(screenRecorder: (Path) -> ScreenRecorder?) {
        traceMonitors.removeIf { it is ScreenRecorder }
        val newMonitor = screenRecorder(outputDir)

        if (newMonitor != null) {
            traceMonitors.add(newMonitor)
        }
    }

    /**
     * Defines how many times the test run should be repeated
     */
    fun repeat(predicate: () -> Int) {
        val repeat = predicate()
        require(repeat >= 1) { "Number of repetitions should be greater or equal to 1" }
        iterations = repeat
    }

    /**
     * Defines the test ([TestCommandsBuilder.testCommands]) and run ([TestCommandsBuilder.runCommands])
     * commands executed before the [transitions] to test
     */
    fun setup(commands: TestCommandsBuilder.() -> Unit) {
        setupCommands.apply { commands() }
    }

    /**
     * Defines the test ([TestCommandsBuilder.testCommands]) and run ([TestCommandsBuilder.runCommands])
     * commands executed after the [transitions] to test
     */
    fun teardown(commands: TestCommandsBuilder.() -> Unit) {
        teardownCommands.apply { commands() }
    }

    /**
     * Defines the commands that trigger the behavior to test
     */
    fun transitions(command: Flicker.() -> Unit) {
        transitionCommands.add(command)
    }

    /**
     * Creates a new Flicker runner based on the current builder configuration
     */
    @JvmOverloads
    fun build(runner: TransitionRunner = TransitionRunner()) = Flicker(
        instrumentation,
        device,
        launcherStrategy,
        outputDir,
        testName,
        iterations,
        frameStatsMonitor,
        traceMonitors,
        setupCommands.buildTestCommands(),
        setupCommands.buildRunCommands(),
        teardownCommands.buildTestCommands(),
        teardownCommands.buildRunCommands(),
        transitionCommands,
        runner,
        wmHelper
    )

    /**
     * Returns a copy of the current builder with the changes of [block] applied
     */
    fun copy(block: FlickerBuilder.() -> Unit) = FlickerBuilder(this).apply(block)
}
